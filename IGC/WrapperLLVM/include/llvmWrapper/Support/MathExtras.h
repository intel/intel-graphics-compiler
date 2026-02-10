/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_MATHEXTRAS_H
#define IGCLLVM_SUPPORT_MATHEXTRAS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MathExtras.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
using llvm::MulOverflow;
using llvm::SubOverflow;

inline uint64_t bit_floor(uint64_t A) {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return llvm::bit_floor(A);
#else
  return llvm::PowerOf2Floor(A);
#endif
}

template <typename T> unsigned countr_zero(T Val) {
  static_assert(std::is_unsigned_v<T>, "Only unsigned integral types are allowed.");
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return llvm::countr_zero(Val);
#else
  return llvm::countTrailingZeros(Val);
#endif
}

template <typename T> unsigned countl_zero(T Val) {
  static_assert(std::is_unsigned_v<T>, "Only unsigned integral types are allowed.");
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return llvm::countl_zero(Val);
#else
  return llvm::countLeadingZeros(Val);
#endif
}

template <typename T> unsigned countl_one(T Val) {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return llvm::countl_one(Val);
#else
  return llvm::countLeadingOnes(Val);
#endif
}
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)

enum ZeroBehavior {
  /// The returned value is undefined.
  ZB_Undefined,
  /// The returned value is numeric_limits<T>::max()
  ZB_Max
};
#endif

#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
template <typename T> T findFirstSet(T Val, IGCLLVM::ZeroBehavior ZB = IGCLLVM::ZB_Max) {
  if (ZB == IGCLLVM::ZB_Max && Val == 0)
    return std::numeric_limits<T>::max();
  return llvm::countr_zero(Val);
#else
template <typename T> T findFirstSet(T Val, llvm::ZeroBehavior ZB = llvm::ZB_Max) {
  return llvm::findFirstSet(Val, ZB);
#endif
}

#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
template <typename T> T findLastSet(T Val, IGCLLVM::ZeroBehavior ZB = IGCLLVM::ZB_Max) {
  if (ZB == IGCLLVM::ZB_Max && Val == 0)
    return std::numeric_limits<T>::max();
  return llvm::countl_zero(Val) ^ (std::numeric_limits<T>::digits - 1);
#else
template <typename T> T findLastSet(T Val, llvm::ZeroBehavior ZB = llvm::ZB_Max) {
  return llvm::findLastSet(Val, ZB);
#endif
}

template <typename T> inline unsigned popcount(T Value) {
  static_assert(std::is_unsigned_v<T>, "Only unsigned integral types are allowed.");
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return (unsigned)llvm::popcount(Value);
#else
  return llvm::countPopulation(Value);
#endif
}

} // namespace IGCLLVM

#endif
