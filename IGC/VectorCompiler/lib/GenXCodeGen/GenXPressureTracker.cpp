/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXPressureTracker.h"
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXUtil.h"

#include "vc/Utils/GenX/RegCategory.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

struct LiveRangeAndLength {
  LiveRange *LR;
  unsigned Length;
  LiveRangeAndLength(LiveRange *LR, unsigned Length) : LR(LR), Length(Length) {}
  bool operator<(const LiveRangeAndLength &Rhs) const {
    return Length > Rhs.Length;
  }
};

} // namespace

unsigned PressureTracker::getSizeInBytes(LiveRange *LR, bool AllowWidening) {
  SimpleValue SV = *LR->value_begin();
  Value *V = SV.getValue();
  Type *Ty = IndexFlattener::getElementType(V->getType(), SV.getIndex());
  unsigned Bytes = vc::getTypeSize(Ty, &DL).inWordsCeil() * WordBytes;
  if (!AllowWidening)
    return Bytes;

  // Check if this will be a live range to be promoted to a word vector:
  // - this is a byte vector
  // - non-of values will be used in indirect regions
  // - all uses are in the same block (local variables only)
  //
  auto toWiden = [=]() -> bool {
    if (!Ty->isVectorTy() ||
        !cast<VectorType>(Ty)->getElementType()->isIntegerTy(8))
      return false;

    BasicBlock *DefBB = nullptr;
    for (auto I = LR->value_begin(), E = LR->value_end(); I != E; ++I) {
      auto Inst = dyn_cast<Instruction>((*I).getValue());
      if (!Inst)
        return false;
      if (!DefBB)
        DefBB = Inst->getParent();
      if (DefBB != Inst->getParent() || Inst->isUsedOutsideOfBlock(DefBB))
        return false;
      for (auto UI : Inst->users()) {
        if (GenXIntrinsic::isRdRegion(UI) || GenXIntrinsic::isWrRegion(UI)) {
          Region R = makeRegionFromBaleInfo(cast<Instruction>(UI), BaleInfo());
          if (R.Indirect)
            return false;
        }
      }
    }

    // OK, this is a candidate for widening.
    return true;
  };

  if (toWiden()) {
    WidenCandidates.push_back(LR);
    Bytes *= 2;
  }
  return Bytes;
}

// Decrease pressure assuming no widening on variable for LR.
void PressureTracker::decreasePressure(LiveRange *LR) {
  if (!LR || LR->getCategory() != vc::RegCategory::General)
    return;

#if _DEBUG
  auto I = std::find(WidenCandidates.begin(), WidenCandidates.end(), LR);
  IGC_ASSERT(I != WidenCandidates.end());
#endif

  unsigned Bytes = getSizeInBytes(LR, /*AllowWidening*/ false);
  for (auto SI = LR->begin(), SE = LR->end(); SI != SE; ++SI) {
    for (unsigned i = SI->getStart(); i != SI->getEnd(); ++i) {
      IGC_ASSERT(i < Pressure.size());
      IGC_ASSERT(Pressure[i] >= Bytes);
      Pressure[i] -= Bytes;
    }
  }
  calculateRedSegments();
}

void PressureTracker::calculate() {
  std::vector<LiveRange *> LRs;
  getLiveRanges(LRs);
  std::vector<LiveRangeAndLength> LRLs;
  for (auto LR : LRs)
    LRLs.emplace_back(LR, LR->getLength(/*WithWeak*/ false));
  LRs.clear();
  std::sort(LRLs.begin(), LRLs.end());

  // Keep count of the rp at each instruction number.
  Pressure.clear();
  for (auto &I : LRLs) {
    LiveRange *LR = I.LR;
    unsigned Bytes = getSizeInBytes(LR, WithByteWidening);
    for (auto SI = LR->begin(), SE = LR->end(); SI != SE; ++SI) {
      if (SI->getEnd() >= Pressure.size())
        Pressure.resize(SI->getEnd() + 1, 0);
      for (unsigned i = SI->getStart(); i != SI->getEnd(); ++i)
        Pressure[i] += Bytes;
    }
  }
}

// Calculate high pressure segments.
void PressureTracker::calculateRedSegments() {
  HighPressureSegments.clear();
  unsigned UNDEF = std::numeric_limits<unsigned>::max();
  unsigned B = UNDEF;
  unsigned E = UNDEF;
  for (unsigned i = 0; i < Pressure.size(); ++i) {
    if (Pressure[i] >= THRESHOLD) {
      if (B == UNDEF)
        B = i;
      else
        E = i;
    } else {
      if (B != UNDEF && E != UNDEF)
        HighPressureSegments.emplace_back(B, E);
      else if (B != UNDEF)
        HighPressureSegments.emplace_back(B, B);
      B = E = UNDEF;
    }
  }
}

// Check if segment [B, E] intersects with a high pressure region or not.
bool PressureTracker::intersectWithRedRegion(unsigned B, unsigned E) const {
  for (auto S : HighPressureSegments) {
    unsigned B1 = S.Begin;
    unsigned E1 = S.End;
    if (B > E1)
      continue;
    return E >= B1;
  }
  return false;
}

bool PressureTracker::intersectWithRedRegion(LiveRange *LR) const {
  if (!LR || LR->getCategory() == vc::RegCategory::None)
    return false;
  for (auto I = LR->begin(), E = LR->end(); I != E; ++I)
    if (intersectWithRedRegion(I->getStart(), I->getEnd()))
      return true;
  return false;
}

void PressureTracker::getLiveRanges(std::vector<LiveRange *> &LRs) {
  for (auto I = FG.begin(), E = FG.end(); I != E; ++I) {
    Function *F = *I;
    for (auto &Arg : F->args())
      getLiveRangesForValue(&Arg, LRs);
    if (I != FG.begin() && !F->getReturnType()->isVoidTy())
      getLiveRangesForValue(Liveness->getUnifiedRet(F), LRs);
    for (auto &BB : F->getBasicBlockList())
      for (auto &Inst : BB.getInstList())
        getLiveRangesForValue(&Inst, LRs);
  }
}

void PressureTracker::getLiveRangesForValue(
    Value *V, std::vector<LiveRange *> &LRs) const {
  auto Ty = V->getType();
  for (unsigned i = 0, e = IndexFlattener::getNumElements(Ty); i != e; ++i) {
    SimpleValue SV(V, i);
    LiveRange *LR = Liveness->getLiveRangeOrNull(SV);
    if (!LR || LR->getCategory() == vc::RegCategory::None)
      continue;
    // Only process an LR if the map iterator is on the value that appears
    // first in the LR. That avoids processing the same LR multiple times.
    if (SV != *LR->value_begin())
      continue;
    LRs.push_back(LR);
  }
}
