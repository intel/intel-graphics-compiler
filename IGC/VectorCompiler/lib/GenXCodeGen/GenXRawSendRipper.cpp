/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// GenXRawSendRipper
/// -----------------
///
/// This pass tears down a series of raw send chained through the old value
/// operand when it's safe.
//===----------------------------------------------------------------------===//
//

#define DEBUG_TYPE "GENX_RAWSENDRIPPER"
#include "GenX.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

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
