/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StaticProfiling.hpp"
#include <cmath>

using namespace vISA;

void StaticProfiling::ALUInstructionProfile(G4_INST *inst) {
  if (inst->isSend() || inst->isLabel() || inst->isCFInst() || inst->isDpas() ||
      inst->isIntrinsic()) {
    return;
  }

  FINALIZER_INFO *jitInfo = builder.getJitInfo();

  jitInfo->statsVerbose.numALUInst++;
}
