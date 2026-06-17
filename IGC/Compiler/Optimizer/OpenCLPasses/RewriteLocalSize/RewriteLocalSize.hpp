/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class RewriteLocalSize {
public:
  RewriteLocalSize() {}
  ~RewriteLocalSize() {}

  static llvm::StringRef getPassName() { return "RewriteLocalSize"; }

  bool run(llvm::Module &M);
};

// Legacy Pass Manager wrapper.
class RewriteLocalSizeLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  RewriteLocalSizeLPM();
  ~RewriteLocalSizeLPM() {}

  llvm::StringRef getPassName() const override { return RewriteLocalSize::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnModule(llvm::Module &M) override { return RewriteLocalSize().run(M); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class RewriteLocalSizeNPM : public llvm::PassInfoMixin<RewriteLocalSizeNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-rewrite-local-size"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
