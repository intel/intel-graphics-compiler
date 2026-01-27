/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Transforms/Scalar/LoopDistribute.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopDistribute.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvmWrapper/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopDistributeLegacyPassWrapper::LoopDistributeLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopDistributeLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopDistributeLegacyPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  LoopDistributePass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void LoopDistributeLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  AU.addRequired<IGCLLVM::LoopAccessAnalysisLegacyPassWrapper>();
#else
  AU.addRequired<LoopAccessLegacyAnalysis>();
#endif
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
}

char LoopDistributeLegacyPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedLoopDistributePass() { return new LoopDistributeLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loop-distribution-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Distribution LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopDistributeLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
#if LLVM_VERSION_MAJOR >= 16
IGC_INITIALIZE_PASS_DEPENDENCY(LoopAccessAnalysisLegacyPassWrapper)
#else
IGC_INITIALIZE_PASS_DEPENDENCY(LoopAccessLegacyAnalysis)
#endif
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
IGC_INITIALIZE_PASS_END(LoopDistributeLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
