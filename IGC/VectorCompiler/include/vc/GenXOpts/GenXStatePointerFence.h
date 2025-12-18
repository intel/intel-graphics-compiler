/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_STATE_POINTER_FENCE
#define GENX_OPTS_GENX_STATE_POINTER_FENCE

namespace llvm {
void initializeGenXStatePointerFencePass(PassRegistry &);
}

struct GenXStatePointerFencePass
    : public llvm::PassInfoMixin<GenXStatePointerFencePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_STATE_POINTER_FENCE
