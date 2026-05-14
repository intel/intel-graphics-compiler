/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPROTATION_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPROTATION_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct LoopRotateLegacyPassWrapper : public FunctionPass {
  LoopRotateLegacyPassWrapper(bool EnableHeaderDuplication = true, bool PrepareForLTO = false);
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLoopRotate"; }

private:
  bool EnableHeaderDuplication;
  bool PrepareForLTO;
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

llvm::Pass *createLegacyWrappedLoopRotatePass(int MaxHeaderSize = -1, bool PrepareForLTO = false);

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPROTATION_H
