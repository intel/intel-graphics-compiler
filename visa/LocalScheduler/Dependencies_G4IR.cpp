/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Dependencies_G4IR.h"
#include "../G4_IR.hpp"

using namespace vISA;

enum retDepType { RET_RAW = 1, RET_WAW, RET_WAR };

// Checks for memory interferences created with the "send" instruction for data port.
static DepType DoMemoryInterfereSend(G4_InstSend *send1, G4_InstSend *send2, retDepType depT, bool BTIIsRestrict)
{
    // If either instruction is not a send then there cannot be a memory interference.
    if (!send1 || !send2 || !send1->isSend() || !send2->isSend())
    {
        return NODEP;
    }
    auto isBarrierOrAtomic = [](const G4_InstSend *i) {
        return i->getMsgDesc()->isBarrier() || i->getMsgDesc()->isAtomic();
    };
    if (isBarrierOrAtomic(send1) || isBarrierOrAtomic(send2)) {
        return MSG_BARRIER;
    }

    bool isSend1HDC = send1->getMsgDesc()->isHDC();
    bool isSend2HDC = send2->getMsgDesc()->isHDC();

    SFID funcId1 = send1->getMsgDesc()->getSFID();
    SFID funcId2 = send2->getMsgDesc()->getSFID();

    if (funcId1 == SFID::SAMPLER || funcId2 == SFID::SAMPLER)
    {
        // sampler acess will never have memory conflict
        return NODEP;
    }

#define MSG_DESC_BTI_MASK 0xFF
#define RESERVED_BTI_START 240
    if (isSend1HDC ^ isSend2HDC)
    {
        // HDC messages will not conflict with other HDC messages (e.g., SAMPLER, URB, RT_WRITE)
        return NODEP;
    }
    else if (isSend1HDC && isSend2HDC)
    {
        if (send1->getMsgDesc()->isSLM() ^ send2->getMsgDesc()->isSLM())
        {
            // SLM may not conflict with other non-SLM messages
            return NODEP;
        }
        else if (send1->getMsgDesc()->getSurface() && send2->getMsgDesc()->getSurface())
        {
            G4_Operand* msgDesc1 = send1->getMsgDescOperand();
            G4_Operand* msgDesc2 = send2->getMsgDescOperand();
            if (msgDesc1->isImm() && msgDesc2->isImm())
            {
                unsigned int bti1 = (unsigned int)msgDesc1->asImm()->getInt() & MSG_DESC_BTI_MASK;
                unsigned int bti2 = (unsigned int)msgDesc2->asImm()->getInt() & MSG_DESC_BTI_MASK;
                auto isBTS = [](uint32_t bti) { return bti < RESERVED_BTI_START; };
                if (BTIIsRestrict && isBTS(bti1) && isBTS(bti2) && bti1 != bti2)
                {
                    // different BTI means no conflict for DP messages
                    return NODEP;
                }
            }
        }
    }

    // TODO: We can add more precise memory conflict checks here for special messages
    // (e.g., URB that have constant offset)

    // scratch RW may only conflict with other scratch RW
    if (send1->getMsgDesc()->isScratch() != send2->getMsgDesc()->isScratch())
    {
        return NODEP;
    }

    // Determine any relevant memory interferences through data port operations.
    if (send1->getMsgDesc()->isWrite())
    {
        if (depT == RET_RAW && send2->getMsgDesc()->isRead())
        {
            return RAW_MEMORY;
        }
        else if (depT == RET_WAW && send2->getMsgDesc()->isWrite())
        {
            return WAW_MEMORY;
        }
        else
        {
            return NODEP;
        }
    }
    else if (send1->getMsgDesc()->isRead())
    {
        if (depT == RET_WAR && send2->getMsgDesc()->isWrite())
        {
            return WAR_MEMORY;
        }

        else
        {
            return NODEP;
        }
    }

    else
    {
        return NODEP;
    }
}

