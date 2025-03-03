/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// GenXTrampolineInsertion
/// -----------------------
///
/// VISA has a restriction that a function can be called either directly or
/// indirectly but a combination of these two methods are not permitted for a
/// single function. This restriction rises two problems that this pass solves:
///
/// 1. We do not know all call instructions of an extern function until final
/// linkage happens. Hence all the unknown call of the function is thought to be
/// always indirect. But the function's module can contain direct calls to it
/// that's why we create a trampoline function:
///   * Original function linkage type is set to internal as it is expected to
///   be called directly and only from its module;
///   * Suffix "direct" is appended to the original function name and all direct
///   call instructions are changed to call the function by its new name;
///   * A function with external linkage type and the old name of the original
///   function is created. This is a trampoline function and all it does is call
///   directly the original function. Thus, direct calls and indirect ones are
///   separated and all unknown possible calls from other modules will call
///   trampoline function without any need to change something on their side.
///
///   Before:
///
///     direct calls   --> |
///                        | --> external foo()
///     indirect calls --> |
///
///   After:
///
///     direct calls   -->                    |
///                                           | --> internal foo_direct()
///     indirect calls --> external foo() --> |
///                           ^
///                           |
///                    trampoline function
///
/// 2. An internal function can also be called directly and indirectly inside a
/// module. The idea is the same: an internal trampoline function is created,
/// and the calls are separated.
///
//===----------------------------------------------------------------------===//

#include "llvmWrapper/IR/Value.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include <llvm/GenXIntrinsics/GenXIntrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

using namespace llvm;

static cl::opt<bool> EnableTrampolineInsertion(
    "vc-enable-trampoline-insertion",
    llvm::cl::desc("Enable/disable GenXTrampolineInsertion"), cl::init(false),
    cl::Hidden);

namespace {

class GenXTrampolineInsertion : public ModulePass,
                                public InstVisitor<GenXTrampolineInsertion> {
  std::vector<Function *> ExternalFuncs;
  std::vector<Function *> InternalIndirectFuncs;

#if LLVM_VERSION_MAJOR >= 16
  GenXBackendConfigPass::Result &BECfg;
#endif

public:
  static char ID;
#if LLVM_VERSION_MAJOR < 16
  GenXTrampolineInsertion() : ModulePass(ID) {
#else
  GenXTrampolineInsertion(GenXBackendConfigPass::Result &BC)
      : BECfg(BC), ModulePass(ID) {
#endif
    initializeGenXTrampolineInsertionPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
  }

  StringRef getPassName() const override { return "GenXTrampolineInsertion"; }

  bool runOnModule(Module &M) override;

  void visitFunction(Function &F);
};

} // namespace

void GenXTrampolineInsertion::visitFunction(Function &F) {
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
    ExternalFuncs.push_back(&F);
    return;
  }

  IGC_ASSERT_MESSAGE(!F.isDeclaration(), "Declaration with local linkage?");
  if (F.hasAddressTaken())
    InternalIndirectFuncs.push_back(&F);
}

// Create a call from the trampoline to the original function.
static void createCallToOrigFunc(Function &Trampoline, Function &Orig,
                                 IRBuilder<> &IRB) {
  BasicBlock *Entry =
      BasicBlock::Create(IRB.getContext(), "entry", &Trampoline);
  IRB.SetInsertPoint(Entry);
  SmallVector<Value *, 8> Args;
  std::transform(Trampoline.arg_begin(), Trampoline.arg_end(),
                 std::back_inserter(Args), [](Argument &Arg) { return &Arg; });
  CallInst *CallToOrig = IRB.CreateCall(Orig.getFunctionType(), &Orig, Args);
  CallToOrig->setCallingConv(Orig.getCallingConv());

  if (CallToOrig->getType()->isVoidTy())
    IRB.CreateRetVoid();
  else
    IRB.CreateRet(CallToOrig);
}

