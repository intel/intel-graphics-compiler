/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_MATHEXTRAS_H
#define IGCLLVM_SUPPORT_MATHEXTRAS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MathExtras.h"

namespace IGCLLVM {
using llvm::MulOverflow;
using llvm::SubOverflow;

inline uint64_t bit_floor(uint64_t A) {
#if LLVM_VERSION_MAJOR > 16
  return llvm::bit_floor(A);
#else
  return llvm::PowerOf2Floor(A);
#endif
}

template <typename T> unsigned countr_zero(T Val) {
  static_assert(std::is_unsigned_v<T>, "Only unsigned integral types are allowed.");
#if LLVM_VERSION_MAJOR > 16
  return llvm::countr_zero(Val);
#else
  return llvm::countTrailingZeros(Val);
#endif
}

template <typename T> unsigned countl_zero(T Val) {
  static_assert(std::is_unsigned_v<T>, "Only unsigned integral types are allowed.");
#if LLVM_VERSION_MAJOR > 16
  return llvm::countl_zero(Val);
#else
  return llvm::countLeadingZeros(Val);
#endif
}
} // namespace IGCLLVM

#endif