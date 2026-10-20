/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FPRangeAnalysis.cpp - Floating Point Range Tracking
// WARNING: The implemented range analysis assumes fast math. NaN is tracked - MayBeNaN == false is a proof of
// non-NaN - but signed zero is not. Results the hardware approximates are widened loosely (widenApprox), but the bounds
// are an estimate rather than a ULP-exact envelope: reassoc, contract and arcp let later passes compute a slightly
// different value, so consumers should apply a margin.
//
//===----------------------------------------------------------------------===//

#include "FPRangeAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/KnownBits.h>
#include "common/LLVMWarningsPop.hpp"

#include <cmath>

using namespace llvm;
using namespace IGC;

namespace IGC {

//===----------------------------------------------------------------------===//
// APFloat Helpers
//===----------------------------------------------------------------------===//

static APFloat apfMin(const APFloat &A, const APFloat &B) { return A.compare(B) == APFloat::cmpLessThan ? A : B; }

static APFloat apfMax(const APFloat &A, const APFloat &B) { return A.compare(B) == APFloat::cmpGreaterThan ? A : B; }

static APFloat makeAPFloat(double val, const fltSemantics &Sem,
                           APFloat::roundingMode RM = APFloat::rmNearestTiesToEven) {
  bool LosesInfo;
  APFloat result(val);
  result.convert(Sem, RM, &LosesInfo);
  return result;
}

static bool isSupportedFPType(Type *Ty) { return Ty->isFloatTy() || Ty->isDoubleTy() || Ty->isHalfTy(); }

static const fltSemantics &getSemanticsForType(Type *Ty) {
  if (Ty->isFloatTy())
    return APFloat::IEEEsingle();
  if (Ty->isDoubleTy())
    return APFloat::IEEEdouble();
  if (Ty->isHalfTy())
    return APFloat::IEEEhalf();
  llvm_unreachable("Unsupported floating-point type in FPRangeAnalysis");
}

//===----------------------------------------------------------------------===//
// Fast-Math Aware Helpers
//===----------------------------------------------------------------------===//

/// Match division a/b, including arcp-transformed a*(1/b).
static bool matchDivision(Value *V, Value *&Num, Value *&Denom) {
  auto *BO = dyn_cast<BinaryOperator>(V);
  if (!BO)
    return false;

  if (BO->getOpcode() == Instruction::FDiv) {
    Num = BO->getOperand(0);
    Denom = BO->getOperand(1);
    return true;
  }

  // arcp: a * (1/b)
  if (BO->getOpcode() == Instruction::FMul) {
    for (int i = 0; i < 2; ++i) {
      if (auto *Div = dyn_cast<BinaryOperator>(BO->getOperand(i))) {
        if (Div->getOpcode() == Instruction::FDiv) {
          if (auto *One = dyn_cast<ConstantFP>(Div->getOperand(0))) {
            if (One->isExactlyValue(1.0)) {
              Num = BO->getOperand(1 - i);
              Denom = Div->getOperand(1);
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

/// Check if a value is an exp or exp2 intrinsic
static bool isExpIntrinsic(Value *V) {
  if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    return II->getIntrinsicID() == Intrinsic::exp || II->getIntrinsicID() == Intrinsic::exp2;
  }
  return false;
}

//===----------------------------------------------------------------------===//
// Exported Helper Functions
//===----------------------------------------------------------------------===//

/// Returns true only if Range is bounded and provably within [lo, hi].
/// Returns false for Full (unknown), Empty (unverified dead), or out-of-bounds.
/// Note: false doesn't mean "outside", just "not proven inside".
bool isWithin(const FPRange &Range, double lo, double hi) {
  if (Range.isFullSet() || Range.isEmptySet())
    return false;

  const fltSemantics &Sem = Range.getSemantics();
  APFloat LoAP = makeAPFloat(lo, Sem, APFloat::rmTowardNegative);
  APFloat HiAP = makeAPFloat(hi, Sem, APFloat::rmTowardPositive);

  // Range.Lo >= lo && Range.Hi <= hi
  return LoAP.compare(Range.Lower) != APFloat::cmpGreaterThan && Range.Upper.compare(HiAP) != APFloat::cmpGreaterThan;
}

/// Get the maximum absolute value in the numeric portion of Range.
/// Returns +Inf for full or empty sets as a conservative fallback.
APFloat maxAbsValue(const FPRange &Range) {
  const fltSemantics &Sem = Range.getSemantics();
  if (Range.isFullSet() || Range.isEmptySet())
    return APFloat::getInf(Sem, false);

  APFloat AbsLower = Range.Lower;
  APFloat AbsUpper = Range.Upper;
  if (AbsLower.isNegative())
    AbsLower.changeSign();
  if (AbsUpper.isNegative())
    AbsUpper.changeSign();

  return apfMax(AbsLower, AbsUpper);
}

/// Convenience wrapper to create range from double bounds.
FPRange makeRange(double lo, double hi, const fltSemantics &Sem) {
  APFloat LoAP = makeAPFloat(lo, Sem, APFloat::rmTowardNegative);
  APFloat HiAP = makeAPFloat(hi, Sem, APFloat::rmTowardPositive);
  return FPRange::getRange(std::move(LoAP), std::move(HiAP));
}

/// R moved outward to cover a result the hardware approximates (fdiv, sqrt, exp, log, sin, cos): each bound by
/// 2^-5 of itself, 2^-4 for half. AbsoluteBelowOne charges bounds below 1.0 the error of 1.0, for log, sin and cos,
/// whose error near zero is absolute.
static FPRange widenApprox(const FPRange &R, bool AbsoluteBelowOne = false) {
  if (!R.isBounded())
    return R;
  const fltSemantics &Sem = R.getSemantics();
  const int Bits = &Sem == &APFloat::IEEEhalf() ? 4 : 5;
  const APFloat One = makeAPFloat(1.0, Sem);
  auto Widen = [&](APFloat X, bool Down) {
    APFloat Mag = llvm::abs(X);
    if (AbsoluteBelowOne && Mag.compare(One) == APFloat::cmpLessThan)
      Mag = One;
    APFloat E = llvm::scalbn(Mag, -Bits, APFloat::rmTowardPositive);
    // Infinite bounds stay put, and a zero error is skipped: 0 - 0 rounded toward negative is -0.
    if (X.isInfinity() || E.isZero())
      return X;
    if (Down)
      X.subtract(E, APFloat::rmTowardNegative);
    else
      X.add(E, APFloat::rmTowardPositive);
    return X;
  };
  return FPRange::getRange(Widen(R.Lower, true), Widen(R.Upper, false)).withNaN(R.MayBeNaN);
}

//===----------------------------------------------------------------------===//
// Pattern Detection (Fast-Math Aware)
//
// TODO: Add leaky ReLU detection: select(x > 0, x, alpha * x) where 0 < alpha < 1.
// Currently handled by analyzeSelect (union of branches), which is conservative.
// A dedicated pattern could compute tighter bounds: [alpha * lo, hi]
//===----------------------------------------------------------------------===//

/// Logistic: 1/(1+exp(x)) or exp(x)/(1+exp(x)) -> (0, 1)
/// The exp(x)/(1+exp(x)) form needs semantic matching because interval
/// arithmetic loses correlation, computing [0,+Inf]/[1,+Inf] = [0,+Inf].
static bool isLogisticPattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result) {
  const fltSemantics &Sem = Result.getSemantics();
  Value *Num = nullptr, *Denom = nullptr;
  if (!matchDivision(V, Num, Denom))
    return false;

  // Check denominator is (1 + exp(...))
  auto *Add = dyn_cast<BinaryOperator>(Denom);
  if (!Add || Add->getOpcode() != Instruction::FAdd)
    return false;

  Value *AddL = Add->getOperand(0);
  Value *AddR = Add->getOperand(1);

  // Look for 1.0 and exp() in either order
  for (int i = 0; i < 2; ++i) {
    Value *MaybeOne = (i == 0) ? AddL : AddR;
    Value *MaybeExp = (i == 0) ? AddR : AddL;

    if (auto *C = dyn_cast<ConstantFP>(MaybeOne)) {
      if (C->isExactlyValue(1.0) && isExpIntrinsic(MaybeExp)) {
        auto *DenomExp = cast<IntrinsicInst>(MaybeExp);
        // compute ExpArg range to align with NaN propagation.
        FPRange ExpArg = computeFPRange(DenomExp->getArgOperand(0), Cache, Depth + 1);
        // 1/(1+exp(x)) -> (0, 1)
        // Large abs(x) can cause overflow/underflow, but result is still bound in [0, 1]
        if (auto *NumC = dyn_cast<ConstantFP>(Num)) {
          if (NumC->isExactlyValue(1.0)) {
            Result = widenApprox(makeRange(0.0, 1.0, Sem)).withNaN(ExpArg.MayBeNaN);
            return true;
          }
        }
        // exp(x)/(1+exp(x)) -> (0, 1), verify same intrinsic and argument
        if (isExpIntrinsic(Num)) {
          auto *NumExp = cast<IntrinsicInst>(Num);
          if (NumExp->getIntrinsicID() == DenomExp->getIntrinsicID() &&
              NumExp->getArgOperand(0) == DenomExp->getArgOperand(0)) {
            Result = widenApprox(makeRange(0.0, 1.0, Sem)).withNaN(ExpArg.MayBeNaN);
            return true;
          }
        }
      }
    }
  }
  return false;
}

// Select-based min/max with constant bound: select(fcmp x > c, x, c) -> max(x, c)
// Only matches when one operand is a constant. The general case with two non-constants is handled conservatively by
// analyzeSelect (union of branches). Can match select format of ReLU which looks like: select(x > 0, x, 0)
/// Why: Note that analyzeSelect computes union of branches, losing the constraint that the condition imposes on which
/// branch is taken.
static bool isSelectMinMaxPattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result) {
  const fltSemantics &Sem = Result.getSemantics();
  auto *Sel = dyn_cast<SelectInst>(V);
  if (!Sel)
    return false;

  auto *FCmp = dyn_cast<FCmpInst>(Sel->getCondition());
  if (!FCmp)
    return false;

  Value *CmpLHS = FCmp->getOperand(0);
  Value *CmpRHS = FCmp->getOperand(1);
  Value *TrueVal = Sel->getTrueValue();
  Value *FalseVal = Sel->getFalseValue();
  CmpInst::Predicate Pred = FCmp->getPredicate();

  // Check: select(x cmp y, x, y) or select(x cmp y, y, x)
  // The true/false values must match the comparison operands
  bool trueIsLHS;
  if (TrueVal == CmpLHS && FalseVal == CmpRHS) {
    trueIsLHS = true;
  } else if (TrueVal == CmpRHS && FalseVal == CmpLHS) {
    trueIsLHS = false;
  } else {
    return false;
  }

  // Require at least one operand to be a constant
  auto *ConstLHS = dyn_cast<ConstantFP>(CmpLHS);
  auto *ConstRHS = dyn_cast<ConstantFP>(CmpRHS);
  if (!ConstLHS && !ConstRHS)
    return false;

  // Determine if this is max or min based on predicate and branch mapping
  // max(x, y) = select(x > y, x, y) or select(x < y, y, x)
  // min(x, y) = select(x < y, x, y) or select(x > y, y, x)
  bool isMax;
  switch (Pred) {
  case CmpInst::FCMP_OGT:
  case CmpInst::FCMP_OGE:
  case CmpInst::FCMP_UGT:
  case CmpInst::FCMP_UGE:
    // x > y: max if true=x (LHS), min if true=y (RHS)
    isMax = trueIsLHS;
    break;
  case CmpInst::FCMP_OLT:
  case CmpInst::FCMP_OLE:
  case CmpInst::FCMP_ULT:
  case CmpInst::FCMP_ULE:
    // x < y: max if true=y (RHS), min if true=x (LHS)
    isMax = !trueIsLHS;
    break;
  default:
    return false;
  }

  // Get constant bound and variable range
  APFloat Bound = ConstLHS ? ConstLHS->getValueAPF() : ConstRHS->getValueAPF();
  Value *Var = ConstLHS ? CmpRHS : CmpLHS;
  FPRange VarRange = computeFPRange(Var, Cache, Depth + 1);

  if (isMax) {
    // max(Var, Bound): result >= Bound
    if (VarRange.isFullSet()) {
      Result = FPRange::getRange(Bound, APFloat::getInf(Sem, false)).withNaN(VarRange.MayBeNaN);
    } else {
      APFloat Lo = apfMax(VarRange.Lower, Bound);
      APFloat Hi = apfMax(VarRange.Upper, Bound);
      Result = FPRange::getRange(std::move(Lo), std::move(Hi)).withNaN(VarRange.MayBeNaN);
    }
  } else {
    // min(Var, Bound): result <= Bound
    if (VarRange.isFullSet()) {
      Result = FPRange::getRange(APFloat::getInf(Sem, true), Bound).withNaN(VarRange.MayBeNaN);
    } else {
      APFloat Lo = apfMin(VarRange.Lower, Bound);
      APFloat Hi = apfMin(VarRange.Upper, Bound);
      Result = FPRange::getRange(std::move(Lo), std::move(Hi)).withNaN(VarRange.MayBeNaN);
    }
  }

  return true;
}

/// Match fmul x, x pattern, returning the base operand x or nullptr if no match.
/// Note: This compares LLVM Values directly, so equivalent expressions in different
/// Values (e.g., c = a + b; d = a + b; c * d) are not recognized as squares.
static Value *matchSquareMul(Value *V) {
  auto *Mul = dyn_cast<BinaryOperator>(V);
  if (!Mul || Mul->getOpcode() != Instruction::FMul)
    return nullptr;
  Value *Op = Mul->getOperand(0);
  return (Op == Mul->getOperand(1)) ? Op : nullptr;
}

/// True if a division matched by matchDivision cannot produce +/-Inf: ninf on the fdiv, or in the arcp form on the
/// multiply or on the 1/Denom reciprocal itself. ninf on a numerator that is itself a division does not count.
static bool divisionHasNoInfs(Value *V, Value *Denom) {
  auto *BO = cast<BinaryOperator>(V);
  if (BO->hasNoInfs())
    return true;
  if (BO->getOpcode() != Instruction::FMul)
    return false;
  for (Value *Op : BO->operands()) {
    auto *Div = dyn_cast<BinaryOperator>(Op);
    if (!Div || Div->getOpcode() != Instruction::FDiv || Div->getOperand(1) != Denom || !Div->hasNoInfs())
      continue;
    auto *One = dyn_cast<ConstantFP>(Div->getOperand(0));
    if (One && One->isExactlyValue(1.0))
      return true;
  }
  return false;
}

/// Normalization: x / sqrt(x^2 + y^2 + ...) -> [-1.5, 1.5]
/// Verifies all sqrt addends are non-negative (squares). Handles arcp transform.
///
/// |x| <= sqrt(x^2 + ...) holds over the reals but not in floating point. A square that underflows to zero gives
/// +/-Inf, which ninf on the division makes poison. A square rounded in the denormal band can fall to 1/2 of its true
/// value (1/1.5 when rounding to nearest), bounding the quotient by sqrt(2); 1.5 also covers sqrt/rsqrt approximation.
/// More addends only shrink the quotient. EnableUnsafeFPRangeNormalize skips the ninf check and bounds to [-1, 1].
static bool isNormalizationPattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result) {
  const fltSemantics &Sem = Result.getSemantics();
  Value *Num = nullptr, *Denom = nullptr;
  if (!matchDivision(V, Num, Denom))
    return false;

  const bool Unsafe = IGC_IS_FLAG_ENABLED(EnableUnsafeFPRangeNormalize);
  if (!Unsafe && !divisionHasNoInfs(V, Denom))
    return false;

  // Check if denominator is sqrt(...)
  auto *Sqrt = dyn_cast<IntrinsicInst>(Denom);
  if (!Sqrt || Sqrt->getIntrinsicID() != Intrinsic::sqrt)
    return false;

  Value *SqrtArg = Sqrt->getArgOperand(0);

  // Track whether we found a term with strictly positive lower bound.
  // If so, the denominator is guaranteed non-zero, avoiding the 0/sqrt(0) NaN case.
  bool foundStrictlyPositive = false;
  bool foundNumSquare = false;
  bool anyMayBeNaN = false;

  // Verify sqrt argument is a sum of non-negative terms containing Num^2.
  auto verifySum = [&](auto &&self, Value *V) -> bool {
    // Check for squares (x * x) - always non-negative
    if (Value *X = matchSquareMul(V)) {
      if (X == Num)
        foundNumSquare = true;
      // x*x is strictly positive if x is non-zero
      FPRange XRange = computeFPRange(X, Cache, Depth + 1);
      anyMayBeNaN |= XRange.MayBeNaN;
      if (XRange.isNonZero())
        foundStrictlyPositive = true;
      return true;
    }

    // Recurse on FAdd
    if (auto *Add = dyn_cast<BinaryOperator>(V)) {
      if (Add->getOpcode() == Instruction::FAdd)
        return self(self, Add->getOperand(0)) && self(self, Add->getOperand(1));
    }

    // Handle fma(a, b, c) = a*b + c from contract optimization
    if (auto *FMA = dyn_cast<IntrinsicInst>(V)) {
      if (FMA->getIntrinsicID() == Intrinsic::fma || FMA->getIntrinsicID() == Intrinsic::fmuladd) {
        Value *A = FMA->getArgOperand(0);
        Value *B = FMA->getArgOperand(1);
        Value *C = FMA->getArgOperand(2);

        // fma(x, x, C) = x^2 + C where x^2 is non-negative
        if (A == B) {
          if (A == Num)
            foundNumSquare = true;
          FPRange ARange = computeFPRange(A, Cache, Depth + 1);
          anyMayBeNaN |= ARange.MayBeNaN;
          if (ARange.isNonZero())
            foundStrictlyPositive = true;
          return self(self, C);
        }
        // fma(a, b, C) where a != b: fall through to range check
      }
    }

    // Non-square term: must have non-negative range to be conservative for sqrt
    FPRange R = computeFPRange(V, Cache, Depth + 1);
    anyMayBeNaN |= R.MayBeNaN;
    if (R.isStrictlyPositive())
      foundStrictlyPositive = true;
    return R.isNonNegative();
  };

  if (!verifySum(verifySum, SqrtArg))
    return false;

  if (!foundNumSquare)
    return false;

  // 0/sqrt(0) can produce NaN, but only if all terms could be zero.
  // If we found any term with strictly positive lower bound, the denominator is guaranteed non-zero.
  const double Bound = Unsafe ? 1.0 : 1.5;
  Result = makeRange(-Bound, Bound, Sem).withNaN(anyMayBeNaN || !foundStrictlyPositive);
  return true;
}

/// Square: x * x -> [0, max^2].
/// Required because mul([lo,hi], [lo,hi]) treats operands as independent, incorrectly allowing negative results for
/// squares.
static bool isSquarePattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result) {
  const fltSemantics &Sem = Result.getSemantics();
  Value *X = matchSquareMul(V);
  if (!X)
    return false;

  FPRange In = computeFPRange(X, Cache, Depth + 1);
  // x*x is always non-negative
  if (In.isFullSet()) {
    Result = FPRange::getRange(APFloat::getZero(Sem), APFloat::getInf(Sem, false)).withNaN(In.MayBeNaN);
    return true;
  }

  APFloat Lo = In.Lower;
  APFloat Hi = In.Upper;
  APFloat Zero = APFloat::getZero(Sem);

  // Compute squares using APFloat multiply with appropriate rounding mode for conservative bounds
  auto square = [](APFloat V, APFloat::roundingMode rm) {
    APFloat R = V;
    R.multiply(V, rm);
    return R;
  };

  APFloat ResultLo = Zero;
  APFloat ResultHi = Zero;

  if (In.isNonNegative()) {
    // Lo >= 0: result is [Lo^2, Hi^2]
    ResultLo = square(Lo, APFloat::rmTowardNegative);
    ResultHi = square(Hi, APFloat::rmTowardPositive);
  } else if (In.isNonPositive()) {
    // Hi <= 0: result is [Hi^2, Lo^2]
    ResultLo = square(Hi, APFloat::rmTowardNegative);
    ResultHi = square(Lo, APFloat::rmTowardPositive);
  } else {
    // Spans zero: result is [0, max(Lo^2, Hi^2)]
    APFloat LoSq = square(Lo, APFloat::rmTowardPositive);
    APFloat HiSq = square(Hi, APFloat::rmTowardPositive);
    ResultHi = apfMax(LoSq, HiSq);
  }

  Result = FPRange::getRange(std::move(ResultLo), std::move(ResultHi)).withNaN(In.MayBeNaN);
  return true;
}

/// Results of the two sign walks for one query. They recurse into each other, and an fmul asks both signs of both
/// operands, so without this the work grows exponentially with depth.
struct SignMemo {
  DenseMap<Value *, bool> NonNeg;
  DenseMap<Value *, bool> NonPos;
};

static bool structNonNeg(Value *V, unsigned Depth, SignMemo &M);
static bool structNonPos(Value *V, unsigned Depth, SignMemo &M);

/// Memoized wrapper around Compute. A false placeholder is stored while V is in progress, so a loop-carried phi
/// terminates rather than recursing.
template <typename ComputeFn>
static bool memoizedSign(Value *V, unsigned Depth, DenseMap<Value *, bool> &Memo, ComputeFn Compute) {
  if (!V || Depth > 12)
    return false;
  auto It = Memo.find(V);
  if (It != Memo.end())
    return It->second;
  Memo[V] = false;
  const bool Result = Compute();
  Memo[V] = Result;
  return Result;
}

static bool computeNonNeg(Value *V, unsigned Depth, SignMemo &M) {
  auto pos = [&](Value *X) { return structNonNeg(X, Depth + 1, M); };
  auto neg = [&](Value *X) { return structNonPos(X, Depth + 1, M); };

  if (auto *C = dyn_cast<ConstantFP>(V)) {
    const APFloat &F = C->getValueAPF();
    return !F.isNaN() && (!F.isNegative() || F.isZero());
  }
  // fsat clamps to [0, 1] and sends NaN to 0; frc returns the fractional part in [0, 1).
  if (auto *GII = dyn_cast<GenIntrinsicInst>(V))
    return GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fsat ||
           GII->getIntrinsicID() == GenISAIntrinsic::GenISA_frc;
  if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    switch (II->getIntrinsicID()) {
    // |x| is non-negative, sqrt of a negative is NaN rather than negative, and exp never returns a negative.
    case Intrinsic::fabs:
    case Intrinsic::sqrt:
    case Intrinsic::exp:
    case Intrinsic::exp2:
      return true;
    // Rounding cannot cross zero.
    case Intrinsic::floor:
    case Intrinsic::ceil:
    case Intrinsic::trunc:
    case Intrinsic::rint:
    case Intrinsic::nearbyint:
      return pos(II->getArgOperand(0));
    // maximum is non-negative as soon as one side is (NaN propagates); min needs both. maxnum returns the other side
    // when one is NaN, so one side is enough only if it is a literal, which the constant case keeps non-NaN.
    case Intrinsic::maximum:
      return pos(II->getArgOperand(0)) || pos(II->getArgOperand(1));
    case Intrinsic::maxnum: {
      Value *A = II->getArgOperand(0), *B = II->getArgOperand(1);
      return (pos(A) && pos(B)) || (isa<ConstantFP>(A) && pos(A)) || (isa<ConstantFP>(B) && pos(B));
    }
    case Intrinsic::minnum:
    case Intrinsic::minimum:
      return pos(II->getArgOperand(0)) && pos(II->getArgOperand(1));
    default:
      return false;
    }
  }
  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return false;
  switch (I->getOpcode()) {
  case Instruction::UIToFP:
    return true;
  // x*x is non-negative whatever x is, as is a product or sum of non-negatives however it is reassociated:
  // overflow gives +Inf and underflow +0.
  case Instruction::FMul:
    return matchSquareMul(I) != nullptr || (pos(I->getOperand(0)) && pos(I->getOperand(1))) ||
           (neg(I->getOperand(0)) && neg(I->getOperand(1)));
  case Instruction::FAdd:
    return pos(I->getOperand(0)) && pos(I->getOperand(1));
  case Instruction::FNeg:
    return neg(I->getOperand(0));
  case Instruction::Select:
    return pos(I->getOperand(1)) && pos(I->getOperand(2));
  case Instruction::PHI: {
    auto *PN = cast<PHINode>(I);
    return PN->getNumIncomingValues() > 0 && llvm::all_of(PN->incoming_values(), pos);
  }
  case Instruction::FPExt:
  case Instruction::FPTrunc:
    return pos(I->getOperand(0));
  default:
    return false;
  }
}

/// Mirror of computeNonNeg. Nothing modelled is unconditionally non-positive, so every case is a composition.
static bool computeNonPos(Value *V, unsigned Depth, SignMemo &M) {
  auto pos = [&](Value *X) { return structNonNeg(X, Depth + 1, M); };
  auto neg = [&](Value *X) { return structNonPos(X, Depth + 1, M); };

  if (auto *C = dyn_cast<ConstantFP>(V)) {
    const APFloat &F = C->getValueAPF();
    return !F.isNaN() && (F.isNegative() || F.isZero());
  }
  if (isa<GenIntrinsicInst>(V))
    return false;
  if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::floor:
    case Intrinsic::ceil:
    case Intrinsic::trunc:
    case Intrinsic::rint:
    case Intrinsic::nearbyint:
      return neg(II->getArgOperand(0));
    // minimum is non-positive as soon as one side is; minnum, as maxnum above; max needs both.
    case Intrinsic::minimum:
      return neg(II->getArgOperand(0)) || neg(II->getArgOperand(1));
    case Intrinsic::minnum: {
      Value *A = II->getArgOperand(0), *B = II->getArgOperand(1);
      return (neg(A) && neg(B)) || (isa<ConstantFP>(A) && neg(A)) || (isa<ConstantFP>(B) && neg(B));
    }
    case Intrinsic::maxnum:
    case Intrinsic::maximum:
      return neg(II->getArgOperand(0)) && neg(II->getArgOperand(1));
    default:
      return false;
    }
  }
  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return false;
  switch (I->getOpcode()) {
  // Opposite signs multiply to a non-positive. This is the common shape: a magnitude scaled by a negative constant.
  case Instruction::FMul:
    return (pos(I->getOperand(0)) && neg(I->getOperand(1))) || (neg(I->getOperand(0)) && pos(I->getOperand(1)));
  case Instruction::FAdd:
    return neg(I->getOperand(0)) && neg(I->getOperand(1));
  case Instruction::FNeg:
    return pos(I->getOperand(0));
  case Instruction::Select:
    return neg(I->getOperand(1)) && neg(I->getOperand(2));
  case Instruction::PHI: {
    auto *PN = cast<PHINode>(I);
    return PN->getNumIncomingValues() > 0 && llvm::all_of(PN->incoming_values(), neg);
  }
  case Instruction::FPExt:
  case Instruction::FPTrunc:
    return neg(I->getOperand(0));
  default:
    return false;
  }
}

