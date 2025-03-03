/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// GenXCloneIndirectFunctions
/// --------------------------
///
/// VISA has a restriction that a function can be called either directly or
/// indirectly but a combination of these two methods are not permitted for a
/// single function. This restriction rises two problems that this pass solves:
///
/// 1. We do not know all call instructions of an extern function until final
/// linkage happens. Hence all the unknown call of the function is thought to be
/// always indirect. But the function's module can contain direct calls to it
/// that's why we create a clone function:
///   * Original function linkage type is set to external as it is expected to
///   be called indirectly;
///   * A function with external linkage type and the name of the original
///   function with "direct" suffix is created. This is a copy of the original
///   function.
/// Thus, direct calls and indirect ones are separated and all unknown possible
/// calls from other modules will original function without any need to change
/// something on their side.
///
///   Before:
///
///     direct calls   --> |
///                        | --> external foo()
///     indirect calls --> |
///
///   After:
///                       clone function
///                           |
///                           v
///     direct calls   --> internal foo_direct()
///     indirect calls --> external foo()
///                           ^
///                           |
///                    original function
///
/// 2. An internal function can also be called directly and indirectly inside a
/// module. The idea is the same: an internal clone function is created,
/// and the calls are separated.
///
//===----------------------------------------------------------------------===//

#include "llvmWrapper/IR/Value.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include <llvm/GenXIntrinsics/GenXIntrinsics.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;
#define DEBUG_TYPE "GenXCloneIndirectFunctions"

static cl::opt<bool> EnableCloneIndirectFunctions(
    "vc-enable-clone-indirect-functions",
    llvm::cl::desc("Enable/disable GenXCloneIndirectFunctions"), cl::init(true),
    cl::Hidden);

namespace {

class GenXCloneIndirectFunctions
    : public ModulePass,
      public InstVisitor<GenXCloneIndirectFunctions> {
  std::vector<std::pair<Function *, bool>> IndirectFuncs;
#if LLVM_VERSION_MAJOR >= 16
  GenXBackendConfigPass::Result &BECfg;
#endif
public:
  static char ID;
#if LLVM_VERSION_MAJOR >= 16
  GenXCloneIndirectFunctions(GenXBackendConfigPass::Result &BC)
      : BECfg(BC), ModulePass(ID) {
#else  // LLVM_VERSION_MAJOR >= 16
  GenXCloneIndirectFunctions() : ModulePass(ID) {
#endif // LLVM_VERSION_MAJOR >= 16
    initializeGenXCloneIndirectFunctionsPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
  }

  StringRef getPassName() const override {
    return "GenXCloneIndirectFunctions";
  }

  bool runOnModule(Module &M) override;

  void visitFunction(Function &F);
};

} // namespace

void GenXCloneIndirectFunctions::visitFunction(Function &F) {
  if (GenXIntrinsic::isAnyNonTrivialIntrinsic(&F))
    return;
  if (vc::InternalIntrinsic::isInternalNonTrivialIntrinsic(&F))
    return;
  if (vc::isBuiltinFunction(F))
    return;
  if (vc::isKernel(&F))
    return;
  if (vc::isCMCallable(F))
    return;

  if (!F.hasLocalLinkage()) {
    IndirectFuncs.emplace_back(&F, true);
    return;
  }

  IGC_ASSERT_MESSAGE(!F.isDeclaration(), "Declaration with local linkage?");
  if (F.hasAddressTaken())
    IndirectFuncs.emplace_back(&F, false);
}

static void cloneIndirectFunction(Function &F,
                                  GlobalValue::LinkageTypes IndirectLinkage) {
  // Clone the function F for direct calls
  ValueToValueMapTy VMap;
  auto *Direct = CloneFunction(&F, VMap);
  Direct->setName(F.getName() + "_direct");
  Direct->setLinkage(GlobalValue::InternalLinkage);

  // Replace all uses of the original function that are direct calls.
  IGCLLVM::replaceUsesWithIf(&F, Direct, [&F](Use &U) {
    auto *CI = dyn_cast<CallInst>(U.getUser());
    return CI && CI->getCalledFunction() == &F;
  });

  // Original function is an indirect stack call
  if (!vc::requiresStackCall(&F))
    F.addFnAttr(genx::FunctionMD::CMStackCall);
}

bool GenXCloneIndirectFunctions::runOnModule(Module &M) {
  if (!EnableCloneIndirectFunctions)
    return false;

#if LLVM_VERSION_MAJOR < 16
  auto &&BECfg = getAnalysis<GenXBackendConfig>();
#endif
  IGC_ASSERT_MESSAGE(
      llvm::none_of(M.functions(),
                    [&](const Function &F) {
                      return F.hasAddressTaken() &&
                             BECfg.directCallsOnly(F.getName());
                    }),
      "A function has address taken inside the module that contradicts "
      "DirectCallsOnly option");

  // If direct calls are forced for all functions.
  if (BECfg.directCallsOnly()) {
    return false;
  }

  visit(M);

  bool Modified = false;

  for (auto &[F, IsExternal] : IndirectFuncs) {
    if (BECfg.directCallsOnly(F->getName()))
      continue;

    auto CheckDirectCall = [Func = F](User *U) {
      auto *CI = dyn_cast<CallInst>(U);
      return CI && CI->getCalledFunction() == Func;
    };

    if (F->isDeclaration()) {
      IGC_ASSERT_MESSAGE(IsExternal,
                         "Internal-linkage function cannot be a declaration");
      if (!vc::requiresStackCall(F)) {
        F->addFnAttr(genx::FunctionMD::CMStackCall);
        Modified = true;
      }
      IGC_ASSERT_MESSAGE(vc::isIndirect(F) || F->hasExternalLinkage(),
                         "Must be indirect");
    } else if (llvm::any_of(F->users(), CheckDirectCall)) {
      cloneIndirectFunction(*F, IsExternal ? GlobalValue::ExternalLinkage
                                           : GlobalValue::InternalLinkage);
      Modified = true;
    } else {
      // If the function is not called directly, there is no need to clone
      if (!vc::requiresStackCall(F)) {
        F->addFnAttr(genx::FunctionMD::CMStackCall);
        Modified = true;
      }
    }

    IGC_ASSERT_MESSAGE(vc::isIndirect(F), "Must be indirect");
  }

  IndirectFuncs.clear();
  return Modified;
}

char GenXCloneIndirectFunctions::ID = 0;
INITIALIZE_PASS_BEGIN(GenXCloneIndirectFunctions, "GenXCloneIndirectFunctions",
                      "GenXCloneIndirectFunctions", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXCloneIndirectFunctions, "GenXCloneIndirectFunctions",
                    "GenXCloneIndirectFunctions", false, false)

#if LLVM_VERSION_MAJOR < 16
namespace llvm {
ModulePass *createGenXCloneIndirectFunctionsPass() {
  return new GenXCloneIndirectFunctions();
}
} // namespace llvm
#else
PreservedAnalyses
GenXCloneIndirectFunctionsPass::run(llvm::Module &M,
                                    llvm::AnalysisManager<llvm::Module> &AM) {
  GenXCloneIndirectFunctions GenXClone(BC);
  if (GenXClone.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif
