/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/CalledValuePropagation.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/CalledValuePropagation.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

CalledValuePropagationLegacyPassWrapper::CalledValuePropagationLegacyPassWrapper() : ModulePass(ID) {
  initializeCalledValuePropagationLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool CalledValuePropagationLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  CalledValuePropagationPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

void CalledValuePropagationLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const { AU.setPreservesAll(); }

char CalledValuePropagationLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedCalledValuePropagationPass() { return new CalledValuePropagationLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "called-value-propagation-legacy-wrapped"
#define PASS_DESCRIPTION "Called Value Propagation LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(CalledValuePropagationLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
