/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_DSE_LEGACY_H
#define IGCLLVM_TRANSFORMS_SCALAR_DSE_LEGACY_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct DSELegacyPassWrapper : public FunctionPass {
  DSELegacyPassWrapper();
  static char ID;
  bool runOnFunction(llvm::Function &F);
  void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

FunctionPass *createLegacyWrappedDeadStoreEliminationPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_DSE_LEGACY_H
