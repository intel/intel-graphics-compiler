/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FPRangeAnalysis.cpp - Floating Point Range Tracking Utilities
//
// This analysis is aware of certain fast-math transformations:
//
//   arcp:     a/b      -> a * (1/b)   [handled by matchDivision]
//   contract: a*b+c    -> fma(a,b,c)  [handled in pattern matchers]
//
// Commutative operands are checked in both orders to handle reassoc reordering.
//
//===----------------------------------------------------------------------===//

#include "FPRangeAnalysis.hpp"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/KnownBits.h>
#include "common/LLVMWarningsPop.hpp"

#include <cmath>
#include <limits>
#include <optional>

using namespace llvm;

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

static const fltSemantics &getSemanticsForType(Type *Ty) {
  if (Ty->isFloatTy())
    return APFloat::IEEEsingle();
  if (Ty->isDoubleTy())
    return APFloat::IEEEdouble();
  if (Ty->isHalfTy())
    return APFloat::IEEEhalf();
  return APFloat::IEEEsingle();
}

//===----------------------------------------------------------------------===//
// Fast-Math Aware Helpers
//===----------------------------------------------------------------------===//

/// Try to identify a division a/b even if transformed by arcp to a * (1/b).
/// Returns true if matched, with Num and Denom set to the numerator and denominator.
static bool matchDivision(Value *V, Value *&Num, Value *&Denom) {
  auto *BO = dyn_cast<BinaryOperator>(V);
  if (!BO)
    return false;

  // Direct division: a / b
  if (BO->getOpcode() == Instruction::FDiv) {
    Num = BO->getOperand(0);
    Denom = BO->getOperand(1);
    return true;
  }

  // arcp transform: a * (1/b) where 1/b is fdiv 1.0, b
  // This is needed for isNormalizationPattern which can't be derived from
  // simple range arithmetic (it requires recognizing x in x/sqrt(x^2+...))
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
    Intrinsic::ID ID = II->getIntrinsicID();
    return ID == Intrinsic::exp || ID == Intrinsic::exp2;
  }
  return false;
}

//===----------------------------------------------------------------------===//
// Exported Helper Functions
//===----------------------------------------------------------------------===//

bool isWithin(const FPRange &Range, double lo, double hi) {
  if (Range.isFullSet() || Range.isEmptySet() || Range.containsNaN())
    return false;

  const fltSemantics &Sem = Range.getLower().getSemantics();
  APFloat LoAP = makeAPFloat(lo, Sem, APFloat::rmTowardNegative);
  APFloat HiAP = makeAPFloat(hi, Sem, APFloat::rmTowardPositive);

  auto loCmp = Range.getLower().compare(LoAP);
  auto hiCmp = Range.getUpper().compare(HiAP);
  return (loCmp == APFloat::cmpGreaterThan || loCmp == APFloat::cmpEqual) &&
         (hiCmp == APFloat::cmpLessThan || hiCmp == APFloat::cmpEqual);
}

double maxAbsValue(const FPRange &Range) {
  if (Range.isFullSet() || Range.isEmptySet() || Range.isNaNOnly())
    return std::numeric_limits<double>::infinity();

  // Note: convertToDouble() may lose precision for values outside double's
  // representable range, but infinity is returned in such cases which is
  // a safe conservative bound.
  APFloat AbsLower = Range.getLower();
  APFloat AbsUpper = Range.getUpper();
  if (AbsLower.isNegative())
    AbsLower.changeSign();
  if (AbsUpper.isNegative())
    AbsUpper.changeSign();

  return apfMax(AbsLower, AbsUpper).convertToDouble();
}

FPRange makeRange(double lo, double hi, const fltSemantics &Sem) {
  APFloat LoAP = makeAPFloat(lo, Sem, APFloat::rmTowardNegative);
  APFloat HiAP = makeAPFloat(hi, Sem, APFloat::rmTowardPositive);

  if (LoAP.compare(HiAP) == APFloat::cmpGreaterThan)
    return FPRange::getEmpty(Sem);

  return FPRange::getNonNaN(std::move(LoAP), std::move(HiAP));
}

