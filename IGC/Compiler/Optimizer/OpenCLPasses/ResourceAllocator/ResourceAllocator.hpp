/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionArgAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief This pass allocates UAV and SRV numbers to kernel arguments.
// Shared implementation. Holds the per-function logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResourceAllocator {
public:
  ResourceAllocator() {}
  ~ResourceAllocator() {}

  static llvm::StringRef getPassName() { return "ResourceAllocatorPass"; }

  /// @brief  Per-function entry point. EAA is the ExtensionArgAnalysis result for F (obtained
  ///         via getAnalysis<> under the legacy pass manager, or computed inline under NPM).
  bool runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD,
                     IGC::CodeGenContext *pCtx, ExtensionArgAnalysis &EAA);
};

// Legacy Pass Manager wrapper.
class ResourceAllocatorLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  ResourceAllocatorLPM();

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return ResourceAllocator::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<ExtensionArgAnalysisLPM>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  /// @brief  Main entry point.
  /// @param  M The destination module.
  virtual bool runOnModule(llvm::Module &M) override;

private:
  ResourceAllocator m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. The ExtensionArgAnalysis result is computed inline per kernel
// (it depends only on the function + MetaDataUtils, no analysis-manager state). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class ResourceAllocatorNPM : public llvm::PassInfoMixin<ResourceAllocatorNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-resource-allocator"; }
  static bool isRequired() { return true; }

private:
  ResourceAllocator m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
