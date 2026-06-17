/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <memory>
#include "llvmWrapper/IR/Module.h"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BufferBoundsCheckingPatcher : public llvm::InstVisitor<BufferBoundsCheckingPatcher> {
public:
  static constexpr const char *BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME = "__bufferboundschecking.bufferSizePlaceholder";

  BufferBoundsCheckingPatcher() {}
  ~BufferBoundsCheckingPatcher() = default;

  static llvm::StringRef getPassName() { return "BufferBoundsCheckingPatcher"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  void visitCallInst(llvm::CallInst &icmp);

private:
  std::unique_ptr<ImplicitArgs> implicitArgs;
  IGCMD::MetaDataUtils *metadataUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
  llvm::SmallVector<llvm::CallInst *, 8> toRemove;

  llvm::Argument *getBufferSizeArg(llvm::Function *function, uint32_t n);
};

// Legacy Pass Manager wrapper.
class BufferBoundsCheckingPatcherLPM : public llvm::ModulePass {
public:
  static char ID;

  BufferBoundsCheckingPatcherLPM();
  ~BufferBoundsCheckingPatcherLPM() = default;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &analysisUsage) const override {
    analysisUsage.addRequired<MetaDataUtilsWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return BufferBoundsCheckingPatcher::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

private:
  BufferBoundsCheckingPatcher m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class BufferBoundsCheckingPatcherNPM : public llvm::PassInfoMixin<BufferBoundsCheckingPatcherNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-buffer-bounds-checking-patcher"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
