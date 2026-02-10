/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InstCombine/InstructionCombining.h"
#include "llvmWrapper/Transforms/InitializePasses.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {
InstructionCombiningPassWrapper::InstructionCombiningPassWrapper() : FunctionPass(ID) {
  initializeInstructionCombiningPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

InstructionCombiningPassWrapper::InstructionCombiningPassWrapper(unsigned MaxIterations) : FunctionPass(ID) {
  initializeInstructionCombiningPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool InstructionCombiningPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  InstCombinePass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void InstructionCombiningPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<AAResultsWrapperPass>();
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<AAResultsWrapperPass>();
  AU.addPreserved<BasicAAWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
  LazyBlockFrequencyInfoPass::getLazyBFIAnalysisUsage(AU);
}

char InstructionCombiningPassWrapper::ID = 0;
FunctionPass *createWrappedInstructionCombiningPass() {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return new InstructionCombiningPassWrapper();
#else
  return llvm::createInstructionCombiningPass();
#endif
}
FunctionPass *createWrappedInstructionCombiningPass(unsigned MaxIterations) {
  return new InstructionCombiningPassWrapper(MaxIterations);
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "instructions-combining-legacy-wrapped"
#define PASS_DESCRIPTION "Instructions Combining LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InstructionCombiningPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(GlobalsAAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyBlockFrequencyInfoPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(InstructionCombiningPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
