/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_APINT_H
#define IGCLLVM_ADT_APINT_H

#include "Probe/Assertion.h"
#include <llvm/ADT/APInt.h>

#include "llvm/Support/DivisionByConstantInfo.h"

namespace IGCLLVM {
using SignedDivisionByConstantInfo = llvm::SignedDivisionByConstantInfo;
using UnsignedDivisionByConstantInfo =
#if (LLVM_VERSION_MAJOR == 14) || defined(IGC_LLVM_TRUNK_REVISION)
    // Account for a typo
    llvm::UnsignedDivisonByConstantInfo;
#else // LLVM_VERSION_MAJOR == 14
    llvm::UnsignedDivisionByConstantInfo;
#endif

inline SignedDivisionByConstantInfo getAPIntMagic(const llvm::APInt &value) {
  return llvm::SignedDivisionByConstantInfo::get(value);
}

inline UnsignedDivisionByConstantInfo getAPIntMagicUnsigned(const llvm::APInt &value, const unsigned LeadingZeros = 0) {
#if LLVM_VERSION_MAJOR >= 16
  // Basing on this: [https://reviews.llvm.org/D140924]
  return UnsignedDivisionByConstantInfo::get(value, LeadingZeros, false);
#else
  return UnsignedDivisionByConstantInfo::get(value, LeadingZeros);
#endif
}

inline bool IsAddition(const UnsignedDivisionByConstantInfo &mu) {
  return mu.IsAdd;
}

inline unsigned ShiftAmount(const UnsignedDivisionByConstantInfo &mu) {
#if LLVM_VERSION_MAJOR >= 16
  // Basing on this: https://reviews.llvm.org/D141014
  return IsAddition(mu) ? mu.PostShift + 1 : mu.PostShift;
#else
  return mu.ShiftAmount;
#endif
}

inline unsigned ShiftAmount(const SignedDivisionByConstantInfo &ms) {
  return ms.ShiftAmount;
}

inline llvm::APInt MagicNumber(const UnsignedDivisionByConstantInfo &mu) {
  return mu.Magic;
}

inline llvm::APInt MagicNumber(const SignedDivisionByConstantInfo &ms) {
  return ms.Magic;
}
} // namespace IGCLLVM

#endif