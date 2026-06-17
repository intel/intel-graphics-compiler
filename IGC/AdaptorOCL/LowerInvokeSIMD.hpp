/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include <string>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class LowerInvokeSIMD : public llvm::InstVisitor<LowerInvokeSIMD> {
public:
  LowerInvokeSIMD() {}
  ~LowerInvokeSIMD() {}

  static llvm::StringRef getPassName() { return "LowerInvokeSIMD"; }

  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);
  void visitCallInst(llvm::CallInst &CI);

private:
  IGCLLVM::IRBuilder<> *m_Builder = nullptr;
  llvm::ValueMap<llvm::Function *, llvm::Function *> m_OldFuncToNewFuncMap;
  bool m_changed = false;

  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::CodeGenContext *m_ctx = nullptr;

  void fixUniformParamsAndSIMDSize(const llvm::Function *ESIMDFunction, llvm::CallInst &NewCall);
};

// Legacy Pass Manager wrapper.
class LowerInvokeSIMDLPM : public llvm::ModulePass {
public:
  static char ID;

  LowerInvokeSIMDLPM();
  ~LowerInvokeSIMDLPM() {}

  virtual llvm::StringRef getPassName() const override { return LowerInvokeSIMD::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.setPreservesCFG();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  LowerInvokeSIMD m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class LowerInvokeSIMDNPM : public llvm::PassInfoMixin<LowerInvokeSIMDNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-lower-invoke-simd"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
