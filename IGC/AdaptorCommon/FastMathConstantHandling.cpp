/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Operator.h>
#include "common/LLVMWarningsPop.hpp"
#include "FastMathConstantHandling.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;
namespace IGC {

class FastMathConstantHandling : public FunctionPass, public InstVisitor<FastMathConstantHandling> {
public:
  FastMathConstantHandling();
  ~FastMathConstantHandling() {}
  static char ID;
  bool runOnFunction(Function &F) override;
  void visitInstruction(Instruction &I);
  void visitFNeg(Instruction &I);
  void visitFDiv(Instruction &I);
  virtual llvm::StringRef getPassName() const override { return "Fast Math Constant Handling"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
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

// This purpose of this pass is to catch bad fast flag management where we are seeing
// constants that are not matching the fast flags that are set on the instructions

void FastMathConstantHandling::visitInstruction(Instruction &I) {
  if (isa<FPMathOperator>(I)) {
    struct BoolSpecialConstants {
      bool hasInf = false;
      bool hasNan = false;
      bool hasNegZero = false;
    } BSC;

    for (auto &Op : I.operands()) {
      if (auto *fp_val = dyn_cast<llvm::ConstantFP>(Op)) {
        auto APF = fp_val->getValueAPF();
        BSC.hasInf |= APF.isInfinity();
        BSC.hasNan |= APF.isNaN();
        BSC.hasNegZero |= APF.isNegZero();
      }
    }

    if (BSC.hasInf)
      I.setHasNoInfs(false);

    if (BSC.hasNan)
      I.setHasNoNaNs(false);

    if (BSC.hasNegZero)
      I.setHasNoSignedZeros(false);
  }
}

void FastMathConstantHandling::visitFNeg(Instruction &I) {
  auto *fp_val = dyn_cast<llvm::ConstantFP>(I.getOperand(0));
  if (fp_val && fp_val->getValueAPF().isZero()) {
    for (auto *UI : I.users()) {
      if (isa<FPMathOperator>(UI))
        cast<Instruction>(UI)->setHasNoSignedZeros(false);
    }
  }
}

void FastMathConstantHandling::visitFDiv(Instruction &I) {
  if (auto *fp_val = dyn_cast<llvm::ConstantFP>(I.getOperand(1)); fp_val && fp_val->getValueAPF().isZero()) {
    I.setHasNoInfs(false);
  }
}

bool FastMathConstantHandling::runOnFunction(Function &F) {
  if (IGC_IS_FLAG_DISABLED(DisableFastMathConstantHandling)) {
    visit(F);
  }
  return true;
}

FunctionPass *createFastMathConstantHandling() { return new FastMathConstantHandling(); }

} // namespace IGC
