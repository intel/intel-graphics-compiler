/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/ElimAvailExtern.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/ElimAvailExtern.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

EliminateAvailableExternallyLegacyPassWrapper::EliminateAvailableExternallyLegacyPassWrapper() : ModulePass(ID) {
  initializeEliminateAvailableExternallyLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool EliminateAvailableExternallyLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  EliminateAvailableExternallyPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char EliminateAvailableExternallyLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedEliminateAvailableExternallyPass() {
  return new EliminateAvailableExternallyLegacyPassWrapper();
}
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "elim-avail-extern-legacy-wrapped"
#define PASS_DESCRIPTION "Eliminate Available Externally Globals LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(EliminateAvailableExternallyLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                    PASS_ANALYSIS)
