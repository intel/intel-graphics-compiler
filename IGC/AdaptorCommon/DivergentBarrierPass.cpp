/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Some applications may contain barrier()s in divergent control flow even
/// though they should be workgroup uniform. Here, we split a kernel into
/// a series of continuations broken up a barrier() points. Once all work-items
/// finish their continuation, there will be barriers placed outside of this
/// for the workgroup to rendezvous. Each work-item will walk through its
/// state machine of continuation calls until completion.
///
//===----------------------------------------------------------------------===//

#include "common/StringMacros.hpp"
#include "DivergentBarrierPass.h"
#include "ImplicitArgs.hpp"
#include "RayTracing/CrossingAnalysis.h"
#include "RayTracing/SplitAsyncUtils.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char DivergentBarrierPass::ID = 0;

void DivergentBarrierPass::updateFenceArgs(
    const GenIntrinsicInst* I, FenceArgs& Args) const
{
    IGC_ASSERT(I->getIntrinsicID() == GenISAIntrinsic::GenISA_memoryfence);
    auto update = [](const Value* V)
    {
        // If we don't know at compile-time, conservatively assume true
        if (auto* C = dyn_cast<ConstantInt>(V))
            return C->getValue().getBoolValue();
        else
            return true;
    };

    // Always CommitEnable

    Args.L3_Flush_RW_Data       |= update(I->getOperand(1));
    Args.L3_Flush_Constant_Data |= update(I->getOperand(2));
    Args.L3_Flush_Texture_Data  |= update(I->getOperand(3));
    Args.L3_Flush_Instructions  |= update(I->getOperand(4));
    Args.Global                 |= update(I->getOperand(5));
    Args.L1_Invalidate          |= update(I->getOperand(6));
}

CallInst* DivergentBarrierPass::insertFence(
    IRBuilder<>& IRB, const FenceArgs& FA) const
{
    Function* FenceFn = GenISAIntrinsic::getDeclaration(
        IRB.GetInsertBlock()->getModule(),
        GenISAIntrinsic::GenISA_memoryfence);
    Value* Args[] = {
        IRB.getInt1(FA.CommitEnable),
        IRB.getInt1(FA.L3_Flush_RW_Data),
        IRB.getInt1(FA.L3_Flush_Constant_Data),
        IRB.getInt1(FA.L3_Flush_Texture_Data),
        IRB.getInt1(FA.L3_Flush_Instructions),
        IRB.getInt1(FA.Global),
        IRB.getInt1(FA.L1_Invalidate),
    };
    return IRB.CreateCall(FenceFn, Args);
}

bool DivergentBarrierPass::hasDivergentBarrier(
    const std::vector<Instruction*>& Barriers) const
{
    if (Barriers.empty())
        return false;

    auto* ModMD = m_CGCtx->getModuleMetaData();
    auto* F = Barriers[0]->getFunction();

    PostDominatorTree PDT(*F);
    DominatorTree DT(*F);

    TranslationTable TT;
    TT.run(*F);
    WIAnalysisRunner WI(F, &DT, &PDT, m_MDUtils, m_CGCtx, ModMD, &TT);
    WI.run();

    return llvm::any_of(Barriers, [&](Instruction* I) {
        return WI.insideWorkgroupDivergentCF(I);
    });
}

Function* DivergentBarrierPass::createContinuation(BasicBlock* EntryBB)
{
    auto* F = EntryBB->getParent();

    std::vector<Type*> ArgTypes;

    for (const Argument& I : F->args())
        ArgTypes.push_back(I.getType());

    IRBuilder<> IRB(EntryBB->getContext());
    FunctionType* FTy = FunctionType::get(
        IRB.getInt32Ty(), // change the 'void' to 'i32' for the continuation ID
        ArgTypes,
        F->getFunctionType()->isVarArg());

    Function* NewFunc = Function::Create(
        FTy, F->getLinkage(), F->getAddressSpace(),
        F->getName(), F->getParent());

    ValueToValueMapTy VMap;

    Function::arg_iterator DestI = NewFunc->arg_begin();
    for (const Argument& I : F->args())
    {
        DestI->setName(I.getName());
        VMap[&I] = &*DestI++;
    }

    SmallVector<ReturnInst*, 8> Returns;
    IGCLLVM::CloneFunctionInto(NewFunc, F, VMap, F->getSubprogram() != nullptr, Returns);

    auto* ContEntryBB = cast<BasicBlock>(VMap.find(EntryBB)->second);
    ContEntryBB->moveBefore(&NewFunc->getEntryBlock());

    llvm::removeUnreachableBlocks(*NewFunc);

    return NewFunc;
}