static DepType DoMemoryInterfereScratchSend(G4_INST *send1, G4_INST *send2, retDepType depT)
{
    // If either instruction is not a send then there cannot be a memory interference.
    if (!send1 || !send2 || !send1->isSend() || !send2->isSend())
    {
        return NODEP;
    }

    // scratch RW may only conflict with other scratch RW
    if (send1->getMsgDesc()->isScratch() != send2->getMsgDesc()->isScratch())
    {
        return NODEP;
    }

    // check dependency between scratch block read/write
    if (send1->getMsgDesc()->isScratch() && send2->getMsgDesc()->isScratch())
    {
        bool send1IsRead = send1->getMsgDesc()->isScratchRead(),
            send2IsRead = send2->getMsgDesc()->isScratchRead();
        if (send1IsRead && send2IsRead)
        {
            return NODEP;
        }
        if ((depT == RET_WAR && send1IsRead && !send2IsRead) ||
            (depT == RET_WAW && !send1IsRead && !send2IsRead) ||
            (depT == RET_RAW && !send1IsRead && send2IsRead))
        {

            uint16_t leftOff1 = send1->getMsgDesc()->getOffset();
            uint16_t leftOff2 = send2->getMsgDesc()->getOffset();
            auto bytesAccessed = [](const G4_INST *send) {
                return send->getMsgDesc()->isRead() ?
                    (uint16_t)send->getMsgDesc()->getDstLenBytes() :
                    (uint16_t)send->getMsgDesc()->getSrc1LenBytes();
            };
            uint16_t rightOff1 = leftOff1 + bytesAccessed(send1) - 1;
            uint16_t rightOff2 = leftOff2 + bytesAccessed(send2) - 1;
            if (leftOff1 > rightOff2 || leftOff2 > rightOff1)
            {
                return NODEP;
            }

            if (send1IsRead && !send2IsRead)
            {
                return WAR_MEMORY;
            }
            if (!send1IsRead && !send2IsRead)
            {
                return WAW_MEMORY;
            }
            if (!send1IsRead && send2IsRead)
            {
                return RAW_MEMORY;
            }
        }
        return NODEP;
    }
    else
    {
        return NODEP;
    }
}

DepType vISA::getDepSend(G4_INST *curInst, G4_INST *liveInst, bool BTIIsRestrict)
{
    for (auto RDEP : { RET_RAW, RET_WAR, RET_WAW })
    {
        DepType dep = DoMemoryInterfereSend(curInst->asSendInst(), liveInst->asSendInst(), RDEP, BTIIsRestrict);
        if (dep != NODEP)
            return dep;
    }
    return NODEP;
}

DepType vISA::getDepScratchSend(G4_INST *curInst, G4_INST *liveInst)
{
    for (auto RDEP : { RET_RAW, RET_WAR, RET_WAW })
    {
        DepType dep = DoMemoryInterfereScratchSend(curInst, liveInst, RDEP);
        if (dep != NODEP)
            return dep;
    }
    return NODEP;
}

DepType vISA::CheckBarrier(G4_INST *inst)
{
    if (inst->isOptBarrier() || inst->isAtomicInst())
    {
        return OPT_BARRIER;
    }
    if (inst->isSend())
    {
        if (inst->asSendInst()->isSendc())
        {
            // sendc may imply synchronization
            return SEND_BARRIER;
        }
        if (inst->getMsgDesc()->isEOT())
        {
            // Send with the EOT message desciptor is a barrier.
            return SEND_BARRIER;
        }
        else if (inst->getMsgDescRaw() && inst->getMsgDescRaw()->isThreadMessage())
        {
            return MSG_BARRIER;
        }
    }
    else if (inst->opcode() == G4_wait || inst->isYieldInst())
    {
        return MSG_BARRIER;
    }
    else if (inst->isFlowControl())
    {
        // All control flow instructions are scheduling barriers
        return CONTROL_FLOW_BARRIER;
    }

    return NODEP;
}

