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

#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#include "GenXRegionUtils.h"

#define DEBUG_TYPE "GENX_LEGALIZE_GVLOAD_USERS"

using namespace llvm;

class GenXLegalizeGVLoadUses : public FunctionPass
{
public:
  static char ID;
  explicit GenXLegalizeGVLoadUses() : FunctionPass(ID) {}
  llvm::StringRef getPassName() const override { return "Fix illegal gvload users."; }
  bool runOnFunction(Function& F) override;
};

bool GenXLegalizeGVLoadUses::runOnFunction(Function& F) {
  bool Changed = false;
  for (auto &BB : F)
    for (auto II = BB.begin(); II != BB.end();) {
      auto *Inst = &*II++;
      if (genx::isAGVLoad(Inst))
        Changed |= genx::legalizeGVLoadForbiddenUses(Inst);
    }
  return Changed;
}

char GenXLegalizeGVLoadUses::ID = 0;
namespace llvm {
void initializeGenXLegalizeGVLoadUsesPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXLegalizeGVLoadUses, "GenXLegalizeGVLoadUses", "GenXLegalizeGVLoadUses", false, false)
INITIALIZE_PASS_END(GenXLegalizeGVLoadUses, "GenXLegalizeGVLoadUses", "GenXLegalizeGVLoadUses", false, false)
FunctionPass *llvm::createGenXLegalizeGVLoadUsesPass() {
  initializeGenXLegalizeGVLoadUsesPass(*PassRegistry::getPassRegistry());
  return new GenXLegalizeGVLoadUses;
}
