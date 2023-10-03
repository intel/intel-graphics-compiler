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
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "genx-raw-send-ripper"

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

  for (auto &I : instructions(F)) {
    auto IID = vc::getAnyIntrinsicID(&I);
    int PredIdx = -1;
    int PassthruIdx = -1;

    switch (IID) {
    default:
      continue;
    case GenXIntrinsic::genx_raw_send:
      PredIdx = 1;
      PassthruIdx = 5;
      break;
    case GenXIntrinsic::genx_raw_sends:
      PredIdx = 1;
      PassthruIdx = 7;
      break;
    case GenXIntrinsic::genx_raw_send2:
      PredIdx = 2;
      PassthruIdx = 9;
      break;
    case GenXIntrinsic::genx_raw_sends2:
      PredIdx = 2;
      PassthruIdx = 11;
      break;
    }

    IGC_ASSERT_EXIT(PredIdx >= 0 && PassthruIdx >= 0);
    auto *Pred = dyn_cast<Constant>(I.getOperand(PredIdx));
    auto *Passthru = I.getOperand(PassthruIdx);

    if (!Pred || !Pred->getType()->isIntOrIntVectorTy(1) ||
        !Pred->isAllOnesValue() || isa<UndefValue>(Passthru))
      continue;

    I.setOperand(PassthruIdx, UndefValue::get(Passthru->getType()));
    Changed = true;
  }

  return Changed;
}
