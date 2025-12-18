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
                                 bool Efficient64b = false,
                                 bool UseBindlessImages = false)
      : GrfByteSize(GrfByteSize), Efficient64b(Efficient64b),
        UseBindlessImages(UseBindlessImages) {};

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  unsigned GrfByteSize;
  bool Efficient64b = false;
  bool UseBindlessImages;
};

#endif // GENX_OPTS_CM_KERNEL_ARG_OFFSET
