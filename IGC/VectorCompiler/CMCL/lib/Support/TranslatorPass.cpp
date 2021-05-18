/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/TranslatorPass.h"
#include "cmcl/Support/BuiltinTranslator.h"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

using namespace llvm;
using namespace cmcl;

class CMCLTranslator : public ModulePass {
public:
  static char ID;
  CMCLTranslator() : ModulePass(ID) {}
  StringRef getPassName() const override { return "CM-CL translator pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

char CMCLTranslator::ID = 0;

INITIALIZE_PASS_BEGIN(CMCLTranslator, "CMCLTranslator", "CMCLTranslator", false,
                      false)
INITIALIZE_PASS_END(CMCLTranslator, "CMCLTranslator", "CMCLTranslator", false,
                    false)

ModulePass *llvm::createCMCLTranslatorPass() {
  initializeCMCLTranslatorPass(*PassRegistry::getPassRegistry());
  return new CMCLTranslator;
};

void CMCLTranslator::getAnalysisUsage(AnalysisUsage &AU) const {}

bool CMCLTranslator::runOnModule(Module &M) { return translateBuiltins(M); }
