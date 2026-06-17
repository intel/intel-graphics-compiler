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

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper that preserves IGC's legacy SCC inlining algorithm by running the legacy
// SimpleInlinerLegacyPassWrapper through a nested legacy pass manager. This keeps the inlining
// decisions identical to the legacy OCL Unify pipeline (the LLVM NPM inliner uses a different,
// advisor-based algorithm). The optional TargetLibraryInfoImpl seeds the inner
// TargetLibraryInfoWrapperPass so the inliner sees the same library configuration (the OCL pipeline
// disables all library functions). name() returns the legacy pass argument for dump/skip matching.
class SimpleInlinerNPMWrapper : public llvm::PassInfoMixin<SimpleInlinerNPMWrapper> {
  llvm::InlineParams Params;
  const llvm::TargetLibraryInfoImpl *TLII;

public:
  SimpleInlinerNPMWrapper(llvm::InlineParams Params, const llvm::TargetLibraryInfoImpl *tlii = nullptr)
      : Params(Params), TLII(tlii) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "inline-legacy-wrapped"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_SIMPLEINLINER_LEGACY_H
