/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "predefined-constant-resolver"
#include "Compiler/CISACodeGen/ResolvePredefinedConstant.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Analysis/ConstantFolding.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Shared, stateless implementation used by both the legacy and new-pass-manager wrappers.
static bool resolvePredefinedConstants(Module &M) {
  bool Changed = false;
  const DataLayout &DL = M.getDataLayout();

  for (auto &GV : M.globals()) {
    if (!GV.isConstant() || !GV.hasUniqueInitializer())
      continue;

    Constant *C = GV.getInitializer();
    for (auto I = GV.user_begin(); I != GV.user_end(); /* empty */) {
      LoadInst *LI = dyn_cast<LoadInst>(*I++);
      if (!LI)
        continue;

      if (Constant *Folded = ConstantFoldLoadFromConst(C, LI->getType(), DL)) {
        LI->replaceAllUsesWith(Folded);
        LI->eraseFromParent();
        Changed = true;
      }
    }
  }
  return Changed;
}

namespace {
// Legacy Pass Manager wrapper.
class PredefinedConstantResolvingLPM : public ModulePass {
public:
  static char ID;

  PredefinedConstantResolvingLPM() : ModulePass(ID) {
    initializePredefinedConstantResolvingLPMPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override { return resolvePredefinedConstants(M); }

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
};
} // End anonymous namespace

ModulePass *IGC::createResolvePredefinedConstantPass() { return new PredefinedConstantResolvingLPM(); }

char PredefinedConstantResolvingLPM::ID = 0;

#define PASS_FLAG "igc-predefined-constant-resolve"
#define PASS_DESC "Resolve compiler predefined constants"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(PredefinedConstantResolvingLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PredefinedConstantResolvingLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses PredefinedConstantResolvingNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = resolvePredefinedConstants(M);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
