/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeAllocasOCL.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

bool MergeAllocasOCL::skipInstruction(llvm::Function& F, AllocationLivenessAnalyzer::LivenessData& LD)
{
    return false;
}

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(MergeAllocasOCL, "igc-ocl-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes - opencl version", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(MergeAllocasOCL, "igc-ocl-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes - opencl version", false, false)

char MergeAllocasOCL::ID = 0;

MergeAllocasOCL::MergeAllocasOCL() : MergeAllocas(ID)
{
    initializeMergeAllocasOCLPass(*PassRegistry::getPassRegistry());
}

namespace IGC
{
    Pass* createMergeAllocasOCL()
    {
        return new MergeAllocasOCL();
    }
}
