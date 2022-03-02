/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SWSB_G4IR.h"
#include "Dependencies_G4IR.h"
#include "../G4_Opcode.h"
#include "../Timer.h"
#include "../RegAlloc.h"
#include "visa_wa.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <queue>

using namespace vISA;

static uint8_t getDPASPipelineCycle(uint8_t repc)
{
    switch (repc)
    {
    case REP_1:
        return DPAS_8x1_CYCLE;
    case REP_2:
        return DPAS_8x2_CYCLE;
    case REP_4:
        return DPAS_8x4_CYCLE;
    case REP_8:
        return DPAS_8x8_CYCLE;
    default:
        assert(0 && "Unexpected DPAS repeat count");
    }

    return 0;
}

static uint8_t getDPASGRFReadCycle(uint8_t repc)
{
    switch (repc)
    {
    case REP_1:
        return DPAS_8x1_GRFREAD_CYCLE;
    case REP_2:
        return DPAS_8x2_GRFREAD_CYCLE;
    case REP_4:
        return DPAS_8x4_GRFREAD_CYCLE;
    case REP_8:
        return DPAS_8x8_GRFREAD_CYCLE;
    default:
        assert(0 && "Unexpected DPAS repeat count");
    }

    return 0;
}

static bool hasSameFunctionID(const G4_INST* inst1, const G4_INST* inst2)
{
    const bool isInst1Send = inst1->isSend();
    const bool isInst2Send = inst2->isSend();
    if (isInst1Send && isInst2Send)
    {
        G4_SendDesc* msgDesc1 = inst1->getMsgDesc();
        G4_SendDesc* msgDesc2 = inst2->getMsgDesc();
        if (msgDesc1->isSLM() ^ msgDesc2->isSLM())
        {
            return false;
        }

        return msgDesc1->getSFID() == msgDesc2->getSFID();
    }
    if (isInst1Send ^ isInst2Send)
    {
        return false;
    }
    if (inst1->isMathPipeInst() ^ inst2->isMathPipeInst())
    {
        return false;
    }
    if (inst1->isDpas() ^ inst2->isDpas())
    {
        return false;
    }
    return true;
}

static bool isSLMMsg(const G4_INST* inst)
{
    assert(inst->isSend());
    const G4_SendDesc* msgDesc = inst->getMsgDesc();
    if (msgDesc->isSLM())
    {
        return true;
    }
    return false;
}

static bool isPrefetch(const G4_INST* inst)
{
    if(!inst->isSend())
    {
        return false;
    }

    const G4_SendDesc* msgDesc = inst->getMsgDesc();
    if (msgDesc->isRead() && (inst->getDst() == nullptr || inst->getDst()->isNullReg()))
    {
        return true;
    }
    return false;
}

static bool isFence(const G4_INST* inst)
{
    assert(inst->isSend());
    const G4_SendDesc* msgDesc = inst->getMsgDesc();
    if (msgDesc->isFence())
    {
        return true;
    }
    return false;
}

static bool hasSamePredicator(const G4_INST* inst1, const G4_INST* inst2)
{
    G4_Predicate* pred1 = inst1->getPredicate();
    G4_Predicate* pred2 = inst2->getPredicate();

    if (pred1 && pred2)
    {
        bool flagRegNumValid = true;
        unsigned short refOff1 = pred1->getBase()->ExRegNum(flagRegNumValid);
        unsigned short subRefOff1 = pred1->getBase()->asRegVar()->getPhyRegOff();;
        unsigned short refOff2 = pred2->getBase()->ExRegNum(flagRegNumValid);
        unsigned short subRefOff2 = pred2->getBase()->asRegVar()->getPhyRegOff();;

        if (refOff1 == refOff2 &&
            subRefOff1 == subRefOff2)
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

static bool hasSameExecMask(const G4_INST* inst1, const G4_INST* inst2)
{
    uint16_t mask1 = inst1->getMaskOffset();
    uint16_t mask2 = inst2->getMaskOffset();
    if (mask1 != mask2)
    {
        return false;
    }

    unsigned char execSize1 = inst1->getExecSize();
    unsigned char execSize2 = inst2->getExecSize();
    if (execSize1 != execSize2)
    {
        return false;
    }

    return true;
}

static bool WARDepRequired(const G4_INST* inst1, const G4_INST* inst2)
{
    if (!hasSameFunctionID(inst1, inst2))
    {
        return true;
    }
    if (!hasSamePredicator(inst1, inst2))
    {
        return true;
    }
    if (!hasSameExecMask(inst1, inst2))
    {
        return true;
    }
    return false;
}

// check if two operands occupy overlapping GRFs
// we put them here instead of inside G4_Operand since this is only valid till after RA
// It's the caller's responsibility to ensure that opnd1 and opnd2 are both GRF allocated
static bool operandOverlap(G4_Operand* opnd1, G4_Operand* opnd2)
{
    return (opnd1->getLinearizedStart() <= opnd2->getLinearizedStart() &&
        opnd1->getLinearizedEnd() > opnd2->getLinearizedStart()) ||
        (opnd2->getLinearizedStart() <= opnd1->getLinearizedStart() &&
            opnd2->getLinearizedEnd() > opnd1->getLinearizedStart());
}

static G4_Type getDPASDataType(GenPrecision p)
{
    switch (p)
    {
    case GenPrecision::U1:
    case GenPrecision::U2:
    case GenPrecision::U4:
    case GenPrecision::U8:   return Type_UB;
    case GenPrecision::S2:
    case GenPrecision::S4:
    case GenPrecision::S8:   return Type_B;
    case GenPrecision::FP16: return Type_HF;
    case GenPrecision::BF16: return Type_BF;
    case GenPrecision::TF32: return Type_UNDEF;
    default:
        assert(false && "illegal Operand Precision");
        return Type_UD;
    }
}

// Compute the range of registers touched by OPND.
SBFootprint* G4_BB_SB::getFootprintForGRF(
    G4_Operand* opnd,
    Gen4_Operand_Number opnd_num,
    G4_INST* inst,
    int startingBucket,
    bool mustBeWholeGRF)
{
    unsigned short LB = 0;
    unsigned short RB = 0;
    int aregOffset = totalGRFNum;
    G4_Type type = opnd->getType();
    if (inst->opcode() == G4_fcvt &&
        (type == Type_UB ||
        (type == Type_UD && builder.hasPartialInt64Support())))
    {
        type = Type_F;
    }
    if (inst->opcode() == G4_srnd)
    {   // srnd ub  hf  hf | srnd hf f f
        type = inst->getSrc(0)->getType();
    }

    if (inst->isDpas() && (opnd_num == Opnd_src1 || opnd_num == Opnd_src2))
    {
        if (opnd_num == Opnd_src1)
        {
            type = getDPASDataType(inst->asDpasInst()->getSrc1Precision());
        }
        if (opnd_num == Opnd_src2)
        {
            type = getDPASDataType(inst->asDpasInst()->getSrc2Precision());
        }
    }

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
            assert((LB % builder.numEltPerGRF<Type_UB>()) == 0);
            //For the operands of the send instructions,
            //we are using the message length to avoid the in-consistence with the HW requirement.
            //
            if (opnd_num == Opnd_src0)
            {
                RB = LB + builder.numEltPerGRF<Type_UB>() * inst->getMsgDesc()->getSrc0LenRegs() - 1;
            }

            if (inst->isSplitSend() &&
                opnd_num == Opnd_src1)
            {
                RB = LB + builder.numEltPerGRF<Type_UB>() * inst->getMsgDesc()->getSrc1LenRegs() - 1;
            }

            if (opnd_num == Opnd_dst)
            {
                int dstSize = inst->getMsgDesc()->getDstLenRegs();
                // DG2 A0 W/A to treat SIMD8 SLM load with single GRF return as two GRF return
                if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14012562260) &&
                    inst->getExecSize() <= 8 && isSLMMsg(inst) && dstSize == 1)
                {
                    if ((LB / builder.numEltPerGRF<Type_UB>()) < 127)
                    {
                        dstSize = 2;
                    }
                }

                if ((LB / builder.numEltPerGRF<Type_UB>()) < (unsigned short)(totalGRFNum - 1))
                {
                    RB = LB + builder.numEltPerGRF<Type_UB>() * dstSize - 1;
                }
            }

            assert(RB < (builder.numEltPerGRF<Type_UB>() * aregOffset) && "Out of register bound");
        }
        //HW WA for DPAS src2, treat all source 2 as 8x8 source 2 to avoid the read suppression issue
        if (builder.hasDPASSrc2ReadSuppressionDepIssue() &&
            inst->opcode() == G4_dpas && opnd_num == Opnd_src2)
        {
            const G4_InstDpas* dpasInst = inst->asDpasInst();
            uint32_t bytesPerLane = dpasInst->getSrc2SizePerLaneInByte();
            uint32_t bytes = bytesPerLane * 8* 8;
            RB = LB + bytes - 1;
        }

        //HW WA for DPAS src1, treat all source 1 8GRF size
        if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14013341720) &&
            inst->opcode() == G4_dpas && opnd_num == Opnd_src1)
        {
            uint32_t bytes = builder.numEltPerGRF<Type_UB>() * 8;
            RB = LB + bytes - 1;
        }
        break;
    default:
        assert(0 && "Bad opnd");
    }

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    if (startingBucket >= aregOffset)
    {
        LB = startingBucket * builder.numEltPerGRF<Type_UB>() + LB;
        RB = startingBucket * builder.numEltPerGRF<Type_UB>() + RB;
    }

    //This is WA which assumes whole GRF will be touched in send instruction, not matter the occupation of real valid value.
    //FIXME: But this is not true in media block read/write, which can specify the byte level size in descriptor, no GRF align required.
    if (mustBeWholeGRF)
    {
        LB = (LB / builder.numEltPerGRF<Type_UB>()) * builder.numEltPerGRF<Type_UB>();
        RB = ((RB / builder.numEltPerGRF<Type_UB>()) + 1) * builder.numEltPerGRF<Type_UB>() - 1;
    }

    SBFootprint* footprint = new (allocedMem)SBFootprint(GRF_T, type, LB, RB, inst);

    return footprint;
}

bool needBothAcc(IR_Builder& builder, G4_INST* inst, G4_Operand * opnd)
{
    switch (opnd->getType())
    {
    case Type_F:
        return inst->getExecSize() == G4_ExecSize(builder.getNativeExecSize() * 2);
    case Type_HF:
    case Type_BF:
        return false;
    case Type_DF:
        return inst->getExecSize() > G4_ExecSize(builder.getNativeExecSize() / 2);
    default:
        return true;
    }
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

    if (needBothAcc(builder, inst, opnd))
    {
        if (((RB - LB + 1) / builder.numEltPerGRF<Type_UB>()) < 2)
        {
            RB = LB + builder.numEltPerGRF<Type_UB>() * 2 - 1;
        }
    }
    int regNum = 0;
    if (opnd->isDstRegRegion())
        regNum = opnd->asDstRegRegion()->getRegOff();
    else if (opnd->isSrcRegRegion())
        regNum = opnd->asSrcRegRegion()->getRegOff();

    regNum += builder.kernel.getNumRegTotal() + builder.getNumScalarRegisters();

    LB += regNum * builder.numEltPerGRF<Type_UB>();
    RB += regNum * builder.numEltPerGRF<Type_UB>();

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    SBFootprint* footprint = nullptr;

    footprint = new (allocedMem)SBFootprint(ACC_T, type, LB, RB, inst);

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
    //1 GRF map to 1 flag: bytes per bit
    unsigned FLAG_TO_GRF_MAP = builder.numEltPerGRF<Type_UB>() / 16;
    unsigned short LB = 0;
    unsigned short RB = 0;
    G4_Type type = opnd->getType();
    bool valid = true;
    unsigned subRegOff = opnd->getBase()->ExSubRegNum(valid);
    LB = (unsigned short)(opnd->getLeftBound() + subRegOff * 16) * FLAG_TO_GRF_MAP;
    RB = (unsigned short)(opnd->getRightBound() + subRegOff * 16) * FLAG_TO_GRF_MAP;

    LB += (builder.kernel.getNumRegTotal() + builder.getNumScalarRegisters() + builder.kernel.getNumAcc()) * builder.numEltPerGRF<Type_UB>();
    RB += (builder.kernel.getNumRegTotal() + builder.getNumScalarRegisters() + builder.kernel.getNumAcc()) * builder.numEltPerGRF<Type_UB>();

    void* allocedMem = mem.alloc(sizeof(SBFootprint));
    SBFootprint* footprint = nullptr;

    footprint = new (allocedMem)SBFootprint(FLAG_T, type, LB, RB, inst);

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
static inline bool hasIndirection(const G4_Operand* opnd, Gen4_Operand_Number opndNum) {
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

static inline bool distanceHonourInstruction(const G4_INST* inst)
{
    return !inst->tokenHonourInstruction() && !inst->isWait() && inst->opcode() != G4_nop && inst->opcode() != G4_halt;
}

static inline bool tokenHonourInstruction(const G4_INST* inst)
{
    return inst->tokenHonourInstruction();
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
                if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
                {
                    (*it)->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                }
                if (fg.builder->getFCPatchInfo()->getFCComposableKernel() && fg.builder->hasFourALUPipes())
                {
                    insertSyncAllWRInstruction(bb, 0, it, (*it)->getCISAOff(), (*it)->getLineNo());
                    insertSyncAllRDInstruction(bb, 0, it, (*it)->getCISAOff(), (*it)->getLineNo());
                }

                return;
            }
        }
    }
}

void SWSB::addSIMDEdge(G4_BB_SB* pred, G4_BB_SB* succ)
{
    pred->Succs.push_back(succ);
    succ->Preds.push_back(pred);
}

// Build SIMD CFG for the global WAR dependence tracking
// 1. When building CFG, except the backedge, all using JIP branch edge.
// 2. For the join and endif instructions which are no separated and place in the head of a BB. We do edge propagation
//    Such as:   BB a, b, c, d,  there is a join in BB b which JIP to d, and there is an edge from a to b, we will add edge from a to d, instead of b to d.
void SWSB::SWSBBuildSIMDCFG()
{
    //Build parallel control flow graph
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        G4_BB_SB* currBB = BBVector[i];
        const G4_INST* lastInst = currBB->getBB()->back();
        for (const G4_INST* firstInst : *currBB->getBB())
        {
            if (firstInst->isLabel())
                continue;

            if (firstInst != lastInst &&
                G4_Inst_Table[firstInst->opcode()].instType == InstTypeFlow)
            {
                if (firstInst->asCFInst()->getJip())
                {
                    G4_Operand* jip = firstInst->asCFInst()->getJip();
                    G4_BB_SB* targetBB = labelToBlockMap[jip->asLabel()];

                    //Do we need to propagate edge for fall through preds?
                    for (G4_BB_SB* predBB : currBB->Preds)
                    {
                        addSIMDEdge(predBB, targetBB);
                    }
                }
            }
            break;
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
            else if (lastInst->isReturn() || lastInst->isCall() ||
                lastInst->isFReturn() || lastInst->isFCall())
            {
                for (const G4_BB* bb : currBB->getBB()->Succs)
                {
                    unsigned bbID = bb->getId();
                    addSIMDEdge(currBB, BBVector[bbID]);
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

    //Initialize global data
    BBVector.resize(fg.size());

    //Set distance 1 at the first instruction in case there are runtime inserted instructions at prolog
    if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D ||
        fg.builder->getOptions()->getOption(vISA_SWSBStitch) )
    {
        setDefaultDistanceAtFirstInstruction();
    }

    unsigned nestLoopLevel = 0;
    //Local dependence analysis
    for (; ib != bend; ++ib)
    {
        BBVector[(*ib)->getId()] = new (mem)G4_BB_SB(
            *this,
            *(fg.builder),
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
            &labelToBlockMap,
            tokenAfterDPASCycle);
        if ((*ib)->getNestLevel())
        {
            nestLoopLevel = nestLoopLevel < (*ib)->getNestLevel() ? (*ib)->getNestLevel() : nestLoopLevel;
        }
    }
}

void SWSB::handleFuncCall()
{
    for (G4_BB_SB *bb : BBVector)
    {
        if (bb->last_node == -1)
        {
            continue;
        }

        SBNode* node = SBNodes[bb->last_node];

        if ((node->GetInstruction()->isCall() || node->GetInstruction()->isFCall()) ||
            (node->GetInstruction()->isReturn() || node->GetInstruction()->isFReturn()))
        {
            LiveGRFBuckets send_use_out(mem, kernel.getNumRegTotal());
            for (const SBBucketNode* sBucketNode : globalSendOpndList)
            {
                SBNode* sNode = sBucketNode->node;
                if (bb->send_live_out.isSrcSet(sNode->globalID) &&
                    (sBucketNode->opndNum == Opnd_src0 ||
                    sBucketNode->opndNum == Opnd_src1 ||
                    sBucketNode->opndNum == Opnd_src2 ||
                    sBucketNode->opndNum == Opnd_src3))
                {
                    bb->createAddGRFEdge(sNode, node, WAR, DEP_EXPLICT);
                }
                if (bb->send_live_out.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
                {
                    bb->createAddGRFEdge(sNode, node, RAW, DEP_EXPLICT);
                }
            }
        }
        if (node->GetInstruction()->isReturn() ||
            node->GetInstruction()->isFReturn())
        {
            node->GetInstruction()->setDistance(1);
            if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
            {
                node->GetInstruction()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            }
        }
    }
}

void SWSB::SWSBGlobalTokenGenerator(PointsToAnalysis& p, LiveGRFBuckets& LB, LiveGRFBuckets& globalSendsLB)
{
    allTokenNodesMap.resize(totalTokenNum);
    for (TokenAllocation& nodeMap : allTokenNodesMap)
    {
        nodeMap.bitset = BitSet(SBSendNodes.size(), false);
    }

    const bool enableGlobalTokenAllocation = fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation);
    const bool enableDistPropTokenAllocation = fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation);
    // Get the live out, may kill bit sets
    for (G4_BB_SB *bb : BBVector)
    {
        bb->send_live_in = SBBitSets(globalSendNum);
        bb->send_live_out = SBBitSets(globalSendNum);
        bb->send_def_out = SBBitSets(globalSendNum);

        bb->send_live_in_scalar = SBBitSets(globalSendNum);
        bb->send_live_out_scalar = SBBitSets(globalSendNum);
        bb->send_kill_scalar = SBBitSets(globalSendNum);
        bb->liveInTokenNodes = BitSet(SBSendNodes.size(), false);
        bb->liveOutTokenNodes = BitSet(SBSendNodes.size(), false);
        bb->killedTokens = BitSet(totalTokenNum, false);

        if (enableGlobalTokenAllocation || enableDistPropTokenAllocation)
        {
            bb->tokenLiveInDist = (unsigned*)mem.alloc(sizeof(unsigned) * globalSendNum);
            bb->tokenLiveOutDist = (unsigned*)mem.alloc(sizeof(unsigned) * globalSendNum);
            for (unsigned k = 0; k < globalSendNum; k++)
            {
                bb->tokenLiveInDist[k] = -1;
                bb->tokenLiveOutDist[k] = -1;
            }
        }
        if (bb->send_start != -1)
        {
            for (int k = bb->send_start; k <= bb->send_end; k++)
            {
                if (globalSendOpndList[k]->opndNum == Opnd_dst)
                {
                    bb->send_def_out.setDst(globalSendOpndList[k]->node->globalID, true);
                    bb->send_live_out.setDst(globalSendOpndList[k]->node->globalID, true);
                }
                if (globalSendOpndList[k]->opndNum == Opnd_src0 ||
                    globalSendOpndList[k]->opndNum == Opnd_src1 ||
                    globalSendOpndList[k]->opndNum == Opnd_src2 ||
                    globalSendOpndList[k]->opndNum == Opnd_src3)
                {
                    bb->send_def_out.setSrc(globalSendOpndList[k]->node->globalID, true);
                    bb->send_live_out.setSrc(globalSendOpndList[k]->node->globalID, true);
                }
            }
        }

        bb->send_may_kill = SBBitSets(globalSendNum);
        bb->send_WAW_may_kill = BitSet(globalSendNum, false);
        bb->setSendOpndMayKilled(&globalSendsLB, &SBNodes, p);

#ifdef DEBUG_VERBOSE_ON
        bb->dumpLiveInfo(&globalSendOpndList, globalSendNum, nullptr);
#endif
    }

    /*
    Loop info is used to reduce the token required for certain instructions, or count the delay of the backedge for token reuse
    We do the token reduction and count delay of backedge only for the nature loops, i.e with the backedge,
    if the instruction distance is far enough, there is no need to set dependence.
    For the irreducible flow graph, those optimizations wouldn't be kicked in.
    */
    for (G4_BB_SB *bb : BBVector)
    {
        for (auto&& be : kernel.fg.backEdges)
        {
            auto loopIt = kernel.fg.naturalLoops.find(be);

            if (loopIt != kernel.fg.naturalLoops.end())
            {
                auto&& bbsInLoop = loopIt->second;

                auto bb1InLoop = bbsInLoop.find(bb->getBB());
                if (bb1InLoop != bbsInLoop.end())
                {
                    if (bb->getLoopStartBBID() != -1)
                    {
                        //Innermost loop only
                        if (bb->getLoopStartBBID() <= be.second->getId() &&
                            bb->getLoopEndBBID() >= be.first->getId())
                        {
                            bb->setLoopStartBBID(be.second->getId());
                            bb->setLoopEndBBID(be.first->getId());
                        }
                    }
                    else
                    {
                        bb->setLoopStartBBID(be.second->getId());
                        bb->setLoopEndBBID(be.first->getId());
                    }
                }
            }
        }
    }

    //Global analysis until no live in change
    SWSBGlobalScalarCFGReachAnalysis();

    //Add dependence according to analysis result
    if (enableGlobalTokenAllocation || enableDistPropTokenAllocation)
    {
        addGlobalDependenceWithReachingDef(globalSendNum, &globalSendOpndList, &SBNodes, p, true);
    }
    else
    {
        addGlobalDependence(globalSendNum, &globalSendOpndList, &SBNodes, p, true);
    }

    handleFuncCall();

    for (G4_BB_SB *bb : BBVector)
    {
        bb->send_live_in_scalar = bb->send_live_in;
        bb->send_live_out_scalar = bb->send_live_out;
    }

    SWSBBuildSIMDCFG();

    SWSBGlobalSIMDCFGReachAnalysis();

    //Add dependence according to analysis result
    addGlobalDependence(globalSendNum, &globalSendOpndList, &SBNodes, p, false);

    //SWSB token allocation with linear scan algorithm.
    if (enableGlobalTokenAllocation)
    {
        tokenAllocationGlobal();
    }
    else if (enableDistPropTokenAllocation)
    {
        tokenAllocationGlobalWithPropogation();
    }
    else if (fg.builder->getOptions()->getOption(vISA_QuickTokenAllocation))
    {
        quickTokenAllocation();
    }
    else
    {
        tokenAllocation();
    }

    //Insert test instruction in case the dependences are more than token field in the instruction.
    insertTest();
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
        SFID = SFIDtoInt(Inst->getMsgDesc()->getSFID()) & 0xF; // 4-bit SFID
    }
    else if (Inst->isMathPipeInst())
    {
        Pipe = FCPatchingInfo::Pipe_Math;
    }
    else if (Inst->isDpas())
    {
        Pipe = FCPatchingInfo::Pipe_Dpas;
    }

    // Pipe ID is encoded as (SFID[3:0] | P[3:0]), where P is ALU, Math, or Send.
    return unsigned(Pipe) | (SFID << 4);
}

static void updateRegAccess(FCPatchingInfo* FCPI, SBNode* Node,
    Gen4_Operand_Number OpndNo, unsigned NumRegs) {
    const IR_Builder& builder = Node->GetInstruction()->getBuilder();
    for (auto F = Node->getFirstFootprint(OpndNo); F != nullptr; F = F->next) {
        unsigned L = F->LeftB / builder.numEltPerGRF<Type_UB>();
        unsigned R = F->RightB / builder.numEltPerGRF<Type_UB>();
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
            Acc.Token = Acc.Inst->getSetToken();
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
        auto T = Inst->getSetToken();
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
    for (auto Node : SBNodes) {
        updatePatchInfo(FCPI, Node, NumRegs, totalTokenNum);
    }

#if 1
    //Update the live out tokens according to the live out of the exit BB of the kernel.
    for (G4_BB* bb : fg)
    {
        if (bb->Succs.size() == 0 &&
            BBVector[bb->getId()]->Succs.size() == 0)
        {
            LiveGRFBuckets send_use_out(mem, kernel.getNumRegTotal());
            for (size_t i = 0; i < globalSendOpndList.size(); i++)
            {
                SBBucketNode* sBucketNode = globalSendOpndList[i];
                SBNode* sNode = sBucketNode->node;
                if (BBVector[bb->getId()]->send_live_out.isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                    sBucketNode->opndNum == Opnd_src1 ||
                    sBucketNode->opndNum == Opnd_src2 ||
                    sBucketNode->opndNum == Opnd_src3))
                {
                    BBVector[bb->getId()]->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_out);
                }
                if (BBVector[bb->getId()]->send_live_out.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
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
                    Acc.Token = Acc.Inst->getSetToken();
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
    for (auto& Access : FirstAccess) {
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
    for (auto& Access : LastAccess) {
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

void SWSB::getDominators(ImmDominator* dom)
{
    //BBVector[bb->getId()]->tokenAssigned = true;
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (size_t i = 0; i < BBVector.size(); i++)
        {
            BitSet currDoms = BBVector[i]->dominators;
            if (dom->getIDoms()[i] != BBVector[i]->getBB())
            {
                currDoms |= BBVector[dom->getIDoms()[i]->getId()]->dominators;
            }

            if (currDoms != BBVector[i]->dominators)
            {
                changed = true;
                BBVector[i]->dominators = currDoms;
            }
        }
    }
}

//
//Entry to the software scoreboard generator
//
void SWSB::SWSBGenerator()
{
    DEBUG_VERBOSE("[SWSB]: Starting...");
    PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
    p.doPointsToAnalysis(kernel.fg);

    kernel.fg.reassignBlockIDs();
    kernel.fg.findBackEdges();
    kernel.fg.findNaturalLoops();

    //Note that getNumFlagRegisters() treat each 16 bits as a flag register
    LiveGRFBuckets LB(mem, kernel.getNumRegTotal() + fg.builder->getNumScalarRegisters() + kernel.getNumAcc() + fg.builder->getNumFlagRegisters());
    LiveGRFBuckets globalSendsLB(mem, kernel.getNumRegTotal() + fg.builder->getNumScalarRegisters() + kernel.getNumAcc() + fg.builder->getNumFlagRegisters());

    SWSBDepDistanceGenerator(p, LB, globalSendsLB);

#ifdef DEBUG_VERBOSE_ON
    dumpDepInfo();
#endif

    if (fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
        fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation))
    {
        auto& dom = fg.getImmDominator();

        //Build dom tree
        for (size_t i = 0; i < BBVector.size(); i++)
        {
            G4_BB* bb = BBVector[i]->getBB();
            BBVector[i]->dominators = BitSet(BBVector.size(), false);
            BBVector[i]->dominators.set(i, true);

            if (dom.getIDoms()[bb->getId()] != bb)
            {
                BBVector[dom.getIDoms()[bb->getId()]->getId()]->domSuccs.push_back(BBVector[i]);
                BBVector[i]->domPreds.push_back(BBVector[dom.getIDoms()[bb->getId()]->getId()]);
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

    if (SBSendNodes.size())
    {
        SWSBGlobalTokenGenerator(p, LB, globalSendsLB);
    }
    else
    {
        handleFuncCall();
        insertTest();
    }

    if (fg.builder->getFCPatchInfo()->getFCComposableKernel())
    {
        genSWSBPatchInfo();
    }

#ifdef DEBUG_VERBOSE_ON
    std::cerr << "\n" << "Dependence Graph:" << "\n";

    for (const SBNode* node : SBNodes)
    {
        G4_INST* inst = node->GetInstruction();
        std::cerr << node->getNodeID() << ":\t";
        inst->dump();
        std::cerr << "Succs:";
        for (const SBDEP_ITEM& curSucc : node->succs)
        {
            std::cerr << curSucc.node->getNodeID() << ",";
        }
        std::cerr << "\n";
        std::cerr << "Preds:";
        for (const SBDEP_ITEM& curPred : node->preds)
        {
            std::cerr << curPred.node->getNodeID() << ",";
        }
        std::cerr << "\n\n";
    }
#endif

    return;
}

unsigned SWSB::calcDepDelayForNode(const SBNode* curNode) const
{
    const G4_INST* inst = curNode->GetInstruction();
    int reuseDelay = 0;

    if (inst->isSend())
    {
        if (inst->getDst() == nullptr ||
            inst->getDst()->isNullReg())
        {
            return TOKEN_AFTER_READ_CYCLE;
        }

        const G4_SendDesc* msgDesc = inst->getMsgDesc();
        if (msgDesc->isSLM())
        {
            reuseDelay = tokenAfterWriteSendSlmCycle;
        }
        else if (msgDesc->isSampler())
        {
            reuseDelay = tokenAfterWriteSendSamplerCycle;
        }
        else
        {
            reuseDelay = tokenAfterWriteSendMemoryCycle;
        }
    }
    else if (inst->isMathPipeInst())
    {
        if (fg.builder->hasFixedCycleMathPipe())
        {
            assert(0 && "Math instruction is assigned token which is not supported in fixed mach cycle platform");
        }

        reuseDelay = tokenAfterWriteMathCycle;
    }
    else if (inst->isDpas())
    {
        reuseDelay = tokenAfterDPASCycle;
    }
    else
    {
        assert(0 && "unexpected token reuse instruction");
    }

    return reuseDelay;
}

std::pair<int, int> SWSB::examineNodeForTokenReuse(
    unsigned nodeID,
    unsigned nodeDelay,
    const SBNode *reuseNode,
    unsigned char nestLoopLevel,
    unsigned curLoopStartBB,
    unsigned curLoopEndBB) const
{
    int reuseDelay = 0;
    int curDistance = 0;
    //The reuse node is before current node.
    if (nodeID > reuseNode->getNodeID())
    {
        unsigned curNodeDelay = reuseNode->getDepDelay();

        //reuse Delay is not accurate in different loop level
        reuseDelay = curNodeDelay - (nodeID - reuseNode->getNodeID());

        //If too far, count distance
        if (reuseDelay < 0)
        {
            curDistance = nodeID - reuseNode->getNodeID();
        }
    }
    else //The reuse node is after current node
    {
        reuseDelay = nodeDelay - (reuseNode->getNodeID() - nodeID);
        if (reuseDelay < 0)
        {
            curDistance = reuseNode->getNodeID() - nodeID;
        }
    }

    const G4_BB_SB *bb = BBVector[reuseNode->getBBID()];
    unsigned char curNodeNestLoopLevel = bb->getBB()->getNestLevel();
    unsigned loopLevelDiff = std::abs(curNodeNestLoopLevel - nestLoopLevel);
    constexpr unsigned loopFactorForTokenReuse = 5;
    if (reuseDelay > 0)
    {
        reuseDelay /= loopFactorForTokenReuse * loopLevelDiff + 1;
    }
    else
    {
        curDistance *= loopFactorForTokenReuse * loopLevelDiff + 1;
        if (nestLoopLevel && loopLevelDiff == 0)
        {
            if (curLoopStartBB == -1 || curLoopEndBB == -1)
            {
                curLoopStartBB = bb->getLoopStartBBID();
                curLoopEndBB = bb->getLoopEndBBID();
            }
            //Count the backedge, if the backedge distance is short, take it
            if (curLoopStartBB != -1 && curLoopEndBB != -1)
            {
                unsigned loopStartID = BBVector[curLoopStartBB]->first_node;
                unsigned loopEndID = BBVector[curLoopEndBB]->last_node;

                // The reused node may in same loop as current node.
                int backEdgeDistance = loopEndID - loopStartID - curDistance;

                if (reuseNode->getNodeID() < loopStartID || reuseNode->getNodeID() > loopEndID)
                {
                    // Or it may in another loop with same nest loop level
                    // Current back edge cannot cover the distance
                    // loop1 {
                    //    node1
                    // }
                    //
                    // loop2 {
                    //    node2
                    // }
                    curDistance = curDistance * (nestLoopLevel * loopFactorForTokenReuse + 1);
                }
                else
                {
                    curDistance = std::min(curDistance, backEdgeDistance);
                }
            }
        }
    }
    return std::make_pair(reuseDelay, curDistance);
}

//The algorithm for reuse selection: The live range which causes the least stall delay of current live range.
//FIXME: for global variable, it's not accurate. Because the AFTER_SOURCE and AFTER_WRITE may in different branches.
//Try not reuse the tokens set in adjacent instructions.
SBNode * SWSB::reuseTokenSelection(const SBNode * node) const
{
    int delay = tokenAfterWriteSendSamplerCycle; //Assume the longest one
    int distance = 0; //Distance between the node
    const unsigned nodeID = node->getNodeID();
    const unsigned nodeDelay = node->getDepDelay(); // The longest delay the node may cause.
    const unsigned char nestLoopLevel = BBVector[node->getBBID()]->getBB()->getNestLevel();
    const unsigned loopStartBB = BBVector[node->getBBID()]->getLoopStartBBID();
    const unsigned loopEndBB = BBVector[node->getBBID()]->getLoopEndBBID();

    assert(linearScanLiveNodes.size() <= totalTokenNum);

    //The live nodes whose dependencies are not resolved in current node.
    SBNode* candidateNode = linearScanLiveNodes.front();
    for (SBNode* curNode : linearScanLiveNodes)
    {
        int maxTokenDelay = std::numeric_limits<int>::min(); //The delay may cause if reuse
        int minTokenDistance = std::numeric_limits<int>::max(); //The distance from the reused node
        // The token may be reused already, so check the 2 nodes that are
        // closest to the node using the same token. In most cases the
        // token allocation is done in ascending order. So, searching backward
        // should be fast. As for searching forward, only do that if there's
        // indeed a such node.
        const unsigned short token = curNode->getLastInstruction()->getSetToken();
        const unsigned lastBefore = allTokenNodesMap[token].bitset.findLastIn(0, node->getSendID());
        unsigned firstAfter = -1;
        if (node->getSendID() < allTokenNodesMap[token].maxSendID)
        {
            firstAfter = allTokenNodesMap[token].bitset.findFirstIn(node->getSendID() + 1,
                                                                    allTokenNodesMap[token].maxSendID + 1);
        }
        if (lastBefore != -1)
        {
            assert(allTokenNodesMap[token].bitset.isSet(lastBefore));
            const SBNode* n = SBSendNodes[lastBefore];
            auto res = examineNodeForTokenReuse(nodeID, nodeDelay, n, nestLoopLevel, loopStartBB, loopEndBB);
            //Largest reuse delay
            maxTokenDelay = std::max(maxTokenDelay, res.first);
            //Closest distance
            minTokenDistance = std::min(minTokenDistance, res.second);
        }
        if (firstAfter != -1)
        {
            assert(allTokenNodesMap[token].bitset.isSet(firstAfter));
            const SBNode* n = SBSendNodes[firstAfter];
            auto res = examineNodeForTokenReuse(nodeID, nodeDelay, n, nestLoopLevel, loopStartBB, loopEndBB);
            //Largest reuse delay
            maxTokenDelay = std::max(maxTokenDelay, res.first);
            //Closest distance
            minTokenDistance = std::min(minTokenDistance, res.second);
        }

        // Smallest one is the best one
        // if Distance is not 0, count the distance, otherwise, use the delay.
        // Distance is not 0 means there are candidate whose distance is larger than the delay
        if (!distance && maxTokenDelay > 0)
        {
            if (maxTokenDelay < delay)
            {
                delay = maxTokenDelay;
                candidateNode = curNode;
            }
        }
        else if (minTokenDistance > distance)
        {
            distance = minTokenDistance;
            candidateNode = curNode;
        }
    }

    return candidateNode;
}

/*
 * If the cycles of the instruction which occupied
*/
bool SWSB::cycleExpired(const SBNode* node, int currentID) const
{
    if (node->GetInstruction()->isSend())
    {
        const G4_SendDesc* msgDesc = node->GetInstruction()->getMsgDesc();

        if (msgDesc->isSLM())
        {
            return tokenAfterWriteSendSlmCycle <= (currentID - node->getLiveStartID());
        }
        else if (msgDesc->isSampler())
        {
            return tokenAfterWriteSendSamplerCycle <= (currentID - node->getLiveStartID());
        }
        else
        {
            return tokenAfterWriteSendMemoryCycle <= (currentID - node->getLiveStartID());
        }
    }
    else if (node->GetInstruction()->isMathPipeInst())
    {
        if (fg.builder->hasFixedCycleMathPipe())
        {
            assert(0 && "Math instruction is assigned token which is not supported in fixed mach cycle platform");
        }
        return tokenAfterWriteMathCycle <= (currentID - node->getLiveStartID());
    }
    else if (node->GetInstruction()->isDpas())
    {
        return tokenAfterDPASCycle <= (int)(currentID - node->getLiveStartID());
    }
    else
    {
        assert(0 && "unexpected token reuse instruction");
    }

    return true;
}

//
// Token dependence reduction is trying to reduce the unnecessary dependence when token reuse happens
// Such as in following case
//  1.  send r20,...           { $0 }
//      ...
//  20. send r30, ...         { $0 }
//  21. add  r40 r20  r60    { $0.dst }
// There is no need to set dependence for instruction 21,
// because the reuse guarantee the dependency from instruction 1 is resolved before token 0 can be reused.
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
            //if ((node1BBID == node2BBID && bbID1 != node2BBID) ||
            //    (node1BBID != node2BBID && bbID1 == node2BBID && succ1->getNodeID() > node2->getNodeID()))
            //{
            //    node_it = node1->succs.erase(node_it);//FIXME, if the succ is the token instruction, do we need free the tokens assigned to the instruction because of the dependence
            //    continue;
            //}

            //When two successors are in same BB, previous one kill the following one
            // FIXME: This may not be good, because the policy is trying to keep the longest dependence and move the short one
            // Of course, if the two predecessors are lived in from different branch, we can only kill the longer one
            bool killed = false;
            for (auto node2_it = node2->succs.begin();
                node2_it != node2->succs.end();
                )
            {
                SBDEP_ITEM& curSucc2 = (*node2_it);
                const SBNode* succ2 = curSucc2.node;
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
            const SBNode* succ2 = node_it->node;
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
    printf("remove token 1: %d\n", n1->getLastInstruction()->getSetToken());
#endif
    return;
}

/*
*
*  We need cycle based expiration because for the case like
*  send   null, r2...      {$0}
*  add  r2                 {$0.src}
*  send   r20   r9...      {$0}
*  The second send should not be assigned with $0.
*  In compiler, if the live range of the r2 is end in the second instruction, token $0 is treated as free.
*  However, the SBID $0 will cleared only when the instruction finished the execution.
*  Assigned the same token to the third instruction will cause a long latency.
*  We delay the end of the lives of the intervals until the cycles are all consumed, so that the token will not be assigned immediately.
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
            const SBNode* node = linearScanLiveNodes.front();
            if (node->hasAWDep() || cycleExpired(node, startID))
            {
                unsigned short token = node->getLastInstruction()->getSetToken();

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
//NOTE THAT: Token reduction happens only when run out of token.
void SWSB::shareToken(const SBNode* node, const SBNode* succ, unsigned short token)
{
    if (node->getBBID() == succ->getBBID())
    {
        return;
    }

    for (const SBDEP_ITEM& curPred : succ->preds)
    {
        const SBNode* succPred = curPred.node;

        if (node->getBBID() != succPred->getBBID() &&
            succPred->getLastInstruction()->getTokenType() == G4_INST::SWSBTokenType::TOKEN_NONE &&
            tokenHonourInstruction(succPred->getLastInstruction()))
        {
            G4_BB_SB* curBB = BBVector[node->getBBID()];
            G4_BB_SB* succPredBB = BBVector[succPred->getBBID()];
            //FIXME: Only define BBs comparison is not enough. It may cause extra delay?
            if (!(curBB->send_live_in.isDstSet((unsigned)succPred->globalID) ||
                curBB->send_live_in.isSrcSet((unsigned)succPred->globalID) ||
                succPredBB->send_live_in.isDstSet((unsigned)node->globalID) ||
                succPredBB->send_live_in.isSrcSet((unsigned)node->globalID)
                ))
            {
                succPred->getLastInstruction()->setSetToken(token);
            }
        }
    }

    return;
}

void SWSB::assignDepToken(SBNode* node)
{
    unsigned short token = node->getLastInstruction()->getSetToken();
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

    for (const SBDEP_ITEM& curSucc : node->succs)
    {
        SBNode* succ = curSucc.node;
        DepType type = curSucc.type;
        SBDependenceAttr attr = curSucc.attr;

        if (attr == DEP_IMPLICIT)
        {
            continue;
        }

        //Same token,reuse happened, no need to set dep token
        if (tokenHonourInstruction(succ->getLastInstruction()) &&
            succ->getLastInstruction()->getSetToken() == token && (succ->instVec.size() <= 1)) //If the node size, the token reuse cannot guard the last instruction.
        {
            continue;
        }

        //set dependence token if live
        SWSBTokenType tokenType = type == WAR ? SWSBTokenType::AFTER_READ : SWSBTokenType::AFTER_WRITE;
        succ->setDepToken(token, tokenType, node);
#ifdef DEBUG_VERBOSE_ON
        dumpSync(node, succ, token, tokenType);
#endif
    }
}

void SWSB::assignDepTokens()
{
    for (SBNode* node : SBSendNodes)
    {
        G4_INST* inst = node->getLastInstruction();

        if (inst->isEOT())
        {
            continue;
        }

        unsigned short token = inst->getSetToken();
        if (token != (unsigned short)-1)
        {
            assignDepToken(node);
        }
    }
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
            token = oldNode->getLastInstruction()->getSetToken();
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
#ifdef DEBUG_VERBOSE_ON
    printf("Assigned token: %d,  node: %d, send: %d,  QUEUE SIZE: %d\n", token, node->getNodeID(), node->getSendID(), linearScanLiveNodes.size());
#endif

    //Set token to send
    node->getLastInstruction()->setSetToken(token);
    //For token reduction
    allTokenNodesMap[token].set(node->sendID);

    //Sort succs according to the BBID and node ID.
    std::sort(node->succs.begin(), node->succs.end(), nodeSortCompare);
    for (auto node_it = node->succs.begin();
        node_it != node->succs.end();
        )
    {
        const SBDEP_ITEM& curSucc = (*node_it);
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
                    succ->getLastInstruction()->getSetToken() == (unsigned short)UNKNOWN_TOKEN)
                {
                    if (fg.builder->getOptions()->getOption(vISA_EnableDPASTokenReduction))
                    {
                        //  If no instruction depends on DPAS, no SBID
                        if (!(succ->GetInstruction()->isDpas() && succ->succs.size() == 0))
                        {
                            succ->getLastInstruction()->setSetToken(token);
                            node->setLiveLatestID(succ->getLiveEndID(), succ->getLiveEndBBID());
                            allTokenNodesMap[token].set(succ->sendID);
                            succ->setTokenReuseNode(node);
                            continue;
                        }
                    }
                    else
                    {
                        succ->getLastInstruction()->setSetToken(token);
                        node->setLiveLatestID(succ->getLiveEndID(), succ->getLiveEndBBID());
                        allTokenNodesMap[token].set(succ->sendID);
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
        const SBNode* curNode = (*node_it);

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
    for (const SBNode *node : freeTokenList)
    {
        if (node != nullptr)
        {
            usedToken++;
        }
    }
    assert(usedToken == linearScanLiveNodes.size());

#ifdef DEBUG_VERBOSE_ON
    printf("Add token: %d\n", node->getLastInstruction()->getSetToken());
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

    assert(BBVector[bbID]->liveInTokenNodes.getSize() != 0);

    BitSet temp_live_in(unsigned(SBSendNodes.size()), false);
    temp_live_in = BBVector[bbID]->liveInTokenNodes;

    //Union all of out of SIMDCF predecessor BB to the live in of current BB.
    for (const G4_BB_SB* predBB : BBVector[bbID]->Preds)
    {
        unsigned predID = predBB->getBB()->getId();
        temp_live_in |= BBVector[predID]->liveOutTokenNodes;
    }

    //Union all of out of scalar predecessor BB to the live in of current BB.
    for (const G4_BB* predBB : bb->Preds)
    {
        unsigned predID = predBB->getId();
        temp_live_in |= BBVector[predID]->liveOutTokenNodes;
    }

    //Changed? Yes, get the new live in, other wise do nothing
    if (temp_live_in != BBVector[bbID]->liveInTokenNodes)
    {
        changed = true;
        BBVector[bbID]->liveInTokenNodes = temp_live_in;
    }

    //Calculate the live out according to the live in and killed tokens in current BB
    for (uint32_t token = 0; token < totalTokenNum; token++)
    {
        if (BBVector[bbID]->killedTokens.isSet(token))
        {
            temp_live_in -= allTokenNodesMap[token].bitset;
        }
    }

    //Get the new live out,
    //FIXME: is it right? the live out is always assigned in increasing.
    //Original, we only have local live out.
    //should we separate the local live out vs total live out?
    //Not necessary, can live out, will always be live out.
    BBVector[bbID]->liveOutTokenNodes |= temp_live_in;

    return changed;
}

void SWSB::SWSBGlobalTokenAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        for (G4_BB* bb : fg)
        {
            if (globalTokenReachAnalysis(bb))
            {
                change = true;
            }
        }
    }
}

void SWSB::SWSBGlobalScalarCFGReachAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        for (G4_BB* bb : fg)
        {
            if (globalDependenceDefReachAnalysis(bb))
            {
                change = true;
            }
        }
    }
}

void SWSB::SWSBGlobalSIMDCFGReachAnalysis()
{
    bool change = true;
    while (change)
    {
        change = false;
        for (G4_BB* bb : fg)
        {
            if (globalDependenceUseReachAnalysis(bb))
            {
                change = true;
            }
        }
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
}

bool SWSB::propogateDist(G4_BB* bb)
{
    bool changed = false;
    unsigned bbID = bb->getId();

    if (bb->Preds.empty())
    {
        return false;
    }

    assert(BBVector[bbID]->send_live_in.getSize() != 0);

    SBBitSets temp_live_in(globalSendNum);
    temp_live_in = BBVector[bbID]->send_live_in;
    std::vector<unsigned> tokenLiveInDist;
    tokenLiveInDist.resize(globalSendNum);

    for (unsigned i = 0; i < globalSendNum; i++)
    {
        tokenLiveInDist[i] = BBVector[bbID]->tokenLiveInDist[i];
    }

    //Get the live out from all predicator BBs
    for (const G4_BB* predBB : bb->Preds)
    {
        unsigned predID = predBB->getId();

        for (unsigned i = 0; i < globalSendNum; i++)
        {
            if (BBVector[predID]->send_live_out.isDstSet(i) &&
                BBVector[predID]->tokenLiveOutDist[i] != -1 &&
                BBVector[predID]->tokenLiveOutDist[i] < tokenLiveInDist[i])
            {
                tokenLiveInDist[i] = BBVector[predID]->tokenLiveOutDist[i];
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
            if (BBVector[bbID]->send_live_in.isDstSet(i) &&
                BBVector[bbID]->send_live_out.isDstSet(i) &&
                !BBVector[bbID]->send_may_kill.isDstSet(i))
            {
                BBVector[bbID]->tokenLiveOutDist[i] = BBVector[bbID]->tokenLiveInDist[i] + bb->size();
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
    for (SBNode* node : SBSendNodes)
    {
        if (BBVector[node->getBBID()]->send_live_out.isDstSet(node->globalID))
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
        for (G4_BB* bb : fg)
        {
            if (propogateDist(bb))
            {
                change = true;
            }
        }
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


/* Quick token allocation, allocate the token in round robin.
 */
void SWSB::quickTokenAllocation()
{
    uint32_t token = 0;

    //Linear scan
    for (SBNode* node : SBSendNodes)
    {
        if (node->getLastInstruction()->isEOT())
        {
            continue;
        }

        assert(node->getLastInstruction()->getSetToken() == (unsigned short)UNKNOWN_TOKEN);
        node->getLastInstruction()->setSetToken(token);
        if (token >= totalTokenNum - 1)
        {
            token = 0;
        }
        else
        {
            token ++;
        }
    }

    assignDepTokens();
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
    freeTokenList.resize(totalTokenNum);
    topIndex = 0;

    tokenProfile.setTokenInstructionCount((int)SBSendNodes.size());
    uint32_t AWTokenReuseCount = 0;
    uint32_t ARTokenReuseCount = 0;
    uint32_t AATokenReuseCount = 0;
    uint32_t mathInstCount = 0;
    //Linear scan
    //Assign tokens to nodes in the order of liveness. Here we only need to
    //iterate SB nodes in that order, and don't actually need to sort
    //SBSendNodes as it might be referenced through allTokenNodesMap.
    auto sortInLivenessOrder = [](const SBNODE_VECT& vec) {
        SBNODE_VECT sorted(vec.size());
        std::partial_sort_copy(vec.begin(), vec.end(), sorted.begin(), sorted.end(), compareInterval);
        return sorted;
    };
    const bool enableSendTokenReduction = fg.builder->getOptions()->getOption(vISA_EnableSendTokenReduction);
    const bool enableDPASTokenReduction = fg.builder->getOptions()->getOption(vISA_EnableDPASTokenReduction);
    for (SBNode* node : sortInLivenessOrder(SBSendNodes))
    {
        unsigned startID = node->getLiveStartID();
        G4_INST* inst = node->getLastInstruction();
#ifdef DEBUG_VERBOSE_ON
        printf("\n=======nodeID: %d, startID: %d, endID: %d\n", node->getNodeID(), node->getLiveStartID(), node->getLiveEndID());
#endif
        if (inst->isEOT())
        {
            continue;
        }

        if (enableSendTokenReduction && node->succs.size() == 0)
        {
            continue;
        }

        if (enableDPASTokenReduction)
        {
            //If there is no instruction depends on a DPAS instruction, no SBID
            if (inst->isDpas() && node->succs.size() == 0)
            {
                continue;
            }
        }

        if (inst->isMathPipeInst())
        {
            mathInstCount++;
        }

        expireIntervals(startID);

        unsigned short assignedToken = node->getLastInstruction()->getSetToken();
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
        for (G4_BB_SB* sb_bb : BBVector)
        {
            sb_bb->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
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
        tokenProfile.setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile.setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile.setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile.setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    assignDepTokens();

    tokenProfile.setAWTokenReuseCount(AWTokenReuseCount);
    tokenProfile.setARTokenReuseCount(ARTokenReuseCount);
    tokenProfile.setAATokenReuseCount(AATokenReuseCount);
    tokenProfile.setMathInstCount(mathInstCount);
}

unsigned short SWSB::reuseTokenSelectionGlobal(SBNode* node, G4_BB* bb, SBNode*& candidateNode, bool& fromSibling)
{
    SBBitSets temp_live_in(globalSendNum);
    temp_live_in = BBVector[bb->getId()]->send_live_in;
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

        for (SBNode* liveNode : *reachTokenArray[i])
        {
            unsigned liveNodeDelay = liveNode->getDepDelay();
            unsigned liveNodeOverhead = 0;

            //What about the global send come back to current BB?
            //Shouldn't be assigned
            if ((liveNode->globalID != -1) &&
                (BBVector[bb->getId()]->tokenLiveInDist[liveNode->globalID] != -1) &&
                (liveNode->getBBID() != bb->getId() || liveNode->getNodeID() > node->getNodeID()) )
            {
                nodeDist = BBVector[bb->getId()]->tokenLiveInDist[liveNode->globalID] + (node->getNodeID() - BBVector[bb->getId()]->first_node);
            }
            else
            {
                if (liveNode->getBBID() == bb->getId())
                {
                    nodeDist = node->getNodeID() - liveNode->getNodeID();
                }
                else //Not dst live out global, which is not calculated, use the node distance
                {
                    nodeDist = node->getNodeID() > liveNode->getNodeID() ? node->getNodeID() - liveNode->getNodeID() : liveNode->getNodeID() - node->getNodeID();
                }
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
            for (SBNode* useNode : *reachUseArray[i])
            {
                unsigned nodeDelay = node->getDepDelay();
                unsigned nodeOverhead = 0;

                //What about the global send come back to current BB?
                //Shouldn't be assigned
                if ((node->globalID != -1) &&
                    (BBVector[useNode->getBBID()]->tokenLiveInDist[node->globalID] != -1) &&
                    (useNode->getBBID() != bb->getId() || useNode->getNodeID() > node->getNodeID()))
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

        if (candidateTokenNode && (tokenReuseOverhead < nodeReuseOverhead))
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
            BBVector[BBID]->localReachingSends.setDst(node->sendID, false);
            continue;
        }
        it++;
    }
}

void SWSB::assignTokenToPred(SBNode* node, SBNode* pred, G4_BB* bb)
{
    unsigned predDist = -1;
    SBNode* canidateNode = nullptr;

    assert(pred->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN);

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
            (otherPred->getLastInstruction()->getSetToken() == (unsigned short)UNKNOWN_TOKEN) &&
            (type == RAW || type == WAW || otherPred->getLastInstruction()->getDst() == nullptr))
        {
            if ((!otherPred->reachingSends.isDstSet(pred->sendID)) &&
                (!pred->reachingSends.isDstSet(otherPred->sendID)))
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
        canidateNode->getLastInstruction()->setSetToken(pred->getLastInstruction()->getSetToken());
#ifdef DEBUG_VERBOSE_ON
        printf("Node: %d, PRED assign: %d, token: %d\n", node->getNodeID(), canidateNode->getNodeID(), canidateNode->getLastInstruction()->getSetToken());
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
            (pred->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN) &&
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
        node->getLastInstruction()->setSetToken(canidateNode->getLastInstruction()->getSetToken());
        allTokenNodesMap[canidateNode->getLastInstruction()->getSetToken()].set(node->sendID);
#ifdef DEBUG_VERBOSE_ON
        printf("Node: %d, pred reuse assign: %d, token: %d\n", node->getNodeID(), canidateNode->getNodeID(), node->getLastInstruction()->getSetToken());
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

    BBVector[bb->getId()]->localReachingSends = SBBitSets(SBSendNodes.size());

    assert((BBVector[bb->getId()]->last_send_node != -1) &&
        (BBVector[bb->getId()]->first_send_node <= BBVector[bb->getId()]->last_send_node));

    SBBitSets send_live(SBSendNodes.size());
    SBBitSets send_use(SBSendUses.size());

    for (int i = BBVector[bb->getId()]->first_send_node; i <= BBVector[bb->getId()]->last_send_node; i++)
    {
        SBNode* node = SBSendNodes[i];

        if (node->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
        {
            continue;
        }

        if (node->getLastInstruction()->isDpas() && node->succs.size() == 0 &&
            fg.builder->getOptions()->getOption(vISA_EnableDPASTokenReduction))
        {
            continue;
        }

        send_live = node->reachingSends; //The tokens will reach current node

        for (unsigned k = 0; k < totalTokenNum; k++)
        {
            reachTokenArray[k]->clear();
            reachUseArray[k]->clear();
        }

        for (size_t k = 0; k < SBSendNodes.size(); k++)
        {
            SBNode* liveNode = SBSendNodes[k];
            if ((liveNode->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN) &&
                (send_live.isDstSet(k) ||
                (send_live.isSrcSet(k) &&
                 isPrefetch(liveNode->getLastInstruction()))))
            {
                reachTokenArray[liveNode->getLastInstruction()->getSetToken()]->push_back(liveNode);
            }
        }

        if (!fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && (node->reachedUses.getSize() != 0))
        {
            send_use = node->reachedUses;    //The uses of other sends can be reached by current node.
            for (size_t k = 0; k < SBSendUses.size(); k++)
            {
                SBNode* liveNode = SBSendUses[k];
                if (send_use.isDstSet(k))
                {
                    for (size_t m = 0; m < liveNode->preds.size(); m++)
                    {
                        SBDEP_ITEM& curPred = liveNode->preds[m];
                        SBNode* pred = curPred.node;
                        if (pred->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
                        {
                            reachUseArray[pred->getLastInstruction()->getSetToken()]->push_back(liveNode);
                        }
                    }
                }
            }
        }

        if (!assignTokenWithPred(node, bb))
        {
            bool assigned = false;

            //Assigned with coalescing
            if (!fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && (node->reachedUses.getSize() != 0))
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
                        unsigned short exToken = exclusiveNode->getLastInstruction()->getSetToken();
                        if (exToken != (unsigned short)UNKNOWN_TOKEN)
                        {
                            if (reachTokenArray[exToken]->size() == 0 &&
                                reachUseArray[exToken]->size() == 0)
                            {
                                node->getLastInstruction()->setSetToken(exToken);
                                allTokenNodesMap[exToken].set(node->sendID);
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
                //Assigned with first free token
                for (unsigned k = 0; k < totalTokenNum; k++)
                {
                    if ((reachTokenArray[k]->size() == 0) &&
                        (reachUseArray[k]->size() == 0))
                    {
                        node->getLastInstruction()->setSetToken(k);
                        allTokenNodesMap[k].set(node->sendID);
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
                bool reuseSibling = !fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation) && (node->reachedUses.getSize() != 0);
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

                node->getLastInstruction()->setSetToken(reuseToken);
                allTokenNodesMap[reuseToken].set(node->sendID);
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
    for (const G4_BB_SB *succ : BBVector[bb->getId()]->domSuccs)
    {
        if (!succ->tokenAssigned)
        {
            tokenAllocationBB(succ->getBB());
        }
    }
}

void SWSB::tokenAllocationWithDistPropogationPerBB(G4_BB * bb)
{
    propogateDist(bb);
    allocateToken(bb);
    BBVector[bb->getId()]->tokenAssigned = true;

    for (const G4_BB_SB *succ : BBVector[bb->getId()]->domSuccs)
    {
        if (!succ->tokenAssigned)
        {
            tokenAllocationWithDistPropogationPerBB(succ->getBB());
        }
    }
}

void SWSB::tokenAllocationWithDistPropogation()
{
#ifdef DEBUG_VERBOSE_ON
    globalSBNodes.resize(globalSendNum);
#endif
    //Initial all live out distance
    for (const SBNode* node : SBSendNodes)
    {
        if (BBVector[node->getBBID()]->send_live_out.isDstSet(node->globalID))
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
        const G4_BB_SB *bb = BBVector[i];
        std::cerr << "BB" << i << ": " << bb->first_node << "-" << bb->last_node << ", succ<";
        for (const G4_BB* succ : bb->getBB()->Succs)
        {
            std::cerr << succ->getId() << ",";
        }
        std::cerr << "> pred<";
        for (const G4_BB* pred : bb->getBB()->Preds)
        {
            std::cerr << pred->getId() << ",";
        }

        std::cerr << ">\n liveIn:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (bb->tokenLiveInDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << bb->tokenLiveInDist[k];
            }
        }
        std::cerr << "\n liveout:";
        for (unsigned k = 0; k < globalSendNum; k++)
        {
            if (bb->tokenLiveOutDist[k] != -1)
            {
                std::cerr << "  n" << globalSBNodes[k]->getNodeID() << ":" << bb->tokenLiveOutDist[k];
            }
        }
        std::cerr << "\n\n";
    }
#endif

}

void SWSB::buildExclusiveForCoalescing()
{
    for (SBNode* node : SBSendNodes)
    {
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

        SBBitSets send_live(SBSendNodes.size());

        for (SBDEP_ITEM& curSucc : node->succs)
        {
            SBNode* succ = curSucc.node;
            DepType type = curSucc.type;
            if (((type == RAW) || (type == WAW)) && (succ->reachingSends.getSize() != 0))
            {
                send_live = succ->reachingSends;
                //FIXME, the complexity may be a little big high, n*n*succSize
                for (size_t k = 0; k < SBSendNodes.size(); k++)
                {
                    SBNode* liveNode = SBSendNodes[k];
                    if (send_live.isDstSet(k) &&
                        (liveNode != node) &&
                        (!(liveNode->reachingSends.isDstSet(node->sendID) ||
                            node->reachingSends.isDstSet(liveNode->sendID)) ||
                            tokenHonourInstruction(succ->GetInstruction())))
                        //If the use is token honour instruction and be assigned with same token as pred,
                        //it will cause dependence any way, cannot be removed.
                        //FIXME: But one send can depends on multiple previous send.
                        //Only the one set to the send will cause non-removable dependence.
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

            for (const SBDEP_ITEM& curPred : succ->preds)
            {
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
        for (G4_BB_SB *bb : BBVector)
        {
            bb->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
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
        tokenProfile.setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile.setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile.setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile.setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    assignDepTokens();
}

void SWSB::tokenAllocationGlobal()
{
    G4_BB* bb = *fg.begin();

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
        for (G4_BB_SB *bb : BBVector)
        {
            bb->getLiveOutToken(unsigned(SBSendNodes.size()), &SBNodes);
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
        tokenProfile.setPrunedEdgeNum(prunedEdgeNum);
        tokenProfile.setPrunedGlobalEdgeNum(prunedGlobalEdgeNum);
        tokenProfile.setPrunedDiffBBEdgeNum(prunedDiffBBEdgeNum);
        tokenProfile.setPrunedDiffBBSameTokenEdgeNum(prunedDiffBBSameTokenEdgeNum);
    }

    assignDepTokens();
}

G4_INST* SWSB::insertSyncInstruction(G4_BB* bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
    G4_INST* syncInst = fg.builder->createSync(G4_sync_nop, src0);
    bb->insertBefore(nextIter, syncInst);
    syncInstCount++;

    return syncInst;
}

G4_INST* SWSB::insertSyncInstructionAfter(G4_BB* bb, INST_LIST_ITER iter, int CISAOff, int lineNo)
{
    INST_LIST_ITER nextIter = iter;
    nextIter++;
    G4_SrcRegRegion* src0 = fg.builder->createNullSrc(Type_UD);
    G4_INST* syncInst = fg.builder->createSync(G4_sync_nop, src0);
    bb->insertBefore(nextIter, syncInst);
    syncInstCount++;

    return syncInst;
}

G4_INST* SWSB::insertTestInstruction(G4_BB* bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo, bool countSync)
{
    G4_INST* nopInst = fg.builder->createNop(InstOpt_NoOpt);
    bb->insertBefore(nextIter, nopInst);
    if (countSync)
    {
        syncInstCount++;
    }

    return nopInst;
}

G4_INST* SWSB::insertSyncAllRDInstruction(G4_BB* bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_INST* syncInst;
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
    bb->insertBefore(nextIter, syncInst);

    return syncInst;
}

G4_INST* SWSB::insertSyncAllWRInstruction(G4_BB* bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo)
{
    G4_INST* syncInst;
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
    bb->insertBefore(nextIter, syncInst);

    return syncInst;
}

bool SWSB::insertSyncToken(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens, bool& keepDst, bool removeAllToken)
{
    //Non-test instruction can only have
    // 1. non-send: one Dst Token with distance, or
    // 2. send: distance only, or
    // 2. one Dst token, or
    // 3. one Src token
    unsigned short dst = 0;
    unsigned short src = 0;
    std::vector<std::pair<unsigned short, unsigned>> dst_loc;
    std::vector<std::pair<unsigned short, unsigned>> src_loc;

    bool multipleDst = false;
    bool multipleSrc = false;
    unsigned short token = (unsigned short)-1;
    unsigned short dstToken = (unsigned short)-1;
    unsigned short srcToken = (unsigned short)-1;
    SWSBTokenType type = G4_INST::SWSBTokenType::TOKEN_NONE;
    bool insertedSync = false;

    for (unsigned int i = 0; i < node->getDepTokenNum();)
    {
        G4_INST* synAllInst = nullptr;
        token = node->getDepToken(i, type);
        unsigned depNodeID = node->getDepTokenNodeID(i);
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
                node->eraseDepToken(i);
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
                    //Token is kept in original instruction
                    keepDst = true;
                    inst->setToken(token);
                    inst->setTokenType(type);
                    inst->setTokenLoc(token, depNodeID);
                    token = (unsigned short)UNKNOWN_TOKEN;
                    i++;
                    continue;
                }

                if (type == SWSBTokenType::AFTER_READ)
                {
                    src |= bitToken;
                    src_loc.push_back(std::make_pair(token, depNodeID));
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
                    dst_loc.push_back(std::make_pair(token, depNodeID));
                    if (!multipleDst && (dst & ~bitToken))
                    {
                        multipleDst = true;
                    }
                    dstToken = token;
                    dstTokens->set(token, true);
                }

                node->eraseDepToken(i);
                continue;
            }
        }
        break;
        case SWSBTokenType::READ_ALL:
        {
            assert(token == (unsigned short)UNKNOWN_TOKEN);
            node->eraseDepToken(i);
            synAllInst = insertSyncAllRDInstruction(bb, 0, inst_it, inst->getCISAOff(), inst->getLineNo());
            synAllInst->setLexicalId(newInstID);
            i++;
            continue;
        }
        break;
        case SWSBTokenType::WRITE_ALL:
        {
            assert(token == (unsigned short)UNKNOWN_TOKEN);
            node->eraseDepToken(i);
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

    G4_INST* synInst;
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
            synInst->setToken(dstToken);
            synInst->setTokenType(SWSBTokenType::AFTER_WRITE);
        }
        synInst->setLexicalId(newInstID);
        insertedSync = true;
        for (auto loc:dst_loc)
        {
            synInst->setTokenLoc(loc.first, loc.second);
        }
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
            synInst->setToken(srcToken);
            synInst->setTokenType(SWSBTokenType::AFTER_READ);
        }
        synInst->setLexicalId(newInstID);
        insertedSync = true;
        for (auto loc:src_loc)
        {
            synInst->setTokenLoc(loc.first, loc.second);
        }
    }

    return insertedSync;
}

/*
 *  For Xe, sync can be used for distance and token at the same time.
 *  The encoding limitations for instruction attached dependence info
 *  a. has to attached with instruction
 *      1. memSet
 *  b. Others
 *      1. Only regDst can be used when there is memSet for DPAS/math
 *      2. Only regDstAll can be used when there is memSet for send
 *      3. Only regDist can be used when there is mem.dst for ALU instructions
 *  c. To be consistent with the previous version (TGLLP)
 *      1. Tried to attach the distance with the original instructions.
 *      2. The only exception is the memSet for out-of-order instructions
 *
 *   SWSB format - non DPAS/send/math (in-order)
 *   7    6    5    4    3    2    1    0
 *   0    0    0    0    0    0    0    0
 *   0    0    0    0    0    regDist
 *   0    0    0    0    1    regDistAll
 *   0    0    0    1    0    regDistFloat
 *   0    0    0    1    1    regDistInt
 *   0    0    1    0    memSBid dst
 *   0    0    1    1    memSBid src
 *   0    1    0    0    R    R    R    R
 *   0    1    0    1    R    regDistLong
 *   0    1    1    R    R    R    R    R
 *   1    regDist           memSBid dst
 *
 *   SWSB format - DPAS/math (out-of-order)
 *   0    0    0    0    0    0    0    0
 *   0    0    0    0    0    regDist
 *   0    0    0    0    1    regDistAll
 *   0    0    0    1    0    regDistFloat
 *   0    0    0    1    1    regDistInt
 *   0    0    1    0    memSBid dst
 *   0    0    1    1    memSBid src
 *   0    1    0    0    memSBid set
 *   0    1    0    1    R    regDistLong
 *   0    1    1    R    R    R    R    R
 *   1    regDist           memSBid set
 *
 *   SWSB format -send (out-of-order)
 *   0    0    0    0    0    0    0    0
 *   0    0    0    0    0    regDist
 *   0    0    0    0    1    regDistAll
 *   0    0    0    1    0    regDistFloat
 *   0    0    0    1    1    regDistInt
 *   0    0    1    0    memSBid dst
 *   0    0    1    1    memSBid src
 *   0    1    0    0    memSBid set
 *   0    1    0    1    R    regDistLong
 *   0    1    1    R    R    R    R    R
 *   1    regDistAll           memSBid set
 */
bool SWSB::insertSyncXe(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens)
{
    G4_INST::DistanceType distType = node->GetInstruction()->getDistanceTypeXe();
    bool insertedSync = false;
    bool keepDst = false;
    bool isCloseALUType = node->GetInstruction()->isClosestALUType();

    if (tokenHonourInstruction(inst))
    {
        //regDist $.set
        if (inst->isDpas())
        {
            if (distType != G4_INST::DistanceType::DIST_NONE &&
                distType != G4_INST::DistanceType::DIST)
            {
                G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                synInst->setDistance(inst->getDistance());
                synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                inst->setDistance(0);
                inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                insertedSync = true;
            }
        }

        if (inst->isMathPipeInst())
        {
            if (isCloseALUType && distType != G4_INST::DistanceType::DIST_NONE)
            {
                node->GetInstruction()->setDistanceTypeXe(G4_INST::DistanceType::DIST);
                distType = G4_INST::DistanceType::DIST;
            }
            if (distType != G4_INST::DistanceType::DIST_NONE &&
                distType != G4_INST::DistanceType::DIST)
            {
                G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                synInst->setDistance(inst->getDistance());
                synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                inst->setDistance(0);
                inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                insertedSync = true;
            }
        }

        // regDistAll $.set
        if (inst->isSend())
        {
            if (isCloseALUType && distType != G4_INST::DistanceType::DIST_NONE && (*inst_it) == inst)
            {
                node->GetInstruction()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                distType = G4_INST::DistanceType::DISTALL;
            }
            if ((distType != G4_INST::DistanceType::DIST_NONE &&
                distType != G4_INST::DistanceType::DISTALL) || ((*inst_it) != inst))
            {
                G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                synInst->setDistance(inst->getDistance());
                synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                inst->setDistance(0);
                inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                insertedSync = true;
            }
        }
        //For out-of-order instruction, all dependence token will be moved out to sync
        insertedSync |= insertSyncToken(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, keepDst, true);
    }
    else
    {
        // regDist $.dst
        //For in-order instruction, trying to keep distance in the original instruction
        if (distType == G4_INST::DistanceType::DIST ||
            distType == G4_INST::DistanceType::DIST_NONE)
        {
            insertedSync = insertSyncToken(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, keepDst, false);
        }
        else
        {
            //Move all token dependence out
            insertedSync = insertSyncToken(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, keepDst, true);
        }
    }

    return insertedSync;
}

//For dpas/dpasw instructions
//      RegDist         SBID.set
//    RegDist         SBID.src
//    RegDist         SBID.dst
//For send instruction
//    RegDistAll     SBID.set
//    RegDistFloat   SBID.set
//    RegDistInt     SBID.set
//For non-send / non-dpas/dpasw instructions
//    RegDist        SBID.dst
//    RegDist        SBID.src
//    RegDistAll     SBID.dst
bool SWSB::insertSyncTokenPVC(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens, bool removeAllToken)
{
    //SBID.set > SBID.dst > SBID.src
    unsigned int dst = 0;
    unsigned int src = 0;
    bool keepDst = false;
    bool multipleDst = false;
    bool multipleSrc = false;
    unsigned short token = (unsigned short)-1;
    unsigned short dstToken = (unsigned short)-1;
    unsigned short srcToken = (unsigned short)-1;
    std::vector<std::pair<unsigned short, unsigned>> dst_loc;
    std::vector<std::pair<unsigned short, unsigned>> src_loc;
    SWSBTokenType type = G4_INST::SWSBTokenType::TOKEN_NONE;
    bool insertedSync = false;

    for (unsigned int i = 0; i < node->getDepTokenNum();)
    {
        token = node->getDepToken(i, type);
        unsigned depNodeID = node->getDepTokenNodeID(i);
        unsigned int bitToken = (unsigned int)(1 << token);
        assert(token != (unsigned short)UNKNOWN_TOKEN);

        switch (type)
        {
        case SWSBTokenType::AFTER_WRITE:
        {
            if (dstTokens->isSet(token))
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
                node->eraseDepToken(i);
                continue;
            }
            else
            {
                if (!removeAllToken &&                  //No set one marked.
                    !keepDst)                            //No dst one kept yet
                {
                    //Token is kept in original instruction
                    keepDst = true;
                    inst->setToken(token);
                    inst->setTokenType(SWSBTokenType::AFTER_WRITE);
                    inst->setTokenLoc(token, depNodeID);
                    token = (unsigned short)UNKNOWN_TOKEN;
                    i++;
                    continue;
                }

                dst |= bitToken;
                dst_loc.push_back(std::make_pair(token, depNodeID));
                if (!multipleDst && (dst & ~bitToken))
                {
                    multipleDst = true;
                }
                dstToken = token;
                dstTokens->set(token, true);

                node->eraseDepToken(i);
                continue;
            }
        }
        break;
        default:
            assert(type == SWSBTokenType::AFTER_READ && "Wrong dependence type");
            break;
        }
        i++;
    }

    bool keepSrc = false;
    for (unsigned int i = 0; i < node->getDepTokenNum();)
    {
        token = node->getDepToken(i, type);
        unsigned depNodeID = node->getDepTokenNodeID(i);
        unsigned int bitToken = (unsigned int)(1 << token);
        assert(token != (unsigned short)UNKNOWN_TOKEN);

        switch (type)
        {
        case SWSBTokenType::AFTER_READ:
        {
            if (dstTokens->isSet(token) || (type == SWSBTokenType::AFTER_READ && srcTokens->isSet(token)))
            {
                node->eraseDepToken(i);
                continue;
            }
            else
            {
                if (!removeAllToken &&
                    !keepDst &&
                    !keepSrc)
                {
                    //Token is kept in original instruction
                    keepSrc = true;
                    inst->setToken(token);
                    inst->setTokenType(SWSBTokenType::AFTER_READ);
                    inst->setTokenLoc(token, depNodeID);
                    token = (unsigned short)UNKNOWN_TOKEN;
                    i++;
                    continue;
                }
                src |= bitToken;
                src_loc.push_back(std::make_pair(token, depNodeID));
                if (!multipleSrc && (src & ~bitToken))
                {
                    multipleSrc = true;
                }
                srcToken = token;
                srcTokens->set(token, true);

                node->eraseDepToken(i);
                continue;
            }
        }
        break;
        default:
            assert(type == SWSBTokenType::AFTER_WRITE && "Wrong dependence type");
            break;
        }
        i++;
    }

    G4_INST* synInst;

    if (dst)
    {
        if (dst == 0xFFFFFFFF)
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
            synInst->setToken(dstToken);
            synInst->setTokenType(SWSBTokenType::AFTER_WRITE);
        }
        synInst->setLexicalId(newInstID);
        for (auto loc:dst_loc)
        {
            synInst->setTokenLoc(loc.first, loc.second);
        }
        insertedSync = true;
    }

    if (src)
    {
        if (src == 0xFFFFFFFF)
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
            synInst->setToken(srcToken);
            synInst->setTokenType(SWSBTokenType::AFTER_READ);
        }
        synInst->setLexicalId(newInstID);
        for (auto loc:src_loc)
        {
            synInst->setTokenLoc(loc.first, loc.second);
        }
        insertedSync = true;
    }

    return insertedSync;
}

//If depends on multiple different ALU pipelines
//    If all operands type matching the ALU pipelines --> regDist
//    otherwise --> regDistAll
//If depends on single different ALU pipeline and other same ALU pipelines.
//    If all operands type matching the ALU pipelines --> regDist
//    otherwise --> regDistAll
//If depends on multiple same ALU pipelines
//    If all operands type matching the ALU pipeline --> accurate/regDist
//    otherwise--> accuarte
//If depends on single ALU pipeline
//    If operands type matching the ALU pipeline --> accurate/regDist
//    otherwise--> accuarte
//
//Note that:
// 1. one instruction can have multiple operands.
// 2. instruction belongs to single pipeline
//Combo:
//For dpas/dpasw instructions
//      RegDist         SBID.set
//    RegDist         SBID.src
//    RegDist         SBID.dst
//For send instruction
//    RegDistAll     SBID.set
//    RegDistFloat   SBID.set
//    RegDistInt     SBID.set
//For non-send / non-dpas/dpasw instructions
//    RegDist        SBID.dst
//    RegDist        SBID.src
//    RegDistAll     SBID.dst
bool SWSB::insertSyncPVC(G4_BB * bb, SBNode * node, G4_INST * inst, INST_LIST_ITER inst_it, int newInstID, BitSet * dstTokens, BitSet * srcTokens)
{
    G4_INST::DistanceType distType = node->GetInstruction()->getDistanceTypeXe();
    bool operandTypeIndicated = node->GetInstruction()->isOperandTypeIndicated();
    bool insertedSync = false;

    if (tokenHonourInstruction(inst))
    {
        if (inst->getDistance())
        {
            //For dpas/dpasw instructions
            //      RegDist         SBID.set
            //    RegDist         SBID.src
            //    RegDist         SBID.dst
            if (inst->isDpas() ||
                inst->isMathPipeInst()) //math Will be filtered out by tokenHonourInstruction in PVC
            {
                if (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN ||
                    node->getDepTokenNum())
                {
                    if (!operandTypeIndicated)
                    {
                        G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                        synInst->setDistance(inst->getDistance());
                        synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                        inst->setDistance(0);
                        inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                        insertedSync = true;
                    }
                    else if (inst->getDistanceTypeXe() != G4_INST::DistanceType::DIST &&
                        inst->getDistanceTypeXe() != G4_INST::DistanceType::DISTALL)
                    {
                        inst->setDistanceTypeXe(G4_INST::DistanceType::DIST);
                    }
                }
            }

            //For send instruction
            //    RegDistAll     SBID.set
            //    RegDistFloat   SBID.set
            //    RegDistInt     SBID.set
            if (inst->isSend())
            {
                if (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
                {  //SBID.set > SBID.dst > SBID.src > distance
                    if (!(distType == G4_INST::DistanceType::DISTALL ||
                        distType == G4_INST::DistanceType::DISTINT ||
                        distType == G4_INST::DistanceType::DISTFLOAT) || (inst != (*inst_it)))
                    {
                        G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                        synInst->setDistance(inst->getDistance());
                        synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                        inst->setDistance(0);
                        inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                        insertedSync = true;
                    }
                }
                else if (node->getDepTokenNum())  //Keep only the SBID deps in the instruction
                {
                    G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                    synInst->setDistance(inst->getDistance());
                    synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                    inst->setDistance(0);
                    inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                    insertedSync = true;
                }
            }
        }
    }
    else
    {
        //For non-send / non-dpas/dpasw instructions
        //    RegDist        SBID.dst
        //    RegDist        SBID.src
        //    RegDistAll     SBID.dst
        if (inst->getDistance())
        {
            if (inst->opcode() == G4_mad && inst->hasNoACCSBSet())
            {
                G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                synInst->setDistance(inst->getDistance());
                synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                inst->setDistance(0);
                inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                insertedSync = true;
            }
            else if (node->getDepTokenNum())  //Keep only the SBID deps in the instruction
            {
                if (!operandTypeIndicated && distType != G4_INST::DistanceType::DISTALL)
                {
                    G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                    synInst->setDistance(inst->getDistance());
                    synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                    inst->setDistance(0);
                    inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                    insertedSync = true;
                }

                if (operandTypeIndicated && distType != G4_INST::DistanceType::DIST && distType != G4_INST::DistanceType::DISTALL)
                {
                    inst->setDistanceTypeXe(G4_INST::DistanceType::DIST);
                }

                if (distType == G4_INST::DistanceType::DISTALL)
                {
                    bool hasAfterWrite = false;
                    for (int i = 0; i < (int)node->getDepTokenNum(); i++)
                    {
                        unsigned short token = (unsigned short)-1;
                        SWSBTokenType type = SWSBTokenType::TOKEN_NONE;
                        token = node->getDepToken(i, type);
                        if (type == SWSBTokenType::AFTER_WRITE)
                        {
                            hasAfterWrite = true;
                        }
                    }
                    if (!hasAfterWrite)
                    {
                        G4_INST* synInst = insertSyncInstruction(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                        synInst->setDistance(inst->getDistance());
                        synInst->setDistanceTypeXe(inst->getDistanceTypeXe());
                        inst->setDistance(0);
                        inst->setDistanceTypeXe(G4_INST::DistanceType::DIST_NONE);
                        insertedSync = true;
                    }
                }
            }
        }
    }

    bool removeAllTokenDep = (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN);
        removeAllTokenDep = removeAllTokenDep || (inst->opcode() == G4_mad && inst->hasNoACCSBSet());
    //For out-of-order instruction, all dependence token will be moved out to sync
    insertedSync |= insertSyncTokenPVC(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, removeAllTokenDep);

    return insertedSync;
}

void SWSB::insertSync(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens)
{
    //The inst after arch register instruction.
    bool insertedSync = false;
    bool keepDst = false;
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
        if (nextIt != bb->end())
        {
            G4_INST *nextInst = *nextIt;
            if (tokenHonourInstruction(nextInst) ||
                distanceHonourInstruction(nextInst))
            {
                hasValidNextInst = true;
            }
        }
    }

    if (fg.builder->hasFourALUPipes()) //PVC
    {
        insertedSync = insertSyncPVC(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens);
    }
    else if (fg.builder->hasThreeALUPipes()) //XeHP_SDV
    {
        insertedSync = insertSyncXe(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens);
    }
    else //TGLLP
    {
        insertedSync = insertSyncToken(bb, node, inst, inst_it, newInstID, dstTokens, srcTokens, keepDst, false);
    }

    if (node->followDistOneAreg() && insertedSync)
    {
        G4_INST* syncInst = insertSyncInstructionAfter(bb, prevIt, inst->getCISAOff(), inst->getLineNo());
        syncInst->setDistance(1);
        if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
        {
            syncInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
        }
    }

    if (node->hasDistOneAreg() && !hasValidNextInst)
    {
        G4_INST* syncInst = insertSyncInstructionAfter(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
        syncInst->setDistance(1);
        if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
        {
            syncInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
        }
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

    for (G4_BB* bb : fg)
    {
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
                    const G4_INST* nextInst = *tmp_it;

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
                            if (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
                            {
                                dstTokens.set(inst->getSetToken(), false);
                                srcTokens.set(inst->getSetToken(), false);
                            }
                        }
                    }
                }
            }
            else if ((kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) && inst->isSend() && inst->getDst())
            {
                //Stack call is using the NOMASK save and restore.
                //This means there will be RAW dependence generated along the SIMD control flow.
                //Such as in following case, {$1.dst} is required.
                //if()
                //{
                //    ...
                //    R1 --> save();
                //    Fcall_0
                //    R1 <-- retore(); {$1}
                //    ...
                //}
                //else
                //{
                //    ...
                //    R1 --> save() {$1.dst}
                //    Fcall_1
                //    R1 <-- retore();
                //    ...
                //}
                //RAW dependence tracking in SWSB is scalar control flow based, because traditional RA will not generate this kind dependence.
                //At the same time, since we handle the SWSB for stack call conservatively. So we can handle this dependence specially.
                G4_Declare *dstDcl = GetTopDclFromRegRegion((G4_DstRegRegion *)inst->getDst());
                if (std::find(kernel.callerRestoreDecls.begin(), kernel.callerRestoreDecls.end(), dstDcl) != kernel.callerRestoreDecls.end())
                {
                    G4_INST* syncInst = insertSyncInstructionAfter(bb, inst_it, inst->getCISAOff(), inst->getLineNo());
                    unsigned short dstToken = (unsigned short)-1;
                    dstToken = node->getLastInstruction()->getSetToken();
                    syncInst->setToken(dstToken);
                    syncInst->setTokenType(SWSBTokenType::AFTER_WRITE);
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
                if (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
                {
                    dstTokens.set(inst->getSetToken(), false);
                    srcTokens.set(inst->getSetToken(), false);
                }
                //tmp_it keeps the position to insert new generated instructions.
                insertSync(bb, node, inst, tmp_it, newInstID, &dstTokens, &srcTokens);
                unsigned short token = inst->getSetToken();
                if (token != (unsigned short)UNKNOWN_TOKEN)
                {
                    G4_INST* synInst = insertSyncInstruction(bb, tmp_it, inst->getCISAOff(), inst->getLineNo());
                    synInst->setToken(token);
                    synInst->setTokenType(SWSBTokenType::AFTER_WRITE);
                    synInst->setLexicalId(newInstID);
                }
            }
            else
            {
                insertSync(bb, node, inst, inst_it, newInstID, &dstTokens, &srcTokens);
            }

            if (inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
            {
                dstTokens.set(inst->getSetToken(), false);
                srcTokens.set(inst->getSetToken(), false);
            }

            inst->setLexicalId(newInstID);
            for (unsigned i = 1; i < node->instVec.size(); i++)
            {
                inst = *iInstNext;
                inst->setLexicalId(newInstID);
                iInstNext++;
            }

            if (tokenHonourInstruction(inst) && inst->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
            {
                dstTokens.set(inst->getSetToken(), false);
                srcTokens.set(inst->getSetToken(), false);
            }

            newInstID++;
            node_it++;
        }
    }

    tokenProfile.setSyncInstCount(syncInstCount);
    tokenProfile.setMathReuseCount(mathReuseCount);
    tokenProfile.setAWSyncInstCount(AWSyncInstCount);
    tokenProfile.setARSyncInstCount(ARSyncInstCount);
    tokenProfile.setAWSyncAllCount(AWSyncAllCount);
    tokenProfile.setARSyncAllCount(ARSyncAllCount);
    tokenProfile.setTokenReuseCount(tokenReuseCount);
}

void SWSB::dumpDepInfo() const
{
    for (const SBNode* node : SBNodes)
    {
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }

        const G4_INST* inst = node->GetInstruction();
        std::cerr << node->getNodeID() << ":\t";
        inst->dump();
        std::cerr << "Succs:";
        for (const SBDEP_ITEM& curSucc : node->succs)
        {
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
        for (const SBDEP_ITEM& curPred : node->preds)
        {
            std::cerr << curPred.node->getNodeID() << ":" << ((curPred.attr == DEP_EXPLICT) ? "E" : "I") << ", ";
        }
        std::cerr << "\n\n";
    }
}

void SWSB::dumpLiveIntervals() const
{
    std::cerr << "Internal:" << "\n";
    for (const SBNode* node : SBSendNodes)
    {
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }
        node->dumpInterval();
    }
}

void SWSB::dumpTokeAssignResult() const
{
    std::cerr << "Internal:" << "\n";
    for (const SBNode* node : SBSendNodes)
    {
        if (node->GetInstruction()->isEOT())
        {
            continue;
        }
        node->dumpAssignedTokens();
    }
}

void SWSB::dumpSync(const SBNode* tokenNode, const SBNode* syncNode, unsigned short token, SWSBTokenType type) const
{
    std::cerr << "#" << syncNode->getNodeID() << "(" << token << ",";
    std::cerr << ((type == SWSBTokenType::AFTER_READ) ? "AR" : "AW") << ")";
    std::cerr << ": " << "#" << tokenNode->getNodeID() << "(" << tokenNode->getLiveStartID() << "-" << tokenNode->getLiveEndID() << ")\n";
}

void SWSB::buildLiveIntervals()
{
    // For all send nodes
    // Set the live ranges according to dependence edges
    const bool trueDepOnly = fg.builder->getOptions()->getOption(vISA_TrueDepOnly);
    for (SBNode* node : SBSendNodes)
    {
        node->setLiveEarliestID(node->getNodeID(), node->getBBID());
        node->setLiveLatestID(node->getNodeID(), node->getBBID());
        for (SBDEP_ITEM& curSucc : node->succs)
        {
            const SBNode* succ = curSucc.node;
            if (trueDepOnly && node->GetInstruction()->isDpas() && node->getBBID() != succ->getBBID())
            {
                node->setLiveLatestID(BBVector[node->getBBID()]->last_node, node->getBBID());
            }
            else
            {
                node->setLiveLatestID(succ->getNodeID(), succ->getBBID());
            }
        }
    }

#ifdef DEBUG_VERBOSE_ON
    dumpLiveIntervals();
    dumpDepInfo();
#endif

    //For global send nodes
    //According to layout, extend the live range of each send operand to
    //the start of the first live in BB and end of last live out BB
    for (BB_LIST_ITER ib(fg.begin()), bend(fg.end()); ib != bend; ++ib)
    {
        unsigned bbID = (*ib)->getId();
        G4_BB_SB* sb_bb = BBVector[bbID];
        SBBitSets& send_live_in = sb_bb->send_live_in;
        SBBitSets& send_live_out = sb_bb->send_live_out;
        SBBitSets& send_live_in_scalar = sb_bb->send_live_in_scalar;
        SBBitSets& send_live_out_scalar = sb_bb->send_live_out_scalar;

        if (send_live_in.isEmpty())
        {
            continue;
        }

        for (SBBucketNode* bucketNode : globalSendOpndList)
        {
            SBNode* node = bucketNode->node;
            int globalID = node->globalID;

            if (trueDepOnly && node->GetInstruction()->isDpas())
            {
                continue;
            }

            if (bucketNode->opndNum == Opnd_dst)
            {
                if (sb_bb->first_node != -1 &&
                    send_live_in_scalar.isDstSet((unsigned)globalID))
                {
                    if (!(*ib)->Preds.empty() || !(sb_bb->Preds.empty()))
                    {
                        node->setLiveEarliestID(sb_bb->first_node, bbID);
                    }
                }
                //FIXME: implicit dependence still have issue.
                //the live range of implicit dependence may not counted. But that's ok? This may cause the delay. ...
                if (sb_bb->first_node != -1 &&
                    send_live_out_scalar.isDstSet((unsigned)globalID))
                {
                    if (!(*ib)->Succs.empty() || !(sb_bb->Succs.empty()))
                    {
                        node->setLiveLatestID(sb_bb->last_node, bbID);
                    }
                }
            }
            else if (!trueDepOnly)
            {
                if (sb_bb->first_node != -1 &&
                    send_live_in.isSrcSet((unsigned)globalID))
                {
                    if (!(*ib)->Preds.empty() || !(sb_bb->Preds.empty()))
                    {
                        node->setLiveEarliestID(sb_bb->first_node, bbID);
                    }
                }
                //FIXME: implicit dependence still have issue.
                //the live range of implicit dependence may not counted. But that's ok? This may cause the delay. ...
                if (sb_bb->first_node != -1 &&
                    send_live_out.isSrcSet((unsigned)globalID))
                {
                    if (!(*ib)->Succs.empty() || !(sb_bb->Succs.empty()))
                    {
                        node->setLiveLatestID(sb_bb->last_node, bbID);
                    }
                }
            }
        }
    }
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

    SBBitSets temp_live_in(globalSendNum);
    temp_live_in = BBVector[bbID]->send_live_in;

    for (const G4_BB* predBB : bb->Preds)
    {
        unsigned predID = predBB->getId();
        temp_live_in |= BBVector[predID]->send_live_out;
    }

    if (temp_live_in != BBVector[bbID]->send_live_in)
    {
        changed = true;
        BBVector[bbID]->send_live_in = temp_live_in;
    }

    //Record the killed dst and src in scalar CF iterating
    SBBitSets temp_kill(globalSendNum);
    temp_kill = temp_live_in;
    temp_kill &= BBVector[bbID]->send_may_kill;
    BBVector[bbID]->send_kill_scalar |= temp_kill;

    temp_kill = temp_live_in;
    temp_kill.src &= BBVector[bbID]->send_may_kill.dst;
    BBVector[bbID]->send_kill_scalar.src |= temp_kill.src;

    //Kill nodes
    //once dst is killed, src definitely is killed
    temp_live_in -= BBVector[bbID]->send_may_kill;
    temp_live_in.src -= BBVector[bbID]->send_may_kill.dst;

    BBVector[bbID]->send_live_out |= temp_live_in;

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

    SBBitSets temp_live_in(globalSendNum);
    temp_live_in = BBVector[bbID]->send_live_in;

    for (BB_SWSB_LIST_ITER it = BBVector[bbID]->Preds.begin(); it != BBVector[bbID]->Preds.end(); it++)
    {
        G4_BB* predBB = (*it)->getBB();
        unsigned predID = predBB->getId();
        temp_live_in |= BBVector[predID]->send_live_out;
    }

    if (temp_live_in != BBVector[bbID]->send_live_in)
    {
        changed = true;
        BBVector[bbID]->send_live_in = temp_live_in;
    }

    //Kill scalar kills
    temp_live_in -= BBVector[bbID]->send_kill_scalar;
    temp_live_in.src -= BBVector[bbID]->send_may_kill.src;
    temp_live_in.dst -= BBVector[bbID]->send_WAW_may_kill;

    BBVector[bbID]->send_live_out |= temp_live_in;

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
        activateLiveIn |= BBVector[i]->liveInTokenNodes;

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
                                //1. Conflict with outer loop
                                //2. There is no preds info required any more in following handling
                                predNode->succs.erase(succ_it);
                                prunedEdgeNum++;
                                if (predNode->globalID != -1)
                                {
                                    if (predNode->getBBID() != node->getBBID() &&
                                        !killedToken.isSet(predNode->getLastInstruction()->getSetToken()) &&
                                        (!(fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
                                           fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation)) ||
                                        !((fg.builder->getOptions()->getOption(vISA_GlobalTokenAllocation) ||
                                            fg.builder->getOptions()->getOption(vISA_DistPropTokenAllocation)) &&
                                          BBVector[node->getBBID()]->dominators.isSet(predNode->getBBID()))))
                                    {
                                        prunedDiffBBEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Diff BB Token: " << predNode->getLastInstruction()->getSetToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                    else if (predNode->getBBID() != node->getBBID())
                                    {
                                        prunedDiffBBSameTokenEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Diff BB Same Token: " << predNode->getLastInstruction()->getSetToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                    else
                                    {
                                        prunedGlobalEdgeNum++;
#ifdef DEBUG_VERBOSE_ON
                                        std::cerr << "Global Token: " << predNode->getLastInstruction()->getSetToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
#endif
                                    }
                                }
#ifdef DEBUG_VERBOSE_ON
                                else
                                {
                                    std::cerr << "Local Token: " << predNode->getLastInstruction()->getSetToken() << " <Pred: " << predNode->getNodeID() << ", Succ: " << node->getNodeID() << ">" << std::endl;;
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
                            int token = predNode->getLastInstruction()->getSetToken();
                            if (token != (unsigned short)UNKNOWN_TOKEN)
                            {
                                activateLiveIn -= allTokenNodesMap[token].bitset;
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
                int token = node->getLastInstruction()->getSetToken();
                if (token != (unsigned short)UNKNOWN_TOKEN)
                {
                    activateLiveIn -= allTokenNodesMap[token].bitset;
                    activateLiveIn.set(node->sendID, true);
                }
            }
        }
    }
}

void G4_BB_SB::getLiveOutToken(unsigned allSendNum,
    const SBNODE_VECT* SBNodes)
{
    //Empty BB
    if (first_node == -1)
    {
        return;
    }

    uint32_t totalTokenNum = builder.kernel.getNumSWSBTokens();
    unsigned* liveNodeID = (unsigned*)mem.alloc(sizeof(unsigned) * totalTokenNum);

    if (tokeNodesMap.size() == 0)
    {
        tokeNodesMap.resize(totalTokenNum);

        //Each token ID has a bitset for all possible send instructions' ID
        for (size_t i = 0; i < totalTokenNum; i++)
        {
            tokeNodesMap[i] = BitSet(allSendNum, false);
            liveNodeID[i] = 0;
        }
    }
    else
    {
        for (size_t i = 0; i < totalTokenNum; i++)
        {
            tokeNodesMap[i].clear();
            liveNodeID[i] = 0;
        }
    }

    // Scan instructions forward to get the live out of current BB
    for (int i = first_node; i <= last_node; i++)
    {
        SBNode* node = (*SBNodes)[i];

        //Check the previous node.
        for (const SBDEP_ITEM& curPred : node->preds)
        {
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
                if (predNode->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
                {
                    unsigned short token = predNode->getLastInstruction()->getSetToken();
                    // 1:  send r112                   {$9}
                    // 2:  send r18                    {$9}
                    // 3:  send r112                   {$9}
                    // 4:  send xxx,     r18           {12}
                    //
                    // Instruction 4 may clear the $9 because of instruction 2
                    // liveNodeID is used to track the live node id of each send. predNode can kill
                    if (liveNodeID[token] < predNode->getNodeID())
                    {
                        tokeNodesMap[token].clear(); //Kill all dependence in following instructions with the same token

                        //Record the killed token by current BB, Kill may kill all previous nodes which reach current node
                        killedTokens.set(token, true);  //Set previous token send killed in current BB
                    }
                }
            }
        }

        //Token reuse will kill all previous nodes with same token
        //Will have only one?, yes, for BB local scan
        if (tokenHonourInstruction(node->getLastInstruction()) &&
            !node->getLastInstruction()->isEOT() &&
            node->getLastInstruction()->getSetToken() != (unsigned short)UNKNOWN_TOKEN)
        {
            unsigned short token = node->getLastInstruction()->getSetToken();
            tokeNodesMap[token].clear();

            //For future live in, will always be killed by current instruction
            killedTokens.set(token, true);

            //Current node may be in live out, if not be killed in following insts.
            tokeNodesMap[token].set(node->sendID, true);
            liveNodeID[token] = node->getNodeID();
        }
    }

    for (size_t i = 0; i < totalTokenNum; i++)
    {
        liveOutTokenNodes |= tokeNodesMap[i];
    }
}
//
// Scan to check which global send operand for sends will be killed by current BB.
// Note that there is no guarantee the send operand will in the live in set of BB.
// !!! Note that: since this "may kill" info is used in global analysis, "may kill" is not accurate, here we in fact record the "definitely kill".
void G4_BB_SB::setSendOpndMayKilled(LiveGRFBuckets* globalSendsLB,
    SBNODE_VECT* SBNodes,
    PointsToAnalysis& p)
{
    std::vector<SBBucketDesc> BDvec;
    if (first_node == -1)
    {
        return;
    }

    bool addGlobalSLMWARWA = false;
    for (int i = first_node; i <= last_node; i++)
    {
        SBNode* node = (*SBNodes)[i];
        G4_INST* curInst = (*SBNodes)[i]->GetInstruction();

        if (curInst->isLabel())
        {
            continue;
        }

        BDvec.clear();
        getGRFBucketDescs(node, BDvec, true);
        if (!BDvec.size())
        {
            continue;
        }

        // For all bucket descriptors of curInst
        for (const SBBucketDesc& BD : BDvec) {
            const int& curBucket = BD.bucket;
            const Gen4_Operand_Number& curOpnd = BD.opndNum;
            const SBFootprint* curFootprint = BD.footprint;

            for (LiveGRFBuckets::BN_iterator bn_it = globalSendsLB->begin(curBucket);
                bn_it != globalSendsLB->end(curBucket);)
            {
                SBBucketNode* liveBN = (*bn_it);
                SBNode* curLiveNode = liveBN->node;
                Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                const SBFootprint* liveFootprint = liveBN->footprint;
                G4_INST* liveInst = liveFootprint->inst;

                //Send operands are all GRF aligned, there is no overlap checking required.
                //Fix me, this is not right, for math instruction, less than 1 GRF may happen.
                //Find DEP type
                unsigned short internalOffset = 0;
                bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);
                if (!hasOverlap)
                {
                    ++bn_it;
                    continue;
                }

                DepType dep = getDepForOpnd(liveOpnd, curOpnd);

                //For SBID global liveness analysis, both explicit and implicit kill counted.
                if (dep == RAW || dep == WAW)
                {
                    send_may_kill.setDst(curLiveNode->globalID, true);
                    if (dep == WAW)
                    {
                        send_WAW_may_kill.set(curLiveNode->globalID, true);
                    }
                }

                if (dep == WAR &&
                    WARDepRequired(liveInst, curFootprint->inst))
                {
                    send_may_kill.setSrc(curLiveNode->globalID, true);
                }

                //FIXME: for NODEP, there is optimization chance.
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

        if (!addGlobalSLMWARWA && builder.hasSLMWARIssue() && curInst->isSend() &&
            (isSLMMsg(curInst) && (curInst->getDst() == nullptr || isFence(curInst))))
        {
            for (int curBucket = 0; curBucket < globalSendsLB->getNumOfBuckets(); curBucket++)
            {
                for (LiveGRFBuckets::BN_iterator bn_it = globalSendsLB->begin(curBucket);
                    bn_it != globalSendsLB->end(curBucket);)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    G4_INST* liveInst = liveBN->footprint->inst;

                    if (liveInst->isSend() &&
                        isSLMMsg(liveInst) && liveInst->getDst() != nullptr && !liveInst->getDst()->isNullReg())
                    {
                        send_may_kill.setDst(curLiveNode->globalID, true);
                    }
                    ++bn_it;
                }
            }
            addGlobalSLMWARWA = true;
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

    if ((builder.hasThreeALUPipes() || builder.hasFourALUPipes()))
    {
        if (isAccReg)
        {
            footprint = getFootprintForACC(opnd, opndNum, inst);
            node->setFootprint(footprint, opndNum);
        }
        if (isFlagReg)
        {
            footprint = getFootprintForFlag(opnd, opndNum, inst);
            node->setFootprint(footprint, opndNum);
        }
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
    MUST_BE_TRUE(addrdcl != nullptr, "address declare can not be nullptr");

#ifdef DEBUG_VERBOSE_ON
    std::cerr << addrdcl->getName() << ":" << std::endl;
    std::cerr << node->getNodeID() << ":";
    node->GetInstruction()->dump();
    std::cerr << "Point to: ";
#endif

    G4_RegVar* ptvar = nullptr;
    int vid = 0;
    unsigned char offset = 0;
    while ((ptvar = p.getPointsTo(addrdcl->getRegVar(), vid++, offset)) != nullptr)
    {

        uint32_t varID = ptvar->getId();
        G4_Declare* dcl = ptvar->getDeclare()->getRootDeclare();
        G4_RegVar* var = dcl->getRegVar();

        MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
        MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

        int linearizedStart = 0;
        int linearizedEnd = 0;
        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
        uint32_t regOff = var->getPhyRegOff();

        {
            linearizedStart = regNum * builder.numEltPerGRF<Type_UB>() + regOff * TypeSize(dcl->getElemType());
            linearizedEnd = linearizedStart + dcl->getByteSize() - 1;
        }

        void* allocedMem = mem.alloc(sizeof(SBFootprint));
        footprint = new (allocedMem)SBFootprint(GRF_T, type, (unsigned short)linearizedStart, (unsigned short)linearizedEnd, node->GetInstruction());
        node->setFootprint(footprint, opnd_num);
#ifdef DEBUG_VERBOSE_ON
        int startingBucket = linearizedStart / builder.numEltPerGRF<Type_UB>();
        int endingBucket = linearizedEnd / builder.numEltPerGRF<Type_UB>();
        std::cerr << dcl->getName() << "<" << startingBucket << "," << endingBucket << ">";
#endif
    }
#ifdef DEBUG_VERBOSE_ON
    std::cerr << std::endl;
#endif
    return;
}

//Create Buckets
void G4_BB_SB::getGRFBuckets(const SBFootprint* footprint,
    Gen4_Operand_Number opndNum,
    std::vector<SBBucketDesc>& BDvec,
    bool GRFOnly)
{
    for (const SBFootprint* curFootprint = footprint; curFootprint != nullptr; curFootprint = curFootprint->next)
    {
        if (GRFOnly && (curFootprint->fType != GRF_T))
        {
            continue;
        }

        int startingBucket = curFootprint->LeftB / builder.numEltPerGRF<Type_UB>();
        int endingBucket = curFootprint->RightB / builder.numEltPerGRF<Type_UB>();
        for (int j = startingBucket; j <= endingBucket ; j++)
        {
            BDvec.push_back(SBBucketDesc(j, opndNum, curFootprint));
        }
    }
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
    std::vector<SBBucketDesc>& BDvec,
    bool GRFOnly)
{
    for (Gen4_Operand_Number opndNum = first_opnd; opndNum <= last_opnd; opndNum = (Gen4_Operand_Number)(opndNum + 1))
    {
        const SBFootprint* footprint = node->getFirstFootprint(opndNum);
        if (!footprint || (GRFOnly && (footprint->fType != GRF_T)))
        {
            continue;
        }
        getGRFBuckets(footprint, opndNum, BDvec, GRFOnly);
    }

    return;
}

bool G4_BB_SB::hasIndirectSource(SBNode* node)
{
    if (!distanceHonourInstruction(node->GetInstruction()))
    {
        return false;
    }

    for (Gen4_Operand_Number opndNum = Opnd_src0; opndNum <= Opnd_src3; opndNum = (Gen4_Operand_Number)(opndNum + 1))
    {
        G4_Operand* opnd = node->GetInstruction()->getOperand(opndNum);

        if (!opnd || !opnd->getBase())
        {
            continue;
        }

        if (opnd->isLabel() || opnd->isImm())
        {
            continue;
        }

        if (!opnd->isSrcRegRegion())
        {
            continue;
        }
        if (hasIndirection(opnd, opndNum))
        {
            return true;
        }
    }

    return false;
}

bool G4_BB_SB::getGRFFootPrint(SBNode* node, PointsToAnalysis& p)
{
    bool hasDistOneAReg = false;
    //We get the description for source first, so for current instruction, the scan order is src0, src1, src2, src3, dst
    for (G4_INST* inst : node->instVec)
    {
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_src0, Opnd_src3, p);
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_pred, Opnd_implAccDst, p);
        hasDistOneAReg |= getGRFFootPrintOperands(node, inst, Opnd_dst, Opnd_dst, p);
    }

    return hasDistOneAReg;
}

void G4_BB_SB::getGRFBucketDescs(SBNode* node, std::vector<SBBucketDesc>& BDvec, bool GRFOnly)
{
    //We get the description for source first, so for current instruction, the scan order is src0, src1, src2, src3, dst
    getGRFBucketsForOperands(node, Opnd_src0, Opnd_src3, BDvec, GRFOnly);
    if (!GRFOnly)
    {
        getGRFBucketsForOperands(node, Opnd_pred, Opnd_implAccDst, BDvec, GRFOnly);
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
void G4_BB_SB::clearKilledBucketNodeXeLP(LiveGRFBuckets* LB, int ALUID)
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

void G4_BB_SB::clearKilledBucketNodeXeHP(LiveGRFBuckets* LB, int integerID, int floatID, int longID, int mathID)
{
    for (int curBucket = 0; curBucket < LB->getNumOfBuckets(); curBucket++)
    {
        for (LiveGRFBuckets::BN_iterator it = LB->begin(curBucket); it != LB->end(curBucket);)
        {
            SBBucketNode* liveBN = (*it);
            SBNode* curLiveNode = liveBN->node;

            if (curLiveNode->isInstKilled() ||
                (curLiveNode->isSourceKilled() &&
                    liveBN->opndNum >= Opnd_src0 &&
                    liveBN->opndNum <= Opnd_src3))
            {
                LB->killOperand(it);
                continue;
            }

            //Long pipeline must be checked first because it's definition is different with Integer and Float
            if (curLiveNode->GetInstruction()->isLongPipeInstructionXe() &&
                ((longID - curLiveNode->getLongID()) > SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT))
            {
                LB->killOperand(it);
                continue;
            }

            if (curLiveNode->GetInstruction()->isIntegerPipeInstructionXe() &&
                ((integerID - curLiveNode->getIntegerID()) > SWSB_MAX_ALU_DEPENDENCE_DISTANCE))
            {
                LB->killOperand(it);
                continue;
            }

            if (curLiveNode->GetInstruction()->isFloatPipeInstructionXe() &&
                ((floatID - curLiveNode->getFloatID()) > SWSB_MAX_ALU_DEPENDENCE_DISTANCE))
            {
                LB->killOperand(it);
                continue;
            }

            if (curLiveNode->GetInstruction()->isMath() &&
                builder.hasFixedCycleMathPipe() &&
                (mathID - curLiveNode->getMathID() > SWSB_MAX_MATH_DEPENDENCE_DISTANCE))
            {
                LB->killOperand(it);
                continue;
            }

            ++it;
        }
    }
}

void G4_BB_SB::clearSLMWARWAissue(SBNode* curNode, LiveGRFBuckets* LB)
{
    for (int curBucket = 0; curBucket < LB->getNumOfBuckets(); curBucket++)
    {
        for (LiveGRFBuckets::BN_iterator it = LB->begin(curBucket); it != LB->end(curBucket);)
        {
            SBBucketNode* liveBN = (*it);
            SBNode* curLiveNode = liveBN->node;
            G4_INST* liveInst = liveBN->footprint->inst;

            if (liveInst->isSend() &&
                isSLMMsg(liveInst) && liveInst->getDst() != nullptr && !liveInst->getDst()->isNullReg())
            {
                createAddGRFEdge(curLiveNode, curNode, RAW, DEP_EXPLICT);
                curLiveNode->setInstKilled(true);  //Instruction level kill
                LB->killOperand(it);
                continue;
            }

            ++it;
        }
    }
}

void G4_BB_SB::setDistance(const SBFootprint* footprint, SBNode* node, SBNode* liveNode, bool dstDep)
{
    if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
    {
        unsigned prevID = 0;
        unsigned currentID = 0;
        switch (liveNode->ALUPipe)
        {
        case PIPE_INT:
            prevID = liveNode->getIntegerID();
            if (prevID < latestDepALUID[PIPE_INT])
            {
                return;
            }
            latestDepALUID[PIPE_INT] = prevID;
            currentID = node->ALUPipe == PIPE_INT ? node->getIntegerID() : integerID;
            break;
        case PIPE_FLOAT:
            prevID = liveNode->getFloatID();
            if (prevID < latestDepALUID[PIPE_FLOAT])
            {
                return;
            }
            latestDepALUID[PIPE_FLOAT] = prevID;
            currentID = node->ALUPipe == PIPE_FLOAT ? node->getFloatID() : floatID;
            break;
        case PIPE_LONG:
            prevID = liveNode->getLongID();
            if (prevID < latestDepALUID[PIPE_LONG])
            {
                return;
            }
            latestDepALUID[PIPE_LONG] = prevID;
            currentID = node->ALUPipe == PIPE_LONG ? node->getLongID() : longID;
            break;
        case PIPE_MATH:
            prevID = liveNode->getMathID();
            if (prevID < latestDepALUID[PIPE_MATH])
            {
                return;
            }
            latestDepALUID[PIPE_MATH] = prevID;
            currentID = node->ALUPipe == PIPE_MATH ? node->getMathID() : mathID;
            break;
        default:
            assert(0 && "None ALU pipe");
            return;
        }
        SBDISTDEP_ITEM depItem;
        depItem.liveNodePipe = liveNode->ALUPipe;
        depItem.nodePipe = node->ALUPipe;
        depItem.operandType = node->GetInstruction()->getDataTypePipeXe(footprint->type);
        depItem.dstDep = dstDep;
        if (node->GetInstruction()->isSend())
        {
            depItem.operandType = PIPE_SEND;
        }
        assert(currentID > prevID && "Wrong node ALU ID");
        node->setDistance(currentID - prevID);
        node->distDep.push_back(depItem);
    }
    else
    {
        auto dist = node->getALUID() - liveNode->getALUID();
        assert(dist <= liveNode->getMaxDepDistance() && "dist should not exceed the max dep distance");
        node->setDistance(dist);
    }
}

void G4_BB_SB::setSpecialDistance(SBNode* node)
{
    G4_INST* inst = node->GetInstruction();
    if (!inst->getDst())
    {
        return;
    }

    bool isIndirect = false;
    if (inst->getDst()->isDstRegRegion())
    {
        isIndirect = inst->getDst()->asDstRegRegion()->isIndirect();
    }

    if (inst->getDst()->isA0() && !isIndirect)
    {
        SBDISTDEP_ITEM depItem;
        depItem.liveNodePipe = PIPE_FLOAT;
        depItem.nodePipe = node->ALUPipe;
        depItem.operandType = PIPE_INT;
        depItem.dstDep = false;
        node->setDistance(1);
        node->distDep.push_back(depItem);
    }

    return;
}
//The merged footprint is ordered from back to front instructions in the macro
//As a result if killed, is the back instruction killed, which means front instructions are killed as well.
void G4_BB_SB::footprintMerge(SBNode* node, const SBNode* nextNode)
{
    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_dst})
    {
        SBFootprint* nextfp = nextNode->getFirstFootprint(opndNum);

        if (nextfp != nullptr)
        {
            if (node->GetInstruction()->isDpas())
            {
                nextfp->setOffset(node->getDPASSize());
            }
            node->setFootprint(nextfp, opndNum);
        }
    }

    return;
}

bool G4_BB_SB::hasInternalDependenceWithinDPAS(SBNode* node)
{
    const SBFootprint* dstfp = node->getFirstFootprint(Opnd_dst);

    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2})
    {
        const SBFootprint* srcfp = node->getFirstFootprint(opndNum);
        unsigned short internalOffset = 0;
        if (dstfp->hasOverlap(srcfp, internalOffset))
        {
            if (opndNum == Opnd_src1)
            {
                assert(0);
            }
            //For 8X8, it's allowed that dst and src0 share same registers (not internal dep). But not including partial overlap.
            if (opndNum == Opnd_src0)
            {
                const G4_INST* curInst = node->getLastInstruction();
                const G4_InstDpas* dpasInst = curInst->asDpasInst();
                uint8_t D = dpasInst->getSystolicDepth();

                if (D == 8) //Works only for 8x8
                {
                    if ((dstfp->LeftB == srcfp->LeftB) && (dstfp->RightB == srcfp->RightB))
                    {
                        continue;
                    }
                }
            }

            return true;
        }
    }

    return false;
}

//No RAW/WAW dependence within a DPAS macro
bool G4_BB_SB::hasDependenceBetweenDPASNodes(SBNode* node, SBNode* nextNode)
{
    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_dst})
    {
        const SBFootprint* fp = node->getFirstFootprint(opndNum);
        if (fp && opndNum == Opnd_dst)
        {
            for (Gen4_Operand_Number opndNum2
                : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_dst})
            {
                const SBFootprint* nextfp = nextNode->getFirstFootprint(opndNum2);
                unsigned short internalOffset = 0;
                if (nextfp && nextfp->hasOverlap(fp, internalOffset))
                {
                    //Exception: if the dependence distance is far enough, it's ok
                    if (node->getDPASSize() - internalOffset > tokenAfterDPASCycle)
                    {
                        return false;
                    }

                    return true;
                }
            }
        }
    }

    return false;
}

#define SRC2_CACHE_SIZE 1024
bool G4_BB_SB::src2FootPrintCachePVC(SBNode * curNode, SBNode * nextNode) const
{
    BitSet cachedGRF(totalGRFNum, false);

    for (const SBFootprint* fp = curNode->getFirstFootprint(Opnd_src2); fp; fp = fp->next)
    {
        unsigned short leftB = fp->LeftB / builder.numEltPerGRF<Type_UB>();
        unsigned short rightB = fp->RightB / builder.numEltPerGRF<Type_UB>();
        for (unsigned short i = leftB; i <= rightB; i++)
        {
            cachedGRF.set(i, true);
        }
    }

    for (const SBFootprint* fp = nextNode->getFirstFootprint(Opnd_src2); fp; fp = fp->next)
    {
        unsigned short leftB = fp->LeftB / builder.numEltPerGRF<Type_UB>();
        unsigned short rightB = fp->RightB / builder.numEltPerGRF<Type_UB>();
        for (unsigned short i = leftB; i <= rightB; i++)
        {
            cachedGRF.set(i, true);
        }
    }

    unsigned short cachedGRFNum = 0;
    for (unsigned short i = 0; i < totalGRFNum; i++)
    {
        if (cachedGRF.isSet(i))
        {
            cachedGRFNum++;
        }
    }

    return cachedGRFNum <= (SRC2_CACHE_SIZE + builder.numEltPerGRF<Type_UB>() - 1) / builder.numEltPerGRF<Type_UB>();
}

bool G4_BB_SB::src2SameFootPrintDiffType(SBNode * curNode, SBNode * nextNode) const
{
    for (const SBFootprint* fp = curNode->getFirstFootprint(Opnd_src2); fp; fp = fp->next)
    {
        unsigned short leftB = fp->LeftB / builder.numEltPerGRF<Type_UB>();
        unsigned short rightB = fp->RightB / builder.numEltPerGRF<Type_UB>();
        G4_Type type = fp->type;

        for (const SBFootprint* nextfp = nextNode->getFirstFootprint(Opnd_src2); nextfp; nextfp = nextfp->next)
        {
            unsigned short nextLeftB = nextfp->LeftB / builder.numEltPerGRF<Type_UB>();
            unsigned short nextRightB = nextfp->RightB / builder.numEltPerGRF<Type_UB>();
            G4_Type nextType = nextfp->type;

            if (!(nextLeftB > rightB || nextRightB < leftB))
            {
                if (type != nextType)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

//restrict a macro to :
//    1. Consecutive instructions of same opcode, same datatype in all sources and dest and same register for Src1.
//  2. Allow having variable repeat count
bool G4_BB_SB::isLastDpas(SBNode* curNode, SBNode* nextNode)
{
    G4_INST* curInst = curNode->getLastInstruction();
    G4_INST* nextInst = nextNode->GetInstruction();
    if (nextInst == nullptr || !nextInst->isDpas())
    {
        return true;
    }

    if (!hasSameExecMask(curInst, nextInst))
    {
        return true;
    }
    //All types should be same for all operands.
    for (Gen4_Operand_Number opndNum
        : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_dst})
    {
        if (curNode->getFirstFootprint(opndNum) && nextNode->getFirstFootprint(opndNum) &&
            curNode->getFirstFootprint(opndNum)->type != nextNode->getFirstFootprint(opndNum)->type)
        {
            return true;
        }
    }

    G4_InstDpas* dpasInst = curInst->asDpasInst();
    G4_Operand* srcOpnd1 = curInst->getSrc(1);
    G4_Operand* srcOpnd2 = curInst->getSrc(2);
    unsigned short leftBound1 = srcOpnd1->getLinearizedStart();
    unsigned short leftBound2 = srcOpnd2->getLinearizedStart();
    uint8_t curD = dpasInst->getSystolicDepth();
    uint8_t curC = dpasInst->getRepeatCount();
    int curSrc1Reg = leftBound1 / builder.numEltPerGRF<Type_UB>();
    int curSrc2Reg = leftBound2 / builder.numEltPerGRF<Type_UB>();

    G4_InstDpas* nextDpasInst = nextInst->asDpasInst();
    uint8_t nextD = nextDpasInst->getSystolicDepth();
    uint8_t nextC = nextDpasInst->getRepeatCount();

    //Same depth
    if (curD != nextD)
    {
        return true;
    }

    if (VISA_WA_CHECK(builder.getPWaTable(), Wa_16011859583) ||
        VISA_WA_CHECK(builder.getPWaTable(), Wa_14012420496) ||
        builder.getOption(vISA_NoDPASMacro))
    {
        if (curD != 8 || nextD != 8 || curC != 8 || nextC != 8)
        {
            return true;
        }
    }

    srcOpnd1 = nextDpasInst->getSrc(1);
    srcOpnd2 = nextDpasInst->getSrc(2);
    leftBound1 = srcOpnd1->getLinearizedStart();
    leftBound2 = srcOpnd2->getLinearizedStart();
    int nextSrc1Reg = leftBound1 / builder.numEltPerGRF<Type_UB>();
    int nextSrc2Reg = leftBound2 / builder.numEltPerGRF<Type_UB>();

    if (builder.hasSrc2ReadSupression() &&
        builder.hasSrc2ReadSupressionSameRegSameType() &&
        src2SameFootPrintDiffType(curNode, nextNode))
    {
        return true;
    }

    if (builder.getOption(vISA_forceDPASMacro))
    {
        return false;
    }

    //src1 or src2 read suppression
    if (curSrc1Reg == nextSrc1Reg ||
        (builder.hasSrc2ReadSupression() &&  (curSrc2Reg == nextSrc2Reg &&
            curC == nextC &&
            curC == 8)))
    {
        return false;
    }

    //Src2 read suppression with GRF cache.
    //Using {Atomic} in the last line of a macro (such as in the lines I highlighted) has some implications in the hardware implementation:
    //1. In 8x8 macros (such as the one you pasted) is fine.
    //2. In other repetitions, it will cause that the src1 of the next macro will be ignored.
    // Hardware uses {Atomic} to indicate that the next instruction will reuse the src1. In an 8x8, they always verify

    if (builder.hasSrc2ReadSupression() &&
        curC == nextC &&
        curC == 8 &&
        src2FootPrintCachePVC(curNode, nextNode) &&
        curNode->getFirstFootprint(Opnd_src2)->isWholeOverlap(nextNode->getFirstFootprint(Opnd_src2)))
    {
        return false;
    }

    return true;
}

void G4_BB_SB::pushItemToQueue(std::vector<unsigned> *nodeIDQueue, unsigned nodeID)
{
    nodeIDQueue->push_back(nodeID);

    if (nodeIDQueue->size() > SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE)
    {
        nodeIDQueue->erase(nodeIDQueue->begin());
    }
}

bool G4_BB_SB::hasInternalDependence(SBNode* nodeFirst, SBNode* nodeNext)
{
    for (Gen4_Operand_Number opndNum1
        : {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2})
    {
        const SBFootprint* firstfp = nodeFirst->getFirstFootprint(opndNum1);

        for (Gen4_Operand_Number opndNum2
            : {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2})
        {
            if (opndNum1 > Opnd_dst && opndNum2 > Opnd_dst) //Don't track read after read.
            {
                continue;
            }

            const SBFootprint* secondfp = nodeNext->getFirstFootprint(opndNum2);
            unsigned short internalOffset = 0;
            if (firstfp->hasOverlap(secondfp, internalOffset))
            {
                return true;
            }
        }
    }

    return false;
}


bool G4_BB_SB::is2xDPBlockCandidate(G4_INST* inst, bool accDST)
{
    if (inst->opcode() != G4_mad)
    {
        return false;
    }

    if (inst->getPredicate())
    {
        return false;
    }

    if (inst->getExecSize() != g4::SIMD16)
    {
        return false;
    }

    if (!inst->getDst() || inst->getDst()->isNullReg())
    {
        return false;
    }

    if (accDST && !inst->getDst()->isAccReg())
    {
        return false;
    }

    for (Gen4_Operand_Number opndNum
        : {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2})
    {
        G4_Operand* opnd = inst->getOperand(opndNum);

        if (opnd->getType() != G4_Type::Type_DF)
        {
            return false;
        }
    }

    return true;
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
    integerID = indexes->integerIndex;
    floatID = indexes->floatIndex;
    longID = indexes->longIndex;
    DPASID = indexes->DPASIndex;
    mathID = indexes->mathIndex;
    first_DPASID = indexes->DPASIndex;

    for (int i = 0; i < PIPE_DPAS; i++)
    {
        latestDepALUID[i] = indexes->latestDepALUID[i];
        latestInstID[i] = &indexes->latestInstID[i];
    }
    SBNODE_LIST tmpSBSendNodes;
    bool hasFollowDistOneAReg = false;
    bool hasFollowDistOneIndirectReg = false;

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
        curInst->setLocalId(0);

        if (builder.hasA0WARHWissue() && (builder.hasThreeALUPipes() || builder.hasFourALUPipes()))
        {
            setSpecialDistance(node);
        }
        //Record the node IDs of the instructions in BB
        if (first_node == -1)
        {
            first_node = nodeID;
        }
        last_node = nodeID;
        nodeID++;

        //For architecture registers ce#, sp, sr0.#, cr0.#, ip, tm0, dbg0, set distance 1
        if (hasFollowDistOneAReg || hasFollowDistOneIndirectReg)
        {
            node->setDistance(1);
            node->setFollowDistOneAReg();
            hasFollowDistOneAReg = false;
            hasFollowDistOneIndirectReg = false;
            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                node->instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            }
        }

        hasFollowDistOneIndirectReg = builder.getOption(vISA_InsertDummyMovForHWRSWA) &&
            (VISA_WA_CHECK(builder.getPWaTable(), Wa_16012061344) ||
             VISA_WA_CHECK(builder.getPWaTable(), Wa_16012292205)) &&
            hasIndirectSource(node);

        hasFollowDistOneAReg = getGRFFootPrint(node, p);

        //For architecture registers ce#, sp, sr0.#, cr0.#, ip, tm0, dbg0, set distance 1
        if (hasFollowDistOneAReg)
        {
            node->setDistance(1);
            node->setDistOneAReg();
            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                node->instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            }
        }

        //Support for the mad block in DPAS pipeline
        if (builder.has2xDP() &&
            builder.getOption(vISA_ScheduleFor2xSP) &&
            is2xDPBlockCandidate(curInst, true))
        {
            unsigned depDistance = curInst->getDst()->getLinearizedEnd() - curInst->getDst()->getLinearizedStart() + 1;
            std::list<G4_INST*>::iterator iNextInst = iInst;
            iNextInst++;
            G4_INST* nInst = *iNextInst;
            while (is2xDPBlockCandidate(nInst, false))
            {
                SBNode nextNode(nodeID, ALUID, bb->getId(), nInst);
                getGRFFootPrint(&nextNode, p);

                if (hasInternalDependence(node, &nextNode))
                {
                    break;
                }
                depDistance += nInst->getDst()->getLinearizedEnd() - nInst->getDst()->getLinearizedStart() + 1;
                iNextInst ++;
                nInst = *iNextInst;
                if (iInstNext == iInstEnd)
                {
                    break;
                }
                if (depDistance >= builder.numEltPerGRF<Type_UB>() * 8)
                {
                    break;
                }
            }

            if (depDistance >= builder.numEltPerGRF<Type_UB>() * 8)
            {
                curInst->setNoACCSBSet();
            }
        }

        // Support for atomic write combine
        // Treat block instructions as one in distance calculation.
        // The write combine in the local scheduling guarantee that all instructions in the block belong to same instruction pipeline.
        auto isWriteCombineBlockCandidate = [&](G4_INST * inst)
        {
            return (inst->opcode() == G4_mov &&
                IS_BTYPE(inst->getDst()->getType()) &&
                (IS_BTYPE(inst->getSrc(0)->getType()) || IS_WTYPE(inst->getSrc(0)->getType()) || IS_DTYPE(inst->getSrc(0)->getType()) || inst->getSrc(0)->getType() == Type_F) &&
                inst->getPredicate() == nullptr);
        };

        if (builder.getOption(vISA_writeCombine) && isWriteCombineBlockCandidate(curInst) && curInst->isAtomicInst())
        {
            while (nextInst && isWriteCombineBlockCandidate(nextInst))
            {
                SBNode nextNode = SBNode(nodeID, ALUID, bb->getId(), nextInst);
                getGRFFootPrint(&nextNode, p);
                footprintMerge(node, &nextNode);
                node->addInstruction(nextInst);

                curInst = nextInst;
                iInst = iInstNext;
                iInstNext++;
                nextInst = *iInstNext;

                if (!curInst->isAtomicInst())
                {
                    break;
                }
            }

            // check last instruction in the block is correct or not
            assert(curInst && isWriteCombineBlockCandidate(curInst) && !curInst->isAtomicInst() && "the last instruction in the write combine block is wrong");
        }

        //Support for DPAS
        //To fully provide the efficiency of DPAS pipeline
        //We'd like to promote the dependence to or before the first instruction of a DPAS block
        //At the same time, push all dependence BD to the last instruction.
        //Keeping the dependence within a DPAS block will drop performance a lot.
        if (curInst->isDpas())
        {
            unsigned dpas_count = 0;
            if (nextInst && nextInst->isDpas())
            {
                SBNode nextNode;
                bool sameSrcDst = false;
                while (curInst != nullptr && curInst->isDpas())
                {
                    //following instructions, first instruction is in node already
                    if (dpas_count != 0)
                    {
                        if (nextNode.getNodeID() != -1)
                        {
                            footprintMerge(node, &nextNode);
                        }
                        node->addInstruction(curInst);
                        const G4_InstDpas* dpasInst = curInst->asDpasInst();
                        node->addDPASSize(dpasInst->getRepeatCount());
                    }
                    else  //If the first node has internal dependence, break immediately
                    {
                        if (hasInternalDependenceWithinDPAS(node))
                        {
                            break;
                        }
                    }

                    nextNode = SBNode(nodeID, ALUID, bb->getId(), nextInst);
                    getGRFFootPrint(&nextNode, p);

                    //Has dependence cannot be merged into same node.
                    //Different Depth, src1 and type cannot be merged
                    //Same register reuse in dest and src cannot be a part of a macro, even the last one.
                    if (sameSrcDst ||
                        isLastDpas(node, &nextNode) ||
                        hasDependenceBetweenDPASNodes(node, &nextNode))
                    {
                        break;
                    }

                    if (hasInternalDependenceWithinDPAS(&nextNode))
                    {
                        sameSrcDst = true;
                    }

                    curInst->setOptionOn(InstOpt_Atomic);
                    dpas_count++;

                    curInst = nextInst;
                    iInst = iInstNext;
                    iInstNext++;
                    if (iInstNext == iInstEnd)
                    {
                        if (nextNode.getNodeID() != -1)
                        {
                            footprintMerge(node, &nextNode);
                        }
                        node->addInstruction(curInst);
                        nextInst = nullptr;
                        break;
                    }
                    nextInst = *iInstNext;
                }
                curInst = node->GetInstruction();
            }
        }
        if (node->getLastInstruction()->isDpas())
        {
            node->setDPASID(DPASID);
            DPASID += node->getDPASSize();
        }

        //Get buckets for all GRF registers which are used in curInst
        std::vector<SBBucketDesc> BDvec;
        std::vector<SBBucketDesc> liveBDvec;
        BDvec.clear();
        liveBDvec.clear();

        getGRFBucketDescs(node, BDvec, false);
        if (node->instVec.size() > 1)
        {
            getGRFBucketDescs(node, liveBDvec, false);
        }

        if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
        {
            node->ALUPipe = curInst->getInstructionPipeXe();
        }

        // For ALU instructions without GRF usage
        if (distanceHonourInstruction(curInst))
        {
            ALUID++;

            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                switch (node->ALUPipe)
                {
                case PIPE_INT:
                    node->setIntegerID(integerID);
                    pushItemToQueue(latestInstID[PIPE_INT], node->getNodeID());
                    integerID++;
                    break;
                case PIPE_FLOAT:
                    node->setFloatID(floatID);
                    pushItemToQueue(latestInstID[PIPE_FLOAT], node->getNodeID());
                    floatID++;
                    break;
                case PIPE_LONG:
                    node->setLongID(longID);
                    pushItemToQueue(latestInstID[PIPE_LONG], node->getNodeID());
                    longID++;
                    break;
                case PIPE_MATH:
                    node->setMathID(mathID);
                    pushItemToQueue(latestInstID[PIPE_MATH], node->getNodeID());
                    mathID++;
                    break;
                default:
                    ASSERT_USER(curInst->hasNoPipe(), "Unexpected instruction found in distance ");
                }
            }

            if (!BDvec.size())
            {
                if (ALUID >= SWSB_MAX_ALU_DEPENDENCE_DISTANCE && ALUID != node->getALUID())
                {
                    if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
                    {
                        clearKilledBucketNodeXeHP(LB, integerID, floatID, longID, mathID);
                    }
                    else
                    {
                        clearKilledBucketNodeXeLP(LB, ALUID);
                    }
                }
                continue;
            }
        }

        // Considering instruction level liveness kill, i.e killing the live instructions/operands,
        // the dependence checking order must be RAR/RAW --> WAR/WAW, the bucket descriptions in BDvec must in the order of src->dst.
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
        // For WAW and RAW, once explicit dependencies are required, kill the liveness of instruction.
        // For WAR, once explicit dependencies is required, kill the source operands.
        // Others, only operand kill.
        bool instKill = false;

        // For all bucket descriptors of curInst
        for (const SBBucketDesc& BD : BDvec) {
            const int& curBucket = BD.bucket;
            const Gen4_Operand_Number& curOpnd = BD.opndNum;
            const SBFootprint* curFootprint = BD.footprint;

            // Check liveness for each live curBucket node.
            // Add explicit dependence if liveness is killed and there is no implicit dependence
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
                const SBFootprint* liveFootprint = liveBN->footprint;
                G4_INST* liveInst = liveFootprint->inst;

                bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);
                bool hasRMWOverlap = false;
                if (builder.hasFourALUPipes() && distanceHonourInstruction(liveInst) &&
                    distanceHonourInstruction(curInst))
                {
                    hasOverlap = curFootprint->hasOverlap(liveFootprint, hasRMWOverlap, internalOffset);
                }

                //RAW:                     R kill W    R-->live       explicit dependence
                //WAW: same pipeline and inorder   W2 kill W1  W2-->live      implicit dependence
                //WAW: different pipelines or OOO  W2 kill W1  W2-->live      explict dependence
                //WAR: different pipelines W kill R    W-->live       explicit dependence
                //WAR: same pipeline       W kill R    W-->live       implicit dependence
                //RAR: same pipeline               R2 kill R1  R2-->live      no dependence
                //RAR: different pipelines         no kill     R1,R2-->live   no dependence
                //Find DEP type
                DepType dep = getDepForOpnd(liveOpnd, curOpnd);

                //W/A for the read suppression caused issue
                //1)(~f0.0.anyv) math.cos(2 | M0)      r23.7<2>:hf   r11.7<4; 2, 2> : hf{ $14 }
                //2)             mul(8 | M0)               acc0.0<1>:ud  r35.3<8; 8, 0> : ud   r23.0<8; 4, 0> : uw   //With execution mask, only r23.0~r23.3 are read
                //3)             mach(8 | M0)              r52.0<1>:ud   r35.3<8; 8, 0> : ud   r23.0<4; 4, 0> : ud{ $14.dst }
                //FIXME, For performance, we need check the 3rd instruction as well

                //W/A for src1 read suppression of all ALUG instructions on PVC:
                //  Whenever there is GRF crossover for src1 (for src1, the read data is distributed over 2  GRFs), we need always set a
                //  Read after Write (RAW)  dependency to any element in the 2 GRFs we are reading for src1  in the current instruction.
                //      (W)     add (16|M0)     r7.14<1>:f    r61.14<1;1,0>:f   r9.14<1;1,0>:f
                //      (W)     mad (16|M0)     r26.10<1>:f   r20.10<1;0>:f     r6.10<1;0>:f      r101.10<1>:f     {F@1}
                if (!hasOverlap && dep == RAW &&
                    ((!builder.hasFixedCycleMathPipe() &&
                        liveInst->isMath() && !curInst->isMath() &&
                        builder.hasRSForSpecificPlatform() &&
                        (!hasSamePredicator(liveInst, curInst) || builder.hasMathRSIsuue())) ||
                    (builder.hasSrc1ReadSuppressionIssue() &&
                        distanceHonourInstruction(curInst) &&
                        curOpnd == Opnd_src1 && curInst->getSrc(1) && curInst->getSrc(1)->asSrcRegRegion() &&
                        curInst->getSrc(1)->asSrcRegRegion()->crossGRF(builder)) ||
                        (builder.hasRMWReadSuppressionIssue() && (liveInst->isMathPipeInst()))))
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
                        if (builder.getOption(vISA_EnableDPASTokenReduction) &&
                            node->getLastInstruction()->isDpas() &&
                            liveNode->getLastInstruction()->isDpas())
                        {
                            if ((node->getDPASID() + curFootprint->offset - (liveNode->getDPASID() + internalOffset) < tokenAfterDPASCycle))
                            {
                                LB->killOperand(bn_it);
                                createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                                liveNode->setInstKilled(true);  //Instruction level kill
                                instKill = true;
                                continue;
                            }
                            else if (dep == WAW)  //For RAW, we cannot
                            {
                                LB->killOperand(bn_it);
                                continue;
                            }
                        }
                        else
                        {
                            LB->killOperand(bn_it);
                            createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                            liveNode->setInstKilled(true);  //Instruction level kill
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

                        //Different pipeline/functionID, added Edge
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

                            if (builder.getOption(vISA_EnableDPASTokenReduction) &&
                                node->getLastInstruction()->isDpas() &&
                                liveNode->getLastInstruction()->isDpas())
                            {
                                //
                                //  dpasw.8x7(8 | M0)         r84 : f         r84 : f             r52 : bf            r14.0 : bf{ Atomic }
                                //  dpasw.8x7(8 | M0)         r92 : f         r92 : f             r52 : bf            r22.0 : bf{ Atomic }
                                //  dpasw.8x7(8 | M0)         r100 : f        r100 : f            r52 : bf            r30.0 : bf{ Atomic }
                                //  dpasw.8x7(8 | M0)         r108 : f        r108 : f            r52 : bf            r38.0 : bf{ Atomic }
                                //  dpasw.8x7(8 | M0)         r116 : f        r116 : f            r52 : bf            r46.0 : bf{ $5 }
                                //  sync.nop                      null{ Compacted, $5.src }
                                //  (W)send.dc0(16 | M0)         r52      r6      null    0x0         0x28805FE  {$0}
                                //
                                //  Although there is WAR dependence because of r52. However, due to the read suppression, the sync.nop is not required.
                                //  The DPAS in-order GRF read cycles can cover the GRF read of r52 to r58.

                                if (liveOpnd == Opnd_src1)
                                {
                                    if (node->getDPASID() + curFootprint->offset - liveNode->getDPASID() <= TOKEN_AFTER_READ_DPAS_CYCLE)
                                    {
                                        createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                                    } //else do nothing, previous whole region check kill the bucket node already.
                                }
                                else  //src0, src2
                                {
                                    if (node->getDPASID() + curFootprint->offset - (liveNode->getDPASID() + internalOffset) <= TOKEN_AFTER_READ_DPAS_CYCLE)
                                    {
                                        createAddGRFEdge(liveNode, node, dep, DEP_EXPLICT);
                                    } //else do nothing
                                }
                            }
                            else
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
                        (curBucket < (totalGRFNum + (int)builder.getNumScalarRegisters())))
                    {//Only need track GRF RAW dependence
                        LB->killOperand(bn_it);
                        setDistance(curFootprint, node, liveNode, false);
                        liveNode->setInstKilled(true);  //Instrtuction level kill
                        instKill = true;
                        continue;
                    }

                    if (dep == WAW) {
                        bool killed = false;
                        //For implicit dependence, the previous node can be killed only when it's wholly overlapped by the following one
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            killed = true;
                        }

                        if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
                        {
                            if (!distanceHonourInstruction(curInst) ||
                                node->ALUPipe != liveNode->ALUPipe ||
                                (node->ALUPipe == liveNode->ALUPipe && hasRMWOverlap))
                            {
                                if (!killed)
                                {
                                    LB->killOperand(bn_it);
                                    killed = true;
                                }

                                setDistance(curFootprint, node, liveNode, true);
                                liveNode->setInstKilled(true); //Instruction level kill
                                instKill = true;
                            }
                        }
                        else if (!curInst->distanceHonourInstruction()
                                || (liveInst->isLongPipeInstructionXe() && !curInst->isLongPipeInstructionXe())
                                )
                            {
                                if (!killed)
                                {
                                    LB->killOperand(bn_it);
                                    killed = true;
                                }
                                setDistance(curFootprint, node, liveNode, true);
                                liveNode->setInstKilled(true); //Instruction level kill
                                instKill = true;
                            }

                        if (killed)
                        {
                            continue;
                        }
                    }

                    if (dep == WAR) {
                        bool killed = false;
                        //For implicit dependence, the previous node can be killed only when it's wholly overlapped by the following one
                        if (curFootprint->isWholeOverlap(liveFootprint))
                        {
                            LB->killOperand(bn_it);
                            killed = true;
                        }

                        if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
                        {
                            if (!curInst->distanceHonourInstruction() || node->ALUPipe != liveNode->ALUPipe)
                            {
                                if (!killed)
                                {
                                    LB->killOperand(bn_it);
                                    killed = true;
                                }
                                setDistance(curFootprint, node, liveNode, true);
                                liveNode->setInstKilled(true); //Instruction level kill
                            }
                        }
                        else if (!hasSameFunctionID(liveInst, curInst))
                        {
                            if (!killed)
                            {
                                LB->killOperand(bn_it);
                                killed = true;
                            }
                            setDistance(curFootprint, node, liveNode, true);
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
                            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
                            {
                                if (node->ALUPipe == liveNode->ALUPipe)
                                {
                                    LB->killOperand(bn_it);
                                    continue;
                                }
                            }
                            else
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

        if (node->distDep.size())
        {
            if (builder.hasFiveALUPipes())
            {
                node->finalizeDistanceType2(builder, latestInstID);
            }
            else if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                node->finalizeDistanceType1(builder, latestInstID);
            }
        }

        if ((builder.getOption(vISA_EnableSwitch) && node->GetInstruction()->isYieldInst()) ||
            (node->GetInstruction()->isCall() || node->GetInstruction()->isFCall()) ||
            (VISA_WA_CHECK(builder.getPWaTable(), Wa_14013672992) && node->GetInstruction()->isEOT()))
        {
            node->setDistance(1);
            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                node->instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            }
        }

        //Simplify the LB according to the distance, and if the node is killed
        if (instKill ||
            (ALUID >= SWSB_MAX_ALU_DEPENDENCE_DISTANCE && ALUID != node->getALUID()))
        {
            if (builder.hasThreeALUPipes() || builder.hasFourALUPipes())
            {
                clearKilledBucketNodeXeHP(LB, integerID, floatID, longID, mathID);
            }
            else
            {
                clearKilledBucketNodeXeLP(LB, ALUID);
            }
        }

        if (builder.hasSLMWARIssue() && curInst->isSend() &&
            (isSLMMsg(curInst) && (curInst->getDst() == nullptr || isFence(curInst))))
        {
            clearSLMWARWAissue(node, LB);
        }

        // Add buckets of current instruction to bucket list
        if (node->instVec.size() > 1)
        {
            std::map<const SBFootprint*, std::vector<SBBucketNode*>> bucketNodes;
            for (const SBBucketDesc& BD : liveBDvec)
            {
                auto iter = std::find_if(bucketNodes[BD.footprint].begin(), bucketNodes[BD.footprint].end(),
                    [&BD](SBBucketNode* node) {return BD.opndNum == node->opndNum; });
                if (iter != bucketNodes[BD.footprint].end())
                {
                    LB->add((*iter), BD.bucket);
                }
                else
                {
                    void* allocedMem = mem.alloc(sizeof(SBBucketNode));
                    SBBucketNode* newNode = new (allocedMem)SBBucketNode(node, BD.opndNum, BD.footprint);
                    bucketNodes[BD.footprint].push_back(newNode);
                    LB->add(newNode, BD.bucket);
                }
            }
        }
        else
        {
            std::vector<SBBucketNode*>  bucketNodes(Opnd_total_num, nullptr);  //The coarse grained footprint of operands
            for (const SBBucketDesc& BD : BDvec)
            {
                if (bucketNodes[BD.opndNum] == nullptr)
                {
                    void* allocedMem = mem.alloc(sizeof(SBBucketNode));
                    SBBucketNode* newNode = new (allocedMem)SBBucketNode(node, BD.opndNum, BD.footprint);
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
            // The dep delay of the node should be constant, so we can
            // calculate and save it for future uses.
            node->setDepDelay(swsb.calcDepDelayForNode(node));
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

                    //Record all send operands which live out current BB.
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
    indexes->integerIndex = integerID;
    indexes->floatIndex = floatID;
    indexes->longIndex = longID;
    indexes->DPASIndex = DPASID;
    indexes->mathIndex = mathID;
    last_DPASID = DPASID;

    for (int i = 0; i < PIPE_DPAS; i++)
    {
        indexes->latestDepALUID[i] = latestDepALUID[i];
    }

#ifdef DEBUG_VERBOSE_ON
    std::cerr << "\nLIVE OUT: \n";
    LB->dumpLives();
#endif

    return;
}

//#ifdef DEBUG_VERBOSE_ON

void G4_BB_SB::dumpLiveInfo(const SBBUCKET_VECTOR* globalSendOpndList, unsigned globalSendNum, const SBBitSets* send_kill) const
{
    std::cerr << "\nBB" << bb->getId() << ":" << first_node << "-" << last_node << ", succ<";
    for (const G4_BB* succ : bb->Succs)
    {
        std::cerr << succ->getId() << ",";
    }
    std::cerr << "> pred<";
    for (const G4_BB* pred : bb->Preds)
    {
        std::cerr << pred->getId() << ",";
    }

    std::cerr << "> JIPSucc <";
    for (const G4_BB_SB* succ : Succs)
    {
        std::cerr << succ->getBB()->getId() << ",";
    }
    std::cerr << "> JIPPred <";
    for (const G4_BB_SB* pred : Preds)
    {
        std::cerr << pred->getBB()->getId() << ",";
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
        const SBBucketNode* sNode = (*globalSendOpndList)[i];
        std::cerr << i << ": ";
        sNode->dump();
    }
    std::cerr << std::endl;

    std::cerr << "Live In:  ";
    std::cerr << std::endl;
    if (send_live_in.getSize() != 0)
    {
        std::cerr << "\tdst:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_live_in.isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;

        std::cerr << "\tsrc:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_live_in.isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "May Kill: ";
    std::cerr << std::endl;
    if (send_may_kill.getSize() != 0)
    {
        std::cerr << "\tdst:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_may_kill.isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_may_kill.isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "WAW May Kill: ";
    std::cerr << std::endl;
    if (send_WAW_may_kill.getSize() != 0)
    {
        std::cerr << "\tdst:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_WAW_may_kill.isSet(sNode->node->globalID))
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
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_kill->isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
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
    if (send_live_out.getSize() != 0)
    {
        std::cerr << "\tdst:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_kill_scalar.isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_kill_scalar.isSrcSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;

    std::cerr << "Live Out: ";
    std::cerr << std::endl;
    if (send_live_out.getSize() != 0)
    {
        std::cerr << "\tdst:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum == Opnd_dst &&
                send_live_out.isDstSet(sNode->node->globalID))
            {
                sNode->dump();
            }
        }
        std::cerr << std::endl;
        std::cerr << "\tsrc:  ";
        for (const SBBucketNode* sNode : *globalSendOpndList)
        {
            if (sNode->opndNum >= Opnd_src0 && sNode->opndNum <= Opnd_src3 &&
                send_live_out.isSrcSet(sNode->node->globalID))
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
                    BBVector[i]->dominators.isSet(k))
                {
                    std::cerr << "#BB" << k << ", ";
                }
            }
            std::cerr << std::endl;
        }

        std::cerr << "Live Out: ";
        std::cerr << std::endl;
        if (BBVector[i]->liveOutTokenNodes.getSize() != 0)
        {
            for (SBNODE_VECT_ITER node_it = SBSendNodes.begin();
                node_it != SBSendNodes.end();
                node_it++)
            {
                SBNode* node = (*node_it);
                if (BBVector[i]->liveOutTokenNodes.isSet(node->sendID))
                {
                    std::cerr << " #" << node->getNodeID() << ":" << node->sendID << ":" << node->GetInstruction()->getSetToken();
                }
            }
            std::cerr << std::endl;
        }

        std::cerr << "Killed Tokens: ";
        std::cerr << std::endl;
        if (BBVector[i]->killedTokens.getSize() != 0)
        {
            uint32_t totalTokenNum = kernel.getNumSWSBTokens();
            for (uint32_t k = 0; k < totalTokenNum; k++)
            {
                if (BBVector[i]->killedTokens.isSet(k))
                {
                    std::cerr << " #" << k << ", ";
                }
            }
        }
        std::cerr << std::endl;

    }

    return;
}

void G4_BB_SB::getLiveBucketsFromFootprint(const SBFootprint* firstFootprint, SBBucketNode* sBucketNode, LiveGRFBuckets* send_use_kills) const
{
    for (const SBFootprint* footprint = firstFootprint; footprint != nullptr; footprint = footprint->next)
    {
        //We only track the global dependence for GRF
        if (footprint->fType != GRF_T)
        {
            continue;
        }

        int startBucket = footprint->LeftB / builder.numEltPerGRF<Type_UB>();
        int endBucket = footprint->RightB / builder.numEltPerGRF<Type_UB>();
        for (int j = startBucket; j < endBucket + 1; j++)
        {
            send_use_kills->add(sBucketNode, j);
        }
    }

    return;
}

/*
* Note that the fall through dependencies are captured in the SBDDD linear scan already
*/
void SWSB::addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite)
{
    const bool enableDPASTokenReduction = fg.builder->getOption(vISA_EnableDPASTokenReduction);
    for (G4_BB_SB* sb_bb : BBVector)
    {
        if (sb_bb->first_node == -1)
        {
            continue;
        }

        //Get global send operands killed by current BB
        SBBitSets send_kill(globalSendNum);
        send_kill |= sb_bb->send_live_in;
        send_kill &= sb_bb->send_may_kill;

#ifdef DEBUG_VERBOSE_ON
        sb_bb->dumpLiveInfo(globalSendOpndList, globalSendNum, &send_kill);
#endif
        //Change the global send operands into live bucket for liveness scan
        //Instruction level liveness kill:
        //   For token dependence, there is only implicit RAR and WAR dependencies.
        //   the order of the operands are scanned is not an issue anymore.
        //   i.e explicit RAW and WAW can cover all other dependences.
        LiveGRFBuckets send_use_kills(mem, kernel.getNumRegTotal());
        for (SBBucketNode* sBucketNode : *globalSendOpndList)
        {
            SBNode* sNode = sBucketNode->node;
            if (send_kill.isSrcSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_src0 ||
                sBucketNode->opndNum == Opnd_src1 ||
                sBucketNode->opndNum == Opnd_src2 ||
                sBucketNode->opndNum == Opnd_src3))
            {
                sb_bb->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
            }
            if (send_kill.isDstSet(sNode->globalID) && (sBucketNode->opndNum == Opnd_dst))
            {
                sb_bb->getLiveBucketsFromFootprint(sNode->getFirstFootprint(sBucketNode->opndNum), sBucketNode, &send_use_kills);
            }
            sNode->setInstKilled(false);
            sNode->setSourceKilled(false);
        }

        //Scan BB again to figure out the dependence caused by global send operands
        std::vector<SBBucketDesc> BDvec;
        for (int j = sb_bb->first_node; j <= sb_bb->last_node; j++)
        {
            SBNode* node = (*SBNodes)[j];
            G4_INST* curInst = node->getLastInstruction();

            BDvec.clear();
            sb_bb->getGRFBucketDescs(node, BDvec, true);
            if (!BDvec.size())
            {
                continue;
            }

            bool instKill = false;
            // For all bucket descriptors of curInst
            for (const SBBucketDesc& BD : BDvec)
            {
                const int& curBucket = BD.bucket;
                const Gen4_Operand_Number& curOpnd = BD.opndNum;
                const SBFootprint* curFootprint = BD.footprint;

                for (LiveGRFBuckets::BN_iterator bn_it = send_use_kills.begin(curBucket);
                    bn_it != send_use_kills.end(curBucket);)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                    const SBFootprint* liveFootprint = liveBN->footprint;
                    G4_INST* liveInst = liveFootprint->inst;
                    unsigned short internalOffset = 0;
                    bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);

                    //Find DEP type
                    DepType dep = getDepForOpnd(liveOpnd, curOpnd);

                    //RAW:                     R kill W    R-->live       explicit dependence
                    //WAW:                     W2 kill W1  W2-->live      explicit dependence
                    //WAW: same pipeline/inorder W2 kill W1  W2-->live      implicit dependence
                    //WAR: different pipelines W kill R    W-->live       explicit dependence
                    //WAR: same pipeline       W kill R    W-->live       implicit dependence
                    //RAR: sample pipeline     R2 kill R1  R2-->live      implicit dependence
                    //RAR: different pipelines   no kill     R1,R2-->live   no dependence
                    if (hasOverlap)
                    {
                        assert(tokenHonourInstruction(liveInst));
                        if (dep == RAW || dep == WAW)
                        {
                            if (sb_bb->isGRFEdgeAdded(curLiveNode, node, dep, DEP_EXPLICT))
                            {
                                send_use_kills.killOperand(bn_it);
                                curLiveNode->setInstKilled(true);  //Instruction level kill
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
                                if (enableDPASTokenReduction &&
                                    node->getLastInstruction()->isDpas() &&
                                    curLiveNode->getLastInstruction()->isDpas())
                                {
                                    if (node->getDPASID() > curLiveNode->getDPASID())
                                    {
                                        if ((node->getDPASID() + curFootprint->offset - (curLiveNode->getDPASID() + internalOffset) < tokenAfterDPASCycle))
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                            curLiveNode->setInstKilled(true);  //Instruction level kill
                                            instKill = true;
                                            continue;
                                        }
                                        else if (dep == WAW)
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            continue;
                                        }
                                    }

                                    if (node->getDPASID() <= curLiveNode->getDPASID())
                                    {
                                        unsigned loopStartBB = BBVector[node->getBBID()]->getLoopStartBBID();
                                        unsigned loopEndBB = BBVector[curLiveNode->getBBID()]->getLoopEndBBID();
                                        if (loopStartBB != -1 && loopEndBB != -1)
                                        {
                                            unsigned frontDist = node->getDPASID() - BBVector[loopStartBB]->first_DPASID;
                                            unsigned endDist = BBVector[loopEndBB]->last_DPASID - curLiveNode->getDPASID();

                                            //Note that if node and live node are in different but nest loop, the calculation will be conservative
                                            if ((int)(frontDist + endDist + curFootprint->offset - internalOffset) < tokenAfterDPASCycle)
                                            {
                                                send_use_kills.killOperand(bn_it);
                                                sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                                curLiveNode->setInstKilled(true);  //Instruction level kill
                                                instKill = true;
                                                continue;
                                            }
                                            else if (dep == WAW)
                                            {
                                                send_use_kills.killOperand(bn_it);
                                                continue;
                                            }
                                        }
                                        else
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                            curLiveNode->setInstKilled(true);
                                            instKill = true;
                                            continue;
                                        }
                                    }
                                }
                                else
                                {
                                    send_use_kills.killOperand(bn_it);
                                    sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                    curLiveNode->setInstKilled(true);  //Instruction level kill
                                    instKill = true;
                                    continue;
                                }
                            }
                        }

                        if (dep == WAR)
                        {
                            bool killed = false;
                            //For implicit dependence, the previous node can be killed only when it's wholly overlapped by the following one
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
                                    sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                }
                            }
                            else
                            {
                                if (!afterWrite) //After read dependence is more comprehensive in SIMDCF, so add edge only in SIMDCF pass
                                {
                                    sb_bb->createAddGRFEdge(curLiveNode, node, dep, DEP_IMPLICIT);
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
                if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
                {
                    sb_bb->clearKilledBucketNodeXeHP(&send_use_kills, 0, 0, 0, 0);
                }
                else
                {
                    sb_bb->clearKilledBucketNodeXeLP(&send_use_kills, 0);
                }
            }
            if (fg.builder->hasSLMWARIssue() && curInst->isSend() &&
                (isSLMMsg(curInst) && (curInst->getDst() == nullptr || isFence(curInst))))
            {
                sb_bb->clearSLMWARWAissue(node, &send_use_kills);
            }
        }
    }

    return;
}

void SWSB::addReachingDefineSet(SBNode* node, SBBitSets* globalLiveSet, SBBitSets* localLiveSet)
{
    if (node->reachingSends.getSize() == 0)
    {
        node->reachingSends = SBBitSets(SBSendNodes.size());
    }

    node->reachingSends |= *globalLiveSet;

    node->reachingSends |= *localLiveSet;

    return;
}

void SWSB::addReachingUseSet(SBNode* node, SBNode* use)
{
    if (use->getSendUseID() != -1)
    {
        if (node->reachedUses.getSize() == 0)
        {
            node->reachedUses = SBBitSets(SBSendUses.size());
        }

        node->reachedUses.setDst(use->getSendUseID(), true);
    }

    return;
}

void SWSB::addGlobalDependenceWithReachingDef(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite)
{
    for (size_t i = 0; i < BBVector.size(); i++)
    {
        //Get global send operands killed by current BB
        SBBitSets send_kill(globalSendNum);
        //send_live record the live ones from out side of BB, but kill by BB
        SBBitSets send_live(SBSendNodes.size());

        SBBitSets send_live_through(globalSendNum);
        //send_reach_all record all the global livs live through the BB
        SBBitSets send_reach_all(SBSendNodes.size());

        send_kill |= BBVector[i]->send_live_in;
        send_kill &= BBVector[i]->send_may_kill;
        send_live_through |= BBVector[i]->send_live_in;
        send_live_through -= send_kill;

#ifdef DEBUG_VERBOSE_ON
        BBVector[i]->dumpLiveInfo(globalSendOpndList, globalSendNum, &send_kill);
#endif
        //Change the global send operands into live bucket for liveness scan
        //Instruction level liveness kill:
        //   For token dependence, there is only implicit RAR and WAR dependencies.
        //   the order of the operands are scanned is not an issue anymore.
        //   i.e explicit RAW and WAW can cover all other dependences.
        LiveGRFBuckets send_use_kills(mem, kernel.getNumRegTotal());
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

        BBVector[i]->localReachingSends = SBBitSets(SBSendNodes.size());

        if (BBVector[i]->first_send_node != -1)
        {
            for (int j = BBVector[i]->first_send_node; j <= BBVector[i]->last_send_node; j++)
            {
                SBNode* node = SBSendNodes[j];

                //Get the live range for the local ones
                if (node->globalID == -1)
                {
                    assert(node->getBBID() == i);

                    node->setLiveEarliestID(node->getNodeID());
                    node->setLiveLatestID(node->getNodeID());
                    if (node->succs.size())
                    {
                        for (int k = 0; k < (int)(node->succs.size()); k++)
                        {
                            SBDEP_ITEM& curSucc = node->succs[k];
                            SBNode* succ = curSucc.node;

                            node->setLiveLatestID(succ->getNodeID(), succ->getBBID());
                        }
                    }
                    else
                    {
                        node->setLiveLatestID(BBVector[i]->last_node);
                    }
                }
                else
                {
                    node->setLiveEarliestID(node->getNodeID());
                    node->setLiveLatestID(BBVector[i]->last_node);
                }
            }
        }
        localTokenUsage.clear(); //Add to the live node

        //Scan BB again to figure out the dependence caused by global send operands
        std::vector<SBBucketDesc> BDvec;
        for (int j = BBVector[i]->first_node; j <= BBVector[i]->last_node; j++)
        {
            SBNode* node = (*SBNodes)[j];
            G4_INST* curInst = (*SBNodes)[j]->getLastInstruction();

            BDvec.clear();
            BBVector[i]->getGRFBucketDescs(node, BDvec, true);
            if (!BDvec.size())
            {
                continue;
            }

            //Tack all the token nodes defined in current BB
            if (tokenHonourInstruction(node->GetInstruction()))
            {
                addReachingDefineSet(node, &send_live, &BBVector[i]->localReachingSends);
                node->reachingSends |= send_reach_all;

                expireLocalIntervals(node->getNodeID(), i);
                if (node->GetInstruction()->getDst() != nullptr &&
                    !node->GetInstruction()->getDst()->isNullReg())
                {
                    BBVector[i]->localReachingSends.setDst(node->sendID, true);
                }
                else
                {
                    BBVector[i]->localReachingSends.setSrc(node->sendID, true);
                }
                localTokenUsage.push_back(node); //Add to the live node
            }

            bool instKill = false;
            // For all bucket descriptors of curInst
            for (const SBBucketDesc& BD : BDvec)
            {
                const int& curBucket = BD.bucket;
                const Gen4_Operand_Number& curOpnd = BD.opndNum;
                const SBFootprint* curFootprint = BD.footprint;

                for (LiveGRFBuckets::BN_iterator bn_it = send_use_kills.begin(curBucket);
                    bn_it != send_use_kills.end(curBucket);)
                {
                    SBBucketNode* liveBN = (*bn_it);
                    SBNode* curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                    const SBFootprint* liveFootprint = liveBN->footprint;
                    G4_INST* liveInst = liveFootprint->inst;
                    unsigned short internalOffset = 0;
                    bool hasOverlap = curFootprint->hasOverlap(liveFootprint, internalOffset);

                    //Find DEP type
                    DepType dep = getDepForOpnd(liveOpnd, curOpnd);

                    //RAW:                     R kill W    R-->live       explicit dependence
                    //WAW:                     W2 kill W1  W2-->live      explicit dependence
                    //WAW: same pipeline/inorder W2 kill W1  W2-->live      implicit dependence
                    //WAR: different pipelines W kill R    W-->live       explicit dependence
                    //WAR: same pipeline       W kill R    W-->live       implicit dependence
                    //RAR: sample pipeline     R2 kill R1  R2-->live      implicit dependence
                    //RAR: different pipelines   no kill     R1,R2-->live   no dependence
                    if (hasOverlap)
                    {
                        assert(tokenHonourInstruction(liveInst));
                        if (dep == RAW || dep == WAW)
                        {
                            if (BBVector[i]->isGRFEdgeAdded(curLiveNode, node, dep, DEP_EXPLICT))
                            {
                                send_use_kills.killOperand(bn_it);
                                curLiveNode->setInstKilled(true);  //Instruction level kill
                                instKill = true;
                                addReachingDefineSet(node, &send_live, &BBVector[i]->localReachingSends);
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
                                if (fg.builder->getOption(vISA_EnableDPASTokenReduction) &&
                                    node->getLastInstruction()->isDpas() &&
                                    curLiveNode->getLastInstruction()->isDpas())
                                {
                                    if (node->getDPASID() > curLiveNode->getDPASID())
                                    {
                                        if ((node->getDPASID() + curFootprint->offset - (curLiveNode->getDPASID() + internalOffset) < tokenAfterDPASCycle))
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                            curLiveNode->setInstKilled(true);  //Instruction level kill
                                            instKill = true;
                                            continue;
                                        }
                                        else if (dep == WAW)
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            continue;
                                        }
                                    }

                                    if (node->getDPASID() <= curLiveNode->getDPASID())
                                    {
                                        unsigned loopStartBB = BBVector[node->getBBID()]->getLoopStartBBID();
                                        unsigned loopEndBB = BBVector[curLiveNode->getBBID()]->getLoopEndBBID();

                                        if (loopStartBB != -1 && loopEndBB != -1)
                                        {
                                            unsigned frontDist = node->getDPASID() - BBVector[loopStartBB]->first_DPASID;
                                            unsigned endDist = BBVector[loopEndBB]->last_DPASID - curLiveNode->getDPASID();

                                            if ((int)(frontDist + endDist + curFootprint->offset - internalOffset) < tokenAfterDPASCycle)
                                            {
                                                send_use_kills.killOperand(bn_it);
                                                BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                                curLiveNode->setInstKilled(true);  //Instruction level kill
                                                instKill = true;
                                                continue;
                                            }
                                            else if (dep == WAW)
                                            {
                                                send_use_kills.killOperand(bn_it);
                                                continue;
                                            }
                                        }
                                        else
                                        {
                                            send_use_kills.killOperand(bn_it);
                                            BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                            curLiveNode->setInstKilled(true);  //Instruction level kill
                                            instKill = true;
                                            continue;
                                        }
                                    }
                                }
                                else
                                {
                                    send_use_kills.killOperand(bn_it);
                                    BBVector[i]->createAddGRFEdge(curLiveNode, node, dep, DEP_EXPLICT);
                                    curLiveNode->setInstKilled(true);  //Instruction level kill
                                    instKill = true;

                                    //Kill from live
                                    addReachingDefineSet(node, &send_live, &BBVector[i]->localReachingSends);
                                    send_live.setDst(curLiveNode->getSendID(), false);
                                    continue;
                                }
                            }
                        }

                        if (dep == WAR)
                        {
                            bool killed = false;
                            //For implicit dependence, the previous node can be killed only when it's wholly overlapped by the following one
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
                                addReachingDefineSet(node, &send_live, &BBVector[i]->localReachingSends);
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
                addReachingDefineSet(node, &send_live, &BBVector[i]->localReachingSends);
                node->reachingSends |= send_reach_all;
                node->setSendUseID(SBSendUses.size());
                SBSendUses.push_back(node);
            }

            if (instKill)
            {
                if (fg.builder->hasThreeALUPipes() || fg.builder->hasFourALUPipes())
                {
                    BBVector[i]->clearKilledBucketNodeXeHP(&send_use_kills, 0, 0, 0, 0);
                }
                else
                {
                    BBVector[i]->clearKilledBucketNodeXeLP(&send_use_kills, 0);
                }
            }
            if (fg.builder->hasSLMWARIssue() && curInst->isSend() &&
                (isSLMMsg(curInst) && (curInst->getDst() == nullptr || isFence(curInst))))
            {
                BBVector[i]->clearSLMWARWAissue(node, &send_use_kills);
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
bool G4_BB_SB::isGRFEdgeAdded(const SBNode* pred, const SBNode* succ, DepType d, SBDependenceAttr a)
{
    // When there are multiple dependence edges between two instructions
    // We think the RAW and WAW > WAR, which means if WAR co-exists with any other, it will be dropped.
    // This is especially important for send instructions. when there are multiple dependencies from same send instruction.
    // For the case like following, only the dst
    //1. Send r2-r5, r8, ....    $1
    //   ...
    //7. Add  r8,  r2, r10   test $1D
    // For WAW and RAW, we think they are equal.
    for (const SBDEP_ITEM& curSucc : pred->succs)
    {
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
    // When there are multiple dependence edges between two instructions
    // We think the RAW and WAW > WAR, which means if WAR co-exists with any other, it will be dropped.
    // This is especially important for send instructions. When there are multiple dependencies from same send instruction.
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


void G4_BB::emitRegInfo(std::ostream& output, G4_INST* inst, int offset)
{
    output << "#" << inst->getLexicalId() << "|" << offset << ":";
    G4_DstRegRegion* dstOpnd = inst->getDst();

    if (dstOpnd &&
        !dstOpnd->isIndirect() &&
        dstOpnd->isGreg())
    {
        uint32_t byteAddress = dstOpnd->getLinearizedStart();
        unsigned dstReg0 = byteAddress / parent->builder->numEltPerGRF<Type_UB>();
        output << " {";
        output << "D:" << dstReg0;
        output << "}";
    }

    for (int i = 0; i < inst->getNumSrc(); i++)
    {
        G4_Operand* srcOpnd = inst->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                !srcOpnd->asSrcRegRegion()->isIndirect() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    unsigned srcReg = byteAddress / parent->builder->numEltPerGRF<Type_UB>();
                    output << " {";
                    output << "S" << i;
                    output << ":" << srcReg;
                    output << "}";
                }
            }
        }
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
            if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
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

    if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
    {
        if (!inst->tokenHonourInstruction())
        {
            inst->setDistance(1);
            inst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
        }
        else
        {
            G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
            G4_INST* extraSyncInst = builder->createSync(G4_sync_nop, src0);
            extraSyncInst->setDistance(1);
            extraSyncInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            bb->insertBefore(inst_it, extraSyncInst);
        }
    }
    else
    {
        inst->setDistance(1);
    }

    if (inst->tokenHonourInstruction())
    {
        inst->setSetToken(0);
        if (inst->isEOT())
        {
            inst->setDistance(1);
            if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
            {
                inst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
            }
        }
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
        syncInst->setToken(0);
        syncInst->setTokenType(tokenType);
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
                    bb->insertBefore(new_it, newInst);
                    newInst->setLexicalId(instID);
                    instID++;
                    if (new_it == bb->end())
                    {
                        break;
                    }
                    inst_it++;
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
        G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
        G4_INST* extraSyncInst = builder->createSync(G4_sync_nop, src0);
        extraSyncInst->setDistance(1);
        if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
        {
            extraSyncInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
        }
        bb->insertBefore(inst_it, extraSyncInst);

        return;
    }

    if (inst->tokenHonourInstruction())
    {
        G4_SrcRegRegion* src0_1 = builder->createNullSrc(Type_UD);
        G4_INST* extraSyncInst = builder->createSync(G4_sync_nop, src0_1);
        extraSyncInst->setDistance(1);
        if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
        {
            extraSyncInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
        }
        bb->insertBefore(inst_it, extraSyncInst);

        if (!inst->isEOT())
        {
            G4_SrcRegRegion* src0 = builder->createNullSrc(Type_UD);
            G4_INST* syncInst = builder->createSync(G4_sync_nop, src0);

            unsigned short token = inst->getSetToken();
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
            syncInst->setToken(token);
            syncInst->setTokenType(tokenType);
            inst_it = bb->insertBefore(next_it, syncInst);
        }
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
    if (builder->hasThreeALUPipes() || builder->hasFourALUPipes())
    {
        syncAllRdInst->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
    }
    INST_LIST_ITER next_it = inst_it;
    next_it++;
    inst_it = bb->insertBefore(next_it, syncAllRdInst);

    G4_INST* syncAllWrInst = nullptr;
    src0 = builder->createNullSrc(Type_UD);
    syncAllWrInst = builder->createSync(G4_sync_allwr, src0);

    next_it = inst_it;
    next_it++;
    inst_it = bb->insertBefore(next_it, syncAllWrInst);
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

void SWSB::dumpImmDom(ImmDominator* dom) const
{
    for (auto bb : fg)
    {
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
        auto& idomBB = dom->getIDoms()[bb->getId()];
        assert(idomBB != nullptr);
        printf("\n\t iDOM: BB%d -- DOM SUCC: ", dom->getIDoms()[bb->getId()]->getId());
        for (const G4_BB_SB* succ : BBVector[bb->getId()]->domSuccs)
        {
            printf("BB%d, ", succ->getBB()->getId());
        }
        printf("\n");
    }
}