static bool structNonNeg(Value *V, unsigned Depth, SignMemo &M) {
  return memoizedSign(V, Depth, M.NonNeg, [&] { return computeNonNeg(V, Depth, M); });
}

static bool structNonPos(Value *V, unsigned Depth, SignMemo &M) {
  return memoizedSign(V, Depth, M.NonPos, [&] { return computeNonPos(V, Depth, M); });
}

bool isStructurallyNonNegative(Value *V) {
  SignMemo M;
  return structNonNeg(V, 0, M);
}

bool isStructurallyNonPositive(Value *V) {
  SignMemo M;
  return structNonPos(V, 0, M);
}

//===----------------------------------------------------------------------===//
// Instruction Analysis
//===----------------------------------------------------------------------===//

static FPRange analyzeIntToFP(Value *V, bool Signed, const fltSemantics &Sem) {
  unsigned Width = V->getType()->getScalarSizeInBits();
  KnownBits Known(Width);

  if (auto *I = dyn_cast<Instruction>(V)) {
    IGC_ASSERT(I->getModule());
    Known = computeKnownBits(V, I->getModule()->getDataLayout());
  }

  APInt LoInt, HiInt;
  if (Signed) {
    LoInt = Known.getSignedMinValue();
    HiInt = Known.getSignedMaxValue();
  } else {
    LoInt = Known.getMinValue();
    HiInt = Known.getMaxValue();
  }

  APFloat Lo(Sem), Hi(Sem);
  Lo.convertFromAPInt(LoInt, Signed, APFloat::rmTowardNegative);
  Hi.convertFromAPInt(HiInt, Signed, APFloat::rmTowardPositive);
  return FPRange::getRange(std::move(Lo), std::move(Hi));
}

