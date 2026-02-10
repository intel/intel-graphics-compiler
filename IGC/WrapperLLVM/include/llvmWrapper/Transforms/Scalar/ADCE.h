/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

#if LLVM_VERSION_MAJOR >= 16
struct ADCELegacyPassWrapper : public FunctionPass {
  ADCELegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedADCE"; }

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};
#endif // LLVM_VERSION_MAJOR >= 16

inline FunctionPass *createLegacyWrappedADCEPass() {
#if LLVM_VERSION_MAJOR >= 16
  return new ADCELegacyPassWrapper();
#else
  return llvm::createAggressiveDCEPass();
#endif
}

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_ADCE_H
