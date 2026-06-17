/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BreakdownIntrinsicPass : public llvm::InstVisitor<BreakdownIntrinsicPass> {
public:
  BreakdownIntrinsicPass() {}

  ~BreakdownIntrinsicPass() {}

  static llvm::StringRef getPassName() { return "BreakdownIntrinsicPass"; }

  // Per-function entry. The caller supplies the context (MetaDataUtils / ModuleMetaData
  // / CodeGenContext) that the legacy pass used to obtain via getAnalysis<>.
  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD,
                     IGC::CodeGenContext *pCtx);
  void visitIntrinsicInst(llvm::IntrinsicInst &I);

private:
  bool m_changed = false;
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::ModuleMetaData *modMD = nullptr;
  IGC::CodeGenContext *m_pCtx = nullptr;
};

// Legacy Pass Manager wrapper.
class BreakdownIntrinsicPassLPM : public llvm::FunctionPass {
public:
  static char ID;

  BreakdownIntrinsicPassLPM();

  ~BreakdownIntrinsicPassLPM() {}

  virtual llvm::StringRef getPassName() const override { return BreakdownIntrinsicPass::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData(),
                                getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  // Reused across functions, mirroring the single legacy instance whose m_changed
  // accumulates over the module.
  BreakdownIntrinsicPass m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions: the seeded IGC context analyses are module-level, and IGC passes never
// use skipFunction, so the per-function result is identical. name() returns the
// legacy pass argument so PrintBefore/PrintAfter=<pass argument> matches.
class BreakdownIntrinsicPassNPM : public llvm::PassInfoMixin<BreakdownIntrinsicPassNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "breakdown-intrinsics"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