Value* DivergentBarrierPass::getGroupSize(Function& Wrapper) const
{
    IRBuilder<> IRB(Wrapper.getContext());
    if (m_CGCtx->type == ShaderType::OPENCL_SHADER)
    {
        ImplicitArgs Args{ Wrapper, m_CGCtx->getMetaDataUtils() };
        auto *Arg = Args.getImplicitArg(Wrapper, ImplicitArg::LOCAL_SIZE);
        IGC_ASSERT_MESSAGE(Arg, "Should have been already added!");
        auto *VTy = cast<IGCLLVM::FixedVectorType>(Arg->getType());
        uint64_t NumElts = VTy->getNumElements();
        IRB.SetInsertPoint(&Wrapper.getEntryBlock().front());
        Value* Size = IRB.CreateExtractElement(Arg, (uint64_t)0);
        for (uint64_t i = 1; i < NumElts; i++)
        {
            auto* Elt = IRB.CreateExtractElement(Arg, i);
            Size = IRB.CreateMul(Size, Elt);
        }
        Size = IRB.CreateIntCast(Size, IRB.getInt32Ty(), false);
        Size->setName(VALUE_NAME("totalGroupSize"));
        return Size;
    }
    else if (m_CGCtx->type == ShaderType::COMPUTE_SHADER)
    {
        auto& M = *Wrapper.getParent();
        GlobalVariable* pGlobal = M.getGlobalVariable("ThreadGroupSize_X");
        unsigned xDim =
            int_cast<unsigned>(cast<ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = M.getGlobalVariable("ThreadGroupSize_Y");
        unsigned yDim =
            int_cast<unsigned>(cast<ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = M.getGlobalVariable("ThreadGroupSize_Z");
        unsigned zDim =
            int_cast<unsigned>(cast<ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        return IRB.getInt32(xDim * yDim * zDim);
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "unhandled shader type!");
        return nullptr;
    }
}

Value* DivergentBarrierPass::allocateSLM(IRBuilder<> &IRB)
{
    if (m_CGCtx->type == ShaderType::OPENCL_SHADER)
    {
        auto* M = IRB.GetInsertBlock()->getModule();
        auto *GV = new GlobalVariable(
            *M,
            IRB.getInt32Ty(),
            false,
            GlobalVariable::InternalLinkage,
            UndefValue::get(IRB.getInt32Ty()),
            "__ThreadDoneCnt",
            nullptr,
            GlobalValue::NotThreadLocal,
            ADDRESS_SPACE_LOCAL);
        return GV;
    }
    else if (m_CGCtx->type == ShaderType::COMPUTE_SHADER)
    {
        IGC_ASSERT(Ctx);
        auto& TGSMSize = *reinterpret_cast<unsigned*>(Ctx);
        // Ideally, we would lower SLM later in the compiler so we don't have to
        // update this value after the fact.
        TGSMSize = iSTD::Align(TGSMSize, 4);
        uint32_t Offset = TGSMSize;
        TGSMSize += 4;

        auto* ThreadDoneCntPtr = IRB.CreateIntToPtr(
            IRB.getInt32(Offset),
            PointerType::get(IRB.getInt32Ty(), ADDRESS_SPACE_LOCAL));

        return ThreadDoneCntPtr;
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "unhandled shader type!");
        return nullptr;
    }
}

void DivergentBarrierPass::generateBody(
    Function* Wrapper,
    Function* Entry,
    const std::vector<Function*>& Continuations,
    const FenceArgs& FA)
{
    // We want the wrapper code to look something like:
    /*
    void shader()
    {
        // EntryBB
        local uint thread_done_cnt = 0;
        uint next = entry();
        barrier();
        uint done = 0;
        while (true) // HeaderBB
        {
            switch (next) // SwitchBB
            {
            case 0:
                next = continuation_0();
                break;
            case 1:
                next = continuation_1();
                break;
            default:
                break;
            }

            if (next == -1 && !done) // SwitchJoinBB
                atomic_inc(&thread_done_cnt);
            done = (next == -1); // AtomicJoinBB

            barrier();

            if (thread_done_cnt == get_local_size())
                break;

            barrier(); // BackEdgeBB
        }
        // ExitBB
    }
    */

    /*
    local uint thread_done_cnt = 0;
    uint next = entry();
    barrier();
    uint done = 0;
    */
    auto& C = Wrapper->getContext();
    Function* BarrierFn = GenISAIntrinsic::getDeclaration(
        Wrapper->getParent(),
        GenISAIntrinsic::GenISA_threadgroupbarrier);

    auto* EntryBB = BasicBlock::Create(C, VALUE_NAME("entry"), Wrapper);
    auto* HeaderBB = BasicBlock::Create(C, VALUE_NAME("header"), Wrapper);
    auto* SwitchBB = BasicBlock::Create(C, VALUE_NAME("switch"), Wrapper);

    std::vector<CallInst*> ContCalls;
    IRBuilder<> IRB(EntryBB);
    auto* DonePtr = IRB.CreateAlloca(IRB.getInt1Ty(), nullptr, VALUE_NAME("&done"));
    auto* NextPtr = IRB.CreateAlloca(IRB.getInt32Ty(), nullptr, VALUE_NAME("&next"));

    Value* ThreadDoneCntPtr = allocateSLM(IRB);
    IRB.CreateStore(IRB.getInt32(0), ThreadDoneCntPtr);

    SmallVector<Value*, 4> Args;
    for (auto& Arg : Wrapper->args())
        Args.push_back(&Arg);
    auto* EntryCI = IRB.CreateCall(Entry, Args, VALUE_NAME("next"));
    IRB.CreateStore(EntryCI, NextPtr);
    ContCalls.push_back(EntryCI);
    insertFence(IRB, FA);
    IRB.CreateCall(BarrierFn);
    IRB.CreateStore(IRB.getFalse(), DonePtr);
    IRB.CreateBr(HeaderBB);

    IRB.SetInsertPoint(HeaderBB);
    IRB.CreateBr(SwitchBB);
    IRB.SetInsertPoint(SwitchBB);
    auto* NextID = IRB.CreateLoad(IRB.getInt32Ty(), NextPtr, VALUE_NAME("next"));
    auto* SwitchJoinBB = BasicBlock::Create(C, VALUE_NAME("switch.end"), Wrapper);
    auto* DefaultBB = BasicBlock::Create(
        C, VALUE_NAME("default"), Wrapper, SwitchJoinBB);
    auto *Switch = IRB.CreateSwitch(NextID, DefaultBB, Continuations.size());
    IRB.SetInsertPoint(DefaultBB);
    IRB.CreateBr(SwitchJoinBB);
    /*
    switch (next) // SwitchBB
    {
    case 0:
        next = continuation_0();
        break;
    case 1:
        next = continuation_1();
        break;
    default:
        break;
    }
    */
    for (uint32_t i = 0; i < Continuations.size(); i++)
    {
        auto* CurContBB = BasicBlock::Create(
            C, VALUE_NAME("case" + Twine(i)), Wrapper, DefaultBB);
        Switch->addCase(IRB.getInt32(i), CurContBB);
        auto* CurCont = Continuations[i];
        IRB.SetInsertPoint(CurContBB);
        auto* ContCI = IRB.CreateCall(CurCont, Args, VALUE_NAME("next"));
        ContCalls.push_back(ContCI);
        IRB.CreateStore(ContCI, NextPtr);
        IRB.CreateBr(SwitchJoinBB);
    }

    /*
    if (next == -1 && !done) // SwitchJoinBB
        atomic_inc(&thread_done_cnt);
    */
    auto* AtomicJoinBB = BasicBlock::Create(C, VALUE_NAME("atomic.join"), Wrapper);
    {
        auto* AtomicIncBB = BasicBlock::Create(
            C, VALUE_NAME("do.atomic.inc"), Wrapper, AtomicJoinBB);

        IRB.SetInsertPoint(SwitchJoinBB);
        auto* NextID  = IRB.CreateLoad(IRB.getInt32Ty(), NextPtr, VALUE_NAME("next"));
        auto* Done    = IRB.CreateLoad(IRB.getInt1Ty(), DonePtr, VALUE_NAME("done"));
        auto* Cmp     = IRB.CreateICmpEQ(NextID, IRB.getInt32(-1));
        auto* NotDone = IRB.CreateXor(Done, IRB.getTrue());
        auto* Cond    = IRB.CreateAnd(Cmp, NotDone);
        IRB.CreateCondBr(Cond, AtomicIncBB, AtomicJoinBB);

        IRB.SetInsertPoint(AtomicIncBB);
        Type* Tys[] = { IRB.getInt32Ty(), ThreadDoneCntPtr->getType() };
        Function* AtomicFn = GenISAIntrinsic::getDeclaration(
            Wrapper->getParent(),
            GenISAIntrinsic::GenISA_intatomicraw,
            Tys);
        Value* Args[] = {
            ThreadDoneCntPtr,
            IRB.CreatePtrToInt(ThreadDoneCntPtr, IRB.getInt32Ty()),
            IRB.getInt32(1),
            IRB.getInt32(EATOMIC_IADD)
        };
        IRB.CreateCall(AtomicFn, Args);
        IRB.CreateBr(AtomicJoinBB);
    }
    /*
        done = (next == -1); // AtomicJoinBB

        barrier();

        if (thread_done_cnt == get_local_size())
            break;
    */
    auto* ExitBB = BasicBlock::Create(C, VALUE_NAME("exit"), Wrapper);
    {
        IRB.SetInsertPoint(AtomicJoinBB);
        auto* NextID  = IRB.CreateLoad(IRB.getInt32Ty(), NextPtr, VALUE_NAME("next"));
        auto* NewDoneVal = IRB.CreateICmpEQ(NextID, IRB.getInt32(-1));
        IRB.CreateStore(NewDoneVal, DonePtr);

        insertFence(IRB, FA);
        IRB.CreateCall(BarrierFn);

        auto* ThreadDoneCnt = IRB.CreateLoad(
            ThreadDoneCntPtr->getType()->getPointerElementType(),
            ThreadDoneCntPtr, VALUE_NAME("thread.done.cnt"));

        Value* GroupSize = getGroupSize(*Wrapper);
        auto *BreakCond =
            IRB.CreateICmpEQ(ThreadDoneCnt, GroupSize);

        auto* BackEdgeBB = BasicBlock::Create(
            C, VALUE_NAME("backedge"), Wrapper, ExitBB);

        IRB.CreateCondBr(BreakCond, ExitBB, BackEdgeBB);

        IRB.SetInsertPoint(BackEdgeBB);
        IRB.CreateCall(BarrierFn);
        IRB.CreateBr(HeaderBB);
    }

    IRB.SetInsertPoint(ExitBB);
    IRB.CreateRetVoid();

    // Now inline all the continuations so we can reassemble SSA.
    for (auto* CI : ContCalls)
    {
        InlineFunctionInfo IFI;
        bool CanInline = IGCLLVM::InlineFunction(CI, IFI);
        IGC_ASSERT_MESSAGE(CanInline, "failed to inline?");
    }

    for (auto* Cont : Continuations)
        Cont->eraseFromParent();
    Entry->eraseFromParent();
}

void DivergentBarrierPass::handleSpillFill(Function* F) const
{
    DenseMap<uint64_t, AllocaInst*> IdxToAlloca;

    IRBuilder<> IRB(F->getContext());

    for (auto II = inst_begin(F), E = inst_end(F); II != E; /* empty */)
    {
        auto* I = &*II++;
        if (auto* SI = dyn_cast<SpillValueIntrinsic>(I))
        {
            uint64_t Offset = SI->getOffset();
            Value* Ptr = nullptr;
            if (auto It = IdxToAlloca.find(Offset); It != IdxToAlloca.end())
            {
                Ptr = It->second;
            }
            else
            {
                IRB.SetInsertPoint(&*F->getEntryBlock().getFirstInsertionPt());
                auto *AI = IRB.CreateAlloca(SI->getData()->getType());
                IdxToAlloca[Offset] = AI;
                Ptr = AI;
            }
            IRB.SetInsertPoint(SI);
            IRB.CreateStore(SI->getData(), Ptr);
            SI->eraseFromParent();
        }
        else if (auto* FI = dyn_cast<FillValueIntrinsic>(I))
        {
            uint64_t Offset = FI->getOffset();
            Value* Ptr = nullptr;
            if (auto It = IdxToAlloca.find(Offset); It != IdxToAlloca.end())
            {
                Ptr = It->second;
            }
            else
            {
                IRB.SetInsertPoint(&*F->getEntryBlock().getFirstInsertionPt());
                auto *AI = IRB.CreateAlloca(FI->getType());
                IdxToAlloca[Offset] = AI;
                Ptr = AI;
            }
            IRB.SetInsertPoint(FI);
            auto *LI = IRB.CreateLoad(
                Ptr->getType()->getPointerElementType(), Ptr);
            LI->takeName(FI);
            FI->replaceAllUsesWith(LI);
            FI->eraseFromParent();
        }
    }
}

bool DivergentBarrierPass::processShader(Function* F)
{
    std::vector<Instruction*> Barriers;
    std::vector<GenIntrinsicInst*> Fences;
    for (auto& I : instructions(*F))
    {
        if (auto* GII = dyn_cast<GenIntrinsicInst>(&I))
        {
            switch (GII->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_threadgroupbarrier:
                Barriers.push_back(GII);
                break;
            case GenISAIntrinsic::GenISA_memoryfence:
                Fences.push_back(GII);
                break;
            default:
                break;
            }
        }
    }

    if (!hasDivergentBarrier(Barriers))
        return false;

    FenceArgs FA;
    for (auto* F : Fences)
    {
        updateFenceArgs(F, FA);
        // TODO: should we split on memoryfence as well if a barrier does not
        // immediately follow it?
        if (auto* GII = dyn_cast<GenIntrinsicInst>(F->getNextNode()); GII &&
            GII->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier)
        {
            F->eraseFromParent();
        }
    }

    // We found at least one barrier that isn't in thread uniform control flow.
    // Time to split into continuations.

    for (auto* B : Barriers)
    {
        splitAround(B, VALUE_NAME("Cont"));
    }

    rewritePHIs(*F);

    SuspendCrossingInfo Checker(*F, Barriers);

    SmallVector<Spill, 8> Spills;
    for (Instruction& I : instructions(*F))
    {
        for (User* U : I.users())
        {
            if (Checker.isDefinitionAcrossSuspend(I, U))
                Spills.emplace_back(&I, U);
        }
    }

    insertSpills(m_CGCtx, *F, Spills);

    /// build continuations
    IRBuilder<> IRB(F->getContext());
    for (auto& BB : *F)
    {
        // First, mark all current returns with a -1 to indicate that this
        // is the end of the shader
        if (auto* RI = dyn_cast<ReturnInst>(BB.getTerminator()))
        {
            RI->eraseFromParent();
            IRB.SetInsertPoint(&BB);
            IRB.CreateRet(IRB.getInt32(-1));
        }
    }

    std::vector<BasicBlock*> NextBlocks;
    uint32_t CurContID = 0;
    for (auto* B : Barriers)
    {
        auto* CurBlock = B->getParent();
        NextBlocks.push_back(CurBlock->getUniqueSuccessor());
        CurBlock->getTerminator()->eraseFromParent();
        IRB.SetInsertPoint(CurBlock);
        IRB.CreateRet(IRB.getInt32(CurContID++));
        // remove barriers in continuations so we can place them in convergent
        // control flow in the outer loop.
        B->eraseFromParent();
    }

    std::vector<Function*> Continuations;
    for (auto* EntryBB : NextBlocks)
    {
        Function *Continuation = createContinuation(EntryBB);
        Continuations.push_back(Continuation);
    }

    Function* Entry = createContinuation(&F->getEntryBlock());

    // Wipe out the old function
    std::vector<Type*> ArgTypes;
    for (const Argument& I : F->args())
        ArgTypes.push_back(I.getType());

    FunctionType* FTy = FunctionType::get(F->getFunctionType()->getReturnType(),
        ArgTypes, F->getFunctionType()->isVarArg());

    Function* Wrapper = Function::Create(
        FTy, F->getLinkage(), F->getAddressSpace(), F->getName(), F->getParent());

    Function::arg_iterator DestI = Wrapper->arg_begin();
    for (const Argument& I : F->args())
    {
        DestI->setName(I.getName());
        DestI++;
    }

    Wrapper->takeName(F);
    IGCMD::IGCMetaDataHelper::moveFunction(
        *m_MDUtils, *m_CGCtx->getModuleMetaData(), F, Wrapper);
    F->eraseFromParent();

    generateBody(Wrapper, Entry, Continuations, FA);
    handleSpillFill(Wrapper);

    return true;
}

bool DivergentBarrierPass::runOnModule(Module& M)
{
    bool Changed = false;
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    SmallVector<Function*, 4> Shaders;
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        if (!isEntryFunc(m_MDUtils, &F))
            continue;

        Shaders.push_back(&F);
    }

    for (auto* F : Shaders)
    {
        Changed |= processShader(F);
    }

    IGC_ASSERT(false == llvm::verifyModule(*m_CGCtx->getModule(), &dbgs()));

    return Changed;
}

namespace IGC {

#define PASS_FLAG "divergent-barrier-pass"
#define PASS_DESCRIPTION "Splits shader into continuations to make barriers uniform"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DivergentBarrierPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(DivergentBarrierPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

ModulePass* createDivergentBarrierPass(void* Ctx)
{
    return new DivergentBarrierPass(Ctx);
}

} // namespace IGC
