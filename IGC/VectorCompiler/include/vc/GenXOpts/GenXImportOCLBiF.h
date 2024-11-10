/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_IMPORT_OCL_BIF
#define GENX_OPTS_GENX_IMPORT_OCL_BIF

namespace llvm {
void initializeGenXImportOCLBiFPass(PassRegistry &);
}

struct GenXImportOCLBiFPass : public llvm::PassInfoMixin<GenXImportOCLBiFPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_IMPORT_OCL_BIF
