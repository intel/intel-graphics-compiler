/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "common/IGCIRBuilder.h"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "BarrierControlFlowOptimization.hpp"
#include "visa_igc_common_header.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;
namespace IGC
{
/////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Optimize LSC fence (UGM/SLM/TGM) and barrier sync control flow
/// proposed optimization:
/// sync()            - all threads in group - assure all prior reads\writes are retired
/// if (thread0)      - pick the owning thread
///    flush()        - simd1 (per-thread-group)
/// sync()            - all threads in group - assure flush op is completed before releasing.
class BarrierControlFlowOptimization : public llvm::FunctionPass
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    BarrierControlFlowOptimization();

    ////////////////////////////////////////////////////////////////////////
    virtual bool runOnFunction(llvm::Function& F);

    ////////////////////////////////////////////////////////////////////////
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;

    ////////////////////////////////////////////////////////////////////////

private:
    ////////////////////////////////////////////////////////////////////////
    bool ProcessFunction();

    ////////////////////////////////////////////////////////////////////////
    void InvalidateMembers();

    ////////////////////////////////////////////////////////////////////////
    void CollectFenceBarrierInstructions();

    ////////////////////////////////////////////////////////////////////////
    bool OptimizeBarrierControlFlow();

    ////////////////////////////////////////////////////////////////////////
    void EraseOldInstructions();

    ////////////////////////////////////////////////////////////////////////
    static bool IsThreadBarrierOperation(const llvm::Instruction* pInst);

    ////////////////////////////////////////////////////////////////////////
    //static bool IsFenceOperation(const llvm::Instruction* pInst);

    std::vector<llvm::Instruction*> m_LscMemoryFences;
    std::vector<llvm::Instruction*> m_ThreadGroupBarriers;
    std::vector<llvm::Instruction*> m_OrderedFenceBarrierInstructions;
    std::vector<std::vector<llvm::Instruction*>> m_OrderedFenceBarrierInstructionsBlock;

    llvm::Function* m_CurrentFunction = nullptr;
};

static inline bool IsLscFenceOperation(const Instruction* pInst);
static inline LSC_SFID GetLscMem(const Instruction* pInst);
static inline LSC_SCOPE GetLscScope(const Instruction* pInst);
static inline LSC_FENCE_OP GetLscFenceOp(const Instruction* pInst);

char BarrierControlFlowOptimization::ID = 0;

