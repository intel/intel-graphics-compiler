/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"
#include "Compiler/IGCPassSupport.h"
#include "SynchronizationObjectCoalescing.hpp"

namespace IGC
{
char SynchronizationObjectCoalescing::ID = 0;

////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescing::SynchronizationObjectCoalescing() :
    FunctionPass(ID)
{
    initializeSynchronizationObjectCoalescingPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
SynchronizationObjectCoalescing::~SynchronizationObjectCoalescing()
{
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescing::runOnFunction(llvm::Function& F)
{
    m_RepeatedBarrierCalls.clear();
    for (llvm::Function::iterator BBIt = F.begin(); BBIt != F.end(); ++BBIt)
    {
        Visit(BBIt->begin(), BBIt->end());
    }
    bool isModified = m_RepeatedBarrierCalls.size() > 0;
    for (llvm::Instruction* I : m_RepeatedBarrierCalls)
    {
        I->eraseFromParent();
    }
    return isModified;
}


////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescing::Visit(llvm::BasicBlock::iterator beg, llvm::BasicBlock::iterator end)
{
    for (llvm::BasicBlock::iterator it = beg; it != end; ++it)
    {
        const llvm::GenIntrinsicInst* genIntrinsicInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(it);

        if (genIntrinsicInst == nullptr)
        {
            continue;
        }

        switch (genIntrinsicInst->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_memoryfence:
            VisitFence(it, end);
            break;
        case llvm::GenISAIntrinsic::GenISA_threadgroupbarrier:
            VisitThreadGroupBarrier(it, end);
            break;
        default:
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescing::VisitFence(llvm::BasicBlock::iterator& it, llvm::BasicBlock::iterator end)
{
    std::vector<llvm::GenIntrinsicInst*> uniqueMemoryFences;
    llvm::BasicBlock::iterator lastUsedIterator = it;
    while (it != end)
    {
        llvm::GenIntrinsicInst* genIntrinsicInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(it);
        if (!genIntrinsicInst)
        {
            break;
        }

        if (genIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_threadgroupbarrier)
        {
            // If fence instructions are interleaved with barrier instructions, skip barrier instruction.
            lastUsedIterator = it++;
        }
        else if (genIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_memoryfence)
        {
            // Merge candidates: two or more consecutive fence instructions,
            // possibly interleaved with a barrier instruction.
            lastUsedIterator = it++;
            if (CompareAndMergeMemoryFenceWithContainerContent(genIntrinsicInst, uniqueMemoryFences))
            {
                m_RepeatedBarrierCalls.push_back(genIntrinsicInst);
            }
            else
            {
                uniqueMemoryFences.push_back(genIntrinsicInst);
            }
        }
        else
        {
            break;
        }
    }
    it = lastUsedIterator;
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescing::VisitThreadGroupBarrier(llvm::BasicBlock::iterator & it, llvm::BasicBlock::iterator end)
{
    IGC_ASSERT(llvm::isa<llvm::GenIntrinsicInst>(it) &&
        llvm::dyn_cast<llvm::GenIntrinsicInst>(it)->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_threadgroupbarrier);
    llvm::BasicBlock::iterator lastUsedIterator = it++;
    while (it != end)
    {
        llvm::GenIntrinsicInst* genIntrinsicInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(it);
        if (genIntrinsicInst)
        {
            if (genIntrinsicInst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_threadgroupbarrier)
            {
                lastUsedIterator = it++;
                m_RepeatedBarrierCalls.push_back(genIntrinsicInst);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    it = lastUsedIterator;
}

////////////////////////////////////////////////////////////////////////
bool SynchronizationObjectCoalescing::CompareAndMergeMemoryFenceWithContainerContent(
    const llvm::GenIntrinsicInst* inst,
    const std::vector<llvm::GenIntrinsicInst*>& container)
{
    const CodeGenContext* const ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    const uint32_t commitEnableArg = 0;
    const uint32_t L3FlushRWDataArg = 1;
    const uint32_t L3FlushConstantDataArg = 2;
    const uint32_t L3FlushTextureDataArg = 3;
    const uint32_t L3FlushInstructionsArg = 4;
    const uint32_t globalMemFenceArg = 5;
    const uint32_t L1CacheInvalidateArg = 6;
    IGC_ASSERT(inst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_memoryfence);

    for (llvm::GenIntrinsicInst* memoryFenceInst : container)
    {
        bool isDuplicate = llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(commitEnableArg))->getValue().getBoolValue() ==
            llvm::cast<llvm::ConstantInt>(inst->getOperand(commitEnableArg))->getValue().getBoolValue();
        isDuplicate &= llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(L3FlushRWDataArg))->getValue().getBoolValue() ==
            llvm::cast<llvm::ConstantInt>(inst->getOperand(L3FlushRWDataArg))->getValue().getBoolValue();
        isDuplicate &= llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(L3FlushConstantDataArg))->getValue().getBoolValue() ==
            llvm::cast<llvm::ConstantInt>(inst->getOperand(L3FlushConstantDataArg))->getValue().getBoolValue();
        isDuplicate &= llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(L3FlushTextureDataArg))->getValue().getBoolValue() ==
            llvm::cast<llvm::ConstantInt>(inst->getOperand(L3FlushTextureDataArg))->getValue().getBoolValue();
        isDuplicate &= llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(L3FlushInstructionsArg))->getValue().getBoolValue() ==
            llvm::cast<llvm::ConstantInt>(inst->getOperand(L3FlushInstructionsArg))->getValue().getBoolValue();
        if (ctx->platform.hasSLMFence() && isDuplicate)
        {
            isDuplicate &= llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(globalMemFenceArg))->getValue().getBoolValue() ==
                llvm::cast<llvm::ConstantInt>(inst->getOperand(globalMemFenceArg))->getValue().getBoolValue();
        }
        if (isDuplicate)
        {
            // Changes values of memory fences operands to most extended
            bool L1CacheInvalidateArgValue = llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(L1CacheInvalidateArg))->getValue().getBoolValue() ||
                llvm::cast<llvm::ConstantInt>(inst->getOperand(L1CacheInvalidateArg))->getValue().getBoolValue();
            memoryFenceInst->setOperand(L1CacheInvalidateArg, llvm::ConstantInt::get(inst->getOperand(L1CacheInvalidateArg)->getType(), L1CacheInvalidateArgValue ? 1 : 0));
            if (ctx->platform.hasSLMFence() == false)
            {
                bool globalMemFenceArgValue = llvm::cast<llvm::ConstantInt>(memoryFenceInst->getOperand(globalMemFenceArg))->getValue().getBoolValue() ||
                    llvm::cast<llvm::ConstantInt>(inst->getOperand(globalMemFenceArg))->getValue().getBoolValue();
                memoryFenceInst->setOperand(L1CacheInvalidateArg, llvm::ConstantInt::get(inst->getOperand(globalMemFenceArg)->getType(), globalMemFenceArgValue ? 1 : 0));
            }
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////
void SynchronizationObjectCoalescing::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addPreserved<CodeGenContextWrapper>();
}

}

using namespace llvm;
using namespace IGC;
#define PASS_FLAG "igc-synchronization-object-coalescing"
#define PASS_DESCRIPTION "SynchronizationObjectCoalescing"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SynchronizationObjectCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SynchronizationObjectCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
