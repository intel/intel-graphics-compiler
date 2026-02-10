/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Transforms/Scalar.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

MemCpyOptLegacyPassWrapper::MemCpyOptLegacyPassWrapper() : FunctionPass(ID) {
  initializeMemCpyOptLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool MemCpyOptLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  // Run the New Pass Manager implementation of the pass.
  MemCpyOptPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void MemCpyOptLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<AAResultsWrapperPass>();
  AU.addPreserved<AAResultsWrapperPass>();
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
}

char MemCpyOptLegacyPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedMemCpyOptPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new MemCpyOptLegacyPassWrapper();
#else
  return llvm::createMemCpyOptPass();
#endif
}
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "memcpyopt-legacy-wrapped"
#define PASS_DESCRIPTION "MemCpy Optimization LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MemCpyOptLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(GlobalsAAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(MemCpyOptLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
