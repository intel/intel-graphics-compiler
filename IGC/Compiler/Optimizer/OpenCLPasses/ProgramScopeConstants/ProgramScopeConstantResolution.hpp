/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass resolves references to inline constants
// Shared implementation. Holds the logic and the pass configuration (RunCautiously) and is
// used by both the legacy and the new-pass-manager wrappers below; it is not itself an
// llvm::Pass.
class ProgramScopeConstantResolution {
public:
  ProgramScopeConstantResolution(bool RunCautiously = false) : RunCautiously(RunCautiously) {}
  ~ProgramScopeConstantResolution() {}

  static llvm::StringRef getPassName() { return "ProgramScopeConstantResolutionPass"; }

  /// @brief  Main entry point.
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD);

private:
  bool RunCautiously;
};

// Legacy Pass Manager wrapper.
class ProgramScopeConstantResolutionLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  ProgramScopeConstantResolutionLPM(bool RunCautiously = false);
  ~ProgramScopeConstantResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return ProgramScopeConstantResolution::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

private:
  ProgramScopeConstantResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via its
// constructor. name() returns the legacy pass argument so PrintBefore/PrintAfter matches
// under the new pass manager.
class ProgramScopeConstantResolutionNPM : public llvm::PassInfoMixin<ProgramScopeConstantResolutionNPM> {
public:
  ProgramScopeConstantResolutionNPM(bool RunCautiously = false) : m_impl(RunCautiously) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-programscope-constant-resolve"; }
  static bool isRequired() { return true; }

private:
  ProgramScopeConstantResolution m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
