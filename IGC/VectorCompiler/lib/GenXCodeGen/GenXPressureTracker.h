/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef TARGET_GENX_PRESSURE_TRACKER_H
#define TARGET_GENX_PRESSURE_TRACKER_H

#include <vector>

namespace llvm {

class Value;
class GenXLiveness;
class FunctionGroup;
class DataLayout;

namespace genx {

class LiveRange;

class PressureTracker {
  const DataLayout &DL;
  FunctionGroup &FG;
  GenXLiveness *Liveness = nullptr;
  // Flag to widen byte vectors to word vectors if applicable.
  bool WithByteWidening;
  // Candidate variable for widening.
  std::vector<LiveRange *> WidenCandidates;
  std::vector<unsigned> Pressure;

  static const unsigned THRESHOLD = sizeof(float) * 8 * 120;
  struct Segment {
    unsigned Begin;
    unsigned End;
    Segment(unsigned B, unsigned E) : Begin(B), End(E) {}
  };
  std::vector<Segment> HighPressureSegments;

public:
  PressureTracker(const DataLayout& DL, FunctionGroup &FG, GenXLiveness *L,
                  bool WithByteWidening = false)
      : DL(DL), FG(FG), Liveness(L), WithByteWidening(WithByteWidening) {
    calculate();
    calculateRedSegments();
  }

  // Estimate the register pressure for each Instruction number.
  void calculate();

  // Calculate high pressure segments.
  void calculateRedSegments();

  // Check if segment [B, E] intersects with a high pressure region or not.
  bool intersectWithRedRegion(unsigned B, unsigned E) const;
  bool intersectWithRedRegion(LiveRange *LR) const;

  // Return the list of variables that are likely to be widened.
  const std::vector<LiveRange *> &getWidenVariables() { return WidenCandidates; }

  // Decrease pressure assuming no widening on variable for LR.
  void decreasePressure(LiveRange *LR);

private:
  void getLiveRanges(std::vector<LiveRange *> &LRs);
  void getLiveRangesForValue(Value *V, std::vector<LiveRange *> &LRs) const;
  unsigned getSizeInBytes(LiveRange *LR, bool AllowWidening);
};

} // namespace genx
} // namespace llvm

#endif  // TARGET_GENX_PRESSURE_TRACKER_H
