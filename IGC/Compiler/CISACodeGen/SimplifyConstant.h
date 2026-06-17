/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

void initializeSimplifyConstantLPMPass(llvm::PassRegistry &);
llvm::ModulePass *createSimplifyConstantPass();

void initializePromoteConstantLPMPass(llvm::PassRegistry &);
llvm::FunctionPass *createPromoteConstantPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrappers. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class SimplifyConstantNPM : public llvm::PassInfoMixin<SimplifyConstantNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "SimplifyConstant"; }
  static bool isRequired() { return true; }
};

class PromoteConstantNPM : public llvm::PassInfoMixin<PromoteConstantNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "PromoteConstant"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
