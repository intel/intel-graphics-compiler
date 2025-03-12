/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"

#include "llvm/IR/InstVisitor.h"

#define DEBUG_TYPE "GENX_UNFREEZE"

using namespace llvm;

namespace {
class GenXUnfreeze : public FunctionPass, public InstVisitor<GenXUnfreeze> {
  bool Changed = false;

public:
  static char ID;
  explicit GenXUnfreeze() : FunctionPass(ID) {}

  StringRef getPassName() const override { return "GenX unfreeze"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  void visitFreezeInst(FreezeInst &FI);

  bool runOnFunction(Function &F) override;
};

} // namespace

char GenXUnfreeze::ID = 0;

namespace llvm {
void initializeGenXUnfreezePass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXUnfreeze, "GenXUnfreeze", "GenXUnfreeze", false,
                      false)
INITIALIZE_PASS_END(GenXUnfreeze, "GenXUnfreeze", "GenXUnfreeze", false, false)

FunctionPass *llvm::createGenXUnfreezePass() {
  initializeGenXUnfreezePass(*PassRegistry::getPassRegistry());
  return new GenXUnfreeze();
}

void GenXUnfreeze::visitFreezeInst(FreezeInst &FI) {
  if (FI.use_empty())
    return;
  auto *V = FI.getOperand(0);
  FI.replaceAllUsesWith(V);
  FI.eraseFromParent();
  Changed = true;
}

bool GenXUnfreeze::runOnFunction(Function &F) {
  visit(F);
  return Changed;
}
