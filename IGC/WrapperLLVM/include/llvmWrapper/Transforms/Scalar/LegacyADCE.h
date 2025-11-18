/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct ADCELegacyPassWrapper : public FunctionPass {
  ADCELegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F);
  void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};

FunctionPass *createLegacyADCEPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H
