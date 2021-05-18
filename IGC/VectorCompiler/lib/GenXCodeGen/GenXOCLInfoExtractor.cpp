/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include "llvm/Pass.h"
#include "Probe/Assertion.h"

using namespace llvm;

namespace llvm {
void initializeGenXOCLInfoExtractorPass(PassRegistry &PR);
}

class GenXOCLInfoExtractor : public ModulePass {
public:
  static char ID;

private:
  GenXOCLRuntimeInfo::CompiledModuleT *Dest = nullptr;

public:
  StringRef getPassName() const override { return "GenX OCL Info Extractor"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXOCLRuntimeInfo>();
  }

  GenXOCLInfoExtractor() : ModulePass(ID) {}

  GenXOCLInfoExtractor(GenXOCLRuntimeInfo::CompiledModuleT &Dst)
      : ModulePass(ID), Dest(&Dst) {
    initializeGenXOCLInfoExtractorPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    IGC_ASSERT_MESSAGE(Dest, "Expected dest to be initialized");
    auto &Info = getAnalysis<GenXOCLRuntimeInfo>();
    *Dest = Info.stealCompiledModule();
    return false;
  }
};

char GenXOCLInfoExtractor::ID = 0;

INITIALIZE_PASS_BEGIN(GenXOCLInfoExtractor, "GenXOCLInfoExtractor",
                      "GenXOCLInfoExtractor", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXOCLRuntimeInfo)
INITIALIZE_PASS_END(GenXOCLInfoExtractor, "GenXOCLInfoExtractor",
                    "GenXOCLInfoExtractor", false, false)

ModulePass *llvm::createGenXOCLInfoExtractorPass(
    GenXOCLRuntimeInfo::CompiledModuleT &Dest) {
  return new GenXOCLInfoExtractor(Dest);
}
