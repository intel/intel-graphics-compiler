/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_WARNMISSEDTRANSFORMS_LEGACY_H
#define IGCLLVM_TRANSFORMS_SCALAR_WARNMISSEDTRANSFORMS_LEGACY_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct WarnMissedTransformsLegacyPassWrapper : public FunctionPass {
  WarnMissedTransformsLegacyPassWrapper();
  static char ID;
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedWarnMissedTransforms"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

Pass *createLegacyWrappedWarnMissedTransformsPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_WARNMISSEDTRANSFORMS_LEGACY_H
