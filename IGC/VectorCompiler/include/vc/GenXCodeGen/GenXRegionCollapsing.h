/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H
#define VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H

namespace llvm {
void initializeGenXRegionCollapsingPass(PassRegistry &);
class GenXSubtarget;
}

struct GenXRegionCollapsingPass
    : public llvm::PassInfoMixin<GenXRegionCollapsingPass> {
  const llvm::TargetMachine *TM;
  explicit GenXRegionCollapsingPass(const llvm::TargetMachine *TM) : TM(TM){};
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // VC_GENXCODEGEN_GENX_REGION_COLLAPSING_H