////////////////////////////////////////////////////////////////////////////
BarrierControlFlowOptimization::BarrierControlFlowOptimization() :
    llvm::FunctionPass(ID)
{
    initializeBarrierControlFlowOptimizationPass(*PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
bool BarrierControlFlowOptimization::runOnFunction(llvm::Function& F)
{
    m_CurrentFunction = &F;

    InvalidateMembers();

    return ProcessFunction();
}

////////////////////////////////////////////////////////////////////////
bool BarrierControlFlowOptimization::IsThreadBarrierOperation(const llvm::Instruction* pInst)
{
    if (llvm::isa<llvm::GenIntrinsicInst>(pInst))
    {
        const llvm::GenIntrinsicInst* pGenIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInst);

        switch (pGenIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_threadgroupbarrier:
            return true;
        default:
            break;
        }
    }

    return false;
}

void BarrierControlFlowOptimization::CollectFenceBarrierInstructions()
{
    for (llvm::BasicBlock& basicBlock : *m_CurrentFunction)
    {
        m_OrderedFenceBarrierInstructions.clear();

        for (llvm::Instruction& inst : basicBlock)
        {
            if (IsLscFenceOperation(&inst) || IsThreadBarrierOperation(&inst))
            {
                m_OrderedFenceBarrierInstructions.push_back(&inst);
            }
            else if (m_OrderedFenceBarrierInstructions.size() > 0)
            {
                // if not lsc fence and threadbarrier, ends a set barrier instructions
                m_OrderedFenceBarrierInstructionsBlock.push_back(m_OrderedFenceBarrierInstructions);
                m_OrderedFenceBarrierInstructions.clear();
            }
        }
    }
}

bool BarrierControlFlowOptimization::OptimizeBarrierControlFlow()
{
    const CodeGenContext* const pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool bModified = false;
    Value* onThread = nullptr;

    // Optimize barrier control flow
    for (auto& B : m_OrderedFenceBarrierInstructionsBlock)
    {
        m_ThreadGroupBarriers.clear();
        m_LscMemoryFences.clear();

        if (IsLscFenceOperation(B[0]))
        {
            Instruction* nextInst = nullptr;

            for (auto* I : B)
            {
                nextInst = I;
                if (IsLscFenceOperation(I))
                {
                    // nextInst is pointing to another fence
                    m_LscMemoryFences.push_back(I);
                }
                else if (IsThreadBarrierOperation(I))
                {
                    m_ThreadGroupBarriers.push_back(I);
                }
            }

            // thread barrier could be not used, but lsc fence must exist
            // if so, found the pattern and stop the loop and return
            if (m_LscMemoryFences.size() >= 1)
            {
                IGCIRBuilder<> IRB(nextInst);

                llvm::Module* module = nextInst->getParent()->getParent()->getParent();

                // all barrier instructions from the blocks are exected on the same thread
                // get the owning thread ID once
                if (!onThread)
                {
                    Function* systemValueIntrinsic = GenISAIntrinsic::getDeclaration(
                        module,
                        GenISAIntrinsic::GenISA_DCL_SystemValue,
                        IRB.getInt32Ty());

                    Value* threadIDX = nullptr;

                    if (pContext->type == ShaderType::TASK_SHADER ||
                        pContext->type == ShaderType::MESH_SHADER)
                    {
                        threadIDX = IRB.CreateCall(systemValueIntrinsic,
                            IRB.getInt32(IGC::THREAD_ID_IN_GROUP_X),
                            VALUE_NAME("TID"));
                    }
                    else
                    {
                        Value* subgroupId = IRB.CreateCall(
                            systemValueIntrinsic,
                            IRB.getInt32(THREAD_ID_WITHIN_THREAD_GROUP));

                        Function* simdSizeIntrinsic = GenISAIntrinsic::getDeclaration(
                            module,
                            GenISAIntrinsic::GenISA_simdSize);

                        Value* simdSize = IRB.CreateZExtOrTrunc(
                            IRB.CreateCall(simdSizeIntrinsic),
                            IRB.getInt32Ty());

                        Function* simdLaneIdIntrinsic = GenISAIntrinsic::getDeclaration(
                            module,
                            GenISAIntrinsic::GenISA_simdLaneId);

                        Value* subgroupLocalInvocationId = IRB.CreateZExtOrTrunc(
                            IRB.CreateCall(simdLaneIdIntrinsic),
                            IRB.getInt32Ty());

                        threadIDX = IRB.CreateAdd(
                            IRB.CreateMul(subgroupId, simdSize),
                            subgroupLocalInvocationId,
                            VALUE_NAME("TID"));
                    }

                    onThread = IRB.CreateICmpEQ(threadIDX, IRB.getInt32(0));
                }

                // 1. All threads do fence, scope=local, flush=None
                for (auto* I : m_LscMemoryFences)
                {
                    if (GetLscMem(I) == LSC_SLM)
                    {
                        Function* FenceFn = GenISAIntrinsic::getDeclaration(
                            module,
                            GenISAIntrinsic::GenISA_LSCFence);

                        Value* Args[] = {
                            IRB.getInt32(LSC_SLM),
                            IRB.getInt32(LSC_SCOPE_GROUP),
                            IRB.getInt32(LSC_FENCE_OP_NONE)
                        };

                        IRB.CreateCall(FenceFn, Args);
                    }
                }

                // 2. All threads barrier (if in group?)
                if (m_ThreadGroupBarriers.size() > 0)
                {
                    Function* BarrierFn = GenISAIntrinsic::getDeclaration(
                        module,
                        GenISAIntrinsic::GenISA_threadgroupbarrier);

                    IRB.CreateCall(BarrierFn);
                }

                // 3. One thread designated to do a fence, scope=GPU, flush=evict
                // if (thread0) - pick the owning thread
                llvm::Instruction* br = nullptr;

                if (nextInst->getNextNode())
                {
                    br = SplitBlockAndInsertIfThen(onThread, nextInst->getNextNode(), false);
                    IRB.SetInsertPoint(&(*br->getParent()->begin()));
                }

                // create new lsc fence based on the fence mem
                for (auto* I : m_LscMemoryFences)
                {
                    LSC_SFID lscMem = GetLscMem(I);

                    if (lscMem != LSC_SLM)
                    {
                        Function* FenceFn = GenISAIntrinsic::getDeclaration(
                            module,
                            GenISAIntrinsic::GenISA_LSCFence);

                        Value* Args[] = {
                            IRB.getInt32(lscMem),
                            IRB.getInt32(LSC_SCOPE_GPU),
                            IRB.getInt32(LSC_FENCE_OP_EVICT)
                        };

                        IRB.CreateCall(FenceFn, Args);
                    }
                }

                // ends the if-then block
                if (br)
                {
                    IRB.SetInsertPoint(&(*br->getSuccessor(0)->begin()));
                }

                if (m_ThreadGroupBarriers.size() > 0)
                {
                    Function* BarrierFn = GenISAIntrinsic::getDeclaration(
                        module,
                        GenISAIntrinsic::GenISA_threadgroupbarrier);

                    IRB.CreateCall(BarrierFn);
                }

                bModified = true;
            }
        }
    }

    return bModified;
}

void BarrierControlFlowOptimization::EraseOldInstructions()
{
    for (auto& B : m_OrderedFenceBarrierInstructionsBlock)
    {
        for (llvm::Instruction* I : B)
        {
            I->eraseFromParent();
        }
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Processes an analysis which results in pointing out redundancies
/// among synchronization instructions appearing in the analyzed function.
bool BarrierControlFlowOptimization::ProcessFunction()
{
    const CodeGenContext* const pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    bool isDisabled =
        IsStage1FastestCompile(pContext->m_CgFlag, pContext->m_StagingCtx) ||
        IGC_GET_FLAG_VALUE(ForceFastestSIMD);
    if (isDisabled)
    {
        return false;
    }

    CollectFenceBarrierInstructions();

    bool bModified = OptimizeBarrierControlFlow();

    // if optimized, then erase old instructions
    if (bModified)
    {
        EraseOldInstructions();
    }

    return bModified;
}

////////////////////////////////////////////////////////////////////////
void BarrierControlFlowOptimization::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<CodeGenContextWrapper>();
}

////////////////////////////////////////////////////////////////////////
void BarrierControlFlowOptimization::InvalidateMembers()
{
    m_ThreadGroupBarriers.clear();
    m_LscMemoryFences.clear();
    m_OrderedFenceBarrierInstructions.clear();
    m_OrderedFenceBarrierInstructionsBlock.clear();
}

//////////////////////////////////////////////////////////////////////////
template<class... Ts> struct overloaded : Ts...
{
    template<typename T>
    overloaded<Ts...>& operator=(const T& lambda)
    {
        ((static_cast<Ts&>(*this) = lambda), ...);
        return *this;
    }

    using Ts::operator()...;
};
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

//////////////////////////////////////////////////////////////////////////
using ForwardIterationCallbackT = std::function<void(llvm::BasicBlock::const_iterator, llvm::BasicBlock::const_iterator)>;

//////////////////////////////////////////////////////////////////////////
using BackwardIterationCallbackT = std::function<void(llvm::BasicBlock::const_reverse_iterator, llvm::BasicBlock::const_reverse_iterator)>;

//////////////////////////////////////////////////////////////////////////
using IterationCallbackT = overloaded<ForwardIterationCallbackT, BackwardIterationCallbackT>;

//////////////////////////////////////////////////////////////////////////
using ForwardBoundaryCallbackT = std::function<llvm::BasicBlock::const_iterator(llvm::BasicBlock::const_iterator, llvm::BasicBlock::const_iterator)>;

//////////////////////////////////////////////////////////////////////////
using BackwardBoundaryCallbackT = std::function<llvm::BasicBlock::const_reverse_iterator(llvm::BasicBlock::const_reverse_iterator, llvm::BasicBlock::const_reverse_iterator)>;

//////////////////////////////////////////////////////////////////////////
using BoundaryCallbackT = overloaded<ForwardBoundaryCallbackT, BackwardBoundaryCallbackT>;

//////////////////////////////////////////////////////////////////////////
using ForwardProcessCallbackT = std::function<bool(llvm::BasicBlock::const_iterator, llvm::BasicBlock::const_iterator)>;

//////////////////////////////////////////////////////////////////////////
using BackwardProcessCallbackT = std::function<bool(llvm::BasicBlock::const_reverse_iterator, llvm::BasicBlock::const_reverse_iterator)>;

//////////////////////////////////////////////////////////////////////////
using ProcessCallbackT = overloaded<ForwardProcessCallbackT, BackwardProcessCallbackT>;

////////////////////////////////////////////////////////////////////////
/// @brief Scan over basic blocks using a custom function which
/// analyzes instructions between the beginning and ending iterator
/// and decides if the scan should be continued. The direction of the
/// scanning process is defined by the kind of used iterators.
/// @param workList holds a collection with the beginning points of searching
/// @param visitedBasicBlocks holds a collection of fully visited basic blocks
/// @param ProcessInstructions holds a function returning information
/// based on the beginning and ending iterators of the currently scanned
/// basic blocks if adjacent basic blocks also should be scanned
/// @tparam BasicBlockterator a basic block iterator type indicating the scan direction
template<typename BasicBlockterator>
static void SearchInstructions(
    std::list<BasicBlockterator>& workList,
    llvm::DenseSet<const llvm::BasicBlock*>& visitedBasicBlocks,
    std::function<bool(BasicBlockterator, BasicBlockterator)>& ProcessInstructions)
{
    auto GetBeginIt = [](const llvm::BasicBlock* pBasicBlock) -> BasicBlockterator
    {
        constexpr bool isForwardDirection = std::is_same_v<BasicBlockterator, llvm::BasicBlock::const_iterator>;
        if constexpr (isForwardDirection)
        {
            return pBasicBlock->begin();
        }
        else
        {
            return pBasicBlock->rbegin();
        }
    };

    auto GetEndIt = [](const llvm::BasicBlock* pBasicBlock) -> BasicBlockterator
    {
        constexpr bool isForwardDirection = std::is_same_v<BasicBlockterator, llvm::BasicBlock::const_iterator>;
        if constexpr (isForwardDirection)
        {
            return pBasicBlock->end();
        }
        else
        {
            return pBasicBlock->rend();
        }
    };

    auto GetSuccessors = [](const llvm::BasicBlock* pBasicBlock)
    {
        constexpr bool isForwardDirection = std::is_same_v<BasicBlockterator, llvm::BasicBlock::const_iterator>;
        if constexpr (isForwardDirection)
        {
            return llvm::successors(pBasicBlock);
        }
        else
        {
            return llvm::predecessors(pBasicBlock);
        }
    };

    for (const auto& it : workList)
    {
        const llvm::BasicBlock* pCurrentBasicBlock = it->getParent();
        // use the iterator only if it wasn't visited or restricted
        auto bbIt = visitedBasicBlocks.find(pCurrentBasicBlock);
        if (bbIt != visitedBasicBlocks.end())
        {
            continue;
        }
        else if (GetBeginIt(pCurrentBasicBlock) == it)
        {
            visitedBasicBlocks.insert(pCurrentBasicBlock);
        }
        BasicBlockterator end = GetEndIt(pCurrentBasicBlock);
        if (ProcessInstructions(it, end))
        {
            for (const llvm::BasicBlock* pSuccessor : GetSuccessors(pCurrentBasicBlock))
            {
                if (visitedBasicBlocks.find(pSuccessor) == visitedBasicBlocks.end())
                {
                    workList.push_back(GetBeginIt(pSuccessor));
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
/// @brief Scan over basic blocks using custom functions which
/// one of them analyzes instructions between the beginning and ending iterator
/// and second of them decides if the scan should be continued. The direction of the
/// scanning process is defined by the kind of used iterators.
/// @param workList holds a collection with the beginning points of searching
/// @param visitedBasicBlocks holds a collection of fully visited basic blocks
/// @param IterateOverMemoryInsts holds a function analyzing the content
/// between the beginning iterator and the boundary iterator.
/// @param GetBoundaryInst holds a function returning information
/// about the boundary of scanning if it is present in the analyzed basic
/// block, otherwise, it returns the ending iterator and it means the
/// scan process is proceeded.
/// based on the beginning and ending iterators of the currently scanned
/// basic blocks if adjacent basic blocks also should be scanned
/// @tparam BasicBlockterator a basic block iterator type indicating the scan direction
template<typename BasicBlockterator>
static void SearchInstructions(
    std::list<BasicBlockterator>& workList,
    llvm::DenseSet<const llvm::BasicBlock*>& visitedBasicBlocks,
    std::function<void(BasicBlockterator, BasicBlockterator)>& IterateOverMemoryInsts,
    std::function<BasicBlockterator(BasicBlockterator, BasicBlockterator)>& GetBoundaryInst)
{
    std::function<bool(BasicBlockterator, BasicBlockterator)> ProcessInstructions{};
    ProcessInstructions = [&GetBoundaryInst, &IterateOverMemoryInsts](auto it, auto end)
    {
        auto beg = it;
        it = GetBoundaryInst(beg, end);
        IterateOverMemoryInsts(beg, it);
        return it == end;
    };
    SearchInstructions(workList, visitedBasicBlocks, ProcessInstructions);
}

////////////////////////////////////////////////////////////////////////////////
static inline bool IsLscFenceOperation(const Instruction* pInst)
{
    const GenIntrinsicInst* pIntr = dyn_cast<GenIntrinsicInst>(pInst);
    if (pIntr && pIntr->isGenIntrinsic(GenISAIntrinsic::GenISA_LSCFence))
    {
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_SFID GetLscMem(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_SFID>(pInst->getOperand(0));
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_SCOPE GetLscScope(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_SCOPE>(pInst->getOperand(1));
}

////////////////////////////////////////////////////////////////////////////////
static inline LSC_FENCE_OP GetLscFenceOp(const Instruction* pInst)
{
    IGC_ASSERT(IsLscFenceOperation(pInst));
    return getImmValueEnum<LSC_FENCE_OP>(pInst->getOperand(2));
}

////////////////////////////////////////////////////////////////////////
llvm::Pass* createBarrierControlFlowOptimization()
{
    return new BarrierControlFlowOptimization();
}

}

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-barrier-control-flow-optimization"
#define PASS_DESCRIPTION "BarrierControlFlowOptimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BarrierControlFlowOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(BarrierControlFlowOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
