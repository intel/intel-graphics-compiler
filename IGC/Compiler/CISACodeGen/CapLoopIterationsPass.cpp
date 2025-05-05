/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// The purpose of this pass is to add a limit for each loop,
// so no infinite loops occur.
//===----------------------------------------------------------------------===//


#include "CapLoopIterationsPass.h"
#include "IGCIRBuilder.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "cap-loop-iterations-pass"
#define PASS_DESCRIPTION "Limit the number of iterations in each loop to UINT_MAX. Prevents inifinite loops"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CapLoopIterations, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(CapLoopIterations, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CapLoopIterations::ID = 0;

CapLoopIterations::CapLoopIterations() : FunctionPass(ID), m_iterationLimit(UINT_MAX)
{
    initializeCapLoopIterationsPass(*PassRegistry::getPassRegistry());
}

CapLoopIterations::CapLoopIterations(uint32_t iterationLimit) : FunctionPass(ID), m_iterationLimit(iterationLimit)
{
    initializeCapLoopIterationsPass(*PassRegistry::getPassRegistry());
}

void CapLoopIterations::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<LoopInfoWrapperPass>();
}

bool CapLoopIterations::runOnFunction(Function& F)
{
    auto changed = false;
    auto& loopInfo = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    auto IRB = IRBuilder(F.getContext());

    for (auto loop : loopInfo.getLoopsInPreorder())
    {
        if (!loop->isLoopSimplifyForm()) // preheader is required
            continue;

        IRB.SetInsertPoint(loop->getHeader()->getFirstNonPHI());

        auto counterphi = IRB.CreatePHI(IRB.getInt32Ty(), 2, "counterphi");
        counterphi->addIncoming(IRB.getInt32(0), loop->getLoopPreheader());

        auto counter = IRB.CreateAdd(counterphi, IRB.getInt32(1), "counter");
        counterphi->addIncoming(counter, loop->getLoopLatch());

        auto forceexit = IRB.CreateICmpEQ(counter, IRB.getInt32(m_iterationLimit), "forceloopexit");

        llvm::SmallVector<std::pair<BasicBlock*, BasicBlock*>, 4> exitedges;

        loop->getExitEdges(exitedges);

        for (const auto& [exitingbb, exitbb] : exitedges)
        {
            if (auto br = dyn_cast<BranchInst>(exitingbb->getTerminator()))
            {
                if (!br->isConditional())
                    continue;

                IRB.SetInsertPoint(br);

                if (br->getSuccessor(0) == exitbb) // br i1 %cond, %exit, %notexit
                {
                    br->setCondition(
                        IRB.CreateOr(forceexit, br->getCondition())
                    );
                }
                else // br i1 %cond, %notexit, %exit
                {
                    br->setCondition(
                        IRB.CreateAnd(IRB.CreateNot(forceexit), br->getCondition())
                    );
                }
            }
        }

        changed = true;
    }
    return changed;
}
