/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_SIMPLEINLINER_LEGACY_H
#define IGCLLVM_TRANSFORMS_IPO_SIMPLEINLINER_LEGACY_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct SimpleInlinerLegacyPassWrapper : public CallGraphSCCPass {
  InlineParams Params;

  SimpleInlinerLegacyPassWrapper();
  SimpleInlinerLegacyPassWrapper(InlineParams Params);

  static char ID;
  bool runOnSCC(CallGraphSCC &SCC) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  InlineCost getInlineCost(CallBase &CB);
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedSimpleInliner"; }
  bool inlineCalls(CallGraphSCC &SCC);

private:
  PassBuilder PB{};
  LoopAnalysisManager LAM{};
  FunctionAnalysisManager FAM{};
  CGSCCAnalysisManager CGAM{};
  ModuleAnalysisManager MAM{};
  std::function<const TargetLibraryInfo &(Function &)> GetTLI{};
  TargetTransformInfoWrapperPass *TTIWP = nullptr;
  AssumptionCacheTracker *ACT = nullptr;
  ProfileSummaryInfo *PSI = nullptr;
  bool InsertLifetime = true;
  llvm::ImportedFunctionsInliningStatistics ImportedFunctionsStats{};
};

Pass *createLegacyWrappedSimpleInlinerPass();
Pass *createLegacyWrappedSimpleInlinerPass(int Threshold);
Pass *createLegacyWrappedSimpleInlinerPass(unsigned OptLevel, unsigned SizeOptLevel, bool DisableInlineHotCallSite);

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_SIMPLEINLINER_LEGACY_H
