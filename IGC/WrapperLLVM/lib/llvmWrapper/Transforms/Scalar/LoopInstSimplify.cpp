/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LoopInstSimplify.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopInstSimplify.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopInstSimplifyLegacyPassWrapper::LoopInstSimplifyLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopInstSimplifyLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopInstSimplifyLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto Adaptor = createFunctionToLoopPassAdaptor(LoopInstSimplifyPass(), /*UseMemorySSA=*/true);
  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}

void LoopInstSimplifyLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.setPreservesCFG();
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
  getLoopAnalysisUsage(AU);
}

char LoopInstSimplifyLegacyPassWrapper::ID = 0;

llvm::Pass *createLoopInstSimplifyPass() { return new LoopInstSimplifyLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "loop-instsimplify-legacy-wrapped"
#define PASS_DESCRIPTION "Simplify instructions in loops LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(LoopInstSimplifyLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LoopInstSimplifyLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
