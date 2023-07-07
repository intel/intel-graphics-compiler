/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _EMU64ADD_H_
#define _EMU64ADD_H_

#include "BuildIR.h"
#include "Common_ISA_util.h"
#include "FlowGraph.h"
#include "G4_IR.hpp"


namespace vISA {
class EmuInt64Add : HWConformity {
  IR_Builder &builder;
  G4_Kernel &kernel;

  G4_DstRegRegion *createSplitDstRegion(G4_Declare *alias, G4_Type newType) {
      G4_Declare *dcl = builder.createTempVar(
          g4::SIMD2, newType, builder.getGRFAlign());
      dcl->setAliasDeclare(alias, 0);
      return builder.createDstRegRegion(dcl, 1);
  }
  G4_SrcRegRegion *createSplitSrcRegion(G4_Declare *alias, G4_Type newType) {
      G4_Declare *dcl = builder.createTempVar(
          g4::SIMD2, newType, builder.getGRFAlign());
      dcl->setAliasDeclare(alias, 0);
      return builder.createSrcRegRegion(dcl, builder.getRegionStride1());
  }
public:
  EmuInt64Add(IR_Builder &b, G4_Kernel &k) : HWConformity(b, k), builder(b), kernel(k) {}
  void transform();
};
} // namespace vISA
// single entry point for emulating int64 add
extern void EmulateInt64Add(vISA::IR_Builder &builder, vISA::G4_Kernel &kernel);

#endif /* _EMU64ADD_H_ */
