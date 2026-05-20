/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPINSTSIMPLIFY_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPINSTSIMPLIFY_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct LoopInstSimplifyLegacyPassWrapper : public FunctionPass {
  LoopInstSimplifyLegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLoopInstSimplify"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

llvm::Pass *createLoopInstSimplifyPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPINSTSIMPLIFY_H
