/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "Probe/Assertion.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "vc-linkage-corruptor"

using namespace llvm;

namespace {
struct GenXLinkageCorruptor final : public ModulePass {
  static char ID;
  GenXLinkageCorruptor() : ModulePass(ID) {}
  StringRef getPassName() const override { return "GenX linkage corruptor"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};
} // namespace

char GenXLinkageCorruptor::ID = 0;

INITIALIZE_PASS_BEGIN(GenXLinkageCorruptor, "GenXLinkageCorruptor",
                      "GenXLinkageCorruptor", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXLinkageCorruptor, "GenXLinkageCorruptor",
                    "GenXLinkageCorruptor", false, false)

ModulePass *llvm::createGenXLinkageCorruptorPass() {
  initializeGenXLinkageCorruptorPass(*PassRegistry::getPassRegistry());
  return new GenXLinkageCorruptor;
}

void GenXLinkageCorruptor::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

bool GenXLinkageCorruptor::runOnModule(Module &M) {
  auto &&BCfg = getAnalysis<GenXBackendConfig>();
  FunctionControl FCtrl = BCfg.getFCtrl();
  bool SaveStackCallLinkage = BCfg.saveStackCallLinkage();
  bool Changed = false;

  for (auto &F : M.getFunctionList()) {
    if (F.isDeclaration() || F.hasDLLExportStorageClass())
      continue;
    if (GenXIntrinsic::getAnyIntrinsicID(&F) !=
        GenXIntrinsic::not_any_intrinsic)
      continue;
    // __cm_intrinsic_impl_* could be used for emulation mul/div etc
    if (F.getName().contains("__cm_intrinsic_impl_"))
      continue;

    // Indirect functions are always stack calls.
    if (F.hasAddressTaken()) {
      F.addFnAttr(genx::FunctionMD::CMStackCall);
      Changed = true;
      IGC_ASSERT(vc::isIndirect(F));
    }

    // Convert non-kernel to stack call if applicable
    if (FCtrl == FunctionControl::StackCall && !vc::requiresStackCall(&F)) {
      LLVM_DEBUG(dbgs() << "Adding stack call to: " << F.getName() << "\n");
      F.addFnAttr(genx::FunctionMD::CMStackCall);
      Changed = true;
    }

    // Do not change stack calls linkage as we may have both types of stack
    // calls.
    if (vc::requiresStackCall(&F) && SaveStackCallLinkage)
      continue;

    F.setLinkage(GlobalValue::InternalLinkage);
    Changed = true;
  }

  return Changed;
}
