/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvm/Analysis/DomTreeUpdater.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Transforms/Scalar.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

CorrelatedValuePropagationLegacyPassWrapper::CorrelatedValuePropagationLegacyPassWrapper() : FunctionPass(ID) {
  initializeCorrelatedValuePropagationLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool CorrelatedValuePropagationLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  CorrelatedValuePropagationPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void CorrelatedValuePropagationLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LazyValueInfoWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<LazyValueInfoWrapperPass>();
}

char CorrelatedValuePropagationLegacyPassWrapper::ID = 0;
Pass *createLegacyWrappedCorrelatedValuePropagationPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new CorrelatedValuePropagationLegacyPassWrapper();
#else
  return llvm::createCorrelatedValuePropagationPass();
#endif
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "correlated-propagation-legacy-wrapped"
#define PASS_DESCRIPTION "Value Propagation LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CorrelatedValuePropagationLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyValueInfoWrapperPass)
IGC_INITIALIZE_PASS_END(CorrelatedValuePropagationLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
