/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _INSTSPLIT_H_
#define _INSTSPLIT_H_

#include "BuildIR.h"
#include "FlowGraph.h"

namespace vISA {
class InstSplitPass {
public:
  InstSplitPass(IR_Builder *builder);
  void run();
  void runOnBB(G4_BB *bb);
  INST_LIST_ITER splitInstruction(INST_LIST_ITER it, INST_LIST &instList);

private:
  bool needSplitByExecSize(G4_ExecSize ExecSize) const;
  G4_CmpRelation compareSrcDstRegRegion(G4_DstRegRegion *dstRegRegion,
                                        G4_Operand *opnd);
  void computeDstBounds(G4_DstRegRegion *dstRegion, uint32_t &leftBound,
                        uint32_t &rightBound);
  void computeSrcBounds(G4_SrcRegRegion *srcRegion, uint32_t &leftBound,
                        uint32_t &rightBound);
  void generateBitMask(G4_Operand *opnd, BitSet &footprint);

  IR_Builder *m_builder;
};
} // namespace vISA

#endif
