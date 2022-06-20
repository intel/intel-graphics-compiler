/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLiveRanges
/// --------------
///
/// GenXLiveRanges calculates the actual live range information (the segments)
/// on the LiveRange object for each value. See the comment at the top of
/// GenXLiveness.h for details of how the live range information works. This
/// pass calls GenXLiveness::buildLiveRange to do the work for each value.
///
/// The LiveRange object for each value already existed before this pass, as it
/// was created by GenXCategory. In the case of a value that we can now see does
/// not want a LiveRange, because it is an Instruction baled in to something,
/// we erase the LiveRange here.
///
//===----------------------------------------------------------------------===//
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXNumbering.h"

#include "vc/Support/BackendConfig.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_LIVERANGES"

using namespace llvm;
using namespace genx;

namespace {

class GenXLiveRanges : public FGPassImplInterface,
                       public IDMixin<GenXLiveRanges> {
  FunctionGroup *FG = nullptr;
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;

public:
  explicit GenXLiveRanges() {}
  static StringRef getPassName() { return "GenX live ranges analysis"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void buildLiveRanges();

  bool isPredefinedVariable(Value *) const;
};

/***********************************************************************
 * Local function for testing one assertion statement.
 * Check we don't have any leftover empty live ranges. If we do, it means
 * that a pass between GenXCategory and here has erased a value and failed
 * to erase its LiveRange, or alternatively this pass has failed to erase
 * the LiveRange for a value that does not need it because it is a baled
 * in instruction.
 */
bool testDuplicates(/*const*/ llvm::GenXLiveness& Liveness) {
  if (std::any_of(Liveness.begin(), Liveness.end(),
                  [](const auto &X) { return X.second == nullptr; })) {
    llvm::errs() << "Liveness contains NULL live ranges\n";
    return false;
  }

  auto testRange = [](const auto &X) {
    auto HasSegment = (X.second->size() > 0);
    auto UndefVal = isa<UndefValue>(X.first.getValue());
    return !(HasSegment || UndefVal);
  };

  auto WrongLR = std::find_if(Liveness.begin(), Liveness.end(), testRange);
  if (WrongLR != Liveness.end()) {
    llvm::errs() << "Bad live range\n";
    WrongLR->second->print(llvm::errs(), true);
    llvm::errs() << "\nWhole liveness:\n";
    Liveness.dump();
    llvm::errs() << "\n";
    return false;
  }
  return true;
}

} // end anonymous namespace

namespace llvm {
void initializeGenXLiveRangesWrapperPass(PassRegistry &);
using GenXLiveRangesWrapper = FunctionGroupWrapperPass<GenXLiveRanges>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXLiveRangesWrapper, "GenXLiveRangesWrapper",
                      "GenXLiveRangesWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXNumberingWrapper)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_END(GenXLiveRangesWrapper, "GenXLiveRangesWrapper",
                    "GenXLiveRangesWrapper", false, false)

ModulePass *llvm::createGenXLiveRangesWrapperPass() {
  initializeGenXLiveRangesWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXLiveRangesWrapper();
}

void GenXLiveRanges::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXNumbering>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<FunctionGroupAnalysis>();
  AU.setPreservesAll();
}

/***********************************************************************
 * runOnFunctionGroup : run the liveness analysis for this FunctionGroup
 */
bool GenXLiveRanges::runOnFunctionGroup(FunctionGroup &ArgFG)
{
  FG = &ArgFG;
  const auto &BC = getAnalysis<GenXBackendConfig>();
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Liveness->setNoCoalescingMode(BC.disableLiveRangesCoalescing());
  Liveness->setBaling(Baling);
  Liveness->setNumbering(&getAnalysis<GenXNumbering>());
  // Build the live ranges.
  Liveness->buildSubroutineLRs();
  buildLiveRanges();
  IGC_ASSERT(testDuplicates(*Liveness));
  return false;
}

/***********************************************************************
 * isPredefinedVariable : check if it's tranlated into predefined
 * variables in vISA.
 */
bool GenXLiveRanges::isPredefinedVariable(Value *V) const {
  switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
  case GenXIntrinsic::genx_predefined_surface:
  case GenXIntrinsic::genx_read_predef_reg:
  case GenXIntrinsic::genx_write_predef_reg:
    return true;
  default:
    break;
  }
  return false;
}

/***********************************************************************
 * buildLiveRanges : build live ranges for all args and instructions
 */
void GenXLiveRanges::buildLiveRanges()
{
  // Build live ranges for global variables;
  for (auto &G : FG->getModule()->globals())
    Liveness->buildLiveRange(&G);
  for (auto i = FG->begin(), e = FG->end(); i != e; ++i) {
    Function *Func = *i;
    // Build live ranges for args.
    for (auto fi = Func->arg_begin(), fe = Func->arg_end(); fi != fe; ++fi)
      Liveness->buildLiveRange(&*fi);
    if (!Func->getReturnType()->isVoidTy()) {
      // Build live range(s) for unified return value.
      Liveness->buildLiveRange(Liveness->getUnifiedRet(Func));
    }
    // Build live ranges for code.
    for (Function::iterator fi = Func->begin(), fe = Func->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        Instruction *Inst = &*bi;
        // Skip building live range for instructions
        // - has no destination
        // - is already baled, or
        // - is predefined variable in vISA.
        if (!Inst->getType()->isVoidTy() && !Baling->isBaled(Inst) &&
            !isPredefinedVariable(Inst)) {
          // Instruction is not baled in to anything.
          // First check if the result is unused and it is an intrinsic whose
          // result is marked RAW_NULLALLOWED. If so, don't create a live range,
          // so no register gets allocated.
          if (Inst->use_empty()) {
            unsigned IID = vc::getAnyIntrinsicID(Inst);
            switch (IID) {
              case GenXIntrinsic::not_any_intrinsic:
              case GenXIntrinsic::genx_rdregioni:
              case GenXIntrinsic::genx_rdregionf:
              case GenXIntrinsic::genx_wrregioni:
              case GenXIntrinsic::genx_wrregionf:
              case GenXIntrinsic::genx_wrconstregion:
                break;
              default: {
                  GenXIntrinsicInfo::ArgInfo AI
                      = GenXIntrinsicInfo(IID).getRetInfo();
                  if (AI.isRaw() && AI.rawNullAllowed()) {
                    // Unused RAW_NULLALLOWED result.
                    Liveness->eraseLiveRange(Inst);
                    continue;
                  }
                  break;
                }
            }
          }
          // Build its live range.
          Liveness->buildLiveRange(Inst);
        } else {
          // Instruction is baled in to something. Erase its live range so the
          // register allocator does not try and allocate it something.
          Liveness->eraseLiveRange(Inst);
        }
      }
    }
  }
}

