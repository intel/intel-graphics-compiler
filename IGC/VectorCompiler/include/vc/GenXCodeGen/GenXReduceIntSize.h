/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef VC_GENXCODEGEN_GENX_REDUCE_INT_SIZE_H
#define VC_GENXCODEGEN_GENX_REDUCE_INT_SIZE_H

namespace llvm {
void initializeGenXReduceIntSizePass(PassRegistry &);
}

struct GenXReduceIntSizePass
    : public llvm::PassInfoMixin<GenXReduceIntSizePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // VC_GENXCODEGEN_GENX_REDUCE_INT_SIZE_H
