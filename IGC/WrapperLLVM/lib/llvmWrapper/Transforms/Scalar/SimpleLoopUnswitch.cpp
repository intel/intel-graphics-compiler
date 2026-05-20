/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/SimpleLoopUnswitch.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/SimpleLoopUnswitch.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

SimpleLoopUnswitchLegacyPassWrapper::SimpleLoopUnswitchLegacyPassWrapper(bool NonTrivial)
    : FunctionPass(ID), NonTrivial(NonTrivial) {
  initializeSimpleLoopUnswitchLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool SimpleLoopUnswitchLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto Adaptor = createFunctionToLoopPassAdaptor(SimpleLoopUnswitchPass(NonTrivial), /*UseMemorySSA=*/true);
  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}

void SimpleLoopUnswitchLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
  getLoopAnalysisUsage(AU);
}

char SimpleLoopUnswitchLegacyPassWrapper::ID = 0;

llvm::Pass *createSimpleLoopUnswitchLegacyPass(bool NonTrivial) {
  return new SimpleLoopUnswitchLegacyPassWrapper(NonTrivial);
}

} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "simple-loop-unswitch-legacy-wrapped"
#define PASS_DESCRIPTION "Simple unswitch loops LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(SimpleLoopUnswitchLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_END(SimpleLoopUnswitchLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
