/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_CLONE_INDERECT_FUNCTIONS
#define GENX_OPTS_GENX_CLONE_INDERECT_FUNCTIONS

namespace llvm {
void initializeGenXCloneIndirectFunctionsPass(PassRegistry &);
}

struct GenXCloneIndirectFunctionsPass
    : public llvm::PassInfoMixin<GenXCloneIndirectFunctionsPass> {

  explicit GenXCloneIndirectFunctionsPass(GenXBackendConfigPass::Result &BC)
      : BC(BC){};

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  GenXBackendConfigPass::Result &BC;
};

#endif // GENX_OPTS_GENX_CLONE_INDERECT_FUNCTIONS
