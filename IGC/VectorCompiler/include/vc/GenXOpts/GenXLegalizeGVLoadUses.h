/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_LEGALIZE_GVLOAD_USES
#define GENX_OPTS_GENX_LEGALIZE_GVLOAD_USES

namespace llvm {
void initializeGenXLegalizeGVLoadUsesPass(PassRegistry &);
}

struct GenXLegalizeGVLoadUsesPass
    : public llvm::PassInfoMixin<GenXLegalizeGVLoadUsesPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_LEGALIZE_GVLOAD_USES
