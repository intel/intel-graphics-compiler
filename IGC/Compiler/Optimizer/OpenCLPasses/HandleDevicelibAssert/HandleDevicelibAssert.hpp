/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass removes definition of __devicelib_assert_fail
///         if provided by DPCPP, so that IGC builtin is used.
///
class HandleDevicelibAssert : public llvm::ModulePass {
public:
  /// @brief  Pass identification.
  static char ID;

  HandleDevicelibAssert();
  ~HandleDevicelibAssert() {}

  virtual llvm::StringRef getPassName() const override {
    return "HandleDevicelibAssert";
  }

  virtual bool runOnModule(llvm::Module &M) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
  }
};

} // namespace IGC
