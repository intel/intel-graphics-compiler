/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_GLOBALDCE_LEGACY_H
#define IGCLLVM_TRANSFORMS_IPO_GLOBALDCE_LEGACY_H

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace IGCLLVM {

struct GlobalDCELegacyPassWrapper : public ModulePass {
  GlobalDCELegacyPassWrapper();
  static char ID;
  bool runOnModule(llvm::Module &M);

private:
  PassBuilder PB;
  ModuleAnalysisManager MAM;
};

ModulePass *createLegacyWrappedGlobalDCEPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_IPO_GLOBALDCE_LEGACY_H
