/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/StripSymbols.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/IPO/StripSymbols.h"
#include "llvmWrapper/Transforms/InitializePasses.h"

#include "Compiler/IGCPassSupport.h"

namespace IGCLLVM {

StripDeadDebugInfoLegacyPassWrapper::StripDeadDebugInfoLegacyPassWrapper() : ModulePass(ID) {
  initializeStripDeadDebugInfoLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool StripDeadDebugInfoLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  StripDeadDebugInfoPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

void StripDeadDebugInfoLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const { AU.setPreservesAll(); }

char StripDeadDebugInfoLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedStripDeadDebugInfoPass() { return new StripDeadDebugInfoLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "strip-legacy-wrapped"
#define PASS_DESCRIPTION "Strip all symbols from a module"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(StripDeadDebugInfoLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
