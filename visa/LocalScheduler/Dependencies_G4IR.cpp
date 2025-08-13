/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Assertions.h"
#include "Dependencies_G4IR.h"
#include "../G4_IR.hpp"

using namespace vISA;

enum retDepType { RET_RAW = 1, RET_WAW, RET_WAR };

// Checks for memory interferences created with the "send" instruction for data
// port.
static DepType DoMemoryInterfereSend(G4_InstSend *send1, G4_InstSend *send2,
                                     retDepType depT, bool BTIIsRestrict) {
  // If either instruction is not a send then there cannot be a memory
  // interference.
  if (!send1 || !send2 || !send1->isSend() || !send2->isSend()) {
    return NODEP;
  }
  auto isBarrierOrAtomic = [](const G4_InstSend *i) {
    return i->getMsgDesc()->isBarrier() || i->getMsgDesc()->isAtomic();
  };
  if (isBarrierOrAtomic(send1) || isBarrierOrAtomic(send2)) {
    return MSG_BARRIER;
  }

  // FIXME: on XE2+, URB is also a LSC instruction even though it's not a
  // dataport message. Since not sure if URB is coherent with other LSC
  // messages, just include it to keep it unreordered for safe.
  bool isSend1DataPort =
      send1->getMsgDesc()->isHDC() || send1->getMsgDesc()->isLSC();
  bool isSend2DataPort =
      send2->getMsgDesc()->isHDC() || send2->getMsgDesc()->isLSC();

  SFID funcId1 = send1->getMsgDesc()->getSFID();
  SFID funcId2 = send2->getMsgDesc()->getSFID();

  if (funcId1 == SFID::SAMPLER || funcId2 == SFID::SAMPLER) {
    // sampler acess will never have memory conflict
    return NODEP;
  }

#define MSG_DESC_BTI_MASK 0xFF
#define RESERVED_BTI_START 240
  if (isSend1DataPort ^ isSend2DataPort) {
    // HDC messages will not conflict with other HDC messages (e.g., SAMPLER,
    // URB, RT_WRITE)
    return NODEP;
  } else if (isSend1DataPort && isSend2DataPort) {
    auto hasImmediateBTI = [](G4_InstSend *send, unsigned int &bti) {
      G4_SendDescRaw *msgDesc = send->getMsgDescRaw();
      if (msgDesc && msgDesc->isLSC() &&
          msgDesc->getLscAddrType() == LSC_ADDR_TYPE_BTI &&
          msgDesc->getBti() == nullptr) {
        // LSC messages
        bti = msgDesc->getExtendedDesc() >> 24;
        return true;
      } else if (msgDesc && msgDesc->isHDC() && msgDesc->getBti() &&
                 send->getMsgDescOperand()->isImm()) {
        // HDC messages
        bti = (unsigned int)send->getMsgDescOperand()->asImm()->getInt() &
              MSG_DESC_BTI_MASK;
        return true;
      }
      return false;
    };

    unsigned int bti1 = 0, bti2 = 0;
    if (send1->getMsgDesc()->isSLM() ^ send2->getMsgDesc()->isSLM()) {
      // SLM may not conflict with other non-SLM messages
      return NODEP;
    } else if (hasImmediateBTI(send1, bti1) && hasImmediateBTI(send2, bti2)) {
      auto isBTS = [](uint32_t bti) { return bti < RESERVED_BTI_START; };
      if (BTIIsRestrict && isBTS(bti1) && isBTS(bti2) && bti1 != bti2) {
        // different BTI means no conflict for DP messages
        return NODEP;
      }
    }
  }

  // TODO: We can add more precise memory conflict checks here for special
  // messages (e.g., URB that have constant offset)

  // scratch RW may only conflict with other scratch RW
  if (send1->getMsgDesc()->isScratch() != send2->getMsgDesc()->isScratch()) {
    return NODEP;
  }

  // Determine any relevant memory interferences through data port operations.
  if (send1->getMsgDesc()->isWrite()) {
    if (depT == RET_RAW && send2->getMsgDesc()->isRead()) {
      return RAW_MEMORY;
    } else if (depT == RET_WAW && send2->getMsgDesc()->isWrite()) {
      return WAW_MEMORY;
    } else {
      return NODEP;
    }
  } else if (send1->getMsgDesc()->isRead()) {
    if (depT == RET_WAR && send2->getMsgDesc()->isWrite()) {
      return WAR_MEMORY;
    }

    else {
      return NODEP;
    }
  }

  else {
    return NODEP;
  }
}

static DepType DoMemoryInterfereScratchSend(G4_INST *send1, G4_INST *send2,
                                            retDepType depT) {
  // If either instruction is not a send then there cannot be a memory
  // interference.
  if (!send1 || !send2 || !send1->isSend() || !send2->isSend()) {
    return NODEP;
  }

  // scratch RW may only conflict with other scratch RW
  if (send1->getMsgDesc()->isScratch() != send2->getMsgDesc()->isScratch()) {
    return NODEP;
  }

  // check dependency between scratch block read/write
  if (send1->getMsgDesc()->isScratch() && send2->getMsgDesc()->isScratch()) {
    bool send1IsRead = send1->getMsgDesc()->isScratchRead(),
         send2IsRead = send2->getMsgDesc()->isScratchRead();
    if (send1IsRead && send2IsRead) {
      return NODEP;
    }
    if ((depT == RET_WAR && send1IsRead && !send2IsRead) ||
        (depT == RET_WAW && !send1IsRead && !send2IsRead) ||
        (depT == RET_RAW && !send1IsRead && send2IsRead)) {
      // scratch guaranteed to return valid linear ImmOff
      uint32_t leftOff1 = (uint32_t)send1->getMsgDesc()->getOffset()->immOff;
      uint32_t leftOff2 = (uint32_t)send2->getMsgDesc()->getOffset()->immOff;
      auto bytesAccessed = [](const G4_INST *send) {
        return send->getMsgDesc()->isRead()
                   ? (uint16_t)send->getMsgDesc()->getDstLenBytes()
                   : (uint16_t)send->getMsgDesc()->getSrc1LenBytes();
      };
      uint32_t rightOff1 = leftOff1 + bytesAccessed(send1) - 1;
      uint32_t rightOff2 = leftOff2 + bytesAccessed(send2) - 1;
      if (leftOff1 > rightOff2 || leftOff2 > rightOff1) {
        return NODEP;
      }

      if (send1IsRead && !send2IsRead) {
        return WAR_MEMORY;
      }
      if (!send1IsRead && !send2IsRead) {
        return WAW_MEMORY;
      }
      if (!send1IsRead && send2IsRead) {
        return RAW_MEMORY;
      }
    }
    return NODEP;
  } else {
    return NODEP;
  }
}

DepType vISA::getDepSend(G4_INST *curInst, G4_INST *liveInst,
                         bool BTIIsRestrict) {
  for (auto RDEP : {RET_RAW, RET_WAR, RET_WAW}) {
    DepType dep = DoMemoryInterfereSend(
        curInst->asSendInst(), liveInst->asSendInst(), RDEP, BTIIsRestrict);
    if (dep != NODEP)
      return dep;
  }
  return NODEP;
}

DepType vISA::getDepScratchSend(G4_INST *curInst, G4_INST *liveInst) {
  for (auto RDEP : {RET_RAW, RET_WAR, RET_WAW}) {
    DepType dep = DoMemoryInterfereScratchSend(curInst, liveInst, RDEP);
    if (dep != NODEP)
      return dep;
  }
  return NODEP;
}

DepType vISA::CheckBarrier(G4_INST *inst) {
  if (inst->isOptBarrier() || inst->isAtomicInst() || inst->opcode() == G4_madm) {
    return OPT_BARRIER;
  }

  if (inst->isSend()) {

    if (inst->isSendConditional()) {
      // sendc may imply synchronization
      return SEND_BARRIER;
    }
    if (inst->isEOT()) {
      // Send with the EOT message desciptor is a barrier.
      return SEND_BARRIER;
    } else if (inst->getMsgDesc()->getSFID() == SFID::GATEWAY ||
               inst->getMsgDesc()->getSFID() == SFID::SPAWNER) {
      return MSG_BARRIER;
    }
  } else if (inst->opcode() == G4_wait ||
      inst->isYieldInst()) {
    return MSG_BARRIER;
  } else if (inst->isFlowControl()) {
    // All control flow instructions are scheduling barriers
    return CONTROL_FLOW_BARRIER;
  }
  return NODEP;
}

// Return the dependence type {RAW,WAW,WAR,NODEP} for the given operand numbers
DepType vISA::getDepForOpnd(Gen4_Operand_Number cur, Gen4_Operand_Number liv) {
  vISA_ASSERT(Opnd_dst <= cur && cur < Opnd_total_num, "bad operand #");
  vISA_ASSERT(Opnd_dst <= liv && liv < Opnd_total_num, "bad operand #");
  // clang-format off
  static constexpr DepType matrix[Opnd_total_num][Opnd_total_num] = {
    // dst,         src0,        src1,        src2,        src3,        src4,        src5,        src6,        src7,        pred,        condMod,     implAccSrc,  implAccDst
      {WAW,         RAW,         RAW,         RAW,         RAW,         RAW,         DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, RAW,         WAW,         RAW,         WAW},         // dst
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // src0
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // src1
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // src2
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // src3
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // src4
      {DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX}, // src5
      {DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX}, // src6
      {DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX}, // src7
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // pred
      {WAW,         RAW,         RAW,         RAW,         RAW,         RAW,         DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, RAW,         WAW,         RAW,         WAW},         // condMod
      {WAR,         NODEP,       NODEP,       NODEP,       NODEP,       NODEP,       DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, NODEP,       WAR,         NODEP,       WAR},         // implAccSrc
      {WAW,         RAW,         RAW,         RAW,         RAW,         RAW,         DEPTYPE_MAX, DEPTYPE_MAX, DEPTYPE_MAX, RAW,         WAW,         RAW,         WAW},         // implAccDst
  };
  // clang-format on
  vISA_ASSERT(matrix[cur][liv] != DEPTYPE_MAX, "undefined dependency");
  return matrix[cur][liv];
}
