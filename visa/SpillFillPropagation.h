/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SPILL_FILL_PROPAGATION_H
#define SPILL_FILL_PROPAGATION_H

#include "G4_IR.hpp"
#include "GraphColor.h"

#include <set>
#include <unordered_map>
#include <vector>

namespace vISA {

struct PendingFill {
  G4_INST *inst;
  INST_LIST_ITER instIt;
  unsigned scratchOffset;
  unsigned numRows;
  int visaId;
  G4_Declare *dstTopDcl;
  short dstRegOff;
  unsigned dstStartGRF;
  unsigned dstEndGRF; // inclusive
};

// Post-RA pass that eliminates redundant fill intrinsics by tracking which
// physical GRFs still hold spilled/filled data and replacing fills with
// GRF-to-GRF mov instructions when possible.
//
// Runs after confirmRegisterAssignments() and before
// expandSpillFillIntrinsics().
class SpillFillPropagation {
  G4_Kernel &kernel;
  IR_Builder &builder;
  GlobalRA &gra;
  unsigned grfSize; // bytes per GRF

  // Forward map: scratch offset (GRF-row units) -> set of physical GRF numbers
  std::unordered_map<unsigned, std::set<unsigned>> offsetToGRFs;
  // Reverse map: physical GRF number -> set of scratch offsets it maps to
  std::unordered_map<unsigned, std::set<unsigned>> grfToOffsets;
  // Per-BB entry state for fall-through propagation
  std::unordered_map<G4_BB *, std::unordered_map<unsigned, std::set<unsigned>>>
      bbEntryState;

  void clearTable();
  void addEntry(unsigned scratchOffset, unsigned grfNum,
                bool clearOld = false);
  void invalidateGRF(unsigned grfNum);
  void invalidateOffset(unsigned offset);
  std::pair<unsigned, unsigned> getGRFRange(G4_Operand *opnd);
  bool canReplaceFill(G4_FillIntrinsic *fill, std::vector<unsigned> &srcGRFs);
  bool replaceFillWithMovs(G4_BB *bb, INST_LIST_ITER &it,
                           G4_FillIntrinsic *fill,
                           const std::vector<unsigned> &srcGRFs);
  void invalidateClobberedEntries(G4_INST *inst);
  void invalidateDst(
      G4_INST *inst,
      std::unordered_map<unsigned, PendingFill> &pendingFills);
  void invalidateSrcs(
      G4_INST *inst,
      std::unordered_map<unsigned, PendingFill> &pendingFills);
  void processBBForward(G4_BB *bb);
  void processBBBackward(G4_BB *bb);
  bool replaceFillWithMovsAfter(G4_BB *bb, INST_LIST_RITER &rit,
                                const PendingFill &pf,
                                const std::vector<unsigned> &srcGRFs);
  bool hasAssignedGRF(G4_Declare *topdcl) const;

public:
  SpillFillPropagation(G4_Kernel &k, IR_Builder &b, GlobalRA &g);
  void run();
};

} // namespace vISA

#endif // SPILL_FILL_PROPAGATION_H
