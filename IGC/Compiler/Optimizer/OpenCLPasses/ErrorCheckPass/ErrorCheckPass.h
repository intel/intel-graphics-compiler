/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ErrorCheck : public llvm::InstVisitor<ErrorCheck> {
public:
  ErrorCheck() {}
  ~ErrorCheck() {}

  static llvm::StringRef getPassName() { return "Error Check"; }

  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD,
                     IGC::CodeGenContext *pCtx);

  void visitInstruction(llvm::Instruction &I);

  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_hasError = false;

  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
  IGC::CodeGenContext *m_ctx = nullptr;

  void checkArgsSize(llvm::Function &F);
  void checkByValAddrSpace(llvm::Function &F);
  void handleFP64EmulationMode(llvm::Instruction &I);
};

// Legacy Pass Manager wrapper.
class ErrorCheckLPM : public llvm::FunctionPass {
public:
  static char ID;

  ErrorCheckLPM();
  ~ErrorCheckLPM() {}

  virtual llvm::StringRef getPassName() const override { return ErrorCheck::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData(),
                                getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  ErrorCheck m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions
// (the seeded context analyses are module-level; IGC passes never use skipFunction). name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class ErrorCheckNPM : public llvm::PassInfoMixin<ErrorCheckNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-error-check"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
