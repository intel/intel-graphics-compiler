/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class UndefinedReferencesPass {
public:
  UndefinedReferencesPass() {}

  static llvm::StringRef getPassName() { return "UndefinedReferencesPass"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);
};

// Legacy Pass Manager wrapper.
class UndefinedReferencesPassLPM : public llvm::ModulePass {
public:
  static char ID;

  UndefinedReferencesPassLPM();

  llvm::StringRef getPassName() const override { return UndefinedReferencesPass::getPassName(); }
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  bool runOnModule(llvm::Module &M) override {
    return UndefinedReferencesPass().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class UndefinedReferencesPassNPM : public llvm::PassInfoMixin<UndefinedReferencesPassNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "undefined-references"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