static FPRange analyzeIntrinsic(IntrinsicInst *II, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  FastMathFlags FMF = II->getFastMathFlags();

  if (auto *GII = dyn_cast<GenIntrinsicInst>(II)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_frc: {
      // frc returns fractional component in [0, 1)
      // Note: The upper bound is technically exclusive, but using 1.0 is a conservative approximation that keeps the
      // range closed and simplifies FPRange implementation.
      // frc(+/-Inf) and frc(NaN) produce NaN.
      if (FMF.noNaNs())
        return makeRange(0.0, 1.0, Sem);
      FPRange In = computeFPRange(GII->getArgOperand(0), Cache, Depth + 1);
      return makeRange(0.0, 1.0, Sem).withNaN(In.MayBeNaN || In.isUnbounded());
    }
    case GenISAIntrinsic::GenISA_fsat: {
      // Clamps to [0, 1]; NaN saturates to 0.
      return makeRange(0.0, 1.0, Sem);
    }
    default:
      return FPRange::getFull(Sem);
    }
  } else {
    switch (II->getIntrinsicID()) {
    case Intrinsic::fabs: {
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      return In.abs(FMF);
    }
    case Intrinsic::sqrt: {
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      // sqrt(x) for x >= 0 produces non-negative result.
      // When input range spans negative values (e.g., [-5, 10]), sqrt of negatives produces NaN.
      // We ignore the NaN-producing portion and return bounds for valid inputs: [0, sqrt(Hi)].
      APFloat SqrtLo = APFloat::getZero(Sem);

      if (In.isFullSet())
        return FPRange::getRange(SqrtLo, APFloat::getInf(Sem, false)).withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs());

      // Strictly negative input produces only NaN, no valid numeric output
      if (In.isStrictlyNegative())
        return FPRange::getEmpty(Sem).withNaN(!FMF.noNaNs());

      // NaN if nnan absent AND (input may be NaN OR input may be negative)
      bool mayBeNaN = !FMF.noNaNs() && (In.MayBeNaN || !In.isNonNegative());

      if (In.isNonNegative()) {
        double lo = In.Lower.convertToDouble();
        SqrtLo = makeAPFloat(std::sqrt(lo), Sem, APFloat::rmTowardNegative);
      }
      double hi = In.Upper.convertToDouble();
      APFloat SqrtHi = makeAPFloat(std::sqrt(hi), Sem, APFloat::rmTowardPositive);
      return widenApprox(FPRange::getRange(std::move(SqrtLo), std::move(SqrtHi)))
          .withNoInf(FMF.noInfs())
          .withNaN(mayBeNaN);
    }
    case Intrinsic::minnum:
    case Intrinsic::minimum: {
      FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
      // minnum: returns non-NaN if one operand is NaN (IEEE 754-2008) -> NaN only if both MayBeNaN
      // minimum: returns NaN if either operand is NaN (IEEE 754-2019) -> NaN if either MayBeNaN
      bool isMinimum = II->getIntrinsicID() == Intrinsic::minimum;
      bool mayBeNaN = !FMF.noNaNs() && (isMinimum ? (A.MayBeNaN || B.MayBeNaN) : (A.MayBeNaN && B.MayBeNaN));

      // minnum returns the other operand when one is NaN, so all of that operand's values are possible too.
      auto Finish = [&](FPRange R) {
        if (!isMinimum && A.MayBeNaN)
          R = R.unionWith(B);
        if (!isMinimum && B.MayBeNaN)
          R = R.unionWith(A);
        return R.withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
      };

      // An Empty operand is NaN-only or unreachable: minimum/maximum then give NaN, minnum/maxnum the other operand.
      if (A.isEmptySet() || B.isEmptySet())
        return Finish(FPRange::getEmpty(Sem));
      if (A.isFullSet() && B.isFullSet())
        return Finish(FPRange::getFull(Sem));
      if (A.isFullSet())
        return Finish(FPRange::getRange(APFloat::getInf(Sem, true), B.Upper));
      if (B.isFullSet())
        return Finish(FPRange::getRange(APFloat::getInf(Sem, true), A.Upper));

      APFloat Lo = apfMin(A.Lower, B.Lower);
      APFloat Hi = apfMin(A.Upper, B.Upper);
      return Finish(FPRange::getRange(std::move(Lo), std::move(Hi)));
    }
    case Intrinsic::maxnum:
    case Intrinsic::maximum: {
      FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
      // maxnum: returns non-NaN if one operand is NaN (IEEE 754-2008) -> NaN only if both MayBeNaN
      // maximum: returns NaN if either operand is NaN (IEEE 754-2019) -> NaN if either MayBeNaN
      bool isMaximum = II->getIntrinsicID() == Intrinsic::maximum;
      bool mayBeNaN = !FMF.noNaNs() && (isMaximum ? (A.MayBeNaN || B.MayBeNaN) : (A.MayBeNaN && B.MayBeNaN));

      // maxnum returns the other operand when one is NaN, so all of that operand's values are possible too.
      auto Finish = [&](FPRange R) {
        if (!isMaximum && A.MayBeNaN)
          R = R.unionWith(B);
        if (!isMaximum && B.MayBeNaN)
          R = R.unionWith(A);
        return R.withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
      };

      // An Empty operand is NaN-only or unreachable: minimum/maximum then give NaN, minnum/maxnum the other operand.
      if (A.isEmptySet() || B.isEmptySet())
        return Finish(FPRange::getEmpty(Sem));
      if (A.isFullSet() && B.isFullSet())
        return Finish(FPRange::getFull(Sem));
      if (A.isFullSet())
        return Finish(FPRange::getRange(B.Lower, APFloat::getInf(Sem, false)));
      if (B.isFullSet())
        return Finish(FPRange::getRange(A.Lower, APFloat::getInf(Sem, false)));

      APFloat Lo = apfMax(A.Lower, B.Lower);
      APFloat Hi = apfMax(A.Upper, B.Upper);
      return Finish(FPRange::getRange(std::move(Lo), std::move(Hi)));
    }
    case Intrinsic::sin:
    case Intrinsic::cos: {
      // sin/cos always return [-1, 1]
      // sin/cos(+/-Inf) and sin/cos(NaN) produce NaN.
      if (FMF.noNaNs())
        return widenApprox(makeRange(-1.0, 1.0, Sem), /*AbsoluteBelowOne=*/true);
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      return widenApprox(makeRange(-1.0, 1.0, Sem), /*AbsoluteBelowOne=*/true).withNaN(In.MayBeNaN || In.isUnbounded());
    }
    case Intrinsic::exp:
    case Intrinsic::exp2: {
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      // exp/exp2 only produce NaN from NaN input (not from +/-Inf)
      bool mayBeNaN = !FMF.noNaNs() && In.MayBeNaN;

      if (In.isFullSet())
        return FPRange::getRange(APFloat::getZero(Sem), APFloat::getInf(Sem, false))
            .withNoInf(FMF.noInfs())
            .withNaN(mayBeNaN);

      // std::exp/exp2 naturally handle overflow (+Inf) and underflow (0)
      // APFloat conversion preserves these semantics
      double inLo = In.Lower.convertToDouble();
      double inHi = In.Upper.convertToDouble();
      bool isExp = II->getIntrinsicID() == Intrinsic::exp;

      double outLo = isExp ? std::exp(inLo) : std::exp2(inLo);
      double outHi = isExp ? std::exp(inHi) : std::exp2(inHi);
      return widenApprox(makeRange(outLo, outHi, Sem)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
    }
    case Intrinsic::log:
    case Intrinsic::log2:
    case Intrinsic::log10: {
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);

      // log(negative) = NaN, log(0) = -Inf (not NaN!)
      // NaN only if input may be negative (not just non-positive)
      bool mayBeNaN = !FMF.noNaNs() && (In.MayBeNaN || !In.isNonNegative());

      if (In.isFullSet())
        return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs());

      // Strictly negative input produces only NaN, no valid numeric output
      if (In.isStrictlyNegative())
        return FPRange::getEmpty(Sem).withNaN(!FMF.noNaNs());

      // Non-positive but not strictly negative means zero produces -Inf, but negative produces NaN
      // Since point infinities are not represented, treat result range as Full (unbounded)
      if (In.isNonPositive())
        return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);

      double hi = In.Upper.convertToDouble();
      // If Lo <= 0, the valid positive portion produces [-Inf, log(Hi)].
      // We compute bounds for the valid positive portion; NaN from negatives handled by mayBeNaN.
      if (!In.isStrictlyPositive()) {
        APFloat LogHi = makeAPFloat(II->getIntrinsicID() == Intrinsic::log    ? std::log(hi)
                                    : II->getIntrinsicID() == Intrinsic::log2 ? std::log2(hi)
                                                                              : std::log10(hi),
                                    Sem, APFloat::rmTowardPositive);
        return widenApprox(FPRange::getRange(APFloat::getInf(Sem, true), std::move(LogHi)), /*AbsoluteBelowOne=*/true)
            .withNoInf(FMF.noInfs())
            .withNaN(mayBeNaN);
      }

      // Lo > 0: compute [log(Lo), log(Hi)]
      double lo = In.Lower.convertToDouble();
      FPRange Result(Sem);
      switch (II->getIntrinsicID()) {
      case Intrinsic::log:
        Result = makeRange(std::log(lo), std::log(hi), Sem);
        break;
      case Intrinsic::log2:
        Result = makeRange(std::log2(lo), std::log2(hi), Sem);
        break;
      case Intrinsic::log10:
        Result = makeRange(std::log10(lo), std::log10(hi), Sem);
        break;
      default:
        break;
      }
      // Lo > 0 means no negatives in range, so NaN only from input propagation
      return widenApprox(Result, /*AbsoluteBelowOne=*/true).withNaN(!FMF.noNaNs() && In.MayBeNaN);
    }
    case Intrinsic::floor:
    case Intrinsic::ceil:
    case Intrinsic::trunc:
    case Intrinsic::round:
    case Intrinsic::roundeven: {
      FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      bool mayBeNaN = !FMF.noNaNs() && In.MayBeNaN;

      if (In.isFullSet())
        return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);

      // All these are monotonic, so [f(Lo), f(Hi)] is exact
      APFloat::roundingMode RM;
      switch (II->getIntrinsicID()) {
      case Intrinsic::floor:
        RM = APFloat::rmTowardNegative;
        break;
      case Intrinsic::ceil:
        RM = APFloat::rmTowardPositive;
        break;
      case Intrinsic::trunc:
        RM = APFloat::rmTowardZero;
        break;
      case Intrinsic::round:
        RM = APFloat::rmNearestTiesToAway;
        break;
      default:
        RM = APFloat::rmNearestTiesToEven;
        break; // roundeven
      }

      APFloat Lo = In.Lower;
      APFloat Hi = In.Upper;
      Lo.roundToIntegral(RM);
      Hi.roundToIntegral(RM);
      return FPRange::getRange(std::move(Lo), std::move(Hi)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
    }
    case Intrinsic::fma:
    case Intrinsic::fmuladd: {
      // fma(a, b, c) = a*b + c - from contract optimization
      FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
      FPRange C = computeFPRange(II->getArgOperand(2), Cache, Depth + 1);
      if (A.isFullSet() || B.isFullSet() || C.isFullSet()) {
        bool mayBeNaN = !FMF.noNaNs() && (A.MayBeNaN || B.MayBeNaN || C.MayBeNaN);
        return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
      }
      return A.mul(B, FMF).add(C, FMF);
    }
    case Intrinsic::powi: {
      FPRange Base = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
      bool mayBeNaN = !FMF.noNaNs() && Base.MayBeNaN;

      // IGC emits powi as pow, which vISA expands to exp2(log2(|x|) * n), unless an earlier pass folds it. For n = 0
      // the expansion gives exp2(-Inf * 0) = NaN for a zero or infinite base, and an unknown exponent may be 0.
      auto *ExpC = dyn_cast<ConstantInt>(II->getArgOperand(1));
      if (!ExpC)
        return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs());

      int exp = static_cast<int>(ExpC->getSExtValue());

      // exp = 0: 1.0, or NaN from the expansion.
      if (exp == 0) {
        const bool ZeroOrInf = Base.isFullSet() || Base.containsZero() || Base.isUnbounded();
        return FPRange::getPoint(makeAPFloat(1.0, Sem)).withNaN(mayBeNaN || (!FMF.noNaNs() && ZeroOrInf));
      }

      // exp = 1: x if folded, an approximation of |x| from the expansion.
      if (exp == 1)
        return Base.unionWith(widenApprox(Base.abs())).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);

      // Even positive exponent: result is non-negative [0, max(|base|)^exp]
      if (exp > 0 && exp % 2 == 0) {
        if (Base.isFullSet())
          return FPRange::getRange(APFloat::getZero(Sem), APFloat::getInf(Sem, false))
              .withNoInf(FMF.noInfs())
              .withNaN(mayBeNaN);
        double m = maxAbsValue(Base).convertToDouble();
        return widenApprox(makeRange(0.0, std::pow(m, exp), Sem)).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
      }
      return FPRange::getFull(Sem).withNoInf(FMF.noInfs()).withNaN(mayBeNaN);
    }
    default:
      // Unsupported intrinsic - return full range for now, with no knowledge of NaN
      return FPRange::getFull(Sem);
    }
  }
}

