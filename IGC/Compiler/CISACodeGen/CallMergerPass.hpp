/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// Utility pass that merges exclusive calls prior to inlining pass in case their
// size would be to great to inline otherwise. In this way functions can be
// inlined as they are called only once.
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
class PassRegistry;
} // namespace llvm

namespace IGC {

class CallMerger : public llvm::ModulePass {
private:
  CodeGenContext *CTX = nullptr;
  EstimateFunctionSize *EFS = nullptr;

public:
  static char ID;

  CallMerger();

  bool runOnModule(llvm::Module &F) override;

  llvm::StringRef getPassName() const override { return "CallMergerPass"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

private:
  bool runOnFunction(llvm::Function &F);
};

void initializeCallMergerPass(llvm::PassRegistry&);
} // namespace IGC