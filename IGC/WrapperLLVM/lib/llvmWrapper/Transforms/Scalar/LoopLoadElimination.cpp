/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Scalar/LoopLoadElimination.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/SizeOpts.h"
#include "llvmWrapper/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include "llvmWrapper/Transforms/Scalar/LoopLoadElimination.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopLoadEliminationLegacyPassWrapper::LoopLoadEliminationLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopLoadEliminationLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopLoadEliminationLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  LoopLoadEliminationPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void LoopLoadEliminationLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredID(LoopSimplifyID);
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();
#if LLVM_VERSION_MAJOR <= 16
  AU.addRequired<LoopAccessLegacyAnalysis>();
#endif
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
  LazyBlockFrequencyInfoPass::getLazyBFIAnalysisUsage(AU);
}

char LoopLoadEliminationLegacyPassWrapper::ID = 0;

FunctionPass *createLegacyWrappedLoopLoadEliminationPass() { return new LoopLoadEliminationLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loop-load-elimination-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Load Elimination LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopLoadEliminationLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopAccessAnalysisLegacyPassWrapper)
#else
IGC_INITIALIZE_PASS_DEPENDENCY(LoopAccessLegacyAnalysis)
#endif
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
IGC_INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyBlockFrequencyInfoPass)
IGC_INITIALIZE_PASS_END(LoopLoadEliminationLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
