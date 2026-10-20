/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FPRangeAnalysis.hpp - Floating Point Range Tracking Utilities
//
// Utilities for computing floating-point value ranges, similar to how ValueTracking.h provides computeKnownBits() for
// integer values.
//
//   FPRange Range = computeFPRange(SomeFloatValue);
//   if (Range.isBounded() && isWithin(Range, 0.0, 1.0)) { ... }
//
// For repeated queries, pass a cache:
//
//   FPRangeCache Cache;
//   FPRange R1 = computeFPRange(V1, &Cache);
//   FPRange R2 = computeFPRange(V2, &Cache);
//
//===----------------------------------------------------------------------===//
// TODO(LLVM-22): Migrate to llvm::ConstantFPRange. The API maps nearly one to one:
//   getFull(Sem).withNaN(false) -> getNonNaN(Sem)          getEmpty(Sem).withNaN(true) -> getNaNOnly(Sem, true, true)
//   getPoint(V)                 -> ConstantFPRange(V)       getRange(Lo, Hi) -> getNonNaN(Lo, Hi), then flushDenormals
//   MayBeNaN -> containsNaN()   withNaN(false) -> getWithoutNaN()   withNoInf(true) -> getWithoutInf()
//   containsZero() -> classify() & fcZero                   fpext/fptrunc -> cast(DstSem, RM)
//   add/sub/mul/div/abs/negate and unionWith/intersectWith keep their names.
// flushDenormals in PreserveSign mode widens exactly as the snap in getRange does.
//
// What stays IGC-side: the instruction walk and FPRangeCache, GenISA_fsat/GenISA_frc, the patterns, the NaN
// discipline in intersectOrFallback, and the margin in FoldProvableFCmp.cpp.
//
// Two hazards. LLVM's flushing is opt-in per operation, so route every operation through a helper that flushes rather
// than calling them from the walk directly; missing one reopens the sqrt-of-denormal hole described below. And
// ConstantFPRange distinguishes -0 from +0, so normalize a -0 lower bound to +0 at construction.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

//===----------------------------------------------------------------------===//
// FPRange - Floating-point interval [Lower, Upper]
//
// Three states:
//   Empty   - No numeric values. May indicate unreachable code OR
//             NaN-only output (when MayBeNaN=true). Bounds are undefined.
//   Bounded - Interval [Lower, Upper]. Bounds may be infinite.
//   Full    - Bounds unknown. Symbolic [-Inf, +Inf]. Says nothing about NaN
//             either way; that is MayBeNaN's job.
//
//   - union(Empty, X) = X
//   - union(X, Full) = Full
//   - intersect(Full, X) = X
//   - intersect(X, Empty) = Empty
//
// NaN Tracking (MayBeNaN)
// MayBeNaN == false is a proof the value is never NaN. getFull() sets it, which covers every give-up path
// (unanalyzable leaf, depth cutoff, unsupported opcode). It is orthogonal to Kind: an nnan operation on unbounded
// operands is Full yet never NaN. Operations that can generate NaN set it and the rest propagate it, except that
// minnum/maxnum need only one non-NaN operand (IEEE 754-2008) and nnan on the consumer clears it. Patterns describe
// exact arithmetic and never decide it, so computeFPRange keeps the instruction walk's value.
//
// Denormals
// getRange pulls any bound in the denormal band to zero, so every bound is zero or normal. It has to happen at
// construction: x*x for x in [1e-22, 1e-20] is entirely denormal and sqrt lifts it back out, after which a division
// looks safe while flushing hardware divides by zero. [0, d] holds both the flushed and the retained value, so nothing
// here depends on the denormal mode. Flushing preserves weak order and sign, so only claims that a value is not zero
// rest on the snap. An identity asserted by a pattern does not get this for free; see isNormalizationPattern.
//
// Usage:
// FPRange R = computeFPRange(V);
// if (R.MayBeNaN) { /* handle potential NaN */ }
//===----------------------------------------------------------------------===//

class FPRange {
public:
  enum class Kind {
    Empty,   // No values (unreachable code)
    Bounded, // Interval [Lower, Upper]
    Full     // Any value (unknown)
  };

  /// Default constructor
  FPRange() : FPRange(llvm::APFloat::IEEEsingle()) {}

