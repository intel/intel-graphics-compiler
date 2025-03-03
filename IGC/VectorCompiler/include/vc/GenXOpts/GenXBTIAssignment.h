/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_BTI_ASSIGNMENT
#define GENX_OPTS_GENX_BTI_ASSIGNMENT

#include "vc/Support/BackendConfig.h"

namespace llvm {
void initializeGenXBTIAssignmentPass(PassRegistry &);
}

struct GenXBTIAssignmentPass
    : public llvm::PassInfoMixin<GenXBTIAssignmentPass> {
  explicit GenXBTIAssignmentPass(GenXBackendConfigPass::Result &BC) : BC(BC){};
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  GenXBackendConfigPass::Result &BC;
};

#endif // GENX_OPTS_GENX_BTI_ASSIGNMENT
