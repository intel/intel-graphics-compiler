/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/CastToGASAnalysis.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC {
/// @brief  Optimizes GenericCastToPtrExplicit calls by replacing them with
///         addrspace_cast instructions.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class GenericCastToPtrOpt {
public:
  GenericCastToPtrOpt() {}
  ~GenericCastToPtrOpt() {}

  static llvm::StringRef getPassName() { return "GenericCastToPtrOpt"; }

  bool run(llvm::Module &M, GASInfo &GI, llvm::CallGraph &CG);
};

// Legacy Pass Manager wrapper.
class GenericCastToPtrOptLPM : public llvm::ModulePass {
public:
  static char ID;

  GenericCastToPtrOptLPM();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  StringRef getPassName() const override { return GenericCastToPtrOpt::getPassName(); }

  bool runOnModule(llvm::Module &M) override {
    if (skipModule(M))
      return false;
    return m_impl.run(M, getAnalysis<CastToGASAnalysis>().getGASInfo(),
                      getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph());
  }

private:
  GenericCastToPtrOpt m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. The CastToGASAnalysis result is computed inline (it depends only on
// the module + call graph + context). The call graph comes from LLVM's standard CallGraphAnalysis
// (registered by PassBuilder::registerModuleAnalyses in IGCNewPassManager). name() returns the
// legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class GenericCastToPtrOptNPM : public llvm::PassInfoMixin<GenericCastToPtrOptNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-generic-cast-to-ptr-opt"; }
  static bool isRequired() { return true; }

private:
  GenericCastToPtrOpt m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
