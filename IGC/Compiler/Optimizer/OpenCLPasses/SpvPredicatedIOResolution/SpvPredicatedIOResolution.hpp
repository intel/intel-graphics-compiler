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
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
class SpvPredicatedIOResolution final : public llvm::ModulePass, public llvm::InstVisitor<SpvPredicatedIOResolution> {
public:
  static char ID;

  SpvPredicatedIOResolution();
  ~SpvPredicatedIOResolution() {}

  virtual llvm::StringRef getPassName() const override { return "SpvPredicatedIOResolution"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override;
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
}; // namespace IGC