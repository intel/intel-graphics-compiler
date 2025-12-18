/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/GenX/Region.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Pass.h>

using namespace llvm;

namespace {
class GenXStatePointerFence final : public FunctionPass {
public:
  static char ID;

  GenXStatePointerFence() : FunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  StringRef getPassName() const override { return "GenX State Pointer Fence"; }

  bool runOnFunction(Function &F) override;

private:
  bool processStatePointer(Value *V);
};
} // namespace

char GenXStatePointerFence::ID = 0;

INITIALIZE_PASS_BEGIN(GenXStatePointerFence, "GenXStatePointerFence",
                      "GenXStatePointerFence", false, false);
INITIALIZE_PASS_END(GenXStatePointerFence, "GenXStatePointerFence",
                    "GenXStatePointerFence", false, false);

namespace llvm {
FunctionPass *createGenXStatePointerFencePass() {
  initializeGenXStatePointerFencePass(*PassRegistry::getPassRegistry());
  return new GenXStatePointerFence();
}
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses GenXStatePointerFencePass::run(Function &F,
                                                 FunctionAnalysisManager &AM) {
  GenXStatePointerFence GenXPoinerFence;
  if (GenXPoinerFence.runOnFunction(F))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif

bool GenXStatePointerFence::runOnFunction(Function &F) {
  using namespace vc::InternalIntrinsic;
  bool Changed = false;

  for (auto &I : instructions(F)) {
    auto IID = vc::getAnyIntrinsicID(&I);

    auto SurfaceOperand = getMemorySurfaceOperandIndex(IID);
    if (SurfaceOperand < 0)
      continue;

    Changed |= processStatePointer(I.getOperand(SurfaceOperand));

    auto SamplerOperand = getMemorySamplerOperandIndex(IID);
    if (SamplerOperand < 0)
      continue;

    Changed |= processStatePointer(I.getOperand(SamplerOperand));
  }

  return Changed;
}

bool GenXStatePointerFence::processStatePointer(Value *V) {
  constexpr unsigned FenceIID = vc::InternalIntrinsic::optimization_fence;

  auto *Trunc = dyn_cast<TruncInst>(V);
  if (!Trunc)
    return false;

  auto *SrcTy = Trunc->getSrcTy();
  auto *DstTy = Trunc->getDestTy();

  if (!SrcTy->isIntegerTy(64) || !DstTy->isIntegerTy(32))
    return false;

  IRBuilder<> Builder(Trunc);

  auto *Src = Trunc->getOperand(0);
  if (vc::getAnyIntrinsicID(Src) == FenceIID)
    return false;

  auto *F = vc::getAnyDeclaration(Trunc->getModule(), FenceIID, {SrcTy});
  auto *Fence = Builder.CreateCall(F, {Src});
  Trunc->setOperand(0, Fence);

  return true;
}
