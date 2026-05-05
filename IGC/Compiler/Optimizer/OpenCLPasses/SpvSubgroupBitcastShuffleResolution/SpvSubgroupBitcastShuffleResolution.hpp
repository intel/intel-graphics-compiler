/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
class SpvSubgroupBitcastShuffleResolution final : public llvm::ModulePass,
                                                  public llvm::InstVisitor<SpvSubgroupBitcastShuffleResolution> {
public:
  static char ID;

  SpvSubgroupBitcastShuffleResolution();
  ~SpvSubgroupBitcastShuffleResolution() override = default;

  llvm::StringRef getPassName() const override { return "SpvSubgroupBitcastShuffleResolution"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;
  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_Changed = false;
  IGC::CodeGenContext *m_Ctx = nullptr;
};
} // namespace IGC