static void createTrampoline(Function &F,
                             GlobalValue::LinkageTypes TrampolineLinkage,
                             IRBuilder<> &IRB) {
  std::string Name = F.getName().str();
  F.setName(Name + "_direct");
  // Direct function is always internal
  F.setLinkage(GlobalValue::InternalLinkage);

  auto *Trampoline = Function::Create(F.getFunctionType(), TrampolineLinkage,
                                      Name, F.getParent());
  Trampoline->copyAttributesFrom(&F);
  // Trampoline function is an indirect stack call regardless of what type of
  // transformation we are doing.
  if (!vc::requiresStackCall(Trampoline))
    Trampoline->addFnAttr(genx::FunctionMD::CMStackCall);

  createCallToOrigFunc(*Trampoline, F, IRB);

  // Replace all uses of the original function that are not direct calls.
  IGCLLVM::replaceUsesWithIf(&F, Trampoline, [&F](Use &U) {
    auto *CI = dyn_cast<CallInst>(U.getUser());
    return !CI || CI->getCalledFunction() != &F;
  });

  IGC_ASSERT_MESSAGE(vc::isIndirect(Trampoline) ||
                         Trampoline->hasExternalLinkage(),
                     "Trampoline function must be indirect");
  IGC_ASSERT_MESSAGE(!vc::isIndirect(F), "Must be direct");
}

bool GenXTrampolineInsertion::runOnModule(Module &M) {
  if (!EnableTrampolineInsertion)
    return false;

  bool Modified = false;
#if LLVM_VERSION_MAJOR < 16
  auto &&BECfg = getAnalysis<GenXBackendConfig>();
#endif
  IGC_ASSERT_MESSAGE(
      llvm::none_of(M.functions(),
        [&](const Function& F) { return F.hasAddressTaken() && BECfg.directCallsOnly(F.getName()); }),
      "A function has address taken inside the module that contradicts "
      "DirectCallsOnly option");

  // If direct calls are forced for all functions.
  if (BECfg.directCallsOnly()) {
    return false;
  }

  visit(M);

  IRBuilder<> IRB(M.getContext());

  for (auto *F : ExternalFuncs) {
    if (BECfg.directCallsOnly(F->getName()))
      continue;
    if (F->isDeclaration()) {
      if (!vc::requiresStackCall(F))
        F->addFnAttr(genx::FunctionMD::CMStackCall);
      IGC_ASSERT_MESSAGE(vc::isIndirect(F) || F->hasExternalLinkage(),
                         "Must be indirect");
      continue;
    }
    createTrampoline(*F, GlobalValue::ExternalLinkage, IRB);
    Modified = true;
  }

  for (auto *F : InternalIndirectFuncs) {
    auto CheckDirectCall = [F](User *U) {
      auto *CI = dyn_cast<CallInst>(U);
      return CI && CI->getCalledFunction() == F;
    };
    if (llvm::none_of(F->users(), CheckDirectCall)) {
      // If the function is not called directly, there is no need to create a
      // trampoline function.
      if (!vc::requiresStackCall(F))
        F->addFnAttr(genx::FunctionMD::CMStackCall);

      IGC_ASSERT_MESSAGE(vc::isIndirect(F), "Must be indirect");
      continue;
    }

    createTrampoline(*F, GlobalValue::InternalLinkage, IRB);
    Modified = true;
  }

  ExternalFuncs.clear();
  InternalIndirectFuncs.clear();
  return Modified;
}

char GenXTrampolineInsertion::ID = 0;
INITIALIZE_PASS_BEGIN(GenXTrampolineInsertion, "GenXTrampolineInsertion",
                      "GenXTrampolineInsertion", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXTrampolineInsertion, "GenXTrampolineInsertion",
                    "GenXTrampolineInsertion", false, false)

#if LLVM_VERSION_MAJOR < 16
namespace llvm {
ModulePass *createGenXTrampolineInsertionPass() {
  return new GenXTrampolineInsertion();
}
} // namespace llvm
#else
PreservedAnalyses
GenXTrampolineInsertionPass::run(Module &M, AnalysisManager<llvm::Module> &AM) {
  GenXTrampolineInsertion GenXTramp(BC);
  if (GenXTramp.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif
