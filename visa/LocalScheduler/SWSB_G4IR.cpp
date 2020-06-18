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

#include <fstream>
#include <functional>
#include <sstream>
#include "SWSB_G4IR.h"
#include "../G4_Opcode.h"
#include "../Timer.h"
#include "../RegAlloc.h"
#include "visa_wa.h"
#include <queue>

using namespace std;
using namespace vISA;

#define MIN(x,y)    (((x)<(y))? (x):(y))


static bool hasSameFunctionID(G4_INST* inst1, G4_INST* inst2)
{
    if (inst1->isSend() && inst2->isSend())
    {
        G4_SendMsgDescriptor* msgDesc1 = inst1->getMsgDesc();
        G4_SendMsgDescriptor* msgDesc2 = inst2->getMsgDesc();

        if (msgDesc1->isSLMMessage() && msgDesc2->isSLMMessage())
        {
            return (msgDesc1->getFuncId() == msgDesc2->getFuncId());
        }
        else if (msgDesc1->isSLMMessage() || msgDesc2->isSLMMessage())
        {
            return false;
        }

        return (msgDesc1->getFuncId() == msgDesc2->getFuncId());
    }
    else if (inst1->isSend() || inst2->isSend())
    {
        return false;
    }
    else if (inst1->isMathPipeInst() && inst2->isMathPipeInst())
    {
        return true;
    }
    else if (inst1->isMathPipeInst() || inst2->isMathPipeInst())
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool isSLMMsg(G4_INST* inst)
{
    assert(inst->isSend());
    G4_SendMsgDescriptor* msgDesc = inst->getMsgDesc();
    if (msgDesc->isSLMMessage())
    {
        return true;
    }
    return false;
}

static bool isFence(G4_INST* inst)
{
    assert(inst->isSend());
    G4_SendMsgDescriptor* msgDesc = inst->getMsgDesc();
    if (msgDesc->isFence())
    {
        return true;
    }
    return false;
}




static bool hasSamePredicator(G4_INST* inst1, G4_INST* inst2)
{
    G4_Predicate* pred1 = inst1->getPredicate();
    G4_Predicate* pred2 = inst2->getPredicate();

    if (pred1 && pred2)
    {
        if (pred1->getRegOff() == pred2->getRegOff() &&
            pred1->getSubRegOff() == pred2->getSubRegOff())
        {
            return true;
        }
        return false;
    }

    if (pred1 || pred2)
    {
        return false;
    }

    if (inst1->isWriteEnableInst() || inst2->isWriteEnableInst())
    {
        return false;
    }

    return true;
}

static bool hasSameExecMask(G4_INST* inst1, G4_INST* inst2)
{
    uint16_t mask1 = inst1->getMaskOffset();
    uint16_t mask2 = inst2->getMaskOffset();
    unsigned char execSize1 = inst1->getExecSize();
    unsigned char execSize2 = inst2->getExecSize();

    if (mask1 != mask2)
    {
        return false;
    }

    if (execSize1 != execSize2)
    {
        return false;
    }

    return true;
}

static bool WARDepRequired(G4_INST* inst1, G4_INST* inst2)
{
    if (!hasSameFunctionID(inst1, inst2) ||
        (hasSameFunctionID(inst1, inst2) &&
        (!hasSamePredicator(inst1, inst2) ||
            !hasSameExecMask(inst1, inst2))))
    {
        return true;
    }

    return false;
}


// Return the dependence type {RAW,WAW,WAR,NODEP} for given operand numbers
static DepType getDepForOpnd(Gen4_Operand_Number cur,
    Gen4_Operand_Number liv) {
    switch (cur) {
    case Opnd_dst:
    case Opnd_implAccDst:
    case Opnd_condMod: {
        switch (liv) {
        case Opnd_dst:
        case Opnd_implAccDst:
        case Opnd_condMod:
            return WAW;
        case Opnd_src0:
        case Opnd_src1:
        case Opnd_src2:
        case Opnd_src3:
        case Opnd_implAccSrc:
        case Opnd_pred:
            return RAW;
        default:
            assert(0 && "bad opnd numb");
            return DEPTYPE_MAX; // Unreachable
        }
    }
    case Opnd_src0:
    case Opnd_src1:
    case Opnd_src2:
    case Opnd_src3:
    case Opnd_implAccSrc:
    case Opnd_pred: {
        switch (liv) {
        case Opnd_dst:
        case Opnd_implAccDst:
        case Opnd_condMod:
            return WAR;
        case Opnd_src0:
        case Opnd_src1:
        case Opnd_src2:
        case Opnd_src3:
        case Opnd_implAccSrc:
        case Opnd_pred:
            return NODEP;
        default:
            assert(0 && "bad opnd numb");
            return DEPTYPE_MAX; // Unreachable
        }
    }
    default:
        assert(0 && "bad opnd numb");
        return DEPTYPE_MAX; // Unreachable
    }
}

// check if two operands occupy overlapping GRFs
// we put them here instead of inside G4_Operand since this is only valid till after RA
// It's the caller's responsibilty to ensure that opnd1 and opnd2 are both GRF allocated
static bool operandOverlap(G4_Operand* opnd1, G4_Operand* opnd2)
{
    return (opnd1->getLinearizedStart() <= opnd2->getLinearizedStart() &&
        opnd1->getLinearizedEnd() > opnd2->getLinearizedStart()) ||
        (opnd2->getLinearizedStart() <= opnd1->getLinearizedStart() &&
            opnd2->getLinearizedEnd() > opnd1->getLinearizedStart());
}

// Compute the range of registers touched by OPND.
SBFootprint* G4_BB_SB::getFootprintForGRF(G4_Operand* opnd,
    Gen4_Operand_Number opnd_num,
    G4_INST* inst,
    int startingBucket,
    bool mustBeWholeGRF)
{
    unsigned short LB = 0;
    unsigned short RB = 0;
    int aregOffset = totalGRFNum;
    G4_Type type = opnd->getType();

    switch (opnd_num) {
    case Opnd_src0:
    case Opnd_src1:
    case Opnd_src2:
    case Opnd_src3:
    case Opnd_dst:
        LB = (unsigned short)opnd->getLinearizedStart();
        RB = (unsigned short)opnd->getLinearizedEnd();
        if (inst->isSend())
        {
            assert((LB % GENX_GRF_REG_SIZ) == 0);
            //For the operands of the send instructions,
            //we are using the message length to avoid the in-consistence with the HW requirement.
            //
            if (opnd_num == Opnd_src0)
            {
                RB = LB + G4_GRF_REG_NBYTES * inst->getMsgDesc()->MessageLength() - 1;
            }

            if (inst->isSplitSend() &&
                opnd_num == Opnd_src1)
            {
                RB = LB + G4_GRF_REG_NBYTES * inst->getMsgDesc()->extMessageLength() - 1;
            }

            if (opnd_num == Opnd_dst)
            {
                RB = LB + G4_GRF_REG_NBYTES * inst->getMsgDesc()->ResponseLength() - 1;
            }

            assert(RB < (G4_GRF_REG_NBYTES * aregOffset) && "Out of register bound");
        }
        break;
    default:
        assert(0 && "Bad opnd");
    }

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    SBFootprint* footprint = nullptr;
    if (startingBucket >= aregOffset)
    {
        LB = startingBucket * G4_GRF_REG_NBYTES + LB;
        RB = startingBucket * G4_GRF_REG_NBYTES + RB;
    }

    //This is WA which assumes whole GRF will be touched in send instruction, not matter the occupation of real valid value.
    //FIXME: But this is not true in media block read/write, which can specify the byte level size in descriptor, no GRF align required.
    if (mustBeWholeGRF)
    {
        LB = (LB / G4_GRF_REG_NBYTES) * G4_GRF_REG_NBYTES;
        RB = ((RB / G4_GRF_REG_NBYTES) + 1) * G4_GRF_REG_NBYTES - 1;
    }

    footprint = new(allocedMem)SBFootprint(GRF_T, type, LB, RB, inst);

    return footprint;
}

// Compute the range of registers touched by OPND.
SBFootprint* G4_BB_SB::getFootprintForACC(G4_Operand* opnd,
    Gen4_Operand_Number opnd_num,
    G4_INST* inst)
{
    unsigned short LB = 0;
    unsigned short RB = 0;
    G4_Type type = opnd->getType();

    switch (opnd_num) {
    case Opnd_src0:
    case Opnd_src1:
    case Opnd_src2:
    case Opnd_src3:
    case Opnd_dst:
    case Opnd_implAccSrc:
    case Opnd_implAccDst:
        LB = (unsigned short)opnd->getLinearizedStart();
        RB = (unsigned short)opnd->getLinearizedEnd();
        break;
    default:
        assert(0 && "Bad opnd");
    }

    int regNum = 0;
    if (opnd->isDstRegRegion())
        regNum += opnd->asDstRegRegion()->getRegOff();
    else if (opnd->isSrcRegRegion())
        regNum += opnd->asSrcRegRegion()->getRegOff();

    LB += regNum * G4_GRF_REG_NBYTES;
    RB += regNum * G4_GRF_REG_NBYTES;

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    SBFootprint* footprint = nullptr;

    footprint = new(allocedMem)SBFootprint(ACC_T, type, LB, RB, inst);

    return footprint;
}

// Compute the range of flag registers touched by OPND.
// Treat each 16 bit of the flag register as a bucket unit, GRF size
// 64 bytes GRF: each bit means 8 bytes
// 32 bytes GRF: each bit means 4 bytes
SBFootprint* G4_BB_SB::getFootprintForFlag(G4_Operand* opnd,
    Gen4_Operand_Number opnd_num,
    G4_INST* inst)
{
    unsigned short LB = 0;
    unsigned short RB = 0;
    G4_Type type = opnd->getType();
    unsigned short bitToBytes = G4_GRF_REG_NBYTES / 16;
    bool valid = true;
    unsigned subRegOff = opnd->getBase()->ExSubRegNum(valid);
    LB = (unsigned short)(opnd->getLeftBound() + subRegOff * 16) * bitToBytes;
    RB = (unsigned short)(opnd->getRightBound() + subRegOff * 16) * bitToBytes;

    LB += (builder.kernel.getNumRegTotal() + builder.kernel.getNumAcc()) * G4_GRF_REG_NBYTES;
    RB += (builder.kernel.getNumRegTotal() + builder.kernel.getNumAcc()) * G4_GRF_REG_NBYTES;

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    SBFootprint* footprint = nullptr;

    footprint = new(allocedMem)SBFootprint(FLAG_T, type, LB, RB, inst);

    return footprint;
}

static bool compareInterval(SBNode* n1, SBNode* n2)
{
    return n1->getLiveStartID() < n2->getLiveStartID();
}

static bool compareBBStart(G4_BB_SB* b1, G4_BB_SB* b2)
{
    return b1->first_node < b2->first_node;
}

static bool nodeSortCompare(SBDEP_ITEM dep1, SBDEP_ITEM dep2)
{
    if (dep1.node->getBBID() < dep2.node->getBBID())
    {
        return true;
    }
    else if (dep1.node->getBBID() == dep2.node->getBBID())
    {
        return (dep1.node->getNodeID() < dep2.node->getNodeID());
    }

    return false;
}

// Return TRUE if opnd corresponding to opndNum has indirect access.
static inline bool hasIndirection(G4_Operand* opnd, Gen4_Operand_Number opndNum) {
    switch (opndNum) {
    case Opnd_dst:
        return opnd->asDstRegRegion()->isIndirect();
    case Opnd_src0:
    case Opnd_src1:
    case Opnd_src2:
        return opnd->asSrcRegRegion()->isIndirect();
    case Opnd_src3:
    case Opnd_pred:
    case Opnd_condMod:
    case Opnd_implAccSrc:
    case Opnd_implAccDst:
        return false;
    default:
        assert(0 && "Bad opndNum");
        return false;           // Unreachable
    }
}

static inline bool distanceHonourInstruction(G4_INST* inst)
{
    return !inst->tokenHonourInstruction() && !inst->isWait() && inst->opcode() != G4_nop && inst->opcode() != G4_halt;
}

static inline bool tokenHonourInstruction(G4_INST* inst)
{
    return inst->tokenHonourInstruction();
}


static bool hasNoPipe(G4_INST* inst)
{
    if (inst->opcode() == G4_wait || inst->opcode() == G4_halt || inst->opcode() == G4_nop)
    {
        return true;
    }
    return false;
}



//Generate the dependence distance
void SWSB::setDefaultDistanceAtFirstInstruction()
{
    for (auto bb : fg)
    {
        for (auto it = bb->begin();
            it != bb->end();
            it++)
        {
            if (!(*it)->isLabel())
            {
                (*it)->setDistance(1);

                return;
            }
        }
    }

    return;
}

void SWSB::addSIMDEdge(G4_BB_SB* pred, G4_BB_SB* succ)
{
    pred->Succs.push_back(succ);
    succ->Preds.push_back(pred);

    return;
}

// Build SIMD CFG for the global WAR dependence tracking
// 1. When buliding CFG, except the backedge, all using JIP branch edge.
// 2. For the join and endif instructions which are no seperated and place in the head of a BB. We do edge propgation
//    Such as:   BB a, b, c, d,  there is a join in BB b which JIP to d, and there is an edge from a to b, we will add edge from a to d, instead of b to d.
void SWSB::SWSBBuildSIMDCFG()
{
    //Build parallel control flow graph
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        G4_BB_SB* currBB = BBVector[i];
        G4_INST* lastInst = currBB->getBB()->back();
        INST_LIST_ITER firstInstIter = currBB->getBB()->begin();
        while (firstInstIter != currBB->getBB()->end() &&
            (*firstInstIter)->isLabel())
            firstInstIter++;
        if (firstInstIter != currBB->getBB()->end())
        {
            G4_INST* firstInst = (*firstInstIter);

            if (firstInst != lastInst &&
                G4_Inst_Table[firstInst->opcode()].instType == InstTypeFlow)
            {
                if (firstInst->asCFInst()->getJip())
                {
                    G4_Operand* jip = firstInst->asCFInst()->getJip();
                    G4_BB_SB* targetBB = labelToBlockMap[jip->asLabel()];

                    //Do we need to propagate edge for fall through preds?
                    for (BB_SWSB_LIST_ITER iter = currBB->Preds.begin();
                        iter != currBB->Preds.end();
                        iter++)
                    {
                        G4_BB_SB* predBB = (*iter);
                        addSIMDEdge(predBB, targetBB);
                    }
                }
            }
        }

        if (lastInst->isEOT())
        {
            continue;
        }

        if (G4_Inst_Table[lastInst->opcode()].instType == InstTypeFlow)
        {
            G4_opcode op = lastInst->opcode();

            if (op == G4_jmpi)
            {
                G4_Operand* jip = lastInst->getSrc(0);
                G4_BB_SB* targetBB = labelToBlockMap[jip->asLabel()];
                addSIMDEdge(currBB, targetBB);
                if (lastInst->getPredicate())
                {
                    if (i + 1 != BBVector.size())
                    {
                        addSIMDEdge(currBB, BBVector[i + 1]);
                    }
                }
            }
            else if (lastInst->isReturn() || lastInst->isCall())
            {
                BB_LIST_ITER iter = currBB->getBB()->Succs.begin();
                while (iter != currBB->getBB()->Succs.end())
                {
                    G4_BB* bb = (*iter);
                    unsigned bbID = bb->getId();
                    addSIMDEdge(currBB, BBVector[bbID]);
                    iter++;
                }
            }
            else if (lastInst->asCFInst()->getJip())
            {
                if (op == G4_goto)
                {
                    G4_Operand* jip = lastInst->asCFInst()->getJip();
                    G4_Operand* uip = lastInst->asCFInst()->getUip();
                    G4_BB_SB* jipBB = labelToBlockMap[jip->asLabel()];
                    G4_BB_SB* uipBB = labelToBlockMap[uip->asLabel()];
                    if (jipBB != uipBB && jipBB->first_node > uipBB->first_node)
                    {//backedge, goto uip
                        addSIMDEdge(currBB, uipBB);
                    }
                    else //goto jip
                    {
                        addSIMDEdge(currBB, jipBB);
                    }

                    if (lastInst->getPredicate())
                    {
                        if (i + 1 != BBVector.size())
                        {
                            addSIMDEdge(currBB, BBVector[i + 1]);
                        }
                    }
                }
                else if (op == G4_break)
                {
                    G4_Operand* jip = lastInst->asCFInst()->getJip();
                    G4_Operand* uip = lastInst->asCFInst()->getUip();
                    G4_BB_SB* jipBB = labelToBlockMap[jip->asLabel()];
                    G4_BB_SB* uipBB = labelToBlockMap[uip->asLabel()];
                    if (jipBB == uipBB)
                    {
                        G4_BB* bb = jipBB->getBB();
                        unsigned bbID = bb->getId();
                        assert(bbID + 1 != BBVector.size());
                        addSIMDEdge(currBB, BBVector[bbID + 1]);
                    }
                    else  //Add the jip edge to the CFG
                    {
                        addSIMDEdge(currBB, jipBB);
                    }
                    if (i + 1 != BBVector.size())
                    {
                        addSIMDEdge(currBB, BBVector[i + 1]);
                    }
                }
                else
                {
                    G4_Operand* jip = lastInst->asCFInst()->getJip();
                    G4_BB_SB* targetBB = labelToBlockMap[jip->asLabel()];
                    addSIMDEdge(currBB, targetBB);
                    if (i + 1 != BBVector.size())
                    {
                        addSIMDEdge(currBB, BBVector[i + 1]);
                    }
                }
            }
            else
            {
                if (i + 1 != BBVector.size())
                {
                    addSIMDEdge(currBB, BBVector[i + 1]);
                }
            }
        }
        else
        {
            if (i + 1 != BBVector.size())
            {
                addSIMDEdge(currBB, BBVector[i + 1]);
            }
        }
    }
}

//Generate the dependence distance
void SWSB::SWSBDepDistanceGenerator(PointsToAnalysis& p, LiveGRFBuckets& LB, LiveGRFBuckets& globalSendsLB)
{
    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    unsigned numBBId = (unsigned)fg.size();

    //Initialize global data
    BBVector.resize(numBBId);

    //Set distance 1 at the first instruction in case there are runtime inserted instructions at prolog
    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_3D)
    {
        setDefaultDistanceAtFirstInstruction();
    }

    unsigned nestLoopLevel = 0;
    //Local dependence analysis
    for (; ib != bend; ++ib)
    {
        BBVector[(*ib)->getId()] = new (mem)G4_BB_SB(*(fg.builder),
            mem,
            *ib,
            &SBNodes,
            &SBSendNodes,
            &globalSendOpndList,
            &indexes,
            globalSendNum,
            &LB,
            &globalSendsLB,
            p,
            &labelToBlockMap);
        if ((*ib)->getNestLevel())
        {
            nestLoopLevel = nestLoopLevel < (*ib)->getNestLevel() ? (*ib)->getNestLevel() : nestLoopLevel;
        }
    }

    return;
}

void SWSB::handleIndirectCall()
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        if (BBVector[i]->last_node == -1)
        {
            continue;
        }

        SBNode* node = SBNodes[BBVector[i]->last_node];

        if ((node->GetInstruction()->isCall() && !node->GetInstruction()->getSrc(0)->isLabel()) ||
            node->GetInstruction()->isReturn())
        {
            LiveGRFBuckets send_use_out(mem, kernel.getNumRegTotal(), *fg.getKernel());
            for (size_t j = 0; j < globalSendOpndList.size(); j++)
            {
                SBBucketNode* sBucketNode = globalSendOpndList[j];
                SBNode* sNode = sBucketNode->node;
                if (BBVector[i]->send_live_out->isSrcSet(sNode->globalID) &&
                    (sBucketNode->opndNum == Opnd_src0 ||
                    sBucketNode->opndNum == Opnd_src1 ||
                    sBucketNode->opndNum == Opnd_src2 ||
                    sBucketNode->opndNum == Opnd_src3))
                {
                    BBVector[i]->createAddGRFEdge(sNode, node, WAR, DEP_EXPLICT);
                }
                if (BBVector[i]->send_live_out->isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
                {
                    BBVector[i]->createAddGRFEdge(sNode, node, RAW, DEP_EXPLICT);
                }
            }
        }
        if (node->GetInstruction()->isReturn())
        {
            node->GetInstruction()->setDistance(1);
        }
    }

    return;
}
void SWSB::SWSBGlobalTokenGenerator(PointsToAnalysis& p, LiveGRFBuckets& LB, LiveGRFBuckets& globalSendsLB)
{
    bool hasIndirectCall = false;
    allTokenNodesMap = (BitSet * *)mem.alloc(sizeof(BitSet*) * totalTokenNum);
    for (size_t i = 0; i < totalTokenNum; i++)
    {
        allTokenNodesMap[i] = new (mem)BitSet(unsigned(SBSendNodes.size()), false);
    }

    // Get the live out, may kill bit sets
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        if (BBVector[i]->last_node != -1)
        {
            SBNode* node = SBNodes[BBVector[i]->last_node];

            if (node->GetInstruction()->isCall() && !node->GetInstruction()->getSrc(0)->isLabel())
            {
                hasIndirectCall = true;
            }
        }

        BBVector[i]->send_live_in = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->send_live_out = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->send_def_out = new (mem)SBBitSets(mem, globalSendNum);

        BBVector[i]->send_live_in_scalar = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->send_live_out_scalar = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->send_kill_scalar = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->liveInTokenNodes = new (mem)BitSet(unsigned(SBSendNodes.size()), false);
        BBVector[i]->liveOutTokenNodes = new (mem)BitSet(unsigned(SBSendNodes.size()), false);
        BBVector[i]->killedTokens = new (mem)BitSet(totalTokenNum, false);

        if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
            fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
        {
            BBVector[i]->tokenLiveInDist = (unsigned*)mem.alloc(sizeof(unsigned) * globalSendNum);
            BBVector[i]->tokenLiveOutDist = (unsigned*)mem.alloc(sizeof(unsigned) * globalSendNum);
            for (unsigned k = 0; k < globalSendNum; k++)
            {
                BBVector[i]->tokenLiveInDist[k] = -1;
                BBVector[i]->tokenLiveOutDist[k] = -1;
            }
        }
        if (BBVector[i]->send_start != -1)
        {
            for (int k = BBVector[i]->send_start; k <= BBVector[i]->send_end; k++)
            {
                if (globalSendOpndList[k]->opndNum == Opnd_dst)
                {
                    BBVector[i]->send_def_out->setDst(globalSendOpndList[k]->node->globalID, true);
                    BBVector[i]->send_live_out->setDst(globalSendOpndList[k]->node->globalID, true);
                }
                if (globalSendOpndList[k]->opndNum == Opnd_src0 ||
                    globalSendOpndList[k]->opndNum == Opnd_src1 ||
                    globalSendOpndList[k]->opndNum == Opnd_src2 ||
                    globalSendOpndList[k]->opndNum == Opnd_src3)
                {
                    BBVector[i]->send_def_out->setSrc(globalSendOpndList[k]->node->globalID, true);
                    BBVector[i]->send_live_out->setSrc(globalSendOpndList[k]->node->globalID, true);
                }
            }
        }

        BBVector[i]->send_may_kill = new (mem)SBBitSets(mem, globalSendNum);
        BBVector[i]->send_WAW_may_kill = new (mem)BitSet(globalSendNum, false);
        BBVector[i]->setSendOpndMayKilled(&globalSendsLB, &SBNodes, p);

#ifdef DEBUG_VERBOSE_ON
        BBVector[i]->dumpLiveInfo(&globalSendOpndList, globalSendNum, nullptr);
#endif
    }

    /*
    Loop info is used to reduce the token required for certain instructions, or count the delay of the backedge for token reuse
    We do the token reduction and count delay of backedge only for the nature loops, i.e with the backedge,
    if the instruction distance is far enough, there is no need to set dependence.
    For the irreducible flow graph, those optimizations wouldn't be kicked in.
    */
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        for (auto&& be : kernel.fg.backEdges)
        {
            auto loopIt = kernel.fg.naturalLoops.find(be);

            if (loopIt != kernel.fg.naturalLoops.end())
            {
                auto&& bbsInLoop = (*loopIt).second;

                auto bb1InLoop = bbsInLoop.find(BBVector[i]->getBB());
                if (bb1InLoop != bbsInLoop.end())
                {
                    if (BBVector[i]->getLoopStartBBID() != -1)
                    {
                        //Innerest loop only
                        if (BBVector[i]->getLoopStartBBID() <= be.second->getId() &&
                            BBVector[i]->getLoopEndBBID() >= be.first->getId())
                        {
                            BBVector[i]->setLoopStartBBID(be.second->getId());
                            BBVector[i]->setLoopEndBBID(be.first->getId());
                        }
                    }
                    else
                    {
                        BBVector[i]->setLoopStartBBID(be.second->getId());
                        BBVector[i]->setLoopEndBBID(be.first->getId());
                    }
                }
            }
        }
    }

    //Global analysis until no live in change
    SWSBGlobalScalarCFGReachAnalysis();

    //Add dependence according ot analysis result
    if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
        fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
    {
        addGlobalDependenceWithReachingDef(globalSendNum, &globalSendOpndList, &SBNodes, p, true);
    }
    else
    {
        addGlobalDependence(globalSendNum, &globalSendOpndList, &SBNodes, p, true);
    }

    if (hasIndirectCall)
    {
        handleIndirectCall();
    }

    for (size_t i = 0; i < BBVector.size(); i++)
    {
        *BBVector[i]->send_live_in_scalar = *BBVector[i]->send_live_in;
        *BBVector[i]->send_live_out_scalar = *BBVector[i]->send_live_out;
    }

    SWSBBuildSIMDCFG();

    SWSBGlobalSIMDCFGReachAnalysis();

    //Add dependence according ot analysis result
    addGlobalDependence(globalSendNum, &globalSendOpndList, &SBNodes, p, false);

    //SWSB token alloation with linear scan algorithm.
    tokenProfile = new (mem)SWSB_TOKEN_PROFILE();
    if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation))
    {
        tokenAllocationGlobal();
    }
    else if (fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
    {
        tokenAllocationGlobalWithPropogation();
    }
    else
    {
        tokenAllocation();
    }

    //Insert test instruction in case the dependences are more than token field in the instruction.
    insertTest();

    return;
}

static FCPatchingInfo::RegAccessType
getRegAccessType(Gen4_Operand_Number OpndNo) {
    if (OpndNo == Opnd_dst)
        return FCPatchingInfo::Fully_Def;
    return FCPatchingInfo::Fully_Use;
}

static unsigned getRegAccessPipe(G4_INST* Inst) {
    FCPatchingInfo::RegAccessPipe Pipe = FCPatchingInfo::Pipe_ALU;
    unsigned SFID = 0;

    if (Inst->isSend())
    {
        Pipe = FCPatchingInfo::Pipe_Send;
        SFID = SFIDtoInt(Inst->getMsgDesc()->getFuncId()) & 0xF; // 4-bit SFID
    }
    else if (Inst->isMathPipeInst())
    {
        Pipe = FCPatchingInfo::Pipe_Math;
    }

    // Pipe ID is encoded as (SFID[3:0] | P[3:0]), where P is ALU, Math, or Send.
    return unsigned(Pipe) | (SFID << 4);
}

static void updateRegAccess(FCPatchingInfo* FCPI, SBNode* Node,
    Gen4_Operand_Number OpndNo, unsigned NumRegs) {
    for (auto F = Node->getFirstFootprint(OpndNo); F != nullptr; F = F->next) {
        unsigned L = F->LeftB / G4_GRF_REG_NBYTES;
        unsigned R = F->RightB / G4_GRF_REG_NBYTES;
        if (F->fType != GRF_T)
        {
            continue;
        }
        ASSERT_USER(L < NumRegs, "Invalid register left bound!");
        ASSERT_USER(R < NumRegs, "Invalid register right bound!");
        for (unsigned n = L; n <= R; ++n) {
            FCPatchingInfo::RegAccess Acc;
            Acc.Type = getRegAccessType(OpndNo);
            Acc.RegNo = n;
            Acc.Pipe = getRegAccessPipe(Node->GetInstruction());
            Acc.Inst = Node->GetInstruction();
            Acc.Token = Acc.Inst->getToken();
            // Update the first access list & map.
            if (!FCPI->RegFirstAccessMap.count(n)) {
                FCPI->RegFirstAccessList.push_back(Acc);
                FCPI->RegFirstAccessMap[n] = &FCPI->RegFirstAccessList.back();
            }
            // Update the last access list & map.
            if (FCPI->RegLastAccessMap.count(n)) {
                if (Acc.Type == FCPatchingInfo::Fully_Def) {
                    // Remove previous accesses.
                    auto PrevAcc = FCPI->RegLastAccessMap[n];
                    while (PrevAcc) {
                        auto Next = PrevAcc->Next;
                        auto PrevAccInst = PrevAcc->Inst;
                        auto PrevAccRegNo = PrevAcc->RegNo;
                        // Remove all previous accesses on the same GRF.
                        FCPI->RegLastAccessList.remove_if(
                            [=](const FCPatchingInfo::RegAccess& A) {
                            return (A.Inst == PrevAccInst) &&
                                (A.RegNo == PrevAccRegNo); });
                        PrevAcc = Next;
                    }
                }
                else {
                    // Remove previous accesses with the same pipe.
                    auto PrevAcc = FCPI->RegLastAccessMap[n];
                    while (PrevAcc) {
                        if (PrevAcc->Type == FCPatchingInfo::Fully_Use &&
                            PrevAcc->Pipe != Acc.Pipe) {
                            // Not the same, re-link them.
                            std::swap(Acc.Next, PrevAcc->Next);
                            std::swap(Acc.Next, PrevAcc);
                            continue;
                        }
                        auto Next = PrevAcc->Next;
                        auto PrevAccInst = PrevAcc->Inst;
                        auto PrevAccRegNo = PrevAcc->RegNo;
                        // Remove all previous accesses on the same GRF and the same pipe.
                        FCPI->RegLastAccessList.remove_if(
                            [=](const FCPatchingInfo::RegAccess& A) {
                            return (A.Inst == PrevAccInst) &&
                                (A.RegNo == PrevAccRegNo); });
                        PrevAcc = Next;
                    }
                }
            }
            FCPI->RegLastAccessList.push_back(Acc);
            FCPI->RegLastAccessMap[n] = &FCPI->RegLastAccessList.back();
        }
    }
}

static void insertSyncBarrier(FCPatchingInfo* FCPI, SBNode* Node,
    unsigned NumRegs) {
    // Skip if sync barrier is already inserted.
    if (FCPI->RegFirstAccessList.size() == 0 || FCPI->RegFirstAccessList.back().RegNo == unsigned(-1))
        return;

    // Sync barrier is a special relocation where all registers are forced to be
    // synchronized.
    FCPatchingInfo::RegAccess Acc;
    Acc.Type = FCPatchingInfo::Fully_Use;
    Acc.RegNo = unsigned(-1); // A special register.
    // Sync barrier is inserted just before this instruction.
    Acc.Inst = Node->GetInstruction();

    // Append this access into the first access list.
    FCPI->RegFirstAccessList.push_back(Acc);
    // Update the first access map.
    for (unsigned n = 0; n < NumRegs; ++n) {
        if (FCPI->RegFirstAccessMap.count(n))
            continue;
        FCPI->RegFirstAccessMap[n] = &FCPI->RegFirstAccessList.back();
    }
    // Invalidate the last access list & map.
    FCPI->RegLastAccessMap.clear();
    FCPI->RegLastAccessList.clear();
}

static bool isBranch(SBNode* N) {
    auto Inst = N->GetInstruction();
    if (!Inst->isFlowControl())
        return false;
    // Skip function call/ret.
    if (Inst->isCall() || Inst->isReturn() ||
        Inst->opcode() == G4_pseudo_fc_call ||
        Inst->opcode() == G4_pseudo_fc_ret)
        return false;
    return true;
}

static void updatePatchInfo(FCPatchingInfo* FCPI, SBNode* Node,
    unsigned NumRegs, unsigned NumTokens) {
    // TODO: Branch is not supported in the current FC patch info as it
    // involves complicated handling. Issue a sync barrier just before the
    // first flow control instruction.
    if (isBranch(Node)) {
        insertSyncBarrier(FCPI, Node, NumRegs);
        return;
    }
    // Update access maps.
    updateRegAccess(FCPI, Node, Opnd_src0, NumRegs);
    updateRegAccess(FCPI, Node, Opnd_src1, NumRegs);
    updateRegAccess(FCPI, Node, Opnd_src2, NumRegs);
    // Per inst, 'use' access always happens before 'def' access.
    updateRegAccess(FCPI, Node, Opnd_dst, NumRegs);
}

static void updateTokenSet(FCPatchingInfo* FCPI, SBNODE_VECT& Nodes,
    unsigned NumTokens) {
    std::set<G4_INST*> LastAccInsts;
    // Collect last access instructions.
    for (auto I = FCPI->RegLastAccessList.begin(),
        E = FCPI->RegLastAccessList.end(); I != E; ++I) {
        LastAccInsts.insert(I->Inst);
    }
    // Scan node for tokens used in non-last access instructions.
    for (auto NI = Nodes.begin(), NE = Nodes.end(); NI != NE; ++NI) {
        auto Inst = (*NI)->GetInstruction();
        if (LastAccInsts.count(Inst))
            continue;
        auto T = Inst->getToken();
        // Skip if token is not allocated.
        if (T == (unsigned short)(-1))
            return;
        ASSERT_USER(T < NumTokens, "Invalid token number!");
        FCPI->AllocatedToken.insert(T);
    }
}

void SWSB::genSWSBPatchInfo() {
    unsigned NumRegs = kernel.getNumRegTotal();
    auto FCPI = fg.builder->getFCPatchInfo();
    for (auto NI = SBNodes.begin(), NE = SBNodes.end(); NI != NE; ++NI) {
        auto Node = *NI;
        updatePatchInfo(FCPI, Node, NumRegs, totalTokenNum);
    }

#if 1
    //Update the live out tokens according to the live out of the exit BB of the kernel.
    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    for (; ib != bend; ++ib)
    {
        G4_BB* bb = (*ib);
        if (bb->Succs.size() == 0 &&
            BBVector[bb->getId()]->Succs.size() == 0)
        {
            LiveGRFBuckets send_use_out(mem, kernel.getNumRegTotal(), *fg.getKernel());
            for (size_t i = 0; i < globalSendOpndList.size(); i++)
            {
                SBBucketNode* sBucketNode = globalSendOpndList[i];
                SBNode* sNode = sBucketNode->node;
                if (BBVector[bb->getId()]->send_live_out->isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                    sBucketNode->opndNum == Opnd_src1 ||
                    sBucketNode->opndNum == Opnd_src2 ||
                    sBucketNode->opndNum == Opnd_src3))
                {
                    BBVector[bb->getId()]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_out);
                }
                if (BBVector[bb->getId()]->send_live_out->isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
                {
                    BBVector[bb->getId()]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_out);
                }
            }

            for (unsigned curBucket = 0; curBucket < kernel.getNumRegTotal(); curBucket++)
            {
                for (LiveGRFBuckets::BN_iterator bn_it = send_use_out.begin(curBucket);
                    bn_it != send_use_out.end(curBucket); ++bn_it)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;

                    FCPatchingInfo::RegAccess Acc;
                    Acc.Type = getRegAccessType(liveOpnd);
                    Acc.RegNo = curBucket;
                    Acc.Pipe = getRegAccessPipe(curLiveNode->GetInstruction());
                    Acc.Inst = curLiveNode->GetInstruction();
                    Acc.Token = Acc.Inst->getToken();
                    FCPI->RegLastAccessList.push_back(Acc);
                    FCPI->RegLastAccessMap[curBucket] = &FCPI->RegLastAccessList.back();
                }
            }
        }
    }
#endif

    updateTokenSet(FCPI, SBNodes, totalTokenNum);

#if defined(DEBUG_VERBOSE_ON)
    // First access.
    std::cerr << "FirstAccess:\n";
    auto& FirstAccess = FCPI->RegFirstAccessList;
    for (auto I = FirstAccess.begin(), E = FirstAccess.end(); I != E; ++I) {
        auto& Access = *I;
        fprintf(stderr, "r%03u.%s", Access.RegNo,
            (Access.Type == FCPatchingInfo::Fully_Def ? "def" : "use"));
        fprintf(stderr, ", P%04x", Access.Pipe);
        if (Access.Token != (unsigned short)(-1))
            fprintf(stderr, ", $%u", Access.Token);
        fprintf(stderr, ":");
        Access.Inst->dump();
    }
    // Last access.
    std::cerr << "LastAccess:\n";
    auto& LastAccess = FCPI->RegLastAccessList;
    for (auto I = LastAccess.begin(), E = LastAccess.end(); I != E; ++I) {
        auto& Access = *I;
        fprintf(stderr, "r%03u.%s", Access.RegNo,
            (Access.Type == FCPatchingInfo::Fully_Def ? "def" : "use"));
        fprintf(stderr, ", P%04x", Access.Pipe);
        if (Access.Token != (unsigned short)(-1))
            fprintf(stderr, ", $%u", Access.Token);
        fprintf(stderr, ":");
        Access.Inst->dump();
    }
    // Allocated token.
    std::cerr << "AllocatedToken:\n";
    for (unsigned t = 0; t != NumTokens; ++t) {
        if (!FCPI->AllocatedToken.count(t))
            continue;
        if (t != 0)
            fprintf(stderr, ", ");
        fprintf(stderr, "$%u", t);
    }
    fprintf(stderr, "\n");
#endif
}

void SWSB::getDominators(Dom* dom)
{
    //BBVector[bb->getId()]->tokenAssigned = true;
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (size_t i = 0; i < BBVector.size(); i++)
        {
            BitSet currDoms = (*BBVector[i]->dominators);
            if (dom->iDoms[i] != BBVector[i]->getBB())
            {
                currDoms |= (*BBVector[dom->iDoms[i]->getId()]->dominators);
            }

            if (currDoms != (*BBVector[i]->dominators))
            {
                changed = true;
                (*BBVector[i]->dominators) = currDoms;
            }
        }
    }

    return;
}

//
//Entry to the software scoreboard generator
//
void SWSB::SWSBGenerator()
{
    DEBUG_VERBOSE("[SWSB]: Starting...");
    PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
    p.doPointsToAnalysis(kernel.fg);

    //For VISA_3D, the naturalLoop finding is handled in register allocation already
    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_3D)
    {
        kernel.fg.findNaturalLoops();
    }

    //Note that getNumFlagRegisters() treat each 16 bits as a flag register
    LiveGRFBuckets LB(mem, kernel.getNumRegTotal() + kernel.getNumAcc() + fg.builder->getNumFlagRegisters(), kernel);
    LiveGRFBuckets globalSendsLB(mem, kernel.getNumRegTotal() + kernel.getNumAcc() + fg.builder->getNumFlagRegisters(), kernel);

    SWSBDepDistanceGenerator(p, LB, globalSendsLB);

#ifdef DEBUG_VERBOSE_ON
    dumpDepInfo();
#endif

    if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
        fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
    {
        Dom dom(kernel, mem);
        dom.runIDOM();  //We can use dom.runDOM() as well, which compute the dominance first. However, it has issue with function call.

        //Build dom tree
        for (size_t i = 0; i < BBVector.size(); i++)
        {
            G4_BB* bb = BBVector[i]->getBB();
            BBVector[i]->dominators = new (mem)BitSet(BBVector.size(), false);
            BBVector[i]->dominators->set(i, true);

            if (dom.iDoms[bb->getId()] != bb)
            {
                BBVector[dom.iDoms[bb->getId()]->getId()]->domSuccs.push_back(BBVector[i]);
                BBVector[i]->domPreds.push_back(BBVector[dom.iDoms[bb->getId()]->getId()]);
            }
        }

        for (size_t i = 0; i < BBVector.size(); i++)
        {
            if (BBVector[i]->domSuccs.size())
            {
                BBVector[i]->domSuccs.sort(compareBBStart);
            }
        }

        getDominators(&dom);
#ifdef DEBUG_VERBOSE_ON
        dumpImmDom(&dom);
#endif
    }

    SWSBGlobalTokenGenerator(p, LB, globalSendsLB);

    genSWSBPatchInfo();

#ifdef DEBUG_VERBOSE_ON
    std::cerr << "\n" << "Dependence Graph:" << "\n";

    for (SBNODE_VECT_ITER node_it = SBNodes.begin();
        node_it != SBNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        G4_INST* inst = node->GetInstruction();
        std::cerr << node->getNodeID() << ":\t";
        inst->dump();
        std::cerr << "Succs:";
        for (int i = 0; i < (int)(node->succs.size()); i++)
        {
            SBDEP_ITEM& curSucc = node->succs[i];
            std::cerr << curSucc.node->getNodeID() << ",";
        }
        std::cerr << "\n";
        std::cerr << "Preds:";
        for (int i = 0; i < (int)(node->preds.size()); i++)
        {
            SBDEP_ITEM& curPred = node->preds[i];
            std::cerr << curPred.node->getNodeID() << ",";
        }
        std::cerr << "\n\n";
    }
#endif

    return;
}

unsigned SWSB::getDepDelay(SBNode* curNode)
{
    int reuseDelay = 0;

    if (curNode->GetInstruction()->isSend())
    {
        G4_SendMsgDescriptor* msgDesc = curNode->GetInstruction()->getMsgDesc();

        if (msgDesc->isSLMMessage())
        {
            reuseDelay = TOKEN_AFTER_WRITE_SEND_SLM_CYCLE;
        }
        else if (msgDesc->isSampler())
        {
            reuseDelay = TOKEN_AFTER_WRITE_SEND_SAMPLER_CYCLE;
        }
        else
        {
            reuseDelay = TOKEN_AFTER_WRITE_SEND_MEMORY_CYCLE;
        }
    }
    else if (curNode->GetInstruction()->isMathPipeInst())
    {

        reuseDelay = TOKEN_AFTER_WRITE_MATH_CYCLE;
    }
    else
    {
        assert(0 && "unexpected token reuse instruction");
    }

    return reuseDelay;
}

#define LOOP_FACTOR_FOR_TOAKE_REUSE  5
//The algorithm for reuse selectoin: The live range which causes the least stall delay of current live range.
//Fixme: for global variable, it's not accurate. Because the AFTER_SOURCE and AFTER_WRITE may in different branches.
//Try not reuse the tokens set in adjacent instructions.
SBNode * SWSB::reuseTokenSelection(SBNode * node)
{
    int delay = TOKEN_AFTER_WRITE_SEND_SAMPLER_CYCLE;
    int distance = 0;
    unsigned nodeID = node->getNodeID();
    unsigned nodeDelay = getDepDelay(node);
    unsigned char nestLoopLevel = BBVector[node->getBBID()]->getBB()->getNestLevel();
    unsigned loopStartBB = BBVector[node->getBBID()]->getLoopStartBBID();
    unsigned loopEndBB = BBVector[node->getBBID()]->getLoopEndBBID();
    unsigned loopStartID = -1;
    unsigned loopEndID = -1;

    assert(linearScanLiveNodes.size() <= totalTokenNum);

    SBNode* candidateNode = *linearScanLiveNodes.begin();
    for (SBNODE_LIST_ITER node_it = linearScanLiveNodes.begin();
        node_it != linearScanLiveNodes.end();
        node_it++)
    {
        SBNode* curNode = (*node_it);
        int reuseDelay = 0;
        int curDistance = 0;
        unsigned curNodeDelay = getDepDelay(curNode);
        unsigned char curNodeNestLoopLevel = BBVector[curNode->getBBID()]->getBB()->getNestLevel();
        unsigned short token = curNode->getLastInstruction()->getToken();

        if (nodeID > curNode->getNodeID())
        {
            reuseDelay = curNodeDelay - (nodeID - curNode->getNodeID());
            if (reuseDelay < 0)
            {
                curDistance = nodeID - curNode->getNodeID();
            }
        }
        else
        {
            reuseDelay = nodeDelay - (curNode->getNodeID() - nodeID);
            if (reuseDelay < 0)
            {
                curDistance = curNode->getNodeID() - nodeID;
            }
        }
        unsigned loopLevelDiff = curNodeNestLoopLevel > nestLoopLevel ? curNodeNestLoopLevel - nestLoopLevel : nestLoopLevel - curNodeNestLoopLevel;
        if (reuseDelay > 0)
        {
            reuseDelay = reuseDelay / (LOOP_FACTOR_FOR_TOAKE_REUSE * loopLevelDiff + 1);
        }
        else
        {
            curDistance = (LOOP_FACTOR_FOR_TOAKE_REUSE * loopLevelDiff + 1) * curDistance;
            if (nestLoopLevel && loopLevelDiff == 0)
            {
                unsigned curLoopStartBB = loopStartBB;
                unsigned curLoopEndBB = loopEndBB;
                if (curLoopStartBB == -1 || curLoopEndBB == -1)
                {
                    curLoopStartBB = BBVector[curNode->getBBID()]->getLoopStartBBID();
                    curLoopEndBB = BBVector[curNode->getBBID()]->getLoopEndBBID();
                }
                //Count the backedge, if the backedge distance is short, take it
                if (curLoopStartBB != -1 && curLoopEndBB != -1)
                {
                    loopStartID = BBVector[curLoopStartBB]->first_node;
                    loopEndID = BBVector[curLoopEndBB]->last_node;
                    int backEdgeDistance = loopEndID - loopStartID - curDistance;
                    if (curDistance > backEdgeDistance)
                    {
                        curDistance = backEdgeDistance;
                    }
                }
            }
        }

        for (SBNODE_LIST_ITER sn_it = sameTokenNodes[token].begin();
            sn_it != sameTokenNodes[token].end();
            sn_it++)
        {
            SBNode* snode = (*sn_it);
            unsigned sNodeDelay = getDepDelay(snode);
            unsigned char sNodeNestLoopLevel = BBVector[snode->getBBID()]->getBB()->getNestLevel();

            int sReuseDelay = 0;
            int sDistance = 0;
            if (nodeID > snode->getNodeID())
            {
                sReuseDelay = sNodeDelay - (nodeID - snode->getNodeID());
                if (sReuseDelay < 0)
                {
                    sDistance = nodeID - snode->getNodeID();
                }
            }
            else
            {
                sReuseDelay = nodeDelay - (snode->getNodeID() - nodeID);
                if (sReuseDelay < 0)
                {
                    sDistance = snode->getNodeID() - nodeID;
                }
            }

            unsigned loopLevelDiff = sNodeNestLoopLevel > nestLoopLevel ? sNodeNestLoopLevel - nestLoopLevel : nestLoopLevel - sNodeNestLoopLevel;
            if (sReuseDelay > 0)
            {
                sReuseDelay = sReuseDelay / (loopLevelDiff * LOOP_FACTOR_FOR_TOAKE_REUSE + 1);
            }
            else
            {
                sDistance = (LOOP_FACTOR_FOR_TOAKE_REUSE * loopLevelDiff + 1) * sDistance;
                if (nestLoopLevel && loopLevelDiff == 0)
                {
                    unsigned curLoopStartBB = loopStartBB;
                    unsigned curLoopEndBB = loopEndBB;
                    if (curLoopStartBB == -1 || curLoopEndBB == -1)
                    {
                        curLoopStartBB = BBVector[curNode->getBBID()]->getLoopStartBBID();
                        curLoopEndBB = BBVector[curNode->getBBID()]->getLoopEndBBID();
                    }
                    //Count the backedge, if the backedge distance is short, take it
                    if (curLoopStartBB != -1 && curLoopEndBB != -1)
                    {
                        loopStartID = BBVector[curLoopStartBB]->first_node;
                        loopEndID = BBVector[curLoopEndBB]->last_node;
                        int backEdgeDistance = loopEndID - loopStartID - sDistance;
                        if (sDistance > backEdgeDistance)
                        {
                            sDistance = backEdgeDistance;
                        }
                    }
                }
            }

            //Get the largest delay for the token node
            if (sReuseDelay > reuseDelay)
            {
                reuseDelay = sReuseDelay;
            }

            if (sDistance < curDistance)
            {
                curDistance = sDistance;
            }
        }

        //Smallest one is the best one
        //if Distance is not 0, count the distance, otherwise, use the delay.
        //Distance is not 0 means there are  candidate whose distance is large than the delay
        if (!distance && reuseDelay > 0)
        {
            if (reuseDelay < delay)
            {
                delay = reuseDelay;
                candidateNode = curNode;
            }
        }
        else if (curDistance > distance)
        {
            distance = curDistance;
            candidateNode = curNode;
        }
    }

    return candidateNode;
}

/*
 * If the cycles of the instruction which occupied
*/
bool  SWSB::cycleExpired(SBNode* node, int currentID)
{
    if (node->GetInstruction()->isSend())
    {
        G4_SendMsgDescriptor* msgDesc = node->GetInstruction()->getMsgDesc();

        if (msgDesc->isSLMMessage())
        {
            return TOKEN_AFTER_WRITE_SEND_SLM_CYCLE <= (currentID - node->getLiveStartID());
        }
        else if (msgDesc->isSampler())
        {
            return TOKEN_AFTER_WRITE_SEND_SAMPLER_CYCLE <= (currentID - node->getLiveStartID());
        }
        else
        {
            return TOKEN_AFTER_WRITE_SEND_MEMORY_CYCLE <= (currentID - node->getLiveStartID());
        }
    }
    else if (node->GetInstruction()->isMathPipeInst())
    {
        return TOKEN_AFTER_WRITE_MATH_CYCLE <= (currentID - node->getLiveStartID());
    }
    else
    {
        assert(0 && "unexpected token reuse instruction");
    }

    return true;
}

//
// Token depndence reduction is trying to reduce the unncessary dependence when token reuse happens
// Such as in following case
//  1.  send r20,...           { $0 }
//      ...
//  20. send r30, ...         { $0 }
//  21. add  r40 r20  r60    { $0.dst }
// There is no need to set dependence for instruction 21,
// because the reuse gaurantee the dependency from instruction 1 is resolved before token 0 can be reused.
// FIXME: Dominator info is required for global reduction
//
void SWSB::tokenDepReduction(SBNode* n1, SBNode* n2)
{
    SBNode* node1 = n1;
    SBNode* node2 = n2;

    assert(node1 != node2);
    if (n1->getNodeID() > n2->getNodeID())
    {
        node1 = n2;
        node2 = n1;
    }

    if (!fg.builder->getOptions()->getOption(vISA_SWSBDepReduction))
    {
        unsigned node1BBID = node1->getBBID();
        unsigned node2BBID = node2->getBBID();

        for (auto node_it = node1->succs.begin();
            node_it != node1->succs.end();
            )
        {
            SBDEP_ITEM& curSucc1 = (*node_it);
            SBNode* succ1 = curSucc1.node;
            unsigned bbID1 = succ1->getBBID();

            //node1(previous) and node2(current) are in same BB: kill all live out of node1
            // BB:
            //     node1
            //     node2
            //
            //Or the succ of node1 and node 2 are in same BB: kill all succ of node1 which after node 2
            // FIXME: will this one conflict with global dependence reduction?
            //BB:
            //     node2
            //     succ(node1)
            if ((node1BBID == node2BBID && bbID1 != node2BBID) ||
                (node1BBID != node2BBID && bbID1 == node2BBID && succ1->getNodeID() > node2->getNodeID()))
            {
                node_it = node1->succs.erase(node_it);//FIXME, if the succ is the token instruction, do we need free the tokens assigned to the instruction because of the dependence
                continue;
            }

            //When two sucessors are in same BB, previous one kill the following one
            // FIXME: This may not be good, because the policy is trying to keep the longest dependence and move the short one
            // Of course, if the two predecssors are lived in from different branch, we can only kill the longer one
            bool killed = false;
            for (auto node2_it = node2->succs.begin();
                node2_it != node2->succs.end();
                )
            {
                SBDEP_ITEM& curSucc2 = (*node2_it);
                SBNode* succ2 = curSucc2.node;
                unsigned bbID2 = succ2->getBBID();

                if (bbID1 == bbID2 &&
                    bbID1 != node1BBID &&
                    bbID2 != node2BBID &&
                    succ2 != succ1)
                {
                    //succ2 is ahead
                    if (succ1->getNodeID() > succ2->getNodeID())
                    {
                        if (curSucc2.attr == DEP_EXPLICT &&
                            (curSucc1.type == curSucc2.type ||
                                curSucc2.type == RAW ||
                                curSucc2.type == WAW))
                        {
                            //succ1 killed
                            killed = true;
                            break;
                        }
                    }
                    else
                    {
                        if (curSucc1.attr == DEP_EXPLICT &&
                            (curSucc1.type == curSucc2.type ||
                                curSucc1.type == RAW ||
                                curSucc1.type == WAW))
                        {
                            node2_it = node2->succs.erase(node2_it);
                            continue;
                        }
                    }
                }
                node2_it++;
            }

            if (killed)
            {
                node_it = node1->succs.erase(node_it);
                continue;
            }

            node_it++;
        }

        //The succs of node2 in same BB as node1 and is behind node1
        for (auto node_it = node2->succs.begin();
            node_it != node2->succs.end();
            )
        {
            SBDEP_ITEM& curSucc2 = (*node_it);
            SBNode* succ2 = curSucc2.node;
            unsigned bbID2 = succ2->getBBID();

            if ((node1BBID != node2BBID && bbID2 == node1BBID && succ2->getNodeID() > node1->getNodeID()))
            {
                node_it = node2->succs.erase(node_it);
                continue;
            }

            node_it++;
        }
    }

    n2->setLiveLatestID(n1->getLiveEndID(), n1->getLiveEndBBID());
    linearScanLiveNodes.remove(n1);

#ifdef DEBUG_VERBOSE_ON
    printf("remove token 1: %d\n", n1->getLastInstruction()->getToken());
#endif
    return;
}

/*
*
*  We need cycle based expieration because for the case like
*  send   null, r2...      {$0}
*  add  r2                 {$0.src}
*  send   r20   r9...      {$0}
*  The second send should not be assigned with $0.
*  In compiler, if the live range of the r2 is end in the second instruction, token $0 is treated as free.
*  However, the SBID $0 will cleared only when the instruction finished the execution.
*  Assgined the same token to the third instruction will cause a long latency.
*  We delay the end of the lives of the interverls until the cycles are all consumed, so that the token will not be assigned immediately.
*  But if the dependence is .dst dependence, the live range is over. The stall will be going until the finish of the instruction.
*
*/
void SWSB::expireIntervals(unsigned startID)
{
    for (SBNODE_LIST_ITER node_it = linearScanLiveNodes.begin();
        node_it != linearScanLiveNodes.end();
        )
    {
        SBNode* curNode = (*node_it);
        if (curNode->getLiveEndID() <= startID)
        {
            SBNode* node = (*linearScanLiveNodes.begin());
            if (node->hasAWDep() || cycleExpired(node, startID))
            {
                unsigned short token = node->getLastInstruction()->getToken();

                assert(token != (unsigned short)-1);
                node_it = linearScanLiveNodes.erase(node_it);
#ifdef DEBUG_VERBOSE_ON
                printf("remove token %d:\n", token);
#endif
                //Remove token to free list
                freeTokenList[token] = nullptr;
                if (topIndex == -1)
                {
                    topIndex = token;
                }
                continue;
            }
        }
        else
        {
            break;
        }
        node_it++;
    }
}

//GraphColoring can provide a more accurate version.
//For linear scan, only if the instruction is not assigned before can be used for this OPT.
//This is to avoid the false token sharing.
//What's the impact on the token reduction?
//Token reduction will remove the succ, i.e remove the dependence.
//NOTE THAT: Token reducton happens only when run out of token.
void SWSB::shareToken(SBNode* node, SBNode* succ, unsigned short token)
{
    if (node->getBBID() == succ->getBBID())
    {
        return;
    }

    for (auto node_it = succ->preds.begin();
        node_it != succ->preds.end();
        node_it++)
    {
        SBDEP_ITEM& curPred = (*node_it);
        SBNode* succPred = curPred.node;

        if (node->getBBID() != succPred->getBBID() &&
            succPred->getLastInstruction()->getToken() == (unsigned short)UNKNOWN_TOKEN &&
            tokenHonourInstruction(succPred->getLastInstruction()))
        {
            G4_BB_SB* curBB = BBVector[node->getBBID()];
            G4_BB_SB* succPredBB = BBVector[succPred->getBBID()];
            //FIXME: Only define BBs comparision is not enough. It may cause extra delay?
            if (!(curBB->send_live_in->isDstSet((unsigned)succPred->globalID) ||
                curBB->send_live_in->isSrcSet((unsigned)succPred->globalID) ||
                succPredBB->send_live_in->isDstSet((unsigned)node->globalID) ||
                succPredBB->send_live_in->isSrcSet((unsigned)node->globalID)
                ))
            {
                succPred->getLastInstruction()->setToken(token);
            }
        }
    }

    return;
}

void SWSB::assignDepToken(SBNode* node)
{
    unsigned short token = node->getLastInstruction()->getToken();
    assert(token != (unsigned short)-1 && "Failed to add token dependence to the node without token");

    //Set the dependent tokens for successors of current send
    //Remove the unnecessary dependent tokens in same BB, this work can be done when adding the edge,
    //However, since that's the bucket based, and is harder to do sorting for different GRF dependence
    //
    //1. Send r2-r5, r8, ....    $1
    //   ...
    //7. Add  r8, r16, r10   test $1S
    //8. Add  r12, r4, r14   test $1D
    //If WAR first as shown in instruction 7, we still need keep dependence for 8.
    //
    //1. Send r2-r5, r8, ....    $1
    //   ...
    //7. Add  r12, r4, r14   test $1D
    //8. Add  r8, r16, r10
    //Instead, if RAW happens first as shown in instruction 7, there is NO need for 8.

    for (auto node_it = node->succs.begin();
        node_it != node->succs.end();
        node_it++)
    {
        SBDEP_ITEM& curSucc = (*node_it);
        SBNode* succ = curSucc.node;
        DepType type = curSucc.type;
        SBDependenceAttr attr = curSucc.attr;

        if (attr == DEP_IMPLICIT)
        {
            continue;
        }

        //Same token,reuse happened, no need to set dep token
        if (tokenHonourInstruction(succ->getLastInstruction()) &&
            succ->getLastInstruction()->getToken() == token && (succ->instVec.size() <= 1)) //If the node size, the token reuse cannot guard the last instruction.
        {
            continue;
        }

        //set dependence token if live
        SWSBTokenType tokenType = type == WAR ? SWSBTokenType::AFTER_READ : SWSBTokenType::AFTER_WRITE;
        succ->GetInstruction()->setDepToken(token, tokenType);
#ifdef DEBUG_VERBOSE_ON
        dumpSync(node, succ, token, tokenType);
#endif
    }

    return;
}

void SWSB::assignToken(SBNode* node,
    unsigned short assignedToken,
    uint32_t& AWtokenReuseCount,
    uint32_t& ARtokenReuseCount,
    uint32_t& AAtokenReuseCount)
{
    unsigned short token = (unsigned short)UNKNOWN_TOKEN;

    if (assignedToken == (unsigned short)UNKNOWN_TOKEN)
    {
        //Get token
        if (topIndex != -1)
        {
            //Have free token
            token = topIndex;
            sameTokenNodes[token].push_back(node);
            freeTokenList[token] = node; //Cannot be moved after setTopTokenIndex();
            setTopTokenIndex();
#ifdef DEBUG_VERBOSE_ON
            printf("Use free token: %d, QUEUE SIZE: %d\n", token, linearScanLiveNodes.size());
#endif
        }
        else
        {
            //Have no free, use the oldest
            SBNode* oldNode = reuseTokenSelection(node);
            token = oldNode->getLastInstruction()->getToken();
            tokenDepReduction(oldNode, node);
            freeTokenList[token] = node;
#ifdef DEBUG_VERBOSE_ON
            printf("Reuse token: %d,  current: %d  %d, reuse: %d  %d, QUEUE SIZE: %d\n", token, node->getSendID(), node->getNodeID(), oldNode->getSendID(), oldNode->getNodeID(), linearScanLiveNodes.size());
#endif
            tokenReuseCount++;
            if (oldNode->hasAWDep())
            {
                AWtokenReuseCount++;
            }
            else if (oldNode->hasARDep())
            {
                ARtokenReuseCount++;
            }
            else
            {
                AAtokenReuseCount++;
            }
            node->setTokenReuseNode(oldNode);
        }
    }
    else
    {
        //This reuse pred node may have been reused already
        //When it is in short of free SBID. So, it's may not in the active list.
        token = assignedToken;
        if (freeTokenList[token] != nullptr)
        { //If the end of predecessor node is current node, the pred node may have expired already. Otherwise do reduction
            SBNode* pred = freeTokenList[token];
            tokenDepReduction(pred, node);
        }
        freeTokenList[token] = node;
        if (topIndex == token)
        {
            setTopTokenIndex();
        }
#ifdef DEBUG_VERBOSE_ON
        printf("Reuse token: %d,  QUEUE SIZE: %d\n", token, linearScanLiveNodes.size());
#endif
    }
    sameTokenNodes[token].push_back(node);
#ifdef DEBUG_VERBOSE_ON
    printf("Assigned token: %d,  node: %d, send: %d,  QUEUE SIZE: %d\n", token, node->getNodeID(), node->getSendID(), linearScanLiveNodes.size());
#endif

    //Set token to send
    node->getLastInstruction()->setToken(token);
    //For token reduction
    allTokenNodesMap[token]->set(node->sendID, true);

    //Sort succs according to the BBID and node ID.
    std::sort(node->succs.begin(), node->succs.end(), nodeSortCompare);
    for (auto node_it = node->succs.begin();
        node_it != node->succs.end();
        )
    {
        SBDEP_ITEM& curSucc = (*node_it);
        SBNode* succ = curSucc.node;
        SBDependenceAttr attr = curSucc.attr;

        if (attr == DEP_IMPLICIT)
        {
            node_it++;
            continue;
        }

        // In the case like following
        //  1. math.rsqrt   r20 r10           { $1 }
        //  2. math.in      r50  r20          { $1 }
        //  3. mul          r60 r50 r40       { $1.dst }
        if (tokenHonourInstruction(succ->getLastInstruction()))
        {
            unsigned distance = succ->getSendID() > node->getSendID() ? succ->getSendID() - node->getSendID() : node->getSendID() - succ->getSendID();
            if ((fg.builder->getOptions()->getOption(vISA_EnableISBIDBUNDLE) ||
                distance < totalTokenNum))
            {
                if ((curSucc.type == RAW || curSucc.type == WAW) &&
                    succ->getLastInstruction()->getToken() == (unsigned short)UNKNOWN_TOKEN)
                {
                    {
                        succ->getLastInstruction()->setToken(token);
                        node->setLiveLatestID(succ->getLiveEndID(), succ->getLiveEndBBID());
                        allTokenNodesMap[token]->set(node->sendID, true);
                        succ->setTokenReuseNode(node);
                        continue;
                    }
                }
            }
        }

        node_it++;
    }

    return;
}

void SWSB::addToLiveList(SBNode* node)
{
    bool insert = false;
    assert(linearScanLiveNodes.size() < totalTokenNum);
    for (SBNODE_LIST_ITER node_it = linearScanLiveNodes.begin();
        node_it != linearScanLiveNodes.end();
        node_it++)
    {
        SBNode* curNode = (*node_it);

        //Sort according to the ascending of the end ID.
        if (curNode->getLiveEndID() > node->getLiveEndID())
        {
            linearScanLiveNodes.insert(node_it, node);
            insert = true;
            break;
        }
        else if (curNode->getLiveEndID() == node->getLiveEndID())
        {
            if (curNode->getLiveStartID() > node->getLiveStartID())
            {
                linearScanLiveNodes.insert(node_it, node);
                insert = true;
                break;
            }
            else if (curNode->getLiveStartID() == node->getLiveStartID())
            {
                if (curNode->getNodeID() > node->getNodeID())
                {
                    linearScanLiveNodes.insert(node_it, node);
                    insert = true;
                    break;
                }
            }
        }
    }

    if (!insert)
    {
        linearScanLiveNodes.push_back(node);
    }

    unsigned usedToken = 0;
    for (size_t i = 0; i < freeTokenList.size(); i++)
    {
        if (freeTokenList[i] != nullptr)
        {
            usedToken++;
        }
    }
    assert(usedToken == linearScanLiveNodes.size());

#ifdef DEBUG_VERBOSE_ON
    printf("Add token: %d\n", node->getLastInstruction()->getToken());
#endif
    return;
}

//
//  Global reaching define analysis for tokens
//
bool SWSB::globalTokenReachAnalysis(G4_BB* bb)
{
    bool changed = false;
    unsigned bbID = bb->getId();

    // Do nothing for the entry BB
    // Because it has no live in
    if (bb->Preds.empty())
    {
        return false;
    }

    assert(BBVector[bbID]->liveInTokenNodes != nullptr);

    BitSet temp_live_in(unsigned(SBSendNodes.size()), false);
    temp_live_in = *BBVector[bbID]->liveInTokenNodes;

    //Union all of out of SIMDCF predecessor BB to the live in of current BB.
    for (BB_SWSB_LIST_ITER it = BBVector[bbID]->Preds.begin(); it != BBVector[bbID]->Preds.end(); it++)
    {
        G4_BB_SB* predBB = (*it);
        unsigned predID = predBB->getBB()->getId();
        temp_live_in |= *(BBVector[predID]->liveOutTokenNodes);
    }

    //Union all of out of scalar predecessor BB to the live in of current BB.
    for (BB_LIST_ITER it = bb->Preds.begin(); it != bb->Preds.end(); it++)
    {
        G4_BB* predBB = (*it);
        unsigned predID = predBB->getId();
        temp_live_in |= *(BBVector[predID]->liveOutTokenNodes);
    }

    //Changed? Yes, get the new live in, other wise do nothing
    if (temp_live_in != *BBVector[bbID]->liveInTokenNodes)
    {
        changed = true;
        *BBVector[bbID]->liveInTokenNodes = temp_live_in;
    }

    //Caculate the live out according to the live in and killed tokens in current BB
    for (uint32_t token = 0; token < totalTokenNum; token++)
    {
        if (BBVector[bbID]->killedTokens->isSet(token))
        {
            temp_live_in -= *allTokenNodesMap[token];
        }
    }

    //Get the new live out,
    //FIXME: is it right? the live out is always assigned in increasing.
    //Original, we only have local live out.
    //should we seperate the local ive out vs total live out?
    //Not necessary, can live out, will always be live out.
    *BBVector[bbID]->liveOutTokenNodes |= temp_live_in;

    return changed;
}

void SWSB::SWSBGlobalTokenAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        BB_LIST::iterator it = fg.begin();
        do
        {
            if (globalTokenReachAnalysis((*it)))
            {
                change = true;
            }

            ++it;

        } while (it != fg.end());
    }
}

void SWSB::SWSBGlobalScalarCFGReachAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        BB_LIST::iterator it = fg.begin();
        do
        {
            if (globalDependenceDefReachAnalysis((*it)))
            {
                change = true;
            }

            ++it;

        } while (it != fg.end());
    }
}

void SWSB::SWSBGlobalSIMDCFGReachAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        BB_LIST::iterator it = fg.begin();
        do
        {
            if (globalDependenceUseReachAnalysis((*it)))
            {
                change = true;
            }

            ++it;

        } while (it != fg.end());
    }
}

void SWSB::setTopTokenIndex()
{
    int startIndex = topIndex;
    if (topIndex == -1)
    {
        startIndex = 0;
    }
    for (int i = startIndex; i < (int)totalTokenNum; i++)
    {
        if (freeTokenList[i] == nullptr)
        {
            topIndex = i;
            return;
        }
    }
    for (int i = 0; i < startIndex; i++)
    {
        if (freeTokenList[i] == nullptr)
        {
            topIndex = i;
            return;
        }
    }

    topIndex = -1;
    return;
}

bool SWSB::propogateDist(G4_BB* bb)
{
    bool changed = false;
    unsigned bbID = bb->getId();

    if (bb->Preds.empty())
    {
        return false;
    }

    assert(BBVector[bbID]->send_live_in != nullptr);

    SBBitSets temp_live_in(mem, globalSendNum);
    temp_live_in = *BBVector[bbID]->send_live_in;
    std::vector<unsigned> tokenLiveInDist;
    tokenLiveInDist.resize(globalSendNum);

    for (unsigned i = 0; i < globalSendNum; i++)
    {
        tokenLiveInDist[i] = BBVector[bbID]->tokenLiveInDist[i];
    }

    //Get the live out from all predicator BBs
    for (BB_LIST_ITER it = bb->Preds.begin(); it != bb->Preds.end(); it++)
    {
        G4_BB* predBB = (*it);
        unsigned predID = predBB->getId();

        for (unsigned i = 0; i < globalSendNum; i++)
        {
            if (BBVector[predID]->send_live_out->isDstSet(i) &&
                BBVector[predID]->tokenLiveOutDist[i] != -1)
            {
                tokenLiveInDist[i] = MIN(BBVector[predID]->tokenLiveOutDist[i], tokenLiveInDist[i]);
            }
        }
    }

    //Update the live in
    for (unsigned i = 0; i < globalSendNum; i++)
    {
        if (tokenLiveInDist[i] != BBVector[bbID]->tokenLiveInDist[i] &&
            tokenLiveInDist[i] != -1)
        {
            changed = true;
            BBVector[bbID]->tokenLiveInDist[i] = tokenLiveInDist[i];
        }
    }

    //Update the live out
    if (changed)
    {
        for (unsigned i = 0; i < globalSendNum; i++)
        {
            if (BBVector[bbID]->send_live_in->isDstSet(i) &&
                BBVector[bbID]->send_live_out->isDstSet(i) &&
                !BBVector[bbID]->send_may_kill->isDstSet(i))
            {
                BBVector[bbID]->tokenLiveOutDist[i] = BBVector[bbID]->tokenLiveInDist[i] + bb->getInstList().size();
            }
        }
    }

    return changed;
}

void SWSB::calculateDist()
{
#ifdef DEBUG_VERBOSE_ON
    globalSBNodes.resize(globalSendNum);
#endif
    //Initial all live out distance
    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;

        if (BBVector[node->getBBID()]->send_live_out->isDstSet(node->globalID))
        {
            BBVector[node->getBBID()]->tokenLiveOutDist[node->globalID] = BBVector[node->getBBID()]->last_node - node->getNodeID();
#ifdef DEBUG_VERBOSE_ON
            globalSBNodes[node->globalID] = node;
#endif
        }
    }

    bool change = true;
    while (change)
    {
        change = false;
        BB_LIST::iterator it = fg.begin();
        do
        {
            if (propogateDist((*it)))
            {
                change = true;
            }

            ++it;
        } while (it != fg.end());
    }

#ifdef DEBUG_VERBOSE_ON
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        std::cerr << "BB" << i << ": " << BBVector[i]->first_node << "-" << BBVector[i]->last_node << ", succ<";
        for (std::list<G4_BB*>::iterator sit = BBVector[i]->getBB()->Succs.begin(); sit != BBVector[i]->getBB()->Succs.end(); ++sit)
        {
            std::cerr << (*sit)->getId() << ",";
        }
        std::cerr << "> pred<";
        for (std::list<G4_BB*>::iterator pit = BBVector[i]->getBB()->Preds.begin(); pit != BBVector[i]->getBB()->Preds.end(); ++pit)
        {
            std::cerr << (*pit)->getId() << ",";
        }

        std::cerr << ">\n liveIn:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (BBVector[i]->tokenLiveInDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << BBVector[i]->tokenLiveInDist[k];
            }
        }
        std::cerr << "\n liveout:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (BBVector[i]->tokenLiveOutDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << BBVector[i]->tokenLiveOutDist[k];
            }
        }
        std::cerr << "\n\n";
    }
#endif

}



/* Linear scan algorithm is used for the token allocation.
 * Based on the assumption that instruction scheduling has scheduled the instruction to the best.
 * FIXME: instruction scheduling doesn't consider the token pressure issue.
 */
void SWSB::tokenAllocation()
{
    //build live intervals
    buildLiveIntervals();

    //Initial free token list
    for (unsigned i = 0; i < totalTokenNum; i++)
    {
        freeTokenList.push_back(nullptr);
    }
    topIndex = 0;

    tokenProfile->setTokenInstructionCount((int)SBSendNodes.size());
    uint32_t AWTokenReuseCount = 0;
    uint32_t ARTokenReuseCount = 0;
    uint32_t AATokenReuseCount = 0;
    uint32_t mathInstCount = 0;
    //Linear scan
    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        unsigned startID = node->getLiveStartID();
        G4_INST* inst = node->getLastInstruction();
#ifdef DEBUG_VERBOSE_ON
        printf("\n=======nodeID: %d, startID: %d, endID: %d\n", node->getNodeID(), node->getLiveStartID(), node->getLiveEndID());
#endif
        if (inst->isEOT())
        {
            continue;
        }

        if (fg.builder->getOptions()->getOption(vISA_EnableSendTokenReduction) && node->succs.size() == 0)
        {
            continue;
        }


        if (inst->isMathPipeInst())
        {
            mathInstCount++;
        }

        expireIntervals(startID);

        unsigned short assignedToken = node->getLastInstruction()->getToken();
        //If token reuse happened, and the live range of old node is longer than current one,
        //we will keep the old one in the active list.
        assignToken(node, assignedToken,
            AWTokenReuseCount,
            ARTokenReuseCount,
            AATokenReuseCount);

        addToLiveList(node);
    }

#ifdef DEBUG_VERBOSE_ON
    dumpTokeAssignResult();
#endif

    if (fg.builder->getOptions()->getOption(vISA_SWSBDepReduction))
    {
        for (size_t i = 0; i < BBVector.size(); i++)
        {
            BBVector[i]->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
        }
#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif
        SWSBGlobalTokenAnalysis();

#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif

        unsigned prunedEdgeNum = 0;
        unsigned prunedGlobalEdgeNum = 0;
        unsigned prunedDiffBBEdgeNum = 0;
        unsigned prunedDiffBBSameTokenEdgeNum = 0;
        tokenEdgePrune(prunedEdgeNum, prunedGlobalEdgeNum, prunedDiffBBEdgeNum, prunedDiffBBSameTokenEdgeNum);
        tokenProfile->setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile->setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile->setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile->setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        G4_INST* inst = node->getLastInstruction();

        if (inst->isEOT())
        {
            continue;
        }

        unsigned short token = node->getLastInstruction()->getToken();
        if (token != (unsigned short)-1)
        {
            assignDepToken(node);
        }
    }

    tokenProfile->setAWTokenReuseCount(AWTokenReuseCount);
    tokenProfile->setARTokenReuseCount(ARTokenReuseCount);
    tokenProfile->setAATokenReuseCount(AATokenReuseCount);
    tokenProfile->setMathInstCount(mathInstCount);
}

unsigned short SWSB::reuseTokenSelectionGlobal(SBNode* node, G4_BB* bb, SBNode*& candidateNode, bool& fromSibling)
{
    SBBitSets temp_live_in(mem, globalSendNum);
    temp_live_in = *BBVector[bb->getId()]->send_live_in;
    unsigned short reuseToken = (unsigned short)UNKNOWN_TOKEN;
    unsigned nodeReuseOverhead = -1;

    tokenReuseCount++;
    for (unsigned int i = 0; i < totalTokenNum; i++)
    {
        unsigned nodeDist = -1;
        unsigned tokenReuseOverhead = 0;
        SBNode* candidateTokenNode = nullptr;
        unsigned short curToken = (unsigned short)UNKNOWN_TOKEN;
        bool fromUse = false;

        for (unsigned int k = 0; k < reachTokenArray[i]->size(); k++)
        {
            SBNode* liveNode = (*reachTokenArray[i])[k];
            unsigned liveNodeDelay = getDepDelay(liveNode);
            unsigned liveNodeOverhead = 0;

            //What about the global send come back to current BB?
            //Shouldn't be assgined
            if ((liveNode->globalID != -1) &&
                (BBVector[bb->getId()]->tokenLiveInDist[liveNode->globalID] != -1))
            {
                nodeDist = BBVector[bb->getId()]->tokenLiveInDist[liveNode->globalID] + (node->getNodeID() - BBVector[bb->getId()]->first_node);
            }
            else
            {
                assert(liveNode->getBBID() == bb->getId());
                nodeDist = node->getNodeID() - liveNode->getNodeID();
            }

            liveNodeOverhead = (liveNodeDelay > nodeDist ? (liveNodeDelay - nodeDist) : 0);
            liveNodeOverhead += liveNode->reuseOverhead;

            if ((candidateTokenNode == nullptr) || (liveNodeOverhead > tokenReuseOverhead))
            {
                tokenReuseOverhead = liveNodeOverhead;
                candidateTokenNode = liveNode;
                curToken = i;
                fromUse = false;
            }
        }

        if (fromSibling)
        {
            for (unsigned int k = 0; k < reachUseArray[i]->size(); k++)
            {
                SBNode* useNode = (*reachUseArray[i])[k];
                unsigned nodeDelay = getDepDelay(node);
                unsigned nodeOverhead = 0;

                //What about the global send come back to current BB?
                //Shouldn't be assgined
                if ((node->globalID != -1) &&
                    (BBVector[useNode->getBBID()]->tokenLiveInDist[node->globalID] != -1))
                {
                    nodeDist = BBVector[useNode->getBBID()]->tokenLiveInDist[node->globalID] + (useNode->getNodeID() - BBVector[useNode->getBBID()]->first_node);
                }
                else
                {
                    assert(useNode->getBBID() == bb->getId());
                    nodeDist = node->getNodeID() - useNode->getNodeID();
                }

                nodeOverhead = (nodeDelay > nodeDist ? (nodeDelay - nodeDist) : 0);
                nodeOverhead += node->reuseOverhead;

                if ((candidateTokenNode == nullptr) || (nodeOverhead > tokenReuseOverhead))
                {
                    tokenReuseOverhead = nodeOverhead;
                    candidateTokenNode = useNode;
                    curToken = i;
                    fromUse = true;
                }
            }
        }

        if (tokenReuseOverhead < nodeReuseOverhead)
        {
            nodeReuseOverhead = tokenReuseOverhead;
            candidateNode = candidateTokenNode;
            reuseToken = curToken;
            fromSibling = fromUse;
        }
    }

    assert(candidateNode != nullptr);
    if (!fromSibling)
    {
        node->reuseOverhead += nodeReuseOverhead;
    }

    return reuseToken;
}

void SWSB::expireLocalIntervals(unsigned startID, unsigned BBID)
{
    for (SBNODE_VECT_ITER it = localTokenUsage.begin(); it != localTokenUsage.end();)
    {
        SBNode* node = (*it);

        if (node->getLiveEndID() < startID)
        {
            it = localTokenUsage.erase(it);
            BBVector[BBID]->localReachingSends->setDst(node->sendID, false);
            continue;
        }
        it++;
    }

    return;
}

void SWSB::assignTokenToPred(SBNode* node, SBNode* pred, G4_BB* bb)
{
    unsigned predDist = -1;
    SBNode* canidateNode = nullptr;

    assert(pred->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN);

    for (auto node_it = node->preds.begin();
        node_it != node->preds.end(); node_it++)
    {
        SBDEP_ITEM& curPred = (*node_it);
        SBNode* otherPred = curPred.node;
        DepType type = curPred.type;
        unsigned dist = 0;

        if (otherPred == pred)
        {
            continue;
        }

        if (tokenHonourInstruction(otherPred->getLastInstruction()) &&
            (otherPred->getLastInstruction()->getToken() == (unsigned short)UNKNOWN_TOKEN) &&
            (type == RAW || type == WAW || otherPred->getLastInstruction()->getDst() == nullptr))
        {
            if ((!otherPred->reachingSends->isDstSet(pred->sendID)) &&
                (!pred->reachingSends->isDstSet(otherPred->sendID)))
            {
                if (otherPred->globalID != -1 &&
                    BBVector[node->getBBID()]->tokenLiveInDist[otherPred->globalID] != -1)
                {
                    dist = BBVector[node->getBBID()]->tokenLiveInDist[otherPred->globalID] + (node->getNodeID() - BBVector[node->getBBID()]->first_node);
                }
                else
                {
                    assert(otherPred->getBBID() == bb->getId());
                    dist = node->getNodeID() - otherPred->getNodeID();
                }
                if (dist < predDist)
                {
                    canidateNode = otherPred;
                    predDist = dist;
                }
            }
        }
    }

    if (canidateNode != nullptr)
    {
        canidateNode->getLastInstruction()->setToken(pred->getLastInstruction()->getToken());
#ifdef DEBUG_VERBOSE_ON
        printf("Node: %d, PRED assign: %d, token: %d\n", node->getNodeID(), canidateNode->getNodeID(), canidateNode->getLastInstruction()->getToken());
#endif
    }
}

bool SWSB::assignTokenWithPred(SBNode* node, G4_BB* bb)
{
    unsigned predDist = -1;
    SBNode* canidateNode = nullptr;
    for (auto node_it = node->preds.begin();
        node_it != node->preds.end(); node_it++)
    {
        SBDEP_ITEM& curPred = (*node_it);
        SBNode* pred = curPred.node;
        DepType type = curPred.type;
        unsigned dist = 0;

        if (tokenHonourInstruction(pred->getLastInstruction()) &&
            (pred->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN) &&
            ((type == RAW) ||(type == WAW) || (pred->getLastInstruction()->getDst() == nullptr)))
        {
            if ((pred->globalID != -1) &&
                (BBVector[bb->getId()]->tokenLiveInDist[pred->globalID] != -1))
            {
                dist = BBVector[bb->getId()]->tokenLiveInDist[pred->globalID] + (node->getNodeID() - BBVector[bb->getId()]->first_node);
            }
            else
            {
                if (fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
                {
                    if (pred->getBBID() == bb->getId())
                    {
                        dist = node->getNodeID() - pred->getNodeID();
                    }
                    else
                    {
#ifdef DEBUG_VERBOSE_ON
                        printf("Untracked distance: pred: BB%d:%d -- succ: BB%d:%d\n", pred->getBBID(), pred->getNodeID(), node->getBBID(), node->getNodeID());
#endif
                        dist = node->getNodeID() - BBVector[bb->getId()]->first_node;
                    }
                }
                else
                {
                    assert(pred->getBBID() == bb->getId());
                    dist = node->getNodeID() - pred->getNodeID();
                }
            }
            if (dist < predDist)
            {
                canidateNode = pred;
                predDist = dist;
            }
        }
    }

    if (canidateNode != nullptr)
    {
        node->getLastInstruction()->setToken(canidateNode->getLastInstruction()->getToken());
        allTokenNodesMap[canidateNode->getLastInstruction()->getToken()]->set(node->sendID, true);
#ifdef DEBUG_VERBOSE_ON
        printf("Node: %d, pred reuse assign: %d, token: %d\n", node->getNodeID(), canidateNode->getNodeID(), node->getLastInstruction()->getToken());
#endif
        return true;
    }

    return false;
}

void SWSB::allocateToken(G4_BB* bb)
{
    if ((BBVector[bb->getId()]->first_send_node == -1) ||
        BBVector[bb->getId()]->tokenAssigned)
    {
        return;
    }

    BBVector[bb->getId()]->localReachingSends = new (mem)SBBitSets(mem, SBSendNodes.size());

    assert((BBVector[bb->getId()]->last_send_node != -1) &&
        (BBVector[bb->getId()]->first_send_node <= BBVector[bb->getId()]->last_send_node));

    SBBitSets send_live(mem, SBSendNodes.size());
    SBBitSets send_use(mem, SBSendUses.size());

    for (int i = BBVector[bb->getId()]->first_send_node; i <= BBVector[bb->getId()]->last_send_node; i++)
    {
        SBNode* node = SBSendNodes[i];

        if (node->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN)
        {
            continue;
        }

        send_live = *node->reachingSends; //The tokens will reach current node

        for (unsigned k = 0; k < totalTokenNum; k++)
        {
            reachTokenArray[k]->clear();
            reachUseArray[k]->clear();
        }

        for (size_t k = 0; k < SBSendNodes.size(); k++)
        {
            SBNode* liveNode = SBSendNodes[k];
            if (send_live.isDstSet(k) &&
                (liveNode->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN))
            {
                reachTokenArray[liveNode->getLastInstruction()->getToken()]->push_back(liveNode);
            }
        }

        if (!fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && node->reachedUses)
        {
            send_use = *node->reachedUses;    //The uses of other sends can be reached by current node.
            for (size_t k = 0; k < SBSendUses.size(); k++)
            {
                SBNode* liveNode = SBSendUses[k];
                if (send_use.isDstSet(k))
                {
                    for (size_t m = 0; m < liveNode->preds.size(); m++)
                    {
                        SBDEP_ITEM& curPred = liveNode->preds[m];
                        SBNode* pred = curPred.node;
                        if (pred->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN)
                        {
                            reachUseArray[pred->getLastInstruction()->getToken()]->push_back(liveNode);
                        }
                    }
                }
            }
        }

        if (!assignTokenWithPred(node, bb))
        {
            bool assigned = false;

            //Assgined with coalescing
            if (!fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && node->reachedUses)
            {
                for (size_t i = 0; i < node->succs.size(); i++)
                {
                    SBDEP_ITEM& curSucc = node->succs[i];

                    if (!curSucc.exclusiveNodes.size())
                    {
                        continue;
                    }

                    for (size_t j = 0; j < curSucc.exclusiveNodes.size(); j++)
                    {
                        SBNode* exclusiveNode = curSucc.exclusiveNodes[j];
                        unsigned short exToken = exclusiveNode->getLastInstruction()->getToken();
                        if (exToken != (unsigned short)UNKNOWN_TOKEN)
                        {
                            if (reachTokenArray[exToken]->size() == 0 &&
                                reachUseArray[exToken]->size() == 0)
                            {
                                node->getLastInstruction()->setToken(exToken);
                                allTokenNodesMap[exToken]->set(node->sendID, true);
#ifdef DEBUG_VERBOSE_ON
                                printf("node: %d :: Use exclusive token: %d\n", node->getNodeID(), exToken);
#endif
                                assigned = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (!assigned)
            {
                //Assgined with first free token
                for (unsigned k = 0; k < totalTokenNum; k++)
                {
                    if ((reachTokenArray[k]->size() == 0) &&
                        (reachUseArray[k]->size() == 0))
                    {
                        node->getLastInstruction()->setToken(k);
                        allTokenNodesMap[k]->set(node->sendID, true);
                        assigned = true;
#ifdef DEBUG_VERBOSE_ON
                        printf("node: %d :: Use free token: %d\n", node->getNodeID(), k);
#endif
                        break;
                    }
                }
            }

            //All tokens are assigned
            if (!assigned)
            {
                SBNode* reuseNode = nullptr;
                bool reuseSibling = !fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && (node->reachedUses != nullptr);
                unsigned short reuseToken = reuseTokenSelectionGlobal(node, bb, reuseNode, reuseSibling);

#ifdef DEBUG_VERBOSE_ON
                if (!reuseSibling)
                {
                    printf("node: %d :: Reuse token: %d, from node: %d\n", node->getNodeID(), reuseToken, reuseNode->getNodeID());
                }
                else
                {
                    printf("node: %d :: Reuse token: %d, from use node: %d\n", node->getNodeID(), reuseToken, reuseNode->getNodeID());
                }
#endif

                node->getLastInstruction()->setToken(reuseToken);
                allTokenNodesMap[reuseToken]->set(node->sendID, true);
            }
        }
    }
}

void SWSB::tokenAllocationBB(G4_BB* bb)
{
    //Token allocation
    allocateToken(bb);
    BBVector[bb->getId()]->tokenAssigned = true;

    //Deep first allocation.
    for (BB_SWSB_LIST_ITER it = BBVector[bb->getId()]->domSuccs.begin(); it != BBVector[bb->getId()]->domSuccs.end(); it++)
    {
        if (!(*it)->tokenAssigned)
        {
            tokenAllocationBB((*it)->getBB());
        }
    }

    return;
}

void SWSB::tokenAllocationWithDistPropogationPerBB(G4_BB * bb)
{
    propogateDist(bb);
    allocateToken(bb);
    BBVector[bb->getId()]->tokenAssigned = true;

    for (BB_SWSB_LIST_ITER it = BBVector[bb->getId()]->domSuccs.begin(); it != BBVector[bb->getId()]->domSuccs.end(); it++)
    {
        if (!(*it)->tokenAssigned)
        {
            tokenAllocationWithDistPropogationPerBB((*it)->getBB());
        }
    }
}

void SWSB::tokenAllocationWithDistPropogation()
{
#ifdef DEBUG_VERBOSE_ON
    globalSBNodes.resize(globalSendNum);
#endif
    //Initial all live out distance
    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;

        if (BBVector[node->getBBID()]->send_live_out->isDstSet(node->globalID))
        {
            BBVector[node->getBBID()]->tokenLiveOutDist[node->globalID] = BBVector[node->getBBID()]->last_node - node->getNodeID();
#ifdef DEBUG_VERBOSE_ON
            globalSBNodes[node->globalID] = node;
#endif
        }
    }

    tokenAllocationWithDistPropogationPerBB(*fg.begin());

#ifdef DEBUG_VERBOSE_ON
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        std::cerr << "BB" << i << ": " << BBVector[i]->first_node << "-" << BBVector[i]->last_node << ", succ<";
        for (std::list<G4_BB*>::iterator sit = BBVector[i]->getBB()->Succs.begin(); sit != BBVector[i]->getBB()->Succs.end(); ++sit)
        {
            std::cerr << (*sit)->getId() << ",";
        }
        std::cerr << "> pred<";
        for (std::list<G4_BB*>::iterator pit = BBVector[i]->getBB()->Preds.begin(); pit != BBVector[i]->getBB()->Preds.end(); ++pit)
        {
            std::cerr << (*pit)->getId() << ",";
        }

        std::cerr << ">\n liveIn:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (BBVector[i]->tokenLiveInDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << BBVector[i]->tokenLiveInDist[k];
            }
        }
        std::cerr << "\n liveout:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (BBVector[i]->tokenLiveOutDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << BBVector[i]->tokenLiveOutDist[k];
            }
        }
        std::cerr << "\n\n";
    }
#endif

}

void SWSB::buildExclusiveForCoalescing()
{
    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        G4_INST* inst = node->getLastInstruction();

        if (inst->isEOT())
        {
            continue;
        }

        //If current one is a node with local live range, reuse cannot happen, because other nodes definitely can reach it.
        if (node->globalID == -1)
        {
            continue;
        }

        SBBitSets send_live(mem, SBSendNodes.size());

        for (size_t i = 0; i < node->succs.size(); i++)
        {
            SBDEP_ITEM& curSucc = node->succs[i];
            SBNode* succ = curSucc.node;
            DepType type = curSucc.type;
            if (((type == RAW) || (type == WAW)) && succ->reachingSends)
            {
                send_live = *succ->reachingSends;
                //FIXME, the complexity may be a little big high, n*n*succSize
                for (size_t k = 0; k < SBSendNodes.size(); k++)
                {
                    SBNode* liveNode = SBSendNodes[k];
                    if (send_live.isDstSet(k) &&
                        (liveNode != node) &&
                        (!(liveNode->reachingSends->isDstSet(node->sendID) ||
                            node->reachingSends->isDstSet(liveNode->sendID)) ||
                            tokenHonourInstruction(succ->GetInstruction())))
                        //If the use is token honour instruction and be assigned with same token as pred,
                        //it will cause dependence any way, cannot be removed.
                        //FIXME: But one send can depends on multiple prevoius send.
                        //Only the one set to the send will cause un-removeable dependence.
                    {
                        addReachingUseSet(liveNode, succ);
                    }
                }
            }

            if ((succ->preds.size() <= 1) ||( curSucc.exclusiveNodes.size()))
            {
                continue;
            }

            if (!((succ->getBBID() == node->getBBID() && succ->getNodeID() > node->getNodeID()) ||
                (succ->getBBID() != node->getBBID())))
            {
                continue;
            }

            for (size_t j = 0; j < succ->preds.size(); j++)
            {
                SBDEP_ITEM& curPred = succ->preds[j];
                DepType type = curPred.type;
                SBNode* pred = curPred.node;

                if (pred == node)
                {
                    continue;
                }

                if (type == WAW || type == RAW)
                {
                    if (!((succ->getBBID() == pred->getBBID() && succ->getNodeID() > pred->getNodeID()) ||
                        (succ->getBBID() != pred->getBBID())))
                    {
                        continue;
                    }

                    curSucc.exclusiveNodes.push_back(pred);
                }
            }
        }
    }

    return;
}

void SWSB::tokenAllocationGlobalWithPropogation()
{
#ifdef DEBUG_VERBOSE_ON
    dumpDepInfo();
#endif

    buildExclusiveForCoalescing();

    reachTokenArray.resize(totalTokenNum);
    reachUseArray.resize(totalTokenNum);

    for (int bucket_i = 0; bucket_i != (int)totalTokenNum; ++bucket_i)
    {
        void* allocedMem = mem.alloc(sizeof(SBNODE_VECT));
        reachTokenArray[bucket_i] = new (allocedMem) SBNODE_VECT();

        allocedMem = mem.alloc(sizeof(SBNODE_VECT));
        reachUseArray[bucket_i] = new (allocedMem) SBNODE_VECT();
    }

    tokenAllocationWithDistPropogation();

    if (fg.builder->getOptions()->getOption(vISA_SWSBDepReduction))
    {
        for (size_t i = 0; i < BBVector.size(); i++)
        {
            BBVector[i]->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
        }
#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif

        SWSBGlobalTokenAnalysis();

#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif


        unsigned prunedEdgeNum = 0;
        unsigned prunedGlobalEdgeNum = 0;
        unsigned prunedDiffBBEdgeNum = 0;
        unsigned prunedDiffBBSameTokenEdgeNum = 0;
        tokenEdgePrune(prunedEdgeNum, prunedGlobalEdgeNum, prunedDiffBBEdgeNum, prunedDiffBBSameTokenEdgeNum);
        tokenProfile->setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile->setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile->setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile->setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        G4_INST* inst = node->getLastInstruction();

        if (inst->isEOT())
        {
            continue;
        }

        unsigned short token = node->getLastInstruction()->getToken();
        if (token != (unsigned short)-1)
        {
            assignDepToken(node);
        }
    }

    return;
}

void SWSB::tokenAllocationGlobal()
{
    G4_BB* bb = (*fg.begin());

#ifdef DEBUG_VERBOSE_ON
    dumpDepInfo();
#endif

    calculateDist();

    buildExclusiveForCoalescing();

    reachTokenArray.resize(totalTokenNum);
    reachUseArray.resize(totalTokenNum);

    for (int bucket_i = 0; bucket_i != (int)totalTokenNum; ++bucket_i)
    {
        void* allocedMem = mem.alloc(sizeof(SBNODE_VECT));
        reachTokenArray[bucket_i] = new (allocedMem) SBNODE_VECT();

        allocedMem = mem.alloc(sizeof(SBNODE_VECT));
        reachUseArray[bucket_i] = new (allocedMem) SBNODE_VECT();
    }

    tokenAllocationBB(bb);

    if (fg.builder->getOptions()->getOption(vISA_SWSBDepReduction))
    {
        for (size_t i = 0; i < BBVector.size(); i++)
        {
            BBVector[i]->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
        }
#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif

        SWSBGlobalTokenAnalysis();

#ifdef DEBUG_VERBOSE_ON
        dumpTokenLiveInfo();
#endif


        unsigned prunedEdgeNum = 0;
        unsigned prunedGlobalEdgeNum = 0;
        unsigned prunedDiffBBEdgeNum = 0;
        unsigned prunedDiffBBSameTokenEdgeNum = 0;
        tokenEdgePrune(prunedEdgeNum, prunedGlobalEdgeNum, prunedDiffBBEdgeNum, prunedDiffBBSameTokenEdgeNum);
        tokenProfile->setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile->setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile->setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile->setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    for (auto node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        G4_INST* inst = node->getLastInstruction();

        if (inst->isEOT())
        {
            continue;
        }

        unsigned short token = node->getLastInstruction()->getToken();
        if (token != (unsigned short)-1)
        {
            assignDepToken(node);
        }
    }

    return;
}

G4_INST* SWSB::insertSyncInstruction(G4_BB* bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
    G4_INST* syncInst = fg.builder->createSync(G4_sync_nop, src0);
    bb->insert(nextIter, syncInst);
    syncInstCount++;

    return syncInst;
}

G4_INST* SWSB::insertSyncInstructionAfter(G4_BB* bb, INST_LIST_ITER iter, int CISAOff, int lineNo)
{
    INST_LIST_ITER nextIter = iter;
    nextIter++;
    G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
    G4_INST* syncInst = fg.builder->createSync(G4_sync_nop, src0);
    bb->insert(nextIter, syncInst);
    syncInstCount++;

    return syncInst;
}

G4_INST* SWSB::insertTestInstruction(G4_BB* bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo, bool countSync)
{
    G4_INST* nopInst = fg.builder->createNop(InstOpt_NoOpt);
    bb->insert(nextIter, nopInst);
    if (countSync)
    {
        syncInstCount++;
    }

    return nopInst;
}

G4_INST* SWSB::insertSyncAllRDInstruction(G4_BB* bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_INST* syncInst = NULL;
    if (SBIDs)
    {
        G4_Imm* src0 = fg.builder->createImm(SBIDs, Type_UD);
        syncInst = fg.builder->createSync(G4_sync_allrd, src0);
        ARSyncInstCount++;
    }
    else
    {
        G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
        syncInst = fg.builder->createSync(G4_sync_allrd, src0);
        ARSyncAllCount++;
    }
    bb->insert(nextIter, syncInst);

    return syncInst;
}

G4_INST* SWSB::insertSyncAllWRInstruction(G4_BB* bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_INST* syncInst = NULL;
    if (SBIDs)
    {
        G4_Imm* src0 = fg.builder->createImm(SBIDs, Type_UD);
        syncInst = fg.builder->createSync(G4_sync_allwr, src0);
        AWSyncInstCount++;
    }
    else
    {
        G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
        syncInst = fg.builder->createSync(G4_sync_allwr, src0);
        AWSyncAllCount++;
    }
    bb->insert(nextIter, syncInst);

    return syncInst;
}

bool SWSB::insertSyncToken(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens, bool removeAllToken)
{
    //Non-test instruction can only have
    // 1. non-send: one Dst Token with distance, or
    // 2. send: distance only, or
    // 2. one Dst token, or
    // 3. one Src token
    unsigned short dst = 0;
    unsigned short src = 0;
    bool keepDst = false;
    bool multipleDst = false;
    bool multipleSrc = false;
    unsigned short token = (unsigned short)-1;
    unsigned short dstToken = (unsigned short)-1;
    unsigned short srcToken = (unsigned short)-1;
    SWSBTokenType type = G4_INST::SWSBTokenType::TOKEN_NONE;
    bool insertedSync = false;

    for (unsigned int i = 0; i < node->GetInstruction()->getDepTokenNum();)
    {
        G4_INST* synAllInst = nullptr;
        token = inst->getDepToken(i, type);
        unsigned short bitToken = (unsigned short)(1 << token);
        assert(token != (unsigned short)UNKNOWN_TOKEN);

        switch (type)
        {
        case SWSBTokenType::AFTER_WRITE:
        case SWSBTokenType::AFTER_READ:
        {
            if (dstTokens->isSet(token) || (type == SWSBTokenType::AFTER_READ && srcTokens->isSet(token)))
            {
                //Do BB level clean up
                //So that there will be no case like following redundant sync
                //     sync.nop {$1.src}
                //     sync.nop {$1.src}
                // or
                //     sync.nop {$1.dst}
                //     sync.nop {$1.src}
                // or
                //     mov        {$1.dst}
                //     add        {$1.src}
                inst->eraseDepToken(i);
                continue;
            }
            else
            {
                if (!tokenHonourInstruction(inst) &&          //For send and math, no dependent token
                    !removeAllToken &&
                    !keepDst &&                                //No one kept yet
                    (!inst->getDistance() ||  //Only Dst can be kept
                        type == SWSBTokenType::AFTER_WRITE)) //Or there is no distance dependence
                        //FIXME: for tokenhonour instruction, we didn't support memdst only or memsrc only modes.
                        //       To support these two modes, the pre-condition is that current instruction has no SBID.
                {
                    //Token is kept in origional instruction
                    keepDst = true;
                    token = (unsigned short)UNKNOWN_TOKEN;
                    i++;
                    continue;
                }

                if (type == SWSBTokenType::AFTER_READ)
                {
                    src |= bitToken;
                    if (!multipleSrc && (src & ~bitToken))
                    {
                        multipleSrc = true;
                    }
                    srcToken = token;
                    srcTokens->set(token, true);
                }
                else
                {
                    assert(type == SWSBTokenType::AFTER_WRITE);
                    dst |= bitToken;
                    if (!multipleDst && (dst & ~bitToken))
                    {
                        multipleDst = true;
                    }
                    dstToken = token;
                    dstTokens->set(token, true);
                }

                inst->eraseDepToken(i);
                continue;
            }
        }
        break;
        case SWSBTokenType::READ_ALL:
        {
            assert(token == (unsigned short)UNKNOWN_TOKEN);
            inst->eraseDepToken(i);
            synAllInst = insertSyncAllRDInstruction(bb, 0, inst_it, inst->getCISAOff(), inst->getLineNo());
            synAllInst->setLexicalId(newInstID);
            i++;
            continue;
        }
        break;
        case SWSBTokenType::WRITE_ALL:
        {
            assert(token == (unsigned short)UNKNOWN_TOKEN);
            inst->eraseDepToken(i);
            synAllInst = insertSyncAllWRInstruction(bb, 0, inst_it, inst->getCISAOff(), inst->getLineNo());
            synAllInst->setLexicalId(newInstID);
            i++;
            continue;
        }
        break;
        default:
            assert(0);
            break;
        }
        i++;
    }

    G4_INST* synInst = nullptr;
    if (dst)
    {
        if (dst == 0xFFFF)
        {
            synInst = insertSyncAllWRInstruction(bb, 0, inst_it, inst->getCISAOff(), inst->getLineNo());
        }
        else if (multipleDst)
        {
            synInst = insertSyncAllWRInstruction(bb, dst, inst_it, inst->getCISAOff(), inst->getLineNo());
        }
        else
        {
            synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
            synInst->setDepToken(dstToken, SWSBTokenType::AFTER_WRITE);
        }
        synInst->setLexicalId(newInstID);
        insertedSync = true;
    }

    if (src)
    {
        if (src == 0xFFFF)
        {
            synInst = insertSyncAllRDInstruction(bb, 0, inst_it, inst->getCISAOff(), inst->getLineNo());
        }
        else if (multipleSrc)
        {
            synInst = insertSyncAllRDInstruction(bb, src, inst_it, inst->getCISAOff(), inst->getLineNo());
        }
        else
        {
            synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
            synInst->setDepToken(srcToken, SWSBTokenType::AFTER_READ);
        }
        synInst->setLexicalId(newInstID);
        insertedSync = true;
    }

    return insertedSync;
}


void SWSB::insertSync(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens)
{
    //The inst after arch register instruction.
    bool insertedSync = false;
    INST_LIST_ITER prevIt = inst_it;
    if (node->followDistOneAreg())
    {
        prevIt--;
    }

    //Architecture register instruction
    bool hasValidNextInst = false;
    if (node->hasDistOneAreg())
    {
        INST_LIST_ITER nextIt = inst_it;
        nextIt++;
        if (nextIt != bb->getInstList().end())
        {
            G4_INST *nextInst = *nextIt;
            if (tokenHonourInstruction(nextInst) ||
                distanceHonourInstruction(nextInst))
            {
                hasValidNextInst = true;
            }
        }
    }

    {
        insertedSync = insertSyncToken(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, false);
    }

    if (node->followDistOneAreg() && insertedSync)
    {
        G4_INST* syncInst = nullptr;

        syncInst = insertSyncInstructionAfter(bb, prevIt, inst->getCISAOff(), inst->getLineNo());
        syncInst->setDistance(1);
    }

    if (node->hasDistOneAreg() && !hasValidNextInst)
    {
        G4_INST* syncInst = nullptr;

        syncInst = insertSyncInstructionAfter(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
        syncInst->setDistance(1);
    }
}

//
// Insert the test instruction according to token assignment result. Re-assign the node id.
// Except the test instruction, one instruction can have at most one token.
// SWSB format - non send
// 7    6    5    4    3    2    1    0
// 0    0    0    0    0    0    0    0    No dependency
// 0    0    0    0    regDist dst                      Reg only dep (1-15)
// 0    0    0    1    R    R    R    R    Reserved
// 0    0    1    0    R    memSBid dst        Memory dst only dep (0-7)
// 0    0    1    1    R    memSBid src         Memory src only dep (0-7)
// 0    1    R    R    R    R    R    R    Reserved for Future extensions
// 1    memSBid dst        regDist dst            Reg and Memory dst dep
//
// SWSB format - send
// 0    0    0    0    0    0    0    0    No dependency
// 0    0    0    0    regDist dst            Reg only dep (1-15)
// 0    0    0    1    R    memSBid set        SBid allocation only (0-7)
// 0    0    1    0    R    memSBid dst        Memory dst only dep (0-7)
// 0    0    1    1    R    memSBid src         Memory src only dep (0-7)
// 0    1    R    R    R    R    R    R    Reserved for Future extensions
// 1    memSBid set        regDist dst            SBid allocation and Reg only dep (1-15)
//
// 8bits [7:0]    8bits [15:8]    4bits [27:24]        1 bit  [29]     1bit [30]    16bits [47:32]
// test = 0x70    SWSB            subOpcode            CmptCtrl = 1 DebugCtrl
//                               0000 - Only SWSB check
//                                 0001 - Check Send status                      2bits x 8 Sbid
//                                                                              00 - SBid not checked
//                                                                              01 - reserved
//                                                                              10 - Check for data sent out
//                                                                               11 - Check for data received
//                              0010 - Check Address Register Dep            1bits x 16 address registers
//                                                                                0 - Not checked
//                                                                               1 - Check for Register dependency
//                                  others - Reserved
//
void SWSB::insertTest()
{
    SBNODE_VECT_ITER node_it = SBNodes.begin();
    int newInstID = 0;

    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    for (; ib != bend; ++ib)
    {
        G4_BB* bb = (*ib);
        BitSet dstTokens(totalTokenNum, false);
        BitSet srcTokens(totalTokenNum, false);

        std::list<G4_INST*>::iterator inst_it(bb->begin()), iInstNext(bb->begin());
        while (iInstNext != bb->end())
        {
            inst_it = iInstNext;
            iInstNext++;
            G4_INST* inst = *inst_it;

            if (inst->isLabel())
            {
                continue;
            }

            SBNode* node = *node_it;
            assert(node->GetInstruction() == inst);

            bool fusedSync = false;
            //HW W/A
            //For fused URB sends, or typed write, in HW, the dependence info of the second send instruction cannot be decoded
            //Software will check and promoted them before the first instruction.
            //If the second one is EOT instruction, syncAll is required.
            if (inst->isSend() &&
                inst->isAtomicInst())
            {
                INST_LIST_ITER tmp_it = inst_it;
                tmp_it++;
                if (tmp_it != bb->end())
                {
                    G4_INST* nextInst = *tmp_it;

                    if (nextInst->isSend())
                    {
                        G4_INST* synInst = nullptr;
                        if (nextInst->isEOT())
                        {
                            //If the second is EOT, sync all can be inserted directly, because EOT has no token info
                            synInst = insertSyncAllWRInstruction(bb, 0, inst_it, inst->getCISAOff(), nextInst->getLineNo());
                            synInst->setLexicalId(newInstID);
                        }
                        else
                        {
                            fusedSync = true;
                            if (inst->getToken() != (unsigned short)UNKNOWN_TOKEN)
                            {
                                dstTokens.set(inst->getToken(), false);
                                srcTokens.set(inst->getToken(), false);
                            }
                        }
                    }
                }
            }
            if (fusedSync)
            {
                insertSync(bb, node, inst, inst_it, newInstID, &dstTokens, &srcTokens);
                inst->setLexicalId(newInstID);
                newInstID++;

                INST_LIST_ITER tmp_it = inst_it;
                inst_it++;
                iInstNext++;
                node_it++;
                inst = *inst_it;
                node = *node_it;
                if (inst->getToken() != (unsigned short)UNKNOWN_TOKEN)
                {
                    dstTokens.set(inst->getToken(), false);
                    srcTokens.set(inst->getToken(), false);
                }
                //tmp_it keeps the postion to insert new generated instructions.
                insertSync(bb, node, inst, tmp_it, newInstID, &dstTokens, &srcTokens);
                unsigned short token = inst->getToken();
                if (token != (unsigned short)UNKNOWN_TOKEN)
                {
                    G4_INST* synInst = insertSyncInstruction(bb, tmp_it, inst->getCISAOff(), inst->getLineNo());
                    synInst->setDepToken(token, SWSBTokenType::AFTER_WRITE);
                    synInst->setLexicalId(newInstID);
                }
            }
            else
            {
                insertSync(bb, node, inst, inst_it, newInstID, &dstTokens, &srcTokens);
            }

            if (inst->getToken() != (unsigned short)UNKNOWN_TOKEN)
            {
                dstTokens.set(inst->getToken(), false);
                srcTokens.set(inst->getToken(), false);
            }

            inst->setLexicalId(newInstID);
            for (unsigned i = 1; i < node->instVec.size(); i++)
            {
                inst = *iInstNext;
                inst->setLexicalId(newInstID);
                iInstNext++;
            }

            if (tokenHonourInstruction(inst) && inst->getToken() != (unsigned short)UNKNOWN_TOKEN)
            {
                dstTokens.set(inst->getToken(), false);
                srcTokens.set(inst->getToken(), false);
            }

            newInstID++;
            node_it++;
        }
    }

    tokenProfile->setSyncInstCount(syncInstCount);
    tokenProfile->setMathReuseCount(mathReuseCount);
    tokenProfile->setAWSyncInstCount(AWSyncInstCount);
    tokenProfile->setARSyncInstCount(ARSyncInstCount);
    tokenProfile->setAWSyncAllCount(AWSyncAllCount);
    tokenProfile->setARSyncAllCount(ARSyncAllCount);
    tokenProfile->setTokenReuseCount(tokenReuseCount);
}

void SWSB::dumpDepInfo()
{
    for (SBNODE_VECT_ITER node_it = SBNodes.begin();
        node_it != SBNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }

        G4_INST* inst = node->GetInstruction();
        std::cerr << node->getNodeID() << ":\t";
        inst->dump();
        std::cerr << "Succs:";
        for (int i = 0; i < (int)(node->succs.size()); i++)
        {
            SBDEP_ITEM& curSucc = node->succs[i];
            std::cerr << curSucc.node->getNodeID() << ":" << ((curSucc.attr == DEP_EXPLICT) ? "E" : "I") << ", ";
            if (curSucc.type == RAW || curSucc.type == WAW)
            {
                std::cerr << "AW;";
            }
            else
            {
                std::cerr << "AR;";
            }
        }
        std::cerr << "\n";
        std::cerr << "Preds:";
        for (int i = 0; i < (int)(node->preds.size()); i++)
        {
            SBDEP_ITEM& curPred = node->preds[i];
            std::cerr << curPred.node->getNodeID() << ":" << ((curPred.attr == DEP_EXPLICT) ? "E" : "I") << ", ";
        }
        std::cerr << "\n\n";
    }
}

void SWSB::dumpLiveIntervals()
{
    std::cerr << "Internal:" << "\n";
    for (SBNODE_VECT_ITER node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }
        node->dumpInterval();
    }
}

void SWSB::dumpTokeAssignResult()
{
    std::cerr << "Internal:" << "\n";
    for (SBNODE_VECT_ITER node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();
        node_it++)
    {
        SBNode* node = *node_it;
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }
        node->dumpAssignedTokens();
    }
}

void SWSB::dumpSync(SBNode* tokenNode, SBNode* syncNode, unsigned short token, SWSBTokenType type)
{
    std::cerr << "#" << syncNode->getNodeID() << "(" << token << ",";
    std::cerr << ((type == SWSBTokenType::AFTER_READ) ? "AR" : "AW") << ")";
    std::cerr << ": " << "#" << tokenNode->getNodeID() << "(" << tokenNode->getLiveStartID() << "-" << tokenNode->getLiveEndID() << ")\n";
}

void SWSB::buildLiveIntervals()
{
    // For all send nodes
    // Set the live ranges according to dependence edges
    for (SBNODE_VECT_ITER node_it = SBSendNodes.begin();
        node_it != SBSendNodes.end();)
    {
        SBNode* node = *node_it;
        SBNODE_VECT_ITER succ_it = node_it;
        succ_it++;

        node->setLiveEarliesID(node->getNodeID(), node->getBBID());
        node->setLiveLatestID(node->getNodeID(), node->getBBID());
        for (int i = 0; i < (int)(node->succs.size()); i++)
        {
            SBDEP_ITEM& curSucc = node->succs[i];
            SBNode* succ = curSucc.node;
            {
                node->setLiveLatestID(succ->getNodeID(), succ->getBBID());
            }
        }
        node_it++;
    }

#ifdef DEBUG_VERBOSE_ON
    dumpLiveIntervals();
    dumpDepInfo();
#endif

    //For global send nodes
    //According to layout, extend the live range of each send operand to
    //the start of the first live in BB and end of last live out BB
    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    for (; ib != bend; ++ib)
    {
        unsigned bbID = (*ib)->getId();
        SBBitSets* send_live_in = BBVector[bbID]->send_live_in;
        SBBitSets* send_live_out = BBVector[bbID]->send_live_out;
        SBBitSets* send_live_in_scalar = BBVector[bbID]->send_live_in_scalar;
        SBBitSets* send_live_out_scalar = BBVector[bbID]->send_live_out_scalar;

        if (send_live_in->isEmpty())
        {
            continue;
        }

        for (size_t i = 0; i < globalSendOpndList.size(); i++)
        {
            SBNode* node = globalSendOpndList[i]->node;
            int globalID = node->globalID;


            if (globalSendOpndList[i]->opndNum == Opnd_dst)
            {
                if ((send_live_in_scalar->isDstSet((unsigned)globalID)) &&
                    BBVector[bbID]->first_node != -1)
                {
                    if (!(*ib)->Preds.empty() || !(BBVector[bbID]->Preds.empty()))
                    {
                        node->setLiveEarliesID(BBVector[bbID]->first_node, bbID);
                    }
                }
                //FIXME: implicit dependence still have issue.
                //the live range of implicit dependence may not counted. But that's ok? This may cause the delay. ...
                if ((send_live_out_scalar->isDstSet((unsigned)globalID)) &&
                    BBVector[bbID]->first_node != -1)
                {
                    if (!(*ib)->Succs.empty() || !(BBVector[bbID]->Succs.empty()))
                    {
                        node->setLiveLatestID(BBVector[bbID]->last_node, bbID);
                    }
                }
            }
            else
            {
                if ((send_live_in->isSrcSet((unsigned)globalID)) &&
                    BBVector[bbID]->first_node != -1)
                {
                    if (!(*ib)->Preds.empty() || !(BBVector[bbID]->Preds.empty()))
                    {
                        node->setLiveEarliesID(BBVector[bbID]->first_node, bbID);
                    }
                }
                //FIXME: implicit dependence still have issue.
                //the live range of implicit dependence may not counted. But that's ok? This may cause the delay. ...
                if ((send_live_out->isSrcSet((unsigned)globalID)) &&
                    BBVector[bbID]->first_node != -1)
                {
                    if (!(*ib)->Succs.empty() || !(BBVector[bbID]->Succs.empty()))
                    {
                        node->setLiveLatestID(BBVector[bbID]->last_node, bbID);
                    }
                }
            }
        }
    }

    //Sort the live ranges
    std::sort(SBSendNodes.begin(), SBSendNodes.end(), compareInterval);

#ifdef DEBUG_VERBOSE_ON
    dumpLiveIntervals();
#endif
    return;
}

//
// live_in(BBi) = Union(def_out(BBj)) // BBj is predecessor of BBi
// live_out(BBi) += live_in(BBi) - may_kill(BBi)
//
bool SWSB::globalDependenceDefReachAnalysis(G4_BB* bb)
{
    bool changed = false;
    unsigned bbID = bb->getId();

    if (bb->Preds.empty())
    {
        return false;
    }

    assert(BBVector[bbID]->send_live_in != nullptr);

    SBBitSets temp_live_in(mem, globalSendNum);
    temp_live_in = *BBVector[bbID]->send_live_in;

    for (BB_LIST_ITER it = bb->Preds.begin(); it != bb->Preds.end(); it++)
    {
        G4_BB* predBB = (*it);
        unsigned predID = predBB->getId();
        temp_live_in |= *(BBVector[predID]->send_live_out);
    }

    if (temp_live_in != *BBVector[bbID]->send_live_in)
    {
        changed = true;
        *BBVector[bbID]->send_live_in = temp_live_in;
    }

    //Record the killed dst and src in scalar CF iterating
    SBBitSets temp_kill(mem, globalSendNum);
    temp_kill = temp_live_in;
    temp_kill &= *BBVector[bbID]->send_may_kill;
    *BBVector[bbID]->send_kill_scalar |= temp_kill;

    temp_kill = temp_live_in;
    temp_kill.src &= BBVector[bbID]->send_may_kill->dst;
    BBVector[bbID]->send_kill_scalar->src |= temp_kill.src;

    //Kill nodes
    //once dst is killed, src definitly is killed
    temp_live_in -= *BBVector[bbID]->send_may_kill;
    temp_live_in.src -= BBVector[bbID]->send_may_kill->dst;

    *BBVector[bbID]->send_live_out |= temp_live_in;

    return changed;
}

//
// live_in(BBi) = Union(def_out(BBj)) // BBj is predecessor of BBi
// live_out(BBi) += live_in(BBi) - may_kill(BBi)
//
bool SWSB::globalDependenceUseReachAnalysis(G4_BB* bb)
{
    bool changed = false;
    unsigned bbID = bb->getId();

    if (bb->Preds.empty())
    {
        return false;
    }

    assert(BBVector[bbID]->send_live_in != nullptr);

    SBBitSets temp_live_in(mem, globalSendNum);
    temp_live_in = *BBVector[bbID]->send_live_in;

    for (BB_SWSB_LIST_ITER it = BBVector[bbID]->Preds.begin(); it != BBVector[bbID]->Preds.end(); it++)
    {
        G4_BB* predBB = (*it)->getBB();
        unsigned predID = predBB->getId();
        temp_live_in |= *(BBVector[predID]->send_live_out);
    }

    if (temp_live_in != *BBVector[bbID]->send_live_in)
    {
        changed = true;
        *BBVector[bbID]->send_live_in = temp_live_in;
    }

    //Kill scalar kills
    temp_live_in -= *BBVector[bbID]->send_kill_scalar;
    temp_live_in.src -= BBVector[bbID]->send_may_kill->src;
    temp_live_in.dst -= *BBVector[bbID]->send_WAW_may_kill;

    *BBVector[bbID]->send_live_out |= temp_live_in;

    return changed;
}


void SWSB::tokenEdgePrune(unsigned& prunedEdgeNum,
    unsigned& prunedGlobalEdgeNum,
    unsigned& prunedDiffBBEdgeNum,
    unsigned& prunedDiffBBSameTokenEdgeNum)
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        if (BBVector[i]->first_node == -1)
        {
            continue;
        }

        BitSet activateLiveIn(SBSendNodes.size(), false);
        activateLiveIn |= *BBVector[i]->liveInTokenNodes;

        //Scan the instruction nodes of current BB
        for (int j = BBVector[i]->first_node; j <= BBVector[i]->last_node; j++)
        {
            SBNode* node = SBNodes[j];
            BitSet killedToken(totalTokenNum, false); //Track the token killed by current instruction.

            //scan the incoming dependence edges of current node
            for (auto node_it = node->preds.begin();
                node_it != node->preds.end();
                node_it++)
            {
                SBDEP_ITEM& curPred = (*node_it);
                DepType type = curPred.type;
                SBNode* predNode = curPred.node;

                //If the predecessor node is a token instruction node.
                if (tokenHonourInstruction(predNode->GetInstruction()))
                {
                    if (!activateLiveIn.isSet(predNode->sendID))
                    {
                        // If not in the live set of current instruction,
                        // (The live in set will be changed during instruction scan)
                        // remove the dependence from success list of previous node
                        // The dependence SBID assignment only depends on the succ nodes.
                        for (auto succ_it = predNode->succs.begin();
                            succ_it != predNode->succs.end();
                            succ_it++)
                        {
                            SBDEP_ITEM& currSucc = (*succ_it);
                            if (currSucc.node == node)
                            {
                                //Don't do remove previous edge here.
                                //1. conflict with outer loop
                                //2. There is no preds info required any more in following handling
                                predNode->succs.erase(succ_it);
                                prunedEdgeNum++;
                                if (predNode->globalID != -1)
                                {
                                    if (predNode->getBBID() != node->getBBID() &&
                                        !killedToken.isSet(predNode->getLastInstruction()->getToken()) &&
                                        (!(fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
                                           fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation)) ||
                                        !((fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
                                            fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation)) &&
                                          BBVector[node->getBBID()]->dominators->isSet(predNode->getBBID()))))
                                    {
                                        prunedDiffBBEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Diff BB Token: " << predNode->getLastInstruction()->getToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                    else if (predNode->getBBID() != node->getBBID())
                                    {
                                        prunedDiffBBSameTokenEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Diff BB Same Token: " << predNode->getLastInstruction()->getToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                    else
                                    {
                                        prunedGlobalEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Global Token: " << predNode->getLastInstruction()->getToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                }
#ifdef DEBUG_VERBOSE_ON
                                else
                                {
                                    std::cerr << "Local Token: " << predNode->getLastInstruction()->getToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
                                }
#endif
                                break;
                            }
                        }
                    }
                    else //In live in set
                    {
                        // Kill the dependence if it's a AW dependence
                        // What about WAR?
                        if (type == RAW || type == WAW)
                        {
                            int token = predNode->getLastInstruction()->getToken();
                            if (token != (unsigned short)UNKNOWN_TOKEN)
                            {
                                activateLiveIn -= *allTokenNodesMap[token];
                                killedToken.set(token, true);
                            }
                        }
                    }
                }
            }

            // Current instruction is marked as alive
            // How to kill the old one? Especially the WAR?
            // Token reuse will kill all previous nodes with same token? yes
            if (tokenHonourInstruction(node->GetInstruction()) && !node->GetInstruction()->isEOT())
            {
                int token = node->getLastInstruction()->getToken();
                if (token != (unsigned short)UNKNOWN_TOKEN)
                {
                    activateLiveIn -= *allTokenNodesMap[token];
                    activateLiveIn.set(node->sendID, true);
                }
            }
        }
    }
}

void G4_BB_SB::getLiveOutToken(unsigned allSendNum,
    SBNODE_VECT* SBNodes)
{
    //Empty BB
    if (first_node == -1)
    {
        return;
    }

    uint32_t totalTokenNum = builder.kernel.getNumSWSBTokens();
    unsigned* liveNodeID = (unsigned*)mem.alloc(sizeof(unsigned) * totalTokenNum);

    if (tokeNodesMap == nullptr)
    {
        tokeNodesMap = (BitSet * *)mem.alloc(sizeof(BitSet*) * totalTokenNum);

        //Each token ID has a bitset for all possible send instructions' ID
        for (size_t i = 0; i < totalTokenNum; i++)
        {
            tokeNodesMap[i] = new (mem)BitSet(allSendNum, false);
            liveNodeID[i] = 0;
        }
    }
    else
    {
        for (size_t i = 0; i < totalTokenNum; i++)
        {
            tokeNodesMap[i]->clear();
            liveNodeID[i] = 0;
        }
    }

    // Scan instructions forward to get the live out of current BB
    for (int i = first_node; i <= last_node; i++)
    {
        SBNode* node = (*SBNodes)[i];

        //Check the previos node.
        for (auto node_it = node->preds.begin();
            node_it != node->preds.end();
            node_it++)
        {
            SBDEP_ITEM& curPred = (*node_it);
            DepType type = curPred.type;
            SBNode* predNode = curPred.node;

            if ((predNode == node) ||
                (predNode->getBBID() != node->getBBID()) ||
                (predNode->getNodeID() > node->getNodeID()))
            {
                continue;
            }


            //If there is a .dst dependence, kill all nodes with same token
            if (tokenHonourInstruction(predNode->getLastInstruction()) && (type == RAW || type == WAW))
            {
                if (predNode->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN)
                {
                    unsigned short token = predNode->getLastInstruction()->getToken();
                    // 1:  send r112                   {$9}
                    // 2:  send r18                    {$9}
                    // 3:  send r112                   {$9}
                    // 4:  send xxx,     r18           {12}
                    //
                    // Instruction 4 may clear the $9 because of instruction 2
                    // liveNodeID is used to track the live node id of each send. predNode can kill
                    if (liveNodeID[token] < predNode->getNodeID())
                    {
                        tokeNodesMap[token]->clear(); //Kill all dependence in following instructions with the same token

                        //Record the killed token by current BB, Kill may kill all previous nodes which reach current node
                        killedTokens->set(token, true);  //Set previous token send killed in current BB
                    }
                }
            }
        }

        //Token reuse will kill all previous nodes with same token
        //Will have only one?, yes, for BB local scan
        if (tokenHonourInstruction(node->getLastInstruction()) &&
            !node->getLastInstruction()->isEOT() &&
            node->getLastInstruction()->getToken() != (unsigned short)UNKNOWN_TOKEN)
        {
            unsigned short token = node->getLastInstruction()->getToken();
            tokeNodesMap[token]->clear();

            //For future live in, will always be killed by current instruction
            killedTokens->set(token, true);

            //Current node may be in live out, if not be killed in following insts.
            tokeNodesMap[token]->set(node->sendID, true);
            liveNodeID[token] = node->getNodeID();
        }
    }

    for (size_t i = 0; i < totalTokenNum; i++)
    {
        (*liveOutTokenNodes) |= *tokeNodesMap[i];
    }
}
//
// Scan to check which global send operand for sends will be killed by current BB.
// Note that there is no gaurantee the send operand will in the live in set of BB.
// !!! Note that: since this "may kill" info is used in global anaysis, "may kill" is not accurate, here we in fact record the "definitely kill".
void G4_BB_SB::setSendOpndMayKilled(LiveGRFBuckets* globalSendsLB,
    SBNODE_VECT* SBNodes,
    PointsToAnalysis& p)
{
    std::vector<SBBucketDescr> BDvec;
    if (first_node == -1)
    {
        return;
    }
    for (int i = first_node; i <= last_node; i++)
    {
        SBNode* node = (*SBNodes)[i];
        G4_INST* curInst = (*SBNodes)[i]->GetInstruction();

        if (curInst->isLabel())
        {
            continue;
        }

        BDvec.clear();
        getGRFBucketDescrs(node, BDvec, true);
        if (!BDvec.size())
        {
            continue;
        }

        // For all bucket descriptors of curInst
        for (const SBBucketDescr& BD : BDvec) {
            const int& curBucket = BD.bucket;
            const Gen4_Operand_Number& curOpnd = BD.opndNum;
            SBFootprint* curFootprint = BD.node->getFootprint(BD.opndNum, BD.inst);

            for (LiveGRFBuckets::BN_iterator bn_it = globalSendsLB->begin(curBucket);
                bn_it != globalSendsLB->end(curBucket);)
            {
                SBBucketNode* liveBN = (*bn_it);
                SBNode* curLiveNode = liveBN->node;
                Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                G4_INST* liveInst = liveBN->inst;
                SBFootprint* liveFootprint = curLiveNode->getFootprint(liveBN->opndNum,liveInst);

                //Send operands are all GRF aligned, there is no overlap checking required.
                //Fix me, this is not right, for math intruction, less than 1 GRF may happen.
                //Find DEP type
                unsigned short internalOffset = 0;
                bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);
                if (!hasOverlap)
                {
                    ++bn_it;
                    continue;
                }

                DepType dep = DEPTYPE_MAX;
                dep = getDepForOpnd(liveOpnd, curOpnd);

                //For SBID global liveness analysis, both explict and implicit kill counted.
                if (dep == RAW || dep == WAW)
                {
                    send_may_kill->setDst(curLiveNode->globalID, true);
                    if (dep == WAW)
                    {
                        send_WAW_may_kill->set(curLiveNode->globalID, true);
                    }
                }

                if (dep == WAR &&
                    WARDepRequired(liveInst, BD.inst))
                {
                    send_may_kill->setSrc(curLiveNode->globalID, true);
                }

                //FIXME: for NODEP, there is optimizatoin chance.
                //               if (hasSameFunctionID(liveInst, curInst))
                //                    send  null,  r1, r73, ...   {$0}
                //                    send  null,  r1, r60, ...   {$1}
                //                    add    r60...    {$1.src}
                //                    add    r73        // There is no need to set {$0.src}
                //
                //                    send  null,  r1, r73, ...   {$0}
                //                    send  null,  r1, r60, ...   {$1}
                //                    add    r73        {$0.src} // We need to set {$0.src}
                //                    add    r60...    {$1.src}
                //if (dep == NODEP && !hasSameFunctionID(liveInst, curInst)) //Conservative, only different pipeline, we will insert dependence tracking
                //{
                //    send_may_kill->setSrc(curLiveNode->globalID, true);
                //}

                assert(dep != DEPTYPE_MAX && "dep unassigned?");
                ++bn_it;
            }
        }
    }
}

bool G4_BB_SB::getFootprintForOperand(SBNode* node,
    G4_INST* inst,
    G4_Operand* opnd,
    Gen4_Operand_Number opndNum)
{
    int startingBucket = UNINIT_BUCKET;
    bool hasDistOneAReg = false;
    bool footprintOperand = false;
    bool isAccReg = false;
    bool isFlagReg = false;
    SBFootprint* footprint = nullptr;
    G4_VarBase* base = opnd->getBase();

    assert(base && "If no base, then the operand is not touched by the instr.");

    G4_VarBase* phyReg = (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

    switch (phyReg->getKind())
    {
    case G4_VarBase::VK_phyGReg:
        startingBucket = 0;
        footprintOperand = true;
        break;
    case G4_VarBase::VK_phyAReg:
        if (phyReg->isSrReg() ||
            phyReg->isCrReg() ||
            phyReg->isSpReg() ||
            phyReg->isIpReg() ||
            phyReg->isTmReg() ||
            phyReg->isMaskReg() ||
            phyReg->isDbgReg())
        {
            hasDistOneAReg = true;
        }
        isAccReg = phyReg->isAccReg();
        isFlagReg = phyReg->isFlag();
        break;
    case G4_VarBase::VK_regVar:
        assert(0 && "Should not be a regvar. PhyReg is extracted from regvar.");
        break;
    default:
        assert(0 && "Bad kind");
        break;
    }

    if (footprintOperand)
    {
        // Create one or more buckets and push them into the vector
        footprint = getFootprintForGRF(opnd, opndNum, inst, startingBucket, inst->isSend());
        node->setFootprint(footprint, opndNum);
    }


    return hasDistOneAReg;
}

void G4_BB_SB::getGRFFootprintForIndirect(SBNode* node,
    Gen4_Operand_Number opnd_num,
    G4_Operand* opnd,
    PointsToAnalysis& p)
{
    G4_Declare* addrdcl = nullptr;
    SBFootprint* footprint = nullptr;
    G4_Type type = opnd->getType();

    if (opnd_num == Opnd_dst)
    {
        G4_DstRegRegion* dstrgn = opnd->asDstRegRegion();
        addrdcl = GetTopDclFromRegRegion(dstrgn);
    }
    else if (opnd_num == Opnd_src0 ||
        opnd_num == Opnd_src1 ||
        opnd_num == Opnd_src2 ||
        opnd_num == Opnd_src3)
    {
        G4_SrcRegRegion* srcrgn = opnd->asSrcRegRegion();
        addrdcl = GetTopDclFromRegRegion(srcrgn);
    }
    else
    {
        assert(0);
    }

#ifdef DEBUG_VERBOSE_ON
    std::cerr << addrdcl->getName() << ":" << std::endl;
    std::cerr << node->getNodeID() << ":";
    node->GetInstruction()->dump();
    std::cerr << "Point to: ";
#endif

    if (addrdcl == nullptr)
    {
        assert(0);
        return;
    }

    G4_RegVar* ptvar = NULL;
    int vid = 0;

    while ((ptvar = p.getPointsTo(addrdcl->getRegVar(), vid++)) != NULL)
    {

        uint32_t varID = ptvar->getId();
        G4_Declare* dcl = ptvar->getDeclare();
        G4_RegVar* var = NULL;

        while (dcl->getAliasDeclare())
        {
            dcl = dcl->getAliasDeclare();
        }


        int linearizedStart = 0;
        int linearizedEnd = 0;

        if (dcl->isSpilled()) //FIXME: Lost point analysis tracking due to spill, assume all registers are touched
        {
            linearizedEnd = totalGRFNum * G4_GRF_REG_NBYTES - 1;
        }
        else
        {
            var = dcl->getRegVar();

            MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
            MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

            uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            uint32_t regOff = var->getPhyRegOff();

            linearizedStart = regNum * G4_GRF_REG_NBYTES + regOff * G4_Type_Table[dcl->getElemType()].byteSize;
            linearizedEnd = regNum * G4_GRF_REG_NBYTES + regOff * G4_Type_Table[dcl->getElemType()].byteSize + dcl->getByteSize() - 1;
        }

        void* allocedMem = mem.alloc(sizeof(SBFootprint));
        footprint = new(allocedMem)SBFootprint(GRF_T, type, (unsigned short)linearizedStart, (unsigned short)linearizedEnd, node->GetInstruction());
        node->setFootprint(footprint, opnd_num);
#ifdef DEBUG_VERBOSE_ON
        int startingBucket = linearizedStart / G4_GRF_REG_NBYTES;
        int endingBucket = linearizedEnd / G4_GRF_REG_NBYTES;
        std::cerr << dcl->getName() << "<" << startingBucket << "," << endingBucket << ">";
#endif
    }
#ifdef DEBUG_VERBOSE_ON
    std::cerr << std::endl;
#endif
    return;
}

//Create Buckets
void G4_BB_SB::getGRFBuckets(SBNode* node,
    SBFootprint* footprint,
    Gen4_Operand_Number opndNum,
    std::vector<SBBucketDescr>& BDvec,
    bool GRFOnly)
{
    SBFootprint* curFootprint = footprint;

    while (curFootprint != nullptr)
    {
        if (GRFOnly && (curFootprint->fType != GRF_T))
        {
            curFootprint = curFootprint->next;
            continue;
        }

        int aregOffset = totalGRFNum;
        int startingBucket = curFootprint->LeftB / G4_GRF_REG_NBYTES;
        int endingBucket = curFootprint->RightB / G4_GRF_REG_NBYTES;
        if (curFootprint->fType == ACC_T)
        {
            startingBucket = startingBucket + aregOffset;
            endingBucket = endingBucket + aregOffset;
        }
        int numBuckets = endingBucket - startingBucket + 1;
        for (int j = startingBucket;
            j < (startingBucket + numBuckets); j++)
        {
            BDvec.push_back(SBBucketDescr(j, opndNum, node, curFootprint->inst));
        }
        curFootprint = curFootprint->next;
    }

    return;
}

bool G4_BB_SB::getGRFFootPrintOperands(SBNode* node,
    G4_INST* inst,
    Gen4_Operand_Number first_opnd,
    Gen4_Operand_Number last_opnd,
    PointsToAnalysis& p)
{
    bool hasDistOneAreg = false;
    for (Gen4_Operand_Number opndNum = first_opnd; opndNum <= last_opnd; opndNum = (Gen4_Operand_Number)(opndNum + 1))
    {

        G4_Operand* opnd = inst->getOperand(opndNum);

        if (!opnd || !opnd->getBase())
        {
            continue;
        }

        if (opnd->isLabel() || opnd->isImm())
        {
            continue;
        }

        hasDistOneAreg |= getFootprintForOperand(node, inst, opnd, opndNum);


        //Get bucket for indirect access
        if (hasIndirection(opnd, opndNum))
        {
            getGRFFootprintForIndirect(node, opndNum, opnd, p);
        }
    }

    return hasDistOneAreg;
}

void G4_BB_SB::getGRFBucketsForOperands(SBNode* node,
    Gen4_Operand_Number first_opnd,
    Gen4_Operand_Number last_opnd,
    std::vector<SBBucketDescr>& BDvec,
    bool GRFOnly)
{
    for (Gen4_Operand_Number opndNum = first_opnd; opndNum <= last_opnd; opndNum = (Gen4_Operand_Number)(opndNum + 1))
    {
        SBFootprint* footprint = node->getFirstFootprint(opndNum);
        if (!footprint || (GRFOnly && (footprint->fType != GRF_T)))
        {
            continue;
        }
        getGRFBuckets(node, footprint, opndNum, BDvec, GRFOnly);
    }

    return;
}

bool G4_BB_SB::getGRFFootPrint(SBNode* node, PointsToAnalysis& p)
{
    bool hasDistOneAReg = false;
    //We get the descript for source first, so for current instruction, the scan order is src0, src1, src2, src3, dst
    for (G4_INST* inst : node->instVec)
    {
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_src0, Opnd_src3, p);
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_condMod, Opnd_implAccDst, p);
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_dst, Opnd_dst, p);
    }

    return hasDistOneAReg;
}

void G4_BB_SB::getGRFBucketDescrs(SBNode* node, std::vector<SBBucketDescr>& BDvec, bool GRFOnly)
{
    //We get the descript for source first, so for current instruction, the scan order is src0, src1, src2, src3, dst
    getGRFBucketsForOperands(node, Opnd_src0, Opnd_src3, BDvec, GRFOnly);
    if (!GRFOnly)
    {
        getGRFBucketsForOperands(node, Opnd_condMod, Opnd_implAccDst, BDvec, GRFOnly);
    }
    getGRFBucketsForOperands(node, Opnd_dst, Opnd_dst, BDvec, GRFOnly);

    return;
}


// Clear the killed bucket nodes
// May be killed by 4 ways
// 1. distance > SWSB_MAX_ALU_DEPENDENCE_DISTANCE
// 2. instruction killed.
// 3. source operands killed.
// 4. operand killed.
// FIXME:
// 1. scanning all buckets is time cost.
// 2. some time, only 1 way checking is required.
// 3. the function is called for every instruction, it's compilation time waste.
void G4_BB_SB::clearKilledBucketNodeGen12LP(LiveGRFBuckets* LB, int ALUID)
{
    for (int curBucket = 0; curBucket < LB->getNumOfBuckets(); curBucket++)
    {
        for (LiveGRFBuckets::BN_iterator it = LB->begin(curBucket); it != LB->end(curBucket);)
        {
            SBBucketNode* liveBN = (*it);
            SBNode* curLiveNode = liveBN->node;

            if ((distanceHonourInstruction(curLiveNode->GetInstruction()) &&
                ((ALUID - curLiveNode->getALUID()) > curLiveNode->getMaxDepDistance())) ||
                curLiveNode->isInstKilled() ||
                (curLiveNode->isSourceKilled() &&
                    liveBN->opndNum >= Opnd_src0 &&
                    liveBN->opndNum <= Opnd_src3))
            {
                LB->killOperand(it);
                continue;
            }

            ++it;
        }
    }
}


void G4_BB_SB::setDistance(SBFootprint* footprint, SBNode* node, SBNode* liveNode)
{
    {
        auto dist = node->getALUID() - liveNode->getALUID();
        assert(dist <= liveNode->getMaxDepDistance() && "dist should not exceed the max dep distance");
        node->setDistance(dist);
    }

    return;
}


//The merged footprint is ordered from back to front instructions in the macro
//As a result if killed, is the back instruction killed, which means front instructions are killed as well.
void G4_BB_SB::footprintMerge(SBNode* node, SBNode* nextNode)
{
    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_dst})
    {
        SBFootprint* nextfp = nextNode->getFirstFootprint(opndNum);

        if (nextfp != nullptr)
        {
            node->setFootprint(nextfp, opndNum);
        }
    }

    return;
}


void G4_BB_SB::SBDDD(G4_BB* bb,
    LiveGRFBuckets*& LB,
    LiveGRFBuckets*& globalSendsLB,
    SBNODE_VECT* SBNodes,
    SBNODE_VECT* SBSendNodes,
    SBBUCKET_VECTOR* globalSendOpndList,
    SWSB_INDEXES* indexes,
    uint32_t& globalSendNum,
    PointsToAnalysis& p,
    std::map<G4_Label*, G4_BB_SB*>* LabelToBlockMap)
{
    nodeID = indexes->instIndex;
    ALUID = indexes->ALUIndex;
    SBNODE_LIST tmpSBSendNodes;
    bool hasFollowDistOneAReg = false;

    std::list<G4_INST*>::iterator iInst(bb->begin()), iInstEnd(bb->end()), iInstNext(bb->begin());
    for (; iInst != iInstEnd; ++iInst)
    {
        SBNode* node = nullptr;
        G4_INST* curInst = *iInst;
        iInstNext = iInst;
        iInstNext++;
        G4_INST* nextInst = nullptr;
        if (iInstNext != iInstEnd)
        {
            nextInst = *iInstNext;
        }

        if (curInst->isLabel())
        {
            (*LabelToBlockMap)[curInst->getLabel()] = this;
            continue;
        }

        //For the instructions not counted in the distance, we assign the same ALUID as the following
        node = new (mem)SBNode(nodeID, ALUID, bb->getId(), curInst);
        SBNodes->emplace_back(node);

        //Record the node IDs of the instrucrtions in BB
        if (first_node == -1)
        {
            first_node = nodeID;
        }
        last_node = nodeID;
        nodeID++;

        //For architecture registers ce#, sp, sr0.#, cr0.#, ip, tm0, dbg0, set distance 1
        if (hasFollowDistOneAReg)
        {
            node->setDistance(1);
            node->setFollowDistOneAReg();
            hasFollowDistOneAReg = false;
        }

        hasFollowDistOneAReg = getGRFFootPrint(node, p);

        //For architecture registers ce#, sp, sr0.#, cr0.#, ip, tm0, dbg0, set distance 1
        if (hasFollowDistOneAReg)
        {
            node->setDistance(1);
            node->setDistOneAReg();
        }


        //Get buckets for all GRF registers which are used in curInst
        std::vector<SBBucketDescr> BDvec;
        std::vector<SBBucketDescr> liveBDvec;
        BDvec.clear();
        liveBDvec.clear();

        getGRFBucketDescrs(node, BDvec, false);
        if (node->instVec.size() > 1)
        {
            getGRFBucketDescrs(node, liveBDvec, false);
        }


        // For ALU instructions without GRF usage
        if (distanceHonourInstruction(curInst))
        {
            curInst->setALUID(ALUID);
            ALUID++;


            if (!BDvec.size())
            {
                if (ALUID >= SWSB_MAX_ALU_DEPENDENCE_DISTANCE && ALUID != curInst->getALUID())
                {
                    {
                        clearKilledBucketNodeGen12LP(LB, ALUID);
                    }
                }
                continue;
            }
        }

        // Considering instruction level liveness kill, i.e killing the live instructions/operands,
        // the dependence checking order must be RAR/RAW --> WAR/WAW, the bucket descripters in BDvec must in the order of src->dst.
        // If WAW is done first, RAW may be missed:
        //    If both live and current instructions are in-order instructions, WAW no dependence required, but RAW is required.
        //    If both live and current instructions are out-of-order instructions, WAW and RAW have same effect.
        //    If live is in-order and current is out-of-order, WAW and RAW have same effect.
        //    If live is out-of-order and current is in-order, WAW and RAW have same effect.
        // If RAR is done before WAR, WAR will not be missed:
        //    If both live and current instructions are in-order instructions, both RAR and WAR are not required.
        //    If both live and current instructions are out-of-order instructions,
        //                                   same pipeline, both RAR and WAR are not required
        //                                   different pipeline, both R are kept for RAR, and WAR dependence is required, RAR will not cause WAR miss.
        //    If live is in-order and current is out-of-order, WAW and RAW have same effect.
        //    If live is out-of-order and current is in-order, WAW and RAW have same effect.
        //                                   Both R will be kept, RAR will not cause WAR miss.
        // For WAW and RAW, once explict dependencies are required, kill the liveness of instruction.
        // For WAR, once explict dependencies is required, kill the source operands.
        // Others, only operand kill.
        bool instKill = false;

        // For all bucket descriptors of curInst
        for (const SBBucketDescr& BD : BDvec) {
            const int& curBucket = BD.bucket;
            const Gen4_Operand_Number& curOpnd = BD.opndNum;
            SBFootprint* curFootprint = BD.node->getFootprint(BD.opndNum, BD.inst);

            // Check liveness for each live curBucket node.
            // Add explict dependence if liveness is killed and there is no implicit dependence
            for (LiveGRFBuckets::BN_iterator bn_it = LB->begin(curBucket);
                bn_it != LB->end(curBucket);)
            {
                SBBucketNode* liveBN = (*bn_it);
                SBNode* liveNode = liveBN->node;

                if (liveNode->isInstKilled() ||
                    (liveNode->isSourceKilled() &&
                        liveBN->opndNum >= Opnd_src0 &&
                        liveBN->opndNum <= Opnd_src3))
                {
                    ++bn_it;
                    continue;
                }

                unsigned short internalOffset = 0;
                Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                G4_INST* liveInst = liveBN->inst;
                SBFootprint* liveFootprint = liveNode->getFootprint(liveBN->opndNum, liveInst);

                bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);

                //RAW:                     R kill W    R-->live       explict dependence
                //WAW: same pipeline and inorder   W2 kill W1  W2-->live      implicit dependence
                //WAW: different pipelines or OOO  W2 kill W1  W2-->live      explict dependence
                //WAR: different pipelines W kill R    W-->live       explict dependence
                //WAR: same pipeline       W kill R    W-->live       implict dependence
                //RAR: same pipeline               R2 kill R1  R2-->live      no dependence
                //RAR: different pipelines         no kill     R1,R2-->live   no dependence
                //Find DEP type
                DepType dep = DEPTYPE_MAX;
                dep = getDepForOpnd(liveOpnd, curOpnd);

                //W/A for the read suppression caused issue
                //1)(~f0.0.anyv) math.cos(2 | M0)      r23.7<2>:hf   r11.7<4; 2, 2> : hf{ $14 }
                //2)             mul(8 | M0)               acc0.0<1>:ud  r35.3<8; 8, 0> : ud   r23.0<8; 4, 0> : uw   //With execution mask, only r23.0~r23.3 are read
                //3)             mach(8 | M0)              r52.0<1>:ud   r35.3<8; 8, 0> : ud   r23.0<4; 4, 0> : ud{ $14.dst }
                //FIXME, For performance, we need check the 3th instruction as well

                if (!hasOverlap &&
                    dep == RAW &&
                    (liveNode->getNodeID() + 1) == node->getNodeID() && //Adjacent nodes
                    liveInst->isMath() &&
                    (!hasSamePredicator(liveInst, curInst)))
                {
                    hasOverlap = curFootprint->hasGRFGrainOverlap(liveFootprint);
                }

                if (!hasOverlap)
                {
                    ++bn_it;
                    continue;
                }


                if (tokenHonourInstruction(liveInst))
                {
                    if (dep == RAW || dep == WAW) {
                        {
                            LB->killOperand(bn_it);
                            createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                            liveNode->setInstKilled(true);  //Instrtuction level kill
                            instKill = true;
                            continue;
                        }
                    }

                    if (dep == WAR) {
                        bool killed = false;

                        //Killed if region overlap
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            liveNode->setAR();
                            if (WARDepRequired(liveInst, curInst))
                            {
                                liveNode->setSourceKilled(true);
                            }
                            killed = true;
                        }

                        //Different pipiline/functionID, added Edge
                        //If not whole region overlap, still killed
                        if (WARDepRequired(liveInst, curInst))
                        {
                            if (!killed)
                            {
                                LB->killOperand(bn_it);
                                liveNode->setAR();
                                liveNode->setSourceKilled(true);
                                killed = true;
                            }

                            {
                                createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                            }
                        }  //else, same pipeline, there is no need to set the dependence.

                        if (killed)
                        {
                            continue;
                        }
                    }

                    if (dep == NODEP &&
                        hasSameFunctionID(liveInst, curInst) &&
                        hasSamePredicator(liveInst, curInst) &&
                        hasSameExecMask(liveInst, curInst))
                    {
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            continue;
                        }
                    }
                    assert(dep != DEPTYPE_MAX && "dep unassigned?");
                }

                if (distanceHonourInstruction(liveInst))
                {
                    if (dep == RAW &&
                        curBucket < totalGRFNum)
                    {//Only need track GRF RAW dependence
                        LB->killOperand(bn_it);
                        setDistance(curFootprint, node, liveNode);
                        liveNode->setInstKilled(true);  //Instrtuction level kill
                        instKill = true;
                        continue;
                    }

                    if (dep == WAW) {
                        bool killed = false;
                        //For implict dependence, the previous node can be killed only when it's wholely overlaped by the following one
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            killed = true;
                        }

                            if (!distanceHonourInstruction(curInst)
                                )

                            {
                                if (!killed)
                                {
                                    LB->killOperand(bn_it);
                                    killed = true;
                                }
                                setDistance(curFootprint, node, liveNode);
                                liveNode->setInstKilled(true); //Instrtuction level kill
                                instKill = true;
                            }

                        if (killed)
                        {
                            continue;
                        }
                    }

                    if (dep == WAR) {
                        bool killed = false;
                        //For implict dependence, the previous node can be killed only when it's wholely overlaped by the following one
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            killed = true;
                        }

                            if (!hasSameFunctionID(liveInst, curInst))
                            {
                                if (!killed)
                                {
                                    LB->killOperand(bn_it);
                                    killed = true;
                                }
                                setDistance(curFootprint, node, liveNode);
                                liveNode->setSourceKilled(true);
                            }
                        if (killed)
                        {
                            continue;
                        }
                    }

                    if (dep == NODEP && hasSameFunctionID(liveInst, curInst))
                    {
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            {
                                LB->killOperand(bn_it);
                                continue;
                            }
                        }
                    }
                    assert(dep != DEPTYPE_MAX && "dep unassigned?");
                }

                ++bn_it;
            }
        }


        if ((builder.getOption(vISA_EnableSwitch) && node->GetInstruction()->isYieldInst()) ||
            (node->GetInstruction()->isCall() && !node->GetInstruction()->getSrc(0)->isLabel()))
        {
            node->setDistance(1);
        }

        //Simplify the LB according to the distance, and if the node is killed
        if (instKill ||
            (ALUID >= SWSB_MAX_ALU_DEPENDENCE_DISTANCE && ALUID != curInst->getALUID()))
        {
            {
                clearKilledBucketNodeGen12LP(LB, ALUID);
            }
        }

        // Add buckets of current instruction to bucket list
        std::vector<SBBucketNode*>  bucketNodes(Opnd_total_num, nullptr);  //The coarse grained footprint of operands
        if (node->instVec.size() > 1)
        {
            for (const SBBucketDescr& BD : liveBDvec)
            {
                if (bucketNodes[BD.opndNum] == nullptr)
                {
                    void* allocedMem = mem.alloc(sizeof(SBBucketNode));
                    SBBucketNode* newNode = new(allocedMem)SBBucketNode(node, BD.opndNum, BD.inst);
                    bucketNodes[BD.opndNum] = newNode;
                }

                LB->add(bucketNodes[BD.opndNum], BD.bucket);
            }
        }
        else
        {
            for (const SBBucketDescr& BD : BDvec)
            {
                if (bucketNodes[BD.opndNum] == nullptr)
                {
                    void* allocedMem = mem.alloc(sizeof(SBBucketNode));
                    SBBucketNode* newNode = new(allocedMem)SBBucketNode(node, BD.opndNum, BD.inst);
                    bucketNodes[BD.opndNum] = newNode;
                }

                LB->add(bucketNodes[BD.opndNum], BD.bucket);
            }
        }

        // Record token sensitive nodes.
        if (tokenHonourInstruction(curInst))
        {
            if (first_send_node == -1)
            {
                first_send_node = SBSendNodes->size();
            }
            last_send_node = SBSendNodes->size();
            node->setSendID(int(SBSendNodes->size()));
            SBSendNodes->push_back(node);
        }
    }

    //Check the live out token nodes after the scan of current BB.
    //Record the nodes and the buckets for global analysis.
    for (int curBucket = 0; curBucket < LB->getNumOfBuckets(); curBucket++)
    {
        for (auto it = LB->begin(curBucket); it != LB->end(curBucket);)
        {
            SBBucketNode* liveBN = (*it);
            SBNode* node = liveBN->node;

            //Only the live outs from current BB
            if (tokenHonourInstruction(node->GetInstruction()) &&
                (int)node->getNodeID() >= first_node &&
                (int)node->getNodeID() <= last_node)
            {
                if (liveBN->getSendID() == -1)
                {
                    if (send_start == -1)
                    {
                        send_start = (int)globalSendOpndList->size();
                    }

                    //Record all send operands which live out currnt BB.
                    globalSendOpndList->push_back(liveBN);
                    send_end = (int)globalSendOpndList->size() - 1;

                    //Record the position of the node in global send operands list.
                    liveBN->setSendID(send_end);
                }

                //Set global send instruction ID
                if (liveBN->node->globalID == -1)
                {
                    liveBN->node->globalID = globalSendNum;
                    globalSendNum++;
                }

                //Record all buckets of the send operand
                globalSendsLB->add(liveBN, curBucket);
                LB->killSingleOperand(it);
                continue;
            }
            ++it;
        }
    }

    //return the node ID and ALU ID for following BB
    indexes->ALUIndex = ALUID;
    indexes->instIndex = nodeID;

#ifdef DEBUG_VERBOSE_ON
    std::cerr << "\nLIVE OUT: \n";
    LB->dumpLives();
#endif

    return;
}

//#ifdef DEBUG_VERBOSE_ON

void G4_BB_SB::dumpLiveInfo(SBBUCKET_VECTOR* globalSendOpndList, unsigned globalSendNum, SBBitSets* send_kill)
{
    std::cerr << "\nBB" << bb->getId() << ":" << first_node << "-" << last_node << ", succ<";
    for (std::list<G4_BB*>::iterator sit = bb->Succs.begin(); sit != bb->Succs.end(); ++sit)
    {
        std::cerr << (*sit)->getId() << ",";
    }
    std::cerr << "> pred<";
    for (std::list<G4_BB*>::iterator pit = bb->Preds.begin(); pit != bb->Preds.end(); ++pit)
    {
        std::cerr << (*pit)->getId() << ",";
    }

    std::cerr << "> JIPSucc <";
    for (std::list<G4_BB_SB*>::iterator pit = Succs.begin(); pit != Succs.end(); ++pit)
    {
        std::cerr << (*pit)->getBB()->getId() << ",";
    }
    std::cerr << "> JIPPred <";
    for (std::list<G4_BB_SB*>::iterator pit = Preds.begin(); pit != Preds.end(); ++pit)
    {
        std::cerr << (*pit)->getBB()->getId() << ",";
    }
    std::cerr << ">";
    if (bb->getBBType() & G4_BB_CALL_TYPE)
    {
        std::cerr << ":CALL";
    }
    if (bb->getBBType() & G4_BB_INIT_TYPE)
    {
        std::cerr << ":INIT";
    }
    if (bb->getBBType() & G4_BB_EXIT_TYPE)
    {
        std::cerr << ":EXIT";
    }
    if (bb->getBBType() & G4_BB_RETURN_TYPE)
    {
        std::cerr << ":RETURN";
    }
    std::cerr << std::endl;

    for (size_t i = 0; i < globalSendOpndList->size(); i++)
    {
        SBBucketNode* sNode = (*globalSendOpndList)[i];
        std::cerr << i << ": ";
        sNode->dump();
    }
    std::cerr << std::endl;

    std::cerr << "Live In:  ";
    std::cerr << std::endl;
    if (send_live_in != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_live_in->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;

        std::cerr << "\tsrc:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_live_in->isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "May Kill: ";
    std::cerr << std::endl;
    if (send_may_kill != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_may_kill->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_may_kill->isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "WAW May Kill: ";
    std::cerr << std::endl;
    if (send_WAW_may_kill != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_WAW_may_kill->isSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "Killed:   ";
    std::cerr << std::endl;
    if (send_kill != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_kill->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_kill->isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "Scalar Killed:   ";
    std::cerr << std::endl;
    if (send_kill_scalar != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_kill_scalar->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_kill_scalar->isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "Live Out: ";
    std::cerr << std::endl;
    if (send_live_out != nullptr)
    {
        std::cerr << "\tdst:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum == Opnd_dst &&
                send_live_out->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (size_t i = 0; i < globalSendOpndList->size(); i++)
        {
            SBBucketNode* sNode = (*globalSendOpndList)[i];
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_live_out->isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

}
//#endif

void SWSB::dumpTokenLiveInfo()
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        G4_BB* bb = BBVector[i]->getBB();

        std::cerr << "\nBB" << bb->getId() << ":" << BBVector[i]->first_node << "-" << BBVector[i]->last_node << ", succ<";
        for (std::list<G4_BB*>::iterator sit = bb->Succs.begin(); sit != bb->Succs.end(); ++sit)
        {
            std::cerr << (*sit)->getId() << ",";
        }
        std::cerr << "> pred<";
        for (std::list<G4_BB*>::iterator pit = bb->Preds.begin(); pit != bb->Preds.end(); ++pit)
        {
            std::cerr << (*pit)->getId() << ",";
        }

        std::cerr << "> JIPSucc <";
        for (std::list<G4_BB_SB*>::iterator pit = BBVector[i]->Succs.begin(); pit != BBVector[i]->Succs.end(); ++pit)
        {
            std::cerr << (*pit)->getBB()->getId() << ",";
        }
        std::cerr << "> JIPPred <";
        for (std::list<G4_BB_SB*>::iterator pit = BBVector[i]->Preds.begin(); pit != BBVector[i]->Preds.end(); ++pit)
        {
            std::cerr << (*pit)->getBB()->getId() << ",";
        }
        std::cerr << ">";
        if (bb->getBBType() & G4_BB_CALL_TYPE)
        {
            std::cerr << ":CALL";
        }
        if (bb->getBBType() & G4_BB_INIT_TYPE)
        {
            std::cerr << ":INIT";
        }
        if (bb->getBBType() & G4_BB_EXIT_TYPE)
        {
            std::cerr << ":EXIT";
        }
        if (bb->getBBType() & G4_BB_RETURN_TYPE)
        {
            std::cerr << ":RETURN";
        }
        std::cerr << std::endl;

        if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
            fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
        {
            std::cerr << "Doms: ";

            for (size_t k = 0; k < BBVector.size(); k++)
            {
                if (k != i &&
                    BBVector[i]->dominators->isSet(k))
                {
                    std::cerr << "#BB" << k << ", ";
                }
            }
            std::cerr << std::endl;
        }

        std::cerr << "Live Out: ";
        std::cerr << std::endl;
        if (BBVector[i]->liveOutTokenNodes != nullptr)
        {
            for (SBNODE_VECT_ITER node_it = SBSendNodes.begin();
                node_it != SBSendNodes.end();
                node_it++)
            {
                SBNode* node = (*node_it);
                if (BBVector[i]->liveOutTokenNodes->isSet(node->sendID))
                {
                    std::cerr << " #" << node->getNodeID() << ":" << node->sendID << ":" << node->GetInstruction()->getToken();
                }
            }
            std::cerr << std::endl;
        }

        std::cerr << "Killed Tokens: ";
        std::cerr << std::endl;
        if (BBVector[i]->killedTokens != nullptr)
        {
            uint32_t totalTokenNum = kernel.getNumSWSBTokens();
            for (uint32_t k = 0; k < totalTokenNum; k++)
            {
                if (BBVector[i]->killedTokens->isSet(k))
                {
                    std::cerr << " #" << k << ", ";
                }
            }
        }
        std::cerr << std::endl;

    }

    return;
}

void G4_BB_SB::getLiveBucketsFromFootprint(SBFootprint* firstFootprint, SBBucketNode* sBucketNode, LiveGRFBuckets* send_use_kills)
{
    SBFootprint* footprint = firstFootprint;
    int aregOffset = totalGRFNum;

    while (footprint)
    {
        int startBucket = footprint->LeftB / G4_GRF_REG_NBYTES;
        int endBucket = footprint->RightB / G4_GRF_REG_NBYTES;
        if (footprint->fType == ACC_T)
        {
            startBucket = startBucket + aregOffset;
            endBucket = endBucket + aregOffset;
        }
        else if (footprint->fType == FLAG_T)
        {
            startBucket = footprint->LeftB + aregOffset + builder.kernel.getNumAcc();
            endBucket = footprint->RightB + aregOffset + builder.kernel.getNumAcc();
        }

        for (int j = startBucket; j < endBucket + 1; j++)
        {
            send_use_kills->add(sBucketNode, j);
        }
        footprint = footprint->next;
    }

    return;
}

/*
* Note that the fall through dependencies are captured in the SBDDD linear scan already
*/
void SWSB::addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite)
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        //Get global send operands killed by current BB
        SBBitSets send_kill(mem, globalSendNum);
        send_kill |= *(BBVector[i]->send_live_in);
        send_kill &= *(BBVector[i]->send_may_kill);

#ifdef DEBUG_VERBOSE_ON
        BBVector[i]->dumpLiveInfo(globalSendOpndList, globalSendNum, &send_kill);
#endif
        //Change the global send operands into live bucket for liveness scan
        //Instruction level liveness kill:
        //   For token dependence, thereis only implicit RAR and WAR dependencies.
        //   the order of the operands are scanned is not an issue anymore.
        //   i.e explicit RAW and WAW can cover all other dependences.
        LiveGRFBuckets send_use_kills(mem, kernel.getNumRegTotal(), BBVector[i]->getBB()->getKernel());
        for (size_t j = 0; j < globalSendOpndList->size(); j++)
        {
            SBBucketNode* sBucketNode = (*globalSendOpndList)[j];
            SBNode* sNode = sBucketNode->node;
            if (send_kill.isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                sBucketNode->opndNum == Opnd_src1 ||
                sBucketNode->opndNum == Opnd_src2 ||
                sBucketNode->opndNum == Opnd_src3))
            {
                BBVector[i]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
            }
            if (send_kill.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
            {
                BBVector[i]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
            }
            sNode->setInstKilled(false);
            sNode->setSourceKilled(false);
        }

        if (BBVector[i]->first_node == -1)
        {
            continue;
        }

        //Scan BB again to figure out the dependence caused by global send operands
        std::vector<SBBucketDescr> BDvec;
        for (int j = BBVector[i]->first_node; j <= BBVector[i]->last_node; j++)
        {
            SBNode* node = (*SBNodes)[j];
            G4_INST* curInst = (*SBNodes)[j]->getLastInstruction();

            BDvec.clear();
            BBVector[i]->getGRFBucketDescrs(node, BDvec, true);
            if (!BDvec.size())
            {
                continue;
            }

            bool instKill = false;
            // For all bucket descriptors of curInst
            for (const SBBucketDescr& BD : BDvec)
            {
                const int& curBucket = BD.bucket;
                const Gen4_Operand_Number& curOpnd = BD.opndNum;
                SBFootprint* curFootprint = BD.node->getFootprint(BD.opndNum, BD.inst);

                for (LiveGRFBuckets::BN_iterator bn_it = send_use_kills.begin(curBucket);
                    bn_it != send_use_kills.end(curBucket);)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                    G4_INST* liveInst = liveBN->inst;
                    SBFootprint* liveFootprint = curLiveNode->getFootprint(liveBN->opndNum, liveInst);
                    unsigned short internalOffset = 0;
                    bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);

                    //Find DEP type
                    DepType dep = DEPTYPE_MAX;
                    dep = getDepForOpnd(liveOpnd, curOpnd);


                    //RAW:                     R kill W    R-->live       explict dependence
                    //WAW:                     W2 kill W1  W2-->live      explict dependence
                    //WAW: same pipeline/inorder W2 kill W1  W2-->live      implicit dependence
                    //WAR: different pipelines W kill R    W-->live       explict dependence
                    //WAR: same pipeline       W kill R    W-->live       implict dependence
                    //RAR: sample pipeline     R2 kill R1  R2-->live      implict dependence
                    //RAR: different pipelines   no kill     R1,R2-->live   no dependence
                    if (hasOverlap)
                    {
                        assert(tokenHonourInstruction(liveInst));
                        if (dep == RAW || dep == WAW)
                        {
                            if (BBVector[i]->isGRFEdgeAdded(curLiveNode, node, dep, DEP_EXPLICT))
                            {
                                send_use_kills.killOperand(bn_it);
                                curLiveNode->setInstKilled(true);  //Instrtuction level kill
                                instKill = true;
                                continue;
                            }
                            //WAW need be tracked in both scalar and SIMD control flow
                            //The reason is that:
                            // 1. RA track the liveness in use-->define way
                            // 2. SWSB track  in define-->use way.
                            // For the case like following
                            //
                            //   if
                            //    v1 <--    //v1 is never be used
                            //    if
                            //       <--v1
                            //    endif
                            //   endif
                            //   v2 <--
                            //RA may assign same register to v1 and v2.
                            //Scalar CFG cannot capture the dependence v1-->v2 when they are assigned with same registers.
                            if (afterWrite || dep == WAW)  //There is no RAW kill for SIMDCF
                            {
                                {
                                    send_use_kills.killOperand(bn_it);
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                    curLiveNode->setInstKilled(true);  //Instrtuction level kill
                                    instKill = true;
                                    continue;
                                }
                            }
                        }

                        if (dep == WAR)
                        {
                            bool killed = false;
                            //For implict dependence, the previous node can be killed only when it's wholely overlaped by the following one
                            if (curFootprint->isWholeOverlap(liveFootprint))
                            {
                                send_use_kills.killOperand(bn_it);
                                if (WARDepRequired(liveInst, curInst))
                                    //Implicit dependence cannot block the following instruction from issue.
                                {
                                    curLiveNode->setSourceKilled(true);
                                }
                                curLiveNode->setAR();
                                killed = true;
                            }

                            if (WARDepRequired(liveInst, curInst))
                            {
                                if (!killed)
                                {
                                    send_use_kills.killOperand(bn_it);
                                    curLiveNode->setSourceKilled(true);
                                    curLiveNode->setAR();
                                    killed = true;
                                }
                                instKill = true;
                                if (!afterWrite) //After read dependence is more comprehensive in SIMDCF, so add edge only in SIMDCF pass
                                {
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                }
                            }
                            else
                            {
                                if (!afterWrite) //After read dependence is more comprehensive in SIMDCF, so add edge only in SIMDCF pass
                                {
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_IMPLICIT);
                                }
                            }

                            if (killed)
                            {
                                continue;
                            }
                        }

                        if (dep == NODEP &&
                            hasSameFunctionID(liveInst, curInst) &&
                            hasSamePredicator(liveInst, curInst) &&
                            hasSameExecMask(liveInst, curInst))
                        {
                            if (curFootprint->isWholeOverlap(liveFootprint))
                            {
                                send_use_kills.killOperand(bn_it);
                                continue;
                            }
                        }
                    }

                    assert(dep != DEPTYPE_MAX && "dep unassigned?");
                    ++bn_it;
                }
            }

            if (instKill)
            {
                {
                    BBVector[i]->clearKilledBucketNodeGen12LP(&send_use_kills, 0);
                }
            }
        }
    }

    return;
}

void SWSB::addReachingDefineSet(SBNode* node, SBBitSets* globalLiveSet, SBBitSets* localLiveSet)
{
    if (node->reachingSends == nullptr)
    {
        node->reachingSends = new (mem)SBBitSets(mem, SBSendNodes.size());
    }

    *node->reachingSends |= *globalLiveSet;

    *node->reachingSends |= *localLiveSet;

    return;
}

void SWSB::addReachingUseSet(SBNode* node, SBNode* use)
{
    if (use->getSendUseID() != -1)
    {
        if (node->reachedUses == nullptr)
        {
            node->reachedUses = new (mem)SBBitSets(mem, SBSendUses.size());
        }

        node->reachedUses->setDst(use->getSendUseID(), true);
    }

    return;
}

void SWSB::addGlobalDependenceWithReachingDef(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite)
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        //Get global send operands killed by current BB
        SBBitSets send_kill(mem, globalSendNum);
        //send_live record the live ones from out side of BB, but kill by BB
        SBBitSets send_live(mem, SBSendNodes.size());

        SBBitSets send_live_through(mem, globalSendNum);
        //send_reach_all record all the global livs live through the BB
        SBBitSets send_reach_all(mem, SBSendNodes.size());

        send_kill |= *(BBVector[i]->send_live_in);
        send_kill &= *(BBVector[i]->send_may_kill);
        send_live_through |= *(BBVector[i]->send_live_in);
        send_live_through -= send_kill;

#ifdef DEBUG_VERBOSE_ON
        BBVector[i]->dumpLiveInfo(globalSendOpndList, globalSendNum, &send_kill);
#endif
        //Change the global send operands into live bucket for liveness scan
        //Instruction level liveness kill:
        //   For token dependence, thereis only implicit RAR and WAR dependencies.
        //   the order of the operands are scanned is not an issue anymore.
        //   i.e explicit RAW and WAW can cover all other dependences.
        LiveGRFBuckets send_use_kills(mem, kernel.getNumRegTotal(), BBVector[i]->getBB()->getKernel());
        for (size_t j = 0; j < globalSendOpndList->size(); j++)
        {
            SBBucketNode* sBucketNode = (*globalSendOpndList)[j];
            SBNode* sNode = sBucketNode->node;
            if (send_kill.isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                sBucketNode->opndNum == Opnd_src1 ||
                sBucketNode->opndNum == Opnd_src2 ||
                sBucketNode->opndNum == Opnd_src3))
            {
                BBVector[i]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
                send_live.setSrc(sNode->getSendID(), true);
            }
            if (send_kill.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
            {
                BBVector[i]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
                send_live.setDst(sNode->getSendID(), true);
            }

            if (send_live_through.isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                sBucketNode->opndNum == Opnd_src1 ||
                sBucketNode->opndNum == Opnd_src2 ||
                sBucketNode->opndNum == Opnd_src3))
            {
                send_reach_all.setSrc(sNode->getSendID(), true);
            }
            if (send_live_through.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
            {
                send_reach_all.setDst(sNode->getSendID(), true);
            }
            sNode->setInstKilled(false);
            sNode->setSourceKilled(false);
        }

        if (BBVector[i]->first_node == -1)
        {
            continue;
        }

        BBVector[i]->localReachingSends = new (mem)SBBitSets(mem, SBSendNodes.size());

        if (BBVector[i]->first_send_node != -1)
        {
            for (int j = BBVector[i]->first_send_node; j <= BBVector[i]->last_send_node; j++)
            {
                SBNode* node = SBSendNodes[j];

                //Get the live range for the local ones
                if (node->globalID == -1)
                {
                    assert(node->getBBID() == i);

                    node->setLiveEarliesID(node->getNodeID());
                    node->setLiveLatestID(node->getNodeID());
                    for (int k = 0; k < (int)(node->succs.size()); k++)
                    {
                        SBDEP_ITEM& curSucc = node->succs[k];
                        SBNode* succ = curSucc.node;

                        node->setLiveLatestID(succ->getNodeID(), succ->getBBID());
                    }
                }
                else
                {
                    node->setLiveEarliesID(node->getNodeID());
                    node->setLiveLatestID(BBVector[i]->last_node);
                }
            }
        }
        localTokenUsage.clear(); //Add to the live node

        //Scan BB again to figure out the dependence caused by global send operands
        std::vector<SBBucketDescr> BDvec;
        for (int j = BBVector[i]->first_node; j <= BBVector[i]->last_node; j++)
        {
            SBNode* node = (*SBNodes)[j];
            G4_INST* curInst = (*SBNodes)[j]->getLastInstruction();

            BDvec.clear();
            BBVector[i]->getGRFBucketDescrs(node, BDvec, true);
            if (!BDvec.size())
            {
                continue;
            }

            //Tack all the token nodes defined in current BB
            if (tokenHonourInstruction(node->GetInstruction()))
            {
                addReachingDefineSet(node, &send_live, BBVector[i]->localReachingSends);
                *node->reachingSends |= send_reach_all;

                expireLocalIntervals(node->getNodeID(), i);
                if (node->GetInstruction()->getDst() != nullptr)
                {
                    BBVector[i]->localReachingSends->setDst(node->sendID, true);
                }
                else
                {
                    BBVector[i]->localReachingSends->setSrc(node->sendID, true);
                }
                localTokenUsage.push_back(node); //Add to the live node
            }

            bool instKill = false;
            // For all bucket descriptors of curInst
            for (const SBBucketDescr& BD : BDvec)
            {
                const int& curBucket = BD.bucket;
                const Gen4_Operand_Number& curOpnd = BD.opndNum;
                SBFootprint* curFootprint = BD.node->getFootprint(BD.opndNum, BD.inst);

                for (LiveGRFBuckets::BN_iterator bn_it = send_use_kills.begin(curBucket);
                    bn_it != send_use_kills.end(curBucket);)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                    G4_INST* liveInst = liveBN->inst;
                    SBFootprint* liveFootprint = curLiveNode->getFootprint(liveBN->opndNum, liveInst);
                    unsigned short internalOffset = 0;
                    bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);

                    //Find DEP type
                    DepType dep = DEPTYPE_MAX;
                    dep = getDepForOpnd(liveOpnd, curOpnd);


                    //RAW:                     R kill W    R-->live       explict dependence
                    //WAW:                     W2 kill W1  W2-->live      explict dependence
                    //WAW: same pipeline/inorder W2 kill W1  W2-->live      implicit dependence
                    //WAR: different pipelines W kill R    W-->live       explict dependence
                    //WAR: same pipeline       W kill R    W-->live       implict dependence
                    //RAR: sample pipeline     R2 kill R1  R2-->live      implict dependence
                    //RAR: different pipelines   no kill     R1,R2-->live   no dependence
                    if (hasOverlap)
                    {
                        assert(tokenHonourInstruction(liveInst));
                        if (dep == RAW || dep == WAW)
                        {
                            if (BBVector[i]->isGRFEdgeAdded(curLiveNode, node, dep, DEP_EXPLICT))
                            {
                                send_use_kills.killOperand(bn_it);
                                curLiveNode->setInstKilled(true);  //Instrtuction level kill
                                instKill = true;
                                addReachingDefineSet(node, &send_live, BBVector[i]->localReachingSends);
                                send_live.setDst(curLiveNode->getSendID(), false);
                                continue;
                            }
                            //WAW need be tracked in both scalar and SIMD control flow
                            //The reason is that:
                            // 1. RA track the liveness in use-->define way
                            // 2. SWSB track  in define-->use way.
                            // For the case like following
                            //
                            //   if
                            //    v1 <--    //v1 is never be used
                            //    if
                            //       <--v1
                            //    endif
                            //   endif
                            //   v2 <--
                            //RA may assign same register to v1 and v2.
                            //Scalar CFG cannot capture the dependence v1-->v2 when they are assigned with same registers.
                            if (afterWrite || dep == WAW)  //There is no RAW kill for SIMDCF
                            {
                                {
                                    send_use_kills.killOperand(bn_it);
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                    curLiveNode->setInstKilled(true);  //Instrtuction level kill
                                    instKill = true;

                                    //Kill from live
                                    addReachingDefineSet(node, &send_live, BBVector[i]->localReachingSends);
                                    send_live.setDst(curLiveNode->getSendID(), false);
                                    continue;
                                }
                            }
                        }

                        if (dep == WAR)
                        {
                            bool killed = false;
                            //For implict dependence, the previous node can be killed only when it's wholely overlaped by the following one
                            if (curFootprint->isWholeOverlap(liveFootprint))
                            {
                                send_use_kills.killOperand(bn_it);
                                if (WARDepRequired(liveInst, curInst))
                                    //Implicit dependence cannot block the following instruction from issue.
                                {
                                    curLiveNode->setSourceKilled(true);
                                }
                                curLiveNode->setAR();
                                killed = true;
                            }

                            if (WARDepRequired(liveInst, curInst))
                            {
                                if (!killed)
                                {
                                    send_use_kills.killOperand(bn_it);
                                    curLiveNode->setSourceKilled(true);
                                    curLiveNode->setAR();
                                    killed = true;
                                }
                                instKill = true;
                                if (!afterWrite) //After read dependence is more comprehensive in SIMDCF, so add edge only in SIMDCF pass
                                {
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                }
                            }
                            else
                            {
                                if (!afterWrite) //After read dependence is more comprehensive in SIMDCF, so add edge only in SIMDCF pass
                                {
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_IMPLICIT);
                                }
                            }
                            if (killed)
                            {
                                addReachingDefineSet(node, &send_live, BBVector[i]->localReachingSends);
                                send_live.setSrc(curLiveNode->getSendID(), false);
                                continue;
                            }
                        }

                        if (dep == NODEP &&
                            hasSameFunctionID(liveInst, curInst) &&
                            hasSamePredicator(liveInst, curInst) &&
                            hasSameExecMask(liveInst, curInst))
                        {
                            if (curFootprint->isWholeOverlap(liveFootprint))
                            {
                                send_use_kills.killOperand(bn_it);
                                continue;
                            }
                        }
                    }

                    assert(dep != DEPTYPE_MAX && "dep unassigned?");
                    ++bn_it;
                }
            }

            if (node->preds.size() != 0)
            {
                addReachingDefineSet(node, &send_live, BBVector[i]->localReachingSends);
                *node->reachingSends |= send_reach_all;
                node->setSendUseID(SBSendUses.size());
                SBSendUses.push_back(node);
            }

            if (instKill)
            {
                {
                    BBVector[i]->clearKilledBucketNodeGen12LP(&send_use_kills, 0);
                }
            }
        }
    }

    return;
}

//
//Works only for RAW and WAW
//Check if edge has been added during the data dependence analysis for SIMDCF control flow.
//If it's added, the tracking for the corresponding bucket will be killed
//
bool G4_BB_SB::isGRFEdgeAdded(SBNode* pred, SBNode* succ, DepType d, SBDependenceAttr a)
{
    // When there are mulitple dependence edges between two instructions
    // We think the RAW and WAW > WAR, which means if WAR co-exists with any other, it will be dropped.
    // This is especially important for send instructions. when there are multiple dependencies from same send instruction.
    // For the case like following, only the dst
    //1. Send r2-r5, r8, ....    $1
    //   ...
    //7. Add  r8,  r2, r10   test $1D
    // For WAW and RAW, we think they are equal.
    for (int i = 0; i < (int)(pred->succs.size()); i++)
    {
        SBDEP_ITEM& curSucc = pred->succs[i];
        if (curSucc.node == succ)
        {
            //If there is dependence edges already current edge will be ignored if it's WAR
            //if exist dependence is RAW or WAW, there is no need to add new edges
            if (curSucc.type == RAW || curSucc.type == WAW)
            {
                return true;
            }
        }
    }

    return false;
}

void SWSB::removePredsEdges(SBNode* node, SBNode* pred)
{
    for (auto pred_it = node->preds.begin();
        pred_it != node->preds.end();)
    {
        if ((*pred_it).node == pred)
        {
            pred_it = node->preds.erase(pred_it);
            continue;
        }
        pred_it++;
    }

    return;
}

void G4_BB_SB::createAddGRFEdge(SBNode* pred, SBNode* succ, DepType d, SBDependenceAttr a)
{
    // When there are mulitple dependence edges between two instructions
    // We think the RAW and WAW > WAR, which means if WAR co-exists with any other, it will be dropped.
    // This is especially important for send instructions. when there are multiple dependencies from same send instruction.
    // For the case like following, only the dst
    //1. Send r2-r5, r8, ....    $1
    //   ...
    //7. Add  r8,  r2, r10   test $1D
    // For WAW and RAW, we think they are equal.

    for (int i = 0; i < (int)(pred->succs.size()); i++)
    {
        SBDEP_ITEM& curSucc = pred->succs[i];
        if (curSucc.node == succ)
        {
            //If there is dependence edges already current edge will be ignored if it's WAR
            //if exist dependence is RAW or WAW, there is no need to add new edges
            if (d == WAR || curSucc.type == RAW || curSucc.type == WAW)
            {
                return;
            }
            //Otherwise, d == RAW or d == WAW, but curSucc.type == WAR
            //Change the dependency type to d
            curSucc.type = d;
            curSucc.attr = a;
            bool findPred = false;
            for (int j = 0; j < (int)(succ->preds.size()); j++)
            {
                SBDEP_ITEM& curPred = succ->preds[j];

                if (curPred.node == pred)
                {
                    curPred.type = d;
                    curPred.attr = a;
                    findPred = true;
                }
            }
            assert(findPred);
            return;
        }
    }

    // No edge with the same successor exists. Append this edge.
    SBDEP_ITEM newEdge = SBDEP_ITEM(succ, d, a);
    pred->succs.emplace_back(newEdge);
    newEdge = SBDEP_ITEM(pred, d, a);
    succ->preds.emplace_back(newEdge);
    return;
}


void G4_Kernel::emit_dep(std::ostream& output)
{
    output << "//.platform " << platformString[fg.builder->getPlatform()];
    output << "\n" << "//.stepping " << GetSteppingString();
    output << "\n" << "//.CISA version " << (unsigned int)major_version
        << "." << (unsigned int)minor_version;
    output << "\n" << "//.kernel ID 0x" << hex << getKernelID() << "\n";
    output << "\n" << "//.Bank_Good_Count " << dec << getBankGoodNum() << "\n";
    output << "\n" << "//.Bank_Ok_Count " << dec << getBankOkNum() << "\n";
    output << "\n" << "//.Bank_Bad_Count " << dec << getBankBadNum() << "\n";
    output << dec << "\n";
    int instOffset = 0;

    for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB)
    {
        for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end(); ++itInst)
        {
            G4_INST* inst = (*itInst);
            if (inst->isLabel())
            {
                continue;
            }
            if (inst->getLexicalId() == -1)
            {
                continue;
            }

            (*itBB)->emitDepInfo(output, inst, instOffset);
            instOffset += inst->isCompactedInst() ? 8 : 16;
        }
    }
    return;
}

void G4_BB::emitDepInfo(std::ostream& output, G4_INST* inst, int offset)
{
    int tabnum = 0;

    output << "#" << inst->getLexicalId() << "|" << offset << ":";

    if (inst->tokenHonourInstruction() && ((unsigned short)inst->getToken() != (unsigned short)-1))
    {
        output << " {";
        output << "T:" << (unsigned)inst->getToken();
        output << "}";
    }

    if ((unsigned)inst->getDistance())
    {
        output << " {";
        output << "D:" << (unsigned)inst->getDistance();
        output << "}";
        tabnum++;
    }
    if (inst->getDepTokenNum())
    {
        output << " {";
        output << "DT:";
        for (int i = 0; i < (int)inst->getDepTokenNum(); i++)
        {
            unsigned short token = (unsigned short)-1;
            SWSBTokenType type = SWSBTokenType::TOKEN_NONE;
            token = inst->getDepToken(i, type);
            output << "<";
            output << (unsigned)token,
                output << ",";
            if (type == SWSBTokenType::AFTER_READ)
            {
                output << "S";
            }
            else if (type == SWSBTokenType::AFTER_WRITE)
            {
                output << "D";
            }
            else
            {
                output << "N";
            }
            output << ">";
        }
        output << "}";
        tabnum = 2;
    }

    if (inst->getALUID() != -1)
    {
        int dumptab = 5 - tabnum;
        for (int i = 0; i < dumptab; i++)
        {
            output << "\t";
        }
        output << "//";
        output << "$" << inst->getALUID();
    }
    output << std::endl;
    return;
}

static bool isSWSBRequired(IR_Builder* builder, G4_INST* inst)
{
    // Iterate over all operands and create buckets.
    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_src3, Opnd_dst}) {
        G4_Operand* opnd = inst->getOperand(opndNum);
        // Skip if no operand or the operand is not touched by the instruction
        if (!opnd || !opnd->getBase()) {
            continue;
        }
        if (opnd->isLabel() || opnd->isImm())
        {
            continue;
        }

        G4_VarBase* base = opnd->getBase();
        assert(base && "If no base, then the operand is not touched by the instr.");
        G4_VarBase* phyReg = (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

        if (phyReg->getKind() == G4_VarBase::VK_phyGReg)
        {
            return true;
        }
        if (phyReg->getKind() == G4_VarBase::VK_phyAReg)
        {
            if (phyReg->getAreg()->getArchRegType() == AREG_A0)
            {
                return true;
            }
        }

    }

    return false;
}

static G4_INST* setForceDebugSWSB(IR_Builder* builder, G4_BB* bb, INST_LIST_ITER inst_it)
{
    G4_INST* inst = (*inst_it);
    G4_INST* syncInst = nullptr;

    if (!isSWSBRequired(builder, inst))
    {
        return nullptr;
    }

    inst->setDistance(1);

    if (inst->tokenHonourInstruction())
    {
        inst->setToken(0);

        G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
        syncInst = builder->createSync(G4_sync_nop, src0);
        G4_Operand* opnd = inst->getOperand(Opnd_dst);
        SWSBTokenType tokenType = SWSBTokenType::TOKEN_NONE;
        if (!opnd || !opnd->getBase() || opnd->isNullReg())
        {
            tokenType = SWSBTokenType::AFTER_READ;
        }
        else
        {
            tokenType = SWSBTokenType::AFTER_WRITE;
        }
        syncInst->setDepToken(0, tokenType);
    }

    return syncInst;
}

void vISA::forceDebugSWSB(G4_Kernel* kernel)
{
    BB_LIST_ITER bbEnd = kernel->fg.end();
    int instID = 0;

    for (BB_LIST_ITER bb_it = kernel->fg.begin();
        bb_it != bbEnd;
        bb_it++)
    {
        G4_BB* bb = (*bb_it);
        if (bb->size() > 0)
        {
            INST_LIST_ITER inst_end = bb->end();
            for (INST_LIST_ITER inst_it = bb->begin();
                inst_it != inst_end;
                inst_it++)
            {
                G4_INST* inst = (*inst_it);
                G4_INST* newInst = nullptr;

                newInst = setForceDebugSWSB(kernel->fg.builder, bb, inst_it);
                inst->setLexicalId(instID);
                instID++;

                if (newInst)
                {
                    INST_LIST_ITER new_it = inst_it;
                    new_it++;
                    bb->insert(new_it, newInst);
                    newInst->setLexicalId(instID);
                    instID++;
                }
            }
        }
    }
}

static void setInstructionStallSWSB(IR_Builder* builder,
    G4_BB* bb,
    INST_LIST_ITER& inst_it)
{
    G4_INST* inst = *inst_it;
    INST_LIST_ITER next_it = inst_it;
    next_it++;

    if (!inst->distanceHonourInstruction() &&
        !inst->tokenHonourInstruction())
    {
        return;
    }

    if (inst->distanceHonourInstruction())
    {
        inst->setDistance(1);
        return;
    }

    if (inst->tokenHonourInstruction())
    {

        G4_INST* syncInst = nullptr;
        G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
        syncInst = builder->createSync(G4_sync_nop, src0);

        unsigned short token = inst->getToken();
        SWSBTokenType tokenType = SWSBTokenType::TOKEN_NONE;
        G4_Operand* opnd = inst->getOperand(Opnd_dst);
        if (!opnd || !opnd->getBase() || opnd->isNullReg())
        {
            tokenType = SWSBTokenType::AFTER_READ;
        }
        else
        {
            tokenType = SWSBTokenType::AFTER_WRITE;
        }
        syncInst->setDepToken(token, tokenType);
        inst_it = bb->insert(next_it, syncInst);
    }

    return;
}

static void setInstructionBarrierSWSB(IR_Builder* builder,
    G4_BB* bb,
    INST_LIST_ITER& inst_it)
{

    G4_INST* syncAllRdInst = nullptr;
    G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
    syncAllRdInst = builder->createSync(G4_sync_allrd, src0);
    syncAllRdInst->setDistance(1);
    INST_LIST_ITER next_it = inst_it;
    next_it++;
    inst_it = bb->insert(next_it, syncAllRdInst);

    G4_INST* syncAllWrInst = nullptr;
    src0 = builder->createNullSrc(Type_UD);
    syncAllWrInst = builder->createSync(G4_sync_allwr, src0);

    next_it = inst_it;
    next_it++;
    inst_it = bb->insert(next_it, syncAllWrInst);
}


void vISA::singleInstStallSWSB(G4_Kernel* kernel, uint32_t instID, uint32_t endInstID, bool is_barrier)
{
    BB_LIST_ITER bbEnd = kernel->fg.end();

    for (BB_LIST_ITER bb_it = kernel->fg.begin();
        bb_it != bbEnd;
        bb_it++)
    {
        G4_BB* bb = (*bb_it);

        if (bb->size() > 0)
        {
            INST_LIST_ITER inst_end = bb->end();
            for (INST_LIST_ITER inst_it = bb->begin();
                inst_it != inst_end;
                inst_it++)
            {
                G4_INST* inst = (*inst_it);

                if (is_barrier && inst->getLexicalId() == instID)
                {
                    setInstructionBarrierSWSB(kernel->fg.builder, bb, inst_it);
                }
                else
                {

                    if ((inst->getLexicalId() <= (int)endInstID &&
                        inst->getLexicalId() >= (int)instID) ||
                        (inst->getLexicalId() == instID))
                    {
                        setInstructionStallSWSB(kernel->fg.builder, bb, inst_it);
                    }
                }
            }
        }
    }
}

G4_BB* Dom::InterSect(G4_BB* bb, int i, int k)
{
    G4_BB* finger1 = immDoms[bb->getId()][i];
    G4_BB* finger2 = immDoms[bb->getId()][k];

    while ((finger1 != finger2) &&
        (finger1 != nullptr) &&
        (finger2 != nullptr))
    {
        if (finger1->getPreId() == finger2->getPreId())
        {
            assert(finger1 == kernel.fg.getEntryBB() || finger2 == kernel.fg.getEntryBB());
            return kernel.fg.getEntryBB();
        }

        while ((iDoms[finger1->getId()] != nullptr) &&
            (finger1->getPreId() > finger2->getPreId()))
        {
            finger1 = iDoms[finger1->getId()];
            immDoms[bb->getId()][i] = finger1;
        }

        while ((iDoms[finger2->getId()] != nullptr) &&
            (finger2->getPreId() > finger1->getPreId()))
        {
            finger2 = iDoms[finger2->getId()];
            immDoms[bb->getId()][k] = finger2;
        }

        if ((iDoms[finger2->getId()] == nullptr) ||
            (iDoms[finger1->getId()] == nullptr))
        {
            break;
        }
    }

    if (finger1 == finger2)
    {
        return finger1;
    }
    else if (finger1->getPreId() > finger2->getPreId())
    {
        return finger2;
    }
    else
    {
        return finger1;
    }
}

/*
* An improvement on the algorithm from "A Simple, Fast Dominance Algorithm"
* 1. Single pred assginment.
* 2. To reduce the back trace in the intersect function, a temp buffer for predictor of each nodes is used to record the back trace result.
*/
void Dom::runIDOM()
{
    iDoms.resize(kernel.fg.size());
    immDoms.resize(kernel.fg.size());

    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        iDoms[bb->getId()] = nullptr;
        immDoms[bb->getId()].resize(bb->Preds.size());

        size_t i = 0;
        for (auto pred : bb->Preds)
        {
            immDoms[bb->getId()][i] = pred;
            i++;
        }
    }

    entryBB = kernel.fg.getEntryBB();
    iDoms[entryBB->getId()] = { entryBB };

    // Actual dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
        {
            auto bb = *I;
            if (bb == entryBB)
                continue;

            if (bb->Preds.size() == 1)
            {
                if (iDoms[bb->getId()] == nullptr)
                {
                    iDoms[bb->getId()] = (*bb->Preds.begin());
                    change = true;
                }
                else
                {
                    assert(iDoms[bb->getId()] == (*bb->Preds.begin()));
                }
            }
            else
            {
                G4_BB* tmpIdom = nullptr;
                int i = 0;
                for (auto pred : bb->Preds)
                {
                    if (iDoms[pred->getId()] != nullptr)
                    {
                        tmpIdom = pred;
                        break;
                    }
                    i++;
                }

                if (tmpIdom != nullptr)
                {
                    int k = 0;
                    for (auto pred : bb->Preds)
                    {
                        if (k == i)
                        {
                            k++;
                            continue;
                        }

                        if (iDoms[pred->getId()] != nullptr)
                        {
                            tmpIdom = InterSect(bb, i, k);
                        }
                        k++;
                    }

                    if (iDoms[bb->getId()] == nullptr ||
                        iDoms[bb->getId()] != tmpIdom)
                    {
                        iDoms[bb->getId()] = tmpIdom;
                        change = true;
                    }
                }
            }
        }
    }

    return;
}

void Dom::runDOM()
{
    Doms.resize(kernel.fg.size());
    entryBB = kernel.fg.getEntryBB();

    MUST_BE_TRUE(entryBB != nullptr, "Entry BB not found!");

    Doms[entryBB->getId()] = { entryBB };
    std::unordered_set<G4_BB*> allBBs;
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        allBBs.insert(bb);
    }

    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        if (bb != entryBB)
        {
            Doms[bb->getId()] = allBBs;
        }
    }

    // Actual dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
        {
            auto bb = *I;
            if (bb == entryBB)
                continue;

            std::unordered_set<G4_BB*> tmp = { bb };

            // Compute intersection of dom of preds
            std::unordered_map<G4_BB*, unsigned int> numInstances;

            //
            for (auto preds : bb->Preds)
            {
                auto& domPred = Doms[preds->getId()];
                for (auto domPredBB : domPred)
                {
                    auto it = numInstances.find(domPredBB);
                    if (it == numInstances.end())  //Not found
                        numInstances.insert(std::make_pair(domPredBB, 1));
                    else
                        it->second = it->second + 1;
                }
            }

            // Common BBs appear in numInstances map with second value == bb->Preds count
            for (auto commonBBs : numInstances)
            {
                if (commonBBs.second == bb->Preds.size()) //same size means the bb from all preds.
                    tmp.insert(commonBBs.first);
            }

            // Check if Dom set changed for bb in current iter
            if (tmp.size() != Doms[bb->getId()].size())  //Same size
            {
                Doms[bb->getId()] = tmp;
                change = true;
                continue;
            }
            else //Same
            {
                auto& domBB = Doms[bb->getId()];
                for (auto tmpBB : tmp)
                {
                    if (domBB.find(tmpBB) == domBB.end()) //Same BB
                    {
                        Doms[bb->getId()] = tmp;
                        change = true;
                        break;
                    }
                    if (change)
                        break;
                }
            }
        }
    }

    updateImmDom();
}


std::unordered_set<G4_BB*>& Dom::getDom(G4_BB* bb)
{
    return Doms[bb->getId()];
}

void SWSB::dumpImmDom(Dom* dom)
{
    for (auto I = fg.cbegin(), E = fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        printf("BB%d %d:%d - SUCC:", bb->getId(), BBVector[bb->getId()]->first_node, BBVector[bb->getId()]->last_node);
        for (auto succ : bb->Succs)
        {
            printf("BB%d, ", succ->getId());
        }
        printf("--PRED:");
        for (auto pred : bb->Preds)
        {
            printf("BB%d, ", pred->getId());
        }
        auto& idomBB = dom->iDoms[bb->getId()];
        assert(idomBB != nullptr);
        printf("\n\t iDOM: BB%d -- DOM SUCC: ", dom->iDoms[bb->getId()]->getId());
        for (BB_SWSB_LIST_ITER it = BBVector[bb->getId()]->domSuccs.begin(); it != BBVector[bb->getId()]->domSuccs.end(); it++)
        {
            printf("BB%d, ", (*it)->getBB()->getId());
        }
        printf("\n");
    }
}

std::vector<G4_BB*>& Dom::getImmDom(G4_BB* bb)
{
    return immDoms[bb->getId()];
}

void Dom::updateImmDom()
{
    BitSet** domBits = (BitSet * *)mem.alloc(sizeof(BitSet*) * kernel.fg.size());

    for (size_t i = 0; i < kernel.fg.size(); i++)
    {
        domBits[i] = new (mem)BitSet(unsigned(kernel.fg.size()), false);
    }

    // Update immDom vector with correct ordering
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        auto& DomBBs = Doms[bb->getId()];

        for (auto domBB : DomBBs)
        {
            domBits[bb->getId()]->set(domBB->getId(), true);
        }
    }

    iDoms.resize(kernel.fg.size());
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        auto& DomBBs = Doms[bb->getId()];
        BitSet tmpBits = *domBits[bb->getId()];
        tmpBits.set(bb->getId(), false);
        iDoms[bb->getId()] = bb;

        for (auto domBB : DomBBs)
        {
            if (domBB == bb)
                continue;

            if (tmpBits == *domBits[domBB->getId()])
            {
                iDoms[bb->getId()] = domBB;
            }
        }
    }
}

G4_BB* Dom::getCommonImmDom(std::unordered_set<G4_BB*>& bbs)
{
    if (bbs.size() == 0)
        return nullptr;

    unsigned int maxId = (*bbs.begin())->getId();

    auto commonImmDoms = getImmDom(*bbs.begin());
    for (auto bb : bbs)
    {
        if (bb->getId() > maxId)
            maxId = bb->getId();

        auto& DomBB = Doms[bb->getId()];
        for (unsigned int i = 0, size = commonImmDoms.size(); i != size; i++)
        {
            if (commonImmDoms[i])
            {
                if (DomBB.find(commonImmDoms[i]) == DomBB.end())
                {
                    commonImmDoms[i] = nullptr;
                }
            }
        }
    }

    // Return first imm dom that is not a BB from bbs set
    for (unsigned int i = 0, size = commonImmDoms.size(); i != size; i++)
    {
        if (commonImmDoms[i] &&
            // Common imm pdom must be lexically last BB
            commonImmDoms[i]->getId() >= maxId &&
            ((commonImmDoms[i]->size() > 1 && commonImmDoms[i]->front()->isLabel()) ||
            (commonImmDoms[i]->size() > 0 && !commonImmDoms[i]->front()->isLabel())))
        {
            return commonImmDoms[i];
        }
    }

    return entryBB;
}

bool G4_INST::isDFInstruction() const
{
    G4_Operand* dst = getDst();
    if (dst && (dst->getType() == Type_DF))
    {
        return true;
    }
    for (int i = 0; i < getNumSrc(); i++)
    {
        G4_Operand* src = getSrc(i);
        if (src && (src->getType() == Type_DF))
        {
            return true;
        }
    }
    return false;
}

bool G4_INST::isMathPipeInst() const
{
    if (isMath())
    {
        return true;
    }


    return false;
}

bool G4_INST::distanceHonourInstruction() const
{
    if (isSend() || op == G4_nop || isWait() || isMath())
    {
        return false;
    }
    return true;
}

bool G4_INST::tokenHonourInstruction() const
{
    return isSend() || isMath();
}
