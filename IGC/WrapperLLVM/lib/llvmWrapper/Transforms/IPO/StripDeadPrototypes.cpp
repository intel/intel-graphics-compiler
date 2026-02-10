/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/StripDeadPrototypes.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/IPO/StripDeadPrototypes.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

namespace IGCLLVM {

StripDeadPrototypesLegacyPassWrapper::StripDeadPrototypesLegacyPassWrapper() : ModulePass(ID) {
  initializeStripDeadPrototypesLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool StripDeadPrototypesLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  StripDeadPrototypesPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char StripDeadPrototypesLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedStripDeadPrototypesPass() { return new StripDeadPrototypesLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "strip-dead-prototypes-legacy-wrapped"
#define PASS_DESCRIPTION "Strip Unused Function Prototypes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(StripDeadPrototypesLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
