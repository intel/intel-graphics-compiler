/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_BTI_ASSIGNMENT
#define GENX_OPTS_GENX_BTI_ASSIGNMENT

namespace llvm {
void initializeGenXBTIAssignmentPass(PassRegistry &);
}

struct GenXBTIAssignmentPass
    : public llvm::PassInfoMixin<GenXBTIAssignmentPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_BTI_ASSIGNMENT
