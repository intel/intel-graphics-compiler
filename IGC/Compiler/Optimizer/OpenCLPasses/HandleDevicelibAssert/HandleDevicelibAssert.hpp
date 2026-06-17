/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass removes definition of __devicelib_assert_fail
///         if provided by DPCPP, so that IGC builtin is used.
///
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class HandleDevicelibAssert {
public:
  HandleDevicelibAssert() {}
  ~HandleDevicelibAssert() {}

  static llvm::StringRef getPassName() { return "HandleDevicelibAssert"; }

  bool run(llvm::Module &M);
};

// Legacy Pass Manager wrapper.
class HandleDevicelibAssertLPM : public llvm::ModulePass {
public:
  static char ID;

  HandleDevicelibAssertLPM();
  ~HandleDevicelibAssertLPM() {}

  llvm::StringRef getPassName() const override { return HandleDevicelibAssert::getPassName(); }

  bool runOnModule(llvm::Module &M) override { return HandleDevicelibAssert().run(M); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {}
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class HandleDevicelibAssertNPM : public llvm::PassInfoMixin<HandleDevicelibAssertNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-handle-devicelib-assert"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
