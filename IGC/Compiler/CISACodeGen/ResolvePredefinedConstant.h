/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_RESOLVEPREDEFINEDCONSTANT_H_
#define _CISA_RESOLVEPREDEFINEDCONSTANT_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
llvm::ModulePass *createResolvePredefinedConstantPass();
void initializePredefinedConstantResolvingLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class PredefinedConstantResolvingNPM : public llvm::PassInfoMixin<PredefinedConstantResolvingNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-predefined-constant-resolve"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC

#endif // _CISA_RESOLVEPREDEFINEDCONSTANT_H_