static FPRange analyzeBinaryOp(BinaryOperator *BO, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  FPRange L = computeFPRange(BO->getOperand(0), Cache, Depth + 1);
  FPRange R = computeFPRange(BO->getOperand(1), Cache, Depth + 1);
  FastMathFlags FMF = BO->getFastMathFlags();

  switch (BO->getOpcode()) {
  case Instruction::FAdd:
    return L.add(R, FMF);
  case Instruction::FSub:
    return L.sub(R, FMF);
  case Instruction::FMul: {
    // Check for square pattern x*x which guarantees non-negative result
    FPRange SquareResult(Sem);
    if (isSquarePattern(BO, Cache, Depth, SquareResult))
      return SquareResult.withNoInf(FMF.noInfs()).withNaN(!FMF.noNaNs() && SquareResult.MayBeNaN);
    return L.mul(R, FMF);
  }
  case Instruction::FDiv: {
    FPRange Quotient = widenApprox(L.div(R, FMF));
    // fdiv is lowered to mul(x, inv(y)), and inv(y) flushes to zero once |y| exceeds 1 / smallest normal.
    APFloat InvLimit = makeAPFloat(1.0, Sem);
    InvLimit.divide(APFloat::getSmallestNormalized(Sem), APFloat::rmTowardZero);
    if (Quotient.isBounded() && maxAbsValue(R).compare(InvLimit) == APFloat::cmpGreaterThan)
      Quotient = Quotient.unionWith(FPRange::getPoint(APFloat::getZero(Sem)));
    return Quotient;
  }
  default:
    // Unsupported binary operator (e.g., FRem) - return full range
    return FPRange::getFull(Sem);
  }
}

