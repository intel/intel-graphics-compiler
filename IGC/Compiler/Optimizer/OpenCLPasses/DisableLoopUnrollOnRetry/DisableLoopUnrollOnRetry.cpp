/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/DisableLoopUnrollOnRetry/DisableLoopUnrollOnRetry.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-disable-loop-unroll"
#define PASS_DESCRIPTION "Disable loop unroll on retry"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DisableLoopUnrollOnRetry, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(DisableLoopUnrollOnRetry, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DisableLoopUnrollOnRetry::ID = 0;

DisableLoopUnrollOnRetry::DisableLoopUnrollOnRetry()
    : LoopPass(ID)
{
    initializeDisableLoopUnrollOnRetryPass(*PassRegistry::getPassRegistry());
}

bool DisableLoopUnrollOnRetry::runOnLoop(Loop* L, LPPassManager& LPM)
{
    bool changed = false;
    if (MDNode * LoopID = L->getLoopID())
    {
        if (!(GetUnrollMetadata(LoopID, "llvm.loop.unroll.enable") ||
            GetUnrollMetadata(LoopID, "llvm.loop.unroll.full") ||
            GetUnrollMetadata(LoopID, "llvm.loop.unroll.count")))
        {
            L->setLoopAlreadyUnrolled(); //This sets loop to llvm.loop.unroll.disable
            changed = true;
        }
    }
    else
    {
        L->setLoopAlreadyUnrolled(); //This sets loop to llvm.loop.unroll.disable
        changed = true;
    }
    return changed;
}
