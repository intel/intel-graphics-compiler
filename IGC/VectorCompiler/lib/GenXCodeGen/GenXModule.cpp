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

  for (auto T : FGA->TypesToProcess) {
    for (auto &F : M) {
      for (auto *U: F.users()) {
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
          const bool UsesStack = F.hasFnAttribute(genx::FunctionMD::CMStackCall);
          IGC_ASSERT_MESSAGE(UsesStack, "Found recursive function without CMStackCall attribute");
          (void) UsesStack;
        }
      }
    }

    for (auto &F : M) {
      if (F.empty() || F.getLinkage() == GlobalValue::InternalLinkage)
        continue;
      ModuleModified |= FGA->buildGroup(CG, &F, nullptr, T);
    }

    FGA->clearVisited();
    CG.clear();
    Visited.clear();
  }

  return ModuleModified;
}
void GenXModule::updateVisaMapping(const Function *F, const Instruction *Inst,
                                   unsigned VisaIndex, StringRef Reason) {
  auto &Mapping = VisaMapping[F];
  LLVM_DEBUG(dbgs() << "visaCounter update: {" << Mapping.visaCounter << "->"
                    << VisaIndex << "}, src: ");
  if (Inst)
    LLVM_DEBUG(dbgs() << *Inst << "\n");
  else
    LLVM_DEBUG(dbgs() << Reason << "\n");

  Mapping.visaCounter = VisaIndex;

  if (!Inst)
    return;

  // Unfortunately, our CISA builder routines are not very consistent with
  // respect to the interfaces used to emit vISA.
  // There may be situations when the debug information for instruction is
  // updated several times (like when we emit an additional VISALifeTime
  // instruction)
  // This check is a workaround for a problem when we may emit an auxiliary
  // visa instruction using the interface which requires us to update the
  // "current instruction" without actually doing so.
  const Instruction *LastInst =
      Mapping.V2I.empty() ? nullptr : Mapping.V2I.rbegin()->Inst;
  using MappingT = genx::di::VisaMapping::Mapping;
  if (LastInst != Inst)
    Mapping.V2I.emplace_back(MappingT{Mapping.visaCounter, Inst});
}

const genx::di::VisaMapping *
GenXModule::getVisaMapping(const Function *F) const {
  IGC_ASSERT(VisaMapping.count(F));
  return &VisaMapping.at(F);
}
