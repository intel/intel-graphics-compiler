/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LICM_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LICM_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct LICMLegacyPassWrapper : public FunctionPass {
  LICMLegacyPassWrapper(unsigned LicmMssaOptCap = 100, unsigned LicmMssaNoAccForPromotionCap = 250,
                        bool LicmAllowSpeculation = true);
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLICM"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
  unsigned LicmMssaOptCap;
  unsigned LicmMssaNoAccForPromotionCap;
  bool LicmAllowSpeculation;
};

llvm::Pass *createLegacyWrappedLICMPass();
llvm::Pass *createLegacyWrappedLICMPass(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                        bool LicmAllowSpeculation);
} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LICM_H