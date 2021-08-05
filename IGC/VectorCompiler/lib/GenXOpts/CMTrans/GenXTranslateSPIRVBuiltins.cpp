/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXTranslateSPIRVBuiltins
/// -----------
///
/// This pass translates SPIR-V builtin functions by linking with BiF module.
/// BiF module holds implementation of those builtin functions.
///
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/General/BiF.h"

#include "Probe/Assertion.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

using namespace llvm;

class GenXTranslateSPIRVBuiltins final : public ModulePass {

public:
  static char ID;
  GenXTranslateSPIRVBuiltins() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX translate SPIR-V builtins";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;

private:
  std::unique_ptr<Module> getBiFModule(BiFKind Kind, LLVMContext &Ctx);
};

char GenXTranslateSPIRVBuiltins::ID = 0;

INITIALIZE_PASS_BEGIN(GenXTranslateSPIRVBuiltins, "GenXTranslateSPIRVBuiltins",
                      "GenXTranslateSPIRVBuiltins", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXTranslateSPIRVBuiltins, "GenXTranslateSPIRVBuiltins",
                    "GenXTranslateSPIRVBuiltins", false, false)

ModulePass *llvm::createGenXTranslateSPIRVBuiltinsPass() {
  initializeGenXTranslateSPIRVBuiltinsPass(*PassRegistry::getPassRegistry());
  return new GenXTranslateSPIRVBuiltins;
}

void GenXTranslateSPIRVBuiltins::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

// May have false positive, e.g. __spirv is in the middle of a function name.
// Having false positives is not that critical as they won't be linked anyway.
static bool isSPIRVBuiltinDecl(const Function &F) {
  if (!F.isDeclaration())
    return false;
  if (F.isIntrinsic() || GenXIntrinsic::isGenXIntrinsic(&F))
    return false;
  return F.getName().contains("__spirv");
}

bool GenXTranslateSPIRVBuiltins::runOnModule(Module &M) {
  // FIXME: It does nothing for now until BiF module is ready.
  return false;

  std::unique_ptr<Module> SPIRVBuiltinsModule =
      getBiFModule(BiFKind::VCSPIRVBuiltins, M.getContext());
  SPIRVBuiltinsModule->setDataLayout(M.getDataLayout());
  SPIRVBuiltinsModule->setTargetTriple(M.getTargetTriple());
  // TODO: Link builtins implementations, make them internal.
  return true;
}

std::unique_ptr<Module>
GenXTranslateSPIRVBuiltins::getBiFModule(BiFKind Kind, LLVMContext &Ctx) {
  MemoryBufferRef BiFModuleBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(Kind);
  return vc::getLazyBiFModuleOrReportError(BiFModuleBuffer, Ctx);
}
