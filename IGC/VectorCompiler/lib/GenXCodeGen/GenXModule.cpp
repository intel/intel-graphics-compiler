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

#include "vc/Utils/GenX/ImplicitArgsBuffer.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/DebugInfo.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/GenXIntrinsics/GenXMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Cloning.h>

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

static bool isImplicitArgsBufferUsed(const Module &M) {
  return llvm::any_of(vc::kernels(M), [](const Function &F) {
    return F.hasFnAttribute(vc::ImplicitArgs::KernelAttr);
  });
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
  const auto &TM = getAnalysis<TargetPassConfig>().getTM<GenXTargetMachine>();

  ST = &TM.getGenXSubtarget();
  Ctx = &M.getContext();
  BC = &getAnalysis<GenXBackendConfig>();

  InlineAsm = CheckForInlineAsm(M);

  DisableFinalizerOpts = TM.getOptLevel() == CodeGenOpt::Level::None;
  EmitDebugInformation =
      BC->emitDWARFDebugInfo() && vc::DIBuilder::checkIfModuleHasDebugInfo(M);
  ImplicitArgsBufferIsUsed = isImplicitArgsBufferUsed(M);

  bool ModuleModified = FGA->legalizeGroups();
  FGA->buildGroups();
  return ModuleModified;
}

using MappingT = genx::di::VisaMapping::Mapping;

void GenXModule::updateVisaMapping(const Function *K, const Instruction *Inst,
                                   unsigned VisaIndex, StringRef Reason) {
  // NOTE: K stands for "kernel function". That is the function that is
  // for the currently generated vISA object
  IGC_ASSERT(K);

  auto PrevCounter = VisaCounter[K];
  VisaCounter[K] = VisaIndex;
  if (!Inst) {
    LLVM_DEBUG(dbgs() << "visaCounter update <" << K->getName() << ">:{"
                      << PrevCounter << "->" << VisaIndex
                      << "}, src: " << Reason << "\n");
    return;
  }

  const auto *F = Inst->getFunction();
  LLVM_DEBUG(dbgs() << "visaCounter update <" << K->getName() << "/"
                    << ((F == K) ? StringRef(".") : F->getName()) << ">: {"
                    << PrevCounter << "->" << VisaIndex << "}, inst: " << *Inst
                    << "\n");

  // Unfortunately, our CISA builder routines are not very consistent with
  // respect to the interfaces used to emit vISA.
  // There may be situations when the debug information for instruction is
  // updated several times (like when we emit an additional VISALifeTime
  // instruction)
  // This check is a workaround for a problem when we may emit an auxiliary
  // visa instruction using the interface which requires us to update the
  // "current instruction" without actually doing so.
  auto &Map = VisaMapping[F];
  const Instruction *LastInst =
      Map.V2I.empty() ? nullptr : Map.V2I.rbegin()->Inst;
  if (LastInst != Inst) {
    bool IsDbgInst = isa<DbgInfoIntrinsic>(Inst);
    Map.V2I.emplace_back(MappingT{VisaIndex, Inst, 0 /*Count*/, IsDbgInst});
    LLVM_DEBUG(dbgs() << "Added :" << VisaIndex << ": " << *Inst << " \n");
  }
  if (!Map.V2I.empty())
    LLVM_DEBUG(dbgs() << "Last element <"
                      << " id =" << Map.V2I.rbegin()->VisaIdx
                      << " count =" << Map.V2I.rbegin()->VisaCount
                      << " inst: " << *(Map.V2I.rbegin()->Inst) << "\n");
}

void GenXModule::updateVisaCountMapping(const Function *K,
                                        const Instruction *Inst,
                                        unsigned VisaIndex, StringRef Reason) {
  IGC_ASSERT(K);
  IGC_ASSERT(Inst);
  auto PrevCounter = VisaCounter[K];
  VisaCounter[K] = VisaIndex;
  const auto *F = Inst->getFunction();
  auto &Map = VisaMapping[F];
  if (Map.V2I.empty())
    return;

  // Update instruction size or remove instruction mapping, if there is
  // no visa instruction found for it.
  auto LastElement = Map.V2I.rbegin();
  // Do not fill mapping for instructions with empty elements
  // update counter does not called for debug-instructions
  if (LastElement->VisaIdx == VisaIndex && LastElement->VisaCount == 0 &&
      LastElement->Inst == Inst) {
    LLVM_DEBUG(dbgs() << "Remove empty mapping for ");
    LLVM_DEBUG(LastElement->Inst->dump());
    Map.V2I.pop_back();
  } else if (!LastElement->IsDbgInst) {
    IGC_ASSERT(VisaIndex - PrevCounter >= 0);
    LastElement->VisaCount = VisaIndex - LastElement->VisaIdx;
    LLVM_DEBUG(dbgs() << "visaCounter update Map <" << K->getName() << "/"
                      << ((F == K) ? StringRef(".") : F->getName())
                      << "> for :{" << PrevCounter << " count = "
                      << LastElement->VisaCount << " in_inst =" << *Inst
                      << "}, src: End " << Reason << "\n");
  } else {
    IGC_ASSERT(!isa<DbgInfoIntrinsic>(Inst));
    LLVM_DEBUG(dbgs() << "remove unused instruction from Map <" << K->getName()
                      << "/" << ((F == K) ? StringRef(".") : F->getName())
                      << "> for :{" << PrevCounter << " -> " << VisaIndex
                      << "} inst =" << *Inst << ", src: Lost " << Reason
                      << "\n");
    Map.V2I.emplace_back(MappingT{PrevCounter, Inst, VisaIndex - PrevCounter,
                                  false /*Not dbg inst!*/});
  }

  if (!Map.V2I.empty())
    LLVM_DEBUG(dbgs() << "Last element <" << Map.V2I.rbegin()->VisaIdx
                      << " id = "
                      << " inst: " << *(Map.V2I.rbegin()->Inst) << "\n");
}

const genx::di::VisaMapping *
GenXModule::getVisaMapping(const Function *F) const {
  IGC_ASSERT(VisaMapping.count(F));
  return &VisaMapping.at(F);
}

GenXModule::InfoForFinalizer GenXModule::getInfoForFinalizer() const {
  InfoForFinalizer Info;
  Info.EmitDebugInformation = EmitDebugInformation;
  Info.DisableFinalizerOpts = DisableFinalizerOpts;
  IGC_ASSERT_MESSAGE(
      ST,
      "GenXSubtarget must be defined to call GenXModule::getInfoForFinalizer");
  Info.EmitCrossThreadOffsetRelocation =
      ST->hasThreadPayloadInMemory() && ImplicitArgsBufferIsUsed;
  return Info;
}
