/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXClobberChecker
//===----------------------------------------------------------------------===//
//
// Read access to GENX_VOLATILE variable yields vload + a user(most of the time
// rdregion, but can be anything including vstore). During internal
// optimizations the user can be (baled in (and or) collapsed (and or) moved
// away) to a position in which it potentially gets affected by a store to the
// same GENX_VOLATILE variable. This situation must be avoided (ideally - only
// when the high-level program employed by-copy semantics, see below).
//
// This pass implements a checker/fixup (available under
// -check-gv-clobbering=true option, turned on by default in Debug build)
// introduced late in pipeline. It is used to identify situations when we have
// potentially clobbered the global volatile value.
//
// The checker warning about potential clobbering means that some optimization
// pass has overlooked the aspect of vload/vstore semantics and must be fixed to
// take it into account. Current list of affected passes:
//
// RegionCollapsing
// FuncBaling
// IMadLegalization
// FuncGroupBaling
// Depressurizer
// ...
//
//----------------------------------------------------------------
// TODO/IMPORTANT: presently there's no way to differentiate by-copy vs
// by-reference semantics, so we try to avoid moving vload users "after" vstores
// for all the cases, which results in less efficient code generation. The way
// to differentiate by-copy vs by-reference access must be implemented and
// optimizations restricted only for those use cases. By-reference accesses must
// be allowed for optimization as before to provide with most efficient code
// possible.
//----------------------------------------------------------------
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
//
// This pass can be used as a standalone tool (under an opt utility) to check
// the intermediate IR dumps acquired by the usage of -vc-dump-ir-split
// -vc-dump-ir-before-pass='*' -vc-dump-ir-after-pass='*' options and/or
// IGC_ShaderDumpEnable="1" and/or during an interactive debugging session.
//
// How to run the checker on individual IR dump (for individual options see
// options descriptions below in this file:
//
// {code}
//         opt \
//         -load <PATH_TO_libVCBackendPlugin.so> \
//         -enable-new-pm=0 \
//         -check-gv-clobbering=1 \
//         -check-gv-clobbering-try-fixup=0 \
//         -check-gv-clobbering-chk-with-bales=0 \
//         -check-gv-clobbering-standalone-mode=1 \
//         -check-gv-clobbering-abort-on-detection=0 \
//         -check-gv-clobbering-collect-kill-call-sites=0 \
//         -GenXGVClobberChecker \
//         -march=genx64 \
//         -mtriple=spir64-unknown-unknown \
//         -mcpu=Gen9 \
//         -disable-output \
//         -S \
//         <YOUR_LLVM_IR_DUMP.ll>
// {code}
//
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/Support/GenXDiagnostic.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/InitializePasses.h>
#include <llvm/Transforms/Utils/Local.h>

#include <unordered_map>

#define DEBUG_TYPE "GENX_CLOBBER_CHECKER"

using namespace llvm;
using namespace genx;

static cl::opt<bool> CheckGVClobbOpt_CollectKillCallSites(
    "check-gv-clobbering-collect-kill-call-sites", cl::init(false), cl::Hidden,
    cl::desc("With this option enabled make it more precise by collecting "
             "user function call sites that can result in clobbering of a "
             "particular global volatile value "
             "and account only for those when checking corresponding gvload. "
             "This reduces false positive probability for particular program "
             "text, but hides potential "
             "problems in optimization passes."));

static cl::opt<bool> CheckGVClobbOpt_StandaloneMode(
    "check-gv-clobbering-standalone-mode", cl::init(false), cl::Hidden,
    cl::desc(
        "For use out of pipeline as a standalone utility under opt command."));

