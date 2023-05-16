/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass adds nontemporal metadata to every load and store present
///         in __devicelib_assert_fail function.
///         This is needed to avoid caching so that stores to "assert buffer"
///         are visible on the host when breakpoint is hit.
class NontemporalLoadsAndStoresInAssert : public llvm::ModulePass {
public:
  /// @brief  Pass identification.
  static char ID;

  NontemporalLoadsAndStoresInAssert();
  ~NontemporalLoadsAndStoresInAssert() {}

  virtual llvm::StringRef getPassName() const override {
    return "NontemporalLoadsAndStoresInAssert";
  }

  virtual bool runOnModule(llvm::Module &M) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }
};

} // namespace IGC
