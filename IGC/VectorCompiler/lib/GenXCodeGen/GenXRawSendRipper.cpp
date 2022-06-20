/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXRawSendRipper
/// -----------------
///
/// This pass tears down a series of raw send chained through the old value
/// operand when it's safe.
//===----------------------------------------------------------------------===//
//
#include "GenX.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "GENX_RAWSENDRIPPER"

using namespace llvm;
using namespace genx;

namespace {

class GenXRawSendRipper : public FunctionPass {

public:
  static char ID;
  explicit GenXRawSendRipper() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "GenX RAW send ripper";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;
};

} // End anonymous namespace

namespace llvm {
void initializeGenXRawSendRipperPass(PassRegistry &);
} // End namespace llvm

char GenXRawSendRipper::ID = 0;
INITIALIZE_PASS(GenXRawSendRipper, "GenXRawSendRipper",
                "Rip chain of raw send", false, false)

FunctionPass *llvm::createGenXRawSendRipperPass() {
  initializeGenXRawSendRipperPass(*PassRegistry::getPassRegistry());
  return new GenXRawSendRipper();
}

bool GenXRawSendRipper::runOnFunction(Function &F) {
  bool Changed = false;
  Value *True = ConstantInt::getTrue(F.getContext());
  for (auto &BB : F)
    for (auto &I : BB) {
      if (GenXIntrinsic::getGenXIntrinsicID(&I) != GenXIntrinsic::genx_raw_send)
        continue;
      auto II = cast<IntrinsicInst>(&I);
      if (II->getOperand(1) != True)
        continue;
      Value *Old = II->getOperand(5);
      if (isa<UndefValue>(Old))
        continue;
      II->setOperand(5, UndefValue::get(Old->getType()));
    }
  return Changed;
}
