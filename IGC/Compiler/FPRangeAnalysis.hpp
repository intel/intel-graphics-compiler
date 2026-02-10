/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FPRangeAnalysis.hpp - Floating Point Range Tracking Utilities
//
// This file provides utilities for computing floating-point value ranges,
// similar to how ValueTracking.h provides integer value tracking utilities
// like computeKnownBits().
//
// FPRange is a lightweight interval type [Lower, Upper] over APFloat.
//
//   TODO(LLVM-22): Consider migrating to llvm::ConstantFPRange
//     Note: ConstantFPRange has a different API:
//       - No getNonNaN() static; use constructor ConstantFPRange(Lo, Hi, QNaN, SNaN)
//       - Arithmetic (add/sub/mul/div) are free functions, not member methods
//       - NaN tracking is split: containsQNaN()/containsSNaN() vs containsNaN()
//       - unionWith() is a free function unionOf()
//     A wrapper or adapter may be needed rather than direct substitution.
//
// Usage:
//   FPRange Range = computeFPRange(SomeFloatValue);
//
// For repeated queries in the same context, pass a cache:
//   FPRangeCache Cache;
//   FPRange R1 = computeFPRange(V1, &Cache);
//   FPRange R2 = computeFPRange(V2, &Cache);
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Value.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

//===----------------------------------------------------------------------===//
// FPRange - Floating-point interval [Lower, Upper] with NaN tracking
//
// This is a simplified interval type for IGC's range analysis needs.
// See header comment for notes on potential LLVM 22 migration.
//===----------------------------------------------------------------------===//

class FPRange {
public:
  //===------------------------------------------------------------------===//
  // Constructors
  //===------------------------------------------------------------------===//

  /// Default: full set (unknown range)
  FPRange()
      : Lower(llvm::APFloat::getLargest(llvm::APFloat::IEEEsingle(), true)),
        Upper(llvm::APFloat::getLargest(llvm::APFloat::IEEEsingle(), false)), MayBeNaN(true), Full(true), Empty(false) {
  }

  /// Construct from a single APFloat value (point range)
  explicit FPRange(const llvm::APFloat &Val)
      : Lower(Val), Upper(Val), MayBeNaN(Val.isNaN()), Full(false), Empty(false) {}

  //===------------------------------------------------------------------===//
  // Named constructors (match ConstantFPRange statics)
  //===------------------------------------------------------------------===//

  /// Full set: any value including NaN
  static FPRange getFull(const llvm::fltSemantics &Sem) {
    FPRange R;
    R.Lower = llvm::APFloat::getLargest(Sem, true);
    R.Upper = llvm::APFloat::getLargest(Sem, false);
    R.MayBeNaN = true;
    R.Full = true;
    R.Empty = false;
    return R;
  }

  /// Empty set: no values
  static FPRange getEmpty(const llvm::fltSemantics &Sem) {
    FPRange R;
    R.Lower = llvm::APFloat::getZero(Sem);
    R.Upper = llvm::APFloat::getZero(Sem);
    R.MayBeNaN = false;
    R.Full = false;
    R.Empty = true;
    return R;
  }

  /// Non-NaN range [Lo, Hi]
  static FPRange getNonNaN(llvm::APFloat Lo, llvm::APFloat Hi) {
    FPRange R;
    R.Lower = std::move(Lo);
    R.Upper = std::move(Hi);
    R.MayBeNaN = false;
    R.Full = false;
    R.Empty = false;
    return R;
  }

  //===------------------------------------------------------------------===//
  // Predicates (match ConstantFPRange)
  //===------------------------------------------------------------------===//

  bool isFullSet() const { return Full; }
  bool isEmptySet() const { return Empty; }
  bool containsNaN() const { return MayBeNaN; }
  bool isNaNOnly() const { return MayBeNaN && !Full && Empty; }

  //===------------------------------------------------------------------===//
  // Accessors (match ConstantFPRange)
  //===------------------------------------------------------------------===//

