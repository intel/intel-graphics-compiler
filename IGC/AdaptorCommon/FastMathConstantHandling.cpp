/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "FastMathConstantHandling.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace IGC {

// A fast-math flag is a claim about the values an instruction will see or produce: 'ninf' claims no infinity, 'nnan' no
// NaN, 'nsz' that the sign of a zero carries no meaning. Frontends set these and they are narrowed down later.
// Contraditctions are visible once the value in question is a constant (the constants are added/folded at various
// stages of the compilation). This pass evaluates the constant expressions and judges the flags against what that
// leaves behind.
//
// This is important because later LLVM and IGC passes rely on the flags to optimize code. There are at least two ways
// that a false claim can cause damage:
//   - a wrong value. e.g. 'nsz' lets InstCombine fold `fmul nnan nsz X, -0.0` to +0.0, dropping a sign that mattered.
//   - missing code. e.g. NaN or infinity operands marked with 'nnan' or 'ninf' make LLVM fold the whole instruction to
//   poison, which then spreads to everything derived from it. Once poison reaches a branch condition the block itself
//   is at risk, and on LLVM 16+ the block and its side effects are dropped outright.
class FastMathConstantHandling : public FunctionPass, public InstVisitor<FastMathConstantHandling> {
public:
  FastMathConstantHandling();
  ~FastMathConstantHandling() {}
  static char ID;
  bool runOnFunction(Function &F) override;
  void visitInstruction(Instruction &I);
  void visitFDiv(Instruction &I);
  void visitFNeg(Instruction &I);
  virtual llvm::StringRef getPassName() const override { return "Fast Math Constant Handling"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

private:
  void foldConstantExpressions(Function &F);
  void clearNoInfs(Instruction &I);
  void clearNoNaNs(Instruction &I);
  void clearNoSignedZeros(Instruction &I);

  bool m_Changed = false;
};

#define PASS_FLAG "FastMathConstantHandling"
#define PASS_DESC "Fast Math Constant Handling"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FastMathConstantHandling, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FastMathConstantHandling, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char FastMathConstantHandling::ID = 0;

FastMathConstantHandling::FastMathConstantHandling() : FunctionPass(ID) {
  initializeFastMathConstantHandlingPass(*PassRegistry::getPassRegistry());
}

void FastMathConstantHandling::foldConstantExpressions(Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();

  for (auto &BB : F) {
    for (auto &I : make_early_inc_range(BB)) {
      // Only floating point results are of interest, since inf, NaN and negative zero are the only values that can
      // contradict a fast-math flag.
      if (!I.getType()->isFPOrFPVectorTy())
        continue;

      auto *folded = ConstantFoldInstruction(&I, DL);
      if (!folded)
        continue;

      I.replaceAllUsesWith(folded);
      if (isInstructionTriviallyDead(&I)) {
        salvageDebugInfo(I);
        I.eraseFromParent();
      }
      m_Changed = true;
    }
  }
}

void FastMathConstantHandling::clearNoInfs(Instruction &I) {
  if (I.hasNoInfs()) {
    I.setHasNoInfs(false);
    m_Changed = true;
  }
}

void FastMathConstantHandling::clearNoNaNs(Instruction &I) {
  if (I.hasNoNaNs()) {
    I.setHasNoNaNs(false);
    m_Changed = true;
  }
}

void FastMathConstantHandling::clearNoSignedZeros(Instruction &I) {
  if (I.hasNoSignedZeros()) {
    I.setHasNoSignedZeros(false);
    m_Changed = true;
  }
}

// Rule 1: an operand that already is an infinity, a NaN or a negative zero contradicts the matching flag on the
// instruction consuming it.
void FastMathConstantHandling::visitInstruction(Instruction &I) {
  if (!isa<FPMathOperator>(I))
    return;

  // These are the predicates LLVM's own instruction simplification uses to decide that an operand contradicts the
  // flags (simplifyFPOp in InstructionSimplify.cpp). They match splat vectors as well, which matters because this pass
  // runs before the IR is scalarized.
  for (auto *Op : I.operand_values()) {
    if (match(Op, m_Inf()))
      clearNoInfs(I);

    if (match(Op, m_NaN()))
      clearNoNaNs(I);

    if (match(Op, m_NegZeroFP()))
      clearNoSignedZeros(I);
  }
}

// Rule 2: a division by zero produces an infinity that none of the operands shows, so rule 1 cannot see it and ninf
// has to come off the fdiv itself.
void FastMathConstantHandling::visitFDiv(Instruction &I) {
  visitInstruction(I);

  if (match(I.getOperand(1), m_AnyZeroFP()))
    clearNoInfs(I);
}

// Rule 3: negating a zero produces the opposite-signed zero. Nothing is wrong with the fneg itself - it is the
// instructions consuming it that must not ignore that sign.
void FastMathConstantHandling::visitFNeg(Instruction &I) {
  visitInstruction(I);

  if (!match(I.getOperand(0), m_AnyZeroFP()))
    return;

  for (auto *user : I.users()) {
    auto *userInst = dyn_cast<Instruction>(user);
    if (userInst && isa<FPMathOperator>(userInst))
      clearNoSignedZeros(*userInst);
  }
}

bool FastMathConstantHandling::runOnFunction(Function &F) {
  if (IGC_IS_FLAG_ENABLED(DisableFastMathConstantHandling))
    return false;

  m_Changed = false;

  // Fold implied values without fast-math flags to avoid invalid transformations from false flags.
  foldConstantExpressions(F);

  // Judge the flags against those constants and drop the ones that are now provably false.
  visit(F);

  return m_Changed;
}

FunctionPass *createFastMathConstantHandling() { return new FastMathConstantHandling(); }

} // namespace IGC
