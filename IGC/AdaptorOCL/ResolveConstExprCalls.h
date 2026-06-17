/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
// Resolves pseudo indirect constexpr cast calls
class ResolveConstExprCalls {
public:
  ResolveConstExprCalls() {}
  ~ResolveConstExprCalls() {}

  static llvm::StringRef getPassName() { return "ResolveConstExprCalls"; }

  bool run(llvm::Module &M);
};

// Legacy Pass Manager wrapper.
class ResolveConstExprCallsLPM : public llvm::ModulePass {
public:
  static char ID;

  ResolveConstExprCallsLPM();
  ~ResolveConstExprCallsLPM() {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  llvm::StringRef getPassName() const override { return ResolveConstExprCalls::getPassName(); }

  bool runOnModule(llvm::Module &M) override { return ResolveConstExprCalls().run(M); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ResolveConstExprCallsNPM : public llvm::PassInfoMixin<ResolveConstExprCallsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-resolve-constexpr-calls"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
