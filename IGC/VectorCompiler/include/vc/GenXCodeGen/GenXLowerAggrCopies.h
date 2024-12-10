/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef VC_GENXCODEGEN_GENX_LOWER_AGGR_COPIES_H
#define VC_GENXCODEGEN_GENX_LOWER_AGGR_COPIES_H

namespace llvm {
void initializeGenXLowerAggrCopiesPass(PassRegistry &);
}

struct GenXLowerAggrCopiesPass
    : public llvm::PassInfoMixin<GenXLowerAggrCopiesPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // VC_GENXCODEGEN_GENX_LOWER_AGGR_COPIES_H
