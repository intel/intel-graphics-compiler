/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

#include "Compiler/MetaDataUtilsWrapper.h"

#include <string>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ConvertUserSemanticDecoratorOnFunctions {
public:
  ConvertUserSemanticDecoratorOnFunctions() {}
  ~ConvertUserSemanticDecoratorOnFunctions() {}

  static llvm::StringRef getPassName() { return "ConvertUserSemanticDecoratorOnFunctions"; }

  bool run(llvm::Module &M, ModuleMetaData *MD);
};

// Legacy Pass Manager wrapper.
class ConvertUserSemanticDecoratorOnFunctionsLPM : public llvm::ModulePass {
public:
  static char ID;

  ConvertUserSemanticDecoratorOnFunctionsLPM();
  ~ConvertUserSemanticDecoratorOnFunctionsLPM() {}

  llvm::StringRef getPassName() const override { return ConvertUserSemanticDecoratorOnFunctions::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper.
class ConvertUserSemanticDecoratorOnFunctionsNPM
    : public llvm::PassInfoMixin<ConvertUserSemanticDecoratorOnFunctionsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-convert-user-semantic-decorator-on-functions"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