//===----------------------------------------------------------------------===//
// Pattern Detection (Fast-Math Aware)
//===----------------------------------------------------------------------===//

/// Detect logistic-style patterns: 1/(1+exp(...)) or exp(...)/(1+exp(...))
/// These patterns always produce values in (0, 1) regardless of the exp argument.
/// Includes sigmoid sigma(x) = 1/(1+exp(-x)) = exp(x)/(1+exp(x)) as a special case.
/// Also handles arcp-transformed versions.
static bool isLogisticPattern(Value *V) {
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
        // Pattern 1: numerator is 1.0 -> 1/(1+exp(...)) in (0, 1)
        if (auto *NumC = dyn_cast<ConstantFP>(Num)) {
          if (NumC->isExactlyValue(1.0))
            return true;
        }
        // Pattern 2: numerator is exp(...) -> exp(...)/(1+exp(...)) in (0, 1)
        if (isExpIntrinsic(Num))
          return true;
      }
    }
  }
  return false;
}

/// Detect softplus pattern: log(1 + exp(x))
static bool isSoftplusPattern(Value *V) {
  auto *II = dyn_cast<IntrinsicInst>(V);
  if (!II)
    return false;
  Intrinsic::ID ID = II->getIntrinsicID();
  if (ID != Intrinsic::log && ID != Intrinsic::log2)
    return false;

  auto *Add = dyn_cast<BinaryOperator>(II->getArgOperand(0));
  if (!Add || Add->getOpcode() != Instruction::FAdd)
    return false;

  Value *AddL = Add->getOperand(0);
  Value *AddR = Add->getOperand(1);

  // Look for 1.0 and exp() in either order
  for (int i = 0; i < 2; ++i) {
    Value *MaybeOne = (i == 0) ? AddL : AddR;
    Value *MaybeExp = (i == 0) ? AddR : AddL;

    if (auto *C = dyn_cast<ConstantFP>(MaybeOne)) {
      if (C->isExactlyValue(1.0) && isExpIntrinsic(MaybeExp))
        return true;
    }
  }
  return false;
}

/// Detect clamp pattern: min(max(x, lo), hi) or max(min(x, hi), lo)
static std::optional<std::pair<APFloat, APFloat>> matchClampPattern(Value *V) {
  auto isMinMax = [](Intrinsic::ID ID, bool &isMin) {
    isMin = (ID == Intrinsic::minnum || ID == Intrinsic::minimum);
    return isMin || ID == Intrinsic::maxnum || ID == Intrinsic::maximum;
  };

  auto *Outer = dyn_cast<IntrinsicInst>(V);
  if (!Outer)
    return std::nullopt;

  bool outerIsMin;
  if (!isMinMax(Outer->getIntrinsicID(), outerIsMin))
    return std::nullopt;

  // Check both operand orders due to reassoc
  for (int i = 0; i < 2; ++i) {
    auto *Inner = dyn_cast<IntrinsicInst>(Outer->getArgOperand(i));
    auto *OuterC = dyn_cast<ConstantFP>(Outer->getArgOperand(1 - i));
    if (!Inner || !OuterC)
      continue;

    bool innerIsMin;
    if (!isMinMax(Inner->getIntrinsicID(), innerIsMin) || outerIsMin == innerIsMin)
      continue;

    for (int j = 0; j < 2; ++j) {
      if (auto *InnerC = dyn_cast<ConstantFP>(Inner->getArgOperand(j))) {
        APFloat lo = outerIsMin ? InnerC->getValueAPF() : OuterC->getValueAPF();
        APFloat hi = outerIsMin ? OuterC->getValueAPF() : InnerC->getValueAPF();
        auto cmp = lo.compare(hi);
        if (cmp == APFloat::cmpLessThan || cmp == APFloat::cmpEqual)
          return std::make_pair(lo, hi);
      }
    }
  }
  return std::nullopt;
}

