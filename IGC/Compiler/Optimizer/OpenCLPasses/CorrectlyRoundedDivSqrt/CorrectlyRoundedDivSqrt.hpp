/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  if the proper compiler option is present, makes sure usages of single precision floating point
///         sqrt and divide are IEEE compliant, otherwise does nothing.
///         Achieved by replacing div and sqrt with calls to compliant versions.
// Shared implementation. Holds the logic and the pass configuration (m_forceCR /
// m_hasHalfTy) and is used by both the legacy and the new-pass-manager wrappers below;
// it is not itself an llvm::Pass.
class CorrectlyRoundedDivSqrt : public llvm::InstVisitor<CorrectlyRoundedDivSqrt> {
public:
  CorrectlyRoundedDivSqrt() : m_forceCR(false), m_hasHalfTy(false), m_IsCorrectlyRounded(false) {}

  CorrectlyRoundedDivSqrt(bool forceCR, bool HasHalf)
      : m_forceCR(forceCR), m_hasHalfTy(HasHalf), m_IsCorrectlyRounded(false) {}

  ~CorrectlyRoundedDivSqrt() {}

  static llvm::StringRef getPassName() { return "CorrectlyRoundedDivSqrt"; }

  bool run(llvm::Module &M, ModuleMetaData *pModMD);

  /// @brief  replace given divide instruction with a call to a correctly rounded version.
  /// @param  I - the fdiv instruction.
  void visitFDiv(llvm::BinaryOperator &I);

private:
  /// @brief  if the given function is a sqrt function, replace the declaration with a correctly rounded version.
  /// @param  F - the function.
  /// @return true if the function was changed.
  static bool processDeclaration(llvm::Function &F);

  llvm::Value *emitIEEEDivide(llvm::BinaryOperator *I, llvm::Value *Op0, llvm::Value *Op1);

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed = false;

  /// @brief  Indicates that correctly rounded sqrt/div should be used even
  ///         when the option is not present in the module metadata.
  bool m_forceCR;

  bool m_hasHalfTy;

  bool m_IsCorrectlyRounded;

  llvm::Module *m_module = nullptr;
};

// Legacy Pass Manager wrapper.
class CorrectlyRoundedDivSqrtLPM : public llvm::ModulePass {
public:
  /// @brief  Pass identification.
  static char ID;

  CorrectlyRoundedDivSqrtLPM();
  CorrectlyRoundedDivSqrtLPM(bool forceCR, bool HasHalf);
  ~CorrectlyRoundedDivSqrtLPM() {}

  virtual llvm::StringRef getPassName() const override { return CorrectlyRoundedDivSqrt::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

private:
  CorrectlyRoundedDivSqrt m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via
// its constructors. name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class CorrectlyRoundedDivSqrtNPM : public llvm::PassInfoMixin<CorrectlyRoundedDivSqrtNPM> {
public:
  CorrectlyRoundedDivSqrtNPM() {}
  CorrectlyRoundedDivSqrtNPM(bool forceCR, bool HasHalf) : m_impl(forceCR, HasHalf) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-correctly-rounded-div-sqrt"; }
  static bool isRequired() { return true; }

private:
  CorrectlyRoundedDivSqrt m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