  const llvm::APFloat &getLower() const { return Lower; }
  const llvm::APFloat &getUpper() const { return Upper; }

  //===------------------------------------------------------------------===//
  // Interval arithmetic (match ConstantFPRange method signatures)
  //===------------------------------------------------------------------===//

  FPRange add(const FPRange &Other) const {
    if (isFullSet() || Other.isFullSet())
      return getFull(sem());
    llvm::APFloat Lo = Lower;
    llvm::APFloat Hi = Upper;
    Lo.add(Other.Lower, llvm::APFloat::rmTowardNegative);
    Hi.add(Other.Upper, llvm::APFloat::rmTowardPositive);
    return makeRangeChecked(Lo, Hi, sem());
  }

  FPRange sub(const FPRange &Other) const {
    if (isFullSet() || Other.isFullSet())
      return getFull(sem());
    llvm::APFloat Lo = Lower;
    llvm::APFloat Hi = Upper;
    Lo.subtract(Other.Upper, llvm::APFloat::rmTowardNegative);
    Hi.subtract(Other.Lower, llvm::APFloat::rmTowardPositive);
    return makeRangeChecked(Lo, Hi, sem());
  }

  FPRange mul(const FPRange &Other) const {
    if (isFullSet() || Other.isFullSet())
      return getFull(sem());

    llvm::APFloat products[4] = {Lower, Lower, Upper, Upper};
    products[0].multiply(Other.Lower, llvm::APFloat::rmTowardNegative);
    products[1].multiply(Other.Upper, llvm::APFloat::rmTowardNegative);
    products[2].multiply(Other.Lower, llvm::APFloat::rmTowardNegative);
    products[3].multiply(Other.Upper, llvm::APFloat::rmTowardNegative);

    llvm::APFloat productsHi[4] = {Lower, Lower, Upper, Upper};
    productsHi[0].multiply(Other.Lower, llvm::APFloat::rmTowardPositive);
    productsHi[1].multiply(Other.Upper, llvm::APFloat::rmTowardPositive);
    productsHi[2].multiply(Other.Lower, llvm::APFloat::rmTowardPositive);
    productsHi[3].multiply(Other.Upper, llvm::APFloat::rmTowardPositive);

    return makeRangeChecked(minOf(products, 4), maxOf(productsHi, 4), sem());
  }

  FPRange div(const FPRange &Other) const {
    if (isFullSet() || Other.isFullSet())
      return getFull(sem());

    llvm::APFloat Zero = llvm::APFloat::getZero(sem());
    if (Other.Lower.compare(Zero) != llvm::APFloat::cmpGreaterThan &&
        Other.Upper.compare(Zero) != llvm::APFloat::cmpLessThan)
      return getFull(sem());

    llvm::APFloat quotients[4] = {Lower, Lower, Upper, Upper};
    quotients[0].divide(Other.Lower, llvm::APFloat::rmTowardNegative);
    quotients[1].divide(Other.Upper, llvm::APFloat::rmTowardNegative);
    quotients[2].divide(Other.Lower, llvm::APFloat::rmTowardNegative);
    quotients[3].divide(Other.Upper, llvm::APFloat::rmTowardNegative);

    llvm::APFloat quotientsHi[4] = {Lower, Lower, Upper, Upper};
    quotientsHi[0].divide(Other.Lower, llvm::APFloat::rmTowardPositive);
    quotientsHi[1].divide(Other.Upper, llvm::APFloat::rmTowardPositive);
    quotientsHi[2].divide(Other.Lower, llvm::APFloat::rmTowardPositive);
    quotientsHi[3].divide(Other.Upper, llvm::APFloat::rmTowardPositive);

    return makeRangeChecked(minOf(quotients, 4), maxOf(quotientsHi, 4), sem());
  }

