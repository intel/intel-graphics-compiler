/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class DropTargetBBs : public llvm::FunctionPass {
public:
  static char ID;
  DropTargetBBs();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  llvm::StringRef getPassName() const override { return "DropTargetBBs"; }

  bool runOnFunction(llvm::Function &func) override;

private:
  bool VerboseLog = false;
};

} // namespace IGC
