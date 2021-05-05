/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_MATHEXTRAS_H
#define IGCLLVM_SUPPORT_MATHEXTRAS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MathExtras.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR < 10

// NOTE: Implementation of SubOverflow and MulOverflow is taken from llvm 10
// from llvm/Support/MathExtras.h.

/// Subtract two signed integers, computing the two's complement truncated
/// result, returning true if an overflow ocurred.
template <typename T>
typename std::enable_if<std::is_signed<T>::value, T>::type
SubOverflow(T X, T Y, T &Result) {
  // Perform the unsigned addition.
  using U = typename std::make_unsigned<T>::type;
  const U UX = static_cast<U>(X);
  const U UY = static_cast<U>(Y);
  const U UResult = UX - UY;

  // Convert to signed.
  Result = static_cast<T>(UResult);

  // Subtracting a positive number from a negative results in a negative number.
  if (X <= 0 && Y > 0)
    return Result >= 0;
  // Subtracting a negative number from a positive results in a positive number.
  if (X >= 0 && Y < 0)
    return Result <= 0;
  return false;
}


/// Multiply two signed integers, computing the two's complement truncated
/// result, returning true if an overflow ocurred.
template <typename T>
typename std::enable_if<std::is_signed<T>::value, T>::type
MulOverflow(T X, T Y, T &Result) {
  // Perform the unsigned multiplication on absolute values.
  using U = typename std::make_unsigned<T>::type;
  const U UX = X < 0 ? (0 - static_cast<U>(X)) : static_cast<U>(X);
  const U UY = Y < 0 ? (0 - static_cast<U>(Y)) : static_cast<U>(Y);
  const U UResult = UX * UY;

  // Convert to signed.
  const bool IsNegative = (X < 0) ^ (Y < 0);
  Result = IsNegative ? (0 - UResult) : UResult;

  // If any of the args was 0, result is 0 and no overflow occurs.
  if (UX == 0 || UY == 0)
    return false;

  // UX and UY are in [1, 2^n], where n is the number of digits.
  // Check how the max allowed absolute value (2^n for negative, 2^(n-1) for
  // positive) divided by an argument compares to the other.
  if (IsNegative)
    return UX > (static_cast<U>(std::numeric_limits<T>::max()) + U(1)) / UY;
  else
    return UX > (static_cast<U>(std::numeric_limits<T>::max())) / UY;
}

#else

using llvm::SubOverflow;
using llvm::MulOverflow;

#endif
}

#endif
