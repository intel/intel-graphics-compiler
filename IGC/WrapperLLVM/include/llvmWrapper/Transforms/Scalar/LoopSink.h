/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPSINK_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPSINK_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct LoopSinkLegacyPassWrapper : public FunctionPass {
  LoopSinkLegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLoopSink"; }

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};

FunctionPass *createLegacyWrappedLoopSinkPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPSINK_H
