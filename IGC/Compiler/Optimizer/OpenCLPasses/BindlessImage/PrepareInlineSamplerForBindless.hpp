/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of PrepareInlineSamplerForBindless llvm pass.
/// This pass searches for __bindless_sampler_initializer(int32) calls, maps
/// them to inline sampler implicit args that will be added in AddImplicitArgs
/// pass and creates metadata for the inline samplers.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include <common/LLVMWarningsPush.hpp>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PrepareInlineSamplerForBindless : public llvm::InstVisitor<PrepareInlineSamplerForBindless> {
public:
  PrepareInlineSamplerForBindless() {}
  ~PrepareInlineSamplerForBindless() {}

  static llvm::StringRef getPassName() { return "PrepareInlineSamplerForBindless"; }

  void visitCallInst(llvm::CallInst &CI);

  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

private:
  IGC::IGCMD::MetaDataUtils *mMDUtils = nullptr;

  IGC::ModuleMetaData *mModMD = nullptr;

  ImplicitArg::ArgMap mArgMap{};

  int mInlineSamplerIndex = 0;

  bool mChanged = false;
};

// Legacy Pass Manager wrapper.
class PrepareInlineSamplerForBindlessLPM : public llvm::FunctionPass {
public:
  static char ID;

  PrepareInlineSamplerForBindlessLPM();
  ~PrepareInlineSamplerForBindlessLPM() override = default;

  llvm::StringRef getPassName() const override { return PrepareInlineSamplerForBindless::getPassName(); }

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
  PrepareInlineSamplerForBindless m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions
// (the seeded MetaDataUtilsAnalysis is module-level; IGC passes never use skipFunction). name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class PrepareInlineSamplerForBindlessNPM : public llvm::PassInfoMixin<PrepareInlineSamplerForBindlessNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "prepare-inline-sampler-for-bindless"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
