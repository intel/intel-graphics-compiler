/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/PassManager.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief Shared implementation of the BuiltinsConverter pass used for converting calls from
///  OpenCL function call Built-ins to common llvm GenISA intrinsics. It is used by both the legacy
///  and the new-pass-manager wrappers below; it is not itself an llvm::Pass. The CodeGenContext is
///  injected by the caller so the engine does not depend on getAnalysis<>.
class BuiltinsConverter : public llvm::InstVisitor<BuiltinsConverter> {
public:
  BuiltinsConverter() {}

  ~BuiltinsConverter() {}

  // @brief iterate on all the functions in the module and replace calls from __builtin_IB_* to the match
  //        GenISA intrinsics
  bool runOnFunction(llvm::Function &F, CodeGenContext *Ctx);
  void visitCallInst(llvm::CallInst &CI);

protected:
  bool fillIndexMap(llvm::Function &F);
  unsigned int getResourceIndex(llvm::MDNode *argResourceTypes, unsigned int argNo);

  CodeGenContext *m_pCtx = nullptr;
  CImagesBI::ParamMap m_argIndexMap;
  CImagesBI::InlineMap m_inlineIndexMap;
  int m_nextSampler{};

  CBuiltinsResolver *m_pResolve = nullptr;
};

// Legacy Pass Manager wrapper.
class BuiltinsConverterLPM : public llvm::FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  BuiltinsConverterLPM();

  ~BuiltinsConverterLPM() {}

  /// @brief Provides name of pass
  llvm::StringRef getPassName() const override { return "BuiltinsConverterFunction"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnFunction(llvm::Function &F) override;

private:
  BuiltinsConverter m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions (the
// seeded CodeGenContextAnalysis is module-level; IGC passes never use skipFunction). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class BuiltinsConverterNPM : public llvm::PassInfoMixin<BuiltinsConverterNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-conv-ocl-to-common"; }
  static llvm::StringRef getPassName() { return "BuiltinsConverterFunction"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC

extern "C" llvm::FunctionPass *createBuiltinsConverterPass(void);
