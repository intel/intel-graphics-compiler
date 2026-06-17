/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace llvm {
class Argument;
}

namespace IGC {
/// @brief  ExtensionArgAnalysis pass used for analyzing if VME extension functions arguments.
///         This information needed by ResourceAllocator and helps create the right VME/media patch tokens

// Shared implementation. Holds the analysis data + queries and is used both as the legacy
// FunctionPass result and (computed inline) by the ResourceAllocator new-pass-manager wrapper;
// it is not itself an llvm::Pass.
class ExtensionArgAnalysis : public llvm::InstVisitor<ExtensionArgAnalysis> {
public:
  ExtensionArgAnalysis() {}
  ~ExtensionArgAnalysis() {}

  static llvm::StringRef getPassName() { return "ExtensionArgAnalysis"; }

  /// @brief  Function entry point.
  ///         Checks if this is a VME function and analyzes its function arguments (images, samplers)
  /// @param  F The destination function.
  void analyze(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);

  /// @brief  Returns true if the given argument is a VME image or sampler argument
  /// @param  arg A function argument
  /// @return true if the given argument is a VME image or sampler argument, false otherwise
  bool isMediaArg(const llvm::Argument *arg) {
    return (m_MediaArgs.count(const_cast<llvm::Argument *>(arg)) > 0) ? true : false;
  }
  bool isMediaSamplerArg(const llvm::Argument *arg) {
    return (m_MediaSamplerArgs.count(const_cast<llvm::Argument *>(arg)) > 0) ? true : false;
  }
  bool isMediaBlockArg(const llvm::Argument *arg) {
    return (m_MediaBlockArgs.count(const_cast<llvm::Argument *>(arg)) > 0) ? true : false;
  }
  bool isVaArg(const llvm::Argument *arg) {
    return (m_vaArgs.count(const_cast<llvm::Argument *>(arg)) > 0) ? true : false;
  }

  ResourceExtensionTypeEnum GetExtensionSamplerType() { return m_extensionType; }

  void visitCallInst(llvm::CallInst &CI);

private:
  llvm::DenseMap<llvm::Argument *, ResourceExtensionTypeEnum> m_ExtensionMap;

  /// @brief  Contains the VME image and sampler arguments of the function
  llvm::SmallPtrSet<llvm::Argument *, 3> m_MediaArgs;
  llvm::SmallPtrSet<llvm::Argument *, 3> m_MediaSamplerArgs;
  llvm::SmallPtrSet<llvm::Argument *, 3> m_MediaBlockArgs;
  llvm::SmallPtrSet<llvm::Argument *, 2> m_vaArgs;
  ResourceExtensionTypeEnum m_extensionType = ResourceExtensionTypeEnum::NonExtensionType;

  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::CodeGenContext *m_pCtx = nullptr;
};

// Legacy Pass Manager wrapper. Its per-function result (the shared ExtensionArgAnalysis) is
// consumed by ResourceAllocator via getAnalysis<ExtensionArgAnalysisLPM>(F).getResult().
class ExtensionArgAnalysisLPM : public llvm::FunctionPass {
public:
  static char ID;

  ExtensionArgAnalysisLPM();
  ~ExtensionArgAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return ExtensionArgAnalysis::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  bool runOnFunction(llvm::Function &F) override {
    // The legacy pass only reached getAnalysis<CodeGenContextWrapper> on error paths inside
    // visitCallInst, relying on a parent pass (ResourceAllocator) having required the immutable
    // CodeGenContextWrapper. Declaring an explicit addRequired here would force the (nested) pass
    // manager to default-construct CodeGenContextWrapper, which asserts. So fetch it lazily: it is
    // present whenever the error paths can actually fire, and null otherwise (errors not reachable).
    CodeGenContext *ctx = nullptr;
    if (auto *cgw = getAnalysisIfAvailable<CodeGenContextWrapper>())
      ctx = cgw->getCodeGenContext();
    m_data.analyze(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), ctx);
    return false;
  }

  /// @brief  The analysis result for the last analyzed function.
  ExtensionArgAnalysis &getResult() { return m_data; }

private:
  ExtensionArgAnalysis m_data;
};

} // namespace IGC