/// extractelement: follow the insertelement chain to the value written to this lane.
static FPRange analyzeExtractElement(ExtractElementInst *EE, FPRangeCache *Cache, unsigned Depth,
                                     const fltSemantics &Sem) {
  auto *CIdx = dyn_cast<ConstantInt>(EE->getIndexOperand());
  if (!CIdx)
    return FPRange::getFull(Sem);

  Value *Vec = EE->getVectorOperand();

  constexpr unsigned MaxSteps = 64;
  // Bound the walk; insertelement chains are short in practice, don't look past MaxSteps.
  for (unsigned Steps = 0; Steps < MaxSteps; ++Steps) {
    if (auto *IE = dyn_cast<InsertElementInst>(Vec)) {
      auto *IEConstIdx = dyn_cast<ConstantInt>(IE->getOperand(2));
      // A non-constant insert index may have clobbered the lane we want.
      if (!IEConstIdx)
        return FPRange::getFull(Sem);
      if (IEConstIdx->getZExtValue() == CIdx->getZExtValue())
        return computeFPRange(IE->getOperand(1), Cache, Depth + 1);
      // Different index - keep walking towards the base vector.
      Vec = IE->getOperand(0);
      continue;
    }

    if (auto *C = dyn_cast<Constant>(Vec)) {
      if (Constant *Elt = C->getAggregateElement(static_cast<unsigned>(CIdx->getZExtValue())))
        if (auto *EltFP = dyn_cast<ConstantFP>(Elt))
          return FPRange::getPoint(EltFP->getValueAPF());
    }

    return FPRange::getFull(Sem);
  }

  return FPRange::getFull(Sem);
}