static cl::opt<bool> CheckGVClobbOpt_ChkWithBales(
    "check-gv-clobbering-chk-with-bales",
    cl::init(!CheckGVClobbOpt_StandaloneMode), cl::Hidden,
    cl::desc("If true, detects \"vload -> vstore -> (vload's users bales "
             "heads)\" cases. In \"standalone\" mode shall spawn standalone "
             "baling analysis."
             "WARNING: not every IR is baling-ready, so turning this option "
             "in standalone mode while checking intermediate IR states can "
             "fail. If so, do not use this in standalone mode runs. "
             "Detects \"vload -> vstore -> (vload's users)\" when false"));

static cl::opt<bool> CheckGVClobbOpt_TryFixup(
    "check-gv-clobbering-try-fixup", cl::init(false), cl::Hidden,
    cl::desc("Try to fixup simple cases if clobbering detected."));

static cl::opt<bool> CheckGVClobbOpt_AbortOnDetection(
    "check-gv-clobbering-abort-on-detection", cl::init(false), cl::Hidden,
    cl::desc("Abort execution if potential clobbering detected."));

namespace {

class GenXGVClobberChecker : public ModulePass,
                             public IDMixin<GenXGVClobberChecker> {
private:
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  llvm::DenseMap<const Function *, GenXBaling *> BalingPerFunc;
  llvm::DenseMap<const Function *, GenXLiveness *> LivenessPerFunc;
  llvm::SmallPtrSet<BasicBlock *, 2> PhiUserExcludeBlocksOnCfgTraversal;

  StringRef DbgPrefix = "[gvload clobber checker] ";

  bool
  checkGVClobberingByInterveningStore(Instruction *LI,
                                      llvm::SmallVector<Instruction *, 8> *SIs);

  using CallSitesPerFunctionT =
      llvm::DenseMap<Function *, llvm::SmallVector<Instruction *, 8>>;
  void collectKillCallSites(
      Function *Func,
      GenXGVClobberChecker::CallSitesPerFunctionT &CallSitesPerFunction);

public:
  explicit GenXGVClobberChecker() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX GV clobber checker/fixup";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    if (CheckGVClobbOpt_StandaloneMode) {
      if (CheckGVClobbOpt_ChkWithBales) {
        AU.addRequired<TargetPassConfig>();
        AU.addRequired<DominatorTreeWrapperPass>();
      }
    } else {
      AU.addRequired<GenXModule>();
      AU.addRequired<FunctionGroupAnalysis>();
      AU.addRequired<GenXGroupBalingWrapper>();
      AU.addRequired<GenXLivenessWrapper>();
    }
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
if (CheckGVClobbOpt_StandaloneMode) {
  if (CheckGVClobbOpt_ChkWithBales) {
    INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
    INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
  }
} else {
  INITIALIZE_PASS_DEPENDENCY(GenXModule)
  INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
  INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
  INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
}
INITIALIZE_PASS_END(GenXGVClobberChecker, "GenXGVClobberChecker",
                    "GenX global volatile clobbering checker", false, false)

ModulePass *llvm::createGenXGVClobberCheckerPass() {
  initializeGenXGVClobberCheckerPass(*PassRegistry::getPassRegistry());
  return new GenXGVClobberChecker();
}

bool GenXGVClobberChecker::checkGVClobberingByInterveningStore(
    Instruction *LI, llvm::SmallVector<Instruction *, 8> *SIs) {

  auto CheckUserInst = [&](Instruction *UI) -> bool {
    // TODO: this is an exceptional case. Maybe change GenXArgIndirectionWrapper
    // logic not to produce such an unused bitcasts.
    if (UI->hasNUses(0) /*llvm::isInstructionTriviallyDead(UI) is more expensive
                           and not necessary for our usecase. */
        && isa<BitCastInst>(UI)) {
      LLVM_DEBUG(
          dbgs()
          << "Skipping " << *UI
          << " a trivially dead bitcast coming from GenXArgIndirectionWrapper "
             "as not a real use of vload result.\n");
      return false;
    }

    if (isa<PHINode>(UI)) {
      vc::diagnose(
          LI->getContext(), DbgPrefix,
          "PHI node as an immediate vload user found, this will "
          "result with phicopy insertion during GenXCoalescing resulting "
          "in additional register pressure, whereas the initial intent was to "
          "have no additional copies of the value being loaded.",
          DS_Warning, vc::WarningName::Generic);
      PhiUserExcludeBlocksOnCfgTraversal.clear();
      for (const auto &V : cast<PHINode>(UI)->incoming_values())
        if (auto *I = dyn_cast<Instruction>(V.get()))
          PhiUserExcludeBlocksOnCfgTraversal.insert(I->getParent());
    }

    const auto *SI = genx::getAVLoadKillOrNull(
        LI, UI, true, nullptr,
        isa<PHINode>(UI) ? &PhiUserExcludeBlocksOnCfgTraversal : nullptr, SIs);

    if (!SI)
      return false;

    vc::diagnose(LI->getContext(), DbgPrefix,
                 "found a vstore intervening before value usage ", DS_Warning,
                 vc::WarningName::Generic, UI);
    vc::diagnose(LI->getContext(), "...", "intervening vstore", DS_Warning,
                 vc::WarningName::Generic, SI);
    LLVM_DEBUG(dbgs() << DbgPrefix << "Found intervening vstore: " << *SI
                      << "\n"
                      << DbgPrefix << "Affected vload: " << *LI << "\n"
                      << DbgPrefix << "User: " << *UI << "\n"
                      << DbgPrefix << "\n");

    if (CheckGVClobbOpt_TryFixup) {
      if (GenXIntrinsic::isRdRegion(UI) &&
          isa<Constant>(
              UI->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum))) {
        UI->moveAfter(LI);
        if (!Baling || !Liveness)
          vc::diagnose(LI->getContext(), DbgPrefix,
                       "Either Baling or Liveness analysis results are not "
                       "available",
                       DS_Warning, vc::WarningName::Generic, UI);
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
        return true;
      }
      vc::diagnose(LI->getContext(), DbgPrefix,
                   "fixup is only possible for rdregion with constant "
                   "offsets as it has single input from vload and "
                   "can be easily moved back to it, however current case is "
                   "more complex.",
                   DS_Warning, vc::WarningName::Generic, UI);
    }
    return CheckGVClobbOpt_AbortOnDetection;
  };

