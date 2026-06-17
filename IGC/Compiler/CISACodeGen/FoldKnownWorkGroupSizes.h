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
llvm::FunctionPass *CreateFoldKnownWorkGroupSizes();
void initializeFoldKnownWorkGroupSizesLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions
// (the seeded CodeGenContextAnalysis is module-level; IGC passes never use skipFunction). name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class FoldKnownWorkGroupSizesNPM : public llvm::PassInfoMixin<FoldKnownWorkGroupSizesNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-fold-workgroup-sizes"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
