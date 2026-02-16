/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Transforms/Utils/ScalarEvolutionExpander.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
namespace SCEVUtils {

/// Represents a deconstructed SCEV expression { start, +, step }.
/// Start SCEV is the initial value (can be calculated in loop preheader),
/// Step SCEV is the increment per iteration.
struct DeconstructedSCEV {
  DeconstructedSCEV() : Start(nullptr), Step(nullptr), ConvertedMulExpr(false) {}

  bool isValid() const { return Start != nullptr && Step != nullptr; }

  bool isStepZero() const { return Step != nullptr && Step->isZero(); }

  bool hasStep() const { return Step != nullptr && !Step->isZero(); }

  void set(const llvm::SCEV *S, const llvm::SCEV *St) {
    Start = S;
    Step = St;
  }

  const llvm::SCEV *getStart() const { return Start; }
  const llvm::SCEV *getStep() const { return Step; }

  /// The starting value of the SCEV expression
  const llvm::SCEV *Start;

  /// The step/increment value per iteration
  const llvm::SCEV *Step;

  /// True if input SCEV:
  ///   x * { start, +, step }
  /// Was converted into:
  ///   { x * start, +, x * step }
  bool ConvertedMulExpr;
};

/// Takes SCEV expression returned by ScalarEvolution and deconstructs it into
/// expected format { start, +, step }. Returns true if expressions can be
/// parsed and reduced.
///
/// Handles the following SCEV types:
/// - SCEVAddRecExpr: Direct { start, +, step } expressions
/// - SCEVZeroExtendExpr/SCEVSignExtendExpr: Unwraps and preserves extension
/// - SCEVAddExpr: Converts x + { start, +, step } to { x + start, +, step }
/// - SCEVMulExpr: Converts x * { start, +, step } to { x*start, +, x*step }
/// - Loop-invariant expressions: Returns { expr, +, 0 }
///
/// @param S      The SCEV expression to deconstruct
/// @param SE     ScalarEvolution analysis
/// @param L      The loop context
/// @param E      SCEVExpander for checking if expansion is safe
/// @param Result Output parameter for the deconstructed SCEV
/// @return true if successfully deconstructed, false otherwise
bool deconstructSCEV(const llvm::SCEV *S, llvm::ScalarEvolution &SE, llvm::Loop *L, llvm::SCEVExpander &E,
                     DeconstructedSCEV &Result);

/// If SCEV is zext/sext, drop the extension and return the inner SCEV.
const llvm::SCEV *dropExt(const llvm::SCEV *S);

/// Returns true if SCEV expression is legal (not SCEVCouldNotCompute and
/// uses only legal integer types).
bool isValidSCEV(const llvm::SCEV *S);

/// Compares two SCEV expressions for equality.
/// Handles special cases like constants with different bit widths.
bool isEqualSCEV(const llvm::SCEV *A, const llvm::SCEV *B);

/// Helper class to build SCEVAddExpr with automatic type extension.
/// Function ScalarEvolution::getAddExpr requires all operands to have the same type.
/// This class wraps ScalarEvolution::getAddExpr, extending operands if needed.
class SCEVAddBuilder {
public:
  SCEVAddBuilder(llvm::ScalarEvolution &SE, bool DropExt = false) : SE(SE), DropExt(DropExt) {}

  SCEVAddBuilder &add(const llvm::SCEV *S, bool Negative = false);
  SCEVAddBuilder &addNegative(const llvm::SCEV *S) { return add(S, true); }

  const llvm::SCEV *build();

private:
  struct Op {
    Op(const llvm::SCEV *S, bool Negative) : S(S), Negative(Negative) {}
    const llvm::SCEV *S;
    bool Negative;
  };

  llvm::ScalarEvolution &SE;
  llvm::SmallVector<Op, 16> Ops;
  bool DropExt;
};

/// Helper class to build SCEVMulExpr with automatic type extension.
/// Function ScalarEvolution::getMulExpr requires all operands to have the same type.
/// This class wraps ScalarEvolution::getMulExpr, extending operands if needed.
class SCEVMulBuilder {
public:
  SCEVMulBuilder(llvm::ScalarEvolution &SE) : SE(SE) {}

  SCEVMulBuilder &add(const llvm::SCEV *S);

  const llvm::SCEV *build();

private:
  llvm::ScalarEvolution &SE;
  llvm::SmallVector<const llvm::SCEV *, 4> Ops;
};

} // namespace SCEVUtils
} // namespace IGC
