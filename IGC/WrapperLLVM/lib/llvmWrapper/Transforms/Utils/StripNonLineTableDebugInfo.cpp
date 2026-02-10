/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Utils/StripNonLineTableDebugInfo.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Utils/StripNonLineTableDebugInfo.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

namespace IGCLLVM {

StripNonLineTableDebugLegacyPassWrapper::StripNonLineTableDebugLegacyPassWrapper() : ModulePass(ID) {
  initializeStripNonLineTableDebugLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool StripNonLineTableDebugLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;

  StripNonLineTableDebugInfoPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

void StripNonLineTableDebugLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const { AU.setPreservesAll(); }

char StripNonLineTableDebugLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedStripNonLineTableDebugPass() { return new StripNonLineTableDebugLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "strip-nonlinetable-debuginfo-legacy-wrapped"
#define PASS_DESCRIPTION "Strip Unused Function Prototypes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(StripNonLineTableDebugLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