  if (!CheckGVClobbOpt_StandaloneMode) {
    Baling = BalingPerFunc[LI->getFunction()];
    if (CheckGVClobbOpt_TryFixup)
      Liveness = LivenessPerFunc[LI->getFunction()];
  }

  bool Detected = false;
  for (const auto &U : LI->users())
    Detected |= CheckUserInst(
        CheckGVClobbOpt_ChkWithBales
            ? Baling->getBaleHead(dyn_cast<Instruction>(U))
            : dyn_cast<Instruction>(genx::peelBitCastsInSingleUseChain(U)));

  return Detected;
};

void GenXGVClobberChecker::collectKillCallSites(
    Function *Func,
    GenXGVClobberChecker::CallSitesPerFunctionT &CallSitesPerFunction) {
  llvm::SmallPtrSet<Function *, 4> VisitedFuncs;
  llvm::SmallVector<Function *, 32> Stack;
  Stack.push_back(Func);
  while (!Stack.empty()) {
    auto *CurrFunc = Stack.pop_back_val();
    if (llvm::find(VisitedFuncs, CurrFunc) != VisitedFuncs.end())
      continue;
    VisitedFuncs.insert(CurrFunc);
    for (const auto &FuncUser : CurrFunc->users()) {
      if (isa<CallBase>(FuncUser)) {
        auto *Call = cast<Instruction>(FuncUser);
        CallSitesPerFunction[Call->getFunction()].push_back(Call);
        Stack.push_back(Call->getFunction());
      }
    }
  }
};