  /// full set (unknown range, set to infinity bounds)
  FPRange(const llvm::fltSemantics &Sem)
      : Lower(llvm::APFloat::getInf(Sem, true)), Upper(llvm::APFloat::getInf(Sem, false)), RangeKind(Kind::Full) {}

  //===------------------------------------------------------------------===//
  // Named constructors
  //===------------------------------------------------------------------===//

  /// Nothing known: any value, including NaN - hence MayBeNaN=true. Caller can override with withNaN().
  static FPRange getFull(const llvm::fltSemantics &Sem) {
    return FPRange(llvm::APFloat::getInf(Sem, true), llvm::APFloat::getInf(Sem, false), Kind::Full, /*NaN=*/true);
  }

  /// Point range [Val, Val].
  /// If Val is NaN or Infinity, returns Full range (special values disallowed as point).
  static FPRange getPoint(const llvm::APFloat &Val) {
    // A NaN or infinite point widens to Full, so no bound is ever NaN. An infinity is still known non-NaN.
    if (Val.isNaN())
      return getFull(Val.getSemantics());
    if (Val.isInfinity())
      return getFull(Val.getSemantics()).withNaN(false);
    return getRange(Val, Val);
  }

  /// Empty set: no values (symbolizes impossible range)
  static FPRange getEmpty(const llvm::fltSemantics &Sem) {
    // Bounds are irrelevant for the empty set.
    return FPRange(llvm::APFloat::getZero(Sem), llvm::APFloat::getZero(Sem), Kind::Empty, /*NaN=*/false);
  }

  /// Range [Lo, Hi].
  /// Infinity bounds are valid (e.g., [0, +Inf] for non-negative values).
  /// Full-span [-Inf, +Inf] and point-infinity ranges are converted to FPRange::Full with NaN set to false.
  static FPRange getRange(llvm::APFloat Lo, llvm::APFloat Hi) {
    IGC_ASSERT_MESSAGE(!Lo.isNaN() && !Hi.isNaN(), "Range bounds cannot be NaN");
    IGC_ASSERT_MESSAGE(Lo.compare(Hi) != llvm::APFloat::cmpGreaterThan, "Range lower bound cannot exceed upper bound");

    // [-Inf, +Inf] is Full, but an interval holds no NaN.
    if (Lo.isInfinity() && Lo.isNegative() && Hi.isInfinity() && !Hi.isNegative())
      return getFull(Lo.getSemantics()).withNaN(false);

    // Point-infinity ranges [+Inf, +Inf] or [-Inf, -Inf] are also Full
    if (Lo.isInfinity() && Lo.bitwiseIsEqual(Hi))
      return getFull(Lo.getSemantics()).withNaN(false);

    // Assume flushing to zero by default for conservative interval analysis: any denormal lower bound that is positive
    // becomes +0, and any denormal upper bound that is negative becomes -0.
    const bool LoFlushes = Lo.isDenormal() && !Lo.isNegative();
    const bool HiFlushes = Hi.isDenormal() && Hi.isNegative();

    if (LoFlushes)
      Lo = llvm::APFloat::getZero(Lo.getSemantics());
    if (HiFlushes)
      Hi = llvm::APFloat::getZero(Hi.getSemantics(), /*Negative=*/true);
    return FPRange(std::move(Lo), std::move(Hi), Kind::Bounded, /*NaN=*/false);
  }

  // Range members
  llvm::APFloat Lower;
  llvm::APFloat Upper;
  Kind RangeKind = Kind::Full;

  // False is a proof of non-NaN; see NaN Tracking above. getFull sets it.
  bool MayBeNaN = false;

  //===------------------------------------------------------------------===//
  // Predicates
  //===------------------------------------------------------------------===//

  bool isFullSet() const { return RangeKind == Kind::Full; }
  bool isEmptySet() const { return RangeKind == Kind::Empty; }
  bool isBounded() const { return RangeKind == Kind::Bounded; }

  /// Returns true if either bound is infinite (or range is Full).
  bool isUnbounded() const {
    if (isFullSet())
      return true;
    if (isEmptySet())
      return false;
    return Lower.isInfinity() || Upper.isInfinity();
  }

