
/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_TLIMAPPING_LEGACY_H
#define IGCLLVM_TRANSFORMS_UTILS_TLIMAPPING_LEGACY_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct InjectTLIMappingsLegacyPassWrapper : public FunctionPass {
  InjectTLIMappingsLegacyPassWrapper();
  static char ID;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override { return "LegacyWrappedInjectTLIMappings"; }

private:
  FunctionAnalysisManager FAM;
  PassBuilder PB;
};

FunctionPass *createLegacyWrappedInjectTLIMappingsPass();
} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_UTILS_TLIMAPPING_LEGACY_H