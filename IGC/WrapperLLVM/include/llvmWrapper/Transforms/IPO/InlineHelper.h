/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_INLINEHELPER_H
#define IGCLLVM_TRANSFORMS_IPO_INLINEHELPER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/Utils/ImportedFunctionsInliningStatistics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DIBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"
#include "llvmWrapper/IR/CallSite.h"
#include <optional>

using namespace llvm;

namespace IGCLLVM {

using InlinedArrayAllocasTy = DenseMap<ArrayType *, std::vector<AllocaInst *>>;

InlineCost getInlineCost(IGCLLVM::CallSiteRef CS);
bool removeDeadFunctions(CallGraph &CG, bool AlwaysInlineOnly = false);
void mergeInlinedArrayAllocas(Function *Caller, InlineFunctionInfo &IFI, InlinedArrayAllocasTy &InlinedArrayAllocas,
                              int InlineHistory);

bool inlineHistoryIncludes(Function *F, int InlineHistoryID,
                           const SmallVectorImpl<std::pair<Function *, int>> &InlineHistory);
InlineResult inlineCallIfPossible(CallBase &CB, InlineFunctionInfo &IFI, InlinedArrayAllocasTy &InlinedArrayAllocas,
                                  int InlineHistory, bool InsertLifetime,
                                  function_ref<AAResults &(Function &)> &AARGetter,
                                  ImportedFunctionsInliningStatistics &ImportedFunctionsStats);

bool inlineCallsImpl(CallGraphSCC &SCC, CallGraph &CG, std::function<AssumptionCache &(Function &)> GetAssumptionCache,
                     ProfileSummaryInfo *PSI, std::function<const TargetLibraryInfo &(Function &)> GetTLI,
                     bool InsertLifetime, function_ref<InlineCost(CallBase &CB)> GetInlineCost,
                     function_ref<AAResults &(Function &)> AARGetter,
                     ImportedFunctionsInliningStatistics &ImportedFunctionsStats);

AAResults createLegacyPMAAResults(Pass &P, Function &F, BasicAAResult &BAR);
BasicAAResult createLegacyPMBasicAAResult(Pass &P, Function &F);

class LegacyAARGetter {
  Pass &P;
  std::optional<BasicAAResult> BAR;
  std::optional<AAResults> AAR;

public:
  LegacyAARGetter(Pass &P) : P(P) {}
  AAResults &operator()(Function &F) {
    BAR.emplace(IGCLLVM::createLegacyPMBasicAAResult(P, F));
    AAR.emplace(IGCLLVM::createLegacyPMAAResults(P, F, *BAR));
    return *AAR;
  }
};
} // namespace IGCLLVM
#endif // IGCLLVM_TRANSFORMS_IPO_INLINEHELPER_H
