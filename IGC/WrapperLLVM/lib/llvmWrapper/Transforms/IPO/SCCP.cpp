/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/SCCP.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/IPO.h"

#include "llvmWrapper/Transforms/IPO/SCCP.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

IPSCCPLegacyPassWrapper::IPSCCPLegacyPassWrapper() : ModulePass(ID) {
  initializeIPSCCPLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool IPSCCPLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  IPSCCPPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

void IPSCCPLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
}

char IPSCCPLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedIPSCCPPass() {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return new IPSCCPLegacyPassWrapper();
#else
  return llvm::createIPSCCPPass();
#endif
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "ipsccp-legacy-wrapped"
#define PASS_DESCRIPTION "Interprocedural Sparse Conditional Constant Propagation LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(IPSCCPLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(IPSCCPLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
