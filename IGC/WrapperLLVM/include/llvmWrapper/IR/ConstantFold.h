/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANT_FOLD_H
#define IGCLLVM_IR_CONSTANT_FOLD_H

#include "IGC/common/LLVMWarningsPush.hpp"
#if (LLVM_VERSION_MAJOR >= 15)
#include "llvm/IR/ConstantFold.h"
#endif
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::Constant *ConstantFoldExtractValueInstruction(llvm::Constant *Agg, llvm::ArrayRef<unsigned> Idxs,
                                                           llvm::Type *OnlyIfReducedTy = nullptr) {
#if (LLVM_VERSION_MAJOR < 15)
  return llvm::ConstantExpr::getExtractValue(Agg, Idxs, OnlyIfReducedTy);
#else
  return llvm::ConstantFoldExtractValueInstruction(Agg, Idxs);
#endif
}

inline llvm::Constant *ConstantFoldInsertValueInstruction(llvm::Constant *Agg, llvm::Constant *Val,
                                                          llvm::ArrayRef<unsigned> Idxs,
                                                          llvm::Type *OnlyIfReducedTy = nullptr) {
#if (LLVM_VERSION_MAJOR < 15)
  return llvm::ConstantExpr::getInsertValue(Agg, Val, Idxs, OnlyIfReducedTy);
#else
  return llvm::ConstantFoldInsertValueInstruction(Agg, Val, Idxs);
#endif
}

inline llvm::Constant *ConstantFoldBinaryInstruction(unsigned Opcode, llvm::Constant *V1, llvm::Constant *V2) {
#if (LLVM_VERSION_MAJOR < 15)
  // TODO: Add other opcodes as needed
  switch (Opcode) {
  case llvm::Instruction::SDiv:
    return llvm::ConstantExpr::getSDiv(V1, V2);
    break;
  case llvm::Instruction::UDiv:
    return llvm::ConstantExpr::getUDiv(V1, V2);
    break;
  default:
    llvm_unreachable("Unhandled binary inst opcode");
  }
#else
  return llvm::ConstantFoldBinaryInstruction(Opcode, V1, V2);
#endif
}
} // namespace IGCLLVM

#endif // IGCLLVM_IR_CONSTANT_FOLD_H
