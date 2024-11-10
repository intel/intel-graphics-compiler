/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_SIMPLIFY
#define GENX_OPTS_GENX_SIMPLIFY

namespace llvm {
void initializeGenXSimplifyPass(PassRegistry &);
}

struct GenXSimplifyPass : public llvm::PassInfoMixin<GenXSimplifyPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_SIMPLIFY
