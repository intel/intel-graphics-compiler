/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  OpenCLPrintf Pass checks if there are printf calls inside a kernel
///         and adds two implicit arguments for printf output buffer and
///         maximum size of the buffer.
class OpenCLPrintfAnalysis : public llvm::ModulePass, public llvm::InstVisitor<OpenCLPrintfAnalysis> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  OpenCLPrintfAnalysis();

  /// @brief  Destructor
  ~OpenCLPrintfAnalysis() {}

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return "OpenCLPrintfAnalysis"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  /// @brief  Main entry point.
  /// @param  M The destination module.
  virtual bool runOnModule(llvm::Module &M) override;

  void visitCallInst(llvm::CallInst &callInst);

  static const llvm::StringRef OPENCL_PRINTF_FUNCTION_NAME;
  static const llvm::StringRef ONEAPI_PRINTF_FUNCTION_NAME;
  static const llvm::StringRef BUILTIN_PRINTF_FUNCTION_NAME;

  static bool isOpenCLPrintf(const llvm::Function *F);
  static bool isOneAPIPrintf(const llvm::Function *F);
  static bool isBuiltinPrintf(const llvm::Function *F);

  // Return true if every top level user of a string literal is a printf
  // call. Note that the function is expected to work only before printf
  // expansion.
  static bool isPrintfStringConstant(const llvm::GlobalVariable *GV);

private:
  /// @brief  Adds an implicit argument for address of printf output buffer
  ///         created by the Runtime and an implicit argument for maximum
  ///         size of the buffer.
  void addPrintfBufferArgs(llvm::Function &F);

  std::unordered_set<llvm::Function *> m_hasPrintfs;

  /// @brief  MetaData utils used to generate LLVM metadata
  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;
};

} // namespace IGC
