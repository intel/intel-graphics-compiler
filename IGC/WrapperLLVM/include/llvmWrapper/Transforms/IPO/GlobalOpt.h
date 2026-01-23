/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_GLOBALOPT_LEGACY_H
#define IGCLLVM_TRANSFORMS_IPO_GLOBALOPT_LEGACY_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct GlobalOptLegacyPassWrapper : public ModulePass {
  GlobalOptLegacyPassWrapper();
  static char ID;
  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedGlobalOpt"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

Pass *createLegacyWrappedGlobalOptPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_GLOBALOPT_LEGACY_H
