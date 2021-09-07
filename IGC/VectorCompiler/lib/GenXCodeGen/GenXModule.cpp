/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenXModule is a module pass whose purpose is to store information
// about the GenX module being written, such as the built kernels and functions.
// See the comment in GenXModule.h.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_MODULE"

#include "GenXModule.h"

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/GenXOpts/Utils/KernelInfo.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <set>
#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_MODULE"

using namespace llvm;

char GenXModule::ID = 0;
INITIALIZE_PASS_BEGIN(GenXModule, "GenXModule", "GenXModule", false,
                      true /*analysis*/)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXModule, "GenXModule", "GenXModule", false,
                    true /*analysis*/)

ModulePass *llvm::createGenXModulePass() {
  initializeGenXModulePass(*PassRegistry::getPassRegistry());
  return new GenXModule;
}

void GenXModule::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesAll();
}

bool GenXModule::CheckForInlineAsm(Module &M) const {
  for (auto &F : M)
    for (auto &BB : F)
      for (auto &I : BB) {
        CallInst *CI = dyn_cast<CallInst>(&I);
        if (CI && CI->isInlineAsm())
          return true;
      }
  return false;
}

/***********************************************************************
 * runOnModule : run GenXModule analysis
 *
 * This populates FunctionGroupAnalysis such that each FunctionGroup
 * corresponds to a GenX kernel/function and its subroutines. If any
 * subroutine would be used in more than one FunctionGroup, it is
 * cloned.
 *
 * The FunctionGroup is populated in an order such that a function appears
 * after all its callers.
 */
bool GenXModule::runOnModule(Module &M) {
  auto FGA = &getAnalysis<FunctionGroupAnalysis>();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  Ctx = &M.getContext();
  BC = &getAnalysis<GenXBackendConfig>();

  InlineAsm = CheckForInlineAsm(M);

  // Iterate, processing each Function that is not yet assigned to a
  // FunctionGroup.
  bool ModuleModified = false;

  // build callgraph and process subgroups
  std::map<Function *, std::list<Function *>> CG;
  // TODO: for now it's a temporary workaround of strange ArgIndirection
  // problems that it depends on order of functions withing a group
  // This should be removed once indirection is fixed
  std::map<Function *, std::set<Function*>> Visited;

  for (auto &F : M) {
    for (auto *U : F.users()) {
      auto *Inst = dyn_cast<Instruction>(U);
      if (!Inst) {
        continue;
      }
      if (!F.empty() && Visited[Inst->getFunction()].count(&F) == 0) {
        CG[Inst->getFunction()].push_back(&F);
        Visited[Inst->getFunction()].insert(&F);
      }
      // recursive funcs must use stack
      if (Inst->getFunction() == &F) {
        const bool UsesStack = genx::requiresStackCall(&F);
        IGC_ASSERT_MESSAGE(
            UsesStack,
            "Found recursive function without CMStackCall attribute");
        (void)UsesStack;
      }
    }
  }

  for (auto T : FGA->TypesToProcess) {
    for (auto &F : M) {
      if (F.isDeclaration())
        continue;
      if (!genx::fg::isHead(F))
        continue;
      // Do not process kernels at subgroup level.
      if (genx::fg::isGroupHead(F) &&
          T == FunctionGroupAnalysis::FGType::SUBGROUP)
        continue;
      // Do not process stack calls at group level.
      if (genx::fg::isSubGroupHead(F) &&
          T == FunctionGroupAnalysis::FGType::GROUP)
        continue;
      // TODO: it seems OK not to update CG each time a function was cloned. But
      // it must be investigated deeper.
      ModuleModified |= FGA->buildGroup(CG, &F, nullptr, T);
    }
  }

  for (auto SubFG : FGA->subgroups()) {
    const Function *Head = SubFG->getHead();
    IGC_ASSERT(Head);

    for (auto U : Head->users()) {
      const auto *UserInst = dyn_cast<Instruction>(U);
      if (!UserInst)
        continue;

      const Function *UserFunction = UserInst->getFunction();
      IGC_ASSERT(UserFunction);
      FunctionGroup *UserFG = FGA->getAnyGroup(UserFunction);
      IGC_ASSERT(UserFG);

      UserFG->addSubgroup(SubFG);
    }
  }

  return ModuleModified;
}
void GenXModule::updateVisaMapping(const Function *K, const Instruction *Inst,
                                   unsigned VisaIndex, StringRef Reason) {
  // NOTE: K stands for "kernel function". That is the function that is
  // for the currently generated vISA object
  IGC_ASSERT(K);

  auto &CurrentCounter = VisaCounter[K];
  auto PrevCounter = CurrentCounter;
  CurrentCounter = VisaIndex;
  if (!Inst) {
    LLVM_DEBUG(dbgs() << "visaCounter update <" << K->getName() << ">:{"
                      << PrevCounter << "->" << CurrentCounter
                      << "}, src: " << Reason << "\n");
    return;
  }

  const auto *F = Inst->getFunction();
  LLVM_DEBUG(dbgs() << "visaCounter update <" << K->getName() << "/"
                    << ((F == K) ? StringRef(".") : F->getName()) << ">: {"
                    << PrevCounter << "->" << CurrentCounter
                    << "}, inst: " << *Inst << "\n");

  // Unfortunately, our CISA builder routines are not very consistent with
  // respect to the interfaces used to emit vISA.
  // There may be situations when the debug information for instruction is
  // updated several times (like when we emit an additional VISALifeTime
  // instruction)
  // This check is a workaround for a problem when we may emit an auxiliary
  // visa instruction using the interface which requires us to update the
  // "current instruction" without actually doing so.
  auto &Mapping = VisaMapping[F];
  const Instruction *LastInst =
      Mapping.V2I.empty() ? nullptr : Mapping.V2I.rbegin()->Inst;
  using MappingT = genx::di::VisaMapping::Mapping;
  if (LastInst != Inst)
    Mapping.V2I.emplace_back(MappingT{CurrentCounter, Inst});
}

const genx::di::VisaMapping *
GenXModule::getVisaMapping(const Function *F) const {
  IGC_ASSERT(VisaMapping.count(F));
  return &VisaMapping.at(F);
}
