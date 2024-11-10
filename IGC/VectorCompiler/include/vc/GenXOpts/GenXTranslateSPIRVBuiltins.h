/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS
#define GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS

namespace llvm {
void initializeGenXTranslateSPIRVBuiltinsPass(PassRegistry &);
}

struct GenXTranslateSPIRVBuiltinsPass
    : public llvm::PassInfoMixin<GenXTranslateSPIRVBuiltinsPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_TRANSLATE_SPIRV_BUILTINS
