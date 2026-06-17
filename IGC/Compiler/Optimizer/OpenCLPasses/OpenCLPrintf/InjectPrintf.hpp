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
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class InjectPrintf {
public:
  InjectPrintf() {}
  ~InjectPrintf() {}

  static llvm::StringRef getPassName() { return "InjectPrintf"; }

  bool run(llvm::Function &F);

private:
  llvm::GlobalVariable *createGlobalFormatStr(llvm::Module *module, llvm::LLVMContext &context);
  llvm::Value *createGEP(llvm::GlobalVariable *globalVariable, llvm::Instruction *insertBefore);
  void insertPrintf(llvm::IRBuilder<> &builder, llvm::FunctionCallee printfFunc, llvm::GlobalVariable *formatStrGlobal,
                    llvm::Instruction *inst, llvm::Value *pointerOperand, llvm::Type *valueType);
};

// Legacy Pass Manager wrapper.
class InjectPrintfLPM : public llvm::FunctionPass {
public:
  static char ID;

  InjectPrintfLPM();
  ~InjectPrintfLPM() {}

  llvm::StringRef getPassName() const override { return InjectPrintf::getPassName(); }

  bool runOnFunction(llvm::Function &F) override { return InjectPrintf().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class InjectPrintfNPM : public llvm::PassInfoMixin<InjectPrintfNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "inject-printf"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC

#endif // INJECT_PRINTF_HPP