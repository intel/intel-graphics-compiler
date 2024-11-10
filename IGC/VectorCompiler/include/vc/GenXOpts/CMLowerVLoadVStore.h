/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_CM_LOWER_VLOAD_VSTORE
#define GENX_OPTS_CM_LOWER_VLOAD_VSTORE

namespace llvm {
void initializeCMLowerVLoadVStorePass(PassRegistry &);
}

struct CMLowerVLoadVStorePass
    : public llvm::PassInfoMixin<CMLowerVLoadVStorePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // GENX_OPTS_CM_LOWER_VLOAD_VSTORE
