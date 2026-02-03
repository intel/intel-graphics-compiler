/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Transforms/Scalar/LoopUnrollPass.h"

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

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/ADT/None.h"

#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
using namespace llvm;

namespace IGCLLVM {

LoopUnrollLegacyPassWrapper::LoopUnrollLegacyPassWrapper(
    int OptLevel, bool OnlyWhenForced, bool ForgetAllSCEV, IGCLLVM::optional<bool> AllowPartial,
    IGCLLVM::optional<bool> Runtime, IGCLLVM::optional<bool> UpperBound, IGCLLVM::optional<bool> AllowPeeling,
    IGCLLVM::optional<bool> AllowProfileBasedPeeling, IGCLLVM::optional<unsigned> ProvidedFullUnrollMaxCount)
    : FunctionPass(ID), OptLevel(OptLevel), OnlyWhenForced(OnlyWhenForced), ForgetAllSCEV(ForgetAllSCEV),
      ProvidedAllowPartial(AllowPartial), ProvidedRuntime(Runtime), ProvidedUpperBound(UpperBound),
      ProvidedAllowPeeling(AllowPeeling), ProvidedAllowProfileBasedPeeling(AllowProfileBasedPeeling),
      ProvidedFullUnrollMaxCount(ProvidedFullUnrollMaxCount) {
  initializeLoopUnrollLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
}

void LoopUnrollLegacyPassWrapper::initializeAnalysisManagers(TargetTransformInfoWrapperPass &TTIWP) {
  // Register New Pass Manager TargetIRAnalysis which is constructed from Legacy Pass Manager
  // TargetTransformInfoWrapperPass to NPM analysis manager
  // registerPass and TargetIRAnalysis constructor require callbacks so nested lambdas are used
  auto LpmTTIWPCallback = [&TTIWP](const Function &F) { return std::move(TTIWP.getTTI(F)); };
  FAM.registerPass([&LpmTTIWPCallback] { return TargetIRAnalysis(LpmTTIWPCallback); });

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

  auto &TTIWP = getAnalysis<TargetTransformInfoWrapperPass>();
  initializeAnalysisManagers(TTIWP);

  LoopUnrollOptions unrollOpts(OptLevel, OnlyWhenForced, ForgetAllSCEV);
  unrollOpts.AllowPartial = ProvidedAllowPartial;
  unrollOpts.FullUnrollMaxCount = ProvidedFullUnrollMaxCount;
  unrollOpts.AllowPeeling = ProvidedAllowPeeling;
  unrollOpts.AllowProfileBasedPeeling = ProvidedAllowProfileBasedPeeling;
  unrollOpts.AllowRuntime = ProvidedRuntime;
  unrollOpts.AllowUpperBound = ProvidedUpperBound;

  LoopUnrollPass Implementation(unrollOpts);
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
#if LLVM_VERSION_MAJOR >= 16
  // Not using Count and Threshold as New Pass Manager LoopUnroll implementation doesn't use them
  // Instead it uses Count and Threshold provided by TargetTransformInfo pass which is registered to NPM analysis
  // manager (FAM member) through initializeAnalysisManagers function above; Count and Threshold have to be provided in
  // createLegacyWrappedLoopUnrollPass function as both are used in Legacy Pass Manager so we need them in our hybrid
  // approach
  return new LoopUnrollLegacyPassWrapper(OptLevel, OnlyWhenForced, ForgetAllSCEV,
                                         AllowPartial == -1 ? IGCLLVM::None : IGCLLVM::optional<bool>(AllowPartial),
                                         Runtime == -1 ? IGCLLVM::None : IGCLLVM::optional<bool>(Runtime),
                                         UpperBound == -1 ? IGCLLVM::None : IGCLLVM::optional<bool>(UpperBound),
                                         AllowPeeling == -1 ? IGCLLVM::None : IGCLLVM::optional<bool>(AllowPeeling));
#else
  return llvm::createLoopUnrollPass(OptLevel, OnlyWhenForced, ForgetAllSCEV, Threshold, Count, AllowPartial, Runtime,
                                    UpperBound, AllowPeeling);
#endif
}

Pass *createLegacyWrappedSimpleLoopUnrollPass(int OptLevel, bool OnlyWhenForced, bool ForgetAllSCEV) {
#if LLVM_VERSION_MAJOR >= 16
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
