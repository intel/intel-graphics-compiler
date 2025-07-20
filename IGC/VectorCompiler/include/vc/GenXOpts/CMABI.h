/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_CMABI
#define GENX_OPTS_CMABI

namespace llvm {
void initializeCMABIPass(PassRegistry &);
void initializeCMABILegacyPass(PassRegistry &);
} // namespace llvm

struct CMABIPass : public llvm::PassInfoMixin<CMABIPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_CMABI
