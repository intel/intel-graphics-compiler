/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/Analysis/Loads.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/MustExecute.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LICM.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LICMLegacyPassWrapper::LICMLegacyPassWrapper(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                             bool LicmAllowSpeculation)
    : FunctionPass(ID), LicmMssaOptCap(LicmMssaOptCap), LicmMssaNoAccForPromotionCap(LicmMssaNoAccForPromotionCap),
      LicmAllowSpeculation(LicmAllowSpeculation) {
  initializeLICMLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LICMLegacyPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  // Run the New Pass Manager implementation via the loop-pass adaptor.
  // Enable MemorySSA so the pass can update it when it performs transformations.
  auto Adaptor = createFunctionToLoopPassAdaptor(
      LICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation), true);

  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}

void LICMLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<AssumptionCacheTracker>();
  getLoopAnalysisUsage(AU);
  LazyBlockFrequencyInfoPass::getLazyBFIAnalysisUsage(AU);
  AU.addPreserved<LazyBlockFrequencyInfoPass>();
  AU.addPreserved<LazyBranchProbabilityInfoPass>();
}
char LICMLegacyPassWrapper::ID = 0;
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
llvm::Pass *createLegacyWrappedLICMPass() { return new LICMLegacyPassWrapper(); }
llvm::Pass *createLegacyWrappedLICMPass(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                        bool LicmAllowSpeculation) {
  return new LICMLegacyPassWrapper(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation);
}
#else
llvm::Pass *createLegacyWrappedLICMPass() { return createLICMPass(); }
llvm::Pass *createLegacyWrappedLICMPass(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                        bool LicmAllowSpeculation) {
  return createLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation);
}
#endif
} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "licm-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Invariant Code Motion LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(LICMLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyBFIPass)
IGC_INITIALIZE_PASS_END(LICMLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