  /// Get the floating-point semantics. Always valid regardless of state.
  const llvm::fltSemantics &getSemantics() const { return Lower.getSemantics(); }

  /// Returns true if the range is bounded and entirely non-negative (Lo >= 0).
  bool isNonNegative() const {
    if (!isBounded())
      return false;
    llvm::APFloat Zero = llvm::APFloat::getZero(getSemantics());
    return Lower.compare(Zero) != llvm::APFloat::cmpLessThan;
  }

  /// Returns true if the range is bounded and entirely non-positive (Hi <= 0).
  bool isNonPositive() const {
    if (!isBounded())
      return false;
    llvm::APFloat Zero = llvm::APFloat::getZero(getSemantics());
    return Upper.compare(Zero) != llvm::APFloat::cmpGreaterThan;
  }

  /// Returns true if the range is bounded and cannot be zero (Lo > 0 or Hi < 0). Denormal bounds have already snapped
  /// to zero, so this is usable as a division-by-zero test.
  bool isNonZero() const { return isStrictlyPositive() || isStrictlyNegative(); }

  /// Returns true if the range is bounded and entirely negative (Hi < 0).
  bool isStrictlyNegative() const {
    if (!isBounded())
      return false;
    llvm::APFloat Zero = llvm::APFloat::getZero(getSemantics());
    return Upper.compare(Zero) == llvm::APFloat::cmpLessThan;
  }

  /// Returns true if the range is bounded and entirely positive (Lo > 0).
  bool isStrictlyPositive() const {
    if (!isBounded())
      return false;
    llvm::APFloat Zero = llvm::APFloat::getZero(getSemantics());
    return Lower.compare(Zero) == llvm::APFloat::cmpGreaterThan;
  }

  /// Returns true if the range could be zero (Lo <= 0 && Hi >= 0), the denormal band included.
  bool containsZero() const {
    if (isFullSet())
      return true;
    if (isEmptySet())
      return false;
    llvm::APFloat Zero = llvm::APFloat::getZero(getSemantics());
    return Lower.compare(Zero) != llvm::APFloat::cmpGreaterThan && Upper.compare(Zero) != llvm::APFloat::cmpLessThan;
  }

  /// Returns true if upper bound is +Inf (or range is Full).
  bool hasPositiveInfinity() const {
    if (isFullSet())
      return true;
    if (isEmptySet())
      return false;
    return Upper.isInfinity() && !Upper.isNegative();
  }

  /// Returns true if lower bound is -Inf (or range is Full).
  bool hasNegativeInfinity() const {
    if (isFullSet())
      return true;
    if (isEmptySet())
      return false;
    return Lower.isInfinity() && Lower.isNegative();
  }

  /// Return copy with MayBeNaN set to given value. On a temporary this updates it in place.
  FPRange withNaN(bool NaN) const & { return FPRange(*this).withNaN(NaN); }
  FPRange withNaN(bool NaN) && {
    MayBeNaN = NaN;
    return std::move(*this);
  }

  /// Apply ninf flag: clamp infinite bounds to largest finite value. On a temporary this updates it in place.
  FPRange withNoInf(bool NoInf) const & { return NoInf ? FPRange(*this).withNoInf(true) : *this; }
  FPRange withNoInf(bool NoInf) && {
    if (NoInf)
      clampInfinities();
    return std::move(*this);
  }

  //===------------------------------------------------------------------===//
  // Printing
  //===------------------------------------------------------------------===//

  void print(llvm::raw_ostream &OS) const {
    if (isEmptySet()) {
      OS << "empty";
    } else if (isFullSet()) {
      OS << "full";
    } else {
      llvm::SmallString<32> LoStr, HiStr;
      Lower.toString(LoStr);
      Upper.toString(HiStr);
      OS << "[" << LoStr << ", " << HiStr << "]";
    }
    if (MayBeNaN)
      OS << " (may be NaN)";
  }

  //===------------------------------------------------------------------===//
  // Interval arithmetic
  //
  // Operations accept FastMathFlags to handle:
  //   - noInfs(): clamp infinite bounds to largest finite value
  //   - noNaNs(): force MayBeNaN=false (instruction promises no NaN)
  // All operations propagate MayBeNaN from inputs and detect NaN-producing cases.
  //===------------------------------------------------------------------===//

