/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class SpvSubgroupBitcastShuffleResolution final : public llvm::InstVisitor<SpvSubgroupBitcastShuffleResolution> {
public:
  SpvSubgroupBitcastShuffleResolution() {}
  ~SpvSubgroupBitcastShuffleResolution() = default;

  static llvm::StringRef getPassName() { return "SpvSubgroupBitcastShuffleResolution"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);
  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_Changed = false;
  IGC::CodeGenContext *m_Ctx = nullptr;
};

// Legacy Pass Manager wrapper.
class SpvSubgroupBitcastShuffleResolutionLPM final : public llvm::ModulePass {
public:
  static char ID;

  SpvSubgroupBitcastShuffleResolutionLPM();
  ~SpvSubgroupBitcastShuffleResolutionLPM() override = default;

  llvm::StringRef getPassName() const override { return SpvSubgroupBitcastShuffleResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return SpvSubgroupBitcastShuffleResolution().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class SpvSubgroupBitcastShuffleResolutionNPM : public llvm::PassInfoMixin<SpvSubgroupBitcastShuffleResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-spv-subgroup-bitcast-shuffle-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
