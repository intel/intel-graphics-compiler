/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXClobberChecker
//===----------------------------------------------------------------------===//
//
// Read access to GENX_VOLATILE variable yields vload + a user(rdregion).
// During internal optimizations the user can be (baled in (and or) collapsed
// (and or) moved away) to a position in which it potentially gets affected by a
// store to the same GENX_VOLATILE variable. Such a situation must be avoided.
//
// This pass implements a checker/fixup (only available in debug build under
// -check-gv-clobbering=true option) introduced late in pipeline right
// before global volatile loads coalescing (NB1).
//
// This checker/fixup is used to diagnose the issue while separate optimization
// passes are being fixed. Current list of affected passes is the following:
//
// RegionCollapsing
// FuncBaling
// IMadLegalization
// FuncGroupBaling
// Depressurizer
// ...
//
// NB1: The "catch-all" check/fixup is based on assumption that in case of
// reference intended by the high level program backend never gets store
// potentially clobbering vload before user neither from frontend nor as the
// result of internal optimizations. Otherwize it would produce false-positives.
//
//-------------------------------
// Pseudocode example
//-------------------------------
// GENX_VOLATILE g = VALID_VALUE
// funN() {  g = INVALID_VALUE }
// fun1() {  funN()  }
// kernel () {
//     cpy = g  // Copy the value of g.
//     fun1()   // Either store down function call changes g
//     g = INVALID_VALUE // or store in the same function.
//     use(cpy) // cpy == VALID_VALUE; use should see the copied value,
//     // ... including complex control flow cases.
//   }
// }
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXUtil.h"

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/GlobalVariable.h"

#include <unordered_map>

#define DEBUG_TYPE "GENX_CLOBBER_CHECKER"

using namespace llvm;
using namespace genx;

static cl::opt<bool> CheckGVClobberingTryFixup(
    "check-gv-clobbering-try-fixup", cl::init(false), cl::Hidden,
    cl::desc("Try to fixup simple cases if clobbering detected."));

static cl::opt<bool> CheckGVClobberingCollectRelatedGVStoreCallSites(
    "check-gv-clobbering-collect-store-related-call-sites", cl::init(false),
    cl::Hidden,
    cl::desc("If not enabled, we shall assume that any user function call can "
             "potentially clobber the GV value."
             "With this option enabled make this more precise by collecting "
             "user function call sites that can result in clobbering "
             "and account only for those."));

namespace {

class GenXGVClobberChecker : public ModulePass,
                             public IDMixin<GenXGVClobberChecker> {
private:
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;

  bool checkGVClobberingByInterveningStore(Instruction *LI,
                                           llvm::SetVector<Instruction *> *SIs);

public:
  explicit GenXGVClobberChecker() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX GV clobber checker/fixup";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addUsedIfAvailable<GenXLiveness>();
    AU.addUsedIfAvailable<GenXGroupBaling>();
    if (!CheckGVClobberingTryFixup)
      AU.setPreservesAll();
  }
  bool runOnModule(Module &) override;
};
} // namespace

namespace llvm {
void initializeGenXGVClobberCheckerPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXGVClobberChecker, "GenXGVClobberChecker",
                      "GenX global volatile clobbering checker", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_END(GenXGVClobberChecker, "GenXGVClobberChecker",
                    "GenX global volatile clobbering checker", false, false)

ModulePass *llvm::createGenXGVClobberCheckerPass() {
  initializeGenXGVClobberCheckerPass(*PassRegistry::getPassRegistry());
  return new GenXGVClobberChecker();
}

bool GenXGVClobberChecker::checkGVClobberingByInterveningStore(
    Instruction *LI, llvm::SetVector<Instruction *> *SIs) {
  bool Changed = false;
  for (auto *UI_ : LI->users()) {
    auto *UI = dyn_cast<Instruction>(UI_);
    if (!UI)
      continue;

    const StringRef DiagPrefix = "potential clobbering detected:";

    if (auto *SI =
            genx::getInterveningVStoreOrNull(LI, UI, true, nullptr, SIs)) {
      vc::diagnose(LI->getContext(), DiagPrefix,
                   "found a vstore intervening before value usage ", DS_Warning,
                   vc::WarningName::Generic, UI);
      vc::diagnose(LI->getContext(), "...", "intervening vstore", DS_Warning,
                   vc::WarningName::Generic, SI);
      LLVM_DEBUG(dbgs() << __FUNCTION__ << ": Found intervening vstore: ";
                 SI->print(dbgs());
                 dbgs() << "\n"
                        << __FUNCTION__ << ": Affected vload: ";
                 LI->print(dbgs()); dbgs() << "\n"
                                           << __FUNCTION__ << ": User: ";
                 UI->print(dbgs()); dbgs() << "\n";);
      if (CheckGVClobberingTryFixup) {
        if (!Baling || !Liveness)
          vc::diagnose(LI->getContext(), DiagPrefix,
                       "Either Baling or Liveness analysis results are not "
                       "available",
                       DS_Warning, vc::WarningName::Generic, UI);

        if (GenXIntrinsic::isRdRegion(UI) &&
            isa<Constant>(
                UI->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum))) {
          UI->moveAfter(LI);
          if (Baling && Baling->isBaled(UI))
            Baling->unbale(UI);
          if (Liveness) {
            if (Liveness->getLiveRangeOrNull(UI))
              Liveness->removeValue(UI);
            auto *LR = Liveness->getOrCreateLiveRange(UI);
            LR->setCategory(Liveness->getLiveRangeOrNull(LI)->getCategory());
            LR->setLogAlignment(
                Liveness->getLiveRangeOrNull(LI)->getLogAlignment());
          }
          Changed |= true;
        } else {
          vc::diagnose(
              LI->getContext(), DiagPrefix,
              "fixup is only possible for rdregion with constant "
              "offsets as it has single input from vload and "
              "can be easily moved back to it, however current case is "
              "more complex.",
              DS_Warning, vc::WarningName::Generic, UI);
        }
      }
    }
  }
  return Changed;
};

bool GenXGVClobberChecker::runOnModule(Module &M) {
  bool Changed = false;
  Baling = getAnalysisIfAvailable<GenXGroupBaling>();
  Liveness = getAnalysisIfAvailable<GenXLiveness>();

  llvm::SetVector<Instruction *> Loads;
  std::unordered_map<
      Value *, std::unordered_map<Function *, llvm::SetVector<Instruction *>>>
      ClobberingInsns{};

  for (auto &F : M.functions()) {
    for (auto &BB : F)
      for (auto &I : BB)
        if (genx::isAGVLoad(&I))
          Loads.insert(&I);
        else if (CheckGVClobberingCollectRelatedGVStoreCallSites &&
                 genx::isAGVStore(&I))
          genx::collectRelatedCallSitesPerFunction(
              &I, nullptr,
              ClobberingInsns[genx::getBitCastedValue(I.getOperand(1))]);
  }

  for (const auto &LI : Loads)
    Changed |= checkGVClobberingByInterveningStore(
        LI, CheckGVClobberingCollectRelatedGVStoreCallSites
                ? &ClobberingInsns[genx::getBitCastedValue(LI->getOperand(0))]
                                  [LI->getFunction()]
                : nullptr);

  return Changed;
}