  FPRange add(const FPRange &Other, llvm::FastMathFlags FMF = {}) const {
    FPRange L = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);
    FPRange R = Other.withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && Other.MayBeNaN);

    if (L.isEmptySet() || R.isEmptySet())
      return getEmpty(sem()).withNaN(L.MayBeNaN || R.MayBeNaN);

    // Full only possible if noInfs=false (otherwise it became Bounded)
    if (L.isFullSet() || R.isFullSet()) {
      // Full spans [-Inf, +Inf], so +Inf + -Inf is possible
      return getFull(sem()).withNaN(!FMF.noNaNs());
    }

    // mayBeNaN if noNaNs is absent AND (propagated from input OR +Inf + -Inf)
    bool mayBeNaN =
        !FMF.noNaNs() && (L.MayBeNaN || R.MayBeNaN || (L.hasPositiveInfinity() && R.hasNegativeInfinity()) ||
                          (L.hasNegativeInfinity() && R.hasPositiveInfinity()));

    // Bound computation is always safe:
    // Lo = (finite or -Inf) + (finite or -Inf) = finite or -Inf, never NaN
    // Hi = (finite or +Inf) + (finite or +Inf) = finite or +Inf, never NaN
    llvm::APFloat Lo = L.Lower;
    llvm::APFloat Hi = L.Upper;
    Lo.add(R.Lower, llvm::APFloat::rmTowardNegative);
    Hi.add(R.Upper, llvm::APFloat::rmTowardPositive);
    return getRange(std::move(Lo), std::move(Hi)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
  }

  FPRange sub(const FPRange &Other, llvm::FastMathFlags FMF = {}) const {
    FPRange L = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);
    FPRange R = Other.withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && Other.MayBeNaN);

    if (L.isEmptySet() || R.isEmptySet())
      return getEmpty(sem()).withNaN(L.MayBeNaN || R.MayBeNaN);

    if (L.isFullSet() || R.isFullSet()) {
      return getFull(sem()).withNaN(!FMF.noNaNs());
    }

    // mayBeNaN if noNaNs is absent AND (propagated from input OR +Inf - +Inf OR -Inf - -Inf)
    bool mayBeNaN =
        !FMF.noNaNs() && (L.MayBeNaN || R.MayBeNaN || (L.hasPositiveInfinity() && R.hasPositiveInfinity()) ||
                          (L.hasNegativeInfinity() && R.hasNegativeInfinity()));

    // Bound computation is always safe:
    // Lo = (finite or -Inf) - (finite or +Inf) = finite or -Inf, never NaN
    // Hi = (finite or +Inf) - (finite or -Inf) = finite or +Inf, never NaN
    llvm::APFloat Lo = L.Lower;
    llvm::APFloat Hi = L.Upper;
    Lo.subtract(R.Upper, llvm::APFloat::rmTowardNegative);
    Hi.subtract(R.Lower, llvm::APFloat::rmTowardPositive);
    return getRange(std::move(Lo), std::move(Hi)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
  }

  FPRange mul(const FPRange &Other, llvm::FastMathFlags FMF = {}) const {
    FPRange L = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);
    FPRange R = Other.withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && Other.MayBeNaN);

    if (L.isEmptySet() || R.isEmptySet())
      return getEmpty(sem()).withNaN(L.MayBeNaN || R.MayBeNaN);

    // mayBeNaN if noNaNs is absent AND (propagated from input OR 0 * Inf)
    bool mayBeNaN = !FMF.noNaNs() && (L.MayBeNaN || R.MayBeNaN || (L.containsZero() && R.isUnbounded()) ||
                                      (R.containsZero() && L.isUnbounded()));

    // Full only possible if noInfs=false (otherwise it became Bounded)
    if (L.isFullSet() || R.isFullSet())
      return getFull(sem()).withNaN(mayBeNaN);

    llvm::APFloat products[4] = {L.Lower, L.Lower, L.Upper, L.Upper};
    products[0].multiply(R.Lower, llvm::APFloat::rmTowardNegative);
    products[1].multiply(R.Upper, llvm::APFloat::rmTowardNegative);
    products[2].multiply(R.Lower, llvm::APFloat::rmTowardNegative);
    products[3].multiply(R.Upper, llvm::APFloat::rmTowardNegative);

    llvm::APFloat productsHi[4] = {L.Lower, L.Lower, L.Upper, L.Upper};
    productsHi[0].multiply(R.Lower, llvm::APFloat::rmTowardPositive);
    productsHi[1].multiply(R.Upper, llvm::APFloat::rmTowardPositive);
    productsHi[2].multiply(R.Lower, llvm::APFloat::rmTowardPositive);
    productsHi[3].multiply(R.Upper, llvm::APFloat::rmTowardPositive);

    // 0 * Inf = NaN in APFloat - skip NaN products when finding bounds.
    // If all products are NaN (e.g., [0,0] * [-Inf,+Inf]), return Full.
    llvm::APFloat Lo = minOfOrNaN(products);
    llvm::APFloat Hi = maxOfOrNaN(productsHi);
    if (Lo.isNaN() || Hi.isNaN())
      return getFull(sem()).withNaN(mayBeNaN);

    return getRange(std::move(Lo), std::move(Hi)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
  }

  FPRange div(const FPRange &Other, llvm::FastMathFlags FMF = {}) const {
    // Pre-apply FMF constraints to operands
    FPRange L = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);
    FPRange R = Other.withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && Other.MayBeNaN);

    if (L.isEmptySet() || R.isEmptySet())
      return getEmpty(sem()).withNaN(L.MayBeNaN || R.MayBeNaN);

    // mayBeNaN if noNaNs is absent AND (propagated from input OR 0/0 OR Inf/Inf)
    bool mayBeNaN = !FMF.noNaNs() && (L.MayBeNaN || R.MayBeNaN || (L.containsZero() && R.containsZero()) ||
                                      (L.isUnbounded() && R.isUnbounded()));

    // Full only possible if noInfs=false (otherwise it became Bounded)
    if (L.isFullSet() || R.isFullSet())
      return getFull(sem()).withNaN(mayBeNaN);

    // A divisor that may be zero gives +/-Inf, so Full - reachable with ninf, hence withNoInf.
    if (R.containsZero())
      return getFull(sem()).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);

    // Compute quotients - use minOfOrNaN/maxOfOrNaN to handle Inf/Inf = NaN
    llvm::APFloat quotients[4] = {L.Lower, L.Lower, L.Upper, L.Upper};
    quotients[0].divide(R.Lower, llvm::APFloat::rmTowardNegative);
    quotients[1].divide(R.Upper, llvm::APFloat::rmTowardNegative);
    quotients[2].divide(R.Lower, llvm::APFloat::rmTowardNegative);
    quotients[3].divide(R.Upper, llvm::APFloat::rmTowardNegative);

    llvm::APFloat quotientsHi[4] = {L.Lower, L.Lower, L.Upper, L.Upper};
    quotientsHi[0].divide(R.Lower, llvm::APFloat::rmTowardPositive);
    quotientsHi[1].divide(R.Upper, llvm::APFloat::rmTowardPositive);
    quotientsHi[2].divide(R.Lower, llvm::APFloat::rmTowardPositive);
    quotientsHi[3].divide(R.Upper, llvm::APFloat::rmTowardPositive);

    // Inf/Inf = NaN in APFloat - skip NaN quotients when finding bounds.
    llvm::APFloat Lo = minOfOrNaN(quotients);
    llvm::APFloat Hi = maxOfOrNaN(quotientsHi);
    if (Lo.isNaN() || Hi.isNaN())
      return getFull(sem()).withNaN(mayBeNaN);

    return getRange(std::move(Lo), std::move(Hi)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
  }

  FPRange abs(llvm::FastMathFlags FMF = {}) const {
    // Pre-apply FMF constraints - abs never generates NaN, only propagates
    FPRange In = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);

    if (In.isEmptySet())
      return getEmpty(sem()).withNaN(In.MayBeNaN);

    // abs(Full) = [0, +Inf] (or [0, FLT_MAX] if noInfs)
    if (In.isFullSet())
      return getRange(llvm::APFloat::getZero(sem()), llvm::APFloat::getInf(sem(), false))
          .withNoInf(FMF.noInfs())
          .withNaN(In.MayBeNaN);

    llvm::APFloat Zero = llvm::APFloat::getZero(sem());

    // Already non-negative: abs is identity
    if (In.Lower.compare(Zero) != llvm::APFloat::cmpLessThan)
      return In;

    // Entirely non-positive: negate bounds
    if (In.Upper.compare(Zero) != llvm::APFloat::cmpGreaterThan) {
      llvm::APFloat Lo = In.Upper;
      llvm::APFloat Hi = In.Lower;
      Lo.changeSign();
      Hi.changeSign();
      return getRange(std::move(Lo), std::move(Hi)).withNaN(In.MayBeNaN);
    }

    // Spans zero: [0, max(|Lo|, |Hi|)]
    llvm::APFloat NegLower = In.Lower;
    NegLower.changeSign();
    llvm::APFloat Hi = (NegLower.compare(In.Upper) == llvm::APFloat::cmpGreaterThan) ? NegLower : In.Upper;
    return getRange(Zero, std::move(Hi)).withNaN(In.MayBeNaN);
  }

  FPRange negate(llvm::FastMathFlags FMF = {}) const {
    // Pre-apply FMF constraints - negate never generates NaN, only propagates
    FPRange In = withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && MayBeNaN);

    if (In.isEmptySet())
      return getEmpty(sem()).withNaN(In.MayBeNaN);

    if (In.isFullSet())
      return getFull(sem()).withNaN(In.MayBeNaN);

    llvm::APFloat Lo = In.Upper;
    llvm::APFloat Hi = In.Lower;
    Lo.changeSign();
    Hi.changeSign();
    return getRange(std::move(Lo), std::move(Hi)).withNaN(In.MayBeNaN);
  }

  /// Union two ranges to get the widest bound.
  FPRange unionWith(const FPRange &Other) const {
    // Empty union X = X (but preserve NaN from empty side)
    if (isEmptySet())
      return Other.withNaN(MayBeNaN || Other.MayBeNaN);
    if (Other.isEmptySet())
      return withNaN(MayBeNaN || Other.MayBeNaN);

    bool mayBeNaN = MayBeNaN || Other.MayBeNaN;

    // Full union X = Full
    if (isFullSet() || Other.isFullSet())
      return getFull(sem()).withNaN(mayBeNaN);

    llvm::APFloat Lo = (Lower.compare(Other.Lower) == llvm::APFloat::cmpLessThan) ? Lower : Other.Lower;
    llvm::APFloat Hi = (Upper.compare(Other.Upper) == llvm::APFloat::cmpGreaterThan) ? Upper : Other.Upper;
    return getRange(std::move(Lo), std::move(Hi)).withNaN(mayBeNaN);
  }

  /// Intersect two ranges to get the tightest bound.
  /// Returns empty set if ranges don't overlap.
  FPRange intersectWith(const FPRange &Other) const {
    // Empty intersect X = Empty (with AND semantics for NaN)
    if (isEmptySet() || Other.isEmptySet())
      return getEmpty(sem()).withNaN(MayBeNaN && Other.MayBeNaN);

    bool mayBeNaN = MayBeNaN && Other.MayBeNaN;

    // Full intersect X = X
    if (isFullSet())
      return Other.withNaN(mayBeNaN);
    if (Other.isFullSet())
      return withNaN(mayBeNaN);

    // Both bounded - compute intersection: [max(Lo1, Lo2), min(Hi1, Hi2)]
    llvm::APFloat Lo = (Lower.compare(Other.Lower) == llvm::APFloat::cmpGreaterThan) ? Lower : Other.Lower;
    llvm::APFloat Hi = (Upper.compare(Other.Upper) == llvm::APFloat::cmpLessThan) ? Upper : Other.Upper;

    // Check if intersection is empty (Lo > Hi)
    if (Lo.compare(Hi) == llvm::APFloat::cmpGreaterThan)
      return getEmpty(sem());

    return getRange(std::move(Lo), std::move(Hi)).withNaN(mayBeNaN);
  }

