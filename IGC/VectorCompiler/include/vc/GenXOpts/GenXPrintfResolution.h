/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_PRINTF_RESOLUTION
#define GENX_OPTS_GENX_PRINTF_RESOLUTION

#include "llvm/Target/TargetMachine.h"

namespace llvm {
void initializeGenXPrintfResolutionPass(PassRegistry &);
}

struct GenXPrintfResolutionPass
    : public llvm::PassInfoMixin<GenXPrintfResolutionPass> {
  const llvm::TargetMachine *TM;
  explicit GenXPrintfResolutionPass(const llvm::TargetMachine *TM) : TM(TM){};
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_PRINTF_RESOLUTION
