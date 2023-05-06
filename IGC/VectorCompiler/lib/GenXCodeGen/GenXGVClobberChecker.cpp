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

#include <map>

#define DEBUG_TYPE "GENX_CLOBBER_CHECKER"

using namespace llvm;
using namespace genx;

static cl::opt<bool> CheckGVClobberingCollectRelatedGVStoreCallSites(
    "check-gv-clobbering-collect-store-related-call-sites", cl::init(false),
    cl::Hidden,
    cl::desc("If not enabled, we shall assume that any user function call can "
             "potentially clobber the GV value."
             "With this option enabled make this more precise by collecting "
             "user function call sites that can result in clobbering "
             "and account only for those."));

namespace {
class GenXGVClobberChecker : public FGPassImplInterface,
                             public IDMixin<GenXGVClobberChecker> {
private:
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;

public:
  explicit GenXGVClobberChecker() {}
  static StringRef getPassName() { return "GenX GV clobber checker/fixup"; }
  static void getAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<GenXLiveness>();
    AU.addRequired<GenXGroupBaling>();
    AU.addPreserved<GenXGroupBaling>();
    AU.addPreserved<GenXLiveness>();
    AU.addPreserved<GenXModule>();
    AU.addPreserved<FunctionGroupAnalysis>();
    AU.setPreservesCFG();
  }
  bool runOnFunctionGroup(FunctionGroup &FG) override;
};
} // namespace

namespace llvm {
void initializeGenXGVClobberCheckerWrapperPass(PassRegistry &);
using GenXGVClobberCheckerWrapper =
    FunctionGroupWrapperPass<GenXGVClobberChecker>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXGVClobberCheckerWrapper,
                      "GenXGVClobberCheckerWrapper",
                      "GenXGVClobberCheckerWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_END(GenXGVClobberCheckerWrapper, "GenXGVClobberCheckerWrapper",
                    "GenXGVClobberCheckerWrapper", false, false)

ModulePass *llvm::createGenXGVClobberCheckerWrapperPass() {
  initializeGenXGVClobberCheckerWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXGVClobberCheckerWrapper();
}

bool GenXGVClobberChecker::runOnFunctionGroup(FunctionGroup &FG) {
  // Get analyses that we use and/or modify.
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();

  for (auto &GV : FG.getModule()->globals()) {
    if (!GV.hasAttribute(genx::FunctionMD::GenXVolatile))
      continue;

    auto *GvLiveRange = Liveness->getLiveRangeOrNull(&GV);
    if (!GvLiveRange)
      continue;

    llvm::SetVector<Instruction *> LoadsInFunctionGroup;
    std::map<Function *, llvm::SetVector<Instruction *>>
        GVStoreRelatedCallSitesPerFunction{};

    for (const auto &User : GV.users()) {
      auto *GVUserInst = dyn_cast<Instruction>(User);
      if (!GVUserInst)
        continue;

      if (llvm::find(FG, GVUserInst->getFunction()) == FG.end())
        continue;

      if (isa<LoadInst>(GVUserInst))
        LoadsInFunctionGroup.insert(GVUserInst);
      else if (CheckGVClobberingCollectRelatedGVStoreCallSites &&
               isa<StoreInst>(GVUserInst))
        genx::collectRelatedCallSitesPerFunction(
            GVUserInst, &FG, GVStoreRelatedCallSitesPerFunction);

      // Global variable is used in a constexpr.
      if (&GV != vc::getUnderlyingGlobalVariable(GVUserInst))
        continue;

      // Loads preceded by bitcasts.
      for (const auto &User : GVUserInst->users())
        if (auto *Load = dyn_cast<LoadInst>(User))
          if (llvm::find(FG, Load->getFunction()) != FG.end())
            LoadsInFunctionGroup.insert(Load);
    }

    for (const auto &LI : LoadsInFunctionGroup)
      genx::checkGVClobberingByInterveningStore(
          LI, Baling, Liveness,
          "Global volatile clobbering checker: clobbering detected,"
          " some optimizations resulted in over-optimization," /*getPassName()*/
          ,
          CheckGVClobberingCollectRelatedGVStoreCallSites
              ? &GVStoreRelatedCallSitesPerFunction[LI->getFunction()]
              : nullptr);
  }

  return true;
}
