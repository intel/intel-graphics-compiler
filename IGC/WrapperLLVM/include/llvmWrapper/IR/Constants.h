/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTS_H
#define IGCLLVM_IR_CONSTANTS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"

#include "llvm/Support/TypeSize.h"

namespace IGCLLVM {
inline llvm::ElementCount getElementCount(const llvm::ConstantAggregateZero &C) {
  return C.getElementCount();
}

namespace ConstantExpr {
inline llvm::Constant *getShuffleVector(llvm::Constant *V1, llvm::Constant *V2, uint64_t Mask,
                                        llvm::Type *OnlyIfReducedTy = nullptr) {
  return llvm::ConstantExpr::getShuffleVector(V1, V2, static_cast<int>(Mask), OnlyIfReducedTy);
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
} // namespace Constant

namespace PoisonValue {
inline llvm::PoisonValue *get(llvm::Type *T) { return llvm::PoisonValue::get(T); }
} // namespace PoisonValue
} // namespace IGCLLVM

#endif
