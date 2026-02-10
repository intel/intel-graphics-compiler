/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/IPO/InlineSimple.h"
#include "llvmWrapper/Transforms/IPO/InlineHelper.h"
#include "llvmWrapper/Transforms/IPO/SCCP.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
#define DEBUG_TYPE "inline"

namespace IGCLLVM {

SimpleInlinerLegacyPassWrapper::SimpleInlinerLegacyPassWrapper()
    : CallGraphSCCPass(ID), Params(llvm::getInlineParams()) {
  initializeSimpleInlinerLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

SimpleInlinerLegacyPassWrapper::SimpleInlinerLegacyPassWrapper(InlineParams Params)
    : CallGraphSCCPass(ID), Params(std::move(Params)) {
  initializeSimpleInlinerLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool SimpleInlinerLegacyPassWrapper::runOnSCC(CallGraphSCC &SCC) {
  TTIWP = &getAnalysis<TargetTransformInfoWrapperPass>();

  if (skipSCC(SCC))
    return false;
  bool changed = inlineCalls(SCC);
  return changed;
}

InlineCost SimpleInlinerLegacyPassWrapper::getInlineCost(CallBase &CB) {
  Function *Callee = CB.getCalledFunction();
  TargetTransformInfo &TTI = TTIWP->getTTI(*Callee);

  bool RemarksEnabled = false;
  const auto &BBs = *CB.getCaller();
  if (!BBs.empty()) {
    auto DI = OptimizationRemark(DEBUG_TYPE, "", DebugLoc(), &BBs.front());
    if (DI.isEnabled())
      RemarksEnabled = true;
  }
  OptimizationRemarkEmitter ORE(CB.getCaller());

  std::function<AssumptionCache &(Function &)> GetAssumptionCache = [&](Function &F) -> AssumptionCache & {
    return ACT->getAssumptionCache(F);
  };
  return llvm::getInlineCost(CB, Params, TTI, GetAssumptionCache, GetTLI,
                             /*GetBFI=*/nullptr, PSI, RemarksEnabled ? &ORE : nullptr);
}

bool SimpleInlinerLegacyPassWrapper::inlineCalls(CallGraphSCC &SCC) {
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  ACT = &getAnalysis<AssumptionCacheTracker>();
  PSI = &getAnalysis<ProfileSummaryInfoWrapperPass>().getPSI();
  GetTLI = [&](Function &F) -> const TargetLibraryInfo & {
    return getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  };
  auto GetAssumptionCache = [&](Function &F) -> AssumptionCache & { return ACT->getAssumptionCache(F); };
  return IGCLLVM::inlineCallsImpl(
      SCC, CG, GetAssumptionCache, PSI, GetTLI, InsertLifetime, [&](CallBase &CB) { return getInlineCost(CB); },
      IGCLLVM::LegacyAARGetter(*this), ImportedFunctionsStats);
}

void SimpleInlinerLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addUsedIfAvailable<ScopedNoAliasAAWrapperPass>();
  AU.addUsedIfAvailable<TypeBasedAAWrapperPass>();
  AU.addUsedIfAvailable<GlobalsAAWrapperPass>();
  AU.addUsedIfAvailable<ExternalAAWrapperPass>();
  CallGraphSCCPass::getAnalysisUsage(AU);
}

char SimpleInlinerLegacyPassWrapper::ID = 0;

Pass *createLegacyWrappedSimpleInlinerPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new SimpleInlinerLegacyPassWrapper();
#else
  return llvm::createFunctionInliningPass();
#endif
}
Pass *createLegacyWrappedSimpleInlinerPass(int Threshold) {
#if LLVM_VERSION_MAJOR >= 16
  return new SimpleInlinerLegacyPassWrapper(llvm::getInlineParams(Threshold));
#else
  return llvm::createFunctionInliningPass(Threshold);
#endif
}
Pass *createLegacyWrappedSimpleInlinerPass(unsigned OptLevel, unsigned SizeOptLevel, bool DisableInlineHotCallSite) {
#if LLVM_VERSION_MAJOR >= 16
  auto Param = llvm::getInlineParams(OptLevel, SizeOptLevel);
  if (DisableInlineHotCallSite)
    Param.HotCallSiteThreshold = 0;
  return new SimpleInlinerLegacyPassWrapper(Param);
#else
  return llvm::createFunctionInliningPass(OptLevel, SizeOptLevel, DisableInlineHotCallSite);
#endif
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "inline-legacy-wrapped"
#define PASS_DESCRIPTION "Function Integration/Inlining LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SimpleInlinerLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(SimpleInlinerLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
