/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Transforms/Vectorize/SLPVectorizer.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize/SLPVectorizer.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/InjectTLIMappings.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

SLPVectorizerPassWrapper::SLPVectorizerPassWrapper() : FunctionPass(ID) {
  initializeSLPVectorizerPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool SLPVectorizerPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  SLPVectorizerPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void SLPVectorizerPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<AAResultsWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
#if LLVM_VERSION_MAJOR <= 16
  AU.addRequired<DemandedBitsWrapperPass>();
  AU.addRequired<InjectTLIMappingsLegacy>();
#endif
  AU.addPreserved<LoopInfoWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<AAResultsWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.setPreservesCFG();
}

char SLPVectorizerPassWrapper::ID = 0;
Pass *createLegacyWrappedSLPVectorizerPass() { return new SLPVectorizerPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "slp-vectorizer-legacy-wrapped"
#define PASS_DESCRIPTION "SLP Vectorizer LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SLPVectorizerPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
IGC_INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
#if LLVM_VERSION_MAJOR <= 16
IGC_INITIALIZE_PASS_DEPENDENCY(DemandedBitsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(InjectTLIMappingsLegacy)
#endif
IGC_INITIALIZE_PASS_END(SLPVectorizerPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
