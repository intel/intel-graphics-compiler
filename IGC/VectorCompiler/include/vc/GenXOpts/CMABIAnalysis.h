/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_CMABIANALYSIS
#define GENX_OPTS_CMABIANALYSIS

// void initializeCMABIAnalysis(PassRegistry &);

class LocalizationInfo;

struct CMABIAnalysisPassResult {
  llvm::SmallPtrSet<llvm::Function *, 8> Kernels;
  llvm::SmallDenseMap<llvm::Function *, LocalizationInfo *> GlobalInfo;

  // TODO: Fill invalidator
  template <class IR, class Analysis, class Invalidator>
  bool invalidate(IR &, Analysis &, Invalidator &) {
    return true;
  }
};

#if LLVM_VERSION_MAJOR >= 16
struct CMABIAnalysisPass : public llvm::AnalysisInfoMixin<CMABIAnalysisPass> {
  using Result = CMABIAnalysisPassResult;
  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::AnalysisKey Key;
};
#endif
#endif // GENX_OPTS_CMABIANALYSIS
