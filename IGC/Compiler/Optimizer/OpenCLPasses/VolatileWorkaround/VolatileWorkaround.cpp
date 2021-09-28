/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/VolatileWorkaround/VolatileWorkaround.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-wa-volatile"
#define PASS_DESCRIPTION "Volatile Workaround"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(VolatileWorkaround, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(VolatileWorkaround, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char VolatileWorkaround::ID = 0;

VolatileWorkaround::VolatileWorkaround() : FunctionPass(ID)
{
    initializeVolatileWorkaroundPass(*PassRegistry::getPassRegistry());
}

bool VolatileWorkaround::runOnFunction(Function& F)
{
    visit(F);
    return true;
}


void VolatileWorkaround::visitLoadInst(LoadInst& I)
{
    I.setVolatile(false);
}

void VolatileWorkaround::visitStoreInst(StoreInst& I)
{
    I.setVolatile(false);
}
