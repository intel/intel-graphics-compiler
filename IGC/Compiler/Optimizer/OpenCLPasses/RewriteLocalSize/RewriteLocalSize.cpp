/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// As per the OCL 2.0 spec for get_enqueued_local_size:
//
// "Returns the same value as that returned by get_local_size(dimindx) if
//  the kernel is executed
//  with a uniform work-group size."
//
// This pass is only invoked when -cl-uniform-work-group-size is present.
// In that case, get_local_size(x) == get_enqueued_local_size(x).
//
// So we will rewrite all of the get_local_size(x) so that we only have
// get_enqueued_local_size(x).  Those calls may further be folded for kernels
// that utilize __attribute__((reqd_work_group_size(X,Y,Z))).
//

#include "Compiler/Optimizer/OpenCLPasses/RewriteLocalSize/RewriteLocalSize.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-rewrite-local-size"
#define PASS_DESCRIPTION "converts get_local_size() to get_enqueued_local_size()"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RewriteLocalSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RewriteLocalSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RewriteLocalSize::ID = 0;

RewriteLocalSize::RewriteLocalSize() : ModulePass(ID)
{
    initializeRewriteLocalSizePass(*PassRegistry::getPassRegistry());
}

bool RewriteLocalSize::runOnModule(Module& M)
{
    Function* LS = M.getFunction(WIFuncsAnalysis::GET_LOCAL_SIZE);
    if (!LS)
        return false;

    Function* ELS = M.getFunction(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE);
    if (!ELS)
        LS->setName(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE);
    else
        LS->replaceAllUsesWith(ELS);

    return true;
}
