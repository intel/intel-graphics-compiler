/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Transforms/Vectorize/LoopVectorize.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize/LoopVectorize.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/InjectTLIMappings.h"
#include "llvmWrapper/Transforms/Utils/InjectTLIMappings.h"
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

LoopVectorizeLegacyPassWrapper::LoopVectorizeLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopVectorizeLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool LoopVectorizeLegacyPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  LoopVectorizePass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void LoopVectorizeLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<BlockFrequencyInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
#if LLVM_VERSION_MAJOR <= 16
  AU.addRequired<LoopAccessLegacyAnalysis>();
  AU.addRequired<DemandedBitsWrapperPass>();
  AU.addRequired<InjectTLIMappingsLegacy>();
#endif
  AU.addPreserved<BasicAAWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
}

char LoopVectorizeLegacyPassWrapper::ID = 0;
Pass *createLegacyWrappedLoopVectorizePass() { return new LoopVectorizeLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loopvectorization-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Vectorization LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopVectorizeLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BasicAAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(GlobalsAAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
#if LLVM_VERSION_MAJOR <= 16
IGC_INITIALIZE_PASS_DEPENDENCY(DemandedBitsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopAccessLegacyAnalysis)
#endif
IGC_INITIALIZE_PASS_END(LoopVectorizeLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
