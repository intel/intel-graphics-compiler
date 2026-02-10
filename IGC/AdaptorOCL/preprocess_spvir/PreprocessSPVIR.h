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

#include <string>

namespace IGC {
class PreprocessSPVIR : public llvm::ModulePass, public llvm::InstVisitor<PreprocessSPVIR> {
public:
  static char ID;

  PreprocessSPVIR();
  ~PreprocessSPVIR() {}

  virtual llvm::StringRef getPassName() const override { return "PreprocessSPVIR"; }

  virtual bool runOnModule(llvm::Module &F) override;
  void visitCallInst(llvm::CallInst &CI);
  void visitOpenCLEISPrintf(llvm::CallInst &CI);

  static bool isSPVIR(llvm::StringRef funcName);

private:
  bool hasArrayArg(llvm::Function &F);
  void processBuiltinsWithArrayArguments(llvm::Function &F);
  void processBuiltinsWithArrayArguments();
  void createCallAndReplace(llvm::CallInst &oldCallInst, llvm::StringRef newFuncName, std::vector<llvm::Value *> &args);

  IGCLLVM::Module *m_Module = nullptr;
  llvm::IRBuilder<> *m_Builder = nullptr;
  bool m_changed = false;
};
} // namespace IGC
