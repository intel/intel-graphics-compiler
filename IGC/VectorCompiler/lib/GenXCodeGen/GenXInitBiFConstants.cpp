/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXInitBiFConstants
/// --------------------
///
/// This pass initializes some special global constants from BiFs with target
/// specific values.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>

using namespace llvm;

class GenXInitBiFConstants final : public ModulePass {
public:
  static char ID;

  GenXInitBiFConstants() : ModulePass(ID) {}

  StringRef getPassName() const override {
    return "GenX initialize BiF constants";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnModule(Module &M) override;

private:
  const GenXSubtarget *ST = nullptr;
  Module *M = nullptr;

  bool initGlobalConstant(StringRef Name, uint32_t Value);
};

char GenXInitBiFConstants::ID = 0;

namespace llvm {
void initializeGenXInitBiFConstantsPass(PassRegistry &);
} // end namespace llvm

INITIALIZE_PASS_BEGIN(GenXInitBiFConstants, "GenXInitBiFConstants",
                      "GenXInitBiFConstants", false, false)
INITIALIZE_PASS_END(GenXInitBiFConstants, "GenXInitBiFConstants",
                    "GenXInitBiFConstants", false, false)

ModulePass *llvm::createGenXInitBiFConstantsPass() {
  initializeGenXInitBiFConstantsPass(*PassRegistry::getPassRegistry());
  return new GenXInitBiFConstants;
}

void GenXInitBiFConstants::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
}

bool GenXInitBiFConstants::runOnModule(Module &Mod) {
  M = &Mod;
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  bool Result = false;

  Result |= initGlobalConstant("__cm_cl_MaxHWThreadIDPerSubDevice",
                               ST->getMaxThreadsNumPerSubDevice());

  return Result;
}

bool GenXInitBiFConstants::initGlobalConstant(StringRef Name, uint32_t Value) {
  IGC_ASSERT_EXIT(M);

  auto *Global = M->getGlobalVariable(Name);
  if (!Global)
    return false;

  Global->setInitializer(ConstantInt::get(Global->getValueType(), Value));
  Global->setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
  Global->setConstant(true);

  return true;
}
