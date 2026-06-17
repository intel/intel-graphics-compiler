/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass sets llvm fast math flags to relevant instructions, according
///         to the present compiler options.
///         -no-signed-zeros and -unsafe-math-optimizations sets nsz flag
///         -finite-math-only sets nnan and ninf flags
///         -fast-relaxed-math sets fast flag which implies all others (including arcp)
// Shared implementation. Holds the logic and the pass configuration (m_Mask /
// m_skipUnsafeFpMathAttr) and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass.
class SetFastMathFlags {
public:
  SetFastMathFlags() { m_Mask.setFast(); }

  SetFastMathFlags(llvm::FastMathFlags Mask) : m_Mask(Mask) {}

  SetFastMathFlags(llvm::FastMathFlags Mask, bool skipUnsafeFpMathAttr)
      : m_Mask(Mask), m_skipUnsafeFpMathAttr(skipUnsafeFpMathAttr) {}

  ~SetFastMathFlags() {}

  static llvm::StringRef getPassName() { return "SetFastMathFlags"; }

  bool run(llvm::Module &M, ModuleMetaData *pModMD);

private:
  /// @brief  sets the given flags to all instruction supporting fast math flags in the given module.
  /// @param  F - the function
  /// @param  fmfs - the fast math flags
  /// @return true if made any changes to the module.
  static bool setFlags(llvm::Function &F, llvm::FastMathFlags fmfs);

  llvm::FastMathFlags m_Mask;

  // We need to distinguish between when the compiler uses the FastRelaxedMath flag alone
  // and when uses the FastRelaxedMath flag with unsafe-fp-math attribute.
  // For the option one, we set the new bool variable to true (we skip unsafe-fp-math attribute)
  bool m_skipUnsafeFpMathAttr = false;
};

// Legacy Pass Manager wrapper.
class SetFastMathFlagsLPM : public llvm::ModulePass {
public:
  /// @brief  Pass identification.
  static char ID;

  SetFastMathFlagsLPM();
  SetFastMathFlagsLPM(llvm::FastMathFlags Mask);
  SetFastMathFlagsLPM(llvm::FastMathFlags Mask, bool skipUnsafeFpMathAttr);
  ~SetFastMathFlagsLPM() {}

  virtual llvm::StringRef getPassName() const override { return SetFastMathFlags::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

private:
  SetFastMathFlags m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via
// its constructors. name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class SetFastMathFlagsNPM : public llvm::PassInfoMixin<SetFastMathFlagsNPM> {
public:
  SetFastMathFlagsNPM() {}
  SetFastMathFlagsNPM(llvm::FastMathFlags Mask) : m_impl(Mask) {}
  SetFastMathFlagsNPM(llvm::FastMathFlags Mask, bool skipUnsafeFpMathAttr) : m_impl(Mask, skipUnsafeFpMathAttr) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-set-fast-math-flags"; }
  static bool isRequired() { return true; }

private:
  SetFastMathFlags m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
