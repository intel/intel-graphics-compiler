/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// A pass that walks over call instructions and replaces all __builtin_IB_work_group
// with corresponding GenISA intrinsics.
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class WGFuncResolution : public llvm::InstVisitor<WGFuncResolution> {
public:
  WGFuncResolution() {}
  ~WGFuncResolution() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "WGFuncResolution"; }

  // Entry point of the pass.
  bool run(llvm::Module &M);

  // Call instructions visitor.
  void visitCallInst(llvm::CallInst &callInst);

private:
  llvm::Module *m_pModule = nullptr;

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed{};
};

// Legacy Pass Manager wrapper.
class WGFuncResolutionLPM : public llvm::ModulePass {
public:
  static char ID;

  WGFuncResolutionLPM();
  ~WGFuncResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return WGFuncResolution::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  virtual bool runOnModule(llvm::Module &M) override { return WGFuncResolution().run(M); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class WGFuncResolutionNPM : public llvm::PassInfoMixin<WGFuncResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-wg-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
