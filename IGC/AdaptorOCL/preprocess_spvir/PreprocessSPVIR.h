/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

#include <string>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PreprocessSPVIR : public llvm::InstVisitor<PreprocessSPVIR> {
public:
  PreprocessSPVIR() {}
  ~PreprocessSPVIR() {}

  static llvm::StringRef getPassName() { return "PreprocessSPVIR"; }

  bool run(llvm::Module &F);
  void visitCallInst(llvm::CallInst &CI);
  void visitOpenCLEISPrintf(llvm::CallInst &CI);

  static bool isSPVIR(llvm::StringRef funcName);

private:
  bool hasArrayArg(llvm::Function &F);
  void processBuiltinsWithArrayArguments(llvm::Function &F);
  void processBuiltinsWithArrayArguments();
  void removePointerAnnotations(llvm::Module &M);
  void createCallAndReplace(llvm::CallInst &oldCallInst, llvm::StringRef newFuncName, std::vector<llvm::Value *> &args);

  IGCLLVM::Module *m_Module = nullptr;
  llvm::IRBuilder<> *m_Builder = nullptr;
  bool m_changed = false;
};

// Legacy Pass Manager wrapper.
class PreprocessSPVIRLPM : public llvm::ModulePass {
public:
  static char ID;

  PreprocessSPVIRLPM();
  ~PreprocessSPVIRLPM() {}

  llvm::StringRef getPassName() const override { return PreprocessSPVIR::getPassName(); }

  bool runOnModule(llvm::Module &M) override { return PreprocessSPVIR().run(M); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper.
class PreprocessSPVIRNPM : public llvm::PassInfoMixin<PreprocessSPVIRNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-preprocess-spvir"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
