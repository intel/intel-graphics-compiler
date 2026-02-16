/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/SCEVUtils/SCEVUtils.hpp"
#include "llvmWrapper/Transforms/Utils/ScalarEvolutionExpander.h"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;

namespace IGC {
namespace SCEVUtils {

const SCEV *dropExt(const SCEV *S) {
  while (true) {
    if (auto *Zext = dyn_cast<SCEVZeroExtendExpr>(S))
      S = Zext->getOperand();
    else if (auto *Sext = dyn_cast<SCEVSignExtendExpr>(S))
      S = Sext->getOperand();
    else
      break;
  }
  return S;
}

bool isValidSCEV(const SCEV *S) {
  if (isa<SCEVCouldNotCompute>(S))
    return false;

  // Scalar Evolution doesn't have SCEV expression for bitwise-and. Instead,
  // if possible, SE produces expressions for any integer size, leaving cleanup
  // to legalization pass.
  // By default don't allow illegal integer types.
  if (IGC_IS_FLAG_ENABLED(EnableGEPLSRAnyIntBitWidth))
    return true;

  auto IsInvalidInt = [](Type *Ty) {
    if (!Ty->isIntegerTy())
      return false;

    auto bits = Ty->getScalarSizeInBits();
    switch (bits) {
    case 8:
    case 16:
    case 32:
    case 64:
      return false;
    default:
      return bits > 8;
    }
  };

  bool HasInvalidInt = SCEVExprContains(S, [&](const SCEV *S) {
    if (auto *Cast = dyn_cast<SCEVCastExpr>(S))
      return IsInvalidInt(Cast->getOperand()->getType()) || IsInvalidInt(Cast->getType());
    return false;
  });

  return !HasInvalidInt;
}

bool isEqualSCEV(const SCEV *A, const SCEV *B) {
  // Scalar Evolution keeps unique SCEV instances, so we can compare pointers.
  if (A == B)
    return true;

  if (A->getSCEVType() != B->getSCEVType())
    return false;

  switch (A->getSCEVType()) {
  case scConstant:
    // Can be different bit width, but same integer value.
    return cast<SCEVConstant>(A)->getValue()->getSExtValue() == cast<SCEVConstant>(B)->getValue()->getSExtValue();
  default:
    return false;
  }
}

SCEVAddBuilder &SCEVAddBuilder::add(const SCEV *S, bool Negative) {
  IGC_ASSERT(S->getType()->isIntegerTy());

  // strip extend
  if (DropExt)
    S = dropExt(S);

  if (auto *Expr = dyn_cast<SCEVAddExpr>(S)) {
    for (auto *Op : Expr->operands())
      add(Op, Negative);
    return *this;
  }

  Ops.emplace_back(S, Negative);
  return *this;
}

const SCEV *SCEVAddBuilder::build() {
  // ScalarEvolution::getAddExpr requires all operands to have the same
  // type. First find the widest type.
  Type *T = nullptr;
  for (auto It = Ops.begin(); It != Ops.end(); ++It) {
    T = T ? SE.getWiderType(T, It->S->getType()) : It->S->getType();
  }

  // Join list of operands, extending type if required.
  SmallVector<const SCEV *, 16> FinalOps;

  for (auto It = Ops.begin(); It != Ops.end(); ++It) {
    const SCEV *S = It->S;
    S = S->getType() == T ? S : SE.getSignExtendExpr(S, T);
    FinalOps.push_back(It->Negative ? SE.getNegativeSCEV(S) : S);
  }

  return SE.getAddExpr(FinalOps);
}

SCEVMulBuilder &SCEVMulBuilder::add(const SCEV *S) {
  IGC_ASSERT(S->getType()->isIntegerTy());
  Ops.emplace_back(S);
  return *this;
}

const SCEV *SCEVMulBuilder::build() {
  // ScalarEvolution::getMulExpr requires all operands to have the same
  // type. First find the widest type.
  Type *T = nullptr;
  for (auto S : Ops) {
    T = T ? SE.getWiderType(T, S->getType()) : S->getType();
  }

  // Join list of operands, extending type if required.
  SmallVector<const SCEV *, 4> FinalOps;

  for (auto S : Ops) {
    FinalOps.push_back(S->getType() == T ? S : SE.getSignExtendExpr(S, T));
  }

  return SE.getMulExpr(FinalOps);
}

// Recursive helper function for deconstructSCEV
static bool deconstructSCEVImpl(const SCEV *S, ScalarEvolution &SE, Loop *L, SCEVExpander &E,
                                DeconstructedSCEV &Result) {
  // In case of ext instruction analyze nested content.
  if (isa<SCEVZeroExtendExpr>(S) || isa<SCEVSignExtendExpr>(S)) {
    if (!deconstructSCEVImpl(dyn_cast<SCEVCastExpr>(S)->getOperand(), SE, L, E, Result))
      return false;

    if (S->getType() != Result.Start->getType())
      Result.Start = isa<SCEVSignExtendExpr>(S) ? SE.getSignExtendExpr(Result.Start, S->getType())
                                                : SE.getZeroExtendExpr(Result.Start, S->getType());

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L->getLoopPreheader()->back(), &SE, &E);
  }

