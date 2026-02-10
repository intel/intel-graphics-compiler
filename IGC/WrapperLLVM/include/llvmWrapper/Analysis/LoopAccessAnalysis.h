/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_LEGACY_LOOPACCESSANALYSIS_H
#define IGCLLVM_ANALYSIS_LEGACY_LOOPACCESSANALYSIS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct LoopAccessAnalysisLegacyPassWrapper : public FunctionPass {
  LoopAccessAnalysisLegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedLoopAccessAnalysis"; }

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};

FunctionPass *createLegacyWrappedLoopAccessAnalysisPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_LEGACY_LOOPACCESSANALYSIS_H
