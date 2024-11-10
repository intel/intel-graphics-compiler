/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_BIF_FLAG_CTRL_RESOLUTION
#define GENX_OPTS_GENX_BIF_FLAG_CTRL_RESOLUTION

namespace llvm {
void initializeGenXBIFFlagCtrlResolutionPass(PassRegistry &);
}

struct GenXBIFFlagCtrlResolutionPass
    : public llvm::PassInfoMixin<GenXBIFFlagCtrlResolutionPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_BIF_FLAG_CTRL_RESOLUTION
