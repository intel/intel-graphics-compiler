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
  bool HasPayloadInMemory = false;
  bool HasEfficient64b = false;

  CMImpParamPass(bool HasPayloadInMemoryIn, bool HasEfficient64bIn)
      : HasPayloadInMemory{HasPayloadInMemoryIn},
        HasEfficient64b{HasEfficient64bIn} {};

  CMImpParamPass();

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_CMIMPLPARAM
