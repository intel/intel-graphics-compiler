/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class DisableInlining final : public llvm::FunctionPass {
public:
  static char ID;

  DisableInlining();
  ~DisableInlining() {}

  llvm::StringRef getPassName() const override { return "DisableInlining"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnFunction(llvm::Function &F) override;
};
}; // namespace IGC
