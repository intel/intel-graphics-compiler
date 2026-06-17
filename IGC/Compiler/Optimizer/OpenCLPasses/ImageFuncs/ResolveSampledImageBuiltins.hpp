/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

namespace IGC {
/// @brief  ResolveSampledImageBuiltins pass is used for resolving getter builtins operating on VMEImageINTEL and
/// SampledImage objects.
///         SPIR-V Friendly IR represents OpVMEImageINTEL and OpSampledImage opcodes as a functions returning global
///         pointer to opaque type. Since it's not convenient to allocate global memory within BiFModule, these builtins
///         are just declared there and resolved in this pass.

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveSampledImageBuiltins : public llvm::InstVisitor<ResolveSampledImageBuiltins> {
public:
  ResolveSampledImageBuiltins() {}
  ~ResolveSampledImageBuiltins() {}

  static llvm::StringRef getPassName() { return "ResolveSampledImageBuiltins"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);
  void visitCallInst(llvm::CallInst &CI);

  static const llvm::StringRef GET_IMAGE;
  static const llvm::StringRef GET_SAMPLER;

private:
  llvm::Value *lowerGetImage(llvm::CallInst &CI);
  llvm::Value *lowerGetSampler(llvm::CallInst &CI);

  bool m_changed = false;
  ModuleMetaData *modMD = nullptr;
  std::unordered_set<llvm::CallInst *> m_builtinsToRemove;
};

// Legacy Pass Manager wrapper.
class ResolveSampledImageBuiltinsLPM : public llvm::ModulePass {
public:
  static char ID;

  ResolveSampledImageBuiltinsLPM();
  ~ResolveSampledImageBuiltinsLPM() {}

  virtual llvm::StringRef getPassName() const override { return ResolveSampledImageBuiltins::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return ResolveSampledImageBuiltins().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ResolveSampledImageBuiltinsNPM : public llvm::PassInfoMixin<ResolveSampledImageBuiltinsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-image-sampler-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
