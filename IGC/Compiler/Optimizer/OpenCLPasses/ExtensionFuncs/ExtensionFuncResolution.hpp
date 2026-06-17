/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  ExtensionFuncsResolution pass used for resolving VME functions.
///         This pass depends on the ExtensionFuncsAnalysis and AddImplicitArgs passes running before it
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ExtensionFuncsResolution : public llvm::InstVisitor<ExtensionFuncsResolution> {
public:
  /// @brief  Constructor
  ExtensionFuncsResolution() {}

  /// @brief  Destructor
  ~ExtensionFuncsResolution() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "ExtensionFuncsResolution"; }

  /// @brief  Main entry point.
  ///         Finds all VME function calls and resolve them into an llvm sequence
  /// @param  F The destination function.
  bool runOnFunction(llvm::Function &F, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  /// @brief  Call instructions visitor.
  ///         Checks for VME functions and resolves them into appropriate sequence of code
  /// @param  CI The call instruction.
  void visitCallInst(llvm::CallInst &CI);

private:
  /// @brief  The implicit arguments of the current function
  ImplicitArgs m_implicitArgs;

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed = false;
};

// Legacy Pass Manager wrapper.
class ExtensionFuncsResolutionLPM : public llvm::FunctionPass {
public:
  static char ID;

  ExtensionFuncsResolutionLPM();
  ~ExtensionFuncsResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return ExtensionFuncsResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

private:
  ExtensionFuncsResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions (the seeded MetaDataUtilsAnalysis is module-level; IGC passes never use
// skipFunction). name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class ExtensionFuncsResolutionNPM : public llvm::PassInfoMixin<ExtensionFuncsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-extension-funcs-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