bool GenXGVClobberChecker::runOnModule(Module &M) {
  llvm::SetVector<Instruction *> Loads;
  std::unordered_map<Value *, CallSitesPerFunctionT> KillCallSites;

  if (CheckGVClobbOpt_CollectKillCallSites)
    LLVM_DEBUG(dbgs() << DbgPrefix
                      << "Checking in non-strict mode (matching as potentially "
                         "clobbering only "
                         "call sites that can result in gvstore to the related "
                         "global volatile value).\n");

  if (CheckGVClobbOpt_AbortOnDetection)
    LLVM_DEBUG(dbgs() << DbgPrefix << "Aborting if potential clobbering.\n");

  if (CheckGVClobbOpt_StandaloneMode) {
    if (CheckGVClobbOpt_ChkWithBales) {
      LLVM_DEBUG(dbgs() << DbgPrefix
                        << "Instantiating local baling info helper.\n");
      vc::diagnose(
          M.getContext(), DbgPrefix,
          "WARNING: not every IR is baling-ready, so turning this option "
          "in standalone mode while checking intermediate IR states can "
          "fail. If so, do not use this in standalone mode runs.",
          DS_Warning, vc::WarningName::Generic);

      Baling = new GenXBaling(BalingKind::BK_Analysis,
                              &getAnalysis<TargetPassConfig>()
                                   .getTM<GenXTargetMachine>()
                                   .getGenXSubtarget());
      bool BalingChangedCode = false;
      for (auto &F : M) {
        if (F.isDeclaration())
          continue;
        BalingChangedCode |= Baling->processFunction(
            F, getAnalysis<DominatorTreeWrapperPass>(F).getDomTree());
      }
      if (BalingChangedCode)
        vc::diagnose(M.getContext(), DbgPrefix,
                     "Baling analysis has changed the original code.",
                     DS_Warning, vc::WarningName::Generic);
    } else {
      LLVM_DEBUG(dbgs() << DbgPrefix << "Checking with no baling info.\n");
    }
  } else {
    auto &FGA = getAnalysis<FunctionGroupAnalysis>();
    auto &LivenessFGWrapper = getAnalysis<GenXLivenessWrapper>();
    for (const auto &F : M) {
      if (F.isDeclaration())
        continue;
      LivenessPerFunc[&F] =
          &LivenessFGWrapper.getFGPassImpl(FGA.getAnyGroup(&F));
      if (CheckGVClobbOpt_ChkWithBales)
        BalingPerFunc[&F] =
            &getAnalysis<GenXGroupBalingWrapper>().getFGPassImpl(
                FGA.getAnyGroup(&F));
    }
  }

  for (auto &G : M.globals()) {
    if (!G.hasAttribute(genx::FunctionMD::GenXVolatile))
      continue;
    for (auto *V : genx::peelBitCastsGetUserValues(&G)) {
      if (auto *I = dyn_cast<Instruction>(V)) {
        if (genx::isAVLoad(I))
          Loads.insert(I);
        else if (CheckGVClobbOpt_CollectKillCallSites && genx::isAVStore(I))
          collectKillCallSites(I->getFunction(), KillCallSites[&G]);
      }
    }
  }

  if (Loads.empty())
    return false;

  bool ChangedOrNeedToAbort = false;
  for (const auto &LI : Loads)
    ChangedOrNeedToAbort |= checkGVClobberingByInterveningStore(
        LI, CheckGVClobbOpt_CollectKillCallSites
                ? &KillCallSites[genx::getBitCastedValue(LI->getOperand(0))]
                                [LI->getFunction()]
                : nullptr);

  if (CheckGVClobbOpt_AbortOnDetection && ChangedOrNeedToAbort) {
    dbgs() << "\n[WARNING] Aborting on potential global volatile clobbering, "
              "as requested.\n";
    std::abort();
  }

  if (CheckGVClobbOpt_ChkWithBales && CheckGVClobbOpt_StandaloneMode)
    delete Baling;

  return ChangedOrNeedToAbort;
}
