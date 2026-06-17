/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of ResolveInlineSamplerForBindless llvm pass.
/// This pass searches for __bindless_sampler_initializer(int32) calls and
/// replaces them with implicit arguments for inline samplers added by
/// AddImplicitArgs pass.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include <common/LLVMWarningsPush.hpp>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveInlineSamplerForBindless : public llvm::InstVisitor<ResolveInlineSamplerForBindless> {
public:
  ResolveInlineSamplerForBindless() {}
  ~ResolveInlineSamplerForBindless() {}

  static llvm::StringRef getPassName() { return "ResolveInlineSamplerForBindless"; }

  void visitCallInst(llvm::CallInst &CI);

  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

private:
  IGC::IGCMD::MetaDataUtils *mMDUtils = nullptr;
  IGC::ModuleMetaData *mModMD = nullptr;

  bool mChanged = false;
};

// Legacy Pass Manager wrapper.
class ResolveInlineSamplerForBindlessLPM : public llvm::FunctionPass {
public:
  static char ID;

  ResolveInlineSamplerForBindlessLPM();
  ~ResolveInlineSamplerForBindlessLPM() override = default;

  llvm::StringRef getPassName() const override { return ResolveInlineSamplerForBindless::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

private:
  ResolveInlineSamplerForBindless m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions (the seeded MetaDataUtilsAnalysis is module-level; IGC passes never use
// skipFunction). name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class ResolveInlineSamplerForBindlessNPM : public llvm::PassInfoMixin<ResolveInlineSamplerForBindlessNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "resolve-inline-sampler-for-bindless"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
