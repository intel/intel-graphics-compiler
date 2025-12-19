/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _G4_INSERT_S0_MOVS_PASS_H_
#define _G4_INSERT_S0_MOVS_PASS_H_

#include "Optimizer.h"

namespace vISA{

class InsertS0Movs {
  private:
    int nextSurfaceS0QW;
    int lasts0SubReg;
    // Rationale for using vector rather than map
    // 1. The size of regS0Vec will be bounded by the # of s0 qwords, which is
    // low
    // 2. lookups is not a simple equality relation but must take into
    // account overlap of regsiter footprint
    // c.f computeHash and getEntryByRegOperand functions
    std::vector<std::pair<int64_t, int> >regS0Vec;
    FlowGraph &fg;
    G4_BB *bb;
    IR_Builder &builder;

    // A sendg that uses the indirect descriptors will have a mov instruction
    // prior to sendg that sets up the indirect descriptors in s0.* registers.
    // Multiple movs to different s0 from same source register can be optimized
    // such that redundant movs are eliminated
    // Example:
    //  (W) mov (1|M0)  s0.2<1>:q     r5.0<0;1,0>:q
    //  sendg.ugm (16|M0) r3       r6:2  null:0  s0.16  0x9000
    //  mul (16|M0) r3.0<1>:f r3.0<1;1,0>:f     r3.0<1;1,0>:f
    //  (W) mov (1|M0) s0.3<1>:q r5.0<0;1,0>:q
    //  sendg.ugm (16|M0) null r6:2  r3:1  s0.24  0x9004 {A@1,$1}
    //
    //  Notice that movs to s0.2 and s0.3 are redundant; the source operand
    //  (r5) remains unchanged

    bool doOpt;
    // Starting the first surface s0 qword from 2 rather than 0 as send indirect
    // (sendi) uses s0.0 and s0.1.
    // TODO: Before allocating s0 movs, scan the BB for sendi.
    // If there are no sendi, then we can use all s0 qwords
    static const unsigned FIRST_SURFACE_S0_QW = 2;

    G4_SrcRegRegion* allocateS0(G4_Operand* ind, INST_LIST_ITER ii);
    int64_t computeHash(G4_Operand* operand);
    int returnS0QWSubReg(int64_t hash);
    bool eraseEntryByRegOperand(int64_t hash);
    bool eraseEntryByS0Operand(int s0QW);
    std::vector<std::pair<int64_t, int> >::const_iterator getEntryByRegOperand(int64_t hash);
    std::vector<std::pair<int64_t, int> >::const_iterator getEntryByS0Operand(int s0QW);

  public:
    InsertS0Movs(FlowGraph &flowGraph, G4_BB *currBB, bool doOptimization)
        : fg(flowGraph), bb(currBB), builder(*fg.builder),
          doOpt(doOptimization) {
      nextSurfaceS0QW = FIRST_SURFACE_S0_QW;
      lasts0SubReg = builder.getScalarRegisterSizeInBytes() / 8;
      if (builder.isEfficient64bEnabled()) {
        // Last s0 QW is reserved if:
        // 1. kernel genertes spill/fill code OR
        // 2. kernel has stack call OR
        // 3. kernel object is a stack call function OR
        // 4. private memory is used
        // Note that this should match with the condition used in prolog
        // code that emits scratch setup sequence.
        if (builder.usesStack() || builder.getJitInfo()->stats.spillMemUsed ||
            fg.getKernel()->isPrivateMemUsed())
          lasts0SubReg -= 1;
      }
    }

    ~InsertS0Movs() = default;

    // top level function exposed to optimizer
    void doInsertS0Movs();
};
}
#endif // _G4_S0_MOVS_H_
