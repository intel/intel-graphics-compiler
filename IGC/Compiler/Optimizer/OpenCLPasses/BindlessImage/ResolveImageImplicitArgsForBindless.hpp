/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of ResolveImageImplicitArgsForBindless
/// LLVM pass.
///
/// This pass searches for built-in calls querying for image properties and
/// replaces them with calls to GenISA_ldraw_indexed intrinsic to fetch the
/// image properties from the ImageImplicitArgs struct in bindless mode.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include <common/LLVMWarningsPush.hpp>
#include <llvm/ADT/SetVector.h>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveImageImplicitArgsForBindless : public llvm::InstVisitor<ResolveImageImplicitArgsForBindless> {
public:
  ResolveImageImplicitArgsForBindless() {}
  ~ResolveImageImplicitArgsForBindless() {}

  static llvm::StringRef getPassName() { return "ResolveImageImplicitArgsForBindless"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);

  void visitCallInst(llvm::CallInst &CI);

private:
  llvm::SmallSetVector<llvm::Instruction *, 16> mInstsToRemove;

  bool mChanged = false;
};

// Legacy Pass Manager wrapper.
class ResolveImageImplicitArgsForBindlessLPM : public llvm::ModulePass {
public:
  static char ID;

  ResolveImageImplicitArgsForBindlessLPM();
  ~ResolveImageImplicitArgsForBindlessLPM() override = default;

  llvm::StringRef getPassName() const override { return ResolveImageImplicitArgsForBindless::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return ResolveImageImplicitArgsForBindless().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ResolveImageImplicitArgsForBindlessNPM : public llvm::PassInfoMixin<ResolveImageImplicitArgsForBindlessNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "resolve-image-implicit-args-for-bindless"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
