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
  G4_DstRegRegion *dst = inst->getDst();
  if (dst && dst->getTopDcl() && dst->getTopDcl()->getRegFile() == G4_GRF) {
    auto useIter = std::find_if(
        inst->use_begin(), inst->use_end(),
        [](USE_DEF_NODE useNode) { return useNode.first->isSend(); });

    if (useIter == inst->use_end()) {
      jitInfo->statsVerbose.numALUOnlyDst++;
    }
  }

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *srcOpnd = inst->getSrc(i);
    if (!srcOpnd) {
      continue;
    }
    if (!srcOpnd->isSrcRegRegion() || srcOpnd->isImm() ||
        srcOpnd->isAddrExp() ||
        srcOpnd->asSrcRegRegion()->getRegAccess() != Direct) {
      continue;
    }

    if (!srcOpnd->asSrcRegRegion()->getBase() ||
        !srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
      continue;
    }
    if (srcOpnd->getBase()->asRegVar()->getDeclare()->getRegFile() != G4_GRF) {
      continue;
    }

    Gen4_Operand_Number opndNum = (Gen4_Operand_Number)(i + 1);
    auto defIter = std::find_if(
        inst->def_begin(), inst->def_end(), [opndNum](USE_DEF_NODE defNode) {
          return defNode.second == opndNum && defNode.first->isSend();
        });

    if (defIter == inst->def_end()) {
      jitInfo->statsVerbose.numALUOnlySrc++;
    }
  }
}
