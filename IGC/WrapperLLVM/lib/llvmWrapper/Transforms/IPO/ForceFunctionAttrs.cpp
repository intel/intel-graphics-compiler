/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/ForceFunctionAttrs.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

ForceFunctionAttrsLegacyPassWrapper::ForceFunctionAttrsLegacyPassWrapper() : ModulePass(ID) {
  initializeForceFunctionAttrsLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool ForceFunctionAttrsLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  ForceFunctionAttrsPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char ForceFunctionAttrsLegacyPassWrapper::ID = 0;
Pass *createLegacyWrappedForceFunctionAttrsPass() { return new ForceFunctionAttrsLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "forceattrs-legacy-wrapped"
#define PASS_DESCRIPTION "Force set function attributes LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(ForceFunctionAttrsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
