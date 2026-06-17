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
/// @brief  This pass adds nontemporal metadata to every load and store present
///         in __devicelib_assert_fail function.
///         This is needed to avoid caching so that stores to "assert buffer"
///         are visible on the host when breakpoint is hit.
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class NontemporalLoadsAndStoresInAssert {
public:
  NontemporalLoadsAndStoresInAssert() {}
  ~NontemporalLoadsAndStoresInAssert() {}

  static llvm::StringRef getPassName() { return "NontemporalLoadsAndStoresInAssert"; }

  bool run(llvm::Module &M);
};

// Legacy Pass Manager wrapper.
class NontemporalLoadsAndStoresInAssertLPM : public llvm::ModulePass {
public:
  static char ID;

  NontemporalLoadsAndStoresInAssertLPM();
  ~NontemporalLoadsAndStoresInAssertLPM() {}

  llvm::StringRef getPassName() const override { return NontemporalLoadsAndStoresInAssert::getPassName(); }

  bool runOnModule(llvm::Module &M) override { return NontemporalLoadsAndStoresInAssert().run(M); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class NontemporalLoadsAndStoresInAssertNPM : public llvm::PassInfoMixin<NontemporalLoadsAndStoresInAssertNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-nontemporal-loads-and-stores-in-assert"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
