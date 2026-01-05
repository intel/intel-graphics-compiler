/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_LEGACY_DEMANDEDBITS_H
#define IGCLLVM_ANALYSIS_LEGACY_DEMANDEDBITS_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct DemandedBitsLegacyPassWrapper : public FunctionPass {
  DemandedBitsLegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedDemandedBits"; }

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};

FunctionPass *createLegacyWrappedDemandedBitsPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_LEGACY_DEMANDEDBITS_H
