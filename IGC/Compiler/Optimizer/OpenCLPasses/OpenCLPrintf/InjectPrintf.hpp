/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef INJECT_PRINTF_HPP
#define INJECT_PRINTF_HPP

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class InjectPrintf : public llvm::FunctionPass {
public:
  static char ID;

  InjectPrintf();
  ~InjectPrintf() {}

  virtual llvm::StringRef getPassName() const override { return "InjectPrintf"; }

  bool runOnFunction(llvm::Function &F) override;

private:
  llvm::GlobalVariable *createGlobalFormatStr(llvm::Module *module, llvm::LLVMContext &context);
  llvm::Value *createGEP(llvm::GlobalVariable *globalVariable, llvm::Instruction *insertBefore);
  void insertPrintf(llvm::IRBuilder<> &builder, llvm::FunctionCallee printfFunc, llvm::GlobalVariable *formatStrGlobal,
                    llvm::Instruction *inst, llvm::Value *pointerOperand, llvm::Type *valueType);
};
} // namespace IGC

#endif // INJECT_PRINTF_HPP