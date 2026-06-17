/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>

#include <map>

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class LegalizeFunctionSignatures {
public:
  LegalizeFunctionSignatures() {}
  ~LegalizeFunctionSignatures() {}

  static llvm::StringRef getPassName() { return "LegalizeFunctionSignatures"; }

  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);

  void FixFunctionSignatures(llvm::Module &M);
  void FixFunctionBody(llvm::Module &M);
  void FixFunctionUsers(llvm::Module &M);
  void FixCallInstruction(llvm::Module &M, llvm::CallInst *callInst);
  void CopyAttributesAndAdjustForSkippedFunctionArgs(llvm::Function *pFunc, llvm::Function *pNewFunc,
                                                     bool functionHasPromotableSRetArg);

private:
  llvm::MapVector<llvm::Function *, llvm::Function *> oldToNewFuncMap;

  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::CodeGenContext *m_pCtx = nullptr;
};

// Legacy Pass Manager wrapper.
class LegalizeFunctionSignaturesLPM : public llvm::ModulePass {
public:
  static char ID;

  LegalizeFunctionSignaturesLPM();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<IGC::CodeGenContextWrapper>();
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) {
    return m_impl.run(M, getAnalysis<IGC::MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<IGC::CodeGenContextWrapper>().getCodeGenContext());
  }

  virtual llvm::StringRef getPassName() const { return LegalizeFunctionSignatures::getPassName(); }

private:
  LegalizeFunctionSignatures m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class LegalizeFunctionSignaturesNPM : public llvm::PassInfoMixin<LegalizeFunctionSignaturesNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-legalize-function-signatures"; }
  static bool isRequired() { return true; }

private:
  LegalizeFunctionSignatures m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16
