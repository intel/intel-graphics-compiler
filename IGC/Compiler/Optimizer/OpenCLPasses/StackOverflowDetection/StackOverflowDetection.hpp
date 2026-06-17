/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This inserts calls to stack overflow detection builtins.
///
// Shared implementation. Holds the logic and the pass configuration (mode) and is used by
// both the legacy and the new-pass-manager wrappers below; it is not itself an llvm::Pass.
class StackOverflowDetectionPass {
public:
  enum class Mode {
    Initialize,
    AnalyzeAndCleanup,
    RemoveDummyCalls,
  };

  static constexpr const char *STACK_OVERFLOW_INIT_BUILTIN_NAME = "__stackoverflow_init";
  static constexpr const char *STACK_OVERFLOW_DETECTION_BUILTIN_NAME = "__stackoverflow_detection";

  Mode mode = Mode::Initialize;

  StackOverflowDetectionPass() {}
  StackOverflowDetectionPass(Mode mode_) : mode(mode_) {}

  static llvm::StringRef getPassName() { return "StackOverflowDetectionPass"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD, CodeGenContext *pCtx);

  bool removeDummyCalls(llvm::Module &M);
  bool removeCallsAndFunctionsIfNoStackCallsOrVLA(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils,
                                                  ModuleMetaData *pModMD);

  bool attachDebugInfo(llvm::Module &M);
};

// Legacy Pass Manager wrapper.
class StackOverflowDetectionPassLPM : public llvm::ModulePass {
public:
  /// @brief  Pass identification.
  static char ID;

  StackOverflowDetectionPassLPM();
  StackOverflowDetectionPassLPM(StackOverflowDetectionPass::Mode mode_);
  ~StackOverflowDetectionPassLPM() {}

  virtual llvm::StringRef getPassName() const override { return StackOverflowDetectionPass::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    auto &MDUWAnalysis = getAnalysis<MetaDataUtilsWrapper>();
    return m_impl.run(M, MDUWAnalysis.getMetaDataUtils(), MDUWAnalysis.getModuleMetaData(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

private:
  StackOverflowDetectionPass m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via its
// constructors. name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class StackOverflowDetectionPassNPM : public llvm::PassInfoMixin<StackOverflowDetectionPassNPM> {
public:
  StackOverflowDetectionPassNPM() {}
  StackOverflowDetectionPassNPM(StackOverflowDetectionPass::Mode mode_) : m_impl(mode_) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-stackoverflow-detection"; }
  static bool isRequired() { return true; }

private:
  StackOverflowDetectionPass m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
