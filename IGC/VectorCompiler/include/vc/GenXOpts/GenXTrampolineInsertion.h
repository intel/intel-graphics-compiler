/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TRAMPOLINE_INSERTION
#define GENX_OPTS_GENX_TRAMPOLINE_INSERTION

#include "vc/Support/BackendConfig.h"

namespace llvm {
void initializeGenXTrampolineInsertionPass(PassRegistry &);
}

struct GenXTrampolineInsertionPass
    : public llvm::PassInfoMixin<GenXTrampolineInsertionPass> {

  explicit GenXTrampolineInsertionPass(GenXBackendConfigPass::Result &BC)
      : BC(BC){};

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  GenXBackendConfigPass::Result &BC;
};

#endif // GENX_OPTS_GENX_TRAMPOLINE_INSERTION
