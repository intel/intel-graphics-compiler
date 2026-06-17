/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "Compiler/InitializePasses.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace IGC {

llvm::FunctionPass *createStaticGASResolution();

// Shared implementation. Holds the logic and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass. The module-level GASInfo (from CastToGASAnalysis)
// is injected by the caller (runOnFunction).
class StaticGASResolution {
public:
  bool runOnFunction(Function &F, GASInfo *GI);

private:
  GASInfo *m_GI = nullptr;
};

// Legacy Pass Manager wrapper.
class StaticGASResolutionLPM : public FunctionPass {
public:
  static char ID;

  StaticGASResolutionLPM() : FunctionPass(ID) {
    initializeStaticGASResolutionLPMPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    return m_impl.runOnFunction(F, &getAnalysis<CastToGASAnalysis>().getGASInfo());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CastToGASAnalysis>(); }

  virtual StringRef getPassName() const override { return "StaticGASResolution"; }

private:
  StaticGASResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that computes the CastToGASAnalysis result
// inline (the seeded analyses are module-level) then loops the defined functions. name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class StaticGASResolutionNPM : public llvm::PassInfoMixin<StaticGASResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "static-gas-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC
