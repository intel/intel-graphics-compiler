/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENX_OPTS_GENX_LINKAGE_CORRUPTOR
#define GENX_OPTS_GENX_LINKAGE_CORRUPTOR

namespace llvm {
void initializeGenXLinkageCorruptorPass(PassRegistry &);
}

struct GenXLinkageCorruptorPass
    : public llvm::PassInfoMixin<GenXLinkageCorruptorPass> {

  explicit GenXLinkageCorruptorPass(GenXBackendConfigPass::Result &BC)
      : BC(BC){};

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

private:
  GenXBackendConfigPass::Result &BC;
};

#endif // GENX_OPTS_GENX_LINKAGE_CORRUPTOR
