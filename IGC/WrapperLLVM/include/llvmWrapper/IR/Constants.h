/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTS_H
#define IGCLLVM_IR_CONSTANTS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Support/TypeSize.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"
#if LLVM_VERSION_MAJOR >= 22
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/DataLayout.h"
#endif
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::ElementCount getElementCount(const llvm::ConstantAggregateZero &C) { return C.getElementCount(); }

namespace ConstantExpr {
inline llvm::Constant *getShuffleVector(llvm::Constant *V1, llvm::Constant *V2, uint64_t Mask,
                                        llvm::Type *OnlyIfReducedTy = nullptr) {
  return llvm::ConstantExpr::getShuffleVector(V1, V2, static_cast<int>(Mask), OnlyIfReducedTy);
}
inline llvm::Constant *getZExt(llvm::Constant *C, llvm::Type *Ty, bool OnlyIfReduced = false) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::ConstantExpr::getZExt(C, Ty, OnlyIfReduced);
#else
  // LLVM 22 no longer supports ZExt as a constant expression; fold it.
  llvm::DataLayout DL("");
  return llvm::ConstantFoldCastOperand(llvm::Instruction::ZExt, C, Ty, DL);
#endif
}
inline llvm::Constant *getSExt(llvm::Constant *C, llvm::Type *Ty, bool OnlyIfReduced = false) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::ConstantExpr::getSExt(C, Ty, OnlyIfReduced);
#else
  llvm::DataLayout DL("");
  return llvm::ConstantFoldCastOperand(llvm::Instruction::SExt, C, Ty, DL);
#endif
}
inline llvm::Constant *getUIToFP(llvm::Constant *C, llvm::Type *Ty, bool OnlyIfReduced = false) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::ConstantExpr::getUIToFP(C, Ty, OnlyIfReduced);
#else
  llvm::DataLayout DL("");
  return llvm::ConstantFoldCastOperand(llvm::Instruction::UIToFP, C, Ty, DL);
#endif
}
} // namespace ConstantExpr

namespace ConstantFixedVector {
inline llvm::Constant *getSplatValue(llvm::ConstantVector *CV, bool AllowUndefs = false) {
  return CV->getSplatValue(AllowUndefs);
}
inline llvm::Constant *getSplat(unsigned NumElements, llvm::Constant *V) {
  return llvm::ConstantVector::getSplat(llvm::ElementCount::getFixed(NumElements), V);
}
} // namespace ConstantFixedVector

namespace Constant {
inline llvm::Constant *getSplatValue(llvm::Constant *C, bool AllowUndefs = false) {
  return C->getSplatValue(AllowUndefs);
}

inline bool isNullValue(const llvm::Constant *C) {
#if LLVM_VERSION_MAJOR >= 23
  return C->isNullValue();
#else
  return C->isZeroValue();
#endif
}
} // namespace Constant

namespace PoisonValue {
inline llvm::PoisonValue *get(llvm::Type *T) { return llvm::PoisonValue::get(T); }
} // namespace PoisonValue
} // namespace IGCLLVM

#endif
