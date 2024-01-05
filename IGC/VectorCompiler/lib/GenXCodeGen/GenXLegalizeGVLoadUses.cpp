/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLegalizeGVLoadUses
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXUtil.h"

#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#include "GenXRegionUtils.h"

#define DEBUG_TYPE "GENX_LEGALIZE_GVLOAD_USES"

using namespace llvm;

class GenXLegalizeGVLoadUses : public ModulePass {
public:
  static char ID;
  explicit GenXLegalizeGVLoadUses() : ModulePass(ID) {}
  llvm::StringRef getPassName() const override {
    return "GenXLegalizeGVLoadUses";
  }
  bool runOnModule(Module &M) override;
};

bool GenXLegalizeGVLoadUses::runOnModule(Module &M) {
  bool Changed = false;
  for (auto &GV : M.globals()) {
    if (!GV.hasAttribute(genx::FunctionMD::GenXVolatile))
      continue;
    for (auto UI = GV.user_begin(), E = GV.user_end(); UI != E;)
      if (auto *I = dyn_cast<Instruction>(*UI++); I && genx::isAGVLoad(I))
        Changed |= genx::legalizeGVLoadForbiddenUsers(I);
  }
  return Changed;
}

char GenXLegalizeGVLoadUses::ID = 0;
namespace llvm {
void initializeGenXLegalizeGVLoadUsesPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXLegalizeGVLoadUses, "GenXLegalizeGVLoadUses", "GenXLegalizeGVLoadUses", false, false)
INITIALIZE_PASS_END(GenXLegalizeGVLoadUses, "GenXLegalizeGVLoadUses", "GenXLegalizeGVLoadUses", false, false)
ModulePass *llvm::createGenXLegalizeGVLoadUsesPass() {
  initializeGenXLegalizeGVLoadUsesPass(*PassRegistry::getPassRegistry());
  return new GenXLegalizeGVLoadUses;
}
