/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_TRANSLATE_INTRINSICS
#define GENX_OPTS_GENX_TRANSLATE_INTRINSICS

namespace llvm {
void initializeGenXTranslateIntrinsicsPass(PassRegistry &);
}

struct GenXTranslateIntrinsicsPass
    : public llvm::PassInfoMixin<GenXTranslateIntrinsicsPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_TRANSLATE_INTRINSICS
