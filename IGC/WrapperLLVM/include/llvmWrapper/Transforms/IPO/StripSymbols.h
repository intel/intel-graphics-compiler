/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_STRIPDEADDEBUGINFO_LEGACY_H
#define IGCLLVM_TRANSFORMS_IPO_STRIPDEADDEBUGINFO_LEGACY_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct StripDeadDebugInfoLegacyPassWrapper : public ModulePass {
  StripDeadDebugInfoLegacyPassWrapper();
  static char ID;
  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedStripDeadDebugInfo"; }

private:
  PassBuilder PB;
  ModuleAnalysisManager MAM;
};

ModulePass *createLegacyWrappedStripDeadDebugInfoPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_STRIPDEADDEBUGINFO_LEGACY_H
