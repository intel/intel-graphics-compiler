/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "IGC/BiFModule/Headers/bif_control_common.h"
#include "IGC/common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#define BIF_FLAG_CONTROL(BIF_FLAG_TYPE, BIF_FLAG_NAME) BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME),

#define BIF_FLAG_CTRL_SET(BIF_FLAG_NAME, BIF_FLAG_VALUE)                                                               \
  ListDelegates.emplace(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME), [this]() -> bool {                                           \
    return replace(BIF_FLAG_VALUE, pModule->getGlobalVariable(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME)));                      \
  })


namespace IGC {
/// @brief  BIFFlagCtrlResolution pass used for resolving BIF flag controls in
/// kernel.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BIFFlagCtrlResolution {
public:
  BIFFlagCtrlResolution() {}
  ~BIFFlagCtrlResolution() {}

  static llvm::StringRef getPassName() { return "BIFFlagCtrlResolution"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);

private:
  CodeGenContext *PtrCGC = nullptr;
  llvm::Module *pModule = nullptr;

  std::map<llvm::StringRef, std::function<bool()>> ListDelegates{};

  template <typename T> bool replace(T Value, llvm::GlobalVariable *GV);

  void FillFlagCtrl();
};

// Legacy Pass Manager wrapper.
class BIFFlagCtrlResolutionLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  BIFFlagCtrlResolutionLPM();

  BIFFlagCtrlResolutionLPM(CodeGenContext *ptrCGC);

  /// @brief  Destructor
  ~BIFFlagCtrlResolutionLPM() {}

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return BIFFlagCtrlResolution::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override { return m_impl.run(M, m_ctorCtx); }

private:
  BIFFlagCtrlResolution m_impl;
  CodeGenContext *m_ctorCtx = nullptr;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. The CodeGenContext comes from the seeded
// CodeGenContextAnalysis. name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class BIFFlagCtrlResolutionNPM : public llvm::PassInfoMixin<BIFFlagCtrlResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-bif-flag-control-resolution"; }
  static llvm::StringRef getPassName() { return "BIFFlagCtrlResolution"; }
  static bool isRequired() { return true; }

private:
  BIFFlagCtrlResolution m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
