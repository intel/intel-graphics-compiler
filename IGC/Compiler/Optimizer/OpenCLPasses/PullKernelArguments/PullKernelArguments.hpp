/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PullKernelArguments {
public:
  PullKernelArguments() {}
  ~PullKernelArguments() {}

  static llvm::StringRef getPassName() { return "PullKernelArguments"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, CodeGenContext *pCtx);
};

// Legacy Pass Manager wrapper.
class PullKernelArgumentsLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  PullKernelArgumentsLPM();
  ~PullKernelArgumentsLPM() {}

  llvm::StringRef getPassName() const override { return PullKernelArguments::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return PullKernelArguments().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                     getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class PullKernelArgumentsNPM : public llvm::PassInfoMixin<PullKernelArgumentsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-pull-kernel-args"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