/// Detect ReLU pattern: max(x, 0)
/// Also handles select-based ReLU from conditional lowering
static bool isReLUPattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result, const fltSemantics &Sem) {
  APFloat Zero = APFloat::getZero(Sem);

  // max(x, 0) or max(0, x)
  if (auto *Call = dyn_cast<IntrinsicInst>(V)) {
    Intrinsic::ID ID = Call->getIntrinsicID();
    if (ID == Intrinsic::maxnum || ID == Intrinsic::maximum) {
      for (int i = 0; i < 2; ++i) {
        if (auto *C = dyn_cast<ConstantFP>(Call->getArgOperand(i))) {
          if (C->isZero()) {
            FPRange In = computeFPRange(Call->getArgOperand(1 - i), Cache, Depth + 1);
            if (In.isFullSet()) {
              Result = FPRange::getNonNaN(Zero, APFloat::getInf(Sem, false));
            } else {
              Result = FPRange::getNonNaN(Zero, apfMax(Zero, In.getUpper()));
            }
            return true;
          }
        }
      }
    }
  }

  // select(fcmp x > 0, x, 0) or similar
  if (auto *Sel = dyn_cast<SelectInst>(V)) {
    if (isa<FCmpInst>(Sel->getCondition())) {
      auto *ZT = dyn_cast<ConstantFP>(Sel->getTrueValue());
      auto *ZF = dyn_cast<ConstantFP>(Sel->getFalseValue());
      bool zeroTrue = ZT && ZT->isZero();
      bool zeroFalse = ZF && ZF->isZero();
      if (zeroTrue || zeroFalse) {
        Value *X = zeroTrue ? Sel->getFalseValue() : Sel->getTrueValue();
        FPRange In = computeFPRange(X, Cache, Depth + 1);
        if (In.isFullSet()) {
          Result = FPRange::getNonNaN(Zero, APFloat::getInf(Sem, false));
        } else {
          Result = FPRange::getNonNaN(Zero, apfMax(Zero, In.getUpper()));
        }
        return true;
      }
    }
  }
  return false;
}

/// Detect Leaky ReLU: select(x > 0, x, alpha * x) where 0 < alpha < 1
static bool isLeakyReLUPattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result,
                               const fltSemantics &Sem) {
  auto *Sel = dyn_cast<SelectInst>(V);
  if (!Sel || !isa<FCmpInst>(Sel->getCondition()))
    return false;

  // Look for alpha * x in either branch
  auto findAlphaMul = [&](Value *V) -> std::pair<const ConstantFP *, Value *> {
    auto *Mul = dyn_cast<BinaryOperator>(V);
    if (!Mul || Mul->getOpcode() != Instruction::FMul)
      return {nullptr, nullptr};

    for (int i = 0; i < 2; ++i) {
      if (auto *C = dyn_cast<ConstantFP>(Mul->getOperand(i))) {
        const APFloat &a = C->getValueAPF();
        APFloat Zero = APFloat::getZero(a.getSemantics());
        APFloat One = makeAPFloat(1.0, a.getSemantics());
        if (a.compare(Zero) == APFloat::cmpGreaterThan && a.compare(One) == APFloat::cmpLessThan) {
          return {C, Mul->getOperand(1 - i)};
        }
      }
    }
    return {nullptr, nullptr};
  };

  auto [Alpha1, Input1] = findAlphaMul(Sel->getFalseValue());
  auto [Alpha2, Input2] = findAlphaMul(Sel->getTrueValue());

  const ConstantFP *Alpha = Alpha1 ? Alpha1 : Alpha2;
  Value *Input = Alpha1 ? Input1 : Input2;

  if (!Alpha)
    return false;

  FPRange In = computeFPRange(Input, Cache, Depth + 1);
  if (In.isFullSet())
    return false;

  APFloat Zero = APFloat::getZero(Sem);
  APFloat OutLo = Zero;
  if (In.getLower().compare(Zero) == APFloat::cmpLessThan) {
    OutLo = In.getLower();
    OutLo.multiply(Alpha->getValueAPF(), APFloat::rmTowardNegative);
  }
  Result = FPRange::getNonNaN(OutLo, apfMax(Zero, In.getUpper()));
  return true;
}

