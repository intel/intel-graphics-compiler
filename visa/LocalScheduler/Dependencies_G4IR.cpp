/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Dependencies_G4IR.h"
#include "../Gen4_IR.hpp"

using namespace vISA;

enum retDepType { RET_RAW = 1, RET_WAW, RET_WAR };

// Checks for memory interferences created with the "send" instruction for data port.
static DepType DoMemoryInterfereSend(G4_InstSend *send1, G4_InstSend *send2, retDepType depT, const Options *m_options,
                                     bool BTIIsRestrict)
{
    // If either instruction is not a send then there cannot be a memory interference.
    if (!send1 || !send2 || !send1->isSend() || !send2->isSend())
    {
        return NODEP;
    }

    if (send1->getMsgDesc()->isSendBarrier() || send2->getMsgDesc()->isSendBarrier())
    {
        return MSG_BARRIER;
    }

    bool isSend1HDC = send1->getMsgDesc()->isHDC();
    bool isSend2HDC = send2->getMsgDesc()->isHDC();

    VISATarget target = m_options->getTarget();
    if (target == VISA_3D || target == VISA_CS)
    {
        // for 3D and compute we can do more precise dependency checks due to API restrictions
        SFID funcId1 = send1->getMsgDesc()->getFuncId();
        SFID funcId2 = send2->getMsgDesc()->getFuncId();

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
            if (send1->getMsgDesc()->isSLMMessage() ^ send2->getMsgDesc()->isSLMMessage())
            {
                // SLM may not conflict with other non-SLM messages
                return NODEP;
            }
            else if (send1->getMsgDesc()->getBti() && send2->getMsgDesc()->getBti())
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
    }

    // TODO: We can add more precise memory conflict checks here for special messages
    // (e.g., URB that have constant offset)

    // scratch RW may only conflict with other scratch RW
    if (send1->getMsgDesc()->isScratchRW() != send2->getMsgDesc()->isScratchRW())
    {
        return NODEP;
    }

    // Determine any relevant memory interferences through data port operations.
    if (send1->getMsgDesc()->isDataPortWrite())
    {
        if (depT == RET_RAW && send2->getMsgDesc()->isDataPortRead())
        {
            return RAW_MEMORY;
        }
        else if (depT == RET_WAW && send2->getMsgDesc()->isDataPortWrite())
        {
            return WAW_MEMORY;
        }
        else
        {
            return NODEP;
        }
    }
    else if (send1->getMsgDesc()->isDataPortRead())
    {
        if (depT == RET_WAR && send2->getMsgDesc()->isDataPortWrite())
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
    if (send1->getMsgDesc()->isScratchRW() != send2->getMsgDesc()->isScratchRW())
    {
        return NODEP;
    }

    // check dependency between scratch block read/write
    if (send1->getMsgDesc()->isScratchRW() && send2->getMsgDesc()->isScratchRW())
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
            uint16_t leftOff1, leftOff2, rightOff1, rightOff2;
            leftOff1 = send1->getMsgDesc()->getScratchRWOffset();
            leftOff2 = send2->getMsgDesc()->getScratchRWOffset();
            rightOff1 = leftOff1 + send1->getMsgDesc()->getScratchRWSize() - 1;
            rightOff2 = leftOff2 + send2->getMsgDesc()->getScratchRWSize() - 1;
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

DepType vISA::getDepSend(G4_INST *curInst, G4_INST *liveInst, const Options *m_options,
    bool BTIIsRestrict)
{
    for (auto RDEP : { RET_RAW, RET_WAR, RET_WAW })
    {
        DepType dep = DoMemoryInterfereSend(curInst->asSendInst(), liveInst->asSendInst(), RDEP, m_options, BTIIsRestrict);
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
        if (inst->getMsgDesc()->isEOTInst())
        {
            // Send with the EOT message desciptor is a barrier.
            return SEND_BARRIER;
        }
        else if (inst->getMsgDesc()->isThreadMessage())
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

