/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class Constant;
class ConstantExpr;
class ConstantVector;
class Instruction;
class ConstantStruct;
} // namespace llvm

namespace IGC {
/// @brief  This pass breaks constant expressions appearing
///         in instructions into instruction sequences.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BreakConstantExpr {
public:
  BreakConstantExpr() {}
  ~BreakConstantExpr() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "BreakConstantExprPass"; }

  /// @brief  Main entry point.
  /// @param  F The destination function.
  bool run(llvm::Function &F);

protected:
  /// @brief  Recursively break up a constant expression by creating instructions
  ///         for each sub-expression.
  ///         The newly created instructions are placed before the user, and
  ///         replace the constantexprs.
  /// @param  expr          The expression to break up.
  /// @param  user          The original user of the expression.
  void breakExpressions(llvm::ConstantExpr *expr, int operandIndex, llvm::Instruction *user);

  /// @brief  Break up constant expressions in a ConstantVector elements by creating instructions
  ///         for each sub-expression.
  ///         The newly created instructions are placed before the user, and replace all constant
  ///         expressions and the constant vector.
  /// @param  cvec          Constant vector with expressions to break up.
  /// @param  operandIndex  Index of the constant vector operand in the parent instruction
  /// @param  user          The original user of the expression.
  bool breakExpressionsInVector(llvm::ConstantVector *cvec, int operandIndex, llvm::Instruction *user);

  /// @brief  Break up constant structure by creating a non constant
  ///         structure and replacing all its constant operands by instructions.
  /// @param  cs            Constant structure to break up
  /// @param  operandIndex  Index of the constant vector operand in the parent instruction
  /// @param  user          The original user of the expression.
  bool breakConstantStruct(llvm::ConstantStruct *cs, int operandIndex, llvm::Instruction *user);

  /// @brief  Replaces input constant expression or constant vector with a new instruction.
  /// @param  exprOrVec     Constant vector or expressions to replace.
  /// @param  newInst       Instruction to replace the constant with
  /// @param  operandIndex  Index of the constant vector operand in the parent instruction
  /// @param  user          The original user of the expression.
  void replaceConstantWith(llvm::Constant *exprOrVec, llvm::Instruction *newInst, int operandIndex,
                           llvm::Instruction *user);

private:
  bool hasConstantExpr(llvm::ConstantVector *cvec) const;
  bool hasConstantExpr(llvm::ConstantStruct *cstruct) const;
};

// Legacy Pass Manager wrapper.
class BreakConstantExprLPM : public llvm::FunctionPass {
public:
  static char ID;

  BreakConstantExprLPM();
  ~BreakConstantExprLPM() {}

  llvm::StringRef getPassName() const override { return BreakConstantExpr::getPassName(); }

  bool runOnFunction(llvm::Function &F) override { return BreakConstantExpr().run(F); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class BreakConstantExprNPM : public llvm::PassInfoMixin<BreakConstantExprNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-break-const-expr"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