/// Detect normalization: x / sqrt(x^2 + ...) which is bounded to [-1, 1]
/// Handles arcp transform: x * (1 / sqrt(...))
static bool isNormalizationPattern(Value *V) {
  Value *Num = nullptr, *Denom = nullptr;
  if (!matchDivision(V, Num, Denom))
    return false;

  // Check if denominator is sqrt(...)
  auto *Sqrt = dyn_cast<IntrinsicInst>(Denom);
  if (!Sqrt || Sqrt->getIntrinsicID() != Intrinsic::sqrt)
    return false;

  Value *SqrtArg = Sqrt->getArgOperand(0);

  // Look for Num^2 in the sqrt argument (sum of squares pattern)
  // Use C++20 recursive lambda pattern to avoid std::function overhead
  auto hasSquare = [&](auto &&self, Value *V) -> bool {
    auto *M = dyn_cast<BinaryOperator>(V);
    if (!M)
      return false;
    if (M->getOpcode() == Instruction::FMul && M->getOperand(0) == Num && M->getOperand(1) == Num)
      return true;
    if (M->getOpcode() == Instruction::FAdd)
      return self(self, M->getOperand(0)) || self(self, M->getOperand(1));
    // Handle fma(Num, Num, ...) from contract optimization
    if (auto *FMA = dyn_cast<IntrinsicInst>(V)) {
      if (FMA->getIntrinsicID() == Intrinsic::fma || FMA->getIntrinsicID() == Intrinsic::fmuladd) {
        if (FMA->getArgOperand(0) == Num && FMA->getArgOperand(1) == Num)
          return true;
        // Recurse into the addend
        return self(self, FMA->getArgOperand(2));
      }
    }
    return false;
  };
  return hasSquare(hasSquare, SqrtArg);
}

/// Detect square pattern: x * x
/// Result is always non-negative, and if input doesn't span zero, lower bound is tighter
static bool isSquarePattern(Value *V, FPRangeCache *Cache, unsigned Depth, FPRange &Result, const fltSemantics &Sem) {
  auto *Mul = dyn_cast<BinaryOperator>(V);
  if (!Mul || Mul->getOpcode() != Instruction::FMul)
    return false;
  if (Mul->getOperand(0) != Mul->getOperand(1))
    return false;

  FPRange In = computeFPRange(Mul->getOperand(0), Cache, Depth + 1);
  if (In.isFullSet())
    return false;

  double lo = In.getLower().convertToDouble();
  double hi = In.getUpper().convertToDouble();

  double resultLo = 0.0;
  double resultHi;

  if (lo >= 0.0) {
    // Input is non-negative: [lo, hi] -> [lo^2, hi^2]
    resultLo = lo * lo;
    resultHi = hi * hi;
  } else if (hi <= 0.0) {
    // Input is non-positive: [lo, hi] -> [hi^2, lo^2]
    resultLo = hi * hi;
    resultHi = lo * lo;
  } else {
    // Input spans zero: [lo, hi] -> [0, max(lo^2, hi^2)]
    resultHi = std::fmax(lo * lo, hi * hi);
  }

  Result = makeRange(resultLo, resultHi, Sem);
  return true;
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

  // Note: Converting to double may lose precision for integers > 2^53,
  // but this is acceptable for range analysis purposes as the bounds
  // remain conservative (the actual value is still within the range).
  if (Signed) {
    int64_t Lo = Known.getMinValue().getSExtValue();
    int64_t Hi = Known.getMaxValue().getSExtValue();
    if (Lo > Hi) {
      Lo = APInt::getSignedMinValue(Width).getSExtValue();
      Hi = APInt::getSignedMaxValue(Width).getSExtValue();
    }
    return makeRange(double(Lo), double(Hi), Sem);
  }
  uint64_t Lo = Known.getMinValue().getZExtValue();
  uint64_t Hi = Known.getMaxValue().getZExtValue();
  if (Lo > Hi || Known.isUnknown()) {
    Lo = 0;
    Hi = APInt::getMaxValue(Width).getZExtValue();
  }
  return makeRange(double(Lo), double(Hi), Sem);
}

