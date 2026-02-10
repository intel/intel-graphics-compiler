/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_LEGACY_STRIPNONLINETABLEDEBUG_H
#define IGCLLVM_TRANSFORMS_UTILS_LEGACY_STRIPNONLINETABLEDEBUG_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct StripNonLineTableDebugLegacyPassWrapper : public ModulePass {
  StripNonLineTableDebugLegacyPassWrapper();
  static char ID;

  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedStripNonLineTableDebug"; }

private:
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

ModulePass *createLegacyWrappedStripNonLineTableDebugPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_UTILS_LEGACY_STRIPNONLINETABLEDEBUG_H
