/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/GlobalOpt.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/GlobalOpt.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

GlobalOptLegacyPassWrapper::GlobalOptLegacyPassWrapper() : ModulePass(ID) {
  initializeGlobalOptLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool GlobalOptLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  GlobalOptPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

void GlobalOptLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<BlockFrequencyInfoWrapperPass>();
}

char GlobalOptLegacyPassWrapper::ID = 0;
Pass *createLegacyWrappedGlobalOptPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new GlobalOptLegacyPassWrapper();
#else
  return llvm::createGlobalOptimizerPass();
#endif
}
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "globalopt-legacy-wrapped"
#define PASS_DESCRIPTION "Global Variable Optimizer LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GlobalOptLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(GlobalOptLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
