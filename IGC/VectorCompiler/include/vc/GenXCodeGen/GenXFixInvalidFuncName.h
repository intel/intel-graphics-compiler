/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef VC_GENXCODEGEN_GENX_FIX_INVALID_FUNC_NAME_H
#define VC_GENXCODEGEN_GENX_FIX_INVALID_FUNC_NAME_H

namespace llvm {
void initializeGenXFixInvalidFuncNamePass(PassRegistry &);
}

struct GenXFixInvalidFuncNamePass
    : public llvm::PassInfoMixin<GenXFixInvalidFuncNamePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

#endif // VC_GENXCODEGEN_GENX_FIX_INVALID_FUNC_NAME_H
