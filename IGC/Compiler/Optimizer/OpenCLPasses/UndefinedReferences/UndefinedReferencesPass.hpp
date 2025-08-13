/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {

class UndefinedReferencesPass : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  UndefinedReferencesPass();

  virtual llvm::StringRef getPassName() const override { return "UndefinedReferencesPass"; }
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnModule(llvm::Module &M) override;
};

} // namespace IGC
