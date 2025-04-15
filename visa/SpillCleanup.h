/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPILLCLEANUP_H__
#define __SPILLCLEANUP_H__

#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "RPE.h"

namespace vISA {
class CoalesceSpillFills {
private:
  G4_Kernel &kernel;
  LivenessAnalysis &liveness;
  GraphColor &graphColor;
  GlobalRA &gra;
  SpillManagerGRF &spill;
  unsigned int iterNo;
  // Store declares spilled by sends like sampler
  std::set<G4_Declare *> sendDstDcl;
  RPE &rpe;
  // Spill cleanup is a per-BB optimization. Store current BB being optimized.
  G4_BB *curBB = nullptr;
  bool isCm = false;

  // Set window size to coalesce
  const unsigned int cWindowSize = 10;
  const unsigned int cMaxWindowSize = 20;
  const unsigned int cMaxFillPayloadSize = 4;
  const unsigned int cMaxSpillPayloadSize = 4;
  const unsigned int cSpillFillCleanupWindowSize = 10;
  const unsigned int cFillWindowThreshold128GRF = 180;
  const unsigned int cSpillWindowThreshold128GRF = 120;
  const unsigned int cHighRegPressureForCleanup = 100;
  const unsigned int cHighRegPressureForWindow = 70;
  const unsigned int cInputSizeLimit = 70;

  unsigned int fillWindowSizeThreshold = 0;
  unsigned int spillWindowSizeThreshold = 0;
  unsigned int highRegPressureForCleanup = 0;
  unsigned int highRegPressureForWindow = 0;
  unsigned int inputSizeLimit = 0;
  unsigned int spillFillCleanupWindowSize = 0;
  unsigned int totalInputSize = 0;

  // Debug flags
  unsigned int spillCleanupStartBBId = 0;
  unsigned int spillCleanupEndBBId = 0xffffffff;

  bool isSpillCleanupEnabled(const G4_BB *bb) const {
    auto bbId = bb->getId();
    return (bbId >= spillCleanupStartBBId && bbId <= spillCleanupEndBBId);
  }

  // <Old fill declare*, std::pair<Coalesced Decl*, Row Off>>
  // This data structure is used to replaced old spill/fill operands
  // with coalesced operands with correct offset.
  std::map<G4_Declare *, std::pair<G4_Declare *, unsigned int>> replaceMap;

  bool replaceCoalescedOperands(G4_INST *);

  void dumpKernel();
  void dumpKernel(unsigned int v1, unsigned int v2);

  bool notOOB(unsigned int min, unsigned int max);
  void sendsInRange(std::list<INST_LIST_ITER> &, std::list<INST_LIST_ITER> &,
                    unsigned int, unsigned int &, unsigned int &);
  void keepConsecutiveSpills(std::list<INST_LIST_ITER> &,
                             std::list<INST_LIST_ITER> &, unsigned int,
                             unsigned int &, unsigned int &, bool &,
                             G4_InstOption &);
  void fills();
  void spills();
  INST_LIST_ITER analyzeFillCoalescing(std::list<INST_LIST_ITER> &,
                                       INST_LIST_ITER, INST_LIST_ITER);
  INST_LIST_ITER analyzeSpillCoalescing(std::list<INST_LIST_ITER> &,
                                        INST_LIST_ITER, INST_LIST_ITER);
  void removeWARFills(std::list<INST_LIST_ITER> &, std::list<INST_LIST_ITER> &);
  void coalesceFills(std::list<INST_LIST_ITER> &, unsigned int, unsigned int);
  G4_INST *generateCoalescedFill(G4_SrcRegRegion *, unsigned int, unsigned int,
                                 unsigned int, bool);
  G4_SrcRegRegion *generateCoalescedSpill(G4_SrcRegRegion *, unsigned int,
                                          unsigned int, bool, G4_InstOption,
                                          G4_Declare *, unsigned int);
  bool fillHeuristic(std::list<INST_LIST_ITER> &, std::list<INST_LIST_ITER> &,
                     const std::list<INST_LIST_ITER> &, unsigned int &,
                     unsigned int &);
  bool overlap(G4_INST *, std::list<INST_LIST_ITER> &);
  bool overlap(G4_INST *, G4_INST *, bool &);
  void coalesceSpills(std::list<INST_LIST_ITER> &, unsigned int, unsigned int,
                      bool, G4_InstOption);
  bool allSpillsSameVar(std::list<INST_LIST_ITER> &);
  void fixSendsSrcOverlap();
  void removeRedundantSplitMovs();
  G4_Declare *createCoalescedSpillDcl(unsigned int);
  void populateSendDstDcl();
  void spillFillCleanup();
  void removeRedundantWrites();
  // For Cm, if BB is in divergent CF and spill inst and fill inst have
  // mismatched WriteEnable bit then return true as cleanup may be
  // illegal.
  bool isIncompatibleEMCm(G4_INST *inst1, G4_INST *inst2) const;

public:
  CoalesceSpillFills(G4_Kernel &k, LivenessAnalysis &l, GraphColor &g,
                     SpillManagerGRF &s, unsigned int iterationNo, RPE &r,
                     GlobalRA &gr)
      : kernel(k), liveness(l), graphColor(g), gra(gr), spill(s),
        iterNo(iterationNo), rpe(r) {
    fillWindowSizeThreshold =
        kernel.getScaledGRFSize(cFillWindowThreshold128GRF);
    spillWindowSizeThreshold =
        kernel.getScaledGRFSize(cSpillWindowThreshold128GRF);
    highRegPressureForCleanup =
        kernel.getScaledGRFSize(cHighRegPressureForCleanup);
    highRegPressureForWindow =
        kernel.getScaledGRFSize(cHighRegPressureForWindow);
    inputSizeLimit = kernel.getScaledGRFSize(cInputSizeLimit);
    spillFillCleanupWindowSize = std::min<unsigned int>(
        kernel.getScaledGRFSize(cSpillFillCleanupWindowSize),
        cSpillFillCleanupWindowSize);

    auto &inputs = k.fg.builder->m_inputVect;
    for (const input_info_t *input_info : inputs) {
      totalInputSize += input_info->size;
    }
    totalInputSize = totalInputSize / k.numEltPerGRF<Type_UB>();

    isCm = (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM);

    spillCleanupStartBBId =
        kernel.getOptions()->getuInt32Option(vISA_SpillCleanupStartBBID);
    spillCleanupEndBBId =
        kernel.getOptions()->getuInt32Option(vISA_SpillCleanupEndBBID);
  }

  void run();

  static void getScratchMsgInfo(G4_INST *inst, unsigned int &scratchOffset,
                                unsigned int &size) {
    if (inst->isSpillIntrinsic()) {
      scratchOffset = inst->asSpillIntrinsic()->getOffset();
      size = inst->asSpillIntrinsic()->getNumRows();
    } else if (inst->isFillIntrinsic()) {
      scratchOffset = inst->asFillIntrinsic()->getOffset();
      size = inst->asFillIntrinsic()->getNumRows();
    } else {
      vISA_ASSERT(false, "unknown inst type");
    }
  }
};
} // namespace vISA

#endif
