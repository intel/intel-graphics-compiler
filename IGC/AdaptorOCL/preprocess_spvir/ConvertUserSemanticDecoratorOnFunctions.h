/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

#include "Compiler/MetaDataUtilsWrapper.h"

#include <string>

namespace IGC {
class ConvertUserSemanticDecoratorOnFunctions : public llvm::ModulePass {
public:
  static char ID;

  ConvertUserSemanticDecoratorOnFunctions();
  ~ConvertUserSemanticDecoratorOnFunctions() {}

  virtual llvm::StringRef getPassName() const override { return "ConvertUserSemanticDecoratorOnFunctions"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnModule(llvm::Module &F) override;
};
} // namespace IGC