static FPRange analyzeSelect(SelectInst *SI, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  FPRange T = computeFPRange(SI->getTrueValue(), Cache, Depth + 1);
  FPRange F = computeFPRange(SI->getFalseValue(), Cache, Depth + 1);
  return T.unionWith(F);
}

static FPRange analyzePHI(PHINode *PN, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  // Seed cache with full range to break cycles.
  // TODO: Consider iterating to fixpoint for tighter bounds on cyclic PHIs.
  if (Cache)
    (*Cache)[PN] = FPRange::getFull(Sem);

  FPRange Result = computeFPRange(PN->getIncomingValue(0), Cache, Depth + 1);
  for (unsigned i = 1; i < PN->getNumIncomingValues(); ++i) {
    FPRange In = computeFPRange(PN->getIncomingValue(i), Cache, Depth + 1);
    Result = Result.unionWith(In);
  }
  return Result;
}

//===----------------------------------------------------------------------===//
// Main Entry Point
//===----------------------------------------------------------------------===//

// This analysis uses interval arithmetic, which treats each use of a variable
// independently. For expressions like (a + a) - a, the result is NOT simplified
// to 'a'. Instead, each operand contributes its full range independently:
//   a + a = [lo, hi] + [lo, hi] = [2*lo, 2*hi]
//   (a + a) - a = [2*lo, 2*hi] - [lo, hi] = [2*lo - hi, 2*hi - lo]
// This conservative approximation is a fundamental property of interval
// arithmetic and is expected behavior, not a limitation to work around.
FPRange computeFPRange(Value *V, FPRangeCache *Cache, unsigned Depth) {
  // Only supported floating-point types can be analyzed.
  if (!isSupportedFPType(V->getType()))
    return FPRange::getFull(APFloat::IEEEsingle());

  const fltSemantics &Sem = getSemanticsForType(V->getType());

  if (Depth > IGC_GET_FLAG_VALUE(FPRangeAnalysisMaxDepth))
    return FPRange::getFull(Sem);

  if (Cache) {
    auto It = Cache->find(V);
    if (It != Cache->end())
      return It->second;
  }

  if (auto *C = dyn_cast<ConstantFP>(V)) {
    return FPRange::getPoint(C->getValueAPF());
  } else if (isa<UndefValue, PoisonValue, Argument>(V)) {
    // Unknown value: Full, which carries MayBeNaN.
    return FPRange::getFull(Sem);
  }

  FPRange Result = FPRange::getFull(Sem);

  if (auto *SI = dyn_cast<SIToFPInst>(V)) {
    Result = analyzeIntToFP(SI->getOperand(0), true, Sem);
  } else if (auto *UI = dyn_cast<UIToFPInst>(V)) {
    Result = analyzeIntToFP(UI->getOperand(0), false, Sem);
  } else if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    Result = analyzeIntrinsic(II, Cache, Depth, Sem);
  } else if (auto *BO = dyn_cast<BinaryOperator>(V)) {
    Result = analyzeBinaryOp(BO, Cache, Depth, Sem);
  } else if (auto *UO = dyn_cast<UnaryOperator>(V)) {
    if (UO->getOpcode() == Instruction::FNeg) {
      FPRange In = computeFPRange(UO->getOperand(0), Cache, Depth + 1);
      Result = In.negate(UO->getFastMathFlags());
    }
  } else if (auto *SI = dyn_cast<SelectInst>(V)) {
    Result = analyzeSelect(SI, Cache, Depth, Sem);
  } else if (auto *PN = dyn_cast<PHINode>(V)) {
    Result = analyzePHI(PN, Cache, Depth, Sem);
  } else if (auto *EE = dyn_cast<ExtractElementInst>(V)) {
    Result = analyzeExtractElement(EE, Cache, Depth, Sem);
  } else if (auto *Ext = dyn_cast<FPExtInst>(V)) {
    FPRange In = computeFPRange(Ext->getOperand(0), Cache, Depth + 1);
    if (In.isFullSet()) {
      Result = FPRange::getFull(Sem).withNaN(In.MayBeNaN);
    } else if (In.isEmptySet()) {
      Result = FPRange::getEmpty(Sem).withNaN(In.MayBeNaN);
    } else {
      // FPExt is lossless; convert bounds to wider semantics
      bool LosesInfo;
      APFloat Lo = In.Lower;
      APFloat Hi = In.Upper;
      Lo.convert(Sem, APFloat::rmTowardNegative, &LosesInfo);
      Hi.convert(Sem, APFloat::rmTowardPositive, &LosesInfo);
      Result = FPRange::getRange(std::move(Lo), std::move(Hi)).withNaN(In.MayBeNaN);
    }
  } else if (auto *Trunc = dyn_cast<FPTruncInst>(V)) {
    FPRange In = computeFPRange(Trunc->getOperand(0), Cache, Depth + 1);
    if (In.isFullSet()) {
      Result = FPRange::getFull(Sem).withNaN(In.MayBeNaN);
    } else if (In.isEmptySet()) {
      Result = FPRange::getEmpty(Sem).withNaN(In.MayBeNaN);
    } else {
      // FPTrunc may round or overflow
      bool LosesInfo;
      APFloat Lo = In.Lower;
      APFloat Hi = In.Upper;
      Lo.convert(Sem, APFloat::rmTowardNegative, &LosesInfo);
      Hi.convert(Sem, APFloat::rmTowardPositive, &LosesInfo);
      // If conversion overflowed to infinity, the range is still valid
      Result = FPRange::getRange(std::move(Lo), std::move(Hi)).withNaN(In.MayBeNaN);
    }
  }

  // Pattern detection: intersect with pattern bounds for tighter results.
  // Patterns provide semantic bounds (e.g., logistic is always in (0,1)), while instruction analysis provides
  // input-dependent bounds.
  // Taking the intersection gives the tightest sound approximation.
  // If intersection is empty (disjoint bounds from a bug or malformed IR), one side is wrong and it is not known
  // which, so fall back to their union.
  //
  // Propagate MayBeNaN from the instruction analysis. Math patterns provide bounds only.
  auto intersectOrFallback = [](const FPRange &Inst, const FPRange &Pattern) {
    FPRange I = Inst.intersectWith(Pattern);
    return (I.isEmptySet() ? Inst.unionWith(Pattern) : I).withNaN(Inst.MayBeNaN);
  };

  {
    FPRange R(Sem);
    if (isLogisticPattern(V, Cache, Depth, R)) {
      Result = intersectOrFallback(Result, R);
    } else if (isNormalizationPattern(V, Cache, Depth, R)) {
      Result = intersectOrFallback(Result, R);
    } else if (isSelectMinMaxPattern(V, Cache, Depth, R)) {
      Result = intersectOrFallback(Result, R);
    }
  }

  if (Cache)
    (*Cache)[V] = Result;

  return Result;
}

bool PrintFPRangeAnalysis::runOnFunction(Function &F) {
  FPRangeCache Cache;

  // Compute ranges for all FP instructions.
  for (auto &I : instructions(F)) {
    if (!I.getType()->isFloatingPointTy())
      continue;
    computeFPRange(&I, &Cache, 0);
  }

  OS << "FPRange results for function: " << F.getName() << "\n";

  for (auto &I : instructions(F)) {
    if (!I.getType()->isFloatingPointTy())
      continue;
    auto It = Cache.find(&I);
    if (It == Cache.end())
      continue;

    const FPRange &R = It->second;

    // Print instruction reference
    if (I.hasName())
      OS << "%" << I.getName();
    else
      I.printAsOperand(OS, false);
    OS << ": " << R << "\n";
  }

  return false;
}

FunctionPass *createPrintFPRangeAnalysisPass() { return new PrintFPRangeAnalysis(); }

} // namespace IGC

char PrintFPRangeAnalysis::ID = 0;

#define PASS_FLAG "print-fp-range-analysis"
#define PASS_DESC "Print computed floating-point ranges for FP instructions"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PrintFPRangeAnalysis, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PrintFPRangeAnalysis, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