  FPRange abs() const {
    if (isFullSet())
      return getFull(sem());

    llvm::APFloat Zero = llvm::APFloat::getZero(sem());

    if (Lower.compare(Zero) != llvm::APFloat::cmpLessThan) {
      return getNonNaN(Lower, Upper);
    }
    if (Upper.compare(Zero) != llvm::APFloat::cmpGreaterThan) {
      llvm::APFloat Lo = Upper;
      llvm::APFloat Hi = Lower;
      Lo.changeSign();
      Hi.changeSign();
      return getNonNaN(std::move(Lo), std::move(Hi));
    }
    llvm::APFloat NegLower = Lower;
    NegLower.changeSign();
    llvm::APFloat Hi = (NegLower.compare(Upper) == llvm::APFloat::cmpGreaterThan) ? NegLower : Upper;
    return getNonNaN(Zero, std::move(Hi));
  }

  FPRange negate() const {
    if (isFullSet())
      return getFull(sem());
    llvm::APFloat Lo = Upper;
    llvm::APFloat Hi = Lower;
    Lo.changeSign();
    Hi.changeSign();
    return getNonNaN(std::move(Lo), std::move(Hi));
  }

  FPRange unionWith(const FPRange &Other) const {
    if (isFullSet() || Other.isFullSet())
      return getFull(sem());
    if (isEmptySet())
      return Other;
    if (Other.isEmptySet())
      return *this;

    llvm::APFloat Lo = (Lower.compare(Other.Lower) == llvm::APFloat::cmpLessThan) ? Lower : Other.Lower;
    llvm::APFloat Hi = (Upper.compare(Other.Upper) == llvm::APFloat::cmpGreaterThan) ? Upper : Other.Upper;
    FPRange R = getNonNaN(std::move(Lo), std::move(Hi));
    R.MayBeNaN = MayBeNaN || Other.MayBeNaN;
    return R;
  }

private:
  llvm::APFloat Lower;
  llvm::APFloat Upper;
  bool MayBeNaN;
  bool Full;
  bool Empty;

  const llvm::fltSemantics &sem() const { return Lower.getSemantics(); }

  static llvm::APFloat minOf(const llvm::APFloat *arr, size_t n) {
    llvm::APFloat result = arr[0];
    for (size_t i = 1; i < n; ++i) {
      if (arr[i].compare(result) == llvm::APFloat::cmpLessThan)
        result = arr[i];
    }
    return result;
  }

  static llvm::APFloat maxOf(const llvm::APFloat *arr, size_t n) {
    llvm::APFloat result = arr[0];
    for (size_t i = 1; i < n; ++i) {
      if (arr[i].compare(result) == llvm::APFloat::cmpGreaterThan)
        result = arr[i];
    }
    return result;
  }

  static FPRange makeRangeChecked(const llvm::APFloat &Lo, const llvm::APFloat &Hi, const llvm::fltSemantics &Sem) {
    if (!Lo.isFinite() || !Hi.isFinite())
      return getFull(Sem);
    return getNonNaN(Lo, Hi);
  }
};

//===----------------------------------------------------------------------===//
// FPRangeCache - Optional cache for repeated queries
//===----------------------------------------------------------------------===//

using FPRangeCache = llvm::DenseMap<llvm::Value *, FPRange>;

//===----------------------------------------------------------------------===//
// Main Entry Point
//===----------------------------------------------------------------------===//

/// Compute the floating-point range of a value.
FPRange computeFPRange(llvm::Value *V, FPRangeCache *Cache = nullptr, unsigned Depth = 0);

/// Check if the entire range is within [lo, hi].
bool isWithin(const FPRange &Range, double lo, double hi);

/// Check if the entire range is within [-bound, +bound].
inline bool isWithinSymmetric(const FPRange &Range, double bound) { return isWithin(Range, -bound, bound); }

/// Get the maximum absolute value in the range.
double maxAbsValue(const FPRange &Range);

/// Create an FPRange from double bounds.
FPRange makeRange(double lo, double hi, const llvm::fltSemantics &Sem = llvm::APFloat::IEEEsingle());

} // namespace IGC
