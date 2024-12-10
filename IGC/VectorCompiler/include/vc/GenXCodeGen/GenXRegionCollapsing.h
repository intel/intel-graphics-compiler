/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H
#define VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H

namespace llvm {
void initializeGenXRegionCollapsingPass(PassRegistry &);
}

struct GenXRegionCollapsingPass
    : public llvm::PassInfoMixin<GenXRegionCollapsingPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H
