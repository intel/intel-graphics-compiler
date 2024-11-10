/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TYPE_LEGALIZATION
#define GENX_OPTS_GENX_TYPE_LEGALIZATION

namespace llvm {
void initializeGenXTypeLegalizationPass(PassRegistry &);
}

struct GenXTypeLegalizationPass
    : public llvm::PassInfoMixin<GenXTypeLegalizationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_TYPE_LEGALIZATION
