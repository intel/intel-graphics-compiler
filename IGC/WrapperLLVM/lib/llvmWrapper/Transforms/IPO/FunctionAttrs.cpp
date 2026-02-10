/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/FunctionAttrs.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {
ReversePostOrderFunctionAttrsLegacyPassWrapper::ReversePostOrderFunctionAttrsLegacyPassWrapper() : ModulePass(ID) {
  initializeReversePostOrderFunctionAttrsLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool ReversePostOrderFunctionAttrsLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  ReversePostOrderFunctionAttrsPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char ReversePostOrderFunctionAttrsLegacyPassWrapper::ID = 0;
Pass *createLegacyWrappedReversePostOrderFunctionAttrsPass() {
  return new ReversePostOrderFunctionAttrsLegacyPassWrapper();
}
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "rpo-function-attrs-legacy-wrapped"
#define PASS_DESCRIPTION "Deduce function attributes in RPO LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ReversePostOrderFunctionAttrsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(ReversePostOrderFunctionAttrsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
