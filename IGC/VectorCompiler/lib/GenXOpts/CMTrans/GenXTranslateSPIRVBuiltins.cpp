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
#include <llvm/Linker/Linker.h>
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
  // Collect SPIRV built-in functions to link.
  auto SPIRVBuiltins = vc::collectFunctionNamesIf(
      M, [](const Function &F) { return isSPIRVBuiltinDecl(F); });
  // Nothing to do if there are no spirv builtins.
  if (SPIRVBuiltins.empty())
    return false;

  std::unique_ptr<Module> SPIRVBuiltinsModule =
      getBiFModule(BiFKind::VCSPIRVBuiltins, M.getContext());
  SPIRVBuiltinsModule->setDataLayout(M.getDataLayout());
  SPIRVBuiltinsModule->setTargetTriple(M.getTargetTriple());

  if (Linker::linkModules(M, std::move(SPIRVBuiltinsModule),
                          Linker::Flags::LinkOnlyNeeded)) {
    IGC_ASSERT_MESSAGE(0, "Error linking spirv implementation builtin module");
  }

  // If declaration appeared, then mark with internal linkage.
  vc::internalizeImportedFunctions(M, SPIRVBuiltins,
                                   /* SetAlwaysInline */ true);

  return true;
}

std::unique_ptr<Module>
GenXTranslateSPIRVBuiltins::getBiFModule(BiFKind Kind, LLVMContext &Ctx) {
  MemoryBufferRef BiFModuleBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(Kind);
  return vc::getLazyBiFModuleOrReportError(BiFModuleBuffer, Ctx);
}
