/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS
#define GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS

#include "vc/Support/BackendConfig.h"

namespace llvm {
void initializeGenXTranslateSPIRVBuiltinsPass(PassRegistry &);
}

struct GenXTranslateSPIRVBuiltinsPass
    : public llvm::PassInfoMixin<GenXTranslateSPIRVBuiltinsPass> {
  explicit GenXTranslateSPIRVBuiltinsPass(GenXBackendConfigPass::Result &BC)
      : BC(BC) {};
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  GenXBackendConfigPass::Result &BC;
};

#endif // GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS
