/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InsertThryld.hpp"

using namespace vISA;

// Detecting thread yield points by tracking samplers, non-sampler loads and
// ALUs separately within basic block and inserting thread yield.
// TODO: Add global analysis
void InsertThryld::run() {
  [[maybe_unused]] const auto samplerTH =
      builder.getOptions()->getuInt32Option(vISA_samplerTholdForThryld);
  const auto nonSamplerLoadTH =
      builder.getOptions()->getuInt32Option(vISA_NonsamplerLoadTholdForThryld);
  const auto aluTH =
      builder.getOptions()->getuInt32Option(vISA_aluTholdForThryld);

  for (auto bb : fg) {
    unsigned samplerCnt = 0, nonSamplerLoadCnt = 0, aluCnt = 0;

    for (INST_LIST_ITER it = bb->begin(); it != bb->end(); it++) {
      G4_INST *inst = *it;
      // Count ALU instructions
      if (!inst->nonALUInstructions())
        ++aluCnt;

      if (inst->isSend()) {
        G4_SendDesc *MsgDesc = inst->getMsgDesc();
        // Count sampler messages
        if (MsgDesc->isSampler())
          ++samplerCnt;
        // Count non-sampler load messages including atomic with return results.
        // More precisely, it's the send message that need to send data back to
        // EU.
        else if (!inst->getDst() || !inst->getDst()->isNullReg())
          ++nonSamplerLoadCnt;
      }

      if (aluCnt >= aluTH || samplerCnt >= nonSamplerLoadTH ||
          nonSamplerLoadCnt >= nonSamplerLoadTH) {
        // Insert thread yield after this instruction, and reset the counters
        auto thryldInst = builder.createInternalInst(
            nullptr, G4_thryld, nullptr, g4::NOSAT, g4::SIMD1, nullptr, nullptr,
            nullptr, InstOpt_NoOpt);
        bb->insertAfter(it, thryldInst);
        aluCnt = 0;
        samplerCnt = 0;
        nonSamplerLoadCnt = 0;
      }
    }
  }
}