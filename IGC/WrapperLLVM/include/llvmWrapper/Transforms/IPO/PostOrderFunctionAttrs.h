/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_LEGACY_POSTORDERFUNCTIONATTRS_H
#define IGCLLVM_TRANSFORMS_IPO_LEGACY_POSTORDERFUNCTIONATTRS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {

struct PostOrderFunctionAttrsLegacyPassWrapper : public ModulePass {
  explicit PostOrderFunctionAttrsLegacyPassWrapper();
  static char ID;

  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedPostOrderFunctionAttrs"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

llvm::Pass *createLegacyWrappedPostOrderFunctionAttrsPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_LEGACY_POSTORDERFUNCTIONATTRS_H