private:
  FPRange(llvm::APFloat Lo, llvm::APFloat Hi, Kind K, bool NaN)
      : Lower(std::move(Lo)), Upper(std::move(Hi)), RangeKind(K), MayBeNaN(NaN) {}

  /// Infinite bounds become the largest finite value: Full becomes [-Largest, +Largest]. ninf says nothing about NaN,
  /// so MayBeNaN is kept.
  void clampInfinities() {
    if (isEmptySet())
      return;
    if (isFullSet() || (Lower.isInfinity() && Lower.isNegative()))
      Lower = llvm::APFloat::getLargest(sem(), /*Negative=*/true);
    if (isFullSet() || (Upper.isInfinity() && !Upper.isNegative()))
      Upper = llvm::APFloat::getLargest(sem(), /*Negative=*/false);
    RangeKind = Kind::Bounded;
  }

  const llvm::fltSemantics &sem() const { return Lower.getSemantics(); }

  /// Find minimum of non-NaN values, or NaN if all values are NaN.
  static llvm::APFloat minOfOrNaN(llvm::ArrayRef<llvm::APFloat> arr) {
    llvm::APFloat result = llvm::APFloat::getNaN(arr[0].getSemantics());
    for (const auto &val : arr) {
      if (val.isNaN())
        continue;
      if (result.isNaN() || val.compare(result) == llvm::APFloat::cmpLessThan)
        result = val;
    }
    return result;
  }

  /// Find maximum of non-NaN values, or NaN if all values are NaN.
  static llvm::APFloat maxOfOrNaN(llvm::ArrayRef<llvm::APFloat> arr) {
    llvm::APFloat result = llvm::APFloat::getNaN(arr[0].getSemantics());
    for (const auto &val : arr) {
      if (val.isNaN())
        continue;
      if (result.isNaN() || val.compare(result) == llvm::APFloat::cmpGreaterThan)
        result = val;
    }
    return result;
  }
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const FPRange &R) {
  R.print(OS);
  return OS;
}

//===----------------------------------------------------------------------===//
// FPRangeCache - Optional cache for repeated queries
//===----------------------------------------------------------------------===//

using FPRangeCache = llvm::DenseMap<llvm::Value *, FPRange>;

/// Compute the floating-point range of a value.
/// The returned FPRange includes MayBeNaN flag indicating if the computation could produce NaN (e.g., sqrt of
/// negative, log of non-positive, 0/0).
/// The range bounds describe the numeric (non-NaN) portion of possible values.
/// Values the walk cannot see through (arguments, loads, unsupported operations) are Full and may be NaN.
FPRange computeFPRange(llvm::Value *V, FPRangeCache *Cache = nullptr, unsigned Depth = 0);

/// Check if the entire range is within [lo, hi].
bool isWithin(const FPRange &Range, double lo, double hi);

/// Check if the entire range is within [-bound, +bound].
inline bool isWithinSymmetric(const FPRange &Range, double bound) { return isWithin(Range, -bound, bound); }

/// Get the maximum absolute value in the range.
llvm::APFloat maxAbsValue(const FPRange &Range);

/// Create an FPRange from double bounds.
FPRange makeRange(double lo, double hi, const llvm::fltSemantics &Sem);

/// True if V cannot be negative by construction - a saturate, fract, fabs, sqrt, square, exp, unsigned conversion or
/// clamp, or a combination of those - rather than by arithmetic a later pass could rewrite. Only such a zero lower
/// bound is safe to use without a margin.
///
/// Conservative: false means "not shown", not "can be negative".
bool isStructurallyNonNegative(llvm::Value *V);

/// Mirror of the above for an upper bound of zero.
bool isStructurallyNonPositive(llvm::Value *V);

//===----------------------------------------------------------------------===//
// PrintFPRangeAnalysis - Printer pass providing FPRange for all FP values
//
// Usage:
//   igc_opt -print-fp-range-analysis -S < input.ll 2>&1
//
// The pass computes ranges for all floating-point instructions in a function
// and prints the result
//===----------------------------------------------------------------------===//

class PrintFPRangeAnalysis : public llvm::FunctionPass {
public:
  static char ID;

  PrintFPRangeAnalysis() : OS(llvm::errs()), llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;

  llvm::StringRef getPassName() const override { return "Print FP Range Analysis"; }

private:
  llvm::raw_ostream &OS;
};

llvm::FunctionPass *createPrintFPRangeAnalysisPass();

} // namespace IGC
