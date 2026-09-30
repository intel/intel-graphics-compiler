/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_LPMAABRIDGE_H
#define IGCLLVM_ANALYSIS_LPMAABRIDGE_H

#if LLVM_VERSION_MAJOR >= 16

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

// NPM AA analysis that delegates to AAs registered through the LPM's ExternalAAWrapperPass
// (IGC's address space AAs). The caller's AAQueryInfo is forwarded so that query state
// such as cross-iteration context is kept.
class LPMAABridge : public llvm::AnalysisInfoMixin<LPMAABridge>, public llvm::AAResultBase {
  friend llvm::AnalysisInfoMixin<LPMAABridge>;
  static inline llvm::AnalysisKey Key;
  llvm::AAResults *ExternalAAResults;

public:
  using Result = LPMAABridge;
  explicit LPMAABridge(llvm::AAResults *AA) : ExternalAAResults(AA) {}
  LPMAABridge(LPMAABridge &&) = default;

  LPMAABridge run(llvm::Function &, llvm::FunctionAnalysisManager &) { return LPMAABridge(ExternalAAResults); }

  llvm::AliasResult alias(const llvm::MemoryLocation &LocA, const llvm::MemoryLocation &LocB, llvm::AAQueryInfo &AAQI,
                          const llvm::Instruction *CtxI) {
    return ExternalAAResults->alias(LocA, LocB, AAQI, CtxI);
  }
};

// Collect only the AAs that the LPM adds through ExternalAAWrapperPass. Standard LLVM AAs
// are already in the NPM default AA pipeline and must not be re-run with a new query.
// The pass must declare AU.addUsedIfAvailable<ExternalAAWrapperPass>().
inline void addExternalAAResults(llvm::Pass &P, llvm::Function &F, llvm::AAResults &AAR) {
  if (auto *ExtWrapperPass = P.getAnalysisIfAvailable<llvm::ExternalAAWrapperPass>();
      ExtWrapperPass && ExtWrapperPass->CB)
    ExtWrapperPass->CB(P, F, AAR);
}

// Make a wrapped NPM pass see IGC's AAs, which PassBuilder does not know about.
// Must be called before PB.registerFunctionAnalyses(FAM) so this AAManager wins the registration.
inline void registerLPMAAChain(llvm::FunctionAnalysisManager &FAM, llvm::PassBuilder &PB,
                               llvm::AAResults &ExternalAAResults) {
  FAM.registerPass([&ExternalAAResults] { return LPMAABridge(&ExternalAAResults); });
  llvm::AAManager CustomAA = PB.buildDefaultAAPipeline();
  CustomAA.registerFunctionAnalysis<LPMAABridge>();
  FAM.registerPass([AA = std::move(CustomAA)]() mutable { return std::move(AA); });
}

} // namespace IGCLLVM

#endif // LLVM_VERSION_MAJOR >= 16

#endif // IGCLLVM_ANALYSIS_LPMAABRIDGE_H
