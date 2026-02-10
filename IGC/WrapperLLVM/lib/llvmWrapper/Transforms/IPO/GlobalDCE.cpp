/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

GlobalDCELegacyPassWrapper::GlobalDCELegacyPassWrapper() : ModulePass(ID) {
  initializeGlobalDCELegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool GlobalDCELegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  GlobalDCEPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char GlobalDCELegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedGlobalDCEPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new GlobalDCELegacyPassWrapper();
#else
  return llvm::createGlobalDCEPass();
#endif
}
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "globaldce-legacy-wrapped"
#define PASS_DESCRIPTION "Dead Global Elimination LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(GlobalDCELegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
