/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Checks if function can be unmased and transforms the attributes of
// function definition and each call site
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class TransformUnmaskedFunctionsPass {
public:
  TransformUnmaskedFunctionsPass() {}
  ~TransformUnmaskedFunctionsPass() {}

  static llvm::StringRef getPassName() { return "TransformUnmaskedFunctionsPass"; }

  bool runOnFunction(llvm::Function &F, IGC::CodeGenContext *pCtx);

private:
  ModuleMetaData *MMD = nullptr;
};

// Legacy Pass Manager wrapper.
class TransformUnmaskedFunctionsPassLPM final : public llvm::FunctionPass {
public:
  static char ID;

  TransformUnmaskedFunctionsPassLPM();
  ~TransformUnmaskedFunctionsPassLPM() {}

  virtual llvm::StringRef getPassName() const override { return TransformUnmaskedFunctionsPass::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
    AU.addRequired<IGC::CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  TransformUnmaskedFunctionsPass m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions
// (the seeded CodeGenContextAnalysis is module-level; IGC passes never use skipFunction). name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class TransformUnmaskedFunctionsPassNPM : public llvm::PassInfoMixin<TransformUnmaskedFunctionsPassNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "transform-unmasked"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

// Inlines functions marked as unmasked correclty modifying all related metadata.
class InlineUnmaskedFunctionsPass final : public llvm::ModulePass {
public:
  static char ID;

  InlineUnmaskedFunctionsPass();
  ~InlineUnmaskedFunctionsPass() {}

  virtual llvm::StringRef getPassName() const override { return "InlineUnmaskedFunctionsPass"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
    AU.addRequired<IGC::CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override;

private:
  ModuleMetaData *MMD;
};
}; // namespace IGC
