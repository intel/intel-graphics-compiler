/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
#define IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/InstructionSimplify.h"

namespace IGCLLVM {
#if (LLVM_VERSION_MAJOR < 15)
inline llvm::Value *simplifyInstruction(llvm::Instruction *I, const llvm::SimplifyQuery &Q,
                                        llvm::OptimizationRemarkEmitter *ORE = nullptr) {
  return llvm::SimplifyInstruction(I, Q, ORE);
}
#else
using llvm::simplifyInstruction;
#endif

#if (LLVM_VERSION_MAJOR < 15)
inline llvm::Value *simplifyBinOp(unsigned Opcode, llvm::Value *LHS, llvm::Value *RHS, const llvm::SimplifyQuery &Q) {
  return llvm::SimplifyBinOp(Opcode, LHS, RHS, Q);
}
#else
using llvm::simplifyBinOp;
#endif

#if (LLVM_VERSION_MAJOR < 15)
inline llvm::Value *simplifyCall(llvm::CallBase *Call, const llvm::SimplifyQuery &Q) {
  return llvm::SimplifyCall(Call, Q);
}
#else
using llvm::simplifyCall;
#endif
} // namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
