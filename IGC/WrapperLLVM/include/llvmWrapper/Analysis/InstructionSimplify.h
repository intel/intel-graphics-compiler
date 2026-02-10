/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
#define IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include "IGC/common/LLVMWarningsPop.hpp"

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

inline llvm::Value *simplifyCall(llvm::CallBase *CI, const llvm::SimplifyQuery &Q) {
#if LLVM_VERSION_MAJOR <= 14
  return llvm::SimplifyCall(CI, Q);
#elif LLVM_VERSION_MAJOR <= 16
  return llvm::simplifyCall(CI, Q);
#else
  // Based on:
  // https://github.com/llvm/llvm-project/commit/3f23c7f5bedc8786d3f4567d2331a7efcbb2a77e
  llvm::SmallVector<llvm::Value *, 4> Args;
  Args.reserve(CI->arg_size());
  for (llvm::Value *Op : CI->args())
    Args.push_back(Op);

  return llvm::simplifyCall(CI, CI->getCalledOperand(), Args, Q.getWithInstruction(CI));
#endif
}
} // namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_INSTRUCTIONSIMPLIFY_H
