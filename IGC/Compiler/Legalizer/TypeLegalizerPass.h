/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_TYPELEGALIZERPASS_H
#define LEGALIZER_TYPELEGALIZERPASS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/IR/PassManager.h"
#include "common/LLVMWarningsPop.hpp"

void initializeTypeLegalizerPass(llvm::PassRegistry &);
llvm::FunctionPass *createTypeLegalizerPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. A plain function pass: the dominator tree is pulled
// from the function analysis manager and injected into the legacy engine. name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under NPM.
class TypeLegalizerNPM : public llvm::PassInfoMixin<TypeLegalizerNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-type-legalizer"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

#endif // LEGALIZER_TYPELEGALIZERPASS_H
