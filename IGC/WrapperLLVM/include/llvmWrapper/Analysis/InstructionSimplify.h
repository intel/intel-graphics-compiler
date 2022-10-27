/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
#define IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/InstructionSimplify.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR >= 15
    using llvm::simplifyInstruction;
#else
    inline llvm::Value* simplifyInstruction(
        llvm::Instruction* I, const llvm::SimplifyQuery& Q,
        llvm::OptimizationRemarkEmitter* ORE = nullptr) {
        return llvm::SimplifyInstruction(I, Q, ORE);
    }
#endif
}

#endif // IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
