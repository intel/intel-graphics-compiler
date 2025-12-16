/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {

class DropTargetFunctions final : public llvm::ModulePass {
public:
  static char ID;

  DropTargetFunctions();
  ~DropTargetFunctions() {}

  llvm::StringRef getPassName() const override { return "DropTargetFunctions"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;

private:
  bool VerboseLog = false;
};
}; // namespace IGC
