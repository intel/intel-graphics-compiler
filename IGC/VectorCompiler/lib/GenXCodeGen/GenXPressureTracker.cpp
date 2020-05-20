/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "GenXPressureTracker.h"
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXRegion.h"
#include "vc/GenXOpts/Utils/RegCategory.h"

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
  unsigned Bytes = (Ty->getPrimitiveSizeInBits() + 15U) / 8U & -2U;
  if (!AllowWidening)
    return Bytes;

  // Check if this will be a live range to be promoted to a word vector:
  // - this is a byte vector
  // - non-of values will be used in indirect regions
  // - all uses are in the same block (local variables only)
  //
  auto toWiden = [=]() -> bool {
    if (!Ty->isVectorTy() || !Ty->getVectorElementType()->isIntegerTy(8))
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
          Region R(cast<Instruction>(UI), BaleInfo());
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
  if (!LR || LR->getCategory() != RegCategory::GENERAL)
    return;

#if _DEBUG
  auto I = std::find(WidenCandidates.begin(), WidenCandidates.end(), LR);
  assert(I != WidenCandidates.end());
#endif

  unsigned Bytes = getSizeInBytes(LR, /*AllowWidening*/ false);
  for (auto SI = LR->begin(), SE = LR->end(); SI != SE; ++SI) {
    for (unsigned i = SI->getStart(); i != SI->getEnd(); ++i) {
      assert(i < Pressure.size());
      assert(Pressure[i] >= Bytes);
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
  if (!LR || LR->getCategory() == RegCategory::NONE)
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
    if (!LR || LR->getCategory() == RegCategory::NONE)
      continue;
    // Only process an LR if the map iterator is on the value that appears
    // first in the LR. That avoids processing the same LR multiple times.
    if (SV != *LR->value_begin())
      continue;
    LRs.push_back(LR);
  }
}
