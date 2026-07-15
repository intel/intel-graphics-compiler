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

typedef struct BBEntryState {
  std::unordered_map<unsigned, std::set<unsigned>> offsetToGRFs;
  std::unordered_map<unsigned, std::set<G4_Declare *>> grfToDeclare;
} BBEntryState;

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
  // Map GRF -> set of G4_Declare* to mark latter as global
  std::unordered_map<unsigned, std::set<G4_Declare *>> grfToDeclare;
  // Per-BB entry state for fall-through propagation
  std::unordered_map<G4_BB *, BBEntryState> bbEntryState;
  // BB_top's exit state, keyed by diamond BB_bot; consumed at BB_mid.
  std::unordered_map<G4_BB *, BBEntryState> diamondTopExitState;

  void clearTable();
  void addEntry(unsigned scratchOffset, unsigned grfNum, bool clearOld = false);
  void invalidateGRF(unsigned grfNum);
  void invalidateOffset(unsigned offset);
  std::pair<unsigned, unsigned> getGRFRange(G4_Operand *opnd);
  bool canReplaceFill(G4_FillIntrinsic *fill, std::vector<unsigned> &srcGRFs);
  bool replaceFillWithMovs(G4_BB *bb, INST_LIST_ITER &it,
                           G4_FillIntrinsic *fill,
                           const std::vector<unsigned> &srcGRFs);
  void invalidateClobberedEntries(G4_INST *inst);
  void invalidateDst(G4_INST *inst,
                     std::unordered_map<unsigned, PendingFill> &pendingFills);
  void invalidateSrcs(G4_INST *inst,
                      std::unordered_map<unsigned, PendingFill> &pendingFills);
  void processBBForward(G4_BB *bb);
  void processBBBackward(G4_BB *bb);
  static bool isPredicatedBranch(G4_INST *inst);
  static bool isDiamondTop(G4_BB *bb, G4_BB *&mid, G4_BB *&bot);
  static bool isDiamondMid(G4_BB *bb, G4_BB *&top, G4_BB *&bot);
  void propagateFallThrough(G4_BB *bb);
  void snapshotDiamondTopExit(G4_BB *bb);
  void applyDiamondMidIntersection(G4_BB *bb);
  void removeRedundantSpills();
  void removeSpillWithoutFill();
  bool replaceFillWithMovsAfter(G4_BB *bb, INST_LIST_RITER &rit,
                                INST_LIST_ITER &insertAfterIt,
                                const PendingFill &pf,
                                const std::vector<unsigned> &srcGRFs);
  bool hasAssignedGRF(G4_Declare *topdcl) const;
  void markGlobalDcls(const std::vector<unsigned int> &GRFs);
  void mapGRFsToDcl(unsigned int startGRF, unsigned int numGRFs,
                    G4_Declare *dcl);

public:
  SpillFillPropagation(G4_Kernel &k, IR_Builder &b, GlobalRA &g);
  void run();
  // Max scratch byte end offset over remaining spill/fill intrinsics.
  unsigned getMaxSpillAreaOffset();
};

} // namespace vISA

#endif // SPILL_FILL_PROPAGATION_H
