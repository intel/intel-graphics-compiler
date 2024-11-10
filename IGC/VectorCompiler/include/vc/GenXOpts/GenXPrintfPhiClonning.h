/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_PRINTF_PHI_CLONNING
#define GENX_OPTS_GENX_PRINTF_PHI_CLONNING

namespace llvm {
void initializeGenXPrintfPhiClonningPass(PassRegistry &);
}

struct GenXPrintfPhiClonningPass
    : public llvm::PassInfoMixin<GenXPrintfPhiClonningPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_PRINTF_PHI_CLONNING