static FPRange analyzeIntrinsic(IntrinsicInst *II, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  switch (II->getIntrinsicID()) {
  case Intrinsic::fabs: {
    FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    return In.abs();
  }
  case Intrinsic::sqrt: {
    FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    if (In.isFullSet())
      return FPRange::getFull(Sem);
    APFloat Zero = APFloat::getZero(Sem);
    auto cmp = In.getLower().compare(Zero);
    if (cmp == APFloat::cmpGreaterThan || cmp == APFloat::cmpEqual)
      return makeRange(0.0, std::sqrt(In.getUpper().convertToDouble()), Sem);
    return FPRange::getFull(Sem);
  }
  case Intrinsic::minnum:
  case Intrinsic::minimum: {
    FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
    if (A.isFullSet() && B.isFullSet())
      return FPRange::getFull(Sem);
    if (A.isFullSet())
      return FPRange::getNonNaN(APFloat::getInf(Sem, true), B.getUpper());
    if (B.isFullSet())
      return FPRange::getNonNaN(APFloat::getInf(Sem, true), A.getUpper());
    APFloat Lo = apfMin(A.getLower(), B.getLower());
    APFloat Hi = apfMin(A.getUpper(), B.getUpper());
    if (!Lo.isFinite() || !Hi.isFinite())
      return FPRange::getFull(Sem);
    return FPRange::getNonNaN(Lo, Hi);
  }
  case Intrinsic::maxnum:
  case Intrinsic::maximum: {
    FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
    if (A.isFullSet() && B.isFullSet())
      return FPRange::getFull(Sem);
    if (A.isFullSet())
      return FPRange::getNonNaN(B.getLower(), APFloat::getInf(Sem, false));
    if (B.isFullSet())
      return FPRange::getNonNaN(A.getLower(), APFloat::getInf(Sem, false));
    APFloat Lo = apfMax(A.getLower(), B.getLower());
    APFloat Hi = apfMax(A.getUpper(), B.getUpper());
    if (!Lo.isFinite() || !Hi.isFinite())
      return FPRange::getFull(Sem);
    return FPRange::getNonNaN(Lo, Hi);
  }
  case Intrinsic::copysign: {
    FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    if (A.isFullSet())
      return FPRange::getFull(Sem);
    double m = maxAbsValue(A);
    return makeRange(-m, m, Sem);
  }
  case Intrinsic::sin:
  case Intrinsic::cos:
    return makeRange(-1.0, 1.0, Sem);
  case Intrinsic::exp:
  case Intrinsic::exp2: {
    FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    APFloat Zero = APFloat::getZero(Sem);
    if (In.isFullSet())
      return FPRange::getNonNaN(Zero, APFloat::getInf(Sem, false));
    double lo = std::fmax(In.getLower().convertToDouble(), -700.0);
    double hi = std::fmin(In.getUpper().convertToDouble(), 700.0);
    bool isExp = II->getIntrinsicID() == Intrinsic::exp;
    return makeRange(isExp ? std::exp(lo) : std::exp2(lo), isExp ? std::exp(hi) : std::exp2(hi), Sem);
  }
  case Intrinsic::log:
  case Intrinsic::log2:
  case Intrinsic::log10: {
    FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    if (In.isFullSet())
      return FPRange::getFull(Sem);
    APFloat Zero = APFloat::getZero(Sem);
    auto cmp = In.getLower().compare(Zero);
    if (cmp == APFloat::cmpLessThan || cmp == APFloat::cmpEqual)
      return FPRange::getFull(Sem);
    double lo = In.getLower().convertToDouble();
    double hi = In.getUpper().convertToDouble();
    switch (II->getIntrinsicID()) {
    case Intrinsic::log:
      return makeRange(std::log(lo), std::log(hi), Sem);
    case Intrinsic::log2:
      return makeRange(std::log2(lo), std::log2(hi), Sem);
    case Intrinsic::log10:
      return makeRange(std::log10(lo), std::log10(hi), Sem);
    default:
      break;
    }
    return FPRange::getFull(Sem);
  }
  case Intrinsic::floor:
  case Intrinsic::ceil:
  case Intrinsic::trunc:
  case Intrinsic::round:
  case Intrinsic::roundeven:
  case Intrinsic::rint:
  case Intrinsic::nearbyint: {
    FPRange In = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    if (In.isFullSet())
      return FPRange::getFull(Sem);
    double lo = In.getLower().convertToDouble();
    double hi = In.getUpper().convertToDouble();
    return makeRange(std::floor(lo), std::ceil(hi), Sem);
  }
  case Intrinsic::fma:
  case Intrinsic::fmuladd: {
    // fma(a, b, c) = a*b + c - from contract optimization
    FPRange A = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    FPRange B = computeFPRange(II->getArgOperand(1), Cache, Depth + 1);
    FPRange C = computeFPRange(II->getArgOperand(2), Cache, Depth + 1);
    if (A.isFullSet() || B.isFullSet() || C.isFullSet())
      return FPRange::getFull(Sem);
    return A.mul(B).add(C);
  }
  case Intrinsic::pow:
  case Intrinsic::powi: {
    // x^n where n is even positive integer -> result >= 0
    FPRange Base = computeFPRange(II->getArgOperand(0), Cache, Depth + 1);
    if (auto *ExpC = dyn_cast<ConstantFP>(II->getArgOperand(1))) {
      double exp = ExpC->getValueAPF().convertToDouble();
      double intpart;
      if (std::modf(exp, &intpart) == 0.0) {
        int expInt = static_cast<int>(intpart);
        if (expInt > 0 && expInt % 2 == 0) {
          // Even positive power: result is non-negative
          if (Base.isFullSet())
            return FPRange::getNonNaN(APFloat::getZero(Sem), APFloat::getInf(Sem, false));
          double m = maxAbsValue(Base);
          return makeRange(0.0, std::pow(m, expInt), Sem);
        }
      }
    }
    if (II->getIntrinsicID() == Intrinsic::powi) {
      if (auto *ExpI = dyn_cast<ConstantInt>(II->getArgOperand(1))) {
        int64_t exp = ExpI->getSExtValue();
        if (exp > 0 && exp % 2 == 0) {
          if (Base.isFullSet())
            return FPRange::getNonNaN(APFloat::getZero(Sem), APFloat::getInf(Sem, false));
          double m = maxAbsValue(Base);
          return makeRange(0.0, std::pow(m, exp), Sem);
        }
      }
    }
    return FPRange::getFull(Sem);
  }
  default:
    return FPRange::getFull(Sem);
  }
}

