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
#ifndef TARGET_GENX_PRESSURE_TRACKER_H
#define TARGET_GENX_PRESSURE_TRACKER_H

#include <vector>

namespace llvm {

class Value;
class GenXLiveness;
class FunctionGroup;

namespace genx {

class LiveRange;

class PressureTracker {
  FunctionGroup &FG;
  GenXLiveness *Liveness;
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
  PressureTracker(FunctionGroup &FG, GenXLiveness *L,
                  bool WithByteWidening = false)
      : FG(FG), Liveness(L), WithByteWidening(WithByteWidening) {
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
