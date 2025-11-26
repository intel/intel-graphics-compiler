/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_SCCP_LEGACY_H
#define IGCLLVM_TRANSFORMS_IPO_SCCP_LEGACY_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct IPSCCPLegacyPassWrapper : public ModulePass {
  IPSCCPLegacyPassWrapper();
  static char ID;
  bool runOnModule(llvm::Module &M);
  void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  PassBuilder PB;
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
};

ModulePass *createLegacyWrappedIPSCCPPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_SCCP_LEGACY_H
