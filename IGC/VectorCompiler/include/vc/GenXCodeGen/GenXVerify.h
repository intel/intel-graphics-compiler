/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXVerify
//===----------------------------------------------------------------------===//
//
// This pass contains GenX-specific IR validity checks.
//
#ifndef GENX_VERIFY_H
#define GENX_VERIFY_H

struct GenXVerifyPass : public llvm::PassInfoMixin<GenXVerifyPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_VERIFY_H