  // First check if expression can be fully expanded in preheader. If so, no need
  // to process further, but instead treat expression as:
  //   { start, +, 0 }
  // This will do LICM-like reduction moving value to preheader, without adding new
  // induction variable.
  if (SE.isLoopInvariant(S, L)) {
    Result.Start = S;
    Result.Step = SE.getConstant(Type::getInt64Ty(L->getHeader()->getContext()), 0);
    return true;
  }

  // Expect SCEV expression:
  //   { start, +, step }
  // where step is constant
  if (auto *Add = dyn_cast<SCEVAddRecExpr>(S)) {
    if (!Add->isAffine())
      return false;

    if (Add->getNumOperands() != 2)
      return false;

    // Scalar Evolution can produce SCEVAddRecExpr based on boolean type, for example:
    //   {(true + (trunc i16 %localIdX to i1)),+,true}
    // Ignore such expressions.
    Type *Ty = Add->getStart()->getType();
    if (Ty->isIntegerTy() && Ty->getScalarSizeInBits() == 1)
      return false;

    const SCEV *OpStep = Add->getOperand(1);

    // Step must be constant in loop's body.
    if (!SE.isLoopInvariant(OpStep, L))
      return false;

    Result.Start = Add->getStart();
    Result.Step = OpStep;

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L->getLoopPreheader()->back(), &SE, &E);
  }

  // If expression is:
  //   x + { start, +, step }
  // then change it to:
  //   { start + x, +, step }
  if (auto *Add = dyn_cast<SCEVAddExpr>(S)) {
    // There can be only one expression with step != 0.
    Result.Step = SE.getConstant(Type::getInt64Ty(L->getHeader()->getContext()), 0);

    SCEVAddBuilder Builder(SE);

    for (auto *Op : Add->operands()) {
      DeconstructedSCEV OpResult;

      if (!deconstructSCEVImpl(Op, SE, L, E, OpResult))
        return false;

      if (!OpResult.Step->isZero()) {
        if (!Result.Step->isZero())
          return false; // unsupported expression with multiple steps
        Result.Step = OpResult.Step;
      }

      Builder.add(OpResult.Start);
    }

    Result.Start = Builder.build();

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L->getLoopPreheader()->back(), &SE, &E);
  }

  // If expression is:
  //   x * { start, +, step }
  // then change it to:
  //   { x * start, +, x * step }
  if (auto *Mul = dyn_cast<SCEVMulExpr>(S)) {
    bool FoundAddRec = false;
    SCEVMulBuilder StartBuilder(SE), StepBuilder(SE);

    for (auto *Op : Mul->operands()) {
      DeconstructedSCEV OpResult;
      if (!deconstructSCEVImpl(Op, SE, L, E, OpResult))
        return false;

      if (OpResult.Step->isZero()) {
        StartBuilder.add(OpResult.Start);
        StepBuilder.add(OpResult.Start);
      } else {
        if (FoundAddRec)
          return false; // unsupported expression with multiple SCEVAddRecExpr
        FoundAddRec = true;

        StartBuilder.add(OpResult.Start);
        StepBuilder.add(OpResult.Step);
      }
    }

    if (!FoundAddRec)
      return false;

    Result.Start = StartBuilder.build();
    Result.Step = StepBuilder.build();
    Result.ConvertedMulExpr = true;

    if (!SE.isLoopInvariant(Result.Step, L))
      return false;

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L->getLoopPreheader()->back(), &SE, &E);
  }

  return false;
}

bool deconstructSCEV(const SCEV *S, ScalarEvolution &SE, Loop *L, SCEVExpander &E, DeconstructedSCEV &Result) {
  if (!L || !L->getLoopPreheader())
    return false;

  return deconstructSCEVImpl(S, SE, L, E, Result);
}

} // namespace SCEVUtils
} // namespace IGC
