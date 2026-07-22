/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  OpenCLPrintf Pass checks if there are printf calls inside a kernel
///         and adds two implicit arguments for printf output buffer and
///         maximum size of the buffer.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class OpenCLPrintfAnalysis : public llvm::InstVisitor<OpenCLPrintfAnalysis> {
public:
  /// @brief  Constructor
  OpenCLPrintfAnalysis() {}

  /// @brief  Destructor
  ~OpenCLPrintfAnalysis() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "OpenCLPrintfAnalysis"; }

  /// @brief  Main entry point.
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD);

  void visitCallInst(llvm::CallInst &callInst);

  static const llvm::StringRef OPENCL_PRINTF_FUNCTION_NAME;
  static const llvm::StringRef BUILTIN_PRINTF_FUNCTION_NAME;

  static bool isOpenCLPrintf(const llvm::Function *F);
  static bool isBuiltinPrintf(const llvm::Function *F);

private:
  /// @brief  Adds an implicit argument for address of printf output buffer
  ///         created by the Runtime and an implicit argument for maximum
  ///         size of the buffer.
  void addPrintfBufferArgs(llvm::Function &F);

  std::unordered_set<llvm::Function *> m_hasPrintfs;

  ModuleMetaData *m_modMD = nullptr;
  /// @brief  MetaData utils used to generate LLVM metadata
  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;
};

// Legacy Pass Manager wrapper.
class OpenCLPrintfAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  OpenCLPrintfAnalysisLPM();
  ~OpenCLPrintfAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return OpenCLPrintfAnalysis::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return OpenCLPrintfAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class OpenCLPrintfAnalysisNPM : public llvm::PassInfoMixin<OpenCLPrintfAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-opencl-printf-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
