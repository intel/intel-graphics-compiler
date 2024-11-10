/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_CM_KERNEL_ARG_OFFSET
#define GENX_OPTS_CM_KERNEL_ARG_OFFSET

namespace llvm {
void initializeCMKernelArgOffsetPass(PassRegistry &);
}

struct CMKernelArgOffsetPass
    : public llvm::PassInfoMixin<CMKernelArgOffsetPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_CM_KERNEL_ARG_OFFSET
