/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvmWrapper/IR/Instructions.h>

namespace IGC {
class CodeGenContext;
}

namespace IGC {
class BfloatBuiltinsResolution : public llvm::FunctionPass, public llvm::InstVisitor<BfloatBuiltinsResolution> {
public:
  static char ID;
  BfloatBuiltinsResolution();
  virtual bool runOnFunction(llvm::Function &F) override;
  void visitCallInst(llvm::CallInst &CI);

  struct CallInstNamePair {
    llvm::CallInst *CI;
    std::string NewName;
  };
  void resolveCallInstPair(const CallInstNamePair &pair);

  std::vector<CallInstNamePair> CallInstPairs;
};
} // namespace IGC
