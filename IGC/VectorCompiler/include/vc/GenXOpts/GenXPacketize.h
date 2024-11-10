/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_PACKETIZE
#define GENX_OPTS_GENX_PACKETIZE

namespace llvm {
void initializeGenXPacketizePass(PassRegistry &);
}

struct GenXPacketizePass : public llvm::PassInfoMixin<GenXPacketizePass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

#endif // GENX_OPTS_GENX_PACKETIZE
