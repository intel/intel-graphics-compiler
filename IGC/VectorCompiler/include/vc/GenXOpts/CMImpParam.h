/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_CMIMPLPARAM
#define GENX_OPTS_CMIMPLPARAM

namespace llvm {
void initializeCMImpParamPass(PassRegistry &);
}

struct CMImpParamPass : public llvm::PassInfoMixin<CMImpParamPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_CMIMPLPARAM
