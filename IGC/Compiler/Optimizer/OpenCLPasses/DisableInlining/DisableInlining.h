/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class DisableInlining final {
public:
  DisableInlining() {}
  ~DisableInlining() {}

  static llvm::StringRef getPassName() { return "DisableInlining"; }

  bool run(llvm::Function &F);
};

// Legacy Pass Manager wrapper.
class DisableInliningLPM final : public llvm::FunctionPass {
public:
  static char ID;

  DisableInliningLPM();
  ~DisableInliningLPM() {}

  llvm::StringRef getPassName() const override { return DisableInlining::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnFunction(llvm::Function &F) override { return DisableInlining().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class DisableInliningNPM : public llvm::PassInfoMixin<DisableInliningNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-disable-inlining"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
}; // namespace IGC