static FPRange analyzeBinaryOp(BinaryOperator *BO, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  FPRange L = computeFPRange(BO->getOperand(0), Cache, Depth + 1);
  FPRange R = computeFPRange(BO->getOperand(1), Cache, Depth + 1);

  // Early exit if either operand is unbounded
  if (L.isFullSet() || R.isFullSet()) {
    // Special case: FRem bounds result by divisor even if dividend is unbounded
    if (BO->getOpcode() == Instruction::FRem && !R.isFullSet()) {
      double m = maxAbsValue(R);
      return makeRange(-m, m, Sem);
    }
    return FPRange::getFull(Sem);
  }

  switch (BO->getOpcode()) {
  case Instruction::FAdd:
    return L.add(R);
  case Instruction::FSub:
    return L.sub(R);
  case Instruction::FMul:
    return L.mul(R);
  case Instruction::FDiv:
    return L.div(R);
  case Instruction::FRem: {
    // FRem result has same sign as dividend and magnitude < |divisor|
    double m = maxAbsValue(R);
    APFloat Zero = APFloat::getZero(Sem);
    auto loCmp = L.getLower().compare(Zero);
    auto hiCmp = L.getUpper().compare(Zero);

    if (loCmp == APFloat::cmpGreaterThan || loCmp == APFloat::cmpEqual) {
      // Dividend is non-negative: result in [0, min(upper, m))
      double upperBound = std::fmin(L.getUpper().convertToDouble(), m);
      return makeRange(0.0, upperBound, Sem);
    }
    if (hiCmp == APFloat::cmpLessThan || hiCmp == APFloat::cmpEqual) {
      // Dividend is non-positive: result in (-min(|lower|, m), 0]
      double lowerBound = -std::fmin(-L.getLower().convertToDouble(), m);
      return makeRange(lowerBound, 0.0, Sem);
    }
    // Dividend spans zero: result in (-m, m)
    return makeRange(-m, m, Sem);
  }
  default:
    return FPRange::getFull(Sem);
  }
}

