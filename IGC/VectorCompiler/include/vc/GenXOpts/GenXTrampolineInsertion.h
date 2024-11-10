/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TRAMPOLINE_INSERTION
#define GENX_OPTS_GENX_TRAMPOLINE_INSERTION

namespace llvm {
void initializeGenXTrampolineInsertionPass(PassRegistry &);
}

struct GenXTrampolineInsertionPass
    : public llvm::PassInfoMixin<GenXTrampolineInsertionPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_TRAMPOLINE_INSERTION
