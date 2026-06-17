/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class SpvPredicatedIOResolution final : public llvm::InstVisitor<SpvPredicatedIOResolution> {
public:
  SpvPredicatedIOResolution() {}
  ~SpvPredicatedIOResolution() {}

  static llvm::StringRef getPassName() { return "SpvPredicatedIOResolution"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);
  void visitCallInst(llvm::CallInst &CI);

private:
  enum Operation {
    Load,
    Store,
  };

  template <Operation operation> void visitPredicatedSPVCallInst(llvm::CallInst &CI);
  llvm::Value *getDefaultAlignValue(llvm::Type *Ty) const;
  bool validateOperandType(const llvm::Value *V) const;

  llvm::DenseSet<llvm::Function *> m_BuiltinsToRemove;
  std::vector<llvm::Instruction *> m_InstructionsToErase;
  bool m_Changed = false;
  IGC::CodeGenContext *m_Ctx = nullptr;
  llvm::Module *m_Module = nullptr;
};

// Legacy Pass Manager wrapper.
class SpvPredicatedIOResolutionLPM final : public llvm::ModulePass {
public:
  static char ID;

  SpvPredicatedIOResolutionLPM();
  ~SpvPredicatedIOResolutionLPM() {}

  llvm::StringRef getPassName() const override { return SpvPredicatedIOResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return SpvPredicatedIOResolution().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class SpvPredicatedIOResolutionNPM : public llvm::PassInfoMixin<SpvPredicatedIOResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-spv-predicatedio-resolution"; }
  static llvm::StringRef getPassName() { return "SpvPredicatedIOResolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
}; // namespace IGC