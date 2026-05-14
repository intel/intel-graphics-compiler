/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"

#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopRotation.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopRotateLegacyPassWrapper::LoopRotateLegacyPassWrapper(bool EnableHeaderDuplication, bool PrepareForLTO)
    : FunctionPass(ID), EnableHeaderDuplication(EnableHeaderDuplication), PrepareForLTO(PrepareForLTO) {
  initializeLoopRotateLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopRotateLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto Adaptor = createFunctionToLoopPassAdaptor(LoopRotatePass(EnableHeaderDuplication, PrepareForLTO));
  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}

void LoopRotateLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
  getLoopAnalysisUsage(AU);

  AU.addPreserved<LazyBlockFrequencyInfoPass>();
  AU.addPreserved<LazyBranchProbabilityInfoPass>();
}

char LoopRotateLegacyPassWrapper::ID = 0;

llvm::Pass *createLegacyWrappedLoopRotatePass(int MaxHeaderSize, bool PrepareForLTO) {
  bool EnableHeaderDuplication = (MaxHeaderSize != 0);
  return new LoopRotateLegacyPassWrapper(EnableHeaderDuplication, PrepareForLTO);
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loop-rotate-legacy-wrapped"
#define PASS_DESCRIPTION "Rotate Loops LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopRotateLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(LoopRotateLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