static FPRange analyzeSelect(SelectInst *SI, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  if (!SI->getType()->isFloatingPointTy())
    return FPRange::getFull(Sem);

  FPRange T = computeFPRange(SI->getTrueValue(), Cache, Depth + 1);
  if (T.isFullSet())
    return FPRange::getFull(Sem);

  FPRange F = computeFPRange(SI->getFalseValue(), Cache, Depth + 1);
  if (F.isFullSet())
    return FPRange::getFull(Sem);

  return T.unionWith(F);
}

static FPRange analyzePHI(PHINode *PN, FPRangeCache *Cache, unsigned Depth, const fltSemantics &Sem) {
  if (!PN->getType()->isFloatingPointTy())
    return FPRange::getFull(Sem);

  if (Cache)
    (*Cache)[PN] = FPRange::getFull(Sem);

  FPRange Result = computeFPRange(PN->getIncomingValue(0), Cache, Depth + 1);
  if (Result.isFullSet())
    return FPRange::getFull(Sem);

  for (unsigned i = 1; i < PN->getNumIncomingValues(); ++i) {
    FPRange In = computeFPRange(PN->getIncomingValue(i), Cache, Depth + 1);
    if (In.isFullSet())
      return FPRange::getFull(Sem);
    Result = Result.unionWith(In);
  }
  return Result;
}

//===----------------------------------------------------------------------===//
// Main Entry Point
//===----------------------------------------------------------------------===//

FPRange computeFPRange(Value *V, FPRangeCache *Cache, unsigned Depth) {
  const fltSemantics &Sem =
      V->getType()->isFloatingPointTy() ? getSemanticsForType(V->getType()) : APFloat::IEEEsingle();

  if (Depth > IGC_GET_FLAG_VALUE(FPRangeAnalysisMaxDepth))
    return FPRange::getFull(Sem);

  if (Cache) {
    auto It = Cache->find(V);
    if (It != Cache->end())
      return It->second;
  }

  FPRange Result = FPRange::getFull(Sem);

  if (auto *C = dyn_cast<ConstantFP>(V)) {
    Result = FPRange(C->getValueAPF());
  } else if (isa<UndefValue>(V) || isa<PoisonValue>(V) || isa<Argument>(V)) {
    Result = FPRange::getFull(Sem);
  } else if (auto *SI = dyn_cast<SIToFPInst>(V)) {
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
      Result = In.negate();
    }
  } else if (auto *SI = dyn_cast<SelectInst>(V)) {
    Result = analyzeSelect(SI, Cache, Depth, Sem);
  } else if (auto *PN = dyn_cast<PHINode>(V)) {
    Result = analyzePHI(PN, Cache, Depth, Sem);
  } else if (auto *Ext = dyn_cast<FPExtInst>(V)) {
    Result = computeFPRange(Ext->getOperand(0), Cache, Depth + 1);
  } else if (auto *Trunc = dyn_cast<FPTruncInst>(V)) {
    Result = computeFPRange(Trunc->getOperand(0), Cache, Depth + 1);
  }

  // High-level pattern detection (fast-math aware)
  // Only run pattern detection if we haven't determined a bounded range yet
  if (Result.isFullSet()) {
    if (isLogisticPattern(V)) {
      Result = makeRange(0.0, 1.0, Sem);
    } else if (isSoftplusPattern(V)) {
      Result = FPRange::getNonNaN(APFloat::getZero(Sem), APFloat::getInf(Sem, false));
    } else if (auto Clamp = matchClampPattern(V)) {
      Result = FPRange::getNonNaN(Clamp->first, Clamp->second);
    } else if (isNormalizationPattern(V)) {
      Result = makeRange(-1.0, 1.0, Sem);
    } else {
      FPRange R;
      if (isReLUPattern(V, Cache, Depth, R, Sem)) {
        Result = R;
      } else if (isLeakyReLUPattern(V, Cache, Depth, R, Sem)) {
        Result = R;
      } else if (isSquarePattern(V, Cache, Depth, R, Sem)) {
        Result = R;
      }
    }
  }

  if (Cache)
    (*Cache)[V] = Result;

  return Result;
}

} // namespace IGC
