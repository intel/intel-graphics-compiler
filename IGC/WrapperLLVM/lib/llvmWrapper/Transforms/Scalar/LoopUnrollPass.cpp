/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopUnrollPass.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"
#include <optional>

using namespace llvm;

namespace IGCLLVM {

LoopUnrollLegacyPassWrapper::LoopUnrollLegacyPassWrapper(
    int OptLevel, bool OnlyWhenForced, bool ForgetAllSCEV, std::optional<unsigned> Threshold,
    std::optional<unsigned> Count, std::optional<bool> AllowPartial, std::optional<bool> Runtime,
    std::optional<bool> UpperBound, std::optional<bool> AllowPeeling, std::optional<bool> AllowProfileBasedPeeling,
    std::optional<unsigned> ProvidedFullUnrollMaxCount)
    : FunctionPass(ID), OptLevel(OptLevel), OnlyWhenForced(OnlyWhenForced), ForgetAllSCEV(ForgetAllSCEV),
      ProvidedCount(std::move(Count)), ProvidedThreshold(Threshold), ProvidedAllowPartial(AllowPartial),
      ProvidedRuntime(Runtime), ProvidedUpperBound(UpperBound), ProvidedAllowPeeling(AllowPeeling),
      ProvidedAllowProfileBasedPeeling(AllowProfileBasedPeeling),
      ProvidedFullUnrollMaxCount(ProvidedFullUnrollMaxCount) {
  initializeLoopUnrollLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopUnrollLegacyPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  LoopUnrollPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void LoopUnrollLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  getLoopAnalysisUsage(AU);
}

char LoopUnrollLegacyPassWrapper::ID = 0;

Pass *createLegacyWrappedLoopUnrollPass(int OptLevel, bool OnlyWhenForced, bool ForgetAllSCEV, int Threshold, int Count,
                                        int AllowPartial, int Runtime, int UpperBound, int AllowPeeling) {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return new LoopUnrollLegacyPassWrapper(OptLevel, OnlyWhenForced, ForgetAllSCEV,
                                         Threshold == -1 ? std::nullopt : std::optional<unsigned>(Threshold),
                                         Count == -1 ? std::nullopt : std::optional<unsigned>(Count),
                                         AllowPartial == -1 ? std::nullopt : std::optional<bool>(AllowPartial),
                                         Runtime == -1 ? std::nullopt : std::optional<bool>(Runtime),
                                         UpperBound == -1 ? std::nullopt : std::optional<bool>(UpperBound),
                                         AllowPeeling == -1 ? std::nullopt : std::optional<bool>(AllowPeeling));
#else
  return llvm::createLoopUnrollPass(OptLevel, OnlyWhenForced, ForgetAllSCEV, Threshold, Count, AllowPartial, Runtime,
                                    UpperBound, AllowPeeling);
#endif
}

Pass *createLegacyWrappedSimpleLoopUnrollPass(int OptLevel, bool OnlyWhenForced, bool ForgetAllSCEV) {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return createLegacyWrappedLoopUnrollPass(OptLevel, OnlyWhenForced, ForgetAllSCEV, -1, -1, 0, 0, 0, 1);
#else
  return llvm::createSimpleLoopUnrollPass(OptLevel, OnlyWhenForced, ForgetAllSCEV);
#endif
}
} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "loop-unroll-legacy-wrapped"
#define PASS_DESCRIPTION "Unroll loops LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(LoopUnrollLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LoopUnrollLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
