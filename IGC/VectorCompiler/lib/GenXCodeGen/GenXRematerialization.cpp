/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXRematerialization
/// ---------------------
///
/// This pass performs rematerialization to reduce register pressure.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXPressureTracker.h"
#include "GenXUtil.h"
#include "llvm/Pass.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

class GenXRematerialization : public FGPassImplInterface,
                              public IDMixin<GenXRematerialization> {
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  bool Modified = false;

public:
  explicit GenXRematerialization() {}
  static StringRef getPassName() { return "GenX rematerialization pass"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void remat(Function *F, PressureTracker &RP);
};

} // namespace

namespace llvm {
void initializeGenXRematerializationWrapperPass(PassRegistry &);
using GenXRematerializationWrapper =
    FunctionGroupWrapperPass<GenXRematerialization>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXRematerializationWrapper,
                      "GenXRematerializationWrapper",
                      "GenXRematerializationWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXNumberingWrapper)
INITIALIZE_PASS_END(GenXRematerializationWrapper,
                    "GenXRematerializationWrapper",
                    "GenXRematerializationWrapper", false, false)

ModulePass *llvm::createGenXRematerializationWrapperPass() {
  initializeGenXRematerializationWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXRematerializationWrapper;
}

void GenXRematerialization::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXNumbering>();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<FunctionGroupAnalysis>();
  AU.setPreservesCFG();
}

bool GenXRematerialization::runOnFunctionGroup(FunctionGroup &FG) {
  if (skipOptWithLargeBlock(FG))
    return false;

  Modified = false;
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Numbering = &getAnalysis<GenXNumbering>();
  const auto &DL = FG.getModule()->getDataLayout();
  PressureTracker RP(DL, FG, Liveness);
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi)
    remat(*fgi, RP);
  return Modified;
}

void GenXRematerialization::remat(Function *F, PressureTracker &RP) {
  // Collect rematerialization candidates.
  std::vector<Use *> Candidates;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto &Inst : BB.getInstList()) {
      // (1) upward cast
      if (auto CI = dyn_cast<CastInst>(&Inst)) {
        if (CI->getOpcode() != Instruction::UIToFP &&
            CI->getOpcode() != Instruction::SIToFP)
          continue;
        if (!CI->getType()->isVectorTy())
          continue;
        if (CI->getSrcTy()->getScalarSizeInBits() >=
            CI->getDestTy()->getScalarSizeInBits())
          continue;
        if (Inst.isUsedOutsideOfBlock(&BB) || Inst.getNumUses() <= 2)
          continue;
        LiveRange *LR = Liveness->getLiveRangeOrNull(CI);
        if (!LR || LR->value_size() != 1)
          continue;
        IGC_ASSERT(LR->value_begin()->getValue() == CI);
        unsigned B = Numbering->getNumber(CI);
        for (auto &U : CI->uses()) {
          auto UI = U.getUser();
          unsigned E = Numbering->getNumber(UI);
          if (E > B && RP.intersectWithRedRegion(B, E))
            Candidates.push_back(&U);
        }
      }
    }
  }

  // Do rematerialization.
  for (auto U : Candidates) {
    Instruction *Inst = cast<Instruction>(U->get());
    Instruction *UI = cast<Instruction>(U->getUser());
    Instruction *Clone = Inst->clone();
    Clone->insertBefore(UI);
    U->set(Clone);
    Modified = true;
  }
}
