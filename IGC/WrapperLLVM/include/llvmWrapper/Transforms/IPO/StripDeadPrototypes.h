/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_LEGACY_STRIPDEADPROTOTYPES_H
#define IGCLLVM_TRANSFORMS_IPO_LEGACY_STRIPDEADPROTOTYPES_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct StripDeadPrototypesLegacyPassWrapper : public ModulePass {
  StripDeadPrototypesLegacyPassWrapper();
  static char ID;

  bool runOnModule(llvm::Module &M) override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedStripDeadPrototypes"; }

private:
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

ModulePass *createLegacyWrappedStripDeadPrototypesPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_LEGACY_STRIPDEADPROTOTYPES_H
