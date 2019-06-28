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

#include "BuildIR.h"
#include "FlowGraph.h"
#include "GraphColor.h"
#include "SpillCode.h"
#include "SpillManagerGMRF.h"
#include <list>
#include <iostream>
#include <sstream>
#include "Timer.h"
#include <fstream>
#include <algorithm>
#include "LocalRA.h"
#include "DebugInfo.h"
#include "SpillCleanup.h"
#include "Rematerialization.h"
#include "RPE.h"
#include "Optimizer.h"
#include <cmath>  // sqrt

using namespace std;
using namespace vISA;

#define FAIL_SAFE_RA_LIMIT 3

#define MIN(x,y)    (((x)<(y))? (x):(y))
#define MAX(x,y)    (((x)<(y))? (y):(x))
#define ROUND(x,y)    ((x) + ((y - x % y) % y))

unsigned int BitMask[BITS_DWORD] =
{
    0x00000001,
    0x00000002,
    0x00000004,
    0x00000008,
    0x00000010,
    0x00000020,
    0x00000040,
    0x00000080,
    0x00000100,
    0x00000200,
    0x00000400,
    0x00000800,
    0x00001000,
    0x00002000,
    0x00004000,
    0x00008000,
    0x00010000,
    0x00020000,
    0x00040000,
    0x00080000,
    0x00100000,
    0x00200000,
    0x00400000,
    0x00800000,
    0x01000000,
    0x02000000,
    0x04000000,
    0x08000000,
    0x10000000,
    0x20000000,
    0x40000000,
    0x80000000
};

const char* GraphColor::StackCallStr = "StackCall";

static const unsigned IN_LOOP_REFERENCE_COUNT_FACTOR = 4;

#define BANK_CONFLICT_HEURISTIC_INST   0.05
#define BANK_CONFLICT_HEURISTIC_REF_COUNT  0.25
#define BANK_CONFLICT_HEURISTIC_LOOP_ITERATION 5
#define BANK_CONFLICT_SEND_INST_CYCLE          60 //Some send 200, some 400 we choose the small one
#define BANK_CONFLICT_SIMD8_OVERHEAD_CYCLE     1
#define BANK_CONFLICT_SIMD16_OVERHEAD_CYCLE    2
#define INTERNAL_CONFLICT_RATIO_HEURISTIC 0.25


Interference::Interference(LivenessAnalysis* l, LiveRange**& lr, unsigned n, unsigned ns, unsigned nm,
    GlobalRA& g) : maxId(n),
    splitStartId(ns), splitNum(nm), builder(*g.kernel.fg.builder),
    gra(g), kernel(g.kernel), lrs(lr), liveAnalysis(l)
{
}

inline bool Interference::varSplitCheckBeforeIntf(unsigned v1, unsigned v2)
{
    LiveRange * l1 = lrs[v1];
    LiveRange * l2 = lrs[v2];

    if (!l1->getIsPartialDcl() &&
        !l2->getIsPartialDcl())
    {
        return false;
    }

    //Don't do interference for two split declares
    if (l1->getIsPartialDcl() &&
        l2->getIsPartialDcl())
    {
        return true;
    }

    unsigned p1 = v1;
    unsigned p2 = v2;
    //Don't do inteference for child and parent delcares
    if (l1->getIsPartialDcl())
    {
        p1 = l1->getParentLRID();
    }

    if (l2->getIsPartialDcl())
    {
        p2 = l2->getParentLRID();
    }

    if (p1 == p2)
    {
        return true;
    }

    return false;
}

BankConflict BankConflictPass::setupBankAccordingToSiblingOperand(BankConflict assignedBank, unsigned int offset, bool oneGRFBank)
{
    BankConflict tgtBank;

    MUST_BE_TRUE(assignedBank != BANK_CONFLICT_NONE, "sibling bank is not assigned");

    //Set according to sibling
    tgtBank = (assignedBank == BANK_CONFLICT_FIRST_HALF_EVEN || assignedBank == BANK_CONFLICT_FIRST_HALF_ODD) ?
        (assignedBank == BANK_CONFLICT_FIRST_HALF_EVEN ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_SECOND_HALF_EVEN) :
        (assignedBank == BANK_CONFLICT_SECOND_HALF_EVEN ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN);

    //Adjust according to the offset
    if (oneGRFBank)
    {
        if (offset % 2)
        {
            if (tgtBank == BANK_CONFLICT_SECOND_HALF_EVEN ||
                tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN)
            {
                tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_SECOND_HALF_ODD;
            }
            else
            {
                tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_ODD) ? BANK_CONFLICT_FIRST_HALF_EVEN : BANK_CONFLICT_SECOND_HALF_EVEN;
            }
        }
    }
    else
    {
        if (offset % 4 >= 2)
        {
            if (tgtBank == BANK_CONFLICT_SECOND_HALF_EVEN ||
                tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN)
            {
                tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_SECOND_HALF_ODD;
            }
            else
            {
                tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_ODD) ? BANK_CONFLICT_FIRST_HALF_EVEN : BANK_CONFLICT_SECOND_HALF_EVEN;
            }
        }
    }

    return tgtBank;
}

void refNumBasedSort(unsigned int *refNum, unsigned int *index)
{
    if (refNum[2] > refNum[1])
    {
        index[0] = 2;
        index[1] = 1;
    }
    else
    {
        index[0] = 1;
        index[1] = 2;
    }

    index[2] = 0;

    return;
}

bool BankConflictPass::hasInternalConflict3Srcs(BankConflict *srcBC)
{
    if (((srcBC[0] == BANK_CONFLICT_SECOND_HALF_EVEN ||
        srcBC[0] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
        (srcBC[1] == BANK_CONFLICT_SECOND_HALF_EVEN ||
            srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
            (srcBC[2] == BANK_CONFLICT_SECOND_HALF_EVEN ||
                srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)) ||
                ((srcBC[0] == BANK_CONFLICT_SECOND_HALF_ODD ||
                    srcBC[0] == BANK_CONFLICT_FIRST_HALF_ODD) &&
                    (srcBC[1] == BANK_CONFLICT_SECOND_HALF_ODD ||
                        srcBC[1] == BANK_CONFLICT_FIRST_HALF_ODD) &&
                        (srcBC[2] == BANK_CONFLICT_SECOND_HALF_ODD ||
                            srcBC[2] == BANK_CONFLICT_FIRST_HALF_ODD)))
    {
        return true;
    }
    if ((srcBC[0] < BANK_CONFLICT_SECOND_HALF_EVEN &&
        srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN &&
        srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN) ||
        (srcBC[0] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
            srcBC[1] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
            srcBC[2] >= BANK_CONFLICT_SECOND_HALF_EVEN))
    {
        return true;
    }

    return false;
}

void BankConflictPass::setupEvenOddBankConflictsForDecls(G4_Declare * dcl_1, G4_Declare * dcl_2,
    unsigned int offset1, unsigned int offset2,
    BankConflict &srcBC1, BankConflict &srcBC2)
{
    ASSERT_USER(srcBC1 == BANK_CONFLICT_NONE, "Wrong Bank initial value");
    ASSERT_USER(srcBC2 == BANK_CONFLICT_NONE, "Wrong Bank initial value");

    unsigned int refNum1 = gra.getNumRefs(dcl_1);
    unsigned int refNum2 = gra.getNumRefs(dcl_2);

    BankConflict bank1 = BANK_CONFLICT_NONE;
    BankConflict bank2 = BANK_CONFLICT_NONE;

    bank1 = (refNum1 >= refNum2) ? BANK_CONFLICT_FIRST_HALF_EVEN : BANK_CONFLICT_SECOND_HALF_ODD;
    bank2 = (bank1 == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;

    srcBC1 = bank1;
    srcBC2 = bank2;

    //Adjust only for the single bank allocation
    if ((offset1 + offset2) % 2)
    {
        if (refNum1 >= refNum2)
        {
            bank2 = (bank2 == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
        else
        {
            bank1 = (bank1 == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
    }

    gra.setBankConflict(dcl_1, bank1);
    gra.setBankConflict(dcl_2, bank2);

    return;
}


//
// inst opcode is G4_mad. This function sets up a simple state machine to prevent conflict
// between src 1 and 2 of mad inst. Following is how GRF file is divided in to banks:
// bank-block A = 0, 2, 4, 6, ..., 62
// bank-block B = 1, 3, 5, 7, ..., 63
// bank-block C = 64, 66, 68, ..., 126
// bank-block D = 65, 67, 69, ..., 127
//
// For ternary ops, if src1 and src2 are to the same bank then there will be an access collision.
// But unary and binary ops will have no collision, no matter what registers they use. The reason
// is second and third src operands are read in the same clock cycle, which is different than
// when src0 operand is read. This is true upto pre-SKL.
//
// Bank Conflict Herustics:
// 1. Try to balance the used registers in two banks for the potential conflicted registers.
// 2. reference number is used to decide which to be assigned first
// 3. When conflict detected, bank can be updated according to the reference count.
//
void BankConflictPass::setupBankConflictsOneGRFOld(G4_INST* inst, int &bank1RegNum, int &bank2RegNum, float GRFRatio, unsigned int &internalConflict)
{
    BankConflict srcBC[3];
    unsigned int regNum[3];
    unsigned int refNum[3];
    unsigned int offset[3];
    G4_Declare * dcls[3];
    G4_Declare * opndDcls[3];
    int bank_num = 0;

    for (int i = 0; i < 3; i++)
    {
        dcls[i] = nullptr;
        opndDcls[i] = nullptr;

        G4_Operand* src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion() || src->isAccReg())
        {
            // bank conflict not possible
            return;
        }

        dcls[i] = GetTopDclFromRegRegion(src);
        opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

        regNum[i] = dcls[i]->getNumRows();
        refNum[i] = gra.getNumRefs(dcls[i]);
        offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) / G4_GRF_REG_NBYTES;
        srcBC[i] = gra.getBankConflict(dcls[i]);

        if (src->getBase()->asRegVar()->isPhyRegAssigned())
        {
            unsigned int reg = src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
            if ((reg + offset[i]) < SECOND_HALF_BANK_START_GRF)
            {
                srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            }
            else
            {
                srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_SECOND_HALF_EVEN;
            }
            if (reg < SECOND_HALF_BANK_START_GRF)
            {
                bank1RegNum += regNum[i];
            }
            else
            {
                bank2RegNum += regNum[i];
            }
            gra.setBankConflict(dcls[i], srcBC[i]);
        }
        else if (srcBC[i] != BANK_CONFLICT_NONE)
        {
            if (offset[i] % 2)
            {
                //Get operand's bank from declare's bank
                if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN ||
                    srcBC[i] == BANK_CONFLICT_FIRST_HALF_ODD)
                {
                    srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
                }
                else
                {
                    srcBC[i] = (srcBC[i] == BANK_CONFLICT_SECOND_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_SECOND_HALF_EVEN;
                }
            }
        }

        if (i > 0)
        {
            bank_num += srcBC[i];
        }
    }

    //In case src1 and src2 share same declare, i.e. use same regsiter
    if (bank_num == 0 &&
        dcls[1] == dcls[2])
    {
        BankConflict bank1 = ((bank1RegNum * GRFRatio) > bank2RegNum) ? BANK_CONFLICT_SECOND_HALF_EVEN : BANK_CONFLICT_FIRST_HALF_EVEN;

        gra.setBankConflict(dcls[1], bank1);
        srcBC[1] = bank1;
        srcBC[2] = bank1;
        bank_num += bank1 * 2;
        if (bank1 < BANK_CONFLICT_SECOND_HALF_EVEN)
        {
            bank1RegNum += regNum[1];
        }
        else
        {
            bank2RegNum += regNum[1];
        }
    }

    //No bank assigned to src 1, 2.
    //assign the two delcares into different bundles/banks.
    if (bank_num == 0)
    {
        BankConflict bank1 = BANK_CONFLICT_NONE;
        BankConflict bank2 = BANK_CONFLICT_NONE;
        bool bank1First = false;
        if (GRFRatio == 1.0)
        {
            //For global RA: Try to reduce the size of bank 2
            if ((float)refNum[1] / regNum[1] >= (float)refNum[2] / regNum[2])
            {
                bank1 = BANK_CONFLICT_SECOND_HALF_EVEN;
                bank2 = BANK_CONFLICT_FIRST_HALF_ODD;
                bank1First = true;
            }
            else
            {
                bank2 = BANK_CONFLICT_SECOND_HALF_EVEN;
                bank1 = BANK_CONFLICT_FIRST_HALF_ODD;
            }
        }
        else
        {
            //For local RA: Try to balance two banks
            if (refNum[1] >= refNum[2])
            {
                bank1 = ((bank1RegNum * GRFRatio) > bank2RegNum) ? BANK_CONFLICT_SECOND_HALF_EVEN : BANK_CONFLICT_FIRST_HALF_EVEN;
                bank2 = (bank1 == BANK_CONFLICT_SECOND_HALF_EVEN) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_SECOND_HALF_ODD;
                bank1First = true;
            }
            else
            {
                bank2 = (bank1RegNum * GRFRatio) > bank2RegNum ? BANK_CONFLICT_SECOND_HALF_EVEN : BANK_CONFLICT_FIRST_HALF_EVEN;
                bank1 = (bank2 == BANK_CONFLICT_SECOND_HALF_EVEN) ? BANK_CONFLICT_FIRST_HALF_ODD : BANK_CONFLICT_SECOND_HALF_ODD;
            }
        }

        //Adjust only for the single bank allocation
        if ((offset[1] + offset[2]) % 2)
        {
            if (bank1First)
            {
                bank2 = (bank2 == BANK_CONFLICT_FIRST_HALF_ODD) ? BANK_CONFLICT_FIRST_HALF_EVEN : BANK_CONFLICT_SECOND_HALF_EVEN;
            }
            else
            {
                bank1 = (bank1 == BANK_CONFLICT_SECOND_HALF_ODD) ? BANK_CONFLICT_SECOND_HALF_EVEN : BANK_CONFLICT_FIRST_HALF_EVEN;
            }
        }

        if (bank1 >= BANK_CONFLICT_SECOND_HALF_EVEN)
        {
            bank2RegNum += regNum[1];
            bank1RegNum += regNum[2];
        }
        else
        {
            bank1RegNum += regNum[1];
            bank2RegNum += regNum[2];
        }

        gra.setBankConflict(dcls[1], bank1);
        gra.setBankConflict(dcls[2], bank2);
    }
    else
    {
        if (srcBC[1] == BANK_CONFLICT_NONE || srcBC[2] == BANK_CONFLICT_NONE)
        {
            //One source operand is assigned bank already
            if (srcBC[2] == BANK_CONFLICT_NONE)
            {
                srcBC[2] = setupBankAccordingToSiblingOperand(srcBC[1], offset[2], true);
                gra.setBankConflict(dcls[2], srcBC[2]);

                if (srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN)
                    bank1RegNum += regNum[2];
                else
                    bank2RegNum += regNum[2];
            }
            else
            {
                srcBC[1] = setupBankAccordingToSiblingOperand(srcBC[2], offset[1], true);
                gra.setBankConflict(dcls[1], srcBC[1]);
                if (srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN)
                    bank1RegNum += regNum[1];
                else
                    bank2RegNum += regNum[1];
            }
        }
        else if (dcls[1] != dcls[2])
        {
            if (((srcBC[1] == BANK_CONFLICT_SECOND_HALF_EVEN ||
                srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
                (srcBC[2] == BANK_CONFLICT_SECOND_HALF_EVEN ||
                    srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)) ||
                    ((srcBC[1] == BANK_CONFLICT_SECOND_HALF_ODD ||
                        srcBC[1] == BANK_CONFLICT_FIRST_HALF_ODD) &&
                        (srcBC[2] == BANK_CONFLICT_SECOND_HALF_ODD ||
                            srcBC[2] == BANK_CONFLICT_FIRST_HALF_ODD)))
            {
                internalConflict++;
            }
            if ((srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN &&
                srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN) ||
                (srcBC[1] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
                    srcBC[2] >= BANK_CONFLICT_SECOND_HALF_EVEN))
            {
                internalConflict++;
            }
        }
    }

#ifdef DEBUG_VERBOSE_ON
    for (int i = 0; i < 3; i++)
    {
        if (opndDcls[i])
        {
            printf("%s, %s\n", opndDcls[i]->getName(), dcls[i]->getBankConflict() > 2 ?
                (dcls[i]->getBankConflict() == BANK_CONFLICT_SECOND_HALF_EVEN ? "HIGH_EVEN" : "HIGH_ODD") :
                dcls[i]->getBankConflict() > 0 ?
                (dcls[i]->getBankConflict() == BANK_CONFLICT_FIRST_HALF_EVEN ? "LOW_EVEN" : "LOW_ODD") : "NONE");
        }
    }
    printf("Bank1 number: %d; Bank2 number: %d\n", bank1RegNum, bank2RegNum);
#endif

    return;
}

void BankConflictPass::getBanks(G4_INST* inst, BankConflict *srcBC, G4_Declare **dcls, G4_Declare **opndDcls, unsigned int *offset)
{
    for (int i = 0; i < 3; i++)
    {
        dcls[i] = nullptr;
        opndDcls[i] = nullptr;
        srcBC[i] = BANK_CONFLICT_NONE;

        G4_Operand* src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion() || src->isAccReg())
        {
            return;
        }

        dcls[i] = GetTopDclFromRegRegion(src);
        if (!dcls[i])
        {
            continue;
        }
        opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

        offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) / G4_GRF_REG_NBYTES;
        srcBC[i] = gra.getBankConflict(dcls[i]);

        if (src->getBase()->asRegVar()->isPhyRegAssigned())
        {
            unsigned int reg = src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
            srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
        else if (srcBC[i] != BANK_CONFLICT_NONE)
        {
            if (offset[i] % 2)
            {
                if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                {
                    srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
                }
                else
                {
                    srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
                }
            }
        }
    }

    return;
}

void BankConflictPass::getPrevBanks(G4_INST* inst, BankConflict *srcBC, G4_Declare **dcls, G4_Declare **opndDcls, unsigned int *offset)
{
    int execSize[G4_MAX_SRCS];

    for (int i = 1; i < 3; i++)
    {
        dcls[i] = nullptr;
        opndDcls[i] = nullptr;
        srcBC[i] = BANK_CONFLICT_NONE;

        G4_Operand* src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion())
        {
            return;
        }
        dcls[i] = GetTopDclFromRegRegion(src);
        if (dcls[i]->getRegFile() != G4_GRF)
        {
            return;
        }
        execSize[i] = src->getLinearizedEnd() - src->getLinearizedStart() + 1;

        opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

        offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) / G4_GRF_REG_NBYTES;
        srcBC[i] = gra.getBankConflict(dcls[i]);

        if (src->getBase()->asRegVar()->isPhyRegAssigned())
        {
            unsigned int reg = src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
            srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
        else if (srcBC[i] != BANK_CONFLICT_NONE)
        {
            if (offset[i] % 2)
            {
                if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                {
                    srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
                }
                else
                {
                    srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
                }
            }
        }
        if (execSize[i] > 32)
        {
            srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
    }

    return;
}



void BankConflictPass::setupBankForSrc0(G4_INST* inst, G4_INST* prevInst)
{
    BankConflict srcBC[3];
    G4_Declare * dcls[3];
    G4_Declare * opndDcls[3];
    unsigned int offset[3];

    BankConflict prevSrcBC[3];
    G4_Declare * prevDcls[3];
    G4_Declare * prevOpndDcls[3];
    unsigned int prevOffset[3];

    if (prevInst->isSend() ||
        prevInst->isMath())
    {
        return;
    }

    getBanks(inst, srcBC, dcls, opndDcls, offset);
    getPrevBanks(prevInst, prevSrcBC, prevDcls, prevOpndDcls, prevOffset);

    if (dcls[0] != nullptr &&
        srcBC[0] == BANK_CONFLICT_NONE &&
        prevSrcBC[1] != BANK_CONFLICT_NONE &&
        prevSrcBC[2] != BANK_CONFLICT_NONE)
    {
        if (prevSrcBC[1] == prevSrcBC[2])
        {
            if (prevSrcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN)
            {
                srcBC[0] = offset[0] % 2 ? BANK_CONFLICT_FIRST_HALF_EVEN : BANK_CONFLICT_SECOND_HALF_ODD;
            }
            else
            {
                srcBC[0] = offset[0] % 2 ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            }

            gra.setBankConflict(dcls[0], srcBC[0]);
        }
    }

    return;
}

void BankConflictPass::setupBankConflictsforTwoGRFs(G4_INST* inst)
{
    BankConflict srcBC[3];
    unsigned int regNum[3];
    unsigned int refNum[3];
    unsigned int offset[3];
    G4_Declare * dcls[3];
    G4_Declare * opndDcls[3];
    int bank_num = 0;
    int execSize[3];

    for (int i = 0; i < 3; i++)
    {
        dcls[i] = nullptr;
        opndDcls[i] = nullptr;
        execSize[i] = 0;

        G4_Operand* src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion() || src->isAccReg())
        {
            // bank conflict not possible
            return;
        }
        execSize[i] = src->getLinearizedEnd() - src->getLinearizedStart() + 1;

        dcls[i] = GetTopDclFromRegRegion(src);
        opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

        regNum[i] = dcls[i]->getNumRows();
        refNum[i] = gra.getNumRefs(dcls[i]);
        offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) / G4_GRF_REG_NBYTES;
        srcBC[i] = gra.getBankConflict(dcls[i]);

        if (src->getBase()->asRegVar()->isPhyRegAssigned())
        {
            unsigned int reg = src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
            srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            gra.setBankConflict(dcls[i], srcBC[i]);
        }
        else if (srcBC[i] != BANK_CONFLICT_NONE)
        {
            if (offset[i] % 2)
            {
                if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                {
                    srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
                }
                else
                {
                    srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
                }
            }
        }
        if (i != 0)
        {
            bank_num += srcBC[i];
        }
    }

    int simd8SrcNum = 0;
    for (int i = 0; i < 3; i++)
    {
        if (execSize[i] <= 32)
        {
            simd8SrcNum++;
        }
    }

    //In case (src0) src1 and src2 use same declare, i.e. use same regsiter
    if ((dcls[0] == dcls[1]) && (dcls[1] == dcls[2]))
    {
        return;
    }

    //No bank assigned to src operands,
    //assign the two delcares into different bundles/banks.
    if (simd8SrcNum <= 1)  //All simd16, do even align
    {
        for (int i = 0; i < 3; i++)
        {
            if (execSize[i] > 32)
            {
                srcBC[i] = offset[i] % 2 ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
                gra.setBankConflict(dcls[i], srcBC[i]);
            }
        }
    }
    else if (bank_num == 0)
    {
        unsigned int index[3];

        refNumBasedSort(refNum, index);

        if (dcls[index[0]] != dcls[index[1]])
        {
            setupEvenOddBankConflictsForDecls(dcls[index[0]], dcls[index[1]],
                offset[index[0]], offset[index[1]],
                srcBC[index[0]], srcBC[index[1]]);
        }
    }
    else
    {
        if (srcBC[1] != BANK_CONFLICT_NONE)
        {
            srcBC[2] = (srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            if (offset[2] % 2)
            {
                srcBC[2] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            }
            gra.setBankConflict(dcls[2], srcBC[2]);
        }
        else
        {
            srcBC[1] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            if (offset[1] % 2)
            {
                srcBC[1] = (srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) ? BANK_CONFLICT_SECOND_HALF_ODD : BANK_CONFLICT_FIRST_HALF_EVEN;
            }
            gra.setBankConflict(dcls[1], srcBC[1]);
        }
    }

#ifdef DEBUG_VERBOSE_ON
    for (int i = 0; i < 3; i++)
    {
        if (opndDcls[i])
        {
            printf("%s, %s\n", opndDcls[i]->getName(), dcls[i]->getBankConflict() > 2 ?
                (dcls[i]->getBankConflict() == BANK_CONFLICT_SECOND_HALF_EVEN ? "HIGH_EVEN" : "HIGH_ODD") :
                dcls[i]->getBankConflict() > 0 ?
                (dcls[i]->getBankConflict() == BANK_CONFLICT_FIRST_HALF_EVEN ? "LOW_EVEN" : "LOW_ODD") : "NONE");
        }
    }
    printf("Bank1 number: %d; Bank2 number: %d\n", bank1RegNum, bank2RegNum);
#endif

    return;
}


void BankConflictPass::setupBankConflictsForBB(G4_BB* bb,
    unsigned int &threeSourceInstNum,
    unsigned int &sendInstNum,
    unsigned int numRegLRA,
    unsigned int & internalConflict)
{
    int bank1RegNum = 0;
    int bank2RegNum = 0;
    float GRFRatio = 0;
    G4_INST* prevInst = nullptr;

    if (numRegLRA)
    {
        GRFRatio = ((float)(numRegLRA - SECOND_HALF_BANK_START_GRF)) / SECOND_HALF_BANK_START_GRF;
    }

    for (auto i = bb->rbegin(), rend = bb->rend();
        i != rend;
        i++)
    {
        G4_INST* inst = (*i);
        if (inst->getNumSrc() == 3 && !inst->isSend())
        {
            threeSourceInstNum++;
            if (gra.kernel.fg.builder->lowHighBundle())
            {
                setupBankConflictsOneGRFOld(inst, bank1RegNum, bank2RegNum, GRFRatio, internalConflict);
            }
            else
            {
                    setupBankConflictsforTwoGRFs(inst);
            }

        }
        if (inst->isSend() && !inst->isEOT())
        {
            //Why only data port read causes issue?
            if (inst->getMsgDesc()->isDataPortRead())
            {
                sendInstNum++;
            }
        }
    }

    if ((float)threeSourceInstNum / bb->size() > 0.1)
    {
        if (!gra.kernel.fg.builder->lowHighBundle())
        {
            for (std::list<G4_INST*>::iterator i = bb->begin();
                i != bb->end();
                i++)
            {
                G4_INST* inst = (*i);
                if (prevInst && inst->getNumSrc() == 3 && !inst->isSend())
                {
                    setupBankForSrc0(inst, prevInst);
                }
                prevInst = inst;
            }
        }
    }
}

//Use for BB sorting according to the loop nest level and the BB size.
bool compareBBLoopLevel(G4_BB* bb1, G4_BB* bb2)
{
    if (bb1->getNestLevel() > bb2->getNestLevel())
    {
        return true;
    }
    else if (bb1->getNestLevel() == bb2->getNestLevel())
    {
        return bb1->size() > bb2->size();
    }

    return false;
}

/*
 * output:
 *        threeSourceCandidate, if there are enough three source instructions
 *        return value, if do bank confliction reduction to RR RA.
 */
bool BankConflictPass::setupBankConflictsForKernel(bool doLocalRR, bool &threeSourceCandidate, unsigned int numRegLRA, bool &highInternalConflict)
{
    BB_LIST orderedBBs;
    unsigned int threeSourceInstNumInKernel = 0;
    unsigned int internalConflict = 0;
    unsigned int instNumInKernel = 0;
    unsigned int sendInstNumInKernel = 0;

    for (auto curBB : gra.kernel.fg)
    {
        orderedBBs.push_back(curBB);
    }
    orderedBBs.sort(compareBBLoopLevel);

    for (auto bb : orderedBBs)
    {
        unsigned int instNum = 0;
        unsigned int sendInstNum = 0;
        unsigned int threeSourceInstNum = 0;
        unsigned int conflicts = 0;

        unsigned int loopNestLevel = 0;

        setupBankConflictsForBB(bb, threeSourceInstNum, sendInstNum, numRegLRA, conflicts);
        loopNestLevel = bb->getNestLevel() + 1;

        if (threeSourceInstNum)
        {
            instNum = (uint32_t)bb->size() * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
            threeSourceInstNum = threeSourceInstNum * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
            sendInstNum = sendInstNum * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
            conflicts = conflicts * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
            internalConflict += conflicts;
            threeSourceInstNumInKernel += threeSourceInstNum;
            instNumInKernel += instNum;
            sendInstNumInKernel += sendInstNum;
        }
    }

    if (!threeSourceInstNumInKernel ||
        (float)threeSourceInstNumInKernel / instNumInKernel < BANK_CONFLICT_HEURISTIC_INST)
    {
        return false;
    }

    highInternalConflict = ((float)internalConflict / threeSourceInstNumInKernel) > INTERNAL_CONFLICT_RATIO_HEURISTIC;

    //Bank conflict reduction is done only when there is enough three source instructions.
    threeSourceCandidate = true;

    if (doLocalRR && sendInstNumInKernel)
    {
        if (
            (sendInstNumInKernel > threeSourceInstNumInKernel))
        {
            return false;
        }
    }

    return true;
}

void GlobalRA::emitFGWithLiveness(LivenessAnalysis& liveAnalysis)
{
    for (BB_LIST_ITER it = kernel.fg.begin();
        it != kernel.fg.end();
        it++)
    {

        DEBUG_VERBOSE(std::endl << "-----------------------------------------------------------------");
        DEBUG_VERBOSE(std::endl << "BB" << (*it)->getId() << ":");
        DEBUG_VERBOSE(std::endl << "Preds: ");
        for (BB_LIST_ITER pred_it = (*it)->Preds.begin();
            pred_it != (*it)->Preds.end();
            pred_it++)
        {
            DEBUG_VERBOSE("BB" << (*pred_it)->getId() << ", ");
        }

        DEBUG_VERBOSE(std::endl << "Succs: ");
        for (BB_LIST_ITER succ_it = (*it)->Succs.begin();
            succ_it != (*it)->Succs.end();
            succ_it++)
        {
            DEBUG_VERBOSE("BB" << (*succ_it)->getId() << ", ");
        }

        if (kernel.getOption(vISA_LocalRA))
        {
            if (auto summary = kernel.fg.getBBLRASummary(*it))
            {
                DEBUG_VERBOSE(std::endl << "Local RA: ");
                {
                    for (unsigned int i = 0; i < kernel.getNumRegTotal(); i++)
                    {
                        if (summary->isGRFBusy(i))
                        {
                            DEBUG_VERBOSE("r" << i << ", ");
                        }
                    }
                }
            }
        }

        DEBUG_VERBOSE(std::endl << "Gen: ");
        for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
            dcl_it != kernel.Declares.end();
            dcl_it++)
        {
            if ((*dcl_it)->getAliasDeclare() != NULL)
                continue;

            if ((*dcl_it)->getRegVar()->isRegAllocPartaker())
            {
                if (liveAnalysis.use_gen[(*it)->getId()].isSet((*dcl_it)->getRegVar()->getId()))
                {
                    DEBUG_VERBOSE((*dcl_it)->getName() << ", ");
                }
            }
        }

        DEBUG_VERBOSE(std::endl << "Kill: ");
        for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
            dcl_it != kernel.Declares.end();
            dcl_it++)
        {
            if ((*dcl_it)->getAliasDeclare() != NULL)
                continue;

            if ((*dcl_it)->getRegVar()->isRegAllocPartaker())
            {
                if (liveAnalysis.use_kill[(*it)->getId()].isSet((*dcl_it)->getRegVar()->getId()))
                {
                    DEBUG_VERBOSE((*dcl_it)->getName() << ", ");
                }
            }
        }

        DEBUG_VERBOSE(std::endl << "Live-in: ");
        for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
            dcl_it != kernel.Declares.end();
            dcl_it++)
        {
            if ((*dcl_it)->getAliasDeclare() != NULL)
                continue;

            if ((*dcl_it)->getRegVar()->isRegAllocPartaker())
            {
                if (liveAnalysis.isLiveAtEntry((*it), (*dcl_it)->getRegVar()->getId()))
                {
                    DEBUG_VERBOSE((*dcl_it)->getName() << ", ");
                }
            }
        }

        DEBUG_VERBOSE(std::endl << "Live-out: ");
        for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
            dcl_it != kernel.Declares.end();
            dcl_it++)
        {
            if ((*dcl_it)->getAliasDeclare() != NULL)
                continue;

            if ((*dcl_it)->getRegVar()->isRegAllocPartaker())
            {
                if (liveAnalysis.isLiveAtExit((*it), (*dcl_it)->getRegVar()->getId()))
                {
                    DEBUG_VERBOSE((*dcl_it)->getName() << ", ");
                }
            }
        }

        DEBUG_VERBOSE(std::endl);

#ifdef DEBUG_VERBOSE_ON
        (*it)->emit(COUT_ERROR);
#endif
    }
}

void GlobalRA::reportSpillInfo(LivenessAnalysis& liveness, GraphColor& coloring)
{
    // Emit out interference graph of each spill candidate
    // and if a spill candidate is a local range, emit its
    // start and end line number in file
    std::ofstream optreport;
    getOptReportStream(optreport, coloring.getOptions());
    LIVERANGE_LIST::const_iterator it = coloring.getSpilledLiveRanges().begin();
    LIVERANGE_LIST::const_iterator itEnd = coloring.getSpilledLiveRanges().end();
    LiveRange** lrs = coloring.getLiveRanges();

    for (; it != itEnd; ++it)
    {
        if ((*it)->getRegKind() == G4_GRF) {
            G4_RegVar* spillVar = (*it)->getVar();
            optreport << "Spill candidate " << spillVar->getName() << " intf:";
            optreport << "\t(" << spillVar->getDeclare()->getNumRows() << "x" <<
                spillVar->getDeclare()->getNumElems() << "):" <<
                G4_Type_Table[spillVar->getDeclare()->getElemType()].str << std::endl;

            if (getLocalLR(spillVar->getDeclare()) != NULL)
            {
                if (getLocalLR(spillVar->getDeclare())->isLiveRangeLocal())
                {
                    int start, end;
                    unsigned int dummy;
                    start = getLocalLR(spillVar->getDeclare())->getFirstRef(dummy)->getLineNo();
                    end = getLocalLR(spillVar->getDeclare())->getLastRef(dummy)->getLineNo();

                    optreport << "(Liverange is local starting at line #" << start <<
                        " and ending at line #" << end << ")" << std::endl;
                }
            }

            const Interference* intf = coloring.getIntf();
            unsigned int spillVarId = (*it)->getVar()->getId();

            for (int i = 0; i < (int)liveness.getNumSelectedVar(); i++)
            {
                if (intf->interfereBetween(spillVarId, i))
                {
                    G4_RegVar* intfRangeVar = lrs[i]->getVar();

                    optreport << "\t" << intfRangeVar->getName() << "(" <<
                        intfRangeVar->getDeclare()->getNumRows() << "x" <<
                        intfRangeVar->getDeclare()->getNumElems() << "):" <<
                        G4_Type_Table[intfRangeVar->getDeclare()->getElemType()].str;

                    if (lrs[i]->getPhyReg() == NULL)
                    {
                        optreport << " --- spilled";
                    }

                    optreport << ", " << std::endl;
                }
            }

            optreport << std::endl << std::endl;
        }
    }

    closeOptReportStream(optreport);
}


LiveRange::LiveRange(G4_RegVar* v, GlobalRA& g) : gra(g)
{
    isCandidate = true;
    var = v;
    dcl = v->getDeclare();
    regKind = dcl->getRegFile();

    if (getRegKind() == G4_ADDRESS)
        numRegNeeded = v->getDeclare()->getNumElems() * v->getDeclare()->getElemSize() / G4_WSIZE;
    else if (getRegKind() == G4_FLAG)
    {
        // number of elements are in words
        numRegNeeded = v->getDeclare()->getNumElems();
    }
    else
    {
        // number of GRFs
        numRegNeeded = v->getDeclare()->getNumRows();
    }
}

void LiveRange::checkForInfiniteSpillCost(G4_BB* bb, std::list<G4_INST*>::reverse_iterator& it)
{
    // G4_INST at *it defines liverange object (this ptr)
    // If next instruction of iterator uses same liverange then
    // it may be a potential infinite spill cost candidate.
    // To confirm, following requirements should be fulfilled:
    // a. this liverange is not a global
    // b. this liverange is defined/used in these 2 instructions only
    //
    // The idea is for ranges marked with infinite spill cost,
    // coloring will attempt to put them on top of stack so they
    // have higher chance of getting a color. If a range that should
    // be infinite spill cost is not marked as being so, the only
    // downside is extra compile time spent in inserting spill code
    // and then punting out when later spilled code will cause
    // even more spills.
    //
    // The assumption is that current live-range is a current register
    // allocation candidate.
    //
    G4_INST* curInst = (*it);

    // Skip the check if curInst is a pseudoKill
    // Otherwise, it may invalidate a previously marked infinite
    // spill cost candidate, e.g.,
    // pseudo_kill (1) P1(0,0)[1]:uw [Align1]
    // mov (1) P1(0,0)[1]:uw TV1(8,0)[0;1,0]:uw [Align1, NoMask]
    // (+P1.0) sel (16) V65(0,0)[1]:f TV0(0,0)[0;1,0]:f 0:f [Align1, H1]
    if (curInst->isPseudoKill())
    {
        return;
    }

    // Check whether dst variable is a global
    if (gra.isBlockLocal(this->getDcl()) == false)
    {
        isCandidate = false;
        isInfiniteCost = false;

        return;
    }

    G4_DstRegRegion* dst = curInst->getDst();
    // If cur instruction dst is indirect write then return
    if (dst &&
        dst->getRegAccess() == IndirGRF &&
        dst->getBase()->asRegVar()->getId() == this->getVar()->getId())
    {
        return;
    }

    // isCandidate is set to true only for first definition ever seen.
    // If more than 1 def if found this gets set to false.
    const std::list<G4_INST*>::reverse_iterator rbegin = bb->rbegin();
    if (this->isCandidate == true && it != rbegin)
    {
        G4_INST* nextInst = NULL;
        if (this->getRefCount() != 2 ||
            (this->getRegKind() == G4_GRF && this->getDcl()->getAddressed() == true))
        {
            // If a liverange has > 2 refs then it
            // cannot be a candidate.
            // Also an address taken GRF is not a candidate.
            // This represents an early exit.
            isCandidate = false;
            isInfiniteCost = false;

            return;
        }

        // Skip all pseudo kills
        std::list<G4_INST*>::reverse_iterator next = it;
        while (true)
        {
            if (next == rbegin)
            {
                isCandidate = isInfiniteCost = false;
                return;
            }
            --next;

            // This is not a pseudo-kill instruction, then find
            // the desired next instruction. Otherwise, continue.
            nextInst = *next;
            if (!(nextInst->isPseudoKill()))
                break;
        }

        // Check whether this liverange is used in nextInst
        for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
        {
            G4_Operand* src = nextInst->getSrc(i);

            if (src &&
                src->isSrcRegRegion() &&
                src->getBase()->isRegAllocPartaker())
            {
                // src can be Direct/Indirect
                G4_SrcRegRegion* srcRgn = src->asSrcRegRegion();

                if (srcRgn->getRegAccess() == Direct &&
                    srcRgn->getBase()->isRegVar() &&
                    srcRgn->getBase()->asRegVar()->getId() == this->getVar()->getId())
                {
                    // Def-use found back-to-back
                    isInfiniteCost = true;
                    // Identify no more candidates
                    isCandidate = false;
                }
                else if (this->getRegKind() == G4_ADDRESS &&
                    srcRgn->getRegAccess() == IndirGRF &&
                    srcRgn->getBase()->isRegVar() &&
                    srcRgn->getBase()->asRegVar()->getId() == this->getVar()->getId())
                {
                    // Def-use found back-to-back
                    isInfiniteCost = true;
                    // Identify no more candidates
                    isCandidate = false;
                }
            }
        }

        G4_DstRegRegion* nextDst = nextInst->getDst();
        if (isCandidate == true &&
            this->getRegKind() == G4_ADDRESS &&
            nextDst &&
            nextDst->getRegAccess() == IndirGRF &&
            nextDst->getBase()->isRegVar() &&
            nextDst->getBase()->asRegVar()->isRegAllocPartaker() &&
            nextDst->getBase()->asRegVar()->getId() == this->getVar()->getId())
        {
            // Pattern found:
            // A0=
            // r[A0]=
            isInfiniteCost = true;
            // Identify no more candidates
            isCandidate = false;
        }

        if (isCandidate == true &&
            this->getRegKind() == G4_FLAG &&
            nextInst->getPredicate() &&
            nextInst->getPredicate()->getBase() &&
            nextInst->getPredicate()->getBase()->isRegVar() &&
            nextInst->getPredicate()->getBase()->asRegVar()->isRegAllocPartaker() &&
            nextInst->getPredicate()->getBase()->asRegVar()->getId() == this->getVar()->getId())
        {
            // Pattern found:
            // P0 = or cmp.P0 = <-- P0 defined
            // (P0) ... <-- P0 used as predicate
            isInfiniteCost = true;
            // Identify no more candidates
            isCandidate = false;
        }

#ifdef DEBUG_VERBOSE_ON
        if (isInfiniteCost == true)
        {
            DEBUG_VERBOSE("Marking " << this->getDcl()->getName() <<
                " as having infinite spill cost due to back-to-back def-use" << std::endl);
        }
#endif

        // Once a def is seen, stop looking for more defs
        isCandidate = false;
    }
    else
    {
#ifdef DEBUG_VERBOSE_ON
        if (isInfiniteCost == true)
        {
            DEBUG_VERBOSE("Unmarking " << this->getDcl()->getName() <<
                " as having infinite spill cost" << std::endl);
        }
#endif
        isCandidate = false;
        isInfiniteCost = false;
    }
}

//
// return true, if live ranges v1 and v2 interfere
//
bool Interference::interfereBetween(unsigned v1, unsigned v2) const
{
    if (v1 > v2)
    {
        std::swap(v1, v2);
    }

    if (useDenseMatrix())
    {
        unsigned col = v2 / BITS_DWORD;
        return (matrix[v1 * getRowSize() + col] & BitMask[v2 - col * BITS_DWORD]) ? true : false;
    }
    else
    {
        auto&& set = sparseMatrix[v1];
        return set.find(v2) != set.end();
    }
}

//
// init live vector with all live ranges that are live at the exit
// also set the next seq use of any live range that is live across to be INT_MAX
// to indicate that this live range does not have exclusive sequential uses and hence
// is not a candidate for being marked with an infinite spill cost.
//
void Interference::buildInterferenceAtBBExit(G4_BB* bb, BitSet& live)
{

    // live must be empty at this point
    live = liveAnalysis->use_out[bb->getId()];
    live &= liveAnalysis->def_out[bb->getId()];
}

//
// Filter out partial or splitted declares in batch interference.
//
inline void Interference::filterSplitDclares(unsigned startIdx, unsigned endIdx, unsigned n, unsigned col, unsigned &elt, bool is_partial)
{

    if (is_partial)  //Don't interference with parent
    {
        unsigned rowSplited = n / BITS_DWORD;
        if (rowSplited == col)
        {
            elt &= ~(BitMask[n % BITS_DWORD]);
        }
    }

    //if current is splitted dcl, don't interference with  any of it's child nodes.
    //if current is partial dcl, don't intereference with any other child nodes.
    if (col >= startIdx / BITS_DWORD  && col < (endIdx / BITS_DWORD + 1))
    {
        unsigned selt = 0;
        unsigned start_id = col * BITS_DWORD > startIdx ? 0 : startIdx % BITS_DWORD;
        unsigned end_id = (col + 1) * BITS_DWORD > endIdx ? endIdx % BITS_DWORD : BITS_DWORD;

        for (unsigned int i = start_id; i < end_id; i++)
        {
            selt |= 1 << i;
        }
        elt &= ~selt;
    }

    return;
}

//
// set interference for all live ranges that are currently live
// for partial declares, following rules are applied
// a. current partial declare does not interference with any other partial declare
// b. current parent declare does not interference with it's children declares, can children declare interference with parent declare?
// c. current partial declare does not interference with hybrid declares added by local RA, the reason is simple, these declares are assigned register already.
//
void Interference::buildInterferenceWithLive(BitSet& live, unsigned i)
{
    bool is_partial = lrs[i]->getIsPartialDcl();
    bool is_splitted = lrs[i]->getIsSplittedDcl();
    unsigned numDwords = 0;
    unsigned numBits = 0;
    unsigned n = 0;

    // For none partial varaible, interference with all varaibles
    numDwords = maxId / BITS_DWORD;
    numBits = maxId % BITS_DWORD;

    if (numBits)
    {
        numDwords++;
    }

    unsigned start_idx = 0;
    unsigned end_idx = 0;
    if (is_splitted) //if current is splitted dcl, don't interference with all it's child nodes.
    {
        start_idx = lrs[i]->getDcl()->getSplitVarStartID();
        end_idx = lrs[i]->getDcl()->getSplitVarStartID() + gra.getSplitVarNum(lrs[i]->getDcl());
    }

    if (is_partial)   //if current is partial dcl, don't interference with all other partial dcls, and it's parent dcl.
    {
        n = gra.getSplittedDeclare(lrs[i]->getDcl())->getRegVar()->getId();
        start_idx = splitStartId;
        end_idx = splitStartId + splitNum;
    }

    unsigned colEnd = i / BITS_DWORD;

    // Set column bits in intf graph
    for (unsigned k = 0; k < colEnd; k++)
    {
        unsigned elt = live.getElt(k);

        if (elt != 0)
        {
            if (is_partial || is_splitted)
            {
                filterSplitDclares(start_idx, end_idx, n, k, elt, is_partial);
            }

            for (unsigned int j = 0; j < BITS_DWORD; j++)
            {
                if (elt & BitMask[j])
                {
                    unsigned curPos = j + (k*BITS_DWORD);
                    safeSetInterference(curPos, i);
                }
            }
        }
    }

    // Set dword at transition point from column to row
    unsigned elt = live.getElt(colEnd);
    //checkAndSetIntf gaurantee partial and splitted cases
    if (elt != 0)
    {
        for (unsigned int j = 0; j < BITS_DWORD; j++)
        {
            if (elt & BitMask[j])
            {
                unsigned curPos = j + (colEnd*BITS_DWORD);
                if (!varSplitCheckBeforeIntf(i, curPos))
                {
                    checkAndSetIntf(i, curPos);
                }
            }
        }
    }

    colEnd++;
    // Set row intf graph
    for (unsigned k = colEnd; k < numDwords; k++)
    {
        unsigned elt = live.getElt(k);

        if (is_partial || is_splitted)
        {
            filterSplitDclares(start_idx, end_idx, n, k, elt, is_partial);
        }

        if (elt != 0)
        {
            setBlockInterferencesOneWay(i, k, elt);
        }
    }
}

void Interference::buildInterferenceWithSubDcl(unsigned lr_id, G4_Operand *opnd, BitSet& live, bool setLive, bool setIntf)
{

    G4_Declare *dcl = lrs[lr_id]->getDcl();
    auto subDclSize = gra.getSubDclSize(dcl);
    for (unsigned i = 0; i < subDclSize; i++)
    {
        G4_Declare * subDcl = gra.getSubDcl(dcl, i);

        unsigned leftBound = gra.getSubOffset(subDcl);
        unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
        if (!(opnd->getRightBound() < leftBound || rightBound < opnd->getLeftBound()))
        {
            int subID = subDcl->getRegVar()->getId();

            if (setIntf)
            {
                buildInterferenceWithLive(live, subID);
            }
            if (setLive)
            {
                live.set(subID, true);
            }
        }
    }

    return;
}

void Interference::buildInterferenceWithAllSubDcl(unsigned v1, unsigned v2)
{
    G4_Declare * d1 = lrs[v1]->getDcl();
    G4_Declare * d2 = lrs[v2]->getDcl();

    if (d1->getIsSplittedDcl() && !d2->getIsPartialDcl())
    {
        auto d1SubDclSize = gra.getSubDclSize(d1);
        for (unsigned i = 0; i < d1SubDclSize; i++)
        {
            G4_Declare * subDcl = gra.getSubDcl(d1, i);
            int subID = subDcl->getRegVar()->getId();
            checkAndSetIntf(v2, subID);
        }
    }

    if (d2->getIsSplittedDcl() && !d1->getIsPartialDcl())
    {
        auto d2SubDclSize = gra.getSubDclSize(d2);
        for (unsigned i = 0; i < d2SubDclSize; i++)
        {
            G4_Declare * subDcl = gra.getSubDcl(d2, i);
            int subID = subDcl->getRegVar()->getId();
            checkAndSetIntf(v1, subID);
        }
    }

    return;
}
//
// Bias the live ranges in "live" to be assigned the callee-save registers as they
// are live through a stack call. Exclude file scope variables as they are always
// save/restore before/after call and are better assigned to the caller-save space.
//
void Interference::addCalleeSaveBias(BitSet& live)
{
    for (unsigned i = 0; i < maxId; i++)
    {
        if (live.isSet(i))
        {
            lrs[i]->setCallerSaveBias(false);
            lrs[i]->setCalleeSaveBias(true);
        }
    }
}

void Interference::buildInterferenceAmongLiveIns()
{
    //
    // Build interference between all live-ins. If all live-ins are only
    // read then their interference will be skipped in earlier phase.
    // For eg, arg and globals are both live-in. And both may only have
    // uses in function and no def.
    //
    G4_BB* entryBB = kernel.fg.getEntryBB();


    for (unsigned int i = 0; i < maxId; i++)
    {
        if (liveAnalysis->isLiveAtEntry(entryBB, i))
        {
            //Mark reference can not gaurantee all the varaibles are local, update here
            if (lrs[i]->getDcl()->getIsSplittedDcl())
            {
                lrs[i]->getDcl()->setIsSplittedDcl(false);
                lrs[i]->setIsSplittedDcl(false);
            }

            for (unsigned j = i + 1; j < maxId; j++)
            {
                if (liveAnalysis->isLiveAtEntry(entryBB, j))
                {
                    if (lrs[i]->getDcl()->getRegFile() == G4_INPUT &&
                        lrs[i]->getVar()->getPhyReg() != NULL &&
                        lrs[j]->getDcl()->getRegFile() == G4_INPUT &&
                        lrs[j]->getVar()->getPhyReg() != NULL)
                    {
                        continue;
                    }
                    else
                    {
                        if (!varSplitCheckBeforeIntf(i, j))
                        {
                            checkAndSetIntf(i, j);
                        }
                    }
                }
            }
        }
    }
}

void Interference::markInterferenceForSend(G4_BB* bb,
    G4_INST* inst,
    G4_DstRegRegion* dst)
{
    bool isDstRegAllocPartaker = false;
    bool isDstLocallyAssigned = false;
    unsigned dstId = 0;
    int dstPreg = 0, dstNumRows = 0;

    if (dst->getBase()->isRegVar())
    {
        if (dst->getBase()->isRegAllocPartaker())
        {
            G4_DstRegRegion* dstRgn = dst;
            isDstRegAllocPartaker = true;
            dstId = ((G4_RegVar*)dstRgn->getBase())->getId();
        }
        else if (kernel.getOption(vISA_LocalRA))
        {
            LocalLiveRange* localLR = NULL;
            G4_Declare* topdcl = GetTopDclFromRegRegion(dst);

            if (topdcl)
                localLR = gra.getLocalLR(topdcl);

            if (localLR && localLR->getAssigned())
            {
                int sreg;
                G4_VarBase* preg = localLR->getPhyReg(sreg);

                MUST_BE_TRUE(preg->isGreg(), "Register in dst was not GRF");

                isDstLocallyAssigned = true;
                dstPreg = preg->asGreg()->getRegNum();
                dstNumRows = localLR->getTopDcl()->getNumRows();
            }
        }

        if (isDstRegAllocPartaker || isDstLocallyAssigned)
        {
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* src = inst->getSrc(j);
                if (src != NULL &&
                    src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->getBase()->isRegVar())
                {
                    if (src->asSrcRegRegion()->getBase()->isRegAllocPartaker())
                    {
                        unsigned srcId = src->asSrcRegRegion()->getBase()->asRegVar()->getId();

                        if (isDstRegAllocPartaker)
                        {
                            if (!varSplitCheckBeforeIntf(dstId, srcId))
                            {
                                checkAndSetIntf(dstId, srcId);
                                buildInterferenceWithAllSubDcl(dstId, srcId);
                            }
                        }
                        else
                        {
                            for (int j = dstPreg; j < dstPreg + dstNumRows; j++)
                            {
                                int k = getGRFDclForHRA(j)->getRegVar()->getId();
                                if (!varSplitCheckBeforeIntf(k, srcId))
                                {
                                    checkAndSetIntf(k, srcId);
                                    buildInterferenceWithAllSubDcl(k, srcId);
                                }
                            }
                        }
                    }
                    else if (kernel.getOption(vISA_LocalRA) && isDstRegAllocPartaker)
                    {
                        LocalLiveRange* localLR = NULL;
                        G4_Declare* topdcl = GetTopDclFromRegRegion(src);

                        if (topdcl)
                            localLR = gra.getLocalLR(topdcl);

                        if (localLR && localLR->getAssigned())
                        {
                            int reg, sreg, numrows;
                            G4_VarBase* preg = localLR->getPhyReg(sreg);
                            numrows = localLR->getTopDcl()->getNumRows();

                            MUST_BE_TRUE(preg->isGreg(), "Register in src was not GRF");

                            reg = preg->asGreg()->getRegNum();

                            for (int j = reg; j < reg + numrows; j++)
                            {
                                int k = getGRFDclForHRA(j)->getRegVar()->getId();
                                if (!varSplitCheckBeforeIntf(dstId, k))
                                {
                                    checkAndSetIntf(dstId, k);
                                    buildInterferenceWithAllSubDcl(dstId, k);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

uint32_t GlobalRA::getRefCount(int loopNestLevel)
{
    if (loopNestLevel == 0)
    {
        return 1;
    }
    return (uint32_t)std::pow(IN_LOOP_REFERENCE_COUNT_FACTOR, std::min(loopNestLevel, 8));
}

// handle return value interference for fcall
void Interference::buildInterferenceForFcall(G4_BB* bb, BitSet& live, G4_INST* inst, std::list<G4_INST*>::reverse_iterator i, G4_VarBase* regVar)
{
    assert(inst->opcode() == G4_pseudo_fcall && "expect fcall inst");
    unsigned refCount = GlobalRA::getRefCount(kernel.getOption(vISA_ConsiderLoopInfoInRA) ?
        bb->getNestLevel() : 0);

    if (regVar->isRegAllocPartaker())
    {
        unsigned id = ((G4_RegVar*)regVar)->getId();
        lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount);

        buildInterferenceWithLive(live, id);
        updateLiveness(live, id, false);
    }
}

bool GlobalRA::isReRAPass()
{
    auto gtPinInfo = kernel.getGTPinData();
    bool reRAPass = gtPinInfo && gtPinInfo->isReRAPass();
    return reRAPass;
}

void Interference::buildInterferenceForDst(G4_BB* bb, BitSet& live, G4_INST* inst, std::list<G4_INST*>::reverse_iterator i, G4_DstRegRegion* dst)
{
    unsigned refCount = GlobalRA::getRefCount(kernel.getOption(vISA_ConsiderLoopInfoInRA) ?
        bb->getNestLevel() : 0);

    if (dst->getBase()->isRegAllocPartaker())
    {
        unsigned id = ((G4_RegVar*)dst->getBase())->getId();
        //
        // In following code,
        // pseudo_kill V10
        // mov (8) V10, V11
        //
        // V10 and V11 do not interfere and can be assigned
        // same register.
        //
        // Following condition skips marking interference for
        // pseudo_kill nodes.
        //
        if (inst->isPseudoKill() == false &&
            inst->isLifeTimeEnd() == false)
        {
            lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount);  // update reference count

            buildInterferenceWithLive(live, id);
            if (lrs[id]->getIsSplittedDcl())
            {
                buildInterferenceWithSubDcl(id, (G4_Operand *)dst, live, false, true);
            }
        }

        //
        // bias all variables that are live through stack calls to get assigned the
        // callee-save registers
        //
        if (kernel.fg.isPseudoVCADcl(lrs[id]->getDcl()))
        {
            addCalleeSaveBias(live);
        }
        //
        // if the write does not cover the whole dst region, we should continue let the
        // liveness propagate upwards
        //
        if (liveAnalysis->writeWholeRegion(bb, inst, dst, builder.getOptions()) ||
            inst->isPseudoKill())
        {
            updateLiveness(live, id, false);

            if (lrs[id]->getIsSplittedDcl())
            {
                for (unsigned i = lrs[id]->getDcl()->getSplitVarStartID();
                    i < lrs[id]->getDcl()->getSplitVarStartID() + gra.getSplitVarNum(lrs[id]->getDcl());
                    i++)
                {
                    live.set(i, false);  //kill all childs, there may be not used childs generated due to splitting, killed also.
                }
            }
        }

        // Indirect defs are actually uses of address reg
        lrs[id]->checkForInfiniteSpillCost(bb, i);
    }
    else if (dst->isIndirect() && liveAnalysis->livenessClass(G4_GRF))
    {
        //
        // add interferences to the list of potential indirect destination accesses.
        //
        auto pointsToSet = liveAnalysis->getPointsToAnalysis().getAllInPointsTo(dst->getBase()->asRegVar());
        if (pointsToSet == nullptr)
        {
            pointsToSet = liveAnalysis->getPointsToAnalysis().getIndrUseVectorPtrForBB(bb->getId());
        }
        for (auto var : *pointsToSet)
        {
            if (var->isRegAllocPartaker())
            {
                buildInterferenceWithLive(live, var->getId());
            }
        }
    }
}


void Interference::buildInterferenceWithinBB(G4_BB* bb, BitSet& live)
{
    DebugInfoState state(kernel.fg.mem);
    unsigned refCount = GlobalRA::getRefCount(kernel.getOption(vISA_ConsiderLoopInfoInRA) ?
        bb->getNestLevel() : 0);

    for (auto i = bb->rbegin(); i != bb->rend(); i++)
    {
        G4_INST* inst = (*i);

        G4_DstRegRegion* dst = inst->getDst();
        if (dst)
        {
            buildInterferenceForDst(bb, live, inst, i, dst);
        }

        if (inst->opcode() == G4_pseudo_fcall)
        {
            if (liveAnalysis->livenessClass(G4_GRF))
            {
                G4_FCALL* fcall = kernel.fg.builder->getFcallInfo(bb->back());
                G4_Declare* arg = kernel.fg.builder->getStackCallArg();
                G4_Declare* ret = kernel.fg.builder->getStackCallRet();
                MUST_BE_TRUE(fcall != NULL, "fcall info not found");
                uint16_t retSize = fcall->getRetSize();
                uint16_t argSize = fcall->getArgSize();
                if (ret && retSize > 0 && ret->getRegVar())
                {
                    buildInterferenceForFcall(bb, live, inst, i, ret->getRegVar());
                }
                if (arg && argSize > 0 && arg->getRegVar())
                {
                    auto id = arg->getRegVar()->getId();
                    updateLiveness(live, id, true);
                }
            }
            else if (liveAnalysis->livenessClass(G4_ADDRESS))
            {
                // assume callee will use A0
                buildInterferenceWithLive(live, inst->asCFInst()->getAssocPseudoA0Save()->getId());
            }
            else if (liveAnalysis->livenessClass(G4_FLAG))
            {
                // assume callee will use both F0 and F1
                buildInterferenceWithLive(live, inst->asCFInst()->getAssocPseudoFlagSave()->getId());
            }
        }

        if (inst->isSend() && !dst->isNullReg())
        {
            if (VISA_WA_CHECK(kernel.fg.builder->getPWaTable(), WaDisableSendSrcDstOverlap))
            {
                markInterferenceForSend(bb, inst, dst);
            }

            // FIXME: revisit this restriction.
            //r127 must not be used for return address when there is a src and dest overlap in send instruction.
            if (kernel.fg.builder->needsToReserveR127() && liveAnalysis->livenessClass(G4_GRF) && !inst->isSplitSend())
            {
                if (dst->getBase()->isRegAllocPartaker() && !dst->getBase()->asRegVar()->isPhyRegAssigned())
                {
                    int dstId = dst->getBase()->asRegVar()->getId();
                    lrs[dstId]->markForbidden(kernel.getNumRegTotal() - 1, 1);
                }
            }
        }

        if (inst->isSplitSend() && !inst->getSrc(1)->isNullReg())
        {
            G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();
            G4_SrcRegRegion* src1 = inst->getSrc(1)->asSrcRegRegion();

            if (src0->getBase()->isRegAllocPartaker() && src1->getBase()->isRegAllocPartaker())
            {
                // src0 and src1 of split send may not overlap. In normal cases this is handled automatically
                // as we add interference edge when we reach src0/src1's def.  If one source is an
                // undefined variable (this can happen for URB write payload) and the other an input, however,
                // we could miss the interference edge between the two.  So we add it explicitly here
                int src0Id = src0->getBase()->asRegVar()->getId();
                int src1Id = src1->getBase()->asRegVar()->getId();

                checkAndSetIntf(src0Id, src1Id);
                buildInterferenceWithAllSubDcl(src0Id, src1Id);
            }
        }


        //
        // process each source operand
        //
        for (unsigned j = 0; j < G4_MAX_SRCS; j++)
        {
            G4_Operand* src = inst->getSrc(j);
            if (src == NULL)
            {
                continue;
            }
            if (src->isSrcRegRegion())
            {
                G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
                if (srcRegion->getBase()->isRegAllocPartaker())
                {
                    unsigned id = ((G4_RegVar*)(srcRegion)->getBase())->getId();
                    lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount); // update reference count

                    if (inst->opcode() != G4_pseudo_lifetime_end)
                    {
                        updateLiveness(live, id, true);
                        if (lrs[id]->getIsSplittedDcl())
                        {
                            buildInterferenceWithSubDcl(id, src, live, true, false);
                        }
                    }

                    if (inst->isEOT() && liveAnalysis->livenessClass(G4_GRF))
                    {
                        //mark the liveRange as the EOT source
                        lrs[id]->setEOTSrc();
                        if (builder.hasEOTGRFBinding())
                        {
                            lrs[id]->markForbidden(0, kernel.getNumRegTotal() - 16);
                        }
                    }

                    if (inst->isReturn())
                    {
                        lrs[id]->setRetIp();
                    }
                }
                else if (srcRegion->isIndirect() && liveAnalysis->livenessClass(G4_GRF))
                {
                    // make every var in points-to set live
                    PointsToAnalysis& pta = liveAnalysis->getPointsToAnalysis();
                    auto pointsToSet = pta.getAllInPointsTo(srcRegion->getBase()->asRegVar());
                    if (pointsToSet == nullptr)
                    {
                        // this can happen if the address is coming from addr spill
                        // ToDo: we can avoid this by linking the spilled addr with its new temp addr
                        pointsToSet = pta.getIndrUseVectorPtrForBB(bb->getId());
                    }
                    for (auto var : *pointsToSet)
                    {
                        if (var->isRegAllocPartaker())
                        {
                            updateLiveness(live, var->getId(), true);
                        }
                    }
                }
            }
        }

        //
        // Process register-indirect destination uses of ARF.
        //
        if (dst) {
            if (dst->getBase()->isRegAllocPartaker() &&
                dst->getRegAccess() != Direct) {
                live.set(dst->getBase()->asRegVar()->getId(), true);
            }
        }

        //
        // Process condMod
        //
        G4_CondMod* mod = inst->getCondMod();
        if (mod != NULL) {
            G4_VarBase *flagReg = mod->getBase();
            if (flagReg != NULL)
            {
                unsigned id = flagReg->asRegVar()->getId();
                if (flagReg->asRegVar()->isRegAllocPartaker())
                {
                    lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount); // update reference count
                    buildInterferenceWithLive(live, id);

                    if (LivenessAnalysis::writeWholeRegion(bb, inst, flagReg, builder.getOptions()))
                    {
                        updateLiveness(live, id, false);
                    }

                    lrs[id]->checkForInfiniteSpillCost(bb, i);
                }
            }
            else
            {
                MUST_BE_TRUE((inst->opcode() == G4_sel ||
                    inst->opcode() == G4_csel) &&
                    inst->getCondMod() != NULL,
                    "Invalid CondMod");
            }
        }

        //
        // Process predicate
        //
        G4_Predicate* predicate = inst->getPredicate();
        if (predicate != NULL) {
            G4_VarBase *flagReg = predicate->getBase();
            unsigned id = flagReg->asRegVar()->getId();
            if (flagReg->asRegVar()->isRegAllocPartaker())
            {
                lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount); // update reference count
                live.set(id, true);
            }
        }

        // Update debug info intervals based on live set
        if (builder.getOption(vISA_GenerateDebugInfo))
        {
            updateDebugInfo(kernel, inst, *liveAnalysis, lrs, live, &state, inst == bb->front());
        }
    }
}

void Interference::computeInterference()
{

    startTimer(TIMER_INTERFERENCE);
    //
    // create bool vector, live, to track live ranges that are currently live
    //
    BitSet live(maxId, false);

    for (BB_LIST_ITER it = kernel.fg.begin(); it != kernel.fg.end(); it++)
    {
        //
        // mark all live ranges dead
        //
        live.clear();
        //
        // start with all live ranges that are live at the exit of BB
        //
        buildInterferenceAtBBExit((*it), live);
        //
        // traverse inst in the reverse order
        //

        buildInterferenceWithinBB((*it), live);
    }

    if (kernel.getOptions()->getTarget() != VISA_3D ||
        kernel.fg.builder->getOption(vISA_enablePreemption) ||
        kernel.fg.getHasStackCalls() ||
        kernel.fg.getIsStackCallFunc())
    {
        buildInterferenceAmongLiveIns();
    }

    //
    // Build interference with physical registers assigned by local RA
    //
    if (kernel.getOption(vISA_LocalRA))
    {
        for (auto curBB : kernel.fg)
        {
            buildInterferenceWithLocalRA(curBB);
        }
    }

    if (builder.getOption(vISA_RATrace))
    {
        RPE rpe(gra, liveAnalysis);
        rpe.run();
        std::cout << "\t--max RP: " << rpe.getMaxRP() << "\n";
    }

    // Augment interference graph to accomodate non-default masks
    Augmentation aug(kernel, *this, *liveAnalysis, lrs, gra);
    aug.augmentIntfGraph();

    generateSparseIntfGraph();
}

#define SPARSE_INTF_VEC_SIZE 64

void Interference::generateSparseIntfGraph()
{
    // Generate sparse intf graph from the dense one
    unsigned int numVars = liveAnalysis->getNumSelectedVar();

    sparseIntf.resize(numVars);

    for (unsigned int row = 0; row < numVars; row++)
    {
        sparseIntf[row].reserve(SPARSE_INTF_VEC_SIZE);
    }

    if (useDenseMatrix())
    {
        // Iterate over intf graph matrix
        for (unsigned int row = 0; row < numVars; row++)
        {
            unsigned int rowOffset = row*getRowSize();
            unsigned int colStart = (row + 1) / BITS_DWORD;
            for (unsigned int j = colStart; j < getRowSize(); j++)
            {
                unsigned int intfBlk = getInterferenceBlk(rowOffset + j);
                if (intfBlk != 0)
                {
                    for (unsigned k = 0; k < BITS_DWORD; k++)
                    {
                        if (intfBlk & BitMask[k])
                        {
                            unsigned int v2 = (j*BITS_DWORD) + k;
                            if (v2 != row)
                            {
                                sparseIntf[v2].push_back(row);
                                sparseIntf[row].push_back(v2);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (uint32_t v1 = 0; v1 < maxId; ++v1)
        {
            auto&& intfSet = sparseMatrix[v1];
            for (uint32_t v2 : intfSet)
            {
                sparseIntf[v1].push_back(v2);
                sparseIntf[v2].push_back(v1);
            }
        }
    }

    if (builder.getOption(vISA_RATrace))
    {
        uint32_t numNeighbor = 0;
        uint32_t maxNeighbor = 0;
        uint32_t maxIndex = 0;
        for (int i = 0, numVar = (int) sparseIntf.size(); i < numVar; ++i)
        {
            if (lrs[i]->getPhyReg() == nullptr)
            {
                auto intf = sparseIntf[i];
                numNeighbor += (uint32_t)intf.size();
                maxNeighbor = std::max(maxNeighbor, (uint32_t)intf.size());
                if (maxNeighbor == (uint32_t)intf.size())
                {
                    maxIndex = i;
                }
            }
        }
        float avgNeighbor = ((float)numNeighbor) / sparseIntf.size();
        std::cout << "\t--avg # neighbors: " << std::setprecision(6) << avgNeighbor << "\n";
        std::cout << "\t--max # neighbors: " << maxNeighbor << " (" << lrs[maxIndex]->getDcl()->getName() << ")\n";
    }

    stopTimer(TIMER_INTERFERENCE);
}

// This function can be invoked before local RA or after augmentation.
// This function will update sub-reg data only for non-NoMask vars and
// leave others unchanged, ie their value will be as per HW conformity
// or earlier phase.
void GlobalRA::updateSubRegAlignment(G4_SubReg_Align subAlign)
{
    // Update alignment of all GRF declares to sub-align
    for (auto dcl : kernel.Declares)
    {
        if (dcl->getRegFile() & G4_GRF && !dcl->getIsPartialDcl())
        {
            G4_Declare* topdcl = dcl->getRootDeclare();

            if (!areAllDefsNoMask(topdcl) &&
                getAugmentationMask(topdcl) != AugmentationMasks::NonDefault)
            {
                dcl->setSubRegAlign(subAlign);
                setSubRegAlign(dcl, subAlign);
            }
        }
    }
}

// This function can be invoked before local RA or after augmentation.
// When invoked before local RA, it sets all vars to be Even aligned,
// including NoMask ones. This is safe, but conservative. Post
// augmentation, dcl masks are available so only non-NoMask vars will
// be Even aligned. Others will be Either aligned. There is no need
// to store old value of align because HW has no restriction on
// even/odd alignment that HW conformity computes.
void GlobalRA::evenAlign()
{
    // Update alignment of all GRF declares to align
    for (auto dcl : kernel.Declares)
    {
        if (dcl->getRegFile() & G4_GRF)
        {
            G4_Declare* topdcl = dcl->getRootDeclare();
            auto topdclAugMask = getAugmentationMask(topdcl);

            if (!areAllDefsNoMask(topdcl) && !topdcl->getIsPartialDcl() &&
                topdclAugMask != AugmentationMasks::NonDefault &&
                topdclAugMask != AugmentationMasks::Default64Bit)
            {
                if ((topdcl->getElemSize() >= 4 || topdclAugMask == AugmentationMasks::Default32Bit) &&
                    topdcl->getByteSize() >= GENX_GRF_REG_SIZ &&
                    !(kernel.fg.builder->getOption(vISA_enablePreemption) &&
                        dcl == kernel.fg.builder->getBuiltinR0()))
                {
                    setEvenAligned(dcl, true);
                }
            }
        }
    }
}

void GlobalRA::getBankAlignment(LiveRange* lr, BankAlign &align)
{
    G4_Declare *dcl = lr->getDcl();
    if (kernel.getSimdSize() < 16)
    {
        return;
    }

    if (dcl->getRegFile() & G4_GRF)
    {
        G4_Declare* topdcl = dcl->getRootDeclare();
        auto topdclBC = getBankConflict(topdcl);

        if (topdclBC != BANK_CONFLICT_NONE)
        {
            if (topdcl->getElemSize() >= 4 &&
                topdcl->getNumRows() > 1 &&
                !(kernel.fg.builder->getOption(vISA_enablePreemption) &&
                    dcl == kernel.fg.builder->getBuiltinR0()))
            {
                if (topdclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                    topdclBC == BANK_CONFLICT_SECOND_HALF_ODD)
                {
                    align = BankAlign::Odd;
                }
            }
        }
    }
}

Augmentation::Augmentation(G4_Kernel& k, Interference& i, LivenessAnalysis& l, LiveRange* ranges[], GlobalRA& g) :
    intf(i), kernel(k), gra(g), liveAnalysis(l), fcallRetMap(g.fcallRetMap), m(kernel.fg.mem)
{
    lrs = ranges;
}

// For Scatter read, the channel is not handled as the block read.
// Update the emask according to the definition of VISA
bool Augmentation::updateDstMaskForScatter(G4_INST* inst, unsigned char* mask)
{

    G4_SendMsgDescriptor *msgDesc = inst->getMsgDesc();
    unsigned char execSize = inst->getExecSize();
    G4_DstRegRegion* dst = inst->getDst();
    unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
    unsigned short elemSize = dst->getElemSize();

    if (inst->isWriteEnableInst())
    {
        curEMBit = NOMASK_BYTE;
    }

    SFID funcID = msgDesc->getFuncId();

    switch (funcID)
    {
    case SFID::DP_DC1:
        switch (msgDesc->getHdcMessageType())
        {
        case DC1_A64_SCATTERED_READ:   //a64 scattered read: svm_gather
        {
            unsigned int blockNum = msgDesc->getBlockNum();
            unsigned int blockSize = msgDesc->getBlockSize();

            for (unsigned int i = 0; i < execSize; i++)
            {
                for (unsigned int j = 0; j < blockNum; j++)
                {
                    for (unsigned int k = 0; k < blockSize; k++)
                    {
                        mask[(j * execSize + i) * blockSize + k] = curEMBit;
                    }
                }
                if (curEMBit != NOMASK_BYTE)
                {
                    curEMBit++;
                    ASSERT_USER(curEMBit <= 32, "Illegal mask channel");
                }
            }
            return true;
        }
        break;

        case DC1_A64_UNTYPED_SURFACE_READ:  //SVM gather 4
        case DC1_UNTYPED_SURFACE_READ:   //VISA gather 4
        case DC1_TYPED_SURFACE_READ:   //Gather 4 typed
        {
            unsigned int channelNum = msgDesc->getEnabledChannelNum();
            if (channelNum == 0)
            {
                return false;
            }
            if (elemSize < 4)
            {
                elemSize = 4;
            }

            for (unsigned int i = 0; i < channelNum; i++)
            {
                for (unsigned int j = 0; j < execSize; j++)
                {
                    for (unsigned int k = 0; k < elemSize; k++)
                    {
                        mask[(i * execSize + j)*elemSize + k] = curEMBit;
                    }
                    if (curEMBit != NOMASK_BYTE)
                    {
                        curEMBit++;
                        ASSERT_USER(curEMBit <= 32, "Illegal mask channel");
                    }
                }
                curEMBit = (unsigned char)inst->getMaskOffset();
            }
            return true;
        }
        break;

        default: return false;
        }
        break;
    case SFID::DP_DC2:
        switch (msgDesc->getHdcMessageType())
        {
        case DC2_UNTYPED_SURFACE_READ:   //gather 4 scaled
        case DC2_A64_UNTYPED_SURFACE_READ: //SVM gather 4 scaled
        {
            unsigned int channelNum = msgDesc->getEnabledChannelNum();
            if (channelNum == 0)
            {
                return false;
            }
            if (elemSize < 4)
            {
                elemSize = 4;
            }

            for (unsigned int i = 0; i < channelNum; i++)
            {
                for (unsigned int j = 0; j < execSize; j++)
                {
                    for (unsigned int k = 0; k < elemSize; k++)
                    {
                        mask[(i * execSize + j)*elemSize + k] = curEMBit;
                    }
                    if (curEMBit != NOMASK_BYTE)
                    {
                        curEMBit++;
                        ASSERT_USER(curEMBit <= 32, "Illegal mask channel");
                    }
                }
                curEMBit = (unsigned char)inst->getMaskOffset();
            }
            return true;
        }

        case DC2_BYTE_SCATTERED_READ:   //scaled byte scattered read: gather_scaled, handled as block read write
        default: return false;
        }
        break;
    case SFID::DP_DC:
        switch (msgDesc->getHdcMessageType())
        {
        case DC_DWORD_SCATTERED_READ:   //dword scattered read: gather(dword), handled as block read write
        case DC_BYTE_SCATTERED_READ:       //byte scattered read:   gather(byte), handled as block read write
        default: return false;
        }
        break;

    case SFID::SAMPLER:
    {
        unsigned int respLength = msgDesc->ResponseLength();
        unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
        elemSize = msgDesc->is16BitReturn() ? 2 : 4;
        unsigned int warpNum = respLength * G4_GRF_REG_NBYTES / (execSize * elemSize);
        if (inst->isWriteEnableInst())
        {
            curEMBit = NOMASK_BYTE;
        }
        for (unsigned int i = 0; i < warpNum; i++)
        {
            for (unsigned int j = 0; j < execSize; j++)
            {
                for (unsigned int k = 0; k < elemSize; k++)
                {
                    mask[(i * execSize + j)*elemSize + k] = curEMBit;
                }
                if (curEMBit != NOMASK_BYTE)
                {
                    curEMBit++;
                    ASSERT_USER(curEMBit <= 32, "Illegal mask channel");
                }
            }
            curEMBit = (unsigned char)inst->getMaskOffset();
        }
        return true;
    }
        
    break;

    default: return false;
    }

    return false;
}


// Value stored at each byte in mask determines which bits
// of EM enable that byte for writing. When checkCmodOnly
// is set dst is ignored and mask only for cmod is set. For
// flag declares, mask is at bit granularity rather than byte.
// Function updates mask field in declaration of correspoing
// variable - dst or cmod.
void Augmentation::updateDstMask(G4_INST* inst, bool checkCmodOnly)
{
    G4_DstRegRegion* dst = inst->getDst();
    G4_CondMod* cmod = inst->getCondMod();

    if ((checkCmodOnly == false && dst &&
        dst->getBase() &&
        dst->getBase()->isRegVar()) ||
        (checkCmodOnly == true && cmod != NULL && cmod->getBase() != NULL))
    {
        int dclOffset = 0;
        unsigned char* mask = NULL;
        G4_Declare* topdcl = NULL;

        if (checkCmodOnly == false)
        {
            topdcl = dst->getBase()->asRegVar()->getDeclare();
        }
        else
        {
            topdcl = cmod->asCondMod()->getTopDcl();
        }

        while (topdcl->getAliasDeclare() != NULL)
        {
            dclOffset += topdcl->getAliasOffset();
            topdcl = topdcl->getAliasDeclare();
        }

        mask = gra.getMask(topdcl);

        unsigned int size = topdcl->getByteSize();
        if (checkCmodOnly == true || dst->isFlag())
        {
            size *= BITS_PER_BYTE;
        }

        if (mask == NULL)
        {
            mask = (unsigned char*)m.alloc(size);

            gra.setMask(topdcl, mask);

            memset(mask, 0, size);
        }

        MUST_BE_TRUE(mask != NULL, "Valid mask not found for dcl " << topdcl->getName());

        unsigned short hstride, elemSize;
        short row, subReg;
        unsigned int startByte;

        if (checkCmodOnly == false)
        {
            hstride = dst->getHorzStride();

            row = dst->getRegOff();
            subReg = dst->getSubRegOff();
            elemSize = dst->getElemSize();

            if (inst->isSend() && !inst->isEOT())
            {
                if (updateDstMaskForScatter(inst, mask))
                {
                    return;
                }
            }

            if (dst->isFlag())
            {
                elemSize = 1;
            }

            startByte = (row * getGRFSize()) + (subReg * elemSize);

            if (dst->isFlag())
            {
                startByte = (row * 32) + (subReg * 8);
            }
        }
        else
        {
            hstride = 1;
            row = 0;
            elemSize = 1;
            startByte = cmod->asCondMod()->getLeftBound();
        }

        unsigned int rb = 0xffffffff;

        if (checkCmodOnly == true)
        {
            rb = cmod->asCondMod()->getRightBound();
        }
        else
        {
            rb = dst->getRightBound();
        }

        unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
        if (inst->isWriteEnableInst())
        {
            curEMBit = NOMASK_BYTE;
        }

        for (unsigned int i = dclOffset + startByte;
            i <= rb;
            i += (hstride * elemSize))
        {
            for (int j = 0; j < elemSize; j++)
            {
                MUST_BE_TRUE2(i + j < size, "updateDstMask writing past end of mask array size:" << size, inst);
                mask[i + j] |= curEMBit;
            }
            if (curEMBit != NOMASK_BYTE)
            {
                curEMBit++;
            }
        }
    }
}

unsigned int Augmentation::getByteSizeFromMask(AugmentationMasks type)
{
    if (type == AugmentationMasks::Default16Bit)
    {
        return 2;
    }
    else if (type == AugmentationMasks::Default32Bit)
    {
        return 4;
    }
    else if (type == AugmentationMasks::Default64Bit)
    {
        return 8;
    }

    MUST_BE_TRUE(false, "Unexpected type of mask");

    return 0;
}

bool Augmentation::isDefaultMaskDcl(G4_Declare* dcl, unsigned int simdSize, AugmentationMasks type)
{
    // default mask is one where dst's hstride is 1 and
    // elem size is 4
    bool isDefault = false;
    unsigned char* mask = gra.getMask(dcl);

    unsigned byteSize = getByteSizeFromMask(type);

    // treat simd32 as simd16 as the instruction is always split to 2 simd16
    if (simdSize == 32)
    {
        simdSize = 16;
    }
    if (mask != NULL)
    {
        G4_Declare* topdcl = dcl->getRootDeclare();
        bool isFlagDcl = (topdcl->getRegFile() == G4_FLAG)
            ? true : false;

        unsigned int size = topdcl->getByteSize();
        unsigned char curEMBit = 0;
        bool found = true;
        unsigned int wrapAround = simdSize*byteSize;

        if (isFlagDcl == true)
        {
            size *= BITS_PER_BYTE;
            wrapAround = 16;
        }

        for (unsigned int i = 0; i < size; i += 1)
        {
            if (isFlagDcl == true)
            {
                curEMBit++;
            }
            else
            {
                if (byteSize && i%byteSize == 0)
                {
                    curEMBit++;
                }
            }

            if (i%wrapAround == 0)
            {
                // Wrap around based on simd size
                // For SIMD8 wrap around each row,
                // for SIMD16 wrap around every other row
                curEMBit = 0;
            }

            if (mask[i] != curEMBit &&
                // For flags, we set bytesize = 2 although
                // the kernel is SIMD8. This means higher 8
                // bits of mask will be set to 0 since those
                // bits are never defined. Such masks need
                // not be considered non-default.
                !(isFlagDcl == true && mask[i] == 0))
            {
                found = false;
                break;
            }
        }

        if (found == true)
        {
            isDefault = true;
        }
    }

    return isDefault;
}

bool Augmentation::isDefaultMaskSubDeclare(unsigned char* mask, unsigned int lb, unsigned int rb, G4_Declare* dcl, unsigned int simdSize)
{
    bool isDefault = false;

    // treat simd32 as simd16 as the instruction is always split to 2 simd16
    if (simdSize == 32)
    {
        simdSize = 16;
    }

    if (mask != NULL)
    {
        unsigned int size = dcl->getByteSize();
        unsigned char curEMBit = 0;
        bool found = true;
        unsigned int wrapAround = simdSize * 4;
        unsigned leftBound = gra.getSubOffset(dcl);
        unsigned rightBound = leftBound + size - 1;

        ASSERT_USER(rightBound <= rb, "Wrong sub declare right bound!");

        for (unsigned int i = lb; i < rightBound + 1; i += 1)
        {
            if ((i - lb) % 4 == 0)
            {
                curEMBit++;
            }

            if ((i - lb) % wrapAround == 0)
            {
                curEMBit = 0;
            }

            if (i >= leftBound)
            {
                if (mask[i] != curEMBit)
                {
                    found = false;
                    break;
                }
            }
        }

        if (found == true)
        {
            isDefault = true;
        }
    }

    return isDefault;
}


void Augmentation::markNonDefaultMaskForSubDcl(G4_Declare *dcl, unsigned lb, unsigned rb, unsigned int simdSize)
{
    unsigned char* parentMask = gra.getMask(dcl);

    if (parentMask == NULL)
    {
        return;
    }

    auto dclSubDclSize = gra.getSubDclSize(dcl);
    for (unsigned i = 0; i < dclSubDclSize; i++)
    {
        G4_Declare * subDcl = gra.getSubDcl(dcl, i);
        unsigned leftBound = gra.getSubOffset(subDcl);
        unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

        if (!(rb < leftBound || rightBound < lb))  //There is overlap
        {
            unsigned char* mask = gra.getMask(subDcl);
            if (mask == NULL)
            {
                mask = parentMask + leftBound;
                gra.setMask(subDcl, mask);
            }

            if (lb <= leftBound && rightBound <= rb)
            {
                if (gra.getHasNonDefaultMaskDef(subDcl) == false &&
                    isDefaultMaskSubDeclare(parentMask, lb, rb, subDcl, simdSize) == false)
                {
                    gra.setAugmentationMask(subDcl, AugmentationMasks::NonDefault);
                }
            }
            else  //With local varaible splitting, it should not go here.
            {
                ASSERT_USER(false, "Wrong local varible splitting happened!");
                gra.setAugmentationMask(subDcl, AugmentationMasks::NonDefault);
            }
        }
    }
}

bool Augmentation::verifyMaskIfInit(G4_Declare* dcl, AugmentationMasks mask)
{
    // Return true if dcl mask is either undetermined or same as mask
    auto m = gra.getAugmentationMask(dcl);
    if (m == mask ||
        m == AugmentationMasks::Undetermined)
    {
        return true;
    }

    return false;
}

bool Augmentation::checkGRFPattern2(G4_Declare* dcl, G4_DstRegRegion* dst, unsigned maskOff,
    unsigned int lb, unsigned int rb, unsigned int execSize)
{
    auto opndByteSize = G4_Type_Table[dst->getType()].byteSize;
    unsigned int modWith = opndByteSize*kernel.getSimdSize();
    if (lb%modWith - (maskOff * opndByteSize * dst->getHorzStride()) <= opndByteSize)
    {
        if ((lb + (execSize * opndByteSize * dst->getHorzStride() - dst->getHorzStride()) - rb) < opndByteSize)
        {
            if (opndByteSize == 2 &&
                verifyMaskIfInit(dcl, AugmentationMasks::Default32Bit))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
                return true;
            }
            else if (opndByteSize == 4 &&
                verifyMaskIfInit(dcl, AugmentationMasks::Default64Bit))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
                return true;
            }
            else
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                return true;
            }
        }
    }

    return false;
}

// Returns true if dcl mask deemed to be non-default, false otherwise.
bool Augmentation::checkGRFPattern1(G4_Declare* dcl, G4_DstRegRegion* dst, unsigned maskOff,
    unsigned int lb, unsigned int rb, unsigned int execSize)
{
    auto opndByteSize = G4_Type_Table[dst->getType()].byteSize;
    unsigned int modWith = opndByteSize*kernel.getSimdSize();
    if (dst->getHorzStride() == 1)
    {
        if ((lb%modWith == (maskOff * opndByteSize) &&
            rb == (lb + (execSize * opndByteSize) - 1)))
        {
            // This will be taken only when hstride = 1
            if (opndByteSize == 2 &&
                verifyMaskIfInit(dcl, AugmentationMasks::Default16Bit))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::Default16Bit);
                return true;
            }
            else if (opndByteSize == 4 &&
                verifyMaskIfInit(dcl, AugmentationMasks::Default32Bit))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
                return true;
            }
            else if (opndByteSize == 8 &&
                verifyMaskIfInit(dcl, AugmentationMasks::Default64Bit))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
                return true;
            }
            else
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                return true;
            }
        }
    }

    return false;
}

void Augmentation::markNonDefaultDstRgn(G4_INST* inst, G4_Operand* opnd)
{
    if (inst->isPseudoKill())
    {
        return;
    }

    G4_DstRegRegion* dst = nullptr;
    G4_CondMod* condMod = nullptr;
    if (opnd->isDstRegRegion())
    {
        dst = opnd->asDstRegRegion();
    }
    else if (opnd->isCondMod())
    {
        condMod = opnd->asCondMod();
    }
    else
    {
        MUST_BE_TRUE(false, "Dont know how to handle this type of operand");
    }

    // Handle condMod
    if (condMod && condMod->getBase())
    {
        G4_Declare* dcl = condMod->getTopDcl();
        dcl = dcl->getRootDeclare();

        if (inst->isWriteEnableInst() ||
            opnd->getLeftBound() != inst->getMaskOffset())
        {
            gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
            return;
        }

        if (verifyMaskIfInit(dcl, AugmentationMasks::DefaultPredicateMask))
        {
            gra.setAugmentationMask(dcl, AugmentationMasks::DefaultPredicateMask);
        }
        return;
    }

    // Handle dst
    if (inst->isCall() || inst->opcode() == G4_pseudo_caller_save)
    {
        G4_Declare* dcl = dst->getBase()->asRegVar()->getDeclare();
        if (dcl && liveAnalysis.livenessClass(dcl->getRegFile()))
        {
            gra.setAugmentationMask(dcl->getRootDeclare(), AugmentationMasks::NonDefault);
        }
        return;
    }

    bool isFlagRA = liveAnalysis.livenessClass(G4_FLAG);
    if (dst &&
        dst->getBase() &&
        dst->getBase()->isRegVar())
    {
        G4_Declare* dcl = dst->getBase()->asRegVar()->getDeclare();
        if (!liveAnalysis.livenessClass(dcl->getRegFile()))
        {
            return;
        }
        unsigned int offTopDcl = 0;
        while (dcl->getAliasDeclare())
        {
            offTopDcl += dcl->getAliasOffset();
            dcl = dcl->getAliasDeclare();
        }

        // NoMask instructions's dst is always non-default
        if (inst->isWriteEnableInst())
        {
            gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
            return;
        }

        if (gra.getAugmentationMask(dcl) == AugmentationMasks::NonDefault)
            return;

        unsigned int maskOff = inst->getMaskOffset();
        unsigned int lb = dst->getLeftBound() + offTopDcl;
        unsigned int rb = dst->getRightBound() + offTopDcl;
        unsigned int execSize = inst->getExecSize();

        if (dcl->getAddressed())
        {
            gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
            return;
        }

        if (!isFlagRA)
        {
            // Treat send as special case because update mask for scatter
            // has some special checks.
            if (inst->isSend())
            {
                if (gra.getAugmentationMask(dcl) == AugmentationMasks::NonDefault)
                {
                    return;
                }

                updateDstMask(inst, false);
                if (isDefaultMaskDcl(dcl, kernel.getSimdSize(), AugmentationMasks::Default16Bit))
                {
                    gra.setAugmentationMask(dcl, AugmentationMasks::Default16Bit);
                }
                else if (isDefaultMaskDcl(dcl, kernel.getSimdSize(), AugmentationMasks::Default32Bit))
                {
                    gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
                }
                else if (isDefaultMaskDcl(dcl, kernel.getSimdSize(), AugmentationMasks::Default64Bit))
                {
                    bool useNonDefault = false;
                    useNonDefault |= (kernel.getSimdSize() >= 16 && dcl->getTotalElems() > 8);
                    useNonDefault |= (kernel.getSimdSize() == 8 && dcl->getTotalElems() > 4);

                    if (useNonDefault)
                    {
                        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                    }
                    else
                    {
                        gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
                    }
                }
                else
                {
                    gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                    return;
                }
            }
            else
            {
                bool found = false;
                // default one
                found |= checkGRFPattern1(dcl, dst, maskOff, lb, rb, execSize);
                if (!found ||
                    gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined)
                {
                    // hstride = 2 case
                    found |= checkGRFPattern2(dcl, dst, maskOff, lb, rb, execSize);
                }

                if (!found ||
                    gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined)
                {
                    gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                }
            }
        }
        else
        {
            // Handle flag register as destination here
            if (!(lb == maskOff && rb == (lb + execSize - 1)))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                return;
            }

            if (verifyMaskIfInit(dcl, AugmentationMasks::DefaultPredicateMask))
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::DefaultPredicateMask);
            }
        }
    }
}

// Returns true if any inst found using non-default mask.
// This function sets up lexical id of all instructions.
bool Augmentation::markNonDefaultMaskDef()
{
    bool nonDefaultMaskDefFound = false;
    unsigned int id = 0;

    // Iterate dcls list and mark obvious ones as non-default.
    // Obvoius non-default is 1 element, ie uniform dcl.
    for (auto dcl : kernel.Declares)
    {
        auto dclRegFile = dcl->getRegFile();
        if (!liveAnalysis.livenessClass(dclRegFile))
            continue;

        if (dclRegFile == G4_GRF || dclRegFile == G4_INPUT || dclRegFile == G4_ADDRESS)
        {
            if (dcl->getTotalElems() < 8)
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
            }
        }
        else if (dclRegFile == G4_FLAG)
        {
            // Flags are processed when processing instructions
        }
    }

    bool isFlagRA = liveAnalysis.livenessClass(G4_FLAG);

    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            inst->setLexicalId(id++);

            G4_DstRegRegion* dst = inst->getDst();

            if (dst)
            {
                markNonDefaultDstRgn(inst, dst);
            }

            if (isFlagRA &&
                inst->getCondMod())
            {
                markNonDefaultDstRgn(inst, inst->getCondMod());
            }
        }
    }

    // Update whether each dcl is default/not
    AugmentationMasks prevAugMask = AugmentationMasks::Undetermined;

    for (auto dcl : kernel.Declares)
    {
        if (liveAnalysis.livenessClass(dcl->getRegFile()))
        {
            if (gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined)
            {
                gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                nonDefaultMaskDefFound = true;
            }

            if (!nonDefaultMaskDefFound &&
                gra.getAugmentationMask(dcl) != prevAugMask &&
                prevAugMask != AugmentationMasks::Undetermined)
            {
                nonDefaultMaskDefFound = true;
            }

            prevAugMask = gra.getAugmentationMask(dcl);
        }

        if (liveAnalysis.livenessClass(G4_GRF) &&
            gra.getAugmentationMask(dcl) == AugmentationMasks::Default32Bit &&
            kernel.getSimdSize() > NUM_DWORDS_PER_GRF)
        {
            auto dclLR = gra.getLocalLR(dcl);
            if (dclLR)
            {
                int s;
                auto phyReg = dclLR->getPhyReg(s);
                if (phyReg && phyReg->asGreg()->getRegNum() % 2 != 0)
                {
                    // If LRA assignment is not 2GRF aligned for SIMD16 then
                    // mark it as non-default. GRA candidates cannot fully
                    // overlap with such ranges. Partial overlap is illegal.
                    gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
                    nonDefaultMaskDefFound = true;
                }
            }
        }
    }

    return nonDefaultMaskDefFound;
}

G4_BB* Augmentation::getTopmostBBDst(G4_BB* src, G4_BB* end, G4_BB* origSrc, unsigned int traversal)
{
    // Start from src BB and do a DFS. If any back-edges
    // are found then recursively invoke itself with dst
    // of back-edge. Any path that reaches BB "end"
    // will not be propagated forward.
    unsigned int topLexId = src->front()->getLexicalId();
    G4_BB* topmostBB = src;

    if (src != end)
    {
        src->markTraversed(traversal);
        src->setNestLevel();

        for (BB_LIST_ITER succ_it = src->Succs.begin();
            succ_it != src->Succs.end();
            succ_it++)
        {
            if ((*succ_it) == origSrc)
            {
                // Src of traversal traversed again without
                // ever traversing end node. So abort this path.
                return NULL;
            }

            if ((*succ_it)->isAlreadyTraversed(traversal) == true)
                continue;

            G4_BB* recursiveTopMostBB = getTopmostBBDst((*succ_it), end, origSrc, traversal);

            if (recursiveTopMostBB != NULL)
            {
                unsigned int recursiveTopMostBBLexId = recursiveTopMostBB->front()->getLexicalId();

                if (recursiveTopMostBBLexId < topLexId)
                {
                    topmostBB = recursiveTopMostBB;
                    topLexId = recursiveTopMostBBLexId;
                }
            }
            else
            {
                if (src != origSrc)
                {
                    topmostBB = NULL;
                    topLexId = 0;
                }
            }

            (*succ_it)->markTraversed(traversal);
            (*succ_it)->setNestLevel();
        }
    }

    return topmostBB;
}

void Augmentation::updateStartIntervalForSubDcl(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd)
{
    auto dclSubDclSize = gra.getSubDclSize(dcl);
    for (unsigned i = 0; i < dclSubDclSize; i++)
    {
        G4_Declare * subDcl = gra.getSubDcl(dcl, i);

        unsigned leftBound = gra.getSubOffset(subDcl);
        unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
        if (!(opnd->getRightBound() < leftBound || rightBound < opnd->getLeftBound()))
        {
            auto subDclStartInterval = gra.getStartInterval(subDcl);
            if (subDclStartInterval == NULL ||
                (subDclStartInterval->getLexicalId() > curInst->getLexicalId()))
            {
                gra.setStartInterval(subDcl, curInst);
            }

            auto subDclEndIntrval = gra.getEndInterval(subDcl);
            if (subDclEndIntrval == NULL ||
                (subDclEndIntrval->getLexicalId() < curInst->getLexicalId()))
            {
                gra.setEndInterval(subDcl, curInst);
            }
        }
    }

    return;
}

void Augmentation::updateEndIntervalForSubDcl(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd)
{
    auto dclSubDclSize = gra.getSubDclSize(dcl);
    for (unsigned i = 0; i < dclSubDclSize; i++)
    {
        G4_Declare * subDcl = gra.getSubDcl(dcl, i);

        unsigned leftBound = gra.getSubOffset(subDcl);
        unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
        if (!(opnd->getRightBound() < leftBound || rightBound < opnd->getLeftBound()))
        {
            auto subDclEndInterval = gra.getEndInterval(subDcl);
            if (subDclEndInterval == NULL ||
                (subDclEndInterval->getLexicalId() < curInst->getLexicalId()))
            {
                gra.setEndInterval(subDcl, curInst);
            }

            auto subDclStartInterval = gra.getStartInterval(subDcl);
            if (subDclStartInterval == NULL ||
                (subDclStartInterval->getLexicalId() > curInst->getLexicalId()))
            {
                gra.setStartInterval(subDcl, curInst);
            }
        }
    }

    return;
}

void Augmentation::updateStartInterval(G4_Declare* dcl, G4_INST* curInst)
{
    auto dclStartInterval = gra.getStartInterval(dcl);
    if (dclStartInterval == NULL ||
        (dclStartInterval->getLexicalId() > curInst->getLexicalId()))
    {
        gra.setStartInterval(dcl, curInst);
    }

    auto dclEndInterval = gra.getEndInterval(dcl);
    if (dclEndInterval == NULL ||
        (dclEndInterval->getLexicalId() < curInst->getLexicalId()))
    {
        gra.setEndInterval(dcl, curInst);
    }
}

void Augmentation::updateEndInterval(G4_Declare* dcl, G4_INST* curInst)
{
    auto dclEndInterval = gra.getEndInterval(dcl);
    if (dclEndInterval == NULL ||
        (dclEndInterval->getLexicalId() < curInst->getLexicalId()))
    {
        gra.setEndInterval(dcl, curInst);
    }

    auto dclStartInterval = gra.getStartInterval(dcl);
    if (dclStartInterval == NULL ||
        (dclStartInterval->getLexicalId() > curInst->getLexicalId()))
    {
        gra.setStartInterval(dcl, curInst);
    }
}

void Augmentation::updateStartIntervalForLocal(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd)
{
    updateStartInterval(dcl, curInst);
    if (dcl->getIsSplittedDcl())
    {
        updateStartIntervalForSubDcl(dcl, curInst, opnd);
    }
}

void Augmentation::updateEndIntervalForLocal(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd)
{
    updateEndInterval(dcl, curInst);
    if (dcl->getIsSplittedDcl())
    {
        updateEndIntervalForSubDcl(dcl, curInst, opnd);
    }
}



void GlobalRA::printLiveIntervals()
{
    for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
        dcl_it != kernel.Declares.end();
        dcl_it++)
    {
        if (getStartInterval(*dcl_it) != NULL ||
            getEndInterval(*dcl_it) != NULL)
        {
            DEBUG_VERBOSE((*dcl_it)->getName() << " (");

            if (getStartInterval(*dcl_it) != NULL)
            {
                DEBUG_VERBOSE(getStartInterval(*dcl_it)->getLexicalId());
            }
            else
            {
                DEBUG_VERBOSE("*");
            }

            DEBUG_VERBOSE(", ");

            if (getEndInterval(*dcl_it) != NULL)
            {
                DEBUG_VERBOSE(getEndInterval(*dcl_it)->getLexicalId());
            }
            else
            {
                DEBUG_VERBOSE("*");
            }

            DEBUG_VERBOSE("] " << std::endl);
        }
    }
}

#ifdef DEBUG_VERBOSE_ON
static int calculateBankConflictsInBB(G4_BB* bb, int &even_odd_num, int &low_high_num, int &threeSourceNum, bool twoSrcsBank)
{
    int conflict_num = 0;

    for (std::list<G4_INST*>::reverse_iterator i = bb->rbegin();
        i != bb->rend();
        i++)
    {
        bool hasSrc0 = false;
        int regNum0 = 0;
        int regNum1 = 0;
        int regNum2 = 0;

        G4_INST* inst = (*i);

        if (!(inst->getNumSrc() == 3 && !inst->isSend()))
            continue;

        G4_Operand* src0 = inst->getSrc(0);
        G4_Operand* src1 = inst->getSrc(1);
        G4_Operand* src2 = inst->getSrc(2);


        if (src1 && src1->isSrcRegRegion() &&
            src1->getBase() && src1->getBase()->asRegVar()->isPhyRegAssigned())
        {
            regNum1 = src1->getBase()->asRegVar()->getPhyReg()->getRegNum();
        }
        if (src2 && src2->isSrcRegRegion() &&
            src2->getBase() && src2->getBase()->asRegVar()->isPhyRegAssigned())
        {
            regNum2 = src2->getBase()->asRegVar()->getPhyReg()->getRegNum();
        }

        if ((src0 && src0->isSrcRegRegion()) &&
            src0->getBase() && src0->getBase()->asRegVar()->isPhyRegAssigned())
        {
            regNum0 = src0->getBase()->asRegVar()->getPhyReg()->getRegNum();
        }

        if (regNum1 == regNum2 && regNum0 == regNum1)
            continue;

        if (!twoSrcsBank)
        {
            if (regNum0 < SECOND_HALF_BANK_START_GRF &&
                regNum1 < SECOND_HALF_BANK_START_GRF &&
                regNum2 < SECOND_HALF_BANK_START_GRF ||
                regNum0 >= SECOND_HALF_BANK_START_GRF &&
                regNum1 >= SECOND_HALF_BANK_START_GRF &&
                regNum2 >= SECOND_HALF_BANK_START_GRF)
            {
                if ((regNum1 % 2) == (regNum2) % 2 &&
                    (regNum0 % 2) == (regNum1 % 2))
                {
                    conflict_num++;
                }
            }
        }
        else
        {
            if ((regNum1 % 2) == (regNum2 % 2))
            {
                if (regNum1 < SECOND_HALF_BANK_START_GRF &&
                    regNum2 < SECOND_HALF_BANK_START_GRF ||
                    regNum1 >= SECOND_HALF_BANK_START_GRF &&
                    regNum2 >= SECOND_HALF_BANK_START_GRF)
                {
                    conflict_num++;
                }
                else
                {
                    low_high_num++;
                }
            }
            else
            {
                even_odd_num++;
            }
        }
        threeSourceNum++;
    }

    return conflict_num;
}

static int calculateBankConflicts(G4_Kernel& kernel)
{
    bool SIMD16 = (kernel.getSimdSize() >= 16);
    bool twoSrcsConflict = kernel.fg.builder->twoSourcesCollision();

    for (BB_LIST_ITER it = kernel.fg.begin(); it != kernel.fg.end(); it++)
    {
        int conflict_num = 0;
        int even_odd_num = 0;
        int low_high_num = 0;
        int threeSourceNum = 0;
        G4_BB* curBB = (*it);

        conflict_num = calculateBankConflictsInBB(curBB, even_odd_num, low_high_num, threeSourceNum, twoSrcsConflict);
        if (threeSourceNum)
        {
            if (SIMD16)
            {
                printf("SIMD16, BB: %d,  Even_odd: %d, low_high: %d, Conflicts: %d, Three: %d, Insts: %d,  kernel: %s\n", curBB->getId(), even_odd_num, low_high_num, conflict_num, threeSourceNum, curBB->size(), kernel.getName());
            }
            else
            {
                printf("SIMD8, BB: %d,  Even_odd: %d, low_high: %d, Conflicts: %d, Three: %d, Insts: %d,  kernel: %s\n", curBB->getId(), even_odd_num, low_high_num, conflict_num, threeSourceNum, curBB->size(), kernel.getName());
            }
        }
    }

    return 0;
}
#endif

void Augmentation::buildLiveIntervals()
{
    // Treat variables live-in to program first
    G4_BB* entryBB = kernel.fg.getEntryBB();

    // Live-in variables have their start interval start with
    // first instruction of entry BB
    for (unsigned int i = 0; i < liveAnalysis.getNumSelectedVar(); i++)
    {
        if (liveAnalysis.isLiveAtEntry(entryBB, i))
        {
            G4_Declare* dcl = lrs[i]->getDcl();

            while (dcl->getAliasDeclare() != NULL)
            {
                dcl = dcl->getAliasDeclare();
            }

            updateStartInterval(dcl, entryBB->front());
        }
    }

    unsigned int funcCnt = 0;

    for (BB_LIST_ITER bb_it = kernel.fg.begin();
        bb_it != kernel.fg.end();
        bb_it++)
    {
        G4_BB* curBB = (*bb_it);

        for (INST_LIST_ITER inst_it = curBB->begin();
            inst_it != curBB->end();
            inst_it++)
        {
            G4_INST* inst = (*inst_it);

            if (inst->isPseudoKill() == true)
            {
                continue;
            }

            G4_DstRegRegion* dst = inst->getDst();

            if (inst->isCall() == true)
            {
                const char* name = kernel.fg.builder->getNameString(kernel.fg.builder->mem, 32, "SCALL_%d", funcCnt++);
                G4_Declare* scallDcl = kernel.fg.builder->createDeclareNoLookup(name, G4_GRF, 1, 1, Type_UD);

                updateStartInterval(scallDcl, inst);
                updateEndInterval(scallDcl, inst);

                FuncInfo* callee = curBB->getCalleeInfo();
                unsigned int funcId = callee->getId();
                std::pair<G4_INST*, unsigned int> callInfo(inst, funcId);
                callDclMap.insert(std::pair<G4_Declare*, std::pair<G4_INST*, unsigned int>>(scallDcl, callInfo));

                continue;
            }

            if (dst &&
                dst->getRegAccess() == Direct &&
                dst->getBase())
            {
                // Destination
                G4_Declare* defdcl = GetTopDclFromRegRegion(dst);

                if (dst->getBase()->isRegAllocPartaker())
                {
                    if (defdcl &&
                        gra.getLocalLR(defdcl))
                    {
                        updateStartIntervalForLocal(defdcl, inst, dst);
                    }
                    else
                    {
                        updateStartInterval(defdcl, inst);
                    }
                }
                else if (liveAnalysis.livenessClass(G4_GRF))
                {
                    LocalLiveRange* defdclLR = nullptr;

                    // Handle ranges allocated by local RA
                    if (defdcl &&
                        (defdclLR = gra.getLocalLR(defdcl)) &&
                        defdclLR->getAssigned() == true &&
                        !defdclLR->isEOT())
                    {
                        updateStartInterval(defdcl, inst);
                    }
                }
            }
            else if (liveAnalysis.livenessClass(G4_ADDRESS) &&
                dst &&
                dst->getRegAccess() == IndirGRF &&
                dst->getBase() &&
                dst->getBase()->isRegVar())
            {
                // Destination is indirect
                G4_Declare* defdcl = dst->getBase()->asRegVar()->getDeclare();

                while (defdcl->getAliasDeclare() != NULL)
                {
                    defdcl = defdcl->getAliasDeclare();
                }

                updateEndInterval(defdcl, inst);
            }

            if (liveAnalysis.livenessClass(G4_FLAG))
            {
                G4_CondMod* cmod = inst->getCondMod();

                if (cmod != NULL &&
                    cmod->getBase() != NULL)
                {
                    // Conditional modifier
                    G4_Declare* dcl = cmod->getBase()->asRegVar()->getDeclare();

                    while (dcl->getAliasDeclare() != NULL)
                    {
                        dcl = dcl->getAliasDeclare();
                    }

                    updateStartInterval(dcl, inst);
                }
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = inst->getSrc(i);
                if (!src || !src->isSrcRegRegion())
                {
                    continue;
                }
                G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();

                if (srcRegion->getRegAccess() == Direct && srcRegion->getBase())
                {
                    G4_Declare* usedcl = GetTopDclFromRegRegion(src);

                    if (srcRegion->getBase()->isRegAllocPartaker())
                    {
                        if (gra.getLocalLR(usedcl))
                        {
                            updateEndIntervalForLocal(usedcl, inst, src);
                        }
                        else
                        {
                            updateEndInterval(usedcl, inst);
                        }
                    }
                    else if (liveAnalysis.livenessClass(G4_GRF))
                    {
                        LocalLiveRange* usedclLR = nullptr;
                        if (usedcl &&
                            (usedclLR = gra.getLocalLR(usedcl)) &&
                            usedclLR->getAssigned() == true &&
                            !usedclLR->isEOT())
                        {
                            updateEndInterval(usedcl, inst);
                        }
                    }
                }
                else if (liveAnalysis.livenessClass(G4_GRF) && srcRegion->isIndirect())
                {
                    PointsToAnalysis& pta = liveAnalysis.getPointsToAnalysis();
                    auto pointsToSet = pta.getAllInPointsTo(srcRegion->getBase()->asRegVar());
                    if (pointsToSet == nullptr)
                    {
                        // this can happen if the address is coming from addr spill
                        pointsToSet = pta.getIndrUseVectorPtrForBB(curBB->getId());
                    }
                    for (auto pointsToVar : *pointsToSet)
                    {
                        if (pointsToVar->isRegAllocPartaker())
                        {
                            updateEndInterval(pointsToVar->getDeclare()->getRootDeclare(), inst);
                        }
                    }
                }
                else if (liveAnalysis.livenessClass(G4_ADDRESS) &&
                    srcRegion->getRegAccess() == IndirGRF &&
                    srcRegion->getBase() &&
                    srcRegion->getBase()->isRegVar())
                {
                    G4_Declare* usedcl = src->asSrcRegRegion()->getBase()->asRegVar()->getDeclare();

                    while (usedcl->getAliasDeclare() != NULL)
                    {
                        usedcl = usedcl->getAliasDeclare();
                    }

                    updateEndInterval(usedcl, inst);
                }
            }

            if (liveAnalysis.livenessClass(G4_FLAG))
            {
                G4_Predicate* pred = inst->getPredicate();

                if (pred != NULL)
                {
                    // Predicate
                    G4_Declare* dcl = pred->getBase()->asRegVar()->getDeclare();

                    while (dcl->getAliasDeclare() != NULL)
                    {
                        dcl = dcl->getAliasDeclare();
                    }

                    updateEndInterval(dcl, inst);
                }
            }
        }
    }

    // extend all variables that are live at bb entry to the given inst
    // ToDo: this seems very slow when # variable is large, should look for sparse implementation
    auto extendVarLiveness = [this](G4_BB* bb, G4_INST* inst)
    {
        for (unsigned int i = 0; i < liveAnalysis.getNumSelectedVar(); i++)
        {
            if (liveAnalysis.isLiveAtEntry(bb, i) == true)
            {
                // Extend ith live-interval
                G4_Declare* dcl = lrs[i]->getDcl();

                while (dcl->getAliasDeclare() != NULL)
                {
                    dcl = dcl->getAliasDeclare();
                }

#ifdef DEBUG_VERBOSE_ON
                unsigned int oldStart = dcl->getStartInterval()->getLexicalId();
#endif

                updateStartInterval(dcl, inst);

#ifdef DEBUG_VREBOSE_ON
                if (oldStart > dcl->getStartInterval()->getLexicalId())
                {
                    std::cout << "Extending " << dcl->getName() << " from old start " <<
                        oldStart << " to " <<
                        startInst->getLexicalId() <<
                        " due to back-edge" <<
                        std::endl;
                }
#endif
            }
        }
    };

    if (!kernel.fg.isReducible())
    {
        //use SCC instead
        //FIXME: does augmentation work in the presence of subroutine? neither SCCAnalysis nor findNaturalLoops
        //considers the call graph
        SCCAnalysis SCCFinder(kernel.fg);
        SCCFinder.run();
        for (auto iter = SCCFinder.SCC_begin(), iterEnd = SCCFinder.SCC_end(); iter != iterEnd; ++iter)
        {
            auto&& anSCC = *iter;
            std::unordered_set<G4_BB*> SCCSucc; // any successor BB of the SCC
            G4_BB* headBB = anSCC.getEarliestBB();
            for (auto BI = anSCC.body_begin(), BIEnd = anSCC.body_end(); BI != BIEnd; ++BI)
            {
                G4_BB* bb = *BI;
                for (auto succ : bb->Succs)
                {
                    if (!anSCC.isMember(succ))
                    {
                        SCCSucc.insert(succ);
                    }
                }
            }
            for (auto exitBB : SCCSucc)
            {
                extendVarLiveness(exitBB, headBB->front());
            }
        }
    }
    else
    {
        // process each natural loop
        for (auto&& iter : kernel.fg.naturalLoops)
        {
            auto&& backEdge = iter.first;
            G4_INST* startInst = (backEdge.second)->front();
            const std::set<G4_BB*>& loopBody = iter.second;

            for (auto block : loopBody)
            {
                // FIXME: this may process a BB multiple times
                for (auto succBB : block->Succs)
                {
                    if (loopBody.find(succBB) == loopBody.end())
                    {
                        G4_BB* exitBB = succBB;

                        unsigned int latchBBId = (backEdge.first)->getId();
                        unsigned int exitBBId = succBB->getId();
                        if (exitBBId < latchBBId &&
                            succBB->Succs.size() == 1)
                        {
                            exitBB = succBB->Succs.front();
                        }

#ifdef DEBUG_VERBOSE_ON
                        std::cout << "==> Extend live-in for BB" << exitBB->getId() << std::endl;
                        exitBB->emit(std::cout);
#endif
                        extendVarLiveness(exitBB, startInst);
                    }
                }
            }

            G4_BB* startBB = backEdge.second;
            G4_BB* EndBB = backEdge.first;
            for (unsigned int i = 0; i < liveAnalysis.getNumSelectedVar(); i++)
            {
                if (liveAnalysis.isLiveAtEntry(startBB, i) == true &&
                    liveAnalysis.isLiveAtExit(EndBB, i) == true)
                {
                    G4_Declare* dcl = lrs[i]->getDcl();

                    while (dcl->getAliasDeclare() != NULL)
                    {
                        dcl = dcl->getAliasDeclare();
                    }

#ifdef DEBUG_VERBOSE_ON
                    unsigned int oldEnd = dcl->getEndInterval()->getLexicalId();
#endif

                    updateEndInterval(dcl, EndBB->back());

#ifdef DEBUG_VREBOSE_ON
                    if (oldEnd < dcl->getEndInterval()->getLexicalId())
                    {
                        std::cout << "Extending " << dcl->getName() << " from old end " <<
                            oldEnd << " to " <<
                            dcl->getEndInterval()->getLexicalId() <<
                            " due to back-edge" <<
                            std::endl;
                    }
#endif
                }
            }

        }
    }

#ifdef DEBUG_VERBOSE_ON
    // Print calculated live-ranges
    gra.printLiveIntervals();
#endif
}

void Augmentation::clearIntervalInfo()
{
    // Clear out calculated information so that subsequent RA
    // iterations dont have stale information
    for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin(), end = kernel.Declares.end();
        dcl_it != end;
        dcl_it++)
    {
        gra.setStartInterval(*dcl_it, nullptr);
        gra.setEndInterval(*dcl_it, nullptr);
        gra.setMask(*dcl_it, nullptr);
        gra.setAugmentationMask(*dcl_it, AugmentationMasks::Undetermined);
    }
}

class compareInterval
{
public:
    GlobalRA& gra;

    compareInterval(GlobalRA& g) : gra(g)
    {
    }

    bool operator()(G4_Declare* dcl1, G4_Declare* dcl2)
    {
        return gra.getStartInterval(dcl1)->getLexicalId() < gra.getStartInterval(dcl2)->getLexicalId();
    }
};

void Augmentation::sortLiveIntervals()
{
    // Sort all intervals in kernel based on their starting point in
    // ascending order and return them in sortedIntervals vector
    // This is actually more efficient (at least according to vTune) than the O(N)
    // bucket sort algorithm below, since it avoids most of the malloc/free overhead from the vector.resize()
    for (DECLARE_LIST_ITER dclIt = kernel.Declares.begin(), dclItEnd = kernel.Declares.end();
        dclIt != dclItEnd;
        ++dclIt)
    {
        G4_Declare* dcl = (*dclIt);
        if (gra.getStartInterval(dcl) != NULL)
        {
            sortedIntervals.push_back(dcl);
        }
    }

    std::sort(sortedIntervals.begin(), sortedIntervals.end(), compareInterval(gra));

#ifdef DEBUG_VERBOSE_ON
    DEBUG_VERBOSE("Live-intervals in sorted order: " << std::endl);
    for (DECLARE_LIST_ITER dcl_it = sortedIntervals.begin();
        dcl_it != sortedIntervals.end();
        dcl_it++)
    {
        DEBUG_VERBOSE((*dcl_it)->getName() << " - " <<
            "(" << (*dcl_it)->getStartInterval()->getLexicalId() <<
            ", " << (*dcl_it)->getEndInterval()->getLexicalId() <<
            "]" << std::endl);
    }
#endif
}

unsigned int Augmentation::getEnd(G4_Declare*& dcl)
{
    return gra.getEndInterval(dcl)->getLexicalId();
}

// Mark interference between dcls. Either one of dcls may have
// register assigned by local RA so handle those cases too.
// Re-entrant function.
void Augmentation::handleSIMDIntf(G4_Declare* firstDcl, G4_Declare* secondDcl, bool isCall)
{
    auto markIntfWithLRAAssignment = [](G4_Declare* firstDcl, G4_Declare* lraAssigned, Interference& intf)
    {
        unsigned int numRows = lraAssigned->getNumRows();
        G4_VarBase* preg = lraAssigned->getRegVar()->getPhyReg();
        MUST_BE_TRUE(preg->isGreg(), "Expecting a physical register during building interference among incompatible masks");
        unsigned int start = preg->asGreg()->getRegNum();

        for (unsigned int i = start; i < (start + numRows); i++)
        {
            auto GRFDcl = intf.getGRFDclForHRA(i);
            intf.checkAndSetIntf(firstDcl->getRegVar()->getId(), GRFDcl->getRegVar()->getId());

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Marking interference between " << firstDcl->getName() <<
                " and " << GRFDcl->getName() << std::endl);
#endif
        }
    };

    if (firstDcl->getRegFile() == G4_INPUT &&
        firstDcl->getRegVar()->getPhyReg() != NULL &&
        secondDcl->getRegFile() == G4_INPUT &&
        secondDcl->getRegVar()->getPhyReg() != NULL)
    {
        return;
    }

    if (kernel.fg.isPseudoVCADcl(firstDcl) || kernel.fg.isPseudoVCADcl(secondDcl))
    {
        // Mark intf for following pattern:
        // V33 =
        // ...
        // if
        //     = V33
        //     fcall
        // ...
        // else
        //     = V33
        // endif
        //
        // V33 will interfere with VCA_SAVE pseudo node.
        // It also needs to interfere with retval to
        // ensure V33 and retval dont get same allocation.
        // Note that if V33 is actually live after fcall
        // then graph coloring will do this for us. In this
        // case however we need to rely on augmentation.
        FCALL_RET_MAP_ITER retIter = kernel.fg.isPseudoVCADcl(firstDcl) ? fcallRetMap.find(firstDcl) : fcallRetMap.find(secondDcl);
        LocalLiveRange* otherDclLR = nullptr;
        if (retIter != fcallRetMap.end())
        {
            G4_Declare* retVar = retIter->second;
            G4_Declare* otherDcl = kernel.fg.isPseudoVCADcl(firstDcl) ? secondDcl : firstDcl;
            if(otherDcl->getRegVar()->isRegAllocPartaker())
                intf.checkAndSetIntf(otherDcl->getRegVar()->getId(), retVar->getRegVar()->getId());
            else if((otherDclLR = gra.getLocalLR(otherDcl)) &&
                otherDclLR->getAssigned() &&
                !otherDclLR->isEOT())
            {
                markIntfWithLRAAssignment(retVar, otherDcl, intf);
            }
        }
    }

    if (firstDcl->getRegVar()->isRegAllocPartaker() &&
        secondDcl->getRegVar()->isRegAllocPartaker())
    {
        if (!intf.varSplitCheckBeforeIntf(firstDcl->getRegVar()->getId(), secondDcl->getRegVar()->getId()))
        {
            intf.checkAndSetIntf(firstDcl->getRegVar()->getId(), secondDcl->getRegVar()->getId());
            if (isCall)
            {
                intf.buildInterferenceWithAllSubDcl(firstDcl->getRegVar()->getId(), secondDcl->getRegVar()->getId());
            }
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Marking interference between " << firstDcl->getName() <<
                " and " << secondDcl->getName() << std::endl);
#endif
        }
    }
    else if (liveAnalysis.livenessClass(G4_GRF))
    {
        LocalLiveRange* secondDclLR = nullptr, *firstDclLR = nullptr;

        if (firstDcl->getRegVar()->isRegAllocPartaker() &&
            (secondDclLR = gra.getLocalLR(secondDcl)) &&
            secondDclLR->getAssigned() &&
            !secondDclLR->isEOT())
        {
            // secondDcl was assigned by local RA and it uses
            markIntfWithLRAAssignment(firstDcl, secondDcl, intf);
        }
        else if (secondDcl->getRegVar()->isRegAllocPartaker() &&
            (firstDclLR = gra.getLocalLR(firstDcl)) &&
            firstDclLR->getAssigned() &&
            !firstDclLR->isEOT())
        {
            // Call self with reversed parameters instead of re-implementing
            // above code
            handleSIMDIntf(secondDcl, firstDcl, isCall);
        }
    }
}

bool Augmentation::isNoMask(G4_Declare* dcl, unsigned int size)
{
    unsigned char* mask = gra.getMask(dcl);
    bool result = false;

    if (mask != NULL)
    {
        result = true;

        for (unsigned int i = 0; i < size; i++)
        {
            if (mask[i] != NOMASK_BYTE)
            {
                result = false;
            }
        }
    }

    return result;
}

bool Augmentation::isConsecutiveBits(G4_Declare* dcl, unsigned int size)
{
    unsigned char* mask = gra.getMask(dcl);
    bool result = false;

    if (mask != NULL)
    {
        result = true;

        for (unsigned int i = 0; i < size; i++)
        {
            if (mask[i] != i)
            {
                result = false;
            }
        }
    }

    return result;
}

bool Augmentation::isCompatible(G4_Declare* testDcl, G4_Declare* biggerDcl)
{
    bool compatible = false;

    unsigned testSize = testDcl->getRegVar()->isFlag() ? testDcl->getNumberFlagElements() : testDcl->getByteSize();
    unsigned biggerSize = biggerDcl->getRegVar()->isFlag() ? biggerDcl->getNumberFlagElements() : biggerDcl->getByteSize();
    unsigned int size = (testSize < biggerSize ? testSize : biggerSize);

    // Masks are compatible when:
    // i. Both decls have exactly 1 EM bit defining each byte
    //  (This means a dcl with Q1 in one inst and Q2 in another
    //   instruction writing same subregisters is not a candidate
    //   for next step).
    // ii. Bytes at common indices are enabled by same EM bit
    //  (This means NoMask dcl is compatible with NoMask dcl and
    //   not with any other dcl).
    // UPDATE: (ii) above is now altered such that NoMask dcls
    // that overlap are considered to be incompatible. This is to
    // handle removal of JIP edges (then->else edge).

    unsigned char* testMask = gra.getMask(testDcl);
    unsigned char* biggerMask = gra.getMask(biggerDcl);

    if (testMask != NULL && biggerMask != NULL)
    {
        // Lets pattern match
        if (testDcl->getRegFile() == G4_FLAG)
        {
            if (isConsecutiveBits(testDcl, size) &&
                isConsecutiveBits(biggerDcl, size))
            {
                compatible = true;
            }
        }
        else
        {
            // Add another pattern to check here
        }
    }

    return compatible;
}

void Augmentation::expireIntervals(unsigned int startIdx)
{
    // Expire elements from both lists
    while (defaultMask.size() > 0)
    {
        if (gra.getEndInterval(defaultMask.front())->getLexicalId() <= startIdx)
        {
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Expiring " << defaultMask.front()->getName() << std::endl);
#endif
            defaultMask.pop_front();
        }
        else
        {
            break;
        }
    }

    while (nonDefaultMask.size() > 0)
    {
        if (gra.getEndInterval(nonDefaultMask.front())->getLexicalId() <= startIdx)
        {
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Expiring " << nonDefaultMask.front()->getName() << std::endl);
#endif
            nonDefaultMask.pop_front();
        }
        else
        {
            break;
        }
    }
}

// Return true if edge between dcl1 and dcl2 is strong.
bool Interference::isStrongEdgeBetween(G4_Declare* dcl1, G4_Declare* dcl2)
{
    auto dcl1RegVar = dcl1->getRegVar();
    auto dcl2RegVar = dcl2->getRegVar();
    auto dcl1RAPartaker = dcl1RegVar->isRegAllocPartaker();
    auto dcl2RAPartaker = dcl2RegVar->isRegAllocPartaker();

    if (dcl1RAPartaker &&
        dcl2RAPartaker)
    {
        if (interfereBetween(dcl1RegVar->getId(),
            dcl2RegVar->getId()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    if (dcl1RAPartaker)
    {
        auto dcl2NumRows = dcl2->getNumRows();
        auto startPhyReg = dcl2RegVar->getPhyReg()->asGreg()->getRegNum();
        auto dcl2LR = gra.getLocalLR(dcl2);

        if (dcl2LR &&
            dcl2LR->getAssigned())
        {
            bool allEdgesStrong = true;
            for (unsigned int i = startPhyReg; i < (startPhyReg + dcl2NumRows); i++)
            {
                G4_Declare* lraPreg = getGRFDclForHRA(i);
                allEdgesStrong &= interfereBetween(lraPreg->getRegVar()->getId(), dcl1RegVar->getId());
            }

            if (allEdgesStrong)
                return true;
        }
    }
    else
    {
        return isStrongEdgeBetween(dcl2, dcl1);
    }

    return false;
}

//
// Mark interference between newDcl and other incompatible dcls in current active lists.
//
void Augmentation::buildSIMDIntfDcl(G4_Declare* newDcl, bool isCall)
{
    auto newDclAugMask = gra.getAugmentationMask(newDcl);

    for (auto defaultDcl : defaultMask)
    {
        if (gra.getAugmentationMask(defaultDcl) != newDclAugMask)
        {
            handleSIMDIntf(defaultDcl, newDcl, isCall);
        }
        else
        {
            if (liveAnalysis.livenessClass(G4_GRF) &&
                // Populate compatible sparse intf data structure
                // only for 64-bit bit types since others can be
                // handled using Even align.
                gra.getAugmentationMask(defaultDcl) == AugmentationMasks::Default64Bit &&
                newDclAugMask == AugmentationMasks::Default64Bit)
            {
                if (defaultDcl->getRegVar()->isPhyRegAssigned() &&
                    newDcl->getRegVar()->isPhyRegAssigned())
                {
                    continue;
                }

                if (intf.isStrongEdgeBetween(defaultDcl, newDcl))
                {
                    // No need to add weak edge
                    continue;
                }

                // defaultDcl and newDcl are compatible live-ranges and can have weak edge in intf graph
                auto it = intf.compatibleSparseIntf.find(defaultDcl);
                if (it != intf.compatibleSparseIntf.end())
                {
                    it->second.push_back(newDcl);
                }
                else
                {
                    std::vector<G4_Declare*> v(1, newDcl);
                    intf.compatibleSparseIntf.insert(
                        std::make_pair(defaultDcl, v));
                }

                it = intf.compatibleSparseIntf.find(newDcl);
                if (it != intf.compatibleSparseIntf.end())
                {
                    it->second.push_back(defaultDcl);
                }
                else
                {
                    std::vector<G4_Declare*> v(1, defaultDcl);
                    intf.compatibleSparseIntf.insert(
                        std::make_pair(newDcl, v));
                }
            }
        }
    }

    // Mark interference among non-default mask variables
    for (auto nonDefaultDcl : nonDefaultMask)
    {
        // Non-default masks are different so mark interference
        handleSIMDIntf(nonDefaultDcl, newDcl, isCall);
    }
}

//
// Mark interference between newDcl and other incompatible dcls in current active lists.
// If newDcl was created for a subroutine call, do this for all varaibles in function summary.
//
void Augmentation::buildSIMDIntfAll(G4_Declare* newDcl)
{
    auto callDclMapIt = callDclMap.find(newDcl);
    if (callDclMapIt != callDclMap.end())
    {

        G4_Declare* varDcl = NULL;

        if (liveAnalysis.livenessClass(G4_GRF))
        {
            G4_INST* callInst = (*callDclMapIt).second.first;
            varDcl = callInst->getDst()->getBase()->asRegVar()->getDeclare();
            buildSIMDIntfDcl(varDcl, false);
        }

        unsigned int funcId = (*callDclMapIt).second.second;
        for (unsigned int i = 0; i < liveAnalysis.getNumSelectedVar(); i++)
        {
            if (liveAnalysis.maydef[funcId].isSet(i))
            {
                varDcl = lrs[i]->getDcl();
                buildSIMDIntfDcl(varDcl, true);
            }
        }
    }
    else
    {
        buildSIMDIntfDcl(newDcl, false);
    }
}

//
// Perform linear scan and mark interference between conflicting dcls with incompatible masks.
//
void Augmentation::buildInterferenceIncompatibleMask()
{
    // Create 2 active lists - 1 for holding active live-intervals
    // with non-default mask and other for default mask
    unsigned int oldStartIdx = 0;

    for (auto dcl_it = sortedIntervals.begin(), end = sortedIntervals.end();
        dcl_it != end;
        dcl_it++)
    {
        G4_Declare* newDcl = (*dcl_it);
        unsigned int startIdx = gra.getStartInterval(newDcl)->getLexicalId();
#ifdef DEBUG_VERBOSE_ON
        DEBUG_VERBOSE("New idx " << startIdx << std::endl);
#endif

        expireIntervals(startIdx);
        buildSIMDIntfAll(newDcl);

        // Add newDcl to correct list
        if (gra.getHasNonDefaultMaskDef(newDcl) || newDcl->getAddressed() == true)
        {
            bool done = false;

            for (auto nonDefaultIt = nonDefaultMask.begin();
                nonDefaultIt != nonDefaultMask.end();
                nonDefaultIt++)
            {
                G4_Declare* nonDefaultDcl = (*nonDefaultIt);

                if (gra.getEndInterval(nonDefaultDcl)->getLexicalId() >= gra.getEndInterval(newDcl)->getLexicalId())
                {
                    nonDefaultMask.insert(nonDefaultIt, newDcl);
                    done = true;
                    break;
                }
            }

            if (done == false)
            {
                nonDefaultMask.push_back(newDcl);
            }

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Adding " << newDcl->getName() <<
                " to non-default list" << std::endl);
#endif
        }
        else
        {
            bool done = false;

            for (auto defaultIt = defaultMask.begin();
                defaultIt != defaultMask.end();
                defaultIt++)
            {
                G4_Declare* defaultDcl = (*defaultIt);

                if (gra.getEndInterval(defaultDcl)->getLexicalId() >= gra.getEndInterval(newDcl)->getLexicalId())
                {
                    defaultMask.insert(defaultIt, newDcl);
                    done = true;
                    break;
                }
            }

            if (done == false)
            {
                defaultMask.push_back(newDcl);
            }

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Adding " << newDcl->getName() <<
                " to default list" << std::endl);
#endif
        }

        oldStartIdx = startIdx;
    }
}

void Augmentation::augmentIntfGraph()
{
    {
        if (!(kernel.getOptions()->getTarget() == VISA_3D &&
            !liveAnalysis.livenessClass(G4_ADDRESS) &&
            kernel.fg.size() > 2))
        {
            return;
        }
    }

    // First check whether any definitions exist with incompatible mask
    bool nonDefaultMaskDef = markNonDefaultMaskDef();

    if (nonDefaultMaskDef == true)
    {
        // Atleast one definition with non-default mask was found so
        // perform steps to augment intf graph with such defs

        // Now build live-intervals globally. This function will
        // calculate live-intervals and assign start/end inst
        // for respective declares.
        buildLiveIntervals();

        // Sort live-intervals based on their start
        sortLiveIntervals();

        if (gra.verifyAugmentation)
        {
            gra.verifyAugmentation->loadAugData(sortedIntervals, lrs, intf.liveAnalysis->getNumSelectedVar(), &intf, gra);
        }

        if (kernel.fg.builder->getOption(vISA_GenerateDebugInfo))
        {
            // Following is done to prevent passing GlobalRA to debug info function
            // for clear interface.
            std::vector<std::tuple<G4_Declare*, G4_INST*, G4_INST*>> dclIntervals;
            dclIntervals.reserve(sortedIntervals.size());
            for (auto& dcl : sortedIntervals)
            {
                dclIntervals.push_back(std::make_tuple(dcl, gra.getStartInterval(dcl), gra.getEndInterval(dcl)));
            }
            updateDebugInfo(kernel, dclIntervals);
        }

        // Perform linear scan to augment graph
        buildInterferenceIncompatibleMask();

        if (liveAnalysis.livenessClass(G4_GRF))
        {
            if (kernel.getSimdSize() > NUM_DWORDS_PER_GRF)
            {
                // Set alignment of all GRF candidates
                // to 2GRF except for NoMask variables
#ifdef DEBUG_VERBOSE_ON
                DEBUG_VERBOSE("Kernel size is SIMD" << kernel.getSimdSize() << " so updating all GRFs to be 2GRF aligned" << std::endl);
#endif
                gra.evenAlign();
            }
            gra.updateSubRegAlignment(GRFALIGN);
        }

        // Clear information calculated in this iteration of RA so
        // a later RA iteration does not use stale information
        clearIntervalInfo();
    }
}

void Interference::buildInterferenceWithLocalRA(G4_BB* bb)
{
    auto LRASummary = kernel.fg.getBBLRASummary(bb);
    if (LRASummary == nullptr)
    {
        return;
    }

    BitSet cur(kernel.getNumRegTotal(), true);
    BitSet live(maxId, false);
    std::vector<int> curUpdate;

    buildInterferenceAtBBExit(bb, live);

#ifdef DEBUG_VERBOSE_ON
    DEBUG_VERBOSE("BB" << bb->getId() << std::endl);
#endif

    for (INST_LIST_RITER rit = bb->rbegin(), rend = bb->rend();
        rit != rend;
        rit++)
    {
        bool update = false;
        G4_INST* inst = (*rit);
        curUpdate.clear();

#ifdef DEBUG_VERBOSE_ON
        inst->emit(COUT_ERROR);
        DEBUG_VERBOSE("    //" << inst->getLineNo() << ":$" << inst->getCISAOff());
        DEBUG_VERBOSE(std::endl);
#endif

        // Any physical registers defined will be marked available if
        // current inst is first def or if complete region is written
        G4_DstRegRegion* dst = inst->getDst();

        if (dst &&
            dst->getBase()->isRegVar())
        {
            LocalLiveRange* localLR = NULL;
            G4_Declare* topdcl = GetTopDclFromRegRegion(dst);
            unsigned int t;

            if (topdcl)
                localLR = gra.getLocalLR(topdcl);

            if (localLR && localLR->getAssigned() && !localLR->isEOT())
            {
                int reg, sreg, numrows;
                G4_VarBase* preg = localLR->getPhyReg(sreg);
                numrows = localLR->getTopDcl()->getNumRows();

                MUST_BE_TRUE(preg->isGreg(), "Register in dst was not GRF");

                reg = preg->asGreg()->getRegNum();

                // Check whether the dst physical register is busy/available.
                // If it is available, and we still see a def that means there was no
                // corresponding use. In such cases mark the physical register as
                // busy, so interference building can take place correctly.
                for (int j = reg; j < reg + numrows; j++)
                {
                    int k = getGRFDclForHRA(j)->getRegVar()->getId();

                    if (cur.isSet(j) == true)
                    {
                        buildInterferenceWithLive(live, k);
#ifdef DEBUG_VERBOSE_ON
                        DEBUG_VERBOSE("Found no use for r" << j << ".0 so marking it as interfering with live set" << std::endl);
#endif
                    }
                }

                if ((localLR->getFirstRef(t) == inst) ||
                    liveAnalysis->writeWholeRegion(bb, inst, dst, builder.getOptions()))
                {
                    // Last row may be only partially used by the current dcl
                    // so we still need to pessimistically mark last range as
                    // busy. Because some other src opnd that is live may still
                    // be using the remaining GRF.
                    if (localLR->getSizeInWords() % NUM_WORDS_PER_GRF != 0)
                        numrows--;

                    for (int j = reg; j < reg + numrows; j++)
                    {
                        cur.set(j, true);
#ifdef DEBUG_VERBOSE_ON
                        DEBUG_VERBOSE("Setting r" << j << ".0 available" << std::endl);
#endif
                    }

                    // Build interference only for point ranges, ideally which shouldnt exist
                    // These are ranges that have a def, but no use
                    if (localLR->getFirstRef(t) == localLR->getLastRef(t))
                    {
                        for (int j = reg; j < reg + localLR->getTopDcl()->getNumRows(); j++)
                        {
                            int k = getGRFDclForHRA(j)->getRegVar()->getId();
                            buildInterferenceWithLive(live, k);
                        }
                    }
                }
            }
            else if (dst->getBase()->isRegAllocPartaker()) {
                // Global range

                // In bottom-up order if the live-range has not started then
                // a use was not seen for this def. But we need to ensure this
                // variable interferes with all other live vars.
                bool isPointRange = !live.isSet(dst->getBase()->asRegVar()->getId());

                if (isPointRange)
                {
                    // Mark interference with all busy physical registers
                    for (unsigned int i = 0; i < kernel.getNumRegTotal(); i++)
                    {
                        if (cur.isSet(i) == false)
                        {
                            int k = getGRFDclForHRA(i)->getRegVar()->getId();
                            checkAndSetIntf(dst->getBase()->asRegVar()->getId(), k);
                        }
                    }
                }

                if (liveAnalysis->writeWholeRegion(bb, inst, dst, builder.getOptions()) ||
                    inst->isPseudoKill())
                {
                    // Whole write or first def found so mark this operand as not live for earlier instructions
                    auto id = dst->getBase()->asRegVar()->getId();
                    updateLiveness(live, id, false);
                }
            }
        }

        // Any physical registers used by src opnds will be busy before the current inst
        for (int i = 0; i < G4_MAX_SRCS; i++)
        {
            G4_Operand* src = inst->getSrc(i);

            if (src &&
                src->isSrcRegRegion() &&
                src->asSrcRegRegion()->getBase()->isRegVar())
            {
                LocalLiveRange* localLR = NULL;
                G4_Declare* topdcl = GetTopDclFromRegRegion(src);

                if (topdcl)
                    localLR = gra.getLocalLR(topdcl);

                if (localLR && localLR->getAssigned() && !localLR->isEOT())
                {
                    int reg, sreg, numrows;
                    G4_VarBase* preg = localLR->getPhyReg(sreg);
                    numrows = localLR->getTopDcl()->getNumRows();

                    MUST_BE_TRUE(preg->isGreg(), "Register in src was not GRF");

                    reg = preg->asGreg()->getRegNum();

                    for (int j = reg; j < reg + numrows; j++)
                    {
                        int k = getGRFDclForHRA(j)->getRegVar()->getId();

                        if (cur.isSet(j) == true)
                        {
                            // G4_RegVar with id k was marked free, but becomes
                            // busy at this instruction. For incremental updates
                            // push this to a vector and use it while updating
                            // interference graph incrementally.
                            curUpdate.push_back(k);
                        }

                        cur.set(j, false);
#ifdef DEBUG_VERBOSE_ON
                        DEBUG_VERBOSE("Setting r" << j << ".0 busy" << std::endl);
#endif
                    }
                }
                else if (src->asSrcRegRegion()->getBase()->isRegAllocPartaker())
                {
                    if (live.isSet(src->asSrcRegRegion()->getBase()->asRegVar()->getId()) == false)
                        update = true;

                    // Mark operand as live from this inst upwards
                    auto id = src->asSrcRegRegion()->getBase()->asRegVar()->getId();
                    updateLiveness(live, id, true);
                }
            }
        }

        if (update == true)
        {
            // Mark interference with all live
            for (unsigned int i = 0; i < kernel.getNumRegTotal(); i++)
            {
                if (cur.isSet(i) == false)
                {
                    int k = getGRFDclForHRA(i)->getRegVar()->getId();
                    buildInterferenceWithLive(live, k);
                }
            }
        }
        else {
            if (curUpdate.size() > 0)
            {
                // Perform incremental update. This code is executed when:
                // 1) live set is unchanged, ie no new global range was started in inst
                // 2) cur set has changed, ie an earlier free GRF has become busy
                // Any new busy GRFs will have to be marked as interfering with
                // currently live-ranges. There is no need to iterate over all
                // busy GRFs. Instead only those GRFs that have got busy in this iteration
                // can be considered for incremental updates.
                for (std::vector<int>::iterator it = curUpdate.begin(), end = curUpdate.end();
                    it != end;
                    it++)
                {
                    int k = (*it);

                    buildInterferenceWithLive(live, k);
                }
            }
        }
    }

    for (unsigned int i = 0; i < maxId; i++)
    {
        bool isLiveIn = liveAnalysis->isLiveAtEntry(bb, i);
        bool isLiveOut = liveAnalysis->isLiveAtExit(bb, i);
        bool isKilled = liveAnalysis->use_kill[bb->getId()].isSet(i);
        bool isAddrSensitive = liveAnalysis->isAddressSensitive(i);
        bool assigned = (lrs[i]->getVar()->getPhyReg() != NULL);

        // If a range is Address taken AND (live-in or live-out or killed)
        // mark it to interfere with all physical registers used by local RA
        // FIXME: need to check if this is actually needed
        if (!assigned && (isAddrSensitive && (isLiveIn || isLiveOut || isKilled)))
        {
            // Make it to interfere with all physical registers used in the BB
            for (uint32_t j = 0, numReg = kernel.getNumRegTotal(); j < numReg; j++)
            {
                if (LRASummary->isGRFBusy(j))
                {
                    int k = getGRFDclForHRA(j)->getRegVar()->getId();
                    checkAndSetIntf(i, k);
                }
            }
        }
    }
}


void Interference::interferenceVerificationForSplit() const
{

    std::cout << "\n\n **** Interference Verificaion Table ****\n";
    for (unsigned i = maxId; i < maxId; i++)
    {
        std::cout << "(" << i << ") ";
        //lrs[i]->dump();
        for (unsigned j = 0; j < maxId; j++)
        {
            if (interfereBetween(i, j))
            {
                if (!interfereBetween(gra.getSplittedDeclare(lrs[i]->getDcl())->getRegVar()->getId(), j) &&
                    (gra.getSplittedDeclare(lrs[i]->getDcl()) != lrs[j]->getDcl()))
                {
                    std::cout << "\t";
                    lrs[j]->getVar()->emit(std::cout);
                }
            }
        }
        std::cout << "\n";
    }
}

void Interference::dumpInterference() const
{

    std::cout << "\n\n **** Interference Table ****\n";
    for (unsigned i = 0; i < maxId; i++)
    {
        std::cout << "(" << i << ") ";
        lrs[i]->dump();
        std::cout << "\n";
        for (unsigned j = 0; j < maxId; j++)
        {
            if (interfereBetween(i, j))
            {
                std::cout << "\t";
                lrs[j]->getVar()->emit(std::cout);
            }
        }
        std::cout << "\n";
    }
}

GraphColor::GraphColor(LivenessAnalysis& live, unsigned totalGRF, bool hybrid, bool forceSpill_) :
    gra(live.gra), isHybrid(hybrid), totalGRFRegCount(totalGRF), numVar(live.getNumSelectedVar()), numSplitStartID(live.getNumSplitStartID()), numSplitVar(live.getNumSplitVar()),
    intf(&live, lrs, live.getNumSelectedVar(), live.getNumSplitStartID(), live.getNumSplitVar(), gra), regPool(gra.regPool),
    builder(gra.builder), lrs(NULL),
    forceSpill(forceSpill_), mem(GRAPH_COLOR_MEM_SIZE),
    liveAnalysis(live), kernel(gra.kernel)
{
    oddTotalDegree = 1;
    evenTotalDegree = 1;
    oddTotalRegNum = 1;
    evenTotalRegNum = 1;
    oddMaxRegNum = 1;
    evenMaxRegNum = 1;
    spAddrRegSig = (unsigned*)mem.alloc(getNumAddrRegisters() * sizeof(unsigned));
    m_options = builder.getOptions();
}

//
// lrs[i] gives the live range whose id is i
//
void GraphColor::createLiveRanges(unsigned reserveSpillSize)
{
    lrs = (LiveRange**)mem.alloc(sizeof(LiveRange*)*numVar);
    bool hasStackCall = builder.kernel.fg.getHasStackCalls() || builder.kernel.fg.getIsStackCallFunc();
    // Modification For Alias Dcl
    for (auto dcl : gra.kernel.Declares)
    {
        G4_RegVar* var = dcl->getRegVar();
        // Do not include alias var in liverange creation
        if (!var->isRegAllocPartaker() || dcl->getAliasDeclare() != NULL)
        {
            continue;
        }
        lrs[var->getId()] = new (mem)LiveRange(var, this->gra);
        unsigned int reservedGRFNum = m_options->getuInt32Option(vISA_ReservedGRFNum);

        if (builder.kernel.fg.isPseudoDcl(dcl))
        {
            lrs[var->getId()]->setIsPseudoNode();
        }
        if (dcl->getIsPartialDcl())
        {
            G4_Declare * parentDcl = this->gra.getSplittedDeclare(dcl);
            lrs[var->getId()]->setParentLRID(parentDcl->getRegVar()->getId());
            lrs[var->getId()]->setIsPartialDcl();
        }
        if (dcl->getIsSplittedDcl())
        {
            lrs[var->getId()]->setIsSplittedDcl(true);
        }
        lrs[var->getId()]->setBC(gra.getBankConflict(dcl));

        lrs[var->getId()]->allocForbidden(mem, hasStackCall, reserveSpillSize, reservedGRFNum);
        lrs[var->getId()]->setCallerSaveBias(hasStackCall);
        G4_Declare* varDcl = lrs[var->getId()]->getDcl();
        if (builder.kernel.fg.isPseudoVCADcl(varDcl))
        {
            lrs[var->getId()]->allocForbiddenCallerSave(mem, &builder.kernel);
        }
        else if (builder.kernel.fg.isPseudoVCEDcl(varDcl))
        {
            lrs[var->getId()]->allocForbiddenCalleeSave(mem, &builder.kernel);
        }
        else if (varDcl == gra.getOldFPDcl())
        {
            lrs[var->getId()]->allocForbiddenCallerSave(mem, &builder.kernel);
        }
    }
}

void GraphColor::computeDegreeForGRF()
{
    for (unsigned i = 0; i < numVar; i++)
    {
        unsigned degree = 0;

        if (!(lrs[i]->getIsPseudoNode()) &&
            !(lrs[i]->getIsPartialDcl()))
        {
            std::vector<unsigned int>& intfs = intf.getSparseIntfForVar(i);
            unsigned bankDegree = 0;
            auto lraBC = lrs[i]->getBC();
            bool isOdd = (lraBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                lraBC == BANK_CONFLICT_SECOND_HALF_ODD);

            for (auto it : intfs)
            {
                if (!lrs[it]->getIsPartialDcl())
                {
                    unsigned edgeDegree = edgeWeightGRF(lrs[i], lrs[it]);

                    degree += edgeDegree;

                    auto lrsitBC = lrs[it]->getBC();
                    bool isOddBC = (lrsitBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                                              lrsitBC == BANK_CONFLICT_SECOND_HALF_ODD);

                    if ((isOdd && isOddBC) ||
                        (!isOdd && !isOddBC))
                    {
                        bankDegree += edgeDegree;
                    }
                }
            }

            if (isOdd)
            {
                oddTotalDegree += bankDegree; //MAX(bankDegree, oddMaxDegree);
                oddTotalRegNum += lrs[i]->getNumRegNeeded();
                oddMaxRegNum = MAX(oddMaxRegNum, lrs[i]->getNumRegNeeded());
            }
            else
            {
                evenTotalDegree += bankDegree; //MAX(bankDegree, evenMaxDegree);
                evenTotalRegNum += lrs[i]->getNumRegNeeded();
                evenMaxRegNum = MAX(evenMaxRegNum, lrs[i]->getNumRegNeeded());
            }
        }

        lrs[i]->setDegree(degree);
    }
}

void GraphColor::computeDegreeForARF()
{
    for (unsigned i = 0; i < numVar; i++)
    {
        unsigned degree = 0;

        if (!(lrs[i]->getIsPseudoNode()))
        {
            std::vector<unsigned int>& intfs = intf.getSparseIntfForVar(i);
            for (auto it : intfs)
            {
                degree += edgeWeightARF(lrs[i], lrs[it]);
            }
        }

        lrs[i]->setDegree(degree);
    }
}

void GraphColor::computeSpillCosts(bool useSplitLLRHeuristic)
{
    std::vector <LiveRange *> addressSensitiveVars;
    float maxNormalCost = 0.0f;

    for (unsigned i = 0; i < numVar; i++)
    {
        G4_Declare* dcl = lrs[i]->getDcl();

        if (dcl->getIsPartialDcl())
        {
            continue;
        }
        //
        // The spill cost of pseudo nodes inserted to aid generation of save/restore code
        // must be the minimum so that such nodes go to the bootom of the color stack.
        //
        if (builder.kernel.fg.isPseudoDcl(dcl))
        {
            if (builder.kernel.fg.isPseudoVCADcl(dcl))
            {
                lrs[i]->setSpillCost(MINSPILLCOST + 1);
            }
            else
            {
                lrs[i]->setSpillCost(MINSPILLCOST);
            }
        }

        auto dclLR = gra.getLocalLR(dcl);
        if (dclLR != NULL &&
            dclLR->getSplit())
        {
            lrs[i]->setSpillCost(MINSPILLCOST + 2);
        }
        //
        // Give the tiny spill/fill ranges an infinite spill cost, so that they are
        // picked first for coloring.
        // Also ARF live ranges with exclusively sequential references within the code are
        // assigned an infinite spill cost as spilling them will not lower the register
        // pressure in the region they are referenced. This does not necessarily hold for
        // GRF/MRF live ranges are these are potentially large in size but the portions
        // accessed by each sequential use are limited to 2 registers for general instructions
        // and 8 registers for SEND instructions.
        //
        else if (gra.isAddrFlagSpillDcl(dcl) ||
            lrs[i]->isRetIp() ||
            lrs[i]->getIsInfiniteSpillCost() == true ||
            ((lrs[i]->getVar()->isRegVarTransient() == true ||
                lrs[i]->getVar()->isRegVarTmp() == true) &&
                lrs[i]->getVar()->isSpilled() == false) ||
            dcl == gra.getOldFPDcl() ||
            (m_options->getOption(vISA_enablePreemption) &&
                dcl == builder.getBuiltinR0()))
        {
            lrs[i]->setSpillCost(MAXSPILLCOST);
        }
        else if (dcl->isDoNotSpill())
        {
            lrs[i]->setSpillCost(MAXSPILLCOST);
        }
        //
        // Calculate spill costs of regular nodes.
        //
        else
        {
            float spillCost;
            // NOTE: Add 1 to degree to avoid divide-by-0.
            if (m_options->getTarget() == VISA_3D)
            {
                if (useSplitLLRHeuristic)
                {
                    spillCost = 1.0f*lrs[i]->getRefCount() / (lrs[i]->getDegree() + 1);
                }
                else
                {
                    spillCost = 1.0f*lrs[i]->getRefCount()*lrs[i]->getRefCount()*lrs[i]->getDcl()->getByteSize()*
                        (float)sqrt(lrs[i]->getDcl()->getByteSize())
                        / (float)sqrt(lrs[i]->getDegree() + 1);
                }
            }
            else
            {
                spillCost =
                    liveAnalysis.livenessClass(G4_GRF) ?
                    lrs[i]->getDegree() : 1.0f*lrs[i]->getRefCount()*lrs[i]->getRefCount() / (lrs[i]->getDegree() + 1);
            }

            lrs[i]->setSpillCost(spillCost);

            // Track address sensitive live range.
            if (liveAnalysis.isAddressSensitive(i))
            {
                addressSensitiveVars.push_back(lrs[i]);
            }
            else
            {
                // Set the spill cost of all other normal live ranges, and
                // track the max normal cost.
                if (maxNormalCost < spillCost)
                {
                    maxNormalCost = spillCost;
                }
            }
        }
    }

    //
    // Set the spill cost of address sensitive live ranges above all the
    // normal live ranges, so that they get colored before all the normal
    // live ranges.
    //
    std::vector <LiveRange *> ::iterator it = addressSensitiveVars.begin();
    std::vector <LiveRange *> ::iterator itEnd = addressSensitiveVars.end();

    for (; it != itEnd; ++it)
    {
        if ((*it)->getSpillCost() != MAXSPILLCOST)
        {
            (*it)->setSpillCost(maxNormalCost + (*it)->getSpillCost());
        }
    }
}


//
// subtract lr's neighbors that are still in work list
//
void GraphColor::relaxNeighborDegreeGRF(LiveRange* lr)
{
    if (!(lr->getIsPseudoNode()) &&
        !(lr->getIsPartialDcl()))
    {
        unsigned lr_id = lr->getVar()->getId();
        std::vector<unsigned int>& intfs = intf.getSparseIntfForVar(lr_id);
        for (auto it : intfs)
        {
            LiveRange* lrs_it = lrs[it];

            if (lrs_it->getActive() &&
                !lrs_it->getIsPseudoNode() &&
                !(lrs_it->getIsPartialDcl()))
            {
                unsigned w = edgeWeightGRF(lrs_it, lr);

#ifdef DEBUG_VERBOSE_ON
                DEBUG_VERBOSE("\t relax ");
                lrs_it->dump();
                DEBUG_VERBOSE(" degree(" << lrs_it->getDegree() << ") - " << w << std::endl);
#endif
                lrs_it->subtractDegree(w);

                unsigned availColor = numColor;
                availColor = numColor - lrs_it->getNumForbidden();

                if (lrs_it->getDegree() + lrs_it->getNumRegNeeded() <= availColor)
                {
                    unconstrainedWorklist.push_back(lrs_it);
                    lrs_it->setActive(false);
                }
            }
        }
    }
}
void GraphColor::relaxNeighborDegreeARF(LiveRange* lr)
{
    if (!(lr->getIsPseudoNode()))
    {
        unsigned lr_id = lr->getVar()->getId();
        std::vector<unsigned int>& intfs = intf.getSparseIntfForVar(lr_id);
        for (auto it : intfs)
        {
            LiveRange* lrs_it = lrs[it];

            if (lrs_it->getActive() &&
                !lrs_it->getIsPseudoNode())
            {
                unsigned w = edgeWeightARF(lrs_it, lr);

#ifdef DEBUG_VERBOSE_ON
                DEBUG_VERBOSE("\t relax ");
                lrs_it->dump();
                DEBUG_VERBOSE(" degree(" << lrs_it->getDegree() << ") - " << w << std::endl);
#endif
                lrs_it->subtractDegree(w);

                unsigned availColor = numColor;

                if (lrs_it->getDegree() + lrs_it->getNumRegNeeded() <= availColor)
                {
                    unconstrainedWorklist.push_back(lrs_it);
                    lrs_it->setActive(false);
                }
            }
        }
    }
}


static bool compareSpillCost(LiveRange* lr1, LiveRange* lr2)
{
    return lr1->getSpillCost() < lr2->getSpillCost() ||
        (lr1->getSpillCost() == lr2->getSpillCost() && lr1->getVar()->getId() < lr2->getVar()->getId());
}

//
// All nodes in work list are all contrained (whose degree > max color)
// find one contrained node and move it to order list
//
void GraphColor::removeConstrained()
{
    if (!constrainedWorklist.empty())
    {
        LiveRange* lr = constrainedWorklist.front();
        constrainedWorklist.pop_front();

        if (lr->getActive())
        {

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE(".... Remove Constrained ");
            lr->dump();
            DEBUG_VERBOSE(std::endl);
#endif

            if (liveAnalysis.livenessClass(G4_GRF))
            {
                relaxNeighborDegreeGRF(lr);
            }
            else
            {
                relaxNeighborDegreeARF(lr);
            }
            colorOrder.push_back(lr);
            lr->setActive(false);
        }
    }
}


void GraphColor::determineColorOrdering()
{
    numColor = 0;
    if (liveAnalysis.livenessClass(G4_GRF))
        numColor = totalGRFRegCount;
    else if (liveAnalysis.livenessClass(G4_ADDRESS))
        numColor = getNumAddrRegisters();
    else if (liveAnalysis.livenessClass(G4_FLAG))
        numColor = builder.getNumFlagRegisters();

    unsigned numUnassignedVar = liveAnalysis.getNumUnassignedVar();

    //
    // create an array for sorting live ranges
    //
    std::vector<LiveRange*> sorted;
    sorted.reserve(numUnassignedVar);
    unsigned j = 0;
    for (unsigned i = 0; i < numVar; i++)
    {
        if (lrs[i]->getPhyReg() == nullptr && !lrs[i]->getIsPartialDcl())
        {
            sorted.push_back(lrs[i]);
            j++;
        }
    }
    MUST_BE_TRUE(j == numUnassignedVar, ERROR_GRAPHCOLOR);

    //
    // sort the live range array
    //
    std::sort(sorted.begin(), sorted.end(), compareSpillCost);

    for (unsigned i = 0; i < numUnassignedVar; i++)
    {
        LiveRange* lr = sorted[i];
        unsigned availColor = numColor;
        availColor = numColor - lr->getNumForbidden();

        if (lr->getDegree() + lr->getNumRegNeeded() <= availColor)
        {
            unconstrainedWorklist.push_back(lr);
            lr->setActive(false);
        }
        else
        {
            constrainedWorklist.push_back(lr);
            lr->setActive(true);
        }
    }

#ifdef DEBUG_VERBOSE_ON
    DEBUG_VERBOSE("\nSPILL COST" << std::endl);
    for (unsigned i = 0; i < numUnassignedVar; i++)
    {
        sorted[i]->dump();
        DEBUG_VERBOSE("\t spillCost=" << sorted[i]->getSpillCost());
        DEBUG_VERBOSE("\t degree=" << sorted[i]->getDegree());
        DEBUG_VERBOSE("\t refCnt=" << sorted[i]->getRefCount());
        DEBUG_VERBOSE("\t size=" << sorted[i]->getDcl()->getByteSize());
        DEBUG_VERBOSE(std::endl);
    }
    DEBUG_VERBOSE(std::endl);
#endif

    while (!constrainedWorklist.empty() ||
        !unconstrainedWorklist.empty())
    {
        while (!unconstrainedWorklist.empty())
        {
            LiveRange* lr = unconstrainedWorklist.front();
            unconstrainedWorklist.pop_front();

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE(".... Remove Unconstrained ");
            lr->dump();
            DEBUG_VERBOSE(std::endl);
#endif

            if (liveAnalysis.livenessClass(G4_GRF))
            {
                relaxNeighborDegreeGRF(lr);
            }
            else
            {
                relaxNeighborDegreeARF(lr);
            }
            colorOrder.push_back(lr);
        }

        removeConstrained();
    }
}

void PhyRegUsage::updateRegUsage(LiveRange* lr)
{
    G4_Declare* dcl = lr->getDcl();
    G4_VarBase* pr = NULL;
    if (lr->getIsPartialDcl())
    {
        pr = lrs[lr->getParentLRID()]->getPhyReg();
    }
    else
    {
        pr = lr->getPhyReg();
    }

    if (!pr)
    {
        return;
    }
    if (pr->isGreg())
    {
        if (dcl->getIsPartialDcl())
        {
            //Assumptions:
            // 1. the offset of the sub declare must be G4_WSIZE aligned
            // 2. the size of the subdeclare must be G4_WSIZE aligned
            markBusyForDclSplit(G4_GRF,
                ((G4_Greg*)pr)->getRegNum(),
                (lrs[lr->getParentLRID()]->getPhyRegOff() * G4_Type_Table[dcl->getElemType()].byteSize + gra.getSubOffset(dcl)) / G4_WSIZE,
                dcl->getByteSize() / G4_WSIZE,
                dcl->getNumRows());
        }
        else
        {
            markBusyGRF(((G4_Greg*)pr)->getRegNum(),
                PhyRegUsage::offsetAllocUnit(lr->getPhyRegOff(), dcl->getElemType()),
                dcl->getWordSize(),
                dcl->getNumRows());
        }
    }
    else if (pr->isFlag())
    {
        auto flagWordOffset = lr->getPhyReg()->asAreg()->getFlagNum() * 2;
        markBusyFlag(0,
            PhyRegUsage::offsetAllocUnit(
                flagWordOffset + lr->getPhyRegOff(),
                dcl->getElemType()),
            PhyRegUsage::numAllocUnit(dcl->getNumElems(), dcl->getElemType()),
            dcl->getNumRows());
    }
    else if (pr->isAreg())
    {
        markBusyAddress(0,
            PhyRegUsage::offsetAllocUnit(lr->getPhyRegOff(), dcl->getElemType()),
            PhyRegUsage::numAllocUnit(dcl->getNumElems(), dcl->getElemType()),
            dcl->getNumRows());
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_GRAPHCOLOR); // un-handled reg type
    }
}

bool GraphColor::assignColors(ColorHeuristic colorHeuristicGRF, bool doBankConflict, bool highInternalConflict)
{
    if (builder.getOption(vISA_RATrace))
    {
        std::cout << "\t--" << (colorHeuristicGRF == ROUND_ROBIN ? "round-robin" : "first-fit") <<
            (doBankConflict ? " BCR" : "") << " graph coloring\n";
    }

    unsigned startARFReg = 0;
    unsigned startFLAGReg = 0;
    unsigned startGRFReg = 0;
    unsigned bank1_end = 0;
    unsigned bank2_end = totalGRFRegCount - 1;
    unsigned bank1_start = 0;
    unsigned bank2_start = totalGRFRegCount - 1;
    unsigned int totalGRFNum = kernel.getNumRegTotal();
    bool oneGRFBankDivision = gra.kernel.fg.builder->oneGRFBankDivision();
    bool allocFromBanks = liveAnalysis.livenessClass(G4_GRF) && builder.lowHighBundle() &&
        !builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
        doBankConflict &&
        ((oneGRFBankDivision && gra.kernel.getSimdSize() >= 16) || (!oneGRFBankDivision && highInternalConflict));

    if (allocFromBanks &&
        (colorHeuristicGRF == ROUND_ROBIN))
    {
        bank1_end = (unsigned)((totalGRFRegCount - 1) * (((float)evenTotalDegree / evenTotalRegNum) / (((float)evenTotalDegree / evenTotalRegNum) + ((float)oddTotalDegree / oddTotalRegNum))));
        if (bank1_end < evenMaxRegNum ||
            totalGRFRegCount - bank1_end < oddMaxRegNum ||
            bank1_end == totalGRFRegCount - 1 ||
            bank1_end == 0)
        {
            return false;
        }

        bank2_end = bank1_end + 1;
    }

    bool* availableGregs = (bool *)mem.alloc(sizeof(bool)* totalGRFNum);
    uint32_t* availableSubRegs = (uint32_t *)mem.alloc(sizeof(uint32_t)* totalGRFNum);
    bool* availableAddrs = (bool *)mem.alloc(sizeof(bool)* getNumAddrRegisters());
    bool* availableFlags = (bool *)mem.alloc(sizeof(bool)* builder.getNumFlagRegisters());
    uint8_t* weakEdgeUsage = (uint8_t*)mem.alloc(sizeof(uint8_t)*totalGRFNum);
    G4_RegFileKind rFile = G4_GRF;
    if (liveAnalysis.livenessClass(G4_FLAG))
        rFile = G4_FLAG;
    else if (liveAnalysis.livenessClass(G4_ADDRESS))
        rFile = G4_ADDRESS;

    unsigned maxGRFCanBeUsed = totalGRFRegCount;
    PhyRegUsageParms parms(gra, lrs, rFile, maxGRFCanBeUsed, startARFReg, startFLAGReg, startGRFReg, bank1_start, bank1_end, bank2_start, bank2_end,
        doBankConflict, availableGregs, availableSubRegs, availableAddrs, availableFlags, weakEdgeUsage);
    bool noIndirForceSpills = builder.getOption(vISA_NoIndirectForceSpills);

    // colorOrder is in reverse order (unconstrained at front)
    for (auto iter = colorOrder.rbegin(), iterEnd = colorOrder.rend(); iter != iterEnd; ++iter)
    {
        LiveRange* lr = *iter;
        auto lrVar = lr->getVar();

        //
        // assign regiser to live ranges
        //
        if (lr->getPhyReg() == NULL && !lrVar->isSpilled() && !lr->getIsPartialDcl()) // no assigned register yet and not spilled
        {
            unsigned lr_id = lrVar->getId();
            //
            // compute what registers are already assigned
            //
            PhyRegUsage regUsage(parms);

            std::vector<unsigned int>& intfs = intf.getSparseIntfForVar(lr_id);
            auto weakEdgeSet = intf.getCompatibleSparseIntf(lrVar->getDeclare()->getRootDeclare());

            for (auto it : intfs)
            {
                LiveRange* lrTemp = lrs[it];
                if (lrTemp->getPhyReg() != nullptr || lrTemp->getIsPartialDcl())
                {
                    if (lrTemp->getIsSplittedDcl())  //Only interfere with children declares
                    {
                        continue;
                    }

                    regUsage.updateRegUsage(lrTemp);
                }
            }

            if (weakEdgeSet)
            {
                regUsage.runOverlapTest(true);
                for (auto weakDcl : *weakEdgeSet)
                {
                    auto regVar = weakDcl->getRootDeclare()->getRegVar();
                    unsigned int pvar = 0, numRegs = 0;
                    if (regVar->isPhyRegAssigned())
                    {
                        // This branch will be taken for dcls assigned
                        // regs by LRA.
                        pvar = regVar->getPhyReg()->asGreg()->getRegNum();
                        numRegs = weakDcl->getNumRows();
                    }
                    else
                    {
                        // For dcls not assigned regs by LRA, lookup temp
                        // registers assigned to LiveRange instances.
                        auto id = regVar->getId();
                        auto lr = lrs[id];
                        auto phyReg = lr->getPhyReg();
                        if (phyReg)
                        {
                            pvar = phyReg->asGreg()->getRegNum();
                            numRegs = weakDcl->getNumRows();
                        }
                    }

                    // For now it is assumed only 8-byte types will appear
                    // here. If other sized types will also appear then
                    // augmentation mask also needs to be sent in
                    // weak edge data structure below.
                    for (unsigned int r = pvar; r < (pvar + numRegs); r++)
                    {
                        auto use = regUsage.getWeakEdgeUse(r);
                        if (use == 0 || use == (r - pvar + 1))
                        {
                            regUsage.setWeakEdgeUse(r, r - pvar + 1);
                        }
                        else
                        {
                            // Indiates two neighbors use a physical
                            // register with different overlap.
                            regUsage.setWeakEdgeUse(r, 0xff);
                        }
                    }
                }
            }

            ColorHeuristic heuristic = colorHeuristicGRF;


            bool failed_alloc = false;
            G4_Declare* dcl = lrVar->getDeclare();

            if (!(noIndirForceSpills &&
                liveAnalysis.isAddressSensitive(lr_id)) &&
                forceSpill &&
                (dcl->getRegFile() == G4_GRF || dcl->getRegFile() == G4_FLAG) &&
                lr->getRefCount() != 0 &&
                lr->getSpillCost() != MAXSPILLCOST)
            {
                failed_alloc = true;
            }

            if (dcl->getNumRows() > totalGRFNum)
            {
                // we sure as hell won't get an assignment
                failed_alloc = true;
            }

            if (!failed_alloc)
            {
                BankAlign align = gra.isEvenAligned(lrVar->getDeclare()) ? BankAlign::Even : BankAlign::Either;
                if (allocFromBanks)
                {
                    
                    if (!isHybrid && oneGRFBankDivision)
                    {
                        gra.getBankAlignment(lr, align);
                    }
                    failed_alloc |= !regUsage.assignGRFRegsFromBanks(lr, align, lr->getForbidden(),
                        heuristic, oneGRFBankDivision);
                }
                else
                {
                    failed_alloc |= !regUsage.assignRegs(highInternalConflict, lr, lr->getForbidden(),
                        align, gra.getSubRegAlign(lrVar->getDeclare()), heuristic, lr->getSpillCost());
                }
            }

            //
            // assign unused color
            //
            if (failed_alloc)
            {
                //
                // for GRF register assignment, if we are performing round-robin (1st pass) then abort on spill
                //
                if ((heuristic == ROUND_ROBIN || doBankConflict) &&
                    (lr->getRegKind() == G4_GRF || lr->getRegKind() == G4_FLAG))
                {
                    return false;
                }
                else if (kernel.fg.isPseudoVCADcl(dcl) || kernel.fg.isPseudoVCEDcl(dcl) ||
                    kernel.fg.isPseudoA0Dcl(dcl) || kernel.fg.isPseudoFlagDcl(dcl))
                {
                    // these pseudo dcls are not (and cannot be) spilled, but instead save/restore code will
                    // be inserted in stack call prolog/epilog
                }
                else
                {
                    // for first-fit register assignment track spilled live ranges
                    spilledLRs.push_back(lr);
                }
            }
        }
#ifdef DEBUG_VERBOSE_ON
        lr->dump();
        COUT_ERROR << std::endl;
#endif
    }

    // record RA type
    if (liveAnalysis.livenessClass(G4_GRF))
    {
        if (colorHeuristicGRF == ROUND_ROBIN)
        {
            kernel.setRAType(doBankConflict ? RA_Type::GRAPH_COLORING_RR_BC_RA : RA_Type::GRAPH_COLORING_RR_RA);
        }
        else
        {
            kernel.setRAType(doBankConflict ? RA_Type::GRAPH_COLORING_FF_BC_RA : RA_Type::GRAPH_COLORING_FF_RA);
        }
    }

    return true;
}

template <class REGION_TYPE>
unsigned GlobalRA::getRegionDisp(
    REGION_TYPE * region
)
{
    unsigned rowOffset = G4_GRF_REG_NBYTES * region->getRegOff();
    unsigned columnOffset = region->getSubRegOff() * region->getElemSize();
    return rowOffset + columnOffset;
}

unsigned GlobalRA::getRegionByteSize(
    G4_DstRegRegion * region,
    unsigned          execSize
)
{
    unsigned size = region->getHorzStride() * region->getElemSize() *
        (execSize - 1) + region->getElemSize();

    return size;
}

unsigned GlobalRA::owordMask()
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 4;
    return mask;
}

bool GlobalRA::owordAligned(
    unsigned offset
)
{
    return (offset & owordMask()) == offset;
}

#define OWORD_BYTE_SIZE 16

template <class REGION_TYPE>
bool GlobalRA::isUnalignedRegion(
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned regionDisp = getRegionDisp(region);
    unsigned regionByteSize = getRegionByteSize(region, execSize);

    if (regionDisp%G4_GRF_REG_NBYTES == 0 && regionByteSize%G4_GRF_REG_NBYTES == 0)
    {
        return
            regionByteSize / G4_GRF_REG_NBYTES != 1 &&
            regionByteSize / G4_GRF_REG_NBYTES != 2 &&
            regionByteSize / G4_GRF_REG_NBYTES != 4;
    }
    return true;

}

bool GlobalRA::shouldPreloadDst(
    G4_DstRegRegion * spilledRangeRegion,
    uint8_t           execSize,
    G4_INST *         instContext,
    G4_BB*            curBB
)
{
    // Check for partial and unaligned regions and add pre-load code, if
    // necessary.
    if (isPartialRegion(spilledRangeRegion, execSize) ||
        isUnalignedRegion(spilledRangeRegion, execSize) ||
        isPartialContext(spilledRangeRegion, instContext, curBB->isInSimdFlow())) {
        return true;
    }
    // No pre-load for whole and aligned region writes
    else {
        return false;
    }
}

void GlobalRA::determineSpillRegSize(unsigned& spillRegSize, unsigned& indrSpillRegSize)
{
    // Iterate over all BBs
    for (auto curBB : kernel.fg)
    {
        // Iterate over all insts
        for (INST_LIST_ITER inst_it = curBB->begin(), iend = curBB->end(); inst_it != iend; ++inst_it)
        {
            unsigned currentSpillRegSize = 0;
            unsigned currentIndrSpillRegSize = 0;

            G4_INST* curInst = (*inst_it);
            uint8_t execSize = curInst->getExecSize();

            if (curInst->isPseudoKill() ||
                curInst->opcode() == G4_pseudo_lifetime_end ||
                curInst->opcode() == G4_pseudo_fcall ||
                curInst->opcode() == G4_pseudo_fret)
            {
                continue;
            }

            if (curInst->isSend())
            {
                G4_SendMsgDescriptor* msgDesc = curInst->getMsgDesc();

                unsigned dstSpillRegSize = 0;
                dstSpillRegSize = msgDesc->ResponseLength();

                unsigned src0FillRegSize = 0;
                src0FillRegSize = msgDesc->MessageLength();

                unsigned src1FillRegSize = 0;
                if (curInst->isSplitSend())
                {
                    src1FillRegSize = msgDesc->extMessageLength();
                }

                if (!kernel.fg.builder->useSends())
                {
                    dstSpillRegSize++;
                }

                currentSpillRegSize = dstSpillRegSize + src0FillRegSize + src1FillRegSize;
            }
            else
            {
                REGVAR_VECTOR indrVars;

                unsigned dstSpillRegSize = 0;
                unsigned indrDstSpillRegSize = 0;
                if (G4_Inst_Table[curInst->opcode()].n_dst == 1)
                {
                    G4_DstRegRegion* dst = curInst->getDst();

                    if (dst &&
                        dst->getBase()->isRegVar())
                    {
                        if (dst->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_GRF)
                        {
                            if (dst->isCrossGRFDst())
                            {
                                dstSpillRegSize = 2;
                            }
                            else
                            {
                                dstSpillRegSize = 1;
                            }

                            if (shouldPreloadDst(dst, execSize, curInst, curBB))
                            {
                                dstSpillRegSize *= 3;
                            }
                            else
                            {
                                dstSpillRegSize *= 2;
                            }

                            if (!kernel.fg.builder->useSends())
                            {
                                dstSpillRegSize++;
                            }
                        }
                        else if (dst->getRegAccess() == IndirGRF)
                        {
                            auto pointsToSet = pointsToAnalysis.getAllInPointsTo(dst->getBase()->asRegVar());
                            if (pointsToSet != nullptr)
                            {
                                for (auto var : *pointsToSet)
                                {
                                    if (var->isRegAllocPartaker())
                                    {
                                        indrVars.push_back(var);
                                        indrDstSpillRegSize += var->getDeclare()->getNumRows();
                                    }
                                }
                            }
                        }
                    }
                }

                unsigned srcFillRegSize = 0;
                unsigned indirSrcFillRegSize = 0;
                // Scan srcs
                for (int i = 0, srcNum = G4_Inst_Table[curInst->opcode()].n_srcs; i < srcNum; i++)
                {
                    G4_Operand* src = curInst->getSrc(i);

                    if (src &&
                        src->isSrcRegRegion() &&
                        src->asSrcRegRegion()->getBase()->isRegVar())
                    {
                        if (src->asSrcRegRegion()->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_GRF)
                        {
                            if (src->asSrcRegRegion()->crossGRF())
                            {
                                srcFillRegSize += 2;
                            }
                            else
                            {
                                srcFillRegSize += 1;
                            }
                        }
                        else if (src->asSrcRegRegion()->getRegAccess() == IndirGRF)
                        {
                            auto pointsToSet = pointsToAnalysis.getAllInPointsTo(src->asSrcRegRegion()->getBase()->asRegVar());
                            if (pointsToSet != nullptr)
                            {
                                for (auto var : *pointsToSet)
                                {
                                    if (var->isRegAllocPartaker())
                                    {
                                        if (std::find(indrVars.begin(), indrVars.end(), var) == indrVars.end())
                                        {
                                            indrVars.push_back(var);
                                            indirSrcFillRegSize += var->getDeclare()->getNumRows();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                currentSpillRegSize = srcFillRegSize > dstSpillRegSize ? srcFillRegSize : dstSpillRegSize;
                currentIndrSpillRegSize = indrDstSpillRegSize + indirSrcFillRegSize;
            }

            spillRegSize = currentSpillRegSize > spillRegSize ? currentSpillRegSize : spillRegSize;
            indrSpillRegSize = currentIndrSpillRegSize > indrSpillRegSize ? currentIndrSpillRegSize : indrSpillRegSize;
        }
    }
}


bool GraphColor::regAlloc(bool doBankConflictReduction,
    bool highInternalConflict,
    bool reserveSpillReg, unsigned& spillRegSize, unsigned& indrSpillRegSize,
    RPE* rpe)
{

    bool useSplitLLRHeuristic = false;

    if (builder.getOption(vISA_RATrace))
    {
        std::cout << "\t--# variables: " << liveAnalysis.getNumSelectedVar() << "\n";
    }

    unsigned reserveSpillSize = 0;
    if (reserveSpillReg)
    {
        gra.determineSpillRegSize(spillRegSize, indrSpillRegSize);
        reserveSpillSize = spillRegSize + indrSpillRegSize;
        MUST_BE_TRUE(reserveSpillSize < kernel.getNumCalleeSaveRegs(), "Invalid reserveSpillSize in fail-safe RA!");
        totalGRFRegCount -= reserveSpillSize;
    }

    // Copy over alignment for vars inserted by RA
    gra.copyMissingAlignment();

    //
    // create an array of live ranges.
    //
    createLiveRanges(reserveSpillSize);
    //
    // set the pre-assigned registers
    //
    for (unsigned i = 0; i < numVar; i++)
    {
        if (lrs[i]->getVar()->getPhyReg())
        {
            lrs[i]->setPhyReg(lrs[i]->getVar()->getPhyReg(), lrs[i]->getVar()->getPhyRegOff());
        }

        G4_Declare* dcl = lrs[i]->getDcl();
        if (!useSplitLLRHeuristic)
        {
            auto dclLR = gra.getLocalLR(dcl);

            if (dclLR != NULL &&
                gra.getLocalLR(dcl)->getSplit())
            {
                useSplitLLRHeuristic = true;
            }
        }

    }

    //
    // compute interference matrix
    //
    intf.init(mem);
    intf.computeInterference();
#ifdef DEBUG_VERBOSE_ON
    intf.dumpInterference();
    //    intf.interferenceVerificationForSplit();
#endif

    startTimer(TIMER_COLORING);
    //
    // compute degree and spill costs for each live range
    //
    if (liveAnalysis.livenessClass(G4_GRF))
    {
        computeDegreeForGRF();
    }
    else
    {
        computeDegreeForARF();
    }
    computeSpillCosts(useSplitLLRHeuristic);

    //
    // determine coloring order
    //
    determineColorOrdering();

    //
    // Set up the sub-reg alignment from declare information
    //
    for (unsigned i = 0; i < numVar; i++)
    {
        G4_Declare* dcl = lrs[i]->getDcl();

        if (gra.getSubRegAlign(dcl) == Any && !dcl->getIsPartialDcl())
        {
            //
            // multi-row, subreg alignment = 16 words
            //
            if (dcl->getNumRows() > 1)
            {
                gra.setSubRegAlign(lrs[i]->getVar()->getDeclare(), GRFALIGN);
            }
            //
            // single-row
            //
            else if (gra.getSubRegAlign(lrs[i]->getVar()->getDeclare()) == Any)
            {
                //
                // set up Odd word or Even word sub reg alignment
                //
                unsigned nbytes = dcl->getNumElems()* G4_Type_Table[dcl->getElemType()].byteSize;
                unsigned nwords = nbytes / G4_WSIZE + nbytes % G4_WSIZE;
                if (nwords >= 2 && lrs[i]->getRegKind() == G4_GRF)
                {
                    gra.setSubRegAlign(lrs[i]->getVar()->getDeclare(), Even_Word);
                }
            }
        }
    }
    //
    // assign registers for GRFs/MRFs, GRFs are first attempted to be assigned using round-robin and if it fails
    // then we retry using a first-fit heuristic; for MRFs we always use the round-robin heuristic
    //
    if (liveAnalysis.livenessClass(G4_GRF))
    {
        bool hasStackCall = kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();

        bool willSpill = kernel.getOptions()->getTarget() == VISA_3D &&
            rpe->getMaxRP() >= kernel.getOptions()->getuInt32Option(vISA_TotalGRFNum) + 24;
        if (willSpill)
        {
            // go straight to first_fit to save compile time since we are definitely spilling
            // we do this for 3D only since with indirect/subroutine the RP pressure can be very unreliable
            // FIXME: due to factors like local split and scalar variables that are not accurately modeled in RP estimate,
            // RA may succeed even when RP is > total #GRF. We should investigate these cases and fix RPE
            assignColors(FIRST_FIT, false, false);
            //assert(requireSpillCode() && "inaccurate GRF pressure estimate");
            stopTimer(TIMER_COLORING);
            return !requireSpillCode();
        }

        if (kernel.getOption(vISA_RoundRobin) && !hasStackCall && !gra.isReRAPass())
        {
            if (assignColors(ROUND_ROBIN, doBankConflictReduction, highInternalConflict) == false)
            {
                resetTemporaryRegisterAssignments();
                bool success = assignColors(FIRST_FIT, doBankConflictReduction, highInternalConflict);

                if (!success && doBankConflictReduction && isHybrid)
                {
                    stopTimer(TIMER_COLORING);
                    return false;
                }

                if (!success && doBankConflictReduction)
                {
                    resetTemporaryRegisterAssignments();
                    assignColors(FIRST_FIT, false, false);
                }
            }
        }
        else
        {
            bool success = assignColors(FIRST_FIT, true, highInternalConflict);
            if (!success)
            {
                resetTemporaryRegisterAssignments();
                assignColors(FIRST_FIT, false, false);
            }
        }
    }
    else if (liveAnalysis.livenessClass(G4_FLAG))
    {
        if (kernel.getOption(vISA_RoundRobin))
        {
            if (assignColors(ROUND_ROBIN, false, false) == false)
            {
                resetTemporaryRegisterAssignments();
                assignColors(FIRST_FIT, false, false);
            }
        }
        else
        {
            assignColors(FIRST_FIT, false, false);
        }
    }
    else
    {
        // assign registers for ARFs using a first-fit heuristic
        assignColors(FIRST_FIT, false, false);
    }

    stopTimer(TIMER_COLORING);
    return (requireSpillCode() == false);
}

void GraphColor::confirmRegisterAssignments()
{
    for (unsigned i = 0; i < numVar; i++)
    {
        if (lrs[i]->getPhyReg()) {
            if (lrs[i]->getVar()->getPhyReg()) {
                MUST_BE_TRUE((lrs[i]->getVar()->getPhyReg() == lrs[i]->getPhyReg()), ERROR_GRAPHCOLOR);
            }
            else {
                lrs[i]->getVar()->setPhyReg(lrs[i]->getPhyReg(), lrs[i]->getPhyRegOff());
            }
        }
    }
}

void GraphColor::resetTemporaryRegisterAssignments()
{
    for (unsigned i = 0; i < numVar; i++)
    {
        if (lrs[i]->getVar()->getPhyReg() == NULL) {
            lrs[i]->resetPhyReg();
        }
    }
}

void GraphColor::cleanupRedundantARFFillCode()
{
    for (BB_LIST_ITER it = builder.kernel.fg.begin(); it != builder.kernel.fg.end(); it++)
    {
        clearSpillAddrLocSignature();

        for (std::list<G4_INST*>::iterator i = (*it)->begin(); i != (*it)->end();)
        {
            G4_INST* inst = (*i);

            //
            // process writes to spill storage (GRF) of addr regs
            //
            G4_DstRegRegion* dst = inst->getDst();

            if (dst && dst->getBase() &&
                dst->getBase()->isRegVar() &&
                (kernel.fg.isPseudoA0Dcl(dst->getBase()->asRegVar()->getDeclare()) ||
                    inst->isPseudoKill()))
            {
                i++;
                continue;
            }

            if (dst != NULL &&
                dst->getRegAccess() == Direct) {

                if (dst->getBase()->isRegVar() &&
                    dst->getBase()->asRegVar()->isRegVarAddrSpillLoc())
                {
                    pruneActiveSpillAddrLocs(dst, inst->getExecSize(), inst->getExecType());
                }
                //
                // process writes to (allocated) addr regs
                //
                else if (dst->getBase()->isRegAllocPartaker())
                {
                    G4_RegVar* addrReg = dst->getBase()->asRegVar();

                    if (gra.isAddrFlagSpillDcl(addrReg->getDeclare()))
                    {
                        G4_SrcRegRegion* srcRgn = inst->getSrc(0)->asSrcRegRegion();

                        if (redundantAddrFill(dst, srcRgn, inst->getExecSize())) {
                            std::list<G4_INST*>::iterator j = i++;
                            (*it)->erase(j);
                            continue;
                        }
                        else {
                            updateActiveSpillAddrLocs(dst, srcRgn, inst->getExecSize());
                        }
                    }
                    else {
                        pruneActiveSpillAddrLocs(dst, inst->getExecSize(), inst->getExecType());
                    }
                }
            }

            i++;
        }
    }
}

void GraphColor::pruneActiveSpillAddrLocs(G4_DstRegRegion* dstRegion, unsigned exec_size, G4_Type exec_type)
{
    if (dstRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
        MUST_BE_TRUE(((exec_type == Type_UW || exec_type == Type_W) && exec_size <= getNumAddrRegisters()) ||
            (exec_size == 1), "Unexpected ADDR spill loc update format!");
        MUST_BE_TRUE(dstRegion->getRegAccess() == Direct, "Unexpected ADDR spill loc");

        G4_RegVarAddrSpillLoc * spillLocReg = static_cast<G4_RegVarAddrSpillLoc*>(dstRegion->getBase());
        unsigned startId = spillLocReg->getLocId() + dstRegion->getSubRegOff();
        unsigned endId = startId + exec_size * dstRegion->getHorzStride();

        for (unsigned i = 0, horzStride = dstRegion->getHorzStride(); i < getNumAddrRegisters(); i += horzStride)
        {
            if (spAddrRegSig[i] >= startId && spAddrRegSig[i] < endId)
            {
                spAddrRegSig[i] = 0;
            }
        }
    }
    else if (dstRegion->getBase()->asRegVar()->isPhyRegAssigned()) {
        G4_RegVar* addrReg = dstRegion->getBase()->asRegVar();
        MUST_BE_TRUE(addrReg->getPhyReg()->isA0(), "Unknown error in ADDR reg spill code cleanup!");
        unsigned startId = addrReg->getPhyRegOff();
        unsigned endId = startId + exec_size * dstRegion->getHorzStride();
        MUST_BE_TRUE(endId <= getNumAddrRegisters(), "Unknown error in ADDR reg spill code cleanup!");

        for (unsigned i = startId; i < endId; i += dstRegion->getHorzStride())
        {
            spAddrRegSig[i] = 0;
        }
    }
    else {
        MUST_BE_TRUE(false, "Unknown error in ADDR reg spill code cleanup!");
    }
}

void GraphColor::updateActiveSpillAddrLocs(G4_DstRegRegion* tmpDstRegion, G4_SrcRegRegion* srcRegion, unsigned exec_size)
{
    MUST_BE_TRUE(gra.isAddrFlagSpillDcl(tmpDstRegion->getBase()->asRegVar()->getDeclare()), "Unknown error in ADDR reg spill code cleanup!");
    G4_RegVar* addrReg = tmpDstRegion->getBase()->asRegVar();
    MUST_BE_TRUE(addrReg->getPhyReg()->isA0(), "Unknown error in ADDR reg spill code cleanup!");
    unsigned startAddrId = addrReg->getPhyRegOff();
    unsigned endAddrId = startAddrId + exec_size * tmpDstRegion->getHorzStride();
    MUST_BE_TRUE(endAddrId <= getNumAddrRegisters(), "Unknown error in ADDR reg spill code cleanup!");

    MUST_BE_TRUE(srcRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc(), "Unknown error in ADDR reg spill code cleanup!");
    G4_RegVarAddrSpillLoc * spillLocReg = static_cast<G4_RegVarAddrSpillLoc*>(srcRegion->getBase());
    unsigned startLocId = spillLocReg->getLocId() + srcRegion->getSubRegOff();

    for (unsigned i = startAddrId, j = startLocId; i < endAddrId;
        i += tmpDstRegion->getHorzStride(), j += srcRegion->getRegion()->horzStride)
    {
        spAddrRegSig[i] = j;
    }
}

bool GraphColor::redundantAddrFill(G4_DstRegRegion* tmpDstRegion, G4_SrcRegRegion* srcRegion, unsigned exec_size)
{
    bool match = true;

    MUST_BE_TRUE(gra.isAddrFlagSpillDcl(tmpDstRegion->getBase()->asRegVar()->getDeclare()), "Unknown error in ADDR reg spill code cleanup!");
    G4_RegVar* addrReg = tmpDstRegion->getBase()->asRegVar();
    MUST_BE_TRUE(addrReg->getPhyReg()->isA0(), "Unknown error in ADDR reg spill code cleanup!");
    unsigned startAddrId = addrReg->getPhyRegOff();
    unsigned endAddrId = startAddrId + exec_size * tmpDstRegion->getHorzStride();
    MUST_BE_TRUE(endAddrId <= getNumAddrRegisters(), "Unknown error in ADDR reg spill code cleanup!");

    MUST_BE_TRUE(srcRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc(), "Unknown error in ADDR reg spill code cleanup!");
    G4_RegVarAddrSpillLoc * spillLocReg = static_cast<G4_RegVarAddrSpillLoc*>(srcRegion->getBase());
    unsigned startLocId = spillLocReg->getLocId() + srcRegion->getSubRegOff();

    for (unsigned i = startAddrId, j = startLocId; i < endAddrId;
        i += tmpDstRegion->getHorzStride(), j += srcRegion->getRegion()->horzStride)
    {
        if (spAddrRegSig[i] != j)
        {
            match = false;
            break;
        }
    }

    return match;
}

unsigned GlobalRA::sendBlockSizeCode(unsigned owordSize)
{
    unsigned code;

    switch (owordSize) {
    case 1:
        code = 0;
        break;
    case 2:
        code = 2;
        break;
    case 4:
        code = 3;
        break;
    case 8:
        code = 4;
        break;
    default:
        MUST_BE_TRUE(false, ERROR_REGALLOC);
        code = 0;
    }

    return code;
}

#define STATELESS_SURFACE_INDEX            0xFF
#define HEADER_PRESENT                    0x80000
#define SEND_OWORD_READ_TYPE            0
#define SEND_OWORD_WRITE_TYPE            8
#define SEND_MSG_TYPE_BIT_OFFSET        14
#define    SEND_RSP_LENGTH_BIT_OFFSET        20
#define    SEND_MSG_LENGTH_BIT_OFFSET        25
#define SEND_DESC_DATA_SIZE_BIT_OFFSET    8

G4_Imm* GlobalRA::createMsgDesc(unsigned owordSize, bool writeType, bool isSplitSend)
{
    // If isSplitSend = true then messageLength = 1 and extMesLength = (owordSize/2) GRFs
    unsigned message = STATELESS_SURFACE_INDEX;
    message |= HEADER_PRESENT;
    if (writeType)
    {
        unsigned messageType = SEND_OWORD_WRITE_TYPE;
        message |= messageType << SEND_MSG_TYPE_BIT_OFFSET;
        unsigned messageLength = 1;
        if (!isSplitSend)
        {
            messageLength += ROUND(owordSize, 2) / 2;
        }
        message |= messageLength << SEND_MSG_LENGTH_BIT_OFFSET;
    }
    else
    {
        unsigned messageType = SEND_OWORD_READ_TYPE;
        message |= messageType << SEND_MSG_TYPE_BIT_OFFSET;
        unsigned responseLength = ROUND(owordSize, 2) / 2;
        message |= responseLength << SEND_RSP_LENGTH_BIT_OFFSET;
        unsigned messageLength = 1;
        message |= messageLength << SEND_MSG_LENGTH_BIT_OFFSET;
    }
    unsigned writeOwordSize = sendBlockSizeCode(owordSize);
    message |= writeOwordSize << SEND_DESC_DATA_SIZE_BIT_OFFSET;
    return builder.createImm(message, Type_UD);
}

void GraphColor::stackCallProlog()
{
    // mov (8) r126.0<1>:ud    r0.0<8;8,1>:ud
    // This sets up the header for oword block r/w used for caller/callee-save

    auto dstRgn = builder.Create_Dst_Opnd_From_Dcl(builder.kernel.fg.scratchRegDcl, 1);
    auto srcRgn = builder.Create_Src_Opnd_From_Dcl(builder.getRealR0(), builder.getRegionStride1());

    G4_INST* mov = builder.createInternalInst(nullptr, G4_mov, nullptr, false, 8, dstRgn, srcRgn, nullptr,
        InstOpt_WriteEnable);

   G4_BB* entryBB = builder.kernel.fg.getEntryBB();
   auto iter = std::find_if(entryBB->begin(), entryBB->end(), [](G4_INST* inst) { return !inst->isLabel(); });
   entryBB->insert(iter, mov);
}

//
// Generate the save code for startReg to startReg+owordSize/2.
//
void GraphColor::saveRegs(
    unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr,
    unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt)
{

    assert(getGenxPlatform() >= GENX_SKL && "stack call only supported on SKL+");

    if (owordSize == 8 || owordSize == 4 || owordSize == 2)
    {
        // add (1) r126.2<1>:ud    r125.7<0;1,0>:ud    0x2:ud
        // sends (8) null<1>:ud    r126.0    r1.0 ...
        uint8_t execSize = (owordSize > 2) ? 16 : 8;
        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);
        G4_Operand* src0 = NULL;
        G4_Imm* src1 = NULL;
        G4_opcode op;
        if (framePtr)
        {
            src0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UD);
            src1 = builder.createImm(frameOwordOffset, Type_UD);
            op = G4_add;
        }
        else
        {
            src0 = builder.createImm(frameOwordOffset, Type_UD);
            op = G4_mov;
        }
        G4_INST* hdrSetInst = builder.createInternalInst(NULL, op, NULL, false, 1,
            dst, src0, src1, InstOpt_WriteEnable);

        bb->insert(insertIt, hdrSetInst);

        builder.instList.clear();
        G4_DstRegRegion * postDst = builder.createNullDst((execSize > 8) ? Type_UW : Type_UD);
        auto sendSrc1 = builder.createSrcRegRegion(Mod_src_undef, Direct, scratchRegDcl->getRegVar(),
            0, 0, builder.rgnpool.createRegion(8, 8, 1), Type_UD);
        unsigned messageLength = owordSize / 2;
        G4_Declare *msgDcl = builder.createTempVar(messageLength * GENX_DATAPORT_IO_SZ,
            Type_UD, GRFALIGN, StackCallStr);
        msgDcl->getRegVar()->setPhyReg(regPool.getGreg(startReg), 0);
        auto sendSrc2 = builder.createSrcRegRegion(Mod_src_undef, Direct, msgDcl->getRegVar(), 0, 0,
            builder.rgnpool.createRegion(8, 8, 1), Type_UD);
        G4_Imm* descImm = gra.createMsgDesc(owordSize, true, true);
        uint32_t extDesc = G4_SendMsgDescriptor::createExtDesc(SFID::DP_DC, false, messageLength);
        auto msgDesc = builder.createSendMsgDesc((uint32_t)descImm->getInt(), extDesc, false, true,
            getScratchSurface());
        auto sendInst = builder.Create_SplitSend_Inst(nullptr, postDst, sendSrc1, sendSrc2, execSize, msgDesc,
            InstOpt_WriteEnable, false);
        auto exDescOpnd = sendInst->getMsgExtDescOperand();
        if (exDescOpnd->isSrcRegRegion())
        {
            exDescOpnd->asSrcRegRegion()->getTopDcl()->getRegVar()->setPhyReg(builder.phyregpool.getAddrReg(), 0);
        }
        bb->splice(insertIt, builder.instList);
    }
    else if (owordSize > 8)
    {
        saveRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + 4, owordSize - 8, scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt);
    }
    //
    // Split into chunks of sizes 4 and remaining owords.
    //
    else if (owordSize > 4)
    {
        saveRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + 2, owordSize - 4, scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt);
    }
    //
    // Split into chunks of sizes 2 and remaining owords.
    //
    else if (owordSize > 2)
    {
        saveRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + 1, owordSize - 2, scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt);
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_REGALLOC);
    }
}

//
// Generate the save code for the i/p saveRegs.
//
void GraphColor::saveActiveRegs(
    std::vector<bool>& saveRegs, unsigned startReg, unsigned frameOffset,
    G4_BB* bb, INST_LIST_ITER insertIt)
{
    G4_Declare* scratchRegDcl = builder.kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;

    unsigned frameOwordPos = frameOffset;
    unsigned startPos = 0;

    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && saveRegs[startPos] == false; startPos++);
        if (startPos < saveRegs.size() && saveRegs[startPos]) {
            unsigned endPos = startPos + 1;
            for (; endPos < saveRegs.size() && saveRegs[endPos] == true; endPos++);
            unsigned owordSize = (endPos - startPos) * 2;
            this->saveRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr, frameOwordPos, bb, insertIt);
            frameOwordPos += owordSize;
            startPos = endPos;
        }
    }
}

G4_SrcRegRegion* GraphColor::getScratchSurface() const
{
    return nullptr; // use stateless access
}

//
// Generate the restore code for startReg to startReg+owordSize/2.
//
void GraphColor::restoreRegs(
    unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr,
    unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt)
{
    //
    // Process chunks of size 8, 4, 2 and 1.
    //
    if (owordSize == 8 || owordSize == 4 || owordSize == 2)
    {

        //
        // add (1) r126<1>:d FP<0;1,0>:d offset
        //
        {
            G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);
            RegionDesc* rDesc = builder.getRegionScalar();
            G4_Operand* src0;
            G4_Imm* src1;
            G4_opcode op;
            if (framePtr)
            {
                src0 = builder.createSrcRegRegion(
                    Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, rDesc, Type_UD);
                src1 = builder.createImm(frameOwordOffset, Type_UD);
                op = G4_add;
            }
            else
            {
                src0 = builder.createImm(frameOwordOffset, Type_UD);
                src1 = NULL;
                op = G4_mov;
            }
            G4_INST* hdrSetInst = builder.createInternalInst(NULL, op, NULL, false, 1,
                dst, src0, src1, InstOpt_WriteEnable);
            bb->insert(insertIt, hdrSetInst);
        }

        //
        //  send (16) r[startReg]<1>:uw r126 0xa desc:ud
        //
        {
            builder.instList.clear();
            uint8_t execSize = (owordSize > 2) ? 16 : 8;

            unsigned responseLength = ROUND(owordSize, 2) / 2;
            G4_Declare *dstDcl = builder.createTempVar(responseLength * GENX_DATAPORT_IO_SZ,
                Type_UD, GRFALIGN, GraphColor::StackCallStr);
            dstDcl->getRegVar()->setPhyReg(regPool.getGreg(startReg), 0);
            G4_DstRegRegion* postDst = builder.createDstRegRegion(Direct, dstDcl->getRegVar(), 0, 0, 1, (execSize > 8) ? Type_UW : Type_UD);
            G4_SrcRegRegion* payload = builder.Create_Src_Opnd_From_Dcl(scratchRegDcl, builder.getRegionStride1());
            G4_Imm* exDesc = builder.createImm(0xa, Type_UD);
            G4_Imm* desc = gra.createMsgDesc(owordSize, false, false);
            auto msgDesc = builder.createSendMsgDesc((uint32_t)desc->getInt(), (uint32_t)exDesc->getInt(), true, false,
                getScratchSurface());
            auto sendInst = builder.Create_SplitSend_Inst(nullptr, postDst, payload, builder.createNullSrc(Type_UD), execSize,
                msgDesc, InstOpt_WriteEnable, false);
            auto exDescOpnd = sendInst->getMsgExtDescOperand();
            if (exDescOpnd->isSrcRegRegion())
            {
                exDescOpnd->asSrcRegRegion()->getTopDcl()->getRegVar()->setPhyReg(builder.phyregpool.getAddrReg(), 0);
            }
            bb->splice(insertIt, builder.instList);
        }

        builder.instList.clear();
    }
    //
    // Split into chunks of sizes 8 and remaining owords.
    //
    else if (owordSize > 8)
    {
        restoreRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + 4, owordSize - 8, scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt);
    }
    //
    // Split into chunks of sizes 4 and remaining owords.
    //
    else if (owordSize > 4)
    {
        restoreRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + 2, owordSize - 4, scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt);
    }
    //
    // Split into chunks of sizes 2 and remaining owords.
    //
    else if (owordSize > 2)
    {
        restoreRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + 1, owordSize - 2, scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt);
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_REGALLOC);
    }
}

//
// Generate the restore code for the i/p restoreRegs.
//
void GraphColor::restoreActiveRegs(
    std::vector<bool>& restoreRegs, unsigned startReg, unsigned frameOffset,
    G4_BB* bb, INST_LIST_ITER insertIt)
{
    G4_Declare* scratchRegDcl = builder.kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;

    unsigned frameOwordPos = frameOffset;
    unsigned startPos = 0;

    while (startPos < restoreRegs.size())
    {
        for (; startPos < restoreRegs.size() && restoreRegs[startPos] == false; startPos++);
        if (startPos < restoreRegs.size() && restoreRegs[startPos]) {
            unsigned endPos = startPos + 1;
            for (; endPos < restoreRegs.size() && restoreRegs[endPos] == true; endPos++);
            unsigned owordSize = (endPos - startPos) * 2;
            this->restoreRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr, frameOwordPos, bb, insertIt);
            frameOwordPos += owordSize;
            startPos = endPos;
        }
    }
}

//
// Optimize the reg footprint so as to reduce the number of "send" instructions required for
// save/restore, at the cost of a little additional save/restore memory (if any). Since we
// are using oword read/write for save/restore, we can only read/write only in units of 1, 2
// or 4 regs per "send" instruction.
//
void GraphColor::OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs)
{
    unsigned startPos = 0;
    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos);
        if (startPos == saveRegs.size())
        {
            break;
        }
        if (startPos + 4 <= saveRegs.size())
        {
            if (saveRegs[startPos] & saveRegs[startPos + 2] & !saveRegs[startPos + 3])
            {
                saveRegs[startPos + 1] = saveRegs[startPos + 3] = true;
            }
            else if (saveRegs[startPos] & saveRegs[startPos + 3])
            {
                if (startPos + 4 < saveRegs.size())
                {
                    if (!saveRegs[startPos + 4])
                    {
                        saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
                    }
                }
                else
                {
                    saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
                }
            }
        }
        unsigned winBound = (unsigned)MIN(saveRegs.size(), startPos + 4);
        for (; startPos < winBound && saveRegs[startPos]; ++startPos);
    }
}

void GraphColor::OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs, std::vector<bool>& retRegs)
{
    unsigned startPos = 0;
    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos);
        if (startPos == saveRegs.size())
        {
            break;
        }
        if (startPos + 4 <= saveRegs.size())
        {
            if (saveRegs[startPos] & saveRegs[startPos + 2])
            {
                if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                {
                    saveRegs[startPos + 1] = true;
                }
                if (!saveRegs[startPos + 3] & !retRegs[startPos + 3])
                {
                    saveRegs[startPos + 3] = true;
                }
            }
            else if (saveRegs[startPos] & saveRegs[startPos + 3])
            {
                if (startPos + 4 < saveRegs.size())
                {
                    if (!saveRegs[startPos + 4])
                    {
                        if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                        {
                            saveRegs[startPos + 1] = true;
                        }
                        if (!saveRegs[startPos + 2] & !retRegs[startPos + 2])
                        {
                            saveRegs[startPos + 2] = true;
                        }
                    }
                }
                else
                {
                    if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                    {
                        saveRegs[startPos + 1] = true;
                    }
                    if (!saveRegs[startPos + 2] & !retRegs[startPos + 2])
                    {
                        saveRegs[startPos + 2] = true;
                    }
                }
            }
        }
        unsigned winBound = (unsigned)MIN(saveRegs.size(), startPos + 4);
        for (; startPos < winBound && saveRegs[startPos]; ++startPos);
    }
}

//
// Add caller save/restore code before/after each stack call.
//
void GraphColor::addCallerSaveRestoreCode()
{
    unsigned int maxCallerSaveSize = builder.kernel.fg.callerSaveAreaOffset;
    unsigned int callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;

    for (BB_LIST_ITER it = builder.kernel.fg.begin(); it != builder.kernel.fg.end(); ++it)
    {
        if ((*it)->isEndWithFCall())
        {
            //
            // Determine the caller-save registers per call site.
            //
            std::vector<bool> callerSaveRegs(callerSaveNumGRF, false);
            std::vector<bool> retRegs(callerSaveNumGRF, false);
            unsigned callerSaveRegCount = 0;
            G4_INST* callInst = (*it)->back();
            /*callInst->getDst()->getTopDcl()->getRegVar()->setPhyReg(regPool.getGreg(1), 0);*/
            unsigned pseudoVCAId = callInst->asCFInst()->getAssocPseudoVCA()->getId();
            ASSERT_USER((*it)->Succs.size() == 1, "fcall basic block cannot have more than 1 successor");
            G4_BB* afterFCallBB = (*it)->Succs.front();

            for (unsigned i = 0; i < numVar; i++)
            {
                if (i != pseudoVCAId &&
                    kernel.fg.isPseudoVCEDcl(lrs[i]->getDcl()) != true &&
                    intf.interfereBetween(pseudoVCAId, i) == true)
                {
                    if (!builder.isPreDefArg(lrs[i]->getDcl()))
                    {
                        // NOTE: Spilled live ranges should not be caller-save.
                        MUST_BE_TRUE(lrs[i]->getPhyReg()->isGreg(), ERROR_REGALLOC);
                        unsigned startReg = lrs[i]->getPhyReg()->asGreg()->getRegNum();
                        unsigned endReg = startReg + lrs[i]->getDcl()->getNumRows();
                        startReg = (startReg < callerSaveNumGRF) ? startReg : callerSaveNumGRF;
                        startReg = (startReg > 0) ? startReg : 1;
                        endReg = (endReg < callerSaveNumGRF) ? endReg : callerSaveNumGRF;
                        endReg = (endReg > 0) ? endReg : 1;
                        for (unsigned j = startReg; j < endReg; j++)
                        {
                            if (builder.isPreDefRet(lrs[i]->getDcl()))
                            {
                                if (retRegs[j] == false)
                                {
                                    retRegs[j] = true;
                                }
                            }
                            else
                            {
                                if (callerSaveRegs[j] == false)
                                {
                                    callerSaveRegs[j] = true;
                                    callerSaveRegCount++;
                                }
                            }
                        }
                    }
                }
            }
            OptimizeActiveRegsFootprint(callerSaveRegs, retRegs);

            unsigned callerSaveRegsWritten = 0;
            for (std::vector<bool>::iterator vit = callerSaveRegs.begin();
                vit != callerSaveRegs.end();
                vit++)
                callerSaveRegsWritten += ((*vit) ? 1 : 0);

            INST_LIST_ITER insertSaveIt = (*it)->end();
            --insertSaveIt, --insertSaveIt;
            MUST_BE_TRUE((*insertSaveIt)->opcode() == G4_pseudo_caller_save, ERROR_REGALLOC);
            INST_LIST_ITER rmIt = insertSaveIt;
            if (insertSaveIt == (*it)->begin())
            {
                insertSaveIt = (*it)->end();
            }

            if (insertSaveIt != (*it)->end())
            {
                ++insertSaveIt;
            }
            else
            {
                insertSaveIt = (*it)->begin();
            }
            if (callerSaveRegCount > 0)
            {
                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    builder.kernel.getKernelDebugInfo()->clearOldInstList();
                    builder.kernel.getKernelDebugInfo()->setOldInstList
                    ((*it));
                }

                saveActiveRegs(callerSaveRegs, 0, builder.kernel.fg.callerSaveAreaOffset,
                    (*it), insertSaveIt);

                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    auto deltaInstList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
                    ((*it));
                    auto fcallInst = (*it)->back();
                    for (auto it : deltaInstList)
                    {
                        builder.kernel.getKernelDebugInfo()->addCallerSaveInst(fcallInst, it);
                    }
                }
            }
            (*it)->erase(rmIt);
            INST_LIST_ITER insertRestIt = afterFCallBB->begin();
            for (; (*insertRestIt)->opcode() != G4_pseudo_caller_restore; ++insertRestIt);
            if (callerSaveRegCount > 0)
            {
                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    builder.kernel.getKernelDebugInfo()->clearOldInstList();
                    builder.kernel.getKernelDebugInfo()->setOldInstList
                    (afterFCallBB);
                }

                restoreActiveRegs(callerSaveRegs, 0, builder.kernel.fg.callerSaveAreaOffset,
                    afterFCallBB, insertRestIt);

                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    auto deltaInsts = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
                    (afterFCallBB);
                    auto fcallInst = (*it)->back();
                    for (auto it : deltaInsts)
                    {
                        builder.kernel.getKernelDebugInfo()->addCallerRestoreInst
                        (fcallInst, it);
                    }
                }
            }
            afterFCallBB->erase(insertRestIt);

            //builder.kernel.fg.paramOverflowAreaOffset = builder.kernel.fg.callerSaveAreaOffset + callerSaveRegsWritten * 2;
            if (maxCallerSaveSize < (builder.kernel.fg.callerSaveAreaOffset + callerSaveRegsWritten * 2))
                maxCallerSaveSize = (builder.kernel.fg.callerSaveAreaOffset + callerSaveRegsWritten * 2);

            if (m_options->getOption(vISA_OptReport))
            {
                std::ofstream optreport;
                getOptReportStream(optreport, m_options);
                optreport << "Caller save size: " << callerSaveRegCount * getGRFSize() <<
                    " bytes for fcall at cisa id " <<
                    (*it)->back()->getCISAOff() << std::endl;
                closeOptReportStream(optreport);
            }
        }
    }

    builder.kernel.fg.paramOverflowAreaOffset = maxCallerSaveSize;

    builder.instList.clear();
}

//
// Add callee save/restore code at stack call function entry/exit.
//
void GraphColor::addCalleeSaveRestoreCode()
{
    builder.kernel.fg.callerSaveAreaOffset = builder.kernel.fg.calleeSaveAreaOffset;
    unsigned int callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;
    unsigned int numCalleeSaveRegs = builder.kernel.getNumCalleeSaveRegs();

    // Determine the callee-save registers.

    std::vector<bool> calleeSaveRegs;
    calleeSaveRegs.reserve(numCalleeSaveRegs);
    calleeSaveRegs.resize(numCalleeSaveRegs, false);
    unsigned calleeSaveRegCount = 0;

    unsigned pseudoVCEId = builder.kernel.fg.pseudoVCEDcl->getRegVar()->getId();
    unsigned int stackCallStartReg = builder.kernel.getStackCallStartReg();
    for (unsigned i = 0; i < numVar; i++)
    {
        if (pseudoVCEId != i && intf.interfereBetween(pseudoVCEId, i))
        {
            if (lrs[i]->getPhyReg())
            {
                MUST_BE_TRUE(lrs[i]->getPhyReg()->isGreg(), ERROR_REGALLOC);
                unsigned startReg = lrs[i]->getPhyReg()->asGreg()->getRegNum();
                unsigned endReg = startReg + lrs[i]->getDcl()->getNumRows();
                startReg = (startReg >= callerSaveNumGRF) ? startReg : callerSaveNumGRF;
                startReg = (startReg < stackCallStartReg) ? startReg : stackCallStartReg;
                endReg = (endReg >= callerSaveNumGRF) ? endReg : callerSaveNumGRF;
                endReg = (endReg < stackCallStartReg) ? endReg : stackCallStartReg;
                for (unsigned j = startReg; j < endReg; j++)
                {
                    if (calleeSaveRegs[j - callerSaveNumGRF] == false)
                    {
                        calleeSaveRegs[j - callerSaveNumGRF] = true;
                        calleeSaveRegCount++;
                    }
                }
            }
        }
    }

    OptimizeActiveRegsFootprint(calleeSaveRegs);
    unsigned calleeSaveRegsWritten = 0;
    for (std::vector<bool>::iterator vit = calleeSaveRegs.begin();
        vit != calleeSaveRegs.end();
        vit++)
        calleeSaveRegsWritten += ((*vit) ? 1 : 0);

    INST_LIST_ITER insertSaveIt = builder.kernel.fg.getEntryBB()->end();
    for (--insertSaveIt; (*insertSaveIt)->opcode() != G4_pseudo_callee_save; --insertSaveIt);
    if (calleeSaveRegCount > 0)
    {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Store old inst list so we can separate callee save
            // instructions that get inserted.
            builder.kernel.getKernelDebugInfo()->clearOldInstList();
            builder.kernel.getKernelDebugInfo()->setOldInstList
            (builder.kernel.fg.getEntryBB());
        }
        saveActiveRegs(calleeSaveRegs, callerSaveNumGRF, builder.kernel.fg.calleeSaveAreaOffset,
            builder.kernel.fg.getEntryBB(), insertSaveIt);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Delta of oldInstList and current instList are all
            // callee save instructions.
            auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
            (builder.kernel.fg.getEntryBB());
            for (auto inst : instList)
            {
                builder.kernel.getKernelDebugInfo()->addCalleeSaveInst(inst);
            }
        }
    }
    builder.kernel.fg.getEntryBB()->erase(insertSaveIt);
    INST_LIST_ITER insertRestIt = builder.kernel.fg.getUniqueReturnBlock()->end();
    for (--insertRestIt; (*insertRestIt)->opcode() != G4_pseudo_callee_restore; --insertRestIt);
    INST_LIST_ITER eraseIt = insertRestIt++;
    if (calleeSaveRegCount > 0)
    {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Store old inst list so we can separate callee save
            // instructions that get inserted.
            builder.kernel.getKernelDebugInfo()->clearOldInstList();
            builder.kernel.getKernelDebugInfo()->setOldInstList
            (builder.kernel.fg.getUniqueReturnBlock());
        }

        restoreActiveRegs(calleeSaveRegs, callerSaveNumGRF, builder.kernel.fg.calleeSaveAreaOffset,
            builder.kernel.fg.getUniqueReturnBlock(), insertRestIt);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
            (builder.kernel.fg.getUniqueReturnBlock());
            for (auto inst : instList)
            {
                builder.kernel.getKernelDebugInfo()->addCalleeRestoreInst(inst);
            }
        }
    }
    builder.kernel.fg.getUniqueReturnBlock()->erase(eraseIt);

    builder.kernel.fg.callerSaveAreaOffset =
        MAX(
            builder.kernel.fg.calleeSaveAreaOffset + calleeSaveRegsWritten * 2,
            builder.kernel.fg.callerSaveAreaOffset);
    builder.instList.clear();

    if (m_options->getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, m_options);
        optreport << "Callee save size: " << calleeSaveRegCount * getGRFSize() <<
            " bytes" << std::endl;
        closeOptReportStream(optreport);
    }
}

//
// Add code to setup the stack frame in callee.
//
void GraphColor::addGenxMainStackSetupCode()
{
    unsigned frameSize = builder.kernel.fg.paramOverflowAreaOffset + builder.kernel.fg.paramOverflowAreaSize;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;
    G4_Declare* stackPtr = builder.kernel.fg.stackPtrDcl;

    INST_LIST_ITER insertIt = builder.kernel.fg.getEntryBB()->begin();
    for (; insertIt != builder.kernel.fg.getEntryBB()->end() && (*insertIt)->isLabel();
        ++insertIt)
        ; // empty body
    //
    // FP = 0
    //
    {
        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, framePtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_Imm * src = builder.createImm(0, Type_UD);
        G4_INST* fpInst = builder.createInternalInst(NULL, G4_mov, NULL, false, 1,
            dst, src, NULL, InstOpt_WriteEnable);
        insertIt = builder.kernel.fg.getEntryBB()->insert(insertIt, fpInst);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(fpInst);
            builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
        }

    }
    //
    // SP = FrameSize (overflow-area offset + overflow-area size)
    //
    {
        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_Imm * src = builder.createImm(frameSize, Type_UD);
        G4_INST* spIncInst = builder.createInternalInst(NULL, G4_mov, NULL, false, 1,
            dst, src, NULL, InstOpt_WriteEnable);
        builder.kernel.fg.getEntryBB()->insert(++insertIt, spIncInst);
    }
    builder.instList.clear();

    if (m_options->getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, m_options);
        optreport << "Total frame size: " << frameSize * 16 << " bytes" << std::endl;
        closeOptReportStream(optreport);
    }
}

//
// Add code to setup the stack frame in callee.
//
void GraphColor::addCalleeStackSetupCode()
{
    int frameSize = (int)builder.kernel.fg.paramOverflowAreaOffset /*- builder.kernel.fg.calleeSaveAreaOffset*/;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;
    G4_Declare* stackPtr = builder.kernel.fg.stackPtrDcl;

    if (frameSize == 0)
    {
        // Remove pseudo_store/restore_be_fp because a new frame is not needed
        INST_LIST_ITER insertIt = builder.kernel.fg.getEntryBB()->begin();
        for (; insertIt != builder.kernel.fg.getEntryBB()->end() && (*insertIt)->opcode() != G4_pseudo_store_be_fp;
            ++insertIt)
        {   /* void */
        };
        builder.kernel.fg.getEntryBB()->erase(insertIt);

        insertIt = builder.kernel.fg.getUniqueReturnBlock()->end();
        for (--insertIt; (*insertIt)->opcode() != G4_pseudo_restore_be_fp; --insertIt)
        {   /* void */
        };
        builder.kernel.fg.getUniqueReturnBlock()->erase(insertIt);

        return;
    }
    //
    // BE_FP = BE_SP
    // BE_SP += FrameSize (overflow-area offset + overflow-area size)
    //
    {
        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_DstRegRegion* fp_dst = builder.createDstRegRegion(Direct, framePtr->getRegVar(), 0, 0, 1, Type_UD);
        RegionDesc* rDesc = builder.getRegionScalar();
        G4_Operand* src0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_Operand* sp_src = builder.createSrcRegRegion(Mod_src_undef, Direct, stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_Imm * src1 = builder.createImm(frameSize, Type_UD);
        auto createBEFP = builder.createInternalInst(NULL, G4_mov, NULL, false, 1,
            fp_dst, sp_src, NULL, InstOpt_WriteEnable);
        auto addInst = builder.createInternalInst(NULL, G4_add, NULL, false, 1,
            dst, src0, src1, InstOpt_WriteEnable);
        G4_BB* entryBB = builder.kernel.fg.getEntryBB();
        auto insertIt = std::find_if(entryBB->begin(), entryBB->end(),
            [](G4_INST* inst) { return inst->opcode() == G4_pseudo_store_be_fp; });

        MUST_BE_TRUE(insertIt != entryBB->end(), "Can't find pseudo_store_be_fp");
        // Convert pseudo_store_be_fp to mov
        (*insertIt)->setOpcode(G4_mov);
        (*insertIt)->setOptionOn(InstOpt_WriteEnable);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            G4_INST* callerFPSave = (*insertIt);
            builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(createBEFP);
            builder.kernel.getKernelDebugInfo()->setCallerBEFPSaveInst(callerFPSave);
            builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
        }

        insertIt++;
        entryBB->insert(insertIt, createBEFP);
        entryBB->insert(insertIt, addInst);
    }
    //
    // BE_SP = BE_FP
    // BE_FP = oldFP (dst of pseudo_restore_be_fp)
    //
    {
        G4_DstRegRegion* sp_dst = builder.createDstRegRegion(Direct, stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        RegionDesc* rDesc = builder.getRegionScalar();
        G4_Operand* fp_src = builder.createSrcRegRegion(
            Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_INST* spRestore = builder.createInternalInst(NULL, G4_mov, NULL, false, 1,
            sp_dst, fp_src, NULL, InstOpt_WriteEnable);
        INST_LIST_ITER insertIt = builder.kernel.fg.getUniqueReturnBlock()->end();
        for (--insertIt; (*insertIt)->opcode() != G4_pseudo_restore_be_fp; --insertIt);
        // Convert pseudo_restore_be_fp to mov
        (*insertIt)->setOpcode(G4_mov);
        (*insertIt)->setOptionOn(InstOpt_WriteEnable);
        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            G4_INST* callerFPRestore = (*insertIt);
            builder.kernel.getKernelDebugInfo()->setCallerSPRestoreInst(spRestore);
            builder.kernel.getKernelDebugInfo()->setCallerBEFPRestoreInst(callerFPRestore);
        }
        builder.kernel.fg.getUniqueReturnBlock()->insert(insertIt, spRestore);
    }
    builder.instList.clear();

    if (m_options->getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, m_options);
        optreport << std::endl << "Total frame size: "
            << frameSize * 16 << " bytes" << std::endl;
        closeOptReportStream(optreport);
    }
}

//
// Add A0 save/restore code for stack calls.
//
void GraphColor::addA0SaveRestoreCode()
{
    uint8_t numA0Elements = (uint8_t)getNumAddrRegisters();

    int count = 0;
    for (auto bb : builder.kernel.fg)
    {
        if (bb->isEndWithFCall())
        {
            G4_BB* succ = bb->Succs.front();
            G4_RegVar* assocPseudoA0 = bb->back()->asCFInst()->getAssocPseudoA0Save();

            if (!assocPseudoA0->getPhyReg())
            {
                // Insert save/restore code because the pseudo node did not get an allocation
                const char* name = builder.getNameString(builder.mem, 20, builder.getIsKernel() ? "SA0_k%d_%d" : "SA0_f%d_%d", builder.getCUnitId(), count++);
                G4_Declare* savedDcl = builder.createDeclareNoLookup(name, G4_GRF, numA0Elements, 1, Type_UW);

                {
                    //
                    // (W) mov (16) TMP_GRF<1>:uw a0.0<16;16,1>:uw
                    //
                    G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, savedDcl->getRegVar(), 0, 0, 1, Type_UW);
                    RegionDesc* rDesc = builder.getRegionStride1();
                    G4_Operand* src = builder.createSrcRegRegion(
                        Mod_src_undef, Direct, regPool.getAddrReg(), 0, 0, rDesc, Type_UW);
                    G4_INST* saveInst = builder.createInternalInst(nullptr, G4_mov, nullptr, false, numA0Elements,
                        dst, src, nullptr, InstOpt_WriteEnable);
                    INST_LIST_ITER insertIt = std::prev(bb->end());
                    bb->insert(insertIt, saveInst);
                }

                {
                    //
                    // (W) mov (16) a0.0<1>:uw TMP_GRF<16;16,1>:uw
                    //
                    G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, regPool.getAddrReg(), 0, 0, 1, Type_UW);
                    RegionDesc* rDesc = builder.getRegionStride1();
                    G4_Operand* src = builder.createSrcRegRegion(
                        Mod_src_undef, Direct, savedDcl->getRegVar(), 0, 0, rDesc, Type_UW);
                    G4_INST* restoreInst = builder.createInternalInst(nullptr, G4_mov, nullptr, false, numA0Elements,
                        dst, src, nullptr, InstOpt_WriteEnable);
                    auto insertIt = std::find_if(succ->begin(), succ->end(), [](G4_INST* inst) { return !inst->isLabel(); });
                    succ->insert(insertIt, restoreInst);
                }
            }
        }
    }

    builder.instList.clear();
}

//
// Add Flag save/restore code for stack calls.
//
void GraphColor::addFlagSaveRestoreCode()
{
    int count = 0;
    int num32BitFlags = builder.getNumFlagRegisters() / 2;

    // each 32-bit flag gets a declare
    // ToDo: should we use flag ARF directly here?
    std::vector<G4_Declare*> tmpFlags;
    for (int i = 0; i < num32BitFlags; ++i)
    {
        G4_Declare* tmpFlag = builder.createTempFlag(2);
        tmpFlag->getRegVar()->setPhyReg(regPool.getFlagAreg(i), 0);
        tmpFlags.push_back(tmpFlag);
    }

    for (auto bb : builder.kernel.fg)
    {
        if (bb->isEndWithFCall())
        {
            G4_BB* succ = bb->Succs.front();
            G4_RegVar* assocPseudoFlag = bb->back()->asCFInst()->getAssocPseudoFlagSave();

            if (!assocPseudoFlag->getPhyReg())
            {
                // Insert save/restore code because the pseudo node did not get an allocation
                const char* name = builder.getNameString(builder.mem, 32, builder.getIsKernel() ? "SFLAG_k%d_%d" : "SFLAG_f%d_%d", builder.getCUnitId(), count++);
                G4_Declare* savedDcl1 = builder.createDeclareNoLookup(name, G4_GRF, num32BitFlags, 1, Type_UD);
                {
                    //
                    // (W) mov (1) TMP_GRF.0<1>:ud f0.0:ud
                    // (W) mov (1) TMP_GRF.1<1>:ud f1.0:ud
                    //
                    auto createFlagSaveInst = [&](int index)
                    {
                        auto flagDcl = tmpFlags[index];
                        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, savedDcl1->getRegVar(), 0, index, 1, Type_UD);
                        G4_Operand* src = builder.createSrcRegRegion(Mod_src_undef, Direct, flagDcl->getRegVar(), 0, 0,
                            builder.getRegionScalar(), Type_UD);
                        return builder.createInternalInst(nullptr, G4_mov, nullptr, false, 1,
                            dst, src, nullptr, InstOpt_WriteEnable);
                    };

                    auto iter = std::prev(bb->end());
                    for (int i = 0; i < num32BitFlags; ++i)
                    {
                        auto saveInst = createFlagSaveInst(i);
                        bb->insert(iter, saveInst);
                    }
                }

                {
                    //
                    // mov (1) f0.0:ud TMP_GRF.0<0;1,0>:ud
                    // mov (1) f1.0:ud TMP_GRF.1<0;1,0>:ud
                    //
                    auto createRestoreFlagInst = [&](int index)
                    {
                        auto flagDcl = tmpFlags[index];
                        G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, flagDcl->getRegVar(), 0, 0, 1, Type_UD);
                        RegionDesc* rDesc = builder.getRegionScalar();
                        G4_Operand* src = builder.createSrcRegRegion(
                            Mod_src_undef, Direct, savedDcl1->getRegVar(), 0, index, rDesc, Type_UD);
                        return builder.createInternalInst(nullptr, G4_mov, nullptr, false, 1,
                            dst, src, nullptr, InstOpt_WriteEnable);
                    };
                    auto insertIt = std::find_if(succ->begin(), succ->end(), [](G4_INST* inst) { return !inst->isLabel(); });
                    for (int i = 0; i < num32BitFlags; ++i)
                    {
                        auto restoreInst = createRestoreFlagInst(i);
                        succ->insert(insertIt, restoreInst);
                    }
                }
            }
        }
    }

    builder.instList.clear();
}

//
// Add GRF caller/callee save/restore code for stack calls.
//
void GraphColor::addSaveRestoreCode(unsigned localSpillAreaOwordSize)
{
    auto gtpin = builder.kernel.getGTPinData();
    if (gtpin &&
        gtpin->isFirstRAPass())
    {
        gtpin->markInsts();
    }

    if (builder.getIsKernel() == true)
    {
        unsigned int spillMemOffset = builder.getOptions()->getuInt32Option(vISA_SpillMemOffset);
        builder.kernel.fg.callerSaveAreaOffset =
            (spillMemOffset / 16) + localSpillAreaOwordSize;
    }
    else
    {
        builder.kernel.fg.calleeSaveAreaOffset = localSpillAreaOwordSize;
        addCalleeSaveRestoreCode();
    }
    addCallerSaveRestoreCode();
    if (builder.getIsKernel())
    {
        addGenxMainStackSetupCode();
    }
    else
    {
        addCalleeStackSetupCode();
    }
    stackCallProlog();
    builder.instList.clear();
}

//
// If the graph has stack calls, then add the caller-save pseudo code immediately before and
// after the stack call. The pseudo code is either converted to actual save/restore code or
// is eliminated at the end of coloringRegAlloc().
//
void GlobalRA::addCallerSavePseudoCode()
{
    std::vector<G4_Declare*>::iterator pseudoVCADclIt = builder.kernel.fg.pseudoVCADclList.begin();
    unsigned int retID = 0;

    for (BB_LIST_ITER it = builder.kernel.fg.begin(); it != builder.kernel.fg.end(); it++)
    {
        G4_BB* bb = *it;

        if (bb->isEndWithFCall())
        {
            // GRF caller save/restore
            G4_Declare* pseudoVCADcl = *pseudoVCADclIt;
            pseudoVCADclIt++;
            G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, pseudoVCADcl->getRegVar(), 0, 0, 1, Type_UD);
            G4_INST* saveInst = builder.createInternalInst(
                NULL, G4_pseudo_caller_save, NULL, false, 1,
                dst, NULL, NULL, InstOpt_WriteEnable);
            INST_LIST_ITER callBBIt = bb->end();
            bb->insert(--callBBIt, saveInst);

            G4_FCALL* fcall = builder.getFcallInfo(bb->back());
            MUST_BE_TRUE(fcall != NULL, "fcall info not found");
            uint16_t retSize = fcall->getRetSize();
            if (retSize > 0)
            {
                const char* name = builder.getNameString(builder.mem, 32, "FCALL_RETVAL_%d", retID++);
                G4_Declare* retDcl = builder.createDeclareNoLookup(name, G4_GRF, 8, retSize, Type_UD);
                retDcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(16), 0);
                fcallRetMap.insert(std::pair<G4_Declare*, G4_Declare*>(pseudoVCADcl, retDcl));
            }

            ASSERT_USER(bb->Succs.size() == 1, "fcall basic block cannot have more than 1 successor node");

            G4_BB* retBB = bb->Succs.front();
            RegionDesc* rd = builder.getRegionScalar();
            G4_Operand* src = builder.createSrcRegRegion(Mod_src_undef, Direct, pseudoVCADcl->getRegVar(), 0, 0, rd, Type_UD);
            INST_LIST_ITER retBBIt = retBB->begin();
            for (; retBBIt != retBB->end() && (*retBBIt)->isLabel(); ++retBBIt);
            G4_INST* restoreInst =
                builder.createInternalInst(
                    NULL, G4_pseudo_caller_restore, NULL, false, 1,
                    NULL, src, NULL, InstOpt_WriteEnable);
            retBB->insert(retBBIt, restoreInst);
        }
    }
    builder.instList.clear();
}

//
// If the graph has stack calls, then add the callee-save pseudo code at the entry/exit blocks
// of the function. The pseudo code is either converted to actual save/restore code or is
// eliminated at the end of coloringRegAlloc().
//
void GlobalRA::addCalleeSavePseudoCode()
{
    G4_Declare* pseudoVCEDcl = builder.kernel.fg.pseudoVCEDcl;

    G4_DstRegRegion* dst = builder.createDstRegRegion(Direct, pseudoVCEDcl->getRegVar(), 0, 0, 1, Type_UD);
    G4_INST* saveInst = builder.createInternalInst(
        NULL, G4_pseudo_callee_save, NULL, false, 1, dst,
        NULL, NULL, InstOpt_WriteEnable);
    INST_LIST_ITER insertIt = builder.kernel.fg.getEntryBB()->begin();
    for (; insertIt != builder.kernel.fg.getEntryBB()->end() && (*insertIt)->isLabel();
        ++insertIt)
    {   /*  void */
    };
    builder.kernel.fg.getEntryBB()->insert(insertIt, saveInst);

    G4_BB* exitBB = builder.kernel.fg.getUniqueReturnBlock();
    RegionDesc* rDesc = builder.getRegionScalar();
    G4_Operand* src = builder.createSrcRegRegion(
        Mod_src_undef, Direct, pseudoVCEDcl->getRegVar(), 0, 0, rDesc, Type_UD);
    G4_INST* restoreInst =
        builder.createInternalInst(
            NULL, G4_pseudo_callee_restore, NULL, false, 1, NULL,
            src, NULL, InstOpt_WriteEnable);
    INST_LIST_ITER exitBBIt = exitBB->end();
    --exitBBIt;
    MUST_BE_TRUE((*exitBBIt)->isFReturn(), ERROR_REGALLOC);
    exitBB->insert(exitBBIt, restoreInst);
    builder.instList.clear();
}

//
// Insert pseudo operation to store fp at entry and restore before return.
// Dst of store will be a temp that will run through RA and get an allocation.
//
void GlobalRA::addStoreRestoreForFP()
{
    G4_Declare* prevFP = builder.createTempVar(1, Type_UD, Any);
    oldFPDcl = prevFP;
    G4_DstRegRegion* oldFPDst = builder.createDstRegRegion(Direct, prevFP->getRegVar(), 0, 0, 1, Type_UD);
    RegionDesc* rd = builder.getRegionScalar();
    G4_Operand* oldFPSrc = builder.createSrcRegRegion(Mod_src_undef, Direct, prevFP->getRegVar(), 0, 0, rd, Type_UD);

    G4_DstRegRegion* FPdst = builder.createDstRegRegion(Direct, builder.kernel.fg.framePtrDcl->getRegVar(), 0, 0, 1, Type_UD);
    rd = builder.getRegionScalar();
    G4_Operand* FPsrc = builder.createSrcRegRegion(Mod_src_undef, Direct, builder.kernel.fg.framePtrDcl->getRegVar(), 0, 0, rd, Type_UD);

    G4_INST* storeInst = builder.createInternalInst(NULL, G4_pseudo_store_be_fp, NULL, false, 1,
        oldFPDst, FPsrc, NULL, 0);
    G4_INST* restoreInst = builder.createInternalInst(NULL, G4_pseudo_restore_be_fp, NULL, false, 1,
        FPdst, oldFPSrc, NULL, 0);

    auto gtpin = builder.kernel.getGTPinData();
    if (gtpin &&
        gtpin->isFirstRAPass())
    {
        gtpin->markInst(storeInst);
        gtpin->markInst(restoreInst);
    }

    INST_LIST_ITER insertIt = builder.kernel.fg.getEntryBB()->begin();
    for (; insertIt != builder.kernel.fg.getEntryBB()->end() && (*insertIt)->isLabel();
        ++insertIt)
    {   /*  void */
    };
    builder.kernel.fg.getEntryBB()->insert(insertIt, storeInst);

    insertIt = builder.kernel.fg.getUniqueReturnBlock()->end();
    for (--insertIt; (*insertIt)->isFReturn() == false; --insertIt)
    {   /*  void */
    };
    builder.kernel.fg.getUniqueReturnBlock()->insert(insertIt, restoreInst);
}

void GlobalRA::reportUndefinedUses(LivenessAnalysis& liveAnalysis, G4_BB* bb, G4_INST* inst, G4_Declare* referencedDcl, std::set<G4_Declare*>& defs, std::ofstream& optreport, Gen4_Operand_Number opndNum)
{
    // Get topmost dcl
    while (referencedDcl->getAliasDeclare() != NULL)
    {
        referencedDcl = referencedDcl->getAliasDeclare();
    }

    if (referencedDcl->getAddressed() == true)
    {
        // Dont run analysis for addressed opnds.
        // Specifically, we dont analyze following,
        //
        // A0 = &V1
        // r[A0] = 0 <-- V1 indirectly defined
        // ... = V1 <-- Use-before-def warning for V1 skipped due to indirect def
        //

        return;
    }

    if (referencedDcl->getRegVar()->isRegAllocPartaker())
    {
        const char* opndName = "";

        if (opndNum == Opnd_pred)
        {
            opndName = "predicate";
        }
        else if (opndNum == Opnd_src0)
        {
            opndName = "src0";
        }
        else if (opndNum == Opnd_src1)
        {
            opndName = "src1";
        }
        else if (opndNum == Opnd_src2)
        {
            opndName = "src2";
        }

        unsigned int id = referencedDcl->getRegVar()->getId();
        if (liveAnalysis.def_in[bb->getId()].isSet(id) == false &&
            defs.find(referencedDcl) == defs.end())
        {
            // Def not found for use so report it
            optreport << "Def not found for use " << referencedDcl->getName() <<
                " (" << opndName << ") at CISA offset " << inst->getCISAOff() << ", src line " <<
                inst->getLineNo() << ":" << std::endl;
            inst->emit(optreport);
            optreport << std::endl << std::endl;
        }
    }
}

void GlobalRA::updateDefSet(std::set<G4_Declare*>& defs, G4_Declare* referencedDcl)
{
    // Get topmost dcl
    while (referencedDcl->getAliasDeclare() != NULL)
    {
        referencedDcl = referencedDcl->getAliasDeclare();
    }

    defs.insert(referencedDcl);
}

void GlobalRA::detectUndefinedUses(LivenessAnalysis& liveAnalysis, G4_Kernel& kernel)
{
    // This function iterates over each inst and checks whether there is
    // a reaching def for each src operand. If not, it reports it to
    // opt report.
    std::ofstream optreport;
    getOptReportStream(optreport, kernel.getOptions());

    optreport << std::endl;
    if (liveAnalysis.livenessClass(G4_FLAG))
    {
        optreport << "=== Uses with reaching def - Flags ===" << std::endl;
    }
    else if (liveAnalysis.livenessClass(G4_ADDRESS))
    {
        optreport << "=== Uses with reaching def - Address ===" << std::endl;
    }
    else
    {
        optreport << "=== Uses with reaching def - GRF ===" << std::endl;
    }
    if (kernel.getOption(vISA_LocalRA))
    {
        optreport << "(Use -nolocalra switch for accurate results of uses without reaching defs)" << std::endl;
    }

    for (BB_LIST_ITER bb_it = kernel.fg.begin(), end = kernel.fg.end();
        bb_it != end;
        bb_it++)
    {
        G4_BB* bb = (*bb_it);
        std::set<G4_Declare*> defs;
        std::set<G4_Declare*>::iterator defs_it;
        G4_Declare* referencedDcl = NULL;

        for (INST_LIST_ITER inst_it = bb->begin(), iend = bb->end();
            inst_it != iend;
            inst_it++)
        {
            G4_INST* inst = (*inst_it);

            // Src/predicate opnds are uses
            if (inst->getPredicate() &&
                inst->getPredicate()->getBase() &&
                inst->getPredicate()->getBase()->isRegVar() &&
                inst->getPredicate()->getBase()->isRegAllocPartaker())
            {
                referencedDcl = inst->getPredicate()->asPredicate()->getBase()->asRegVar()->getDeclare();
                reportUndefinedUses(liveAnalysis, bb, inst, referencedDcl, defs, optreport, Opnd_pred);
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* opnd = inst->getSrc(i);

                if (opnd &&
                    opnd->isAddrExp() == false &&
                    opnd->getBase() &&
                    opnd->getBase()->isRegVar() &&
                    opnd->getBase()->isRegAllocPartaker())
                {
                    referencedDcl = opnd->getBase()->asRegVar()->getDeclare();
                    reportUndefinedUses(liveAnalysis, bb, inst, referencedDcl, defs, optreport, (Gen4_Operand_Number)(i + Opnd_src0));
                }
            }

            // Dst/cond modifier opnds are defs
            if (inst->getCondModBase() &&
                inst->getCondMod()->getBase()->isRegVar() &&
                inst->getCondMod()->getBase()->isRegAllocPartaker())
            {
                referencedDcl = inst->getCondMod()->asCondMod()->getBase()->asRegVar()->getDeclare();
                updateDefSet(defs, referencedDcl);
            }

            if (inst->getDst() &&
                inst->getDst()->getBase() &&
                inst->getDst()->getBase()->isRegVar() &&
                inst->getDst()->getBase()->isRegAllocPartaker())
            {
                referencedDcl = inst->getDst()->getBase()->asRegVar()->getDeclare();
                updateDefSet(defs, referencedDcl);
            }
        }
    }

    optreport << std::endl << std::endl;

    closeOptReportStream(optreport);
}

void GlobalRA::detectNeverDefinedUses()
{
    // Detect variables that are used but never defined in entire CFG.
    // This does not use liveness information.
    // Hold all decls from symbol table as key.
    // Boolean mapped value determines whether the dcl is
    // defined in kernel or not.
    std::map<G4_Declare*, bool> vars;
    std::map<G4_Declare*, bool>::iterator map_it;

    for (auto bb : kernel.fg)
    {
        for (INST_LIST_ITER inst_it = bb->begin(), iend = bb->end();
            inst_it != iend;
            inst_it++)
        {
            G4_INST* inst = (*inst_it);
            G4_Declare* referencedDcl = NULL;

            if (inst->getDst() &&
                inst->getDst()->getBase() &&
                inst->getDst()->getBase()->isRegVar())
            {
                referencedDcl = inst->getDst()->getBase()->asRegVar()->getDeclare();
                while (referencedDcl->getAliasDeclare() != NULL)
                {
                    referencedDcl = referencedDcl->getAliasDeclare();
                }

                // Always insert top-most dcl
                map_it = vars.find(referencedDcl);
                if (map_it == vars.end())
                {
                    vars.insert(make_pair(referencedDcl, true));
                }
                else
                {
                    (*map_it).second = true;
                }
            }

            if (inst->getCondModBase() &&
                inst->getCondMod()->getBase()->isRegVar())
            {
                referencedDcl = inst->getCondMod()->asCondMod()->getBase()->asRegVar()->getDeclare();
                while (referencedDcl->getAliasDeclare() != NULL)
                {
                    referencedDcl = referencedDcl->getAliasDeclare();
                }

                map_it = vars.find(referencedDcl);
                if (map_it == vars.end())
                {
                    vars.insert(make_pair(referencedDcl, true));
                }
                else
                {
                    (*map_it).second = true;
                }
            }

            if (inst->getPredicate() &&
                inst->getPredicate()->getBase() &&
                inst->getPredicate()->getBase()->isRegVar())
            {
                referencedDcl = inst->getPredicate()->asPredicate()->getBase()->asRegVar()->getDeclare();
                while (referencedDcl->getAliasDeclare() != NULL)
                {
                    referencedDcl = referencedDcl->getAliasDeclare();
                }

                // Check whether dcl was already added to list.
                // If not, add it with flag set to false to indicate
                // that a use was found but a def hasnt been seen yet.
                map_it = vars.find(referencedDcl);
                if (map_it == vars.end())
                {
                    vars.insert(make_pair(referencedDcl, false));
                }
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* opnd = inst->getSrc(i);

                if (opnd &&
                    opnd->getBase() &&
                    opnd->getBase()->isRegVar())
                {
                    referencedDcl = opnd->getBase()->asRegVar()->getDeclare();
                    while (referencedDcl->getAliasDeclare() != NULL)
                    {
                        referencedDcl = referencedDcl->getAliasDeclare();
                    }

                    map_it = vars.find(referencedDcl);
                    if (map_it == vars.end())
                    {
                        vars.insert(make_pair(referencedDcl, false));
                    }
                }
            }
        }
    }

    std::ofstream optreport;
    getOptReportStream(optreport, kernel.getOptions());
    optreport << std::endl << "=== Variables used but never defined ===" << std::endl << std::endl;

    for (auto dcl : kernel.Declares)
    {
        while (dcl->getAliasDeclare() != NULL)
        {
            dcl = dcl->getAliasDeclare();
        }

        map_it = vars.find(dcl);
        if (map_it != vars.end())
        {
            if ((*map_it).second == false &&
                dcl->getRegFile() != G4_INPUT &&
                dcl->getAddressed() == false)
            {
                // No def found for this non-input variable in
                // entire CFG so report it.
                optreport << dcl->getName();
                if (dcl->getRegFile() == G4_GRF)
                {
                    optreport << " (General)";
                }
                else if (dcl->getRegFile() == G4_ADDRESS)
                {
                    optreport << " (Address)";
                }
                else if (dcl->getRegFile() == G4_FLAG)
                {
                    optreport << " (Flag)";
                }

                optreport << std::endl;
            }
        }
    }

    optreport << std::endl << std::endl;

    closeOptReportStream(optreport);
}

void GlobalRA::emitVarLiveIntervals()
{
    for (auto dcl : kernel.Declares)
    {
        std::vector<std::pair<uint32_t, uint32_t>> liveIntervals;
        LiveIntervalInfo* lr = kernel.getKernelDebugInfo()->getLiveIntervalInfo(dcl, false);

        if (lr != NULL)
        {
            lr->getLiveIntervals(liveIntervals);

            if (liveIntervals.size() > 0)
            {
                DEBUG_VERBOSE(dcl->getName() << " - ");
            }

            for (auto&& i : liveIntervals)
            {
                std::cerr << "(" << i.first << ", " << i.second << ")\n";
            }
        }
    }
}

//
//  Check the overlap of two sources' ranges and do range splitting
//  Such as, range1: 0~63, range2: 32~95  --> 0~31,32~63,64~95
//       or, range1: 0~63, range2: 32~63  --> 0~31,32~63
//
VarRange* VarSplit::splitVarRange(VarRange *src1,
    VarRange *src2,
    std::stack<VarRange*> *toDelete)
{
    unsigned int left1, left2;
    unsigned int right1, right2;
    VarRange * new_var_range = NULL;

    ASSERT_USER(!(src1->leftBound == src2->leftBound && src1->rightBound == src2->rightBound), "Same ranges can not be spiltted");

    if (src1->leftBound > src2->rightBound ||
        src1->rightBound < src2->leftBound)  //No overlap
    {
        return NULL;
    }

    left1 = MIN(src1->leftBound, src2->leftBound);  //left
    right1 = MAX(src1->leftBound, src2->leftBound);

    left2 = MIN(src1->rightBound, src2->rightBound); //right
    right2 = MAX(src1->rightBound, src2->rightBound);

    if (left1 == right1) //Same left
    {
        src1->leftBound = left1;
        src1->rightBound = left2;

        src2->leftBound = left2 + 1;
        src2->rightBound = right2;
    }
    else if (left2 == right2)  //Same right
    {
        src1->leftBound = left1;
        src1->rightBound = right1 - 1;
        src2->leftBound = right1;
        src2->rightBound = right2;
    }
    else  //No same boundary
    {
        src1->leftBound = left1;           //Left one: in list already
        src1->rightBound = right1 - 1;

        src2->leftBound = left2 + 1;       //Right one: keep in list
        src2->rightBound = right2;

        new_var_range = new VarRange;
        new_var_range->leftBound = right1; //Middle one: need add one range object
        new_var_range->rightBound = left2;
        toDelete->push(new_var_range);
    }

    return new_var_range;
}

//
// Scan the range list, Insert the new range into the range list.
// Range splitting is applied if required.
//
void VarSplit::rangeListSpliting(VAR_RANGE_LIST *rangeList, G4_Operand *opnd, std::stack<VarRange*> *toDelete)
{
    VarRange *range = new VarRange;
    range->leftBound = opnd->getLeftBound();
    range->rightBound = opnd->getRightBound();
    toDelete->push(range);

    VAR_RANGE_LIST_ITER it = rangeList->begin();

    //The ranges in the list are ordered from low to high
    while (it != rangeList->end())
    {
        if ((*it)->leftBound == range->leftBound &&
            ((*it)->rightBound == range->rightBound))
        {
            //Same range exists in the list already
            return;
        }

        if ((*it)->leftBound > range->rightBound)
        {
            //The range item in the list is on the right of current range, insert it before the postion.
            //Since the whole range is inserted first, all the ranges should be continous.
            ASSERT_USER((*it)->leftBound - range->rightBound == 1, "none continous spliting happened\n");
            rangeList->insert(it, range);
            return;
        }

        //Overlap happened, do splitting.
        //(*lt) is updated to the left range
        //"range" is updated to the right range
        //If "newRange" is not NULL, it's the middle range.
        VarRange *newRange = splitVarRange((*it), range, toDelete);

        //Insert the middle one
        it++;
        if (newRange)
        {
            it = rangeList->insert(it, newRange);
        }
    }

    rangeList->push_back(range);  //Insert the right one

    return;
}

void VarSplit::getHeightWidth(G4_Type type, unsigned int numberElements, unsigned short &dclWidth, unsigned short &dclHeight, int &totalByteSize)
{
    dclWidth = 1, dclHeight = 1;
    totalByteSize = numberElements * G4_Type_Table[type].byteSize;
    if (totalByteSize <= G4_GRF_REG_NBYTES)
    {
        dclWidth = (uint16_t)numberElements;
    }
    else {
        // here we assume that the start point of the var is the beginning of a GRF?
        // so subregister must be 0?
        dclWidth = G4_GRF_REG_NBYTES / G4_Type_Table[type].byteSize;
        dclHeight = totalByteSize / G4_GRF_REG_NBYTES;
        if (totalByteSize % G4_GRF_REG_NBYTES != 0) {
            dclHeight++;
        }
    }
}


void VarSplit::createSubDcls(G4_Kernel& kernel, G4_Declare* oldDcl, std::vector<G4_Declare*> &splitDclList)
{
    if (oldDcl->getByteSize() <= G4_GRF_REG_NBYTES || oldDcl->getByteSize() % G4_GRF_REG_NBYTES)
    {
        return;
    }

    int splitVarSize = kernel.getSimdSize() == 8 ? 1 : 2;
    for (unsigned int i = 0; i < oldDcl->getByteSize() / G4_GRF_REG_NBYTES; i += splitVarSize)
    {
        G4_Declare* splitDcl = NULL;
        unsigned leftBound = i * G4_GRF_REG_NBYTES;
        unsigned rightBound = (i + splitVarSize) * G4_GRF_REG_NBYTES - 1;
        unsigned short dclWidth = 0;
        unsigned short dclHeight = 0;
        int dclTotalSize = 0;

        getHeightWidth(oldDcl->getElemType(), (rightBound - leftBound + 1) / oldDcl->getElemSize(), dclWidth, dclHeight, dclTotalSize);
        const char* splitDclName = kernel.fg.builder->getNameString(kernel.fg.builder->mem, 16, "split_%d_%s", i, oldDcl->getName());
        splitDcl = kernel.fg.builder->createDeclareNoLookup(splitDclName, G4_GRF, dclWidth, dclHeight, oldDcl->getElemType());
        gra.setSubOffset(splitDcl, leftBound);
        splitDcl->copyAlign(oldDcl);
        gra.copyAlignment(splitDcl, oldDcl);
        unsigned nElementSize = (rightBound - leftBound + 1) / oldDcl->getElemSize();
        if ((rightBound - leftBound + 1) % oldDcl->getElemSize())
        {
            nElementSize++;
        }
        splitDcl->setTotalElems(nElementSize);
        splitDclList.push_back(splitDcl);
    }

    return;
}

void VarSplit::insertMovesToTemp(IR_Builder& builder, G4_Declare* oldDcl, G4_Operand *dstOpnd, G4_BB* bb, INST_LIST_ITER instIter, std::vector<G4_Declare*> &splitDclList)
{
    G4_INST *inst = (*instIter);
    INST_LIST_ITER iter = instIter;
    iter++;

    for (size_t i = 0, size = splitDclList.size(); i < size; i++)
    {
        G4_Declare * subDcl = splitDclList[i];
        unsigned leftBound = gra.getSubOffset(subDcl);
        unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

        if (!(dstOpnd->getRightBound() < leftBound || rightBound < dstOpnd->getLeftBound()))
        {
            unsigned maskFlag = (inst->getOption() & 0xFFF010C);
            G4_DstRegRegion* dst = builder.Create_Dst_Opnd_From_Dcl(subDcl, 1);
            G4_SrcRegRegion* src = builder.Create_Src_Opnd_From_Dcl(oldDcl, builder.getRegionStride1());
            src->setRegOff((gra.getSubOffset(subDcl)) / G4_GRF_REG_NBYTES);
            G4_INST* splitInst = builder.createInternalInst(nullptr, G4_mov, nullptr, false,
                (unsigned char)subDcl->getTotalElems(), dst, src, nullptr, maskFlag,
                inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());
            bb->insert(iter, splitInst);
        }
    }

    return;
}

void VarSplit::insertMovesFromTemp(G4_Kernel& kernel, G4_Declare* oldDcl, int index, G4_Operand *srcOpnd, int pos, G4_BB* bb, INST_LIST_ITER instIter, std::vector<G4_Declare*> &splitDclList)
{
    G4_INST *inst = (*instIter);

    int sizeInGRF = (srcOpnd->getRightBound() - srcOpnd->getLeftBound() + G4_GRF_REG_NBYTES - 1) /
        G4_GRF_REG_NBYTES;
    int splitSize = kernel.getSimdSize() == 8 ? 1 : 2;
    if (sizeInGRF != splitSize)
    {
        unsigned short dclWidth = 0;
        unsigned short dclHeight = 0;
        int dclTotalSize = 0;
        G4_SrcRegRegion* oldSrc = srcOpnd->asSrcRegRegion();
        getHeightWidth(oldSrc->getType(), (srcOpnd->getRightBound() - srcOpnd->getLeftBound() + 1) / oldSrc->getElemSize(), dclWidth, dclHeight, dclTotalSize);
        const char* newDclName = kernel.fg.builder->getNameString(kernel.fg.builder->mem, 16, "copy_%d_%s", index, oldDcl->getName());
        G4_Declare * newDcl = kernel.fg.builder->createDeclareNoLookup(newDclName, G4_GRF, dclWidth, dclHeight, oldSrc->getType());
        newDcl->copyAlign(oldDcl);
        gra.copyAlignment(newDcl, oldDcl);

        unsigned newLeftBound = 0;

        for (size_t i = 0, size = splitDclList.size(); i < size; i++)
        {
            G4_Declare * subDcl = splitDclList[i];
            unsigned leftBound = gra.getSubOffset(subDcl);
            unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

            if (!(srcOpnd->getRightBound() < leftBound || rightBound < srcOpnd->getLeftBound()))
            {

                G4_DstRegRegion* dst = kernel.fg.builder->createDstRegRegion(Direct,
                    newDcl->getRegVar(),
                    newLeftBound / G4_GRF_REG_NBYTES,
                    0,
                    1,
                    oldSrc->getType());
                newLeftBound += subDcl->getByteSize();
                G4_SrcRegRegion* src = kernel.fg.builder->createSrcRegRegion(
                    Mod_src_undef,
                    Direct,
                    subDcl->getRegVar(),
                    0,
                    0,
                    kernel.fg.builder->getRegionStride1(),
                    oldSrc->getType());
                G4_INST* movInst = kernel.fg.builder->createInternalInst(nullptr, G4_mov, nullptr, false,
                    (unsigned char)subDcl->getTotalElems(), dst, src, nullptr, InstOpt_WriteEnable,
                    inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());
                bb->insert(instIter, movInst);
            }
        }
        G4_SrcRegRegion* newSrc = kernel.fg.builder->Create_Src_Opnd_From_Dcl(newDcl, oldSrc->getRegion());
        newSrc->setRegOff(0);
        newSrc->setSubRegOff(oldSrc->getSubRegOff());
        newSrc->setModifier(oldSrc->getModifier());
        inst->setSrc(newSrc, pos);
    }
    else
    {
        for (size_t i = 0, size = splitDclList.size(); i < size; i++)
        {
            G4_Declare * subDcl = splitDclList[i];
            unsigned leftBound = gra.getSubOffset(subDcl);
            unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

            if (!(srcOpnd->getRightBound() < leftBound || rightBound < srcOpnd->getLeftBound()))
            {
                G4_SrcRegRegion* oldSrc = srcOpnd->asSrcRegRegion();
                G4_SrcRegRegion* newSrc = kernel.fg.builder->createSrcRegRegion(oldSrc->getModifier(),
                    Direct,
                    subDcl->getRegVar(),
                    0,
                    oldSrc->getSubRegOff(),
                    oldSrc->getRegion(),
                    oldSrc->getType());
                inst->setSrc(newSrc, pos);
                break;
            }
        }
    }

    return;
}

bool VarSplit::canDoGlobalSplit(IR_Builder& builder, G4_Kernel &kernel, uint32_t instNum, uint32_t spillRefCount, uint32_t sendSpillRefCount)
{
    if (!builder.getOption(vISA_GlobalSendVarSplit))
    {
        return false;
    }

    if (!builder.getOption(vISA_Debug) &&               //Not work in debug mode
        kernel.getOptions()->getTarget() == VISA_3D &&      //Only works for 3D/OCL/OGL
        kernel.getSimdSize() < 16 &&                        //Only works for simd8, FIXME:can work for SIMD16 also
        sendSpillRefCount &&              //There is spills/fills are due to the interference with send related varaibles.
        (float)spillRefCount / sendSpillRefCount < 2.0 && //Most spilled varaibles interference with splittable send instructions.
        (float)instNum / sendSpillRefCount < 20.0) //The percentage of the spill instructions is high enough.
    {
        return true;
    }

    return false;
}

void VarSplit::globalSplit(IR_Builder& builder, G4_Kernel &kernel)
{
    typedef std::list<std::tuple<G4_BB*, G4_Operand*, int, unsigned, INST_LIST_ITER>> SPLIT_OPERANDS;
    typedef std::list<std::tuple<G4_BB*, G4_Operand*, int, unsigned, INST_LIST_ITER>>::iterator SPLIT_OPERANDS_ITER;
    typedef std::map<G4_RegVar*, SPLIT_OPERANDS> SPLIT_DECL_OPERANDS;
    typedef std::map<G4_RegVar*, SPLIT_OPERANDS>::iterator SPLIT_DECL_OPERANDS_ITER;

    SPLIT_DECL_OPERANDS splitDcls;
    unsigned int instIndex = 0;
    int splitSize = kernel.getSimdSize() == 8 ? 1 : 2;
    for (auto bb : kernel.fg)
    {
        for (INST_LIST_ITER it = bb->begin(), iend = bb->end(); it != iend; ++it, ++instIndex)
        {
            G4_INST* inst = (*it);
            G4_DstRegRegion* dst = inst->getDst();

            if (inst->opcode() == G4_pseudo_lifetime_end || inst->isPseudoKill())
            {
                continue;
            }

            //
            // process send destination operand
            //
            if (inst->isSend() &&
                inst->getMsgDesc()->ResponseLength() > splitSize &&
                inst->asSendInst()->isDirectSplittableSend())
            {
                G4_DstRegRegion* dstrgn = dst;
                G4_Declare* topdcl = GetTopDclFromRegRegion(dstrgn);

                if (topdcl &&
                    dstrgn->getRegAccess() == Direct &&
                    !topdcl->getAddressed() &&
                    topdcl->getRegFile() != G4_INPUT &&
                    (dstrgn->getRightBound() - dstrgn->getLeftBound() + 1) == topdcl->getByteSize() &&
                    (dstrgn->getRightBound() - dstrgn->getLeftBound()) > G4_GRF_REG_NBYTES)
                {
                    //The tuple<G4_BB*, G4_Operand*, int pos, unsigned instIndex, INST_LIST_ITER>,
                    //these info are tuning and split operand/instruction generation
                    splitDcls[topdcl->getRegVar()].push_front(make_tuple(bb, dst, 0, instIndex, it));
                }
            }
        }
    }

    instIndex = 0;
    for (auto bb : kernel.fg)
    {
        for (INST_LIST_ITER it = bb->begin(), end = bb->end(); it != end; ++it, ++instIndex)
        {

            G4_INST* inst = (*it);

            if (inst->opcode() == G4_pseudo_lifetime_end || inst->isPseudoKill())
            {
                continue;
            }

            //
            // process each source operand
            //
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* src = inst->getSrc(j);

                if (src == NULL)
                {
                    continue;
                }

                if (src->isSrcRegRegion())
                {
                    G4_Declare* topdcl = GetTopDclFromRegRegion(src);

                    if (topdcl &&
                        topdcl->getRegFile() != G4_INPUT &&
                        !topdcl->getAddressed() &&
                        splitDcls.find(topdcl->getRegVar()) != splitDcls.end() &&
                        ((src->asSrcRegRegion()->getRightBound() - src->asSrcRegRegion()->getLeftBound() + 1) < topdcl->getByteSize()) &&
                        src->asSrcRegRegion()->getRegAccess() == Direct)  //We don't split the indirect access
                    {
                        splitDcls[topdcl->getRegVar()].push_back(make_tuple(bb, src, j, instIndex, it));
                    }
                }
            }
        }
    }

    for (SPLIT_DECL_OPERANDS_ITER it = splitDcls.begin();
        it != splitDcls.end();)
    {
        unsigned srcIndex = 0xFFFFFFFF;
        unsigned dstIndex = 0;
        SPLIT_DECL_OPERANDS_ITER succIt = it;
        succIt++;
        G4_Declare * topDcl = (*it).first->getDeclare();
        if (topDcl->getByteSize() <= G4_GRF_REG_NBYTES * 2u)
        {
            splitDcls.erase(it);
            it = succIt;
            continue;
        }

        bool hasSrcOpearnd = false;
        for (SPLIT_OPERANDS_ITER vt = (*it).second.begin(); vt != (*it).second.end(); vt++)
        {
            G4_BB *bb = nullptr;
            G4_Operand *opnd = nullptr;
            INST_LIST_ITER instIter;
            int pos = 0;
            unsigned iIndex = 0;

            std::tie(bb, opnd, pos, iIndex, instIter) = (*vt);

            if (opnd == nullptr)
            {
                continue;
            }

            if (opnd->isDstRegRegion())
            {
                dstIndex = std::max(dstIndex, iIndex);
            }

            if (opnd->isSrcRegRegion())
            {
                srcIndex = std::min(srcIndex, iIndex);
                hasSrcOpearnd = true;
            }
        }

        if (!hasSrcOpearnd || (dstIndex > srcIndex &&
            dstIndex - srcIndex < (*it).second.size() + 1))
        {
            splitDcls.erase(it);
            it = succIt;
            continue;
        }

        it++;
    }

    for (SPLIT_DECL_OPERANDS_ITER it = splitDcls.begin();
        it != splitDcls.end();
        it++)
    {
        G4_Declare * topDcl = (*it).first->getDeclare();
        std::vector<G4_Declare*> splitDclList;
        splitDclList.clear();

        createSubDcls(kernel, topDcl, splitDclList);
        int srcIndex = 0;
        for (SPLIT_OPERANDS_ITER vt = (*it).second.begin(); vt != (*it).second.end(); vt++)
        {
            G4_BB *bb = nullptr;
            G4_Operand *opnd = nullptr;
            INST_LIST_ITER instIter;
            int pos = 0;
            unsigned instIndex = 0;
            std::tie(bb, opnd, pos, instIndex, instIter) = (*vt);

            if (opnd == nullptr)
            {
                continue;
            }

            if (opnd->isDstRegRegion())
            {
                insertMovesToTemp(builder, topDcl, opnd, bb, instIter, splitDclList);
            }

            if (opnd->isSrcRegRegion())
            {
                insertMovesFromTemp(kernel, topDcl, srcIndex, opnd, pos, bb, instIter, splitDclList);
            }

            srcIndex++;
        }
    }

    return;
}

void VarSplit::localSplit(IR_Builder& builder,
    G4_BB* bb)
{
    class CmpRegVarId
    {
    public:
        bool operator()(G4_RegVar* first, G4_RegVar* second) const
        {
            return first->getDeclare()->getDeclId() < second->getDeclare()->getDeclId();
        }
    };
    std::map<G4_RegVar*, std::vector<std::pair<G4_Operand*, INST_LIST_ITER>>, CmpRegVarId> localRanges;
    std::map<G4_RegVar*, std::vector<std::pair<G4_Operand*, INST_LIST_ITER>>, CmpRegVarId>::iterator localRangesIt;
    std::map<G4_RegVar*, VarRangeListPackage, CmpRegVarId> varRanges;
    std::map<G4_RegVar*, VarRangeListPackage, CmpRegVarId>::iterator varRangesIt;
    std::stack<VarRange*> toDelete;

    //
    // Iterate instruction in BB from back to front
    //
    for (INST_LIST::reverse_iterator rit = bb->rbegin(), rend = bb->rend(); rit != rend; ++rit)
    {
        G4_INST* i = (*rit);
        G4_DstRegRegion* dst = i->getDst();

        if (i->opcode() == G4_pseudo_lifetime_end || i->isPseudoKill())
        {
            continue;
        }

        //
        // process destination operand
        //
        if (dst != NULL)
        {
            G4_DstRegRegion* dstrgn = dst;

            //It's RA candidate
            G4_Declare* topdcl = GetTopDclFromRegRegion(dstrgn);

            LocalLiveRange* topdclLR = nullptr;
            //Local only
            if ((topdcl &&
                (topdclLR = gra.getLocalLR(topdcl)) &&
                topdcl->getIsRefInSendDcl() &&
                topdclLR->isLiveRangeLocal()) &&
                topdcl->getRegFile() != G4_INPUT)
            {
                varRangesIt = varRanges.find(topdcl->getRegVar());
                INST_LIST_ITER iterToInsert = rit.base();
                iterToInsert--; //Point to the iterator of current instruction
                if (varRangesIt == varRanges.end())
                {
                    VarRange* new_range = new VarRange;
                    new_range->leftBound = 0;
                    new_range->rightBound = topdcl->getByteSize() - 1;
                    toDelete.push(new_range);
                    varRanges[topdcl->getRegVar()].list.push_back(new_range);
                }
                else
                {
                    rangeListSpliting(&(varRanges[topdcl->getRegVar()].list), dstrgn, &toDelete);
                }

                localRanges[topdcl->getRegVar()].push_back(pair<G4_Operand*, INST_LIST_ITER>(dst, iterToInsert));  //Ordered from back to front.
            }
        }

        //
        // process each source operand
        //
        for (unsigned j = 0; j < G4_MAX_SRCS; j++)
        {
            G4_Operand* src = i->getSrc(j);

            if (src == NULL)
            {
                continue;
            }

            //Local only
            if (src->isSrcRegRegion())
            {
                G4_Declare* topdcl = GetTopDclFromRegRegion(src);
                LocalLiveRange* topdclLR = nullptr;

                if (topdcl &&
                    (topdclLR = gra.getLocalLR(topdcl)) &&
                    topdcl->getIsRefInSendDcl() &&
                    topdclLR->isLiveRangeLocal() &&
                    topdcl->getRegFile() != G4_INPUT)
                {
                    G4_VarBase* base = (topdcl != NULL ? topdcl->getRegVar() : src->asSrcRegRegion()->getBase());

                    INST_LIST_ITER iterToInsert = rit.base();
                    iterToInsert--;

                    varRangesIt = varRanges.find(base->asRegVar());
                    if (varRangesIt == varRanges.end())
                    {
                        VarRange* new_range = new VarRange;
                        new_range->leftBound = 0;
                        new_range->rightBound = topdcl->getByteSize() - 1;
                        toDelete.push(new_range);
                        varRanges[topdcl->getRegVar()].list.push_back(new_range);
                    }

                    rangeListSpliting(&(varRanges[topdcl->getRegVar()].list), src, &toDelete);

                    localRanges[topdcl->getRegVar()].push_back(pair<G4_Operand*, INST_LIST_ITER>(src, iterToInsert));  //Ordered from back to front.
                }
            }
        }
    }

    //Clean the varaibles without no partial usage, or whose partial live range is too short
    std::map<G4_RegVar*, VarRangeListPackage>::iterator it = varRanges.begin();
    while (it != varRanges.end())
    {
        std::map<G4_RegVar*, VarRangeListPackage>::iterator succ_it = it;
        succ_it++;

        //No partial
        if ((*it).second.list.size() <= 1)
        {
            varRanges.erase(it);
            it = succ_it;
            continue;
        }

        //If total GRF size divides partial number is less than 16 bytes (half GRF), remove it
        if (((*(*it).second.list.rbegin())->rightBound - (*(*it).second.list.begin())->leftBound) / (*it).second.list.size() < G4_GRF_REG_SIZE * 2 / 2)
        {
            varRanges.erase(it);
            it = succ_it;
            continue;
        }

        G4_Declare * topDcl = (*it).first->getDeclare();
        bool aligned = true;
        for (VAR_RANGE_LIST_ITER vt = (*it).second.list.begin(); vt != (*it).second.list.end(); vt++)
        {
            unsigned leftBound = (*vt)->leftBound;
            unsigned rightBound = (*vt)->rightBound;
            int elementSize = topDcl->getElemSize() > G4_WSIZE ? topDcl->getElemSize() : G4_WSIZE;
            unsigned short elemsNum = (rightBound - leftBound + 1) / elementSize;

            if (!elemsNum)
            {
                aligned = false;
                break;
            }

            //TODO: we can merge serveral unaligned sub declares into one aligned.  Such as [0-1], [2-63]  --> [0-63]
            if (leftBound % G4_GRF_REG_SIZE || (rightBound + 1) % G4_GRF_REG_SIZE)
            {
                aligned = false;
                break;
            }
        }

        if (!aligned)
        {
            varRanges.erase(it);
            it = succ_it;
            continue;
        }


        it = succ_it;
    }

    int splitid = 0;
    for (std::map<G4_RegVar*, VarRangeListPackage>::iterator it = varRanges.begin();
        it != varRanges.end();
        it++)
    {
        G4_Declare * topDcl = (*it).first->getDeclare();
        const char * dclName = topDcl->getName();

        topDcl->setIsSplittedDcl(true);

        // Vertical split: varaible split
        unsigned splitVarNum = 0;
        unsigned pre_rightBound = 0;
        for (VAR_RANGE_LIST_ITER vt = (*it).second.list.begin(); vt != (*it).second.list.end(); vt++)
        {
            unsigned leftBound = (*vt)->leftBound;
            unsigned rightBound = (*vt)->rightBound;
            int elementSize = topDcl->getElemSize() > G4_WSIZE ? topDcl->getElemSize() : G4_WSIZE;
            unsigned short elemsNum = (rightBound - leftBound + 1) / elementSize;

            if (!elemsNum)
            {
                assert(0);
                pre_rightBound = rightBound;
                continue;
            }

            if (leftBound && pre_rightBound + 1 != leftBound)
            {
                assert(0);
            }
            pre_rightBound = rightBound;

            const char* name = builder.getNameString(builder.mem, strlen(dclName) + 16, "%s_%d_%d_%d", dclName, splitid, leftBound, rightBound);
            unsigned short dclWidth = 0;
            unsigned short dclHeight = 0;
            int dclTotalSize = 0;

            getHeightWidth(topDcl->getElemType(), (rightBound - leftBound + 1) / topDcl->getElemSize(), dclWidth, dclHeight, dclTotalSize);
            G4_Declare* partialDcl = builder.createDeclareNoLookup(name, G4_GRF, dclWidth, dclHeight, topDcl->getElemType());
            gra.setSubOffset(partialDcl, leftBound);
            partialDcl->setIsPartialDcl(true);
            gra.setSplittedDeclare(partialDcl, topDcl);
            unsigned nElementSize = (rightBound - leftBound + 1) / topDcl->getElemSize();
            if ((rightBound - leftBound + 1) % topDcl->getElemSize())
            {
                nElementSize++;
            }
            partialDcl->setTotalElems(nElementSize);
            gra.addSubDcl(topDcl, partialDcl);
            splitVarNum++;
#ifdef DEBUG_VERBOSE_ON
            std::cout << "==> Sub Declare: " << splitid << "::" << name << std::endl;
#endif
            splitid++;
        }
        if (splitVarNum)
        {
            gra.setSplitVarNum(topDcl, splitVarNum);
        }
    }

    while (toDelete.size() > 0)
    {
        delete toDelete.top();
        toDelete.pop();
    }

    return;
}

void GlobalRA::addrRegAlloc()
{
    uint32_t addrSpillId = 0;
    unsigned maxRAIterations = 10;
    unsigned iterationNo = 0;

    while (iterationNo < maxRAIterations)
    {
        if (builder.getOption(vISA_RATrace))
        {
            std::cout << "--address RA iteration " << iterationNo << "\n";
        }
        //
        // choose reg vars whose reg file kind is ARF
        //
        LivenessAnalysis liveAnalysis(*this, G4_ADDRESS);
        liveAnalysis.computeLiveness();

        //
        // if no reg var needs to reg allocated, then skip reg allocation
        //
        if (liveAnalysis.getNumSelectedVar() > 0)
        {
            GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false, false);
            unsigned spillRegSize = 0;
            unsigned indrSpillRegSize = 0;
            if (coloring.regAlloc(false, false, false, spillRegSize, indrSpillRegSize, nullptr) == false)
            {
                SpillManager spillARF(*this, coloring.getSpilledLiveRanges(), addrSpillId);
                spillARF.insertSpillCode();
                addrSpillId = spillARF.getNextTempDclId();

                //
                // if new addr temps are created, we need to do RA again so that newly created temps
                // can get registers. If there are no more newly created temps, we then commit reg assignments
                //
                if (spillARF.isAnyNewTempCreated() == false)
                {
                    coloring.confirmRegisterAssignments();
                    coloring.cleanupRedundantARFFillCode();
                    if ((builder.kernel.fg.getHasStackCalls() || builder.kernel.fg.getIsStackCallFunc()))
                    {
                        coloring.addA0SaveRestoreCode();
                    }
                    break; // no more new addr temps; done with ARF allocation
                }
            }
            else  // successfully allocate register without spilling
            {
                coloring.confirmRegisterAssignments();
                coloring.cleanupRedundantARFFillCode();
                if ((builder.kernel.fg.getHasStackCalls() || builder.kernel.fg.getIsStackCallFunc()))
                {
                    coloring.addA0SaveRestoreCode();
                }
                if (builder.getOption(vISA_OptReport))
                {
                    detectUndefinedUses(liveAnalysis, kernel);
                }

                break; // done with ARF allocation
            }
        }
        else {
            break; // no ARF allocation needed
        }

        iterationNo++;

        if (builder.getOption(vISA_DumpDotAll))
        {
            kernel.dumpDotFile("Address_RA");
        }
    }

    MUST_BE_TRUE(iterationNo < maxRAIterations, "Address RA has failed.");
}

void GlobalRA::flagRegAlloc()
{
    uint32_t flagSpillId = 0;
    unsigned maxRAIterations = 10;
    uint32_t iterationNo = 0;
    bool spillingFlag = false;

    while (iterationNo < maxRAIterations)
    {
        if (builder.getOption(vISA_RATrace))
        {
            std::cout << "--flag RA iteration " << iterationNo << "\n";
        }

        //
        // choose reg vars whose reg file kind is FLAG
        //
        LivenessAnalysis liveAnalysis(*this, G4_FLAG);
        liveAnalysis.computeLiveness();

        //
        // if no reg var needs to reg allocated, then skip reg allocation
        //
        if (liveAnalysis.getNumSelectedVar() > 0)
        {
            GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false, false);
            unsigned spillRegSize = 0;
            unsigned indrSpillRegSize = 0;
            if (coloring.regAlloc(false, false, false, spillRegSize, indrSpillRegSize, nullptr) == false)
            {
                SpillManager spillFlag(*this, coloring.getSpilledLiveRanges(), flagSpillId);
                spillFlag.insertSpillCode();
#ifdef DEBUG_VERBOSE_ON
                printf("FLAG Spill inst count: %d\n", spillFlag.getNumFlagSpillStore());
                printf("FLAG Fill inst count: %d\n", spillFlag.getNumFlagSpillLoad());
                printf("*************************\n");
#endif
                flagSpillId = spillFlag.getNextTempDclId();

                spillingFlag = true;
                if (spillFlag.isAnyNewTempCreated() == false)
                {
                    coloring.confirmRegisterAssignments();

                    if ((builder.kernel.fg.getHasStackCalls() || builder.kernel.fg.getIsStackCallFunc()))
                    {
                        coloring.addFlagSaveRestoreCode();
                    }
                    break;
                }
                builder.getJitInfo()->numFlagSpillStore = spillFlag.getNumFlagSpillStore();
                builder.getJitInfo()->numFlagSpillLoad = spillFlag.getNumFlagSpillLoad();
            }
            else  // successfully allocate register without spilling
            {
                coloring.confirmRegisterAssignments();
                if ((builder.kernel.fg.getHasStackCalls() || builder.kernel.fg.getIsStackCallFunc()))
                {
                    coloring.addFlagSaveRestoreCode();
                }

                if (spillingFlag && builder.getOption(vISA_FlagSpillCodeCleanup))
                {
                    CLEAN_NUM_PROFILE clean_num_profile;

#ifdef DEBUG_VERBOSE_ON
                    for (int i = 0; i < 3; i++)
                    {
                        clean_num_profile.spill_clean_num[i] = 0;
                        clean_num_profile.fill_clean_num[i] = 0;
                    }
#endif

                    FlagSpillCleanup f(*this);
                    f.spillFillCodeCleanFlag(builder, kernel, &clean_num_profile);

#ifdef DEBUG_VERBOSE_ON1
                    for (int i = 0; i < 3; i++)
                    {
                        printf("Profiler %d Spill clean: %d\n", i, clean_num_profile.spill_clean_num[i]);
                        printf("Profiler %d Fill clean: %d\n", i, clean_num_profile.fill_clean_num[i]);
                        clean_num += clean_num_profile.spill_clean_num[i];
                        clean_num += clean_num_profile.fill_clean_num[i];
                    }
                    printf("**Flag clean num: %d\n", clean_num);
#endif
                }

                if (builder.getOption(vISA_OptReport))
                {
                    detectUndefinedUses(liveAnalysis, kernel);
                }

                break; // done with FLAG allocation
            }
        }
        else {
            break; // no FLAG allocation needed
        }

        iterationNo++;

        if (builder.getOption(vISA_DumpDotAll))
        {
            kernel.dumpDotFile("Flag_RA");
        }
    }

    MUST_BE_TRUE(iterationNo < maxRAIterations, "Flag RA has failed.");
}

void GlobalRA::assignRegForAliasDcl()
{
    //
    // assign Reg for Alias DCL
    //
    for (DECLARE_LIST_ITER di = kernel.Declares.begin(), end = kernel.Declares.end(); di != end; ++di)
    {
        G4_RegVar * AliasRegVar;
        G4_RegVar * CurrentRegVar;
        unsigned tempoffset;

        if ((*di)->getAliasDeclare() != NULL)
        {
            AliasRegVar = (*di)->getAliasDeclare()->getRegVar();
            CurrentRegVar = (*di)->getRegVar();
            tempoffset = AliasRegVar->getPhyRegOff()*AliasRegVar->getDeclare()->getElemSize() + (*di)->getAliasOffset();
            if (AliasRegVar->getPhyReg() != NULL)
            {
                //
                // alias register assignment for A0
                //
                if (CurrentRegVar->getDeclare()->useGRF())
                {
                    // if the tempoffset is one grf
                    if (tempoffset < G4_GRF_REG_SIZE * 2u)
                    {
                        CurrentRegVar->setPhyReg(AliasRegVar->getPhyReg(), tempoffset / CurrentRegVar->getDeclare()->getElemSize());
                    }
                    // tempoffset covers several GRFs
                    else
                    {
                        unsigned addtionalrow = tempoffset / (G4_GRF_REG_SIZE * 2);
                        unsigned actualoffset = tempoffset % (G4_GRF_REG_SIZE * 2);
                        bool valid = false;
                        unsigned orignalrow = AliasRegVar->ExRegNum(valid);
                        MUST_BE_TRUE(valid == true, ERROR_REGALLOC);
                        CurrentRegVar->setPhyReg(regPool.getGreg(orignalrow + addtionalrow), actualoffset / CurrentRegVar->getDeclare()->getElemSize());
                    }
                }
                else if (CurrentRegVar->getDeclare()->getRegFile() == G4_ADDRESS)
                {
                    MUST_BE_TRUE(tempoffset < getNumAddrRegisters() * 2,
                        ERROR_REGALLOC);    // Must hold tempoffset in one A0 reg
                    CurrentRegVar->setPhyReg(AliasRegVar->getPhyReg(), tempoffset / CurrentRegVar->getDeclare()->getElemSize());
                }
                else
                {
                    MUST_BE_TRUE(false, ERROR_REGALLOC);
                }
            }
            else {
                // Propagate addr taken spill/fill to aliases
                CurrentRegVar->getDeclare()->setAddrTakenSpillFill(AliasRegVar->getDeclare()->getAddrTakenSpillFill());

                if ((*di)->isSpilled() == false)
                    (*di)->setSpillFlag();
            }
        }
    }

    return;
}

void GlobalRA::removeSplitDecl()
{
    for (auto dcl : kernel.Declares)
    {
        if (getSubDclSize(dcl))
        {
            clearSubDcl(dcl);
            dcl->setIsSplittedDcl(false);
        }
    }

    kernel.Declares.erase(std::remove_if(kernel.Declares.begin(), kernel.Declares.end(),
        [](G4_Declare* dcl) { return dcl->getIsPartialDcl(); }), kernel.Declares.end());
}

// FIXME: doBankConflictReduction and highInternalConflict are computed by local RA
//        they should be moved to some common code
bool GlobalRA::hybridRA(bool doBankConflictReduction, bool highInternalConflict, LocalRA& lra)
{
    if (builder.getOption(vISA_RATrace))
    {
        std::cout << "--hybrid RA--\n";
    }
    uint32_t numOrigDcl = (uint32_t) kernel.Declares.size();
    insertPhyRegDecls();

    LivenessAnalysis liveAnalysis(*this, G4_GRF | G4_INPUT);
    liveAnalysis.computeLiveness();

    if (liveAnalysis.getNumSelectedVar() > 0)
    {
        RPE rpe(*this, &liveAnalysis);
        rpe.run();

        bool spillLikely = kernel.getOptions()->getTarget() == VISA_3D &&
            rpe.getMaxRP() >= kernel.getOptions()->getuInt32Option(vISA_TotalGRFNum) - 16;
        if (spillLikely)
        {
            if (builder.getOption(vISA_RATrace))
            {
                std::cout << "\t--skip hybrid RA due to high pressure: " << rpe.getMaxRP() << "\n";
            }
            kernel.Declares.resize(numOrigDcl);
            lra.undoLocalRAAssignments(false);
            return false;
        }

        GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), true, false);

        unsigned spillRegSize = 0;
        unsigned indrSpillRegSize = 0;
        bool isColoringGood = coloring.regAlloc(doBankConflictReduction, highInternalConflict, false, spillRegSize, indrSpillRegSize, &rpe);
        if (isColoringGood == false)
        {
            if (!kernel.getOption(vISA_Debug))
            {
                // Why?? Keep LRA results when -debug is passed
                kernel.Declares.resize(numOrigDcl);
                lra.undoLocalRAAssignments(false);
            }
            // Restore alignment in case LRA modified it
            copyAlignment();
            return false;
        }
        coloring.confirmRegisterAssignments();

        if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
        {
            coloring.addSaveRestoreCode(0);
        }

        if (verifyAugmentation)
        {
            assignRegForAliasDcl();
            computePhyReg();
            verifyAugmentation->verify();
        }
    }


    kernel.setRAType(doBankConflictReduction ? RA_Type::HYBRID_BC_RA : RA_Type::HYBRID_RA);
    return true;
}

bool canDoLRA(G4_Kernel& kernel)
{
    bool ret = true;


    return ret;
}

//
// graph coloring entry point.  returns nonzero if RA fails
//
int GlobalRA::coloringRegAlloc()
{
    if (kernel.getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, builder.getOptions());
        optreport << std::endl << "=== Register Allocation ===" << std::endl;
        if (builder.getIsKernel() == false)
        {
            optreport << "Function: " << kernel.getName() << std::endl;
        }
        else
        {
            optreport << "Kernel: " << kernel.getName() << std::endl;
        }
        closeOptReportStream(optreport);

        detectNeverDefinedUses();
    }

    bool hasStackCall = kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();

    // this needs to be called before addr/flag RA since it changes their alignment as well
    fixAlignment();

    startTimer(TIMER_ADDR_FLAG_RA);
    addrRegAlloc();

    flagRegAlloc();
    stopTimer(TIMER_ADDR_FLAG_RA);

    //
    // If the graph has stack calls, then add the caller-save/callee-save pseudo declares and code.
    // This currently must be done after flag/addr RA due to the assumption about the location
    // of the pseudo save/restore instructions
    //
    if (hasStackCall)
    {
        addCallerSavePseudoCode();

        // Only GENX sub-graphs require callee-save code.

        if (builder.getIsKernel() == false)
        {
            addCalleeSavePseudoCode();
            addStoreRestoreForFP();
        }
    }

    if (builder.getOption(vISA_LocalRA) && !isReRAPass() && canDoLRA(kernel))
    {
        startTimer(TIMER_LOCAL_RA);
        copyMissingAlignment();
        BankConflictPass bc(*this);
        LocalRA lra(bc, *this);
        bool success = lra.localRA();
        stopTimer(TIMER_LOCAL_RA);
        if (!success)
        {
            startTimer(TIMER_HYBRID_RA);
            success = hybridRA(lra.doHybridBCR(), lra.hasHighInternalBC(), lra);
            stopTimer(TIMER_HYBRID_RA);
        }
        if (success)
        {
            // either local or hybrid RA succeeds
            assignRegForAliasDcl();
            computePhyReg();
            return CM_SUCCESS;
        }
    }

    startTimer(TIMER_GRF_GLOBAL_RA);
    unsigned maxRAIterations = 10;
    unsigned iterationNo = 0;

    std::vector<SpillManagerGMRF::EDGE> prevIntfEdges;

    int globalScratchOffset = builder.getOptions()->getuInt32Option(vISA_SpillMemOffset);
    bool useScratchMsgForSpill = globalScratchOffset < SCRATCH_MSG_LIMIT * 0.6 && !hasStackCall;
    bool enableSpillSpaceCompression = builder.getOption(vISA_SpillSpaceCompression);

    uint32_t nextSpillOffset = 0;
    uint32_t scratchOffset = 0;

    uint32_t GRFSpillFillCount = 0;
    uint32_t beforeSplitGRFSpillFillCount = 0;
    uint32_t sendAssociatedGRFSpillFillCount = 0;
    unsigned failSafeRAIteration = builder.getOption(vISA_FastSpill) ? 1 : FAIL_SAFE_RA_LIMIT;

    bool rematDone = false;
    VarSplit splitPass(*this);
    while (iterationNo < maxRAIterations)
    {
        if (builder.getOption(vISA_RATrace))
        {
            std::cout << "--GRF RA iteration " << iterationNo << "--\n";
        }

        resetGlobalRAStates();

        if (builder.getOption(vISA_clearScratchWritesBeforeEOT) &&
            (globalScratchOffset + nextSpillOffset) > 0)
        {
            // we need to set r0 be live out for this WA
            builder.getBuiltinR0()->setLiveOut();
        }

        //Identify the local variables to speedup following analysis
        markGraphBlockLocalVars();

        //Do variable splitting in each iteration
        if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA))
        {
            if (builder.getOption(vISA_RATrace))
            {
                std::cout << "\t--split local send--\n";
            }
            for (auto bb : kernel.fg)
            {
                if (bb->isSendInBB())
                {
                    splitPass.localSplit(builder, bb);
                }
            }
        }

        bool doBankConflictReduction = false;
        bool highInternalConflict = false;  // this is set by setupBankConflictsForKernel

        if (builder.getOption(vISA_LocalBankConflictReduction) &&
            builder.hasBankCollision())
        {
            bool reduceBCInRR = false;
            bool reduceBCInTAandFF = false;
            BankConflictPass bc(*this);

            reduceBCInRR = bc.setupBankConflictsForKernel(true, reduceBCInTAandFF, SECOND_HALF_BANK_START_GRF * 2, highInternalConflict);
            doBankConflictReduction = reduceBCInRR && reduceBCInTAandFF;
        }

        bool allowAddrTaken = builder.getOption(vISA_FastSpill) ||
            !kernel.getHasAddrTaken();
        bool reserveSpillReg = false;
        if (builder.getOption(vISA_FailSafeRA) &&
            kernel.getOptions()->getTarget() == VISA_3D &&
            !hasStackCall &&
            allowAddrTaken &&
            iterationNo == failSafeRAIteration)
        {
            if (builder.getOption(vISA_RATrace))
            {
                std::cout << "\t--enable failSafe RA\n";
            }
            reserveSpillReg = true;
        }

        LivenessAnalysis liveAnalysis(*this, G4_GRF | G4_INPUT);
        liveAnalysis.computeLiveness();
        if (builder.getOption(vISA_dumpLiveness))
        {
            liveAnalysis.dump();
        }

#ifdef DEBUG_VERBOSE_ON
        emitFGWithLiveness(liveAnalysis);
#endif
        //
        // if no reg var needs to reg allocated, then skip reg allocation
        //
        if (liveAnalysis.getNumSelectedVar() > 0)
        {
            // force spill should be done only for the 1st iteration
            bool forceSpill = iterationNo > 0 ? false : builder.getOption(vISA_ForceSpills);
            RPE rpe(*this, &liveAnalysis);
            rpe.run();
            GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false, forceSpill);

            if (builder.getOption(vISA_dumpRPE) && iterationNo == 0 && !rematDone)
            {
                // dump pressure the first time we enter global RA
                coloring.dumpRegisterPressure();
            }

            unsigned spillRegSize = 0;
            unsigned indrSpillRegSize = 0;
            bool isColoringGood = coloring.regAlloc(doBankConflictReduction, highInternalConflict, reserveSpillReg, spillRegSize, indrSpillRegSize, &rpe);
            if (isColoringGood == false)
            {
                if (isReRAPass())
                {
                    // Dont modify program if reRA pass spills
                    return CM_SPILL;
                }

                bool runRemat = kernel.getOptions()->getTarget() == VISA_CM ? true :
                    kernel.getSimdSize() < G4_GRF_REG_NBYTES;
                // -noremat takes precedence over -forceremat
                bool rematOff = !kernel.getOption(vISA_Debug) &&
                    (!kernel.getOption(vISA_NoRemat) || kernel.getOption(vISA_FastSpill)) &&
                    (kernel.getOption(vISA_ForceRemat) || runRemat);
                bool rematChange = false;
                bool globalSplitChange = false;

                if (!rematDone &&
                    rematOff)
                {
                    if (builder.getOption(vISA_RATrace))
                    {
                        std::cout << "\t--rematerialize\n";
                    }
                    Rematerialization remat(kernel, liveAnalysis, coloring, rpe, *this);
                    remat.run();
                    rematDone = true;

                    // Re-run GRA loop only if remat caused changes to IR
                    if (remat.getChangesMade())
                    {
                        rematChange = true;
                    }
                }

                //Calculate the spill caused by send to decide if global splitting is required or not
                for (auto spilled : coloring.getSpilledLiveRanges())
                {
                    beforeSplitGRFSpillFillCount += spilled->getRefCount();
                    auto spillDcl = spilled->getDcl();
                    if (spillDcl->getIsRefInSendDcl() && spillDcl->getNumRows() > 1)
                    {
                        sendAssociatedGRFSpillFillCount += spilled->getRefCount();
                    }
                }

                int instNum = 0;
                for (auto bb : kernel.fg)
                {
                    instNum += (int)bb->size();
                }

                if (iterationNo == 0 &&                             //Only works when first iteration of Global RA failed.
                    !splitPass.didGlobalSplit &&                      //Do only one time.
                    splitPass.canDoGlobalSplit(builder, kernel, instNum, beforeSplitGRFSpillFillCount, sendAssociatedGRFSpillFillCount))
                {
                    if (builder.getOption(vISA_RATrace))
                    {
                        std::cout << "\t--global send split\n";
                    }
                    splitPass.globalSplit(builder, kernel);
                    splitPass.didGlobalSplit = true;
                    globalSplitChange = true;
                }

                if (iterationNo == 0 &&
                    (rematChange || globalSplitChange))
                {
                    continue;
                }

                //Calculate the spill caused by send to decide if global splitting is required or not
                for (auto spilled : coloring.getSpilledLiveRanges())
                {
                    GRFSpillFillCount += spilled->getRefCount();
                }

                if (builder.getOption(vISA_OptReport) && iterationNo == 0)
                {
                    // Dump out interference graph information of spill candidates
                    reportSpillInfo(liveAnalysis, coloring);
                }

                // vISA_AbortOnSpillThreshold is defined as [0..200]
                // where 0 means abort on any spill and 200 means never abort
                auto underSpillThreshold = [this](int numSpill, int asmCount)
                {
                    int threshold = std::min(builder.getOptions()->getuInt32Option(vISA_AbortOnSpillThreshold), 200u);
                    return (numSpill * 200) < (threshold * asmCount);
                };

                if (builder.getOption(vISA_AbortOnSpill) && !underSpillThreshold(GRFSpillFillCount, instNum))
                {
                    // update jit metadata information
                    if (auto jitInfo = builder.getJitInfo())
                    {
                        jitInfo->isSpill = true;
                        jitInfo->spillMemUsed = 0;
                        jitInfo->numAsmCount = instNum;
                        jitInfo->numGRFSpillFill = GRFSpillFillCount;
                    }

                    // Early exit when -abortonspill is passed, instead of
                    // spending time inserting spill code and then aborting.
                    stopTimer(TIMER_GRF_GLOBAL_RA);
                    return CM_SPILL;
                }

                if (iterationNo == 0 &&
                    enableSpillSpaceCompression &&
                    kernel.getOptions()->getTarget() == VISA_3D &&
                    !hasStackCall)
                {
                    unsigned int spillSize = 0;
                    const LIVERANGE_LIST& spilledLRs = coloring.getSpilledLiveRanges();
                    for (auto lr : spilledLRs)
                    {
                        spillSize += lr->getDcl()->getByteSize();
                    }
                    if (spillSize * 1.5 < (SCRATCH_MSG_LIMIT - globalScratchOffset))
                    {
                        enableSpillSpaceCompression = false;
                    }
                }

                startTimer(TIMER_SPILL);
                SpillManagerGMRF spillGMRF(*this,
                    nextSpillOffset,
                    liveAnalysis.getNumSelectedVar(),
                    &liveAnalysis,
                    coloring.getLiveRanges(),
                    coloring.getIntf(), prevIntfEdges,
                    (LIVERANGE_LIST&)coloring.getSpilledLiveRanges(),
                    iterationNo++,
                    reserveSpillReg,
                    spillRegSize,
                    indrSpillRegSize,
                    enableSpillSpaceCompression,
                    useScratchMsgForSpill);

                bool success = spillGMRF.insertSpillFillCode(&kernel, pointsToAnalysis);
                nextSpillOffset = spillGMRF.getNextOffset();

                if (builder.getOption(vISA_RATrace))
                {
                    std::cout << "\t--# variables spilled: " << coloring.getSpilledLiveRanges().size() << "\n";
                    std::cout << "\t--current spill size: " << nextSpillOffset << "\n";
                }

                if (!success)
                {
                    iterationNo = maxRAIterations;
                    break;
                }

                if (builder.getOption(vISA_DumpDotAll))
                {
                    kernel.dumpDotFile("Spill_GRF");
                }

#ifdef FIX_SCRATCH_SPILL_MESSAGE
                scratchOffset = std::max(scratchOffset, spillGMRF.getNextScratchOffset());
                if (scratchOffset >= SCRATCH_MSG_LIMIT && useScratchMsgForSpill)
                {
                    spillGMRF.fixSpillFillCode(&kernel);
                }
#endif
                bool disableSpillCoalecse = builder.getOption(vISA_DisableSpillCoalescing) ||
                    builder.getOption(vISA_FastSpill) || builder.getOption(vISA_Debug);
                if (!reserveSpillReg && !disableSpillCoalecse && builder.useSends())
                {
                    CoalesceSpillFills c(kernel, liveAnalysis, coloring, spillGMRF, iterationNo, rpe, *this);
                    c.run();
                }

                if (iterationNo >= FAIL_SAFE_RA_LIMIT)
                {
                    if (coloring.getSpilledLiveRanges().size() < 2)
                    {
                        failSafeRAIteration++;
                    }
                }

                stopTimer(TIMER_SPILL);
            }

            // RA successfully allocates regs
            if (isColoringGood == true || reserveSpillReg)
            {
                coloring.confirmRegisterAssignments();

                if (hasStackCall)
                {
                    unsigned localSpillAreaOwordSize = ROUND(scratchOffset, 16) / 16;
                    coloring.addSaveRestoreCode(localSpillAreaOwordSize);
                }

                if (builder.getOption(vISA_OptReport))
                {
                    detectUndefinedUses(liveAnalysis, kernel);
                }

                if (nextSpillOffset)
                {
                    switch (kernel.getRAType())
                    {
                    case RA_Type::GRAPH_COLORING_RR_BC_RA:
                        kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_RR_BC_RA);
                        break;
                    case RA_Type::GRAPH_COLORING_FF_BC_RA:
                        kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_FF_BC_RA);
                        break;
                    case RA_Type::GRAPH_COLORING_RR_RA:
                        kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_RR_RA);
                        break;
                    case RA_Type::GRAPH_COLORING_FF_RA:
                        kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_FF_RA);
                        break;
                    default:
                        assert(0);
                        break;
                    }
                }

                if (verifyAugmentation)
                {
                    assignRegForAliasDcl();
                    computePhyReg();
                    verifyAugmentation->verify();
                }


                break; // done
            }
        }
        else
        {
            break;
        }
    }

    assignRegForAliasDcl();
    computePhyReg();

    stopTimer(TIMER_GRF_GLOBAL_RA);

    //
    // Report failure to allocate due to excessive register pressure.
    //
    if (iterationNo == maxRAIterations)
    {
        std::stringstream spilledVars;
        for (auto dcl : kernel.Declares)
        {
            if (dcl->isSpilled() && dcl->getRegFile() == G4_GRF)
            {
                spilledVars << dcl->getName() << "\t";
            }
        }

        MUST_BE_TRUE(false,
            "ERROR: " << kernel.getNumRegTotal() - builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum)
            << " GRF registers are NOT enough to compile kernel " << kernel.getName() << "!"
            << " The maximum register pressure in the kernel is higher"
            << " than the available physical registers in hardware (even"
            << " with spill code)."
            << " Please consider rewriting the kernel."
            << " Compiling with the symbolic register option and inspecting the"
            << " spilled registers may help in determining the region of high pressure.\n"
            << "The spilling virtual registers are as follows: "
            << spilledVars.str());

        return CM_SPILL;
    }

    // do not double count the spill mem offset
    // Note that this includes both SLM and scratch space spills
    uint32_t spillMemUsed = nextSpillOffset;
    //
    // Report spill memory usage information.
    //
    if (spillMemUsed)
    {
        RELEASE_MSG("Spill memory used = " << spillMemUsed << " bytes for kernel " <<
            kernel.getName() << std::endl << " Compiling kernel with spill code may degrade performance." <<
            " Please consider rewriting the kernel to use less registers." << std::endl);
    }

    // update jit metadata information for spill
    if (auto jitInfo = builder.getJitInfo())
    {
        uint32_t scratchSize = kernel.fg.getHasStackCalls() ? 8 * 1024 : 0;
        spillMemUsed = std::max(spillMemUsed, scratchSize);
        jitInfo->isSpill = spillMemUsed > 0;
        jitInfo->spillMemUsed = spillMemUsed;
        jitInfo->numGRFSpillFill = GRFSpillFillCount;
        jitInfo->numBytesScratchGtpin = kernel.getGTPinData()->getNumBytesScratchUse();
    }

    kernel.getGTPinData()->setScratchNextFree(spillMemUsed);

    if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA))
    {
        removeSplitDecl();
    }

    return CM_SUCCESS;
}

/********************************************************************************************************************************************/
/********************************************************Spill Code Clean up ****************************************************************/
/********************************************************************************************************************************************/

#define SPILL_MEMORY_OVERLAP(x, y) \
    (!(x->leftOff > y->rightOff || y->leftOff > x->rightOff))

#define SPILL_MEMORY_OVERWRITE(target_memory, overwrite_memory) \
    (target_memory->leftOff >= overwrite_memory->leftOff && overwrite_memory->rightOff >= target_memory->rightOff)

#define IS_FLAG_MOVE(inst)  (\
    inst->opcode() == G4_mov &&  \
    (inst->getDst() && inst->getSrc(0)) && \
    (inst->getDst()->getTopDcl() && inst->getSrc(0)->getTopDcl()) && \
    (inst->getDst()->getTopDcl()->getRegFile() == G4_FLAG && inst->getSrc(0)->getTopDcl()->getRegFile() == G4_GRF || \
    inst->getDst()->getTopDcl()->getRegFile() == G4_GRF && inst->getSrc(0)->getTopDcl()->getRegFile() == G4_FLAG) \
    )

#define IS_SPILL_KILL_CANDIDATE(preScratchAccess) \
    (preScratchAccess->isSpill && !preScratchAccess->fillInUse) \

#define IS_USE_KILL_CANDIDATE(preScratchAccess) \
    (!(preScratchAccess->regKilled || preScratchAccess->regPartialKilled || preScratchAccess->scratchDefined)) \

#define IS_GRF_RANGE_OVERLAP(s1, e1, sa) \
    (e1 >= sa->linearizedStart && sa->linearizedEnd >= s1)

#define IS_SCRATCH_RANGE_OVERLAP(s1, e1, sa) \
    (!(e1 < sa->leftOff || sa->rightOff < s1))

#define IS_MERGEABLE_SCRATCH_RANGES(r1, r2) \
    (!(((int)r1.leftOff - (int)r2.rightOff)> 1 || ((int)r2.leftOff - (int)r1.rightOff) > 1))

#define IS_MERGEABLE_GRF_RANGES(r1, r2) \
    (!(((int)r1.linearizedStart - (int)r2.linearizedEnd) > 1 || ((int)r2.linearizedStart - (int)r1.linearizedEnd) > 1))

#define IS_GRF_RANGE_OVERWRITE(sa, s1, e1) \
    (s1 <= sa->linearizedStart && sa->linearizedEnd <= e1)

#define IS_SCRATCH_RANGE_OVERWRITE(sa, s1, e1) \
    (s1 <= sa->leftOff && sa->rightOff <= e1)

#define IS_FLAG_RANGE_OVERLAP(s1, e1, sa) \
    (!(e1 < sa->linearizedStart || sa->linearizedEnd < s1))

#define IS_FLAG_RANGE_OVERWRITE(t, s, e) \
    ((s <= t->linearizedStart && t->linearizedEnd <= e))

void  FlagSpillCleanup::FlagLineraizedStartAndEnd(G4_Declare*  topdcl,
    unsigned int& linearizedStart,
    unsigned int& linearizedEnd)
{
    G4_Areg* areg = topdcl->getRegVar()->getPhyReg()->asAreg();
    linearizedStart = areg->getFlagNum() * 4;
    linearizedStart += topdcl->getRegVar()->getPhyRegOff() * topdcl->getElemSize();
    linearizedEnd = linearizedStart + topdcl->getByteSize();
    return;
}

/*
 * Reuse previous register
 */
bool FlagSpillCleanup::replaceWithPreDcl(IR_Builder&     builder,
    SCRATCH_ACCESS* scratchAccess,
    SCRATCH_ACCESS* preScratchAccess)
{
    int preRegOff = 0;
    int payloadHeaderSize = 0;
    G4_Operand *reuseOpnd = NULL;
    G4_INST *preInst = *preScratchAccess->inst_it;

    //Get reuse operand
    if (preScratchAccess->isSpill)
    {
        reuseOpnd = preInst->getSrc(0);
        preRegOff = reuseOpnd->asSrcRegRegion()->getSubRegOff();
        reuseOpnd = preInst->getSrc(0);
    }
    else
    {
        reuseOpnd = preInst->getDst();
        preRegOff = reuseOpnd->asDstRegRegion()->getSubRegOff();//For flag register, only subRegOff
    }
    G4_Declare *dcl = reuseOpnd->getBase()->asRegVar()->getDeclare();

    if (VISA_WA_CHECK(builder.getPWaTable(), WaDisableSendSrcDstOverlap))
    {
        for (auto &renameOpnd : scratchAccess->renameOperandVec)
        {
            if (renameOpnd.second < -1) //Flag
            {
                break;
            }

            G4_INST *inst = renameOpnd.first;

            if (renameOpnd.second >= 0)
            {
                if (inst->isSend() && !inst->getDst()->isNullReg())
                {
                    G4_DstRegRegion* dst = inst->getDst();
                    bool noOverlap = dst->getLinearizedEnd() < preScratchAccess->linearizedStart ||
                        preScratchAccess->linearizedEnd < dst->getLinearizedStart();
                    if (!noOverlap)
                    {
                        return false;
                    }
                }
            }
        }
    }

    //Replace the declare for all operands assciated with this scratch fill.
    for (auto &renameOpnd : scratchAccess->renameOperandVec)
    {
        G4_INST *inst = renameOpnd.first;

        if (renameOpnd.second == -3) //Flag modifier
        {
            G4_CondMod* mod = inst->getCondMod();
            int regOff = preRegOff;
            G4_CondMod* mod_Opnd = builder.createCondMod(mod->getMod(),
                dcl->getRegVar(),
                (unsigned short)regOff);

            inst->setCondMod(mod_Opnd);

        }
        else if (renameOpnd.second == -2) //Flag predicate
        {
            G4_Predicate* predicate = inst->getPredicate();
            int regOff = preRegOff;
            G4_Predicate * pred_Opnd = builder.createPredicate(predicate->getState(),
                dcl->getRegVar(),
                (unsigned short)regOff,
                predicate->getControl());

            inst->setPredicate(pred_Opnd);
        }
        else if (renameOpnd.second == -1)  //GRF dst
        {
            G4_DstRegRegion *orgDstRegion = inst->getDst();
            int regOff = preRegOff + (scratchAccess->leftOff - preScratchAccess->leftOff) / G4_GRF_REG_NBYTES + payloadHeaderSize / G4_GRF_REG_NBYTES;
            G4_DstRegRegion * dstOpnd = builder.createDstRegRegion(orgDstRegion->getRegAccess(),
                dcl->getRegVar(),
                (short)regOff,
                orgDstRegion->getSubRegOff(),
                orgDstRegion->getHorzStride(), orgDstRegion->getType());
            inst->setDest(dstOpnd);
        }
        else //GRF src
        {
            G4_Operand *opnd = inst->getSrc(renameOpnd.second);
            G4_SrcRegRegion *orgSrcRegion = opnd->asSrcRegRegion();

            int regOff = preRegOff + (scratchAccess->leftOff - preScratchAccess->leftOff) / G4_GRF_REG_NBYTES + payloadHeaderSize / G4_GRF_REG_NBYTES;
            G4_Operand * srcOpnd = builder.createSrcRegRegion(orgSrcRegion->getModifier(),
                orgSrcRegion->getRegAccess(),
                dcl->getRegVar(),
                (short)regOff,
                orgSrcRegion->getSubRegOff(),
                orgSrcRegion->getRegion(),
                orgSrcRegion->getType());

            inst->setSrc(srcOpnd, renameOpnd.second);
        }
    }

    return true;
}

/*
 *  1) The reuse target register in pre scratch access may be partial killed,
 *  2) and the corresponding scracth memory range is overlap with the memory of current scratch access.
 *  In both cases, the current fill can not be removed
 */
bool FlagSpillCleanup::scratchKilledByPartial(SCRATCH_ACCESS* scratchAccess,
    SCRATCH_ACCESS* preScratchAccess)
{
    bool killed = false;

    for (auto &range : preScratchAccess->killedScratchRange)
    {
        if (!(scratchAccess->leftOff > range.rightOff ||
            range.leftOff > scratchAccess->rightOff))
        {
            killed = true;
        }
    }

    for (auto &range : preScratchAccess->killedRegRange)
    {
        //Map the register kill to scratch kill
        unsigned int leftOff = preScratchAccess->leftOff + (range.linearizedStart - preScratchAccess->linearizedStart);
        unsigned int rightOff = preScratchAccess->leftOff + (range.linearizedEnd - preScratchAccess->linearizedStart);

        if (!(scratchAccess->leftOff > rightOff ||
            leftOff > scratchAccess->rightOff))
        {
            killed = true;
        }
    }

    return killed;
}

/*
 *  Record all killed GRF ranges.
 *  do merging of ranges when possible.
 */
bool FlagSpillCleanup::addKilledGRFRanges(unsigned int    linearizedStart,
    unsigned int    linearizedEnd,
    SCRATCH_ACCESS* scratchAccess,
    G4_Predicate*   predicate)
{
    REG_RANGE range;
    range.linearizedStart = MAX(scratchAccess->linearizedStart, linearizedStart);
    range.linearizedEnd = MIN(scratchAccess->linearizedEnd, linearizedEnd);
    range.predicate = predicate ? true : false;

    if (scratchAccess->killedRegRange.size() == 0)
    {
        scratchAccess->killedRegRange.push_back(range);
    }
    else
    {
        bool merged = false;
        REG_RANGE_VEC_ITER range_iter = scratchAccess->killedRegRange.begin();
        REG_RANGE_VEC_ITER range_iter_next;
        REG_RANGE *merged_range = NULL;
        while (range_iter != scratchAccess->killedRegRange.end())
        {
            REG_RANGE &killedRange = *(range_iter);
            range_iter_next = range_iter;
            range_iter_next++;

            if (killedRange.predicate) //With predicate, the range can not be merged with others
            {
                range_iter = range_iter_next;
                continue;
            }

            if (!merged && IS_MERGEABLE_GRF_RANGES(range, killedRange))
            {
                killedRange.linearizedStart = MIN(killedRange.linearizedStart, range.linearizedStart);
                killedRange.linearizedEnd = MAX(killedRange.linearizedEnd, range.linearizedEnd);
                merged = true;
                merged_range = &killedRange;
            }
            else if (merged)
            {
                if (IS_MERGEABLE_GRF_RANGES((*merged_range), killedRange))
                {
                    merged_range->linearizedStart = MIN(killedRange.linearizedStart, merged_range->linearizedStart);
                    merged_range->linearizedEnd = MAX(killedRange.linearizedEnd, merged_range->linearizedEnd);
                }
            }
            if (IS_GRF_RANGE_OVERWRITE(scratchAccess, killedRange.linearizedStart, killedRange.linearizedEnd))
            {
                scratchAccess->regKilled = true;
                return true;
            }
            range_iter = range_iter_next;
        }
        if (!merged)
        {
            scratchAccess->killedRegRange.push_back(range);
        }
    }

    return false;
}

/*
 * Check if the register in previous scratch access is fully killed by current register define
 */
bool FlagSpillCleanup::regFullyKilled(SCRATCH_ACCESS* scratchAccess,
    unsigned int        linearizedStart,
    unsigned int        linearizedEnd,
    unsigned short      maskFlag)
{

    if (IS_FLAG_RANGE_OVERWRITE(scratchAccess, linearizedStart, linearizedEnd))
    {
        if (maskFlag & InstOpt_WriteEnable)  // No mask == all range killed
        {
            return true;
        }

        if (linearizedStart == scratchAccess->linearizedStart &&
            linearizedEnd == scratchAccess->linearizedEnd &&
            scratchAccess->maskFlag == maskFlag)
        {
            return true;
        }
    }

    return false;
}

/*
 *  Check only part of scratch register is killed, at the same time no overlap.
 *  This is to make sure if the associated fill is removed, the define register can be replaced with reuse register or not.
 */
bool FlagSpillCleanup::inRangePartialKilled(SCRATCH_ACCESS* scratchAccess,
    unsigned int    linearizedStart,
    unsigned int    linearizedEnd,
    unsigned short  maskFlag)
{
    if ((scratchAccess->linearizedStart <= linearizedStart &&
        scratchAccess->linearizedEnd >= linearizedEnd))
    {
        if (maskFlag & InstOpt_WriteEnable)
        {
            return true;
        }

        if (scratchAccess->linearizedStart == linearizedStart &&
            scratchAccess->linearizedEnd == linearizedEnd &&
            scratchAccess->maskFlag == maskFlag)
        {
            return true;
        }
    }

    return false;
}

/*
 * Register kill analysis
 */
bool FlagSpillCleanup::regDefineAnalysis(SCRATCH_ACCESS* scratchAccess,
    unsigned int       linearizedStart,
    unsigned int       linearizedEnd,
    unsigned short     maskFlag,
    G4_Predicate*      predicate)
{
    if (regFullyKilled(scratchAccess, linearizedStart, linearizedEnd, maskFlag))
    {
        return true;
    }
    else if (!scratchAccess->regKilled)
    {
        // Handle partial overlap
        // What about the mask?
        if (addKilledGRFRanges(linearizedStart, linearizedEnd, scratchAccess, predicate))
        {
            //The register range is killed by accumulated partial range kills
            return true;
        }
        scratchAccess->regPartialKilled = true;
    }

    return false;
}

void FlagSpillCleanup::regDefineFlag(SCRATCH_PTR_LIST* scratchTraceList,
    G4_INST*          inst,
    G4_Operand*       opnd)
{
    //Get the linearized address in GRF register file
    unsigned int linearizedStart = 0;
    unsigned int linearizedEnd = 0;
    G4_Predicate* predicate = inst->getPredicate();
    G4_Declare*  topdcl = opnd->getTopDcl();

    FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

    //Impact on previous scratch access
    SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

    if (it != itEnd &&
        inst == *(scratchTraceList->back()->inst_it))
    {
        itEnd--;
    }

    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;

        SCRATCH_ACCESS * scratchAccess = *it;

        //Not instruction itself, def->use can not happen in single instruction.
        if (scratchAccess->regKilled)
        {
            it = kt;
            continue;
        }

        // Checked if the registers used in the previous scratch accesses (both spill and fill) are killed (redefined).
        if (linearizedEnd &&
            IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess))
        {
            //E mask
            unsigned maskFlag = (inst->getOption() & 0xFFF010C);

            if (regDefineAnalysis(scratchAccess, linearizedStart, linearizedEnd, (unsigned short)maskFlag, predicate))
            {
                //Fully killed
                scratchAccess->regKilled = true;
                if (scratchAccess->evicted)  //Not in use
                {
                    scratchTraceList->erase(it); //The previous one is not candidate for future use
                }
            }

            // For prefill and associated define and spill instructions
            // 1. Same dcl is used
            // 2. If the prefill register is fulled killed,
            //     a. The prefill instruction can be removed.
            //     b. But the define and instruction's registers are kept and will not reuse previous one.
            // 3. If the prefill register is partial killed, and the killed register region is part of prefill region.
            //     a. The prefill instruction can be removed.
            //     b. and the register in define and spill instruction can reuse previous one.
            // 4. Otherwise, the (pre)fill instruction can not be removed, and no reuse will happen.
            // 5. For pure fill, it's no killed by same declare
            G4_Declare *preDcl = NULL;
            preDcl = scratchAccess->flagOpnd->getTopDcl();

            if (topdcl == preDcl)
            {
                if (inRangePartialKilled(scratchAccess, linearizedStart, linearizedEnd, (unsigned short)maskFlag))
                {
                    scratchAccess->renameOperandVec.push_back(make_pair(inst, -1));
                    scratchAccess->inRangePartialKilled = true;
                }
                else
                {
                    scratchAccess->removeable = false;
                }
            }
        }

        it = kt;
    }
}

/*
 *  Analysis the use of register to determine if the scratchAccess can be removed or not
 *
 */
bool FlagSpillCleanup::regUseAnalysis(SCRATCH_ACCESS* scratchAccess,
    unsigned int    linearizedStart,
    unsigned int    linearizedEnd)
{
    //GRF in previous fill is used as part of current reg,
    //In this case, the fill can not be removed since the reuse can not happen.
    //Caller gauranteed the overlap of the registers
    if (linearizedEnd > scratchAccess->linearizedEnd ||
        linearizedStart < scratchAccess->linearizedStart)
    {
        return true;
    }

    //Can not be removed when the previous scratch access is killed or partial killed
    //before the use of current scratch access register
    //b
    SCRATCH_ACCESS * preScratchAccess = scratchAccess->preScratchAccess;
    if (preScratchAccess &&
        (preScratchAccess->regKilled ||
         scratchKilledByPartial(scratchAccess, preScratchAccess)))
    {
        return true;
    }

    //Back trace to update the reachable scratch accesses
    if (scratchAccess->prePreScratchAccess)
    {
        SCRATCH_ACCESS * prePreScratchAccess = preScratchAccess;
        preScratchAccess = scratchAccess;

        do {
            if ((prePreScratchAccess->regKilled ||
                scratchKilledByPartial(scratchAccess, prePreScratchAccess)))
            {
                scratchAccess->prePreScratchAccess = preScratchAccess;
                break;
            }
            preScratchAccess = prePreScratchAccess;
            prePreScratchAccess = preScratchAccess->preScratchAccess;
        } while (prePreScratchAccess && preScratchAccess != scratchAccess->prePreScratchAccess);
    }

    return false;
}

void FlagSpillCleanup::regUseFlag(SCRATCH_PTR_LIST*  scratchTraceList,
    G4_INST*           inst,
    G4_Operand*        opnd,
    int                opndIndex)
{
    //Get the linearized address in GRF register file
    unsigned int linearizedStart = 0;
    unsigned int linearizedEnd = 0;
    G4_Declare *topdcl = NULL;

    topdcl = opnd->getTopDcl();
    FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

    //Impact on previous scratch access
    SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();
    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;
        SCRATCH_ACCESS * scratchAccess = *it;

        if (linearizedEnd &&
            IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess))
        {
            //Not handle indirect GRF
            if (inst->isEOT() ||
                inst->isPseudoUse())
            {
                scratchAccess->removeable = false;
                it = kt;
                continue;
            }

            if (scratchAccess->flagOpnd->getTopDcl() == topdcl)  //Same declare
            {
                if (regUseAnalysis(scratchAccess, linearizedStart, linearizedEnd))
                {
                    //The filled register is in use
                    scratchAccess->removeable = false;
                }
                else if (scratchAccess->inRangePartialKilled || !scratchAccess->regKilled)
                {
                    //can reuse previous register
                    scratchAccess->renameOperandVec.push_back(make_pair(inst, opndIndex));
                }
            }
        }
        it = kt;
    }
}

void FlagSpillCleanup::regUseScratch(SCRATCH_PTR_LIST*  scratchTraceList,
    G4_INST*           inst,
    G4_Operand*        opnd,
    Gen4_Operand_Number opndNum)
{
    G4_Declare *topdcl = NULL;
    topdcl = opnd->getTopDcl();

    //Impact on previous scratch access
    SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();
    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;
        SCRATCH_ACCESS * scratchAccess = *it;
        if (topdcl == scratchAccess->scratchDcl)
        {
            if (opndNum == Opnd_dst)
            {
                scratchAccess->scratchDefined = true;
            }
            else
            {
                scratchAccess->removeable = false;
            }
        }

        it = kt;
    }
}

void FlagSpillCleanup::initializeScratchAccess(SCRATCH_ACCESS *scratchAccess, INST_LIST_ITER inst_it)
{
#ifdef _DEBUG
    scratchAccess->regNum = -1;
#endif
    scratchAccess->scratchDcl = NULL;
    scratchAccess->flagOpnd = NULL;

    scratchAccess->linearizedStart = 0;
    scratchAccess->linearizedEnd = 0;
    scratchAccess->leftOff = 0;
    scratchAccess->rightOff = 0;
    scratchAccess->useCount = 0;

    scratchAccess->isSpill = false;
    scratchAccess->isBlockLocal = false;
    scratchAccess->directKill = false;

    scratchAccess->regKilled = false;
    scratchAccess->regPartialKilled = false;
    scratchAccess->regOverKilled = false;
    scratchAccess->inRangePartialKilled = false;
    scratchAccess->regInUse = false;

    scratchAccess->fillInUse = false;
    scratchAccess->removeable = true;
    scratchAccess->instKilled = false;
    scratchAccess->evicted = false;
    scratchAccess->scratchDefined = false;

    scratchAccess->preScratchAccess = NULL;
    scratchAccess->prePreScratchAccess = NULL;
    scratchAccess->preFillAccess = NULL;

    scratchAccess->inst_it = inst_it;
    G4_INST *inst = *inst_it;
    scratchAccess->maskFlag = (inst->getOption() & 0xFFF010C);

    return;
}

bool FlagSpillCleanup::initializeFlagScratchAccess(SCRATCH_PTR_VEC* scratchAccessList,
    SCRATCH_ACCESS*   &scratchAccess,
    INST_LIST_ITER    inst_it)
{
    G4_INST* inst = (*inst_it);

    G4_DstRegRegion* dst = inst->getDst();
    G4_Operand* src = inst->getSrc(0);
    G4_Declare* topDcl_1 = dst->getTopDcl();
    G4_Declare* topDcl_2 = src->getTopDcl();

    //Create the spill/fill description
    if (topDcl_1->getRegFile() == G4_FLAG && topDcl_2->getRegFile() == G4_GRF)
    {
        if (src->asSrcRegRegion()->getBase()->isRegVar() &&
            src->asSrcRegRegion()->getBase()->asRegVar()->isRegVarAddrSpillLoc())
        {
            scratchAccess = new SCRATCH_ACCESS;
            scratchAccessList->push_back(scratchAccess);
            initializeScratchAccess(scratchAccess, inst_it);
            //Fill
#ifdef _DEBUG
            scratchAccess->regNum = topDcl_1->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
            scratchAccess->scratchDcl = topDcl_2;  //Spill location

            if (gra.isBlockLocal(topDcl_2))
            {
                scratchAccess->isBlockLocal = true;
            }
            FlagLineraizedStartAndEnd(topDcl_1, scratchAccess->linearizedStart, scratchAccess->linearizedEnd);
            scratchAccess->flagOpnd = dst;
            return true;
        }
    }
    else
    {   //Spill
        if (dst->getBase()->isRegVar() &&
            dst->getBase()->asRegVar()->isRegVarAddrSpillLoc())
        {
            scratchAccess = new SCRATCH_ACCESS;
            scratchAccessList->push_back(scratchAccess);
            initializeScratchAccess(scratchAccess, inst_it);
#ifdef _DEBUG
            scratchAccess->regNum = topDcl_2->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
            scratchAccess->scratchDcl = topDcl_1;

            if (gra.isBlockLocal(topDcl_1))
            {
                scratchAccess->isBlockLocal = true;
            }

            scratchAccess->isSpill = true;
            FlagLineraizedStartAndEnd(topDcl_2, scratchAccess->linearizedStart, scratchAccess->linearizedEnd);
            scratchAccess->flagOpnd = src;
            return true;
        }
    }

    return false;
}

void FlagSpillCleanup::freeScratchAccess(SCRATCH_PTR_VEC *scratchAccessList)
{
    SCRATCH_PTR_VEC::iterator it = scratchAccessList->begin();
    SCRATCH_PTR_VEC::iterator itEnd = scratchAccessList->end();
    while (it != itEnd)
    {
        SCRATCH_PTR_VEC::iterator kt = it;
        kt++;
        SCRATCH_ACCESS * scratchAccess = *it;

        delete scratchAccess;
        it = kt;
    }

    scratchAccessList->clear();

    return;
}

//Check the flag define instruction.
void FlagSpillCleanup::flagDefine(SCRATCH_PTR_LIST& scratchTraceList,
    G4_INST*          inst)
{
    G4_DstRegRegion* dst = inst->getDst();

    if (dst)
    {
        G4_Declare* topdcl = NULL;
        topdcl = GetTopDclFromRegRegion(dst);

        if (topdcl && topdcl->getRegFile() == G4_FLAG)
        {
            //Flag register define
            regDefineFlag(&scratchTraceList, inst, dst);
        }
    }

    G4_CondMod* mod = inst->getCondMod();
    if (!mod)
    {
        return;
    }

    // ConMod, handled as register define
    unsigned maskFlag = (inst->getOption() & 0xFFF010C);

    unsigned int linearizedStart = 0;
    unsigned int linearizedEnd = 0;

    G4_VarBase *flagReg = mod->getBase();
    if (!flagReg)
    {
        return;
    }

    G4_Declare* topdcl = flagReg->asRegVar()->getDeclare();
    FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

    SCRATCH_PTR_LIST_ITER it = scratchTraceList.begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList.end();
    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;

        SCRATCH_ACCESS *preScratchAccess = *it;
        if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, preScratchAccess))
        {
            G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();

            if (regDefineAnalysis(preScratchAccess, linearizedStart, linearizedEnd, (unsigned short)maskFlag, NULL))
            {
                preScratchAccess->regKilled = true;
                if (preScratchAccess->evicted)  //Not in use
                {
                    scratchTraceList.erase(it); //The previous one is not candidate for reuse
                }
            }
            if (topdcl == preDcl)
            {
                if (preScratchAccess->inRangePartialKilled)
                {
                    preScratchAccess->renameOperandVec.push_back(make_pair(inst, -3));
                }
                else
                {
                    preScratchAccess->removeable = false;
                }
            }
        }
        it = kt;
    }

    return;
}

void FlagSpillCleanup::scratchUse(SCRATCH_PTR_LIST& scratchTraceList, G4_INST* inst)
{
    G4_DstRegRegion* dst = inst->getDst();

    if (dst)
    {
        G4_Declare* topdcl = NULL;
        topdcl = GetTopDclFromRegRegion(dst);

        if (topdcl && topdcl->getRegFile() == G4_GRF)
        {
            //Flag scratch variable is redefined
            regUseScratch(&scratchTraceList, inst, dst, Opnd_dst);
        }
    }

    for (unsigned i = 0; i < G4_MAX_SRCS; i++)
    {
        G4_Operand* src = inst->getSrc(i);

        if (src && src->isSrcRegRegion())
        {
            G4_Declare* topdcl = NULL;

            if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar())
            {
                topdcl = GetTopDclFromRegRegion(src);
            }

            if (!topdcl || (topdcl->getRegFile() == G4_FLAG))
            {
                continue;
            }

            regUseScratch(&scratchTraceList, inst, src, Opnd_src0);
        }
    }
}

void FlagSpillCleanup::flagUse(SCRATCH_PTR_LIST& scratchTraceList, G4_INST* inst)
{
    for (unsigned i = 0; i < G4_MAX_SRCS; i++)
    {
        G4_Operand* src = inst->getSrc(i);

        if (src && src->isSrcRegRegion())
        {
            G4_Declare* topdcl = NULL;

            if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar())
            {
                topdcl = GetTopDclFromRegRegion(src);
            }

            if (!topdcl || (topdcl->getRegFile() != G4_FLAG))
            {
                continue;
            }

            regUseFlag(&scratchTraceList, inst, src, i);
        }
    }

    //Flag register is used as predicate
    G4_Predicate* predicate = inst->getPredicate();
    if (!predicate)
    {
        return;
    }

    G4_VarBase *flagReg = predicate->getBase();
    if (!flagReg)
    {
        return;
    }

    G4_Declare* topdcl = flagReg->asRegVar()->getDeclare();
    unsigned int linearizedStart = 0;
    unsigned int linearizedEnd = 0;
    FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

    SCRATCH_PTR_LIST_ITER it = scratchTraceList.begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList.end();
    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;

        SCRATCH_ACCESS *preScratchAccess = *it;
        if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, preScratchAccess))
        {
            G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();
            //Use should have same top declare
            if (preDcl == topdcl)
            {
                if (regUseAnalysis(preScratchAccess, linearizedStart, linearizedEnd))
                {
                    preScratchAccess->removeable = false;
                }
                else if (preScratchAccess->inRangePartialKilled || !preScratchAccess->regKilled)
                {
                    //can reuse previous register
                    preScratchAccess->renameOperandVec.push_back(make_pair(inst, -2));
                }
            }
        }
        it = kt;
    }

    return;
}

bool FlagSpillCleanup::flagScratchDefineUse(G4_BB* bb,
    SCRATCH_PTR_LIST*  scratchTraceList,
    SCRATCH_PTR_VEC*  candidateList,
    SCRATCH_ACCESS*    scratchAccess,
    CLEAN_NUM_PROFILE* clean_num_profile)
{
    SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
    SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

    while (it != itEnd)
    {
        SCRATCH_PTR_LIST_ITER kt = it;
        kt++;

        SCRATCH_ACCESS * preScratchAccess = *it;

        //Evicted
        if (preScratchAccess->evicted)
        {
            it = kt;
            continue;
        }

        //Same scratch declare
        if (preScratchAccess->scratchDcl == scratchAccess->scratchDcl) //Same scratch location
        {
            if (scratchAccess->isSpill)  //Current is spill
            {
                if (IS_SPILL_KILL_CANDIDATE(preScratchAccess))  //previoius is spill as well and previous spill is not used
                {
                    //kill the previous spill
                    bb->erase(preScratchAccess->inst_it);
                    preScratchAccess->instKilled = true;
                    clean_num_profile->spill_clean_num[0]++;
                    scratchTraceList->erase(it); //The previous one is not candidate for reuse
                    it = kt;

                    continue;
                }

                preScratchAccess->evicted = true;
                scratchTraceList->erase(it); //The previous one is not a good candidate for reuse any more
            }
            else  //Current is fill
            {
                preScratchAccess->fillInUse = true;
                preScratchAccess->useCount++;

                if (IS_USE_KILL_CANDIDATE(preScratchAccess))   //Is not used before
                {
                    scratchAccess->preScratchAccess = preScratchAccess;   //set previous scrach location define
                    candidateList->push_back(scratchAccess);  //Add to candidate list
                    if (IS_FLAG_RANGE_OVERWRITE(scratchAccess, preScratchAccess->linearizedStart, preScratchAccess->linearizedEnd))
                    {
                        //Exactly same GRF, it's useless fill, since prevous fill or spill not been killed
                        scratchAccess->directKill = true;
                        scratchTraceList->push_back(scratchAccess);
                        return true;
                    }
                }
            }
        }
        it = kt;
    }

    scratchTraceList->push_back(scratchAccess);

    return false;
}

void FlagSpillCleanup::flagSpillFillClean(G4_BB* bb,
    INST_LIST_ITER     inst_it,
    SCRATCH_PTR_VEC&  scratchAccessList,
    SCRATCH_PTR_LIST&  scratchTraceList,
    SCRATCH_PTR_VEC&  candidateList,
    CLEAN_NUM_PROFILE* clean_num_profile)
{
    G4_INST* inst = (*inst_it);
    if (inst->isPseudoKill())
    {
        return;
    }

    bool noDefineAnalysis = false;

    //Check if there is flag use
    flagUse(scratchTraceList, inst);

    //Check if it's spill/fill of the flag
    if (IS_FLAG_MOVE(inst))
    {
        SCRATCH_ACCESS *scratchAccess = NULL;

        if (initializeFlagScratchAccess(&scratchAccessList, scratchAccess, inst_it))
        {
            //Build the trace list and the candidate list
            //Trace list includes all spill/fill
            //Candidate includues ??
            //Checking if the spill/fill can be removed at the same time by comparing previous one.
            noDefineAnalysis = flagScratchDefineUse(bb, &scratchTraceList, &candidateList, scratchAccess, clean_num_profile);
        }
    }
    else
    {
        scratchUse(scratchTraceList, inst);
    }

    //Check if there is flag define
    if (!noDefineAnalysis)
    {
        flagDefine(scratchTraceList, inst);
    }

    return;
}

#ifdef _DEBUG
#define FILL_DEBUG_THRESHOLD 0xffffffff
#define SPILL_DEBUG_THRESHOLD 0xffffffff //25
#endif

void FlagSpillCleanup::regFillClean(IR_Builder&        builder,
    G4_BB*             bb,
    SCRATCH_PTR_VEC&  candidateList,
    CLEAN_NUM_PROFILE* clean_num_profile)
{
    if (candidateList.size())
    {
        SCRATCH_PTR_VEC::iterator it = candidateList.begin();
        SCRATCH_PTR_VEC::iterator itEnd = candidateList.end();
        while (it != itEnd)
        {
            SCRATCH_PTR_VEC::iterator kt = it;
            kt++;
            SCRATCH_ACCESS* scratchAccess = *it;
            SCRATCH_ACCESS* preScratchAccess = scratchAccess->preScratchAccess;

            // Since the reuse happens from front to end.
            // If the pre scratchAccess is killed, current candidate can not reuse previous register any more
            if (!scratchAccess->instKilled &&
                (scratchAccess->removeable || scratchAccess->directKill))
            {
                if (scratchAccess->prePreScratchAccess)
                {
                    while (preScratchAccess &&
                        preScratchAccess->preScratchAccess &&
                        preScratchAccess != scratchAccess->prePreScratchAccess)
                    {
                        //If possible, propagate to previous scratchAccess
                        if (preScratchAccess->preFillAccess)
                        {
                            //to jump over prefill.
                            if (preScratchAccess->isSpill &&
                                preScratchAccess->preFillAccess &&
                                preScratchAccess->preFillAccess->instKilled &&
                                preScratchAccess->preScratchAccess)
                            {
                                preScratchAccess = preScratchAccess->preScratchAccess;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            if (!preScratchAccess->instKilled)
                            {
                                break;
                            }
                            preScratchAccess = preScratchAccess->preScratchAccess;
                        }
                    }

                    if (preScratchAccess)
                    {
                        if (preScratchAccess->isSpill &&
                            preScratchAccess->preFillAccess &&
                            preScratchAccess->preFillAccess->instKilled)
                        {
                        }
                        else if (!preScratchAccess->instKilled)
                        {
                            if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess))
                            {
                                bb->erase(scratchAccess->inst_it);
                                scratchAccess->instKilled = true;
                                scratchAccess->preScratchAccess->useCount--;
                                clean_num_profile->fill_clean_num[0]++;
                            }
                        }
                    }
                }
                else
                {
                    if (preScratchAccess && !preScratchAccess->instKilled)
                    {
                        if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess))
                        {
                            bb->erase(scratchAccess->inst_it);
                            scratchAccess->instKilled = true;
                            scratchAccess->preScratchAccess->useCount--;
                            clean_num_profile->fill_clean_num[0]++;
                        }
                    }
                }
            }
#ifdef _DEBUG
            if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
                return;
#endif

            it = kt;
        }
    }

    return;
}

void FlagSpillCleanup::regSpillClean(IR_Builder&        builder,
    G4_BB*             bb,
    SCRATCH_PTR_VEC&  candidateList,
    CLEAN_NUM_PROFILE* clean_num_profile)
{
    if (candidateList.size())
    {
        SCRATCH_PTR_VEC::iterator it = candidateList.begin();
        SCRATCH_PTR_VEC::iterator itEnd = candidateList.end();
        while (it != itEnd)
        {
            SCRATCH_PTR_VEC::iterator kt = it;
            kt++;
            SCRATCH_ACCESS * scratchAccess = *it;
            if (scratchAccess->instKilled)
            {
                it = kt;
                continue;
            }
            if (!scratchAccess->instKilled &&
                scratchAccess->isSpill &&
                scratchAccess->removeable &&
                scratchAccess->evicted &&
                scratchAccess->useCount == 0)
            {
                bb->erase(scratchAccess->inst_it);
                scratchAccess->instKilled = true;
                clean_num_profile->spill_clean_num[0]++;
#ifdef _DEBUG
                if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD)
                {
                    return;
                }
#endif
            }
            it = kt;
        }
    }

    return;
}


// Replace Scratch Block Read/Write message with OWord Block Read/Write message
// For spill code clean up, clean target may exist in all WAW, RAR, RAW, WAR.
void FlagSpillCleanup::spillFillCodeCleanFlag(IR_Builder&        builder,
    G4_Kernel&         kernel,
    CLEAN_NUM_PROFILE* clean_num_profile)
{
    SCRATCH_PTR_VEC scratchAccessList;
    SCRATCH_PTR_LIST scratchTraceList;
    SCRATCH_PTR_VEC candidateList;
    FlowGraph& fg = kernel.fg;

//#ifdef _DEBUG
    int candidate_size = 0;
//#endif
    for (auto bb : fg)
    {
        INST_LIST_ITER inst_it = bb->begin();

        scratchTraceList.clear();
        candidateList.clear();
        freeScratchAccess(&scratchAccessList);

        //Top down scan within BB
        while (inst_it != bb->end())
        {
            INST_LIST_ITER inst_it_next = inst_it;
            inst_it_next++;

            flagSpillFillClean(bb, inst_it, scratchAccessList, scratchTraceList, candidateList, clean_num_profile);

            inst_it = inst_it_next;
        }

#ifdef _DEBUG
        candidate_size += (int)candidateList.size();
#endif
        //Clean the fills.
        regFillClean(builder, bb, candidateList, clean_num_profile);

#ifdef _DEBUG
        if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
            return;
#endif
        //Clean the spills
        regSpillClean(builder, bb, scratchAccessList, clean_num_profile);

#ifdef _DEBUG
        if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD)
        {
            return;
        }
#endif
    }

    freeScratchAccess(&scratchAccessList);
    scratchTraceList.clear();
    candidateList.clear();

#ifdef DEBUG_VERBOSE_ON
    printf("Candidate size: %d\n", candidate_size);
#endif

    return;
}

// Insert declarations with pre-assigned registers in kernel
// this is needed for HRA, and the fake declares will be removed at the end of HRA
void GlobalRA::insertPhyRegDecls()
{
    int numGRF = kernel.getNumRegTotal();
    std::vector<bool> grfUsed;
    grfUsed.resize(numGRF, false);
    GRFDclsForHRA.resize(numGRF);

    for (auto curBB : kernel.fg)
    {
        if (auto summary = kernel.fg.getBBLRASummary(curBB))
        {
            for (int i = 0; i < numGRF; i++)
            {
                if (summary->isGRFBusy(i))
                {
                    grfUsed[i] = true;
                }
            }
        }
    }

    // Insert declarations for each GRF that is used
    unsigned int numGRFsUsed = 0;
    for (int i = 0; i < numGRF; i++)
    {
        if (grfUsed[i] == true)
        {
            const char* dclName = builder.getNameString(builder.mem, 10, "r%d", i);
            G4_Declare* phyRegDcl = builder.createDeclareNoLookup(dclName, G4_GRF, 8, 1, Type_D, Regular, NULL, NULL, 0);
            G4_Greg* phyReg = builder.phyregpool.getGreg(i);
            phyRegDcl->getRegVar()->setPhyReg(phyReg, 0);
            GRFDclsForHRA[i] = phyRegDcl;
            numGRFsUsed++;
        }
    }

    if (builder.getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, builder.getOptions());
        optreport << "Local RA used " << numGRFsUsed << " GRFs\n";
    }
}

// compute physical register info and adjust foot print
// find indexed GRFs and construct a foot print for them
// set live operand in each instruction
void GlobalRA::computePhyReg()
{
    auto& fg = kernel.fg;
    for (auto bb : fg)
    {
        for (auto inst : *bb)
        {
            if (inst->isPseudoKill() ||
                inst->isLifeTimeEnd() ||
                inst->isPseudoUse())
            {
                continue;
            }

            if (inst->getDst() &&
                !(inst->hasNULLDst()))
            {
                G4_DstRegRegion *currDstRegion = inst->getDst();
                if (currDstRegion->getBase()->isRegVar() &&
                    currDstRegion->getBase()->asRegVar()->getDeclare()->getGRFBaseOffset() == 0)
                {
                    // Need to compute linearized offset only once per dcl
                    currDstRegion->computePReg();
                }
            }

            for (unsigned j = 0, size = G4_Inst_Table[inst->opcode()].n_srcs; j < size; j++)
            {
                G4_Operand *curr_src = inst->getSrc(j);
                if (!curr_src || curr_src->isImm() || (inst->opcode() == G4_math && j == 1 && curr_src->isNullReg()) || curr_src->isLabel())
                {
                    continue;
                }

                if (curr_src->isSrcRegRegion() &&
                    curr_src->asSrcRegRegion()->getBase() &&
                    curr_src->asSrcRegRegion()->getBase()->isRegVar() &&
                    curr_src->asSrcRegRegion()->getBase()->asRegVar()->getDeclare()->getGRFBaseOffset() == 0)
                {
                    curr_src->asSrcRegRegion()->computePReg();
                }
            }
        }
    }
}

void GraphColor::dumpRegisterPressure()
{
    RPE rpe(gra, &liveAnalysis);
    uint32_t max = 0;
    vector<G4_INST*> maxInst;
    rpe.run();

    for (auto bb : builder.kernel.fg)
    {
        std::cerr << "BB " << bb->getId() << ": (Pred: ";
        for (auto pred : bb->Preds)
        {
            std::cerr << pred->getId() << ",";
        }
        std::cerr << " Succ: ";
        for (auto succ : bb->Succs)
        {
            std::cerr << succ->getId() << ",";
        }
        std::cerr << ")\n";
        for (auto inst : *bb)
        {
            uint32_t pressure = rpe.getRegisterPressure(inst);
            if (pressure > max)
            {
                max = pressure;
                maxInst.clear();
                maxInst.push_back(inst);
            }
            else if (pressure == max)
            {
                maxInst.push_back(inst);
            }

            std::cerr << "[" << pressure << "] ";
            inst->dump();
        }
    }
    std::cerr << "max pressure: " << max << ", " << maxInst.size() << " inst(s)\n";
    for (auto inst : maxInst)
    {
        inst->dump();
    }
}

void GlobalRA::fixAlignment()
{
    // Copy over alignment from G4_RegVar to GlobalRA instance
    // Rest of RA shouldnt have to read/modify alignment of G4_RegVar
    copyAlignment();

    if (kernel.getSimdSize() == 32)
    {
        // we have to force all flags to be 32-bit aligned even if they are < 32-bit,
        // due to potential emask usage.
        // ToDo: may be better to simply allocate them as 32-bit?
        for (auto dcl : kernel.Declares)
        {
            if (dcl->getRegFile() & G4_FLAG)
            {
                setSubRegAlign(dcl, G4_SubReg_Align::Even_Word);
            }
        }
    }

    // ToDo: remove these as it should be done by HWConformity
    for (auto BB : kernel.fg)
    {
        for (auto inst : *BB)
        {
            G4_DstRegRegion* dst = inst->getDst();
            if (dst && dst->getTopDcl())
            {
                G4_RegVar* var = dst->getBase()->asRegVar();
                if (inst->isSend() && dst->getRegAccess() == Direct) 
                {
                    if (!var->isPhyRegAssigned())
                    {
                        setSubRegAlign(dst->getTopDcl(), GRFALIGN);
                    }
                }

                if (!var->isPhyRegAssigned() && var->getDeclare()->getNumRows() <= 1
                    && dst->getRegAccess() == Direct && var->getDeclare()->getSubRegAlign() == Any)
                {
                    if (inst->isAccSrcInst())
                    {
                        setSubRegAlign(dst->getTopDcl(), var->getDeclare()->getRegFile() != G4_ADDRESS ? GRFALIGN : Eight_Word);
                    }
                }
            }
        }
    }
}

void VerifyAugmentation::verifyAlign(G4_Declare* dcl)
{
    // Verify that dcl with Default32Bit align mask are 2GRF aligned
    auto it = masks.find(dcl);
    if (it == masks.end())
        return;

    auto augData = (*it);
    auto dclMask = std::get<1>((*it).second);

    if (dclMask == AugmentationMasks::Default32Bit)
    {
        auto assignment = dcl->getRegVar()->getPhyReg();
        if (assignment && assignment->isGreg())
        {
            auto phyRegNum = assignment->asGreg()->getRegNum();
            if (phyRegNum % 2 != 0)
            {
                printf("Dcl %s is Default32Bit but assignment is not Even aligned\n", dcl->getName());
            }
        }
    }
}

void VerifyAugmentation::dump(const char* dclName)
{
    std::string dclStr = dclName;
    for (auto& m : masks)
    {
        std::string first = m.first->getName();
        if (first == dclStr)
        {
            printf("%s, %d, %s\n", dclName, m.first->getRegVar()->getId(), getStr(std::get<1>(m.second)));
        }
    }
}

void VerifyAugmentation::labelBBs()
{
    std::string prev = "X:";
    unsigned int id = 0;
    for (auto bb : kernel->fg)
    {
        if (bbLabels.find(bb) == bbLabels.end())
            bbLabels[bb] = prev;
        else
            prev = bbLabels[bb];

        if (bb->back()->opcode() == G4_opcode::G4_if)
        {
            auto TBB = bb->Succs.front();
            auto FBB = bb->Succs.back();

            bool hasEndif = false;
            for (auto inst : *FBB)
            {
                if (inst->opcode() == G4_opcode::G4_endif)
                {
                    hasEndif = true;
                    break;
                }
            }

            bbLabels[TBB] = prev + "T" + std::to_string(id) + ":";

            if (!hasEndif)
            {
                // else
                bbLabels[FBB] = prev + "F" + std::to_string(id) + ":";
            }
            else
            {
                // endif block
                bbLabels[FBB] = prev;
            }

            prev = prev + "T" + std::to_string(id) + ":";

            id++;
        }
        else if (bb->back()->opcode() == G4_opcode::G4_else)
        {
            auto succBB = bb->Succs.front();
            auto lbl = prev;
            lbl.pop_back();
            while (lbl.back() != ':')
            {
                lbl.pop_back();
            }

            bbLabels[succBB] = lbl;
        }
        else if (bb->back()->opcode() == G4_opcode::G4_endif)
        {

        }
    }

#if 1
    for (auto bb : kernel->fg)
    {
        printf("BB%d -> %s\n", bb->getId(), bbLabels[bb].data());
    }
#endif
}

bool VerifyAugmentation::interfereBetween(G4_Declare* dcl1, G4_Declare* dcl2)
{
    bool interferes = true;
    unsigned int v1 = dcl1->getRegVar()->getId();
    unsigned int v2 = dcl2->getRegVar()->getId();
    bool v1Partaker = dcl1->getRegVar()->isRegAllocPartaker();
    bool v2Partaker = dcl2->getRegVar()->isRegAllocPartaker();

    if (v1Partaker && v2Partaker)
    {
        auto interferes = intf->interfereBetween(v1, v2);
        if (!interferes)
        {
            if (dcl1->getIsPartialDcl())
            {
                interferes |= intf->interfereBetween(gra->getSplittedDeclare(dcl1)->getRegVar()->getId(), v2);
                if (dcl2->getIsPartialDcl())
                {
                    interferes |= intf->interfereBetween(v1,
                        gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
                    interferes |= intf->interfereBetween(gra->getSplittedDeclare(dcl1)->getRegVar()->getId(),
                        gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
                }
            }
            else if (dcl2->getIsPartialDcl())
            {
                interferes |= intf->interfereBetween(v1, gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
            }
        }
        return interferes;
    }
    else if (!v1Partaker && v2Partaker)
    {
        // v1 is assigned by LRA
        unsigned int startGRF = dcl1->getRegVar()->getPhyReg()->asGreg()->getRegNum();
        unsigned int numGRFs = dcl1->getNumRows();

        for (unsigned int grf = startGRF; grf != (startGRF + numGRFs); grf++)
        {
            for (unsigned int var = 0; var != numVars; var++)
            {
                if (lrs[var] &&
                    lrs[var]->getPhyReg() == kernel->fg.builder->phyregpool.getGreg(grf) &&
                    std::string(lrs[var]->getVar()->getName()) == "r" + std::to_string(grf))
                {
                    if (!intf->interfereBetween(var, v2))
                    {
                        std::cerr << dcl1->getName() << "'s LRA assignment " << grf << " doesnt interfere with " << dcl2->getName() << std::endl;
                        interferes = false;
                    }
                }
            }
        }
    }
    else if (v1Partaker && !v2Partaker)
    {
        return interfereBetween(dcl2, dcl1);
    }
    else if (!v1Partaker && !v2Partaker)
    {
        // both assigned by LRA
        if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF && dcl2->getRegFile() == G4_RegFileKind::G4_GRF)
        {
            auto lr1 = gra->getLocalLR(dcl1);
            auto lr2 = gra->getLocalLR(dcl2);

            if (lr1->getAssigned() && lr2->getAssigned())
            {
                auto preg1Start = dcl1->getGRFBaseOffset();
                auto preg2Start = dcl2->getGRFBaseOffset();
                auto preg1End = preg1Start + dcl1->getByteSize();
                auto preg2End = preg2Start + dcl2->getByteSize();

                if (preg2Start >= preg1Start && preg2Start < preg1End)
                {
                    return false;
                }
                else if (preg1Start >= preg2Start && preg1Start < preg2End)
                {
                    return false;
                }
            }
        }

        interferes = true;
    }

    return interferes;
}

void VerifyAugmentation::verify()
{
    std::cerr << "Start verification for kernel: " << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << std::endl;

    for (auto dcl : kernel->Declares)
    {
        if (dcl->getIsSplittedDcl())
        {
            auto& tup = masks[dcl];
            std::cerr << dcl->getName() << "(" << getStr(std::get<1>(tup)) << ") is split" << std::endl;
            for (unsigned int i = 0; i != gra->getSubDclSize(dcl); i++)
            {
                auto& tupSub = masks[gra->getSubDcl(dcl, i)];
                std::cerr << "\t" << gra->getSubDcl(dcl, i)->getName() << " (" << getStr(std::get<1>(tupSub)) << ")" << std::endl;
            }
        }
    }

    std::cerr << std::endl << std::endl << std::endl;

    auto overlapDcl = [](G4_Declare* dcl1, G4_Declare* dcl2)
    {
        if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF && dcl2->getRegFile() == G4_RegFileKind::G4_GRF)
        {
            auto preg1Start = dcl1->getGRFBaseOffset();
            auto preg2Start = dcl2->getGRFBaseOffset();
            auto preg1End = preg1Start + dcl1->getByteSize();
            auto preg2End = preg2Start + dcl2->getByteSize();

            if (preg2Start >= preg1Start && preg2Start < preg1End)
            {
                return true;
            }
            else if (preg1Start >= preg2Start && preg1Start < preg2End)
            {
                return true;
            }
        }
        return false;
    };

    auto overlap = [](LiveRange* p1, LiveRange* p2)
    {
        if (!p1 || !p2)
            return false;

        auto grf1 = p1->getPhyReg();
        auto grf2 = p2->getPhyReg();

        if (!grf1 || !grf2)
            return false;

        if (!grf1->isGreg() || !grf2->isGreg())
            return false;

        unsigned startp1 = grf1->asGreg()->getRegNum() * G4_GRF_REG_NBYTES + (p1->getPhyRegOff() * p1->getDcl()->getElemSize());
        unsigned int startp2 = grf2->asGreg()->getRegNum() * G4_GRF_REG_NBYTES + (p2->getPhyRegOff()*p2->getDcl()->getElemSize());
        unsigned int endp1 = startp1 + p1->getVar()->getDeclare()->getByteSize();
        unsigned int endp2 = startp2 + p2->getVar()->getDeclare()->getByteSize();

        if (startp1 > startp2 && startp1 < endp2)
            return true;
        if (startp2 > startp1 && startp2 < endp1)
            return true;

        return false;
    };

    std::list<G4_Declare*> active;
    for (auto dcl : sortedLiveRanges)
    {
        auto& tup = masks[dcl];
        unsigned int startIdx = std::get<2>(tup)->getLexicalId();
        auto dclMask = std::get<1>(tup);

        auto getMaskStr = [](AugmentationMasks m)
        {
            std::string str = "Undetermined";
            if (m == AugmentationMasks::Default16Bit)
                str = "Default16Bit";
            else if (m == AugmentationMasks::Default32Bit)
                str = "Default32Bit";
            else if (m == AugmentationMasks::Default64Bit)
                str = "Default64Bit";
            else if (m == AugmentationMasks::NonDefault)
                str = "NonDefault";
            else if (m == AugmentationMasks::DefaultPredicateMask)
                str = "DefaultPredicateMask";
            str.append("\n");

            return str;
        };
        
        std::cerr << dcl->getName() << " - " << getMaskStr(dclMask);
        
        verifyAlign(dcl);

        for (auto it = active.begin(); it != active.end();)
        {
            auto activeDcl = (*it);
            auto& tupActive = masks[activeDcl];
            if (startIdx >= std::get<3>(tupActive)->getLexicalId())
            {
                it = active.erase(it);
                continue;
            }
            it++;
        }

        for (auto activeDcl : active)
        {
            auto& tupActive = masks[activeDcl];
            auto aDclMask = std::get<1>(tupActive);

            if (dclMask != aDclMask)
            {
                bool interfere = interfereBetween(activeDcl, dcl);

                if (!interfere)
                {
                    std::cerr << dcl->getRegVar()->getName() << "(" << getStr(dclMask) << ") and " << activeDcl->getRegVar()->getName() << "(" <<
                        getStr(aDclMask) << ") are overlapping with incompatible emask but not masked as interfering" << std::endl;
                }

                if (overlapDcl(activeDcl, dcl))
                {
                    if (!interfere)
                    {
                        std::cerr << dcl->getRegVar()->getName() << "(" << getStr(dclMask) << ") and " << activeDcl->getName() << "(" <<
                            getStr(aDclMask) << ") use overlapping physical assignments but not marked as interfering" << std::endl;
                    }
                }
            }
        }

        active.push_back(dcl);
    }

    std::cerr << "End verification for kenel: " << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << std::endl << std::endl << std::endl;

    return;

#if 0
    // Following is useful for debugging when test has only if-else-endif constructs
    labelBBs();
    populateBBLexId();
    std::string msg;
    for (auto dcl : sortedLiveRanges)
    {
        auto lr = DclLRMap[dcl];
        if (lr->getPhyReg() && isClobbered(lr, msg))
        {
            printf("%s clobbered:\n\t%s\n\n", dcl->getName(), msg.data());
        }
    }
#endif
}

void VerifyAugmentation::populateBBLexId()
{
    for (auto bb : kernel->fg)
    {
        if (bb->size() > 0)
            BBLexId.push_back(std::make_tuple(bb, bb->front()->getLexicalId(), bb->back()->getLexicalId()));
    }
}

bool VerifyAugmentation::isClobbered(LiveRange* lr, std::string& msg)
{
    msg.clear();

    auto& tup = masks[lr->getDcl()];

    auto startLexId = std::get<2>(tup)->getLexicalId();
    auto endLexId = std::get<3>(tup)->getLexicalId();

    std::vector<std::pair<G4_INST*, G4_BB*>> insts;
    std::vector<std::tuple<INST_LIST_ITER, G4_BB*>> defs;
    std::vector<std::tuple<INST_LIST_ITER, G4_BB*>> uses;

    for (auto bb : kernel->fg)
    {
        if (bb->size() == 0)
            continue;

        if (bb->back()->getLexicalId() > endLexId && bb->front()->getLexicalId() > endLexId)
            continue;

        if (bb->back()->getLexicalId() < startLexId && bb->front()->getLexicalId() < startLexId)
            continue;

        // lr is active in current bb
        for (auto instIt = bb->begin(), end = bb->end(); instIt != end; instIt++)
        {
            auto inst = (*instIt);
            if (inst->isPseudoKill())
                continue;

            if (inst->getLexicalId() > startLexId && inst->getLexicalId() <= endLexId)
            {
                insts.push_back(std::make_pair(inst, bb));
                auto dst = inst->getDst();
                if (dst &&
                    dst->isDstRegRegion())
                {
                    auto topdcl = dst->asDstRegRegion()->getTopDcl();
                    if (topdcl == lr->getDcl())
                        defs.push_back(std::make_tuple(instIt, bb));
                }

                for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
                {
                    auto src = inst->getSrc(i);
                    if (src && src->isSrcRegRegion())
                    {
                        auto topdcl = src->asSrcRegRegion()->getTopDcl();
                        if (topdcl == lr->getDcl())
                            uses.push_back(std::make_tuple(instIt, bb));
                    }
                }
            }
        }
    }

    for (auto& use : uses)
    {
        auto& useStr = bbLabels[std::get<1>(use)];
        auto inst = *std::get<0>(use);
        MUST_BE_TRUE(useStr.size() > 0, "empty string found");
        std::list<std::tuple<G4_INST*, G4_BB*>> rd;

        for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
        {
            auto src = inst->getSrc(i);
            if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getTopDcl() == lr->getDcl())
            {
                unsigned int lb = 0, rb = 0;
                lb = lr->getPhyReg()->asGreg()->getRegNum() * G4_GRF_REG_NBYTES + (lr->getPhyRegOff()*lr->getDcl()->getElemSize());
                lb += src->getLeftBound();
                rb = lb + src->getRightBound() - src->getLeftBound();

                for (auto& otherInsts : insts)
                {
                    if (otherInsts.first->getLexicalId() > inst->getLexicalId())
                        break;

                    auto oiDst = otherInsts.first->getDst();
                    auto oiBB = otherInsts.second;
                    if (oiDst && oiDst->isDstRegRegion() && oiDst->getTopDcl())
                    {
                        unsigned int oilb = 0, oirb = 0;
                        auto oiLR = DclLRMap[oiDst->getTopDcl()];
                        if (oiLR && !oiLR->getPhyReg())
                            continue;

                        oilb = oiLR->getPhyReg()->asGreg()->getRegNum()*G4_GRF_REG_NBYTES +
                            (oiLR->getPhyRegOff()*oiLR->getDcl()->getElemSize());
                        oilb += oiDst->getLeftBound();
                        oirb = oilb + oiDst->getRightBound() - oiDst->getLeftBound();

                        if (oilb <= (unsigned int)rb && oirb >= (unsigned int)lb)
                        {
                            rd.push_back(std::make_tuple(otherInsts.first, oiBB));
                        }
                    }
                }
            }
        }

        auto isComplementary = [](std::string& cur, std::string& other)
        {
            if (cur.size() < other.size())
                return false;

            if (cur.substr(0, other.size() - 1) == other.substr(0, other.size() - 1))
            {
                char lastAlphabet = cur.at(other.size() - 1);
                if (lastAlphabet == 'T' && other.back() == 'F')
                    return true;
                if (lastAlphabet == 'F' && other.back() == 'T')
                    return true;
            }

            return false;
        };

        auto isSameEM = [](G4_INST* inst1, G4_INST* inst2)
        {
            if (inst1->getMaskOption() == inst2->getMaskOption() &&
                inst1->getMaskOffset() == inst2->getMaskOffset())
                return true;
            return false;
        };

        if (rd.size() > 0)
        {
            printf("Current use str = %s for inst:\t", useStr.data());
            inst->emit(std::cerr);
            printf("\t$%d\n", inst->getCISAOff());
        }
        // process all reaching defs
        for (auto rid = rd.begin(); rid != rd.end(); )
        {
            auto& reachingDef = (*rid);

            auto& str = bbLabels[std::get<1>(reachingDef)];

            // skip rd if it is from complementary branch
            if (isComplementary(str, useStr) && isSameEM(inst, std::get<0>(reachingDef)))
            {
#if 0
                printf("\tFollowing in complementary branch %s, removed:\t", str.data());
                std::get<0>(reachingDef)->emit(std::cerr);
                printf("\t$%d\n", std::get<0>(reachingDef)->getCISAOff());
#endif
                rid = rd.erase(rid);
                continue;
            }
            rid++;
        }

        // keep rd that appears last in its BB
        for (auto rid = rd.begin(); rid != rd.end();)
        {
            auto ridBB = std::get<1>(*rid);
            for (auto rid1 = rd.begin(); rid1 != rd.end(); )
            {
                if (*rid == *rid1)
                {
                    rid1++;
                    continue;
                }

                auto rid1BB = std::get<1>(*rid1);
                if (ridBB == rid1BB &&
                    std::get<0>(*rid)->getLexicalId() > std::get<0>(*rid1)->getLexicalId())
                {
#if 0
                    printf("\tErasing inst at $%d due to later def at $%d\n", std::get<0>(*rid1)->getLexicalId(),
                        std::get<0>(*rid)->getLexicalId());
#endif
                    rid1 = rd.erase(rid1);
                    continue;
                }
                rid1++;
            }

            if (rid != rd.end())
                rid++;
        }

        if (rd.size() > 0)
        {
            bool printed = false;
            // display left overs in rd from different dcl
            for (auto& reachingDef : rd)
            {
                if (std::get<0>(reachingDef)->getDst()->getTopDcl() == lr->getDcl()->getRootDeclare())
                    continue;

                if (inst->getCISAOff() == std::get<0>(reachingDef)->getCISAOff())
                    continue;

                if (!printed)
                {
                    printf("\tLeft-over rd:\n");
                    printed = true;
                }
                printf("\t");
                std::get<0>(reachingDef)->emit(std::cerr);
                printf("\t$%d\n", std::get<0>(reachingDef)->getCISAOff());
            }
        }
    }

    return false;
}

void VerifyAugmentation::loadAugData(std::vector<G4_Declare*>& s, LiveRange** l, unsigned int n, Interference* i, GlobalRA& g)
{
    reset();
    sortedLiveRanges = s;
    gra = &g;
    kernel = &gra->kernel;
    lrs = l;
    numVars = n;
    intf = i;

    for (unsigned int i = 0; i != numVars; i++)
    {
        DclLRMap[lrs[i]->getDcl()] = lrs[i];
    }
    for (auto dcl : kernel->Declares)
    {
        if (dcl->getRegFile() == G4_RegFileKind::G4_GRF ||
            dcl->getRegFile() == G4_RegFileKind::G4_INPUT)
        {
            LiveRange* lr = nullptr;
            auto it = DclLRMap.find(dcl);
            if (it != DclLRMap.end())
            {
                lr = (*it).second;
            }
            auto start = gra->getStartInterval(dcl);
            auto end = gra->getEndInterval(dcl);
            masks[dcl] = std::make_tuple(lr, gra->getAugmentationMask(dcl), start, end);
        }
    }
}

//
// DFS to check if there is any conflict in subroutine return location
//
bool GlobalRA::isSubRetLocConflict(G4_BB *bb, std::vector<unsigned> &usedLoc, unsigned stackTop)
{
    auto& fg = kernel.fg;
    if (bb->isAlreadyTraversed(fg.getTraversalNum()))
        return false;
    bb->markTraversed(fg.getTraversalNum());

    G4_INST* lastInst = bb->size() == 0 ? NULL : bb->back();
    if (lastInst && lastInst->isReturn())
    {
        if (lastInst->getPredicate() == NULL)
            return false;
        else
        {
            return isSubRetLocConflict(bb->fallThroughBB(), usedLoc, stackTop);
        }
    }
    else if (lastInst && lastInst->isCall())     // need to traverse to next level
    {
        unsigned curSubRetLoc = getSubRetLoc(bb);
        //
        // check conflict firstly
        //
        for (unsigned i = 0; i<stackTop; i++)
            if (usedLoc[i] == curSubRetLoc)
                return true;
        //
        // then traverse all the subroutines and return BB
        //
        usedLoc[stackTop] = curSubRetLoc;
        unsigned afterCallId = bb->BBAfterCall()->getId();
        for (std::list<G4_BB*>::iterator it = bb->Succs.begin(), end = bb->Succs.end(); it != end; it++)
        {
            if ((*it)->getId() == afterCallId)
            {
                if (isSubRetLocConflict(bb->BBAfterCall(), usedLoc, stackTop))
                    return true;
            }
            else
            {
                G4_BB* subEntry = (*it);
                if (isSubRetLocConflict(subEntry, usedLoc, stackTop + 1))
                    return true;
            }
        }
    }
    else
    {
        for (BB_LIST_ITER it = bb->Succs.begin(); it != bb->Succs.end(); it++)
            if (isSubRetLocConflict(*it, usedLoc, stackTop))
                return true;
    }

    return false;
}

//
// The routine traverses all BBs that can be reached from the entry of a subroutine (not
// traversing into nested subroutine calls). Mark retLoc[bb] = entryId (to associate bb
// with the subroutine entry. When two subroutines share code, we return the location of the
// subroutine that was previously traversed so that the two routines can then use
// the same location to save their return addresses.
//
unsigned GlobalRA::determineReturnAddrLoc(unsigned entryId, unsigned* retLoc, G4_BB* bb)
{
    auto& fg = kernel.fg;
    if (bb->isAlreadyTraversed(fg.getTraversalNum()))
        return retLoc[bb->getId()];
    bb->markTraversed(fg.getTraversalNum());

    if (retLoc[bb->getId()] != UNDEFINED_VAL)
        return retLoc[bb->getId()];
    else
    {
        retLoc[bb->getId()] = entryId;
        G4_INST* lastInst = bb->size() == 0 ? NULL : bb->back();

        if (lastInst && lastInst->isReturn())
        {
            if (lastInst->getPredicate() == NULL)
                return entryId;
            else
                return determineReturnAddrLoc(entryId, retLoc, bb->fallThroughBB());
        }
        else if (lastInst && lastInst->isCall()) // skip nested subroutine calls
        {
            return determineReturnAddrLoc(entryId, retLoc, bb->BBAfterCall());
        }
        unsigned sharedId = entryId;
        for (BB_LIST_ITER it = bb->Succs.begin(); it != bb->Succs.end(); it++)
        {
            unsigned loc = determineReturnAddrLoc(entryId, retLoc, *it);
            if (loc != entryId)
            {
                while (retLoc[loc] != loc)  // find the root of subroutine loc
                    loc = retLoc[loc];      // follow the link to reach the root
                if (sharedId == entryId)
                {
                    sharedId = loc;
                }
                else if (sharedId != loc)
                {
                    //
                    // The current subroutine share code with two other subroutines, we
                    // force all three of them to use the same location by linking them
                    // togethers.
                    //
                    retLoc[loc] = sharedId;
                }
            }
        }
        return sharedId;
    }
}

void GlobalRA::assignLocForReturnAddr()
{
    auto& fg = kernel.fg;
    unsigned* retLoc = (unsigned*)builder.mem.alloc(fg.getNumBB() * sizeof(unsigned));
    //
    // a data structure for doing a quick map[id] ---> block
    //
    G4_BB**  BBs = (G4_BB**)builder.mem.alloc(fg.getNumBB() * sizeof(G4_BB*));
    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++)
    {
        unsigned i = (*it)->getId();
        retLoc[i] = UNDEFINED_VAL;
        BBs[i] = (*it);                                                     // BBs are sorted by ID
    }

    //
    // Firstly, keep the original algorithm unchanged to mark the retLoc
    //
    std::list<G4_BB *> caller;                                          // just to accelerate the algorithm later

    for (unsigned i = 0; i < fg.getNumBB(); i++)
    {
        G4_BB* bb = BBs[i];
        if (bb->isEndWithCall() == false)
        {
            continue;
        }

#ifdef _DEBUG
        G4_INST *last = bb->empty() ? NULL : bb->back();
        MUST_BE_TRUE(last, ERROR_FLOWGRAPH);
#endif

        caller.push_back(bb);                   // record the  callers, just to accelerate the algorithm

        G4_BB* subEntry = bb->getCalleeInfo()->getInitBB();
        if (retLoc[subEntry->getId()] != UNDEFINED_VAL) // a loc has been assigned to the subroutine
        {
            // Need to setSubRetLoc if subEntry is part of another subRoutine because,
            // in the final phase, we use SubRetLoc != UNDEFINED_VAL to indicate
            // a block is an entry of a subroutine.
            setSubRetLoc(subEntry, retLoc[subEntry->getId()]);
        }
        else
        {
            fg.prepareTraversal();
            unsigned loc = determineReturnAddrLoc(subEntry->getId(), retLoc, subEntry);
            if (loc != subEntry->getId())
            {
                retLoc[subEntry->getId()] = loc;
            }
            setSubRetLoc(subEntry, loc);
            //
            // We do not merge indirect call here, because it will createt additional (bb->getSubRetLoc() != bb->getId())  cases that kill the share code detection
            //
        }

        // retBB is the exit basic block of callee, ie the block with return statement at end
        G4_BB* retBB = bb->getCalleeInfo()->getExitBB();

        if (retLoc[retBB->getId()] == UNDEFINED_VAL)
        {
            // retBB block was unreachable so retLoc element corresponding to that block was
            // left undefined
            retLoc[retBB->getId()] = getSubRetLoc(subEntry);
        }
    }
#ifdef DEBUG_VERBOSE_ON
    DEBUG_MSG(std::endl << "Before merge indirect call: " << std::endl);
    for (unsigned i = 0; i < fg.getNumBB(); i++)
        if (retLoc[i] == UNDEFINED_VAL) {
            DEBUG_MSG("BB" << i << ": X   ");
        }
        else {
            DEBUG_MSG("BB" << i << ": " << retLoc[i] << "   ");
        }
        DEBUG_MSG(std::endl);
#endif

        //
        // this final phase is needed. Consider the following scenario.  Sub2 shared code with both
        // Sub1 and Sub3. All three must use the same location to save return addresses. If we traverse
        // Sub1 then Sub3, retLoc[Sub1] and retLoc[Sub3] all point to their own roots.  As we traverse
        // Sub2, code sharing is detected, we need to this phase to make sure that Sub1 and Sub3 use the
        // same location.
        //
        for (unsigned i = 0; i < fg.getNumBB(); i++)
        {
            G4_BB* bb = BBs[i];
            if (getSubRetLoc(bb) != UNDEFINED_VAL)
            {
                if (getSubRetLoc(bb) != bb->getId())
                {
                    unsigned loc = bb->getId();
                    while (retLoc[loc] != loc)  // not root
                        loc = retLoc[loc];  // follow the link to reach the root
                }
            }
        }

        //
        // Merge the retLoc in indirect call cases
        //
        for (std::list<G4_BB*>::iterator it = caller.begin(); it != caller.end(); it++)
        {
            G4_BB *bb = *it;
            G4_INST *last = bb->empty() ? NULL : bb->back();
            MUST_BE_TRUE(last, ERROR_FLOWGRAPH);

            unsigned fallThroughId = bb->fallThroughBB() == NULL ? UNDEFINED_VAL : bb->fallThroughBB()->getId();
            if ((last && last->getPredicate() == NULL && bb->Succs.size() > 1) || (last && last->getPredicate() != NULL && bb->Succs.size() > 2))
            {
                //
                // merge all subroutines to the last one, it is a trick to conduct the conditional call by using last one instead of first one
                //
                unsigned masterEntryId = bb->Succs.back()->getId();
                //
                // find the root of the master subroutine
                //
                unsigned masterRetLoc = masterEntryId;
                while (retLoc[masterRetLoc] != masterRetLoc)
                    masterRetLoc = retLoc[masterRetLoc];
                //
                // check other subroutines in one vertex
                //
                for (std::list<G4_BB*>::iterator it1 = bb->Succs.begin(); it1 != bb->Succs.end(); it1++)
                {
                    G4_BB *subBB = *it1;
                    if (subBB->getId() != masterEntryId && subBB->getId() != fallThroughId)
                    {
                        //
                        // find the root of the current subroutine
                        //
                        unsigned loc = subBB->getId();
                        while (retLoc[loc] != loc)
                            loc = retLoc[loc];
                        //
                        // Merge: let all the items in retLoc with value loc pointing to masterRetLoc
                        // Suppose indirect call X calls subroutine A and B, indirect call Y calls B and C, and indirect call Z calls C and D.
                        // Before merge, the A~D will be assigned different return location. Suppose we process the callers in order X-->Z-->Y in the merge,
                        // if we just modified the return locations of one indirect call, we will fail to merge the return locations of A~D.
                        //
                        if (loc != masterRetLoc)
                        {
                            for (unsigned i = 0; i < fg.getNumBB(); i++)
                                if (retLoc[i] == loc)
                                    retLoc[i] = masterRetLoc;
                        }
                    }
                }
            }
        }

#ifdef DEBUG_VERBOSE_ON
        DEBUG_MSG(std::endl << "After merge indirect call: " << std::endl);
        for (unsigned i = 0; i < fg.getNumBB(); i++)
            if (retLoc[i] == UNDEFINED_VAL) {
                DEBUG_MSG("BB" << i << ": X   ");
            }
            else {
                DEBUG_MSG("BB" << i << ": " << retLoc[i] << "   ");
            }
            DEBUG_MSG(std::endl << std::endl);
#endif

            //
            //  Assign ret loc for subroutines firstly, and then check if it is wrong (due to circle in call graph).
            //
            for (unsigned i = 0; i < fg.getNumBB(); i++)
            {
                //
                // reset the return BB's retLoc
                //
                unsigned loc = i;
                if (retLoc[i] != UNDEFINED_VAL)
                {
                    while (retLoc[loc] != loc)
                        loc = retLoc[loc];
                    retLoc[i] = loc;
                    setSubRetLoc(BBs[i], retLoc[loc]);
                }
            }

            for (std::list<G4_BB*>::iterator it = caller.begin(); it != caller.end(); it++)
            {
                //
                // set caller BB's retLoc
                //
                G4_BB *bb = *it;
#ifdef _DEBUG
                G4_INST *last = bb->empty() ? NULL : bb->back();
                MUST_BE_TRUE(last, ERROR_FLOWGRAPH);
#endif
                G4_BB *subBB = bb->getCalleeInfo()->getInitBB();
                //
                // 1: Must use retLoc here, because some subBB is also the caller of another subroutine, so the entry loc in BB may be changed in this step
                // 2: In some cases, the caller BB is also the entry BB. At this time, the associated entry BB ID will be overwritten. However, it will not impact the
                // conflict detection and return location assignment, since we only check the return BB and/or caller BB in these two moudles.
                //
                setSubRetLoc(bb, retLoc[subBB->getId()]);
            }

#ifdef _DEBUG
            for (unsigned i = 0; i < fg.getNumBB(); i++)
            {
                G4_BB* bb = BBs[i];
                if (getSubRetLoc(bb) != UNDEFINED_VAL)
                {
                    if (!bb->empty() && bb->front()->isLabel())
                    {
                        DEBUG_VERBOSE(((G4_Label*)bb->front()->getSrc(0))->getLabel()
                            << " assigned location " << bb->getSubRetLoc() << std::endl);
                    }
                }
            }
#endif

            //
            // detect the conflict (circle) at last
            //
            std::vector<unsigned> usedLoc(fg.getNumBB());
            unsigned stackTop = 0;
            for (std::list<G4_BB*>::iterator it = caller.begin(); it != caller.end(); it++)
            {
                G4_BB* bb = *it;
                MUST_BE_TRUE(bb->BBAfterCall() != NULL, ERROR_FLOWGRAPH);
                //
                // Must re-start the traversal from each caller, otherwise will lose some circle cases like TestRA_Call_1_1_3B, D, F, G, H
                //
                fg.prepareTraversal();

                usedLoc[stackTop] = getSubRetLoc(bb);
                unsigned afterCallId = bb->BBAfterCall()->getId();
                for (std::list<G4_BB*>::iterator it = bb->Succs.begin(); it != bb->Succs.end(); it++)
                {
                    G4_BB* subEntry = (*it);
                    if (subEntry->getId() == afterCallId)
                        continue;

                    if (isSubRetLocConflict(subEntry, usedLoc, stackTop + 1))
                    {
                        MUST_BE_TRUE(false,
                            "ERROR: Fail to assign call-return variables due to cycle in call graph!");
                    }
                }
            }

            insertCallReturnVar();
}

void  GlobalRA::insertCallReturnVar()
{
    for (auto bb : kernel.fg)
    {
        G4_INST *last = bb->empty() ? NULL : bb->back();
        if (last)
        {
            if (last->isCall())
            {
                insertSaveAddr(bb);
            }
            else
            {
                if (last->isReturn())
                {
                    // G4_BB_EXIT_TYPE is just a dummy BB, and the return will be the last
                    // inst in each of its predecessors
                    insertRestoreAddr(bb);
                }
            }
        }
    }
}

void  GlobalRA::insertSaveAddr(G4_BB* bb)
{
    MUST_BE_TRUE(bb != NULL, ERROR_INTERNAL_ARGUMENT);
    MUST_BE_TRUE(getSubRetLoc(bb) != UNDEFINED_VAL,
        ERROR_FLOWGRAPH); // must have a assigned loc


    G4_INST *last = bb->back();
    MUST_BE_TRUE1(last->isCall(), last->getLineNo(),
        ERROR_FLOWGRAPH);
    if (last->getDst() == NULL)
    {
        unsigned loc = getSubRetLoc(bb);
        G4_Declare* dcl = getRetDecl(loc);

        last->setDest(builder.createDstRegRegion(Direct, dcl->getRegVar(), 0, 0, 1, Type_UD)); // RET__loc12<1>:ud

        last->setExecSize(2);
    }
}

void  GlobalRA::insertRestoreAddr(G4_BB* bb)
{
    MUST_BE_TRUE(bb != NULL, ERROR_INTERNAL_ARGUMENT);

    G4_INST *last = bb->back();
    MUST_BE_TRUE1(last->isReturn(), last->getLineNo(),
        ERROR_FLOWGRAPH);
    if (last->getSrc(0) == NULL)
    {
        unsigned loc = getSubRetLoc(bb);
        G4_Declare* dcl = getRetDecl(loc);

        G4_SrcRegRegion* new_src = builder.createSrcRegRegion(Mod_src_undef,   // RET__loc12<0;2,1>:ud
            Direct,
            dcl->getRegVar(),
            0,
            0,
            builder.createRegionDesc(0, 2, 1),
            Type_UD);

        last->setSrc(new_src, 0);
        last->setDest(builder.createNullDst(Type_UD));

        last->setExecSize(2);
    }
}

// This function returns the weight of interference edge lr1--lr2,
// which is used for computing the degree of lr1.
//
// When there is no alignment restriction, we should use the normal weight,
// which is lr1_nreg + lr2_nreg - 1.
//
// Otherewise, we need to take into account additional space that may be
// required because of the alignment restriction. For example,
// if lr1 has even alignment and lr2 has no alignment restriction,
// we need to consider the following cases that would require the
// maximal available GRF space for successful allocation:
// 1) lr1's size is odd, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// 2) lr1's size is odd, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 3) lr1's size is even, lr2's size is odd and lr2's start position is odd,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 4) lr1's size is even, lr2's size is even and lr2's start position is odd,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// The above logic can be simplified to the following formula:
//    lr1_nreg + lr2_nreg + 1 - ((lr1_nreg + lr2_nreg) % 2)
//
// If both lr1 and lr2 have even alignment restriction,
// we need to consider the following cases that would require the
// maximal available GRF space for successful allocation:
// 1) lr1's size is odd, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// 2) lr1's size is odd, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 3) lr1's size is even, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 4) lr1's size is even, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg - 1)
// The above logic can be simplified to the following formula:
//    lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2)
//
unsigned GraphColor::edgeWeightGRF(LiveRange* lr1, LiveRange* lr2)
{
    bool lr1EvenAlign = gra.isEvenAligned(lr1->getVar()->getDeclare());
    bool lr2EvenAlign = gra.isEvenAligned(lr2->getVar()->getDeclare());
    unsigned lr1_nreg = lr1->getNumRegNeeded();
    unsigned lr2_nreg = lr2->getNumRegNeeded();

    if (!lr1EvenAlign)
    {
        return  lr1_nreg + lr2_nreg - 1;
    }
    else if (!lr2EvenAlign)
    {
        unsigned sum = lr1_nreg + lr2_nreg;
        return sum + 1 - ((sum) % 2);
    }
    else if (lr2EvenAlign)
    {
        return lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2);
    }
    else
    {
        assert(false && "should be unreachable");
        return 0;
    }
}

unsigned GraphColor::edgeWeightARF(LiveRange* lr1, LiveRange* lr2)
{
    if (lr1->getRegKind() == G4_FLAG)
    {
        G4_SubReg_Align lr1_align = gra.getSubRegAlign(lr1->getVar()->getDeclare());
        G4_SubReg_Align lr2_align = gra.getSubRegAlign(lr2->getVar()->getDeclare());
        unsigned lr1_nreg = lr1->getNumRegNeeded();
        unsigned lr2_nreg = lr2->getNumRegNeeded();

        if (lr1_align == Any)
        {
            return  lr1_nreg + lr2_nreg - 1;
        }
        else if (lr1_align == Even_Word && lr2_align == Any)
        {
            return lr1_nreg + lr2_nreg + 1 - ((lr1_nreg + lr2_nreg) % 2);
        }
        else if (lr1_align == Even_Word && lr2_align == Even_Word)
        {
            if (lr1_nreg % 2 == 0 && lr2_nreg % 2 == 0)
            {
                return lr1_nreg + lr2_nreg - 2;
            }
            else
            {
                return lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2);
            }
        }
        else
        {
            MUST_BE_TRUE(false, "Found unsupported subRegAlignment in flag register allocation!");
            return 0;
        }
    }
    else if (lr1->getRegKind() == G4_ADDRESS)
    {
        G4_SubReg_Align lr1_align = gra.getSubRegAlign(lr1->getVar()->getDeclare());
        G4_SubReg_Align lr2_align = gra.getSubRegAlign(lr2->getVar()->getDeclare());
        unsigned lr1_nreg = lr1->getNumRegNeeded();
        unsigned lr2_nreg = lr2->getNumRegNeeded();

        if (lr1_align == Any)
        {
            return  lr1_nreg + lr2_nreg - 1;
        }
        else if (lr1_align == Four_Word && lr2_align == Any)
        {
            return lr1_nreg + lr2_nreg + 3 - (lr1_nreg + lr2_nreg) % 4;
        }
        else if (lr1_align == Four_Word && lr2_align == Four_Word)
        {
            return lr1_nreg + lr2_nreg - 1 + (4 - lr1_nreg % 4) % 4 + (4 - lr2_nreg % 4) % 4;
        }
        else if (lr1_align == Eight_Word && lr2_align == Any)
        {
            return lr1_nreg + lr2_nreg + 7 - (lr1_nreg + lr2_nreg) % 8;
        }
        else if (lr1_align == Eight_Word && lr2_align == Four_Word)
        {
            if (((8 - lr1_nreg % 8) % 8) >= 4)
                return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 - 4;
            return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 +
                (4 - lr2_nreg % 4) % 4;
        }
        else if (lr1_align == Eight_Word && lr2_align == Eight_Word)
        {
            return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 +
                (8 - lr2_nreg % 8) % 8;
        }
        else
        {
            MUST_BE_TRUE(false, "Found unsupported subRegAlignment in address register allocation!");
            return 0;
        }
    }
    MUST_BE_TRUE(false, "Found unsupported ARF reg type in register allocation!");
    return 0;
}