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

  static char ID;
  explicit CMKernelArgOffsetPass(unsigned GrfByteSize = 32,
                                 bool UseBindlessImages = false)
      : GrfByteSize(GrfByteSize), UseBindlessImages(UseBindlessImages){};

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  unsigned GrfByteSize;
  bool UseBindlessImages;
};

#endif // GENX_OPTS_CM_KERNEL_ARG_OFFSET
