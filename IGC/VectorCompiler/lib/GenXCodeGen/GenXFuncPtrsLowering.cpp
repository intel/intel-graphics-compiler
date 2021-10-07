/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///
/// GenXFunctionPointersLowering
/// ---------------------
///
/// Most of pass functionality was moved to GenXGlobalValueLowering.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXUtil.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <deque>

using namespace llvm;
using namespace genx;

namespace {

class GenXFunctionPointersLowering : public ModulePass {
public:
  static char ID;
  explicit GenXFunctionPointersLowering();
  StringRef getPassName() const override {
    return "GenX function pointers lowering";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

} // namespace

char GenXFunctionPointersLowering::ID = 0;
namespace llvm {
void initializeGenXFunctionPointersLoweringPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXFunctionPointersLowering,
                      "GenXFunctionPointersLowering",
                      "GenXFunctionPointersLowering", false, false)
INITIALIZE_PASS_END(GenXFunctionPointersLowering,
                    "GenXFunctionPointersLowering",
                    "GenXFunctionPointersLowering", false, false)

GenXFunctionPointersLowering::GenXFunctionPointersLowering() : ModulePass(ID) {
  initializeGenXFunctionPointersLoweringPass(*PassRegistry::getPassRegistry());
}

ModulePass *llvm::createGenXFunctionPointersLoweringPass() {
  return new GenXFunctionPointersLowering();
}

void GenXFunctionPointersLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

bool GenXFunctionPointersLowering::runOnModule(Module &M) {
  bool Modified = false;

  for (auto &F : M)
    if (F.hasAddressTaken()) {
      F.addFnAttr(genx::FunctionMD::CMStackCall);
      F.addFnAttr(genx::FunctionMD::ReferencedIndirectly);
    }

  return Modified;
}
