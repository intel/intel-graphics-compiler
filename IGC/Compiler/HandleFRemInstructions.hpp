/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// This pass replaces all occurences of frem instructions with proper builtin calls
// This is needed, because new SPIRV-LLVM translator outputs frem instructions
// which are not fully handled by IGC.
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class HandleFRemInstructions : public llvm::InstVisitor<HandleFRemInstructions> {
public:
  HandleFRemInstructions() {}

  static llvm::StringRef getPassName() { return "HandleFremInstructions"; }

  /// @brief Main entry point.
  ///        Find all frem instructions and replace them with proper builtin calls
  /// @param M The destination module.
  bool run(llvm::Module &M);

  void visitFRem(llvm::BinaryOperator &I);

private:
  llvm::Module *m_module = nullptr;
  bool m_changed = false;
};

// Legacy Pass Manager wrapper.
class HandleFRemInstructionsLPM : public llvm::ModulePass {
public:
  static char ID;

  HandleFRemInstructionsLPM();

  llvm::StringRef getPassName() const override { return HandleFRemInstructions::getPassName(); }

  bool runOnModule(llvm::Module &M) override { return HandleFRemInstructions().run(M); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class HandleFRemInstructionsNPM : public llvm::PassInfoMixin<HandleFRemInstructionsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-handle-frem-inst"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
