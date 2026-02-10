/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_JUMPTHREADING_LEGACY_H
#define IGCLLVM_TRANSFORMS_SCALAR_JUMPTHREADING_LEGACY_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct JumpThreadingPassWrapper : public FunctionPass {
  JumpThreadingPassWrapper();
  JumpThreadingPassWrapper(int Threshold);
#if LLVM_VERSION_MAJOR <= 14
  JumpThreadingPassWrapper(bool InsertFreezeWhenUnfoldingSelect, int Threshold);
#endif
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedJumpThreading"; }

private:
  PassBuilder PB;
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  int Threshold = -1;
};
FunctionPass *createLegacyWrappedJumpThreadingPass(int Threshold = -1);
#if LLVM_VERSION_MAJOR <= 14
FunctionPass *createLegacyWrappedJumpThreadingPass(bool InsertFreezeWhenUnfoldingSelect, int Threshold = -1);
#endif
} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_JUMPTHREADING_LEGACY_H
