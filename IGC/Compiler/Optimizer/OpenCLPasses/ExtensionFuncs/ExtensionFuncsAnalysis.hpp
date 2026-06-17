/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  ExtensionFuncsAnalysis pass used for analyzing if VME functions are used in the
///         different functions in the module and creating metadata that represents
///         the implicit information needed by each function for resolving these function calls

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ExtensionFuncsAnalysis : public llvm::InstVisitor<ExtensionFuncsAnalysis> {
public:
  /// @brief  Constructor
  ExtensionFuncsAnalysis() {}

  /// @brief  Destructor
  ~ExtensionFuncsAnalysis() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "ExtensionFuncsAnalysis"; }

  /// @brief  Main entry point.
  ///         Runs on all functions defined in the given module, finds all VME function calls,
  ///         analyzes them and creates metadata that represents the implicit information needed
  ///         by each function for resolving these function calls
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  /// @brief  Function entry point.
  ///         Finds all VME function calls in this function, analyzes them and creates
  ///         metadata that represents the implicit information needed by this function
  ///         for resolving these function calls
  /// @param  F The destination function.
  bool runOnFunction(llvm::Function &F);

  /// @brief  Call instructions visitor.
  ///         Checks for VME functions and analyzes it
  /// @param  CI The call instruction.
  void visitCallInst(llvm::CallInst &CI);

  static const llvm::StringRef VME_MB_BLOCK_TYPE;
  static const llvm::StringRef VME_SUBPIXEL_MODE;
  static const llvm::StringRef VME_SAD_ADJUST_MODE;
  static const llvm::StringRef VME_SEARCH_PATH_TYPE;
  static const llvm::StringRef VME_HELPER_GET_HANDLE;
  static const llvm::StringRef VME_HELPER_GET_AS;

private:
  /// @brief  Marks whether VME information is needed by the current function
  bool m_hasVME{};
  /// @brief  MetaData utils used to generate LLVM metadata
  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
};

// Legacy Pass Manager wrapper.
class ExtensionFuncsAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  ExtensionFuncsAnalysisLPM();
  ~ExtensionFuncsAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return ExtensionFuncsAnalysis::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return ExtensionFuncsAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                        getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ExtensionFuncsAnalysisNPM : public llvm::PassInfoMixin<ExtensionFuncsAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-extension-funcs-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
