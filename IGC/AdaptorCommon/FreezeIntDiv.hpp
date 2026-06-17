/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

////////////////////////////////////////////////////////////////////////////
// This pass stops propagation of poison values returned in case of integer
// division by zero. LLVM 10+ freeze instruction is used for that purpose.

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
llvm::FunctionPass *createFreezeIntDivPass();
void initializeFreezeIntDivLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. No analysis dependencies, so a plain function pass. name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class FreezeIntDivNPM : public llvm::PassInfoMixin<FreezeIntDivNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-freeze-int-div-pass"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
