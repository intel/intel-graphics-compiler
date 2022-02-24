/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _SWSB_H_
#define _SWSB_H_
#include <string>
#include <set>
#include <bitset>
#include "../Mem_Manager.h"
#include "../FlowGraph.h"
#include "../G4_IR.hpp"
#include "../Timer.h"
#include "../RegAlloc.h"
#include <vector>
#include "../BitSet.h"
#include "LocalScheduler_G4IR.h"
#include <utility>

namespace vISA
{
    class SBNode;
    struct SBBucketNode;
    class SBDDD;
    class G4_BB_SB;
    class SWSB;
}

//FIXME, build a table for different platforms
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE 11
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT 15
#define SWSB_MAX_MATH_DEPENDENCE_DISTANCE 18
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE 7u

#define TOKEN_AFTER_READ_DPAS_CYCLE 8
#define SWSB_MAX_DPAS_DEPENDENCE_DISTANCE 4
#define FOUR_THREAD_TOKEN_NUM  32

#define DEPENCENCE_ATTR(DO) \
    DO(DEP_EXPLICT) \
    DO(DEP_IMPLICIT)

enum SBDependenceAttr
{
    DEPENCENCE_ATTR(MAKE_ENUM)
};

#define DPAS_BLOCK_SIZE 8
#define DPAS_8x8_CYCLE 28
#define DPAS_8x4_CYCLE 24
#define DPAS_8x2_CYCLE 22
#define DPAS_8x1_CYCLE 21

#define DPAS_CYCLES_BEFORE_GRF_READ 8
#define DPAS_8x8_GRFREAD_CYCLE 8
#define DPAS_8x4_GRFREAD_CYCLE 4
#define DPAS_8x2_GRFREAD_CYCLE 2
#define DPAS_8x1_GRFREAD_CYCLE 1

typedef enum  _DPAS_DEP
{
    REP_1 = 1,
    REP_2 = 2,
    REP_4 = 4,
    REP_8 = 8
} DPAS_DEP;

#define UNKNOWN_TOKEN         -1
#define UNINIT_BUCKET         -1

namespace vISA {
    struct SBDep {
        SBDep(SBNode *SBNode, DepType Type, SBDependenceAttr Attr) {
            node = SBNode;
            type = Type;
            attr = Attr;
            exclusiveNodes.clear();
        }
        SBNode* node;
        DepType type;
        SBDependenceAttr attr;
        std::vector<vISA::SBNode*> exclusiveNodes;
    };

   struct SBDISTDep {
       SB_INST_PIPE liveNodePipe;
       SB_INST_PIPE nodePipe;
       SB_INST_PIPE operandType;
       bool dstDep;
   };

    using SWSBTokenType = G4_INST::SWSBTokenType;
    void singleInstStallSWSB(G4_Kernel *kernel, uint32_t instID, uint32_t endInstID, bool is_barrier);
    void forceDebugSWSB(G4_Kernel *kernel);
    void forceFCSWSB(G4_Kernel *kernel);
}

typedef vISA::SBDep SBDEP_ITEM;
typedef std::vector< SBDEP_ITEM> SBDEP_VECTOR;
typedef SBDEP_VECTOR::iterator SBDEP_VECTOR_ITER;

typedef vISA::SBDISTDep SBDISTDEP_ITEM;
typedef std::vector< SBDISTDEP_ITEM> SBDISTDEP_VECTOR;

namespace vISA
{
    typedef enum  _FOOTPRINT_TYPE
    {
        GRF_T = 1,
        ACC_T = 2,
        FLAG_T = 4,
    } FOOTPRINT_TYPE;

    struct SBFootprint {
        const FOOTPRINT_TYPE fType;
        const G4_Type type;
        const unsigned short LeftB;
        const unsigned short RightB;
        unsigned short offset = 0;
        G4_INST*       inst;
        struct SBFootprint *next = nullptr; //The ranges linked together to represent the possible register ranges may be occupied by an operand.
                                         //For indirect access, non-contiguous ranges exist.

        SBFootprint() : fType(GRF_T), type (Type_UNDEF), LeftB(0), RightB(0), inst(nullptr) { ; }
        SBFootprint(FOOTPRINT_TYPE ft, G4_Type t, unsigned short LB, unsigned short RB, G4_INST *i)
            : fType(ft), type(t), LeftB(LB), RightB(RB), inst(i) {
            ;
        }
        ~SBFootprint()
        {
        }

        void setOffset(unsigned short o) { offset = o; }

        bool hasOverlap(const SBFootprint *liveFootprint, unsigned short &internalOffset) const
        {
            for (const SBFootprint *curFootprintPtr = this; curFootprintPtr; curFootprintPtr = curFootprintPtr->next)
            {
                FOOTPRINT_TYPE curFType = curFootprintPtr->fType;
                for (const SBFootprint *curFootprint2Ptr = liveFootprint; curFootprint2Ptr; curFootprint2Ptr = curFootprint2Ptr->next)
                {
                    // Negative of no overlap: !(LeftB > curFootprint2Ptr->RightB || RightB < curFootprint2Ptr->LeftB)
                    if (curFType == curFootprint2Ptr->fType &&
                        curFootprintPtr->LeftB <= curFootprint2Ptr->RightB &&
                        curFootprintPtr->RightB >= curFootprint2Ptr->LeftB)
                    {
                        internalOffset = curFootprint2Ptr->offset;
                        return true;
                    }
                }
            }

            return false;
        }

        bool hasOverlap(const SBFootprint *liveFootprint, bool &isRMWOverlap, unsigned short &internalOffset) const
        {
            //Overlap with other ranges.
            for (const SBFootprint *curFootprintPtr = this; curFootprintPtr; curFootprintPtr = curFootprintPtr->next)
            {
                FOOTPRINT_TYPE curFType = curFootprintPtr->fType;
                G4_Type curType = curFootprintPtr->type;
                for (const SBFootprint *curFootprint2Ptr = liveFootprint; curFootprint2Ptr; curFootprint2Ptr = curFootprint2Ptr->next)
                {
                    // Negative of no overlap: !(LeftB > curFootprint2Ptr->RightB || RightB < curFootprint2Ptr->LeftB)
                    if (curFType == curFootprint2Ptr->fType)
                    {
                        if (curFootprintPtr->LeftB <= curFootprint2Ptr->RightB && curFootprintPtr->RightB >= curFootprint2Ptr->LeftB)
                        {
                            internalOffset = curFootprint2Ptr->offset;
                            if (curFType == GRF_T && IS_BTYPE(curType))
                            {
                                isRMWOverlap = true;
                            }
                            return true;
                        }
                        else if (curFType == GRF_T && IS_BTYPE(curType))
                        {
                            unsigned short w_LeftB = curFootprintPtr->LeftB / 2;
                            unsigned short w_RightB = curFootprintPtr->RightB / 2;
                            unsigned short w_curLeftB = curFootprint2Ptr->LeftB / 2;
                            unsigned short w_curRightB = curFootprint2Ptr->RightB / 2;
                            if (w_LeftB <= w_curRightB && w_RightB >= w_curLeftB)
                            {
                                isRMWOverlap = true;
                                return true;
                            }
                        }
                    }
                }
            }

            return false;
        }

        bool hasGRFGrainOverlap(const SBFootprint *liveFootprint) const
        {
            assert(inst != nullptr);
            const IR_Builder& irb = inst->getBuilder();
            for (const SBFootprint *curFootprintPtr = this; curFootprintPtr; curFootprintPtr = curFootprintPtr->next)
            {
                FOOTPRINT_TYPE curFType = curFootprintPtr->fType;
                assert(fType == curFType);
                for (const SBFootprint *curFootprint2Ptr = liveFootprint; curFootprint2Ptr; curFootprint2Ptr = curFootprint2Ptr->next)
                {
                    if (curFType == curFootprint2Ptr->fType &&
                        ((curFootprintPtr->LeftB  / irb.numEltPerGRF<Type_UB>()) <= (curFootprint2Ptr->RightB  / irb.numEltPerGRF<Type_UB>())) &&
                        ((curFootprintPtr->RightB  / irb.numEltPerGRF<Type_UB>()) >= (curFootprint2Ptr->LeftB  / irb.numEltPerGRF<Type_UB>())))
                    {
                        return true;
                    }
                }
            }

            return false;
        }


        //Check if current footprint overlaps footprint2
        //FIXME: it's conservative. Because for the indirect, the ranges may be contiguous?
        bool isWholeOverlap(const SBFootprint *liveFootprint) const
        {
            bool findOverlap = false;

            for (const SBFootprint *footprint2Ptr = liveFootprint; footprint2Ptr; footprint2Ptr = footprint2Ptr->next)
            {
                findOverlap = false;

                if (fType == footprint2Ptr->fType &&
                    LeftB <= footprint2Ptr->LeftB && RightB >= footprint2Ptr->RightB)
                {
                    findOverlap = true;
                    continue;
                }
                for (const SBFootprint *curFootprintPtr = next; curFootprintPtr; curFootprintPtr = curFootprintPtr->next)
                {
                    FOOTPRINT_TYPE curFType = curFootprintPtr->fType;
                    if (curFType == footprint2Ptr->fType &&
                            curFootprintPtr->LeftB <= footprint2Ptr->LeftB && curFootprintPtr->RightB >= footprint2Ptr->RightB)
                    {
                        findOverlap = true;
                        break;
                    }
                }

                if (!findOverlap)
                {
                    return false;
                }
            }

            return findOverlap;
        }

    };

    // Bit set which is used for global dependence analysis for SBID.
    // Since dependencies may come from dst and src and there may be dependence kill between dst and src dependencies,
    // we use internal bit set to track the live of dst and src separately.
    // Each bit map to one global SBID node according to the node's global ID.
    struct SBBitSets {
        BitSet dst;
        BitSet src;

        SBBitSets()
        {
        }

        SBBitSets(unsigned size) : dst(size, false), src(size, false)
        {
        }

        ~SBBitSets()
        {
        }

        void setDst(int ID, bool value)
        {
            dst.set(ID, value);
        }
        void setSrc(int ID, bool value)
        {
            src.set(ID, value);
        }

        unsigned getSize() const
        {
            assert(dst.getSize() == src.getSize());
            return dst.getSize();
        }

        bool isEmpty() const
        {
            return dst.isEmpty() && src.isEmpty();
        }

        bool isDstEmpty() const
        {
            return dst.isEmpty();
        }

        bool isSrcEmpty() const
        {
            return src.isEmpty();
        }

        bool isDstSet(unsigned i) const
        {
            return dst.isSet(i);
        }

        bool isSrcSet(unsigned i) const
        {
            return src.isSet(i);
        }

        SBBitSets& operator= (const SBBitSets& other)
        {
            dst = other.dst;
            src = other.src;
            return *this;
        }

        SBBitSets& operator= (SBBitSets&& other) noexcept = default;

        SBBitSets& operator|= (const SBBitSets& other)
        {
            dst |= other.dst;
            src |= other.src;
            return *this;
        }

        SBBitSets& operator&= (const SBBitSets& other)
        {
            dst &= other.dst;
            src &= other.src;
            return *this;
        }

        SBBitSets& operator-= (const SBBitSets& other)
        {
            dst -= other.dst;
            src -= other.src;
            return *this;
        }

        bool operator!= (const SBBitSets& other) const
        {
            return (dst != other.dst) || (src != other.src);
        }

        void* operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }
    };

    class SBNode {
    private:
        std::vector<SBFootprint*>  footprints;  // The coarse grained footprint of operands
        unsigned      nodeID = -1;          // Unique ID of the node
        unsigned      BBID = 0;           // ID of basic block
        int      ALUID = 0;          // The ID for in-order instructions. The out-of-order instructions are not counted.
        unsigned      liveStartID = 0; // The start ID of live range
        unsigned      liveEndID = 0;   // The end ID of live range
        unsigned      liveStartBBID = -1; // The start BB ID of live range
        unsigned      liveEndBBID = -1; // The start BB ID of live range
        int      integerID = -1;
        int      floatID = -1;
        int      longID = -1;
        int      mathID = -1;
        int      DPASID = -1;
        unsigned short DPASSize = 0;
        bool          instKilled = false; // Used for global analysis, if the node generated dependencies have all been resolved.
        bool          sourceKilled = false; // If the dependencies caused by source operands have all been resolved.
        bool          hasAW = false;      // Used for global analysis, if has AW (RAW or WAW) dependencies from the node
        bool          hasAR = false;      // Used for global analysis, if has AR (WAR) dependencies from the node
        bool          hasFollowDistOneAReg = false;
        bool          followDistOneAReg = false;
        unsigned depDelay = 0;

        struct DepToken {
            unsigned short token;
            SWSBTokenType  type;
            SBNode*        depNode;
        };
        std::vector <DepToken> depTokens;

    public:
        std::vector<G4_INST *> instVec;
        SBDEP_VECTOR   succs;          // A list of node's successors in dependence graph
        SBDEP_VECTOR   preds;          // A list of node's predecessors in dependence graph
        int            globalID = -1;  // ID of global send instructions
        int            sendID = -1;
        int            sendUseID = -1;
        SBNode *       tokenReusedNode = nullptr;  // For global token reuse optimization, the node whose token is reused by current one.
        SB_INST_PIPE   ALUPipe;
        SBDISTDEP_VECTOR  distDep;
        SBBitSets reachingSends;
        SBBitSets reachedUses;
        unsigned reuseOverhead = 0;

        /* Constructor */
        SBNode()
        {

        }

        SBNode(uint32_t id, int ALUId, uint32_t BBId, G4_INST *i)
            : nodeID(id), ALUID(ALUId), BBID(BBId),
            footprints(Opnd_total_num, nullptr)
        {
            ALUPipe = PIPE_NONE;
            if (i->isDpas())
            {
                G4_InstDpas* dpasInst = i->asDpasInst();
                DPASSize = dpasInst->getRepeatCount(); //Set the size of the current dpas inst, if there is a macro, following will be added.
            }

            instVec.push_back(i);
        }

        ~SBNode()
        {
            for (SBFootprint *sm : footprints)
            {
                sm->~SBFootprint();
            }
        }

        void setSendID(int ID)
        {
            sendID = ID;
        }

        void setSendUseID(int ID)
        {
            sendUseID = ID;
        }

        int getSendID() const
        {
            return sendID;
        }

        int getSendUseID() const
        {
            return sendUseID;
        }

        /* Member functions */
        G4_INST*  GetInstruction() const { return instVec.front(); }
        void addInstruction(G4_INST *i) { instVec.push_back(i); }
        G4_INST*  getLastInstruction() const { return instVec.back(); }
        void setDepDelay(unsigned val) { depDelay = val; }
        unsigned getDepDelay() const { return depDelay; }

        int getALUID() const { return ALUID; }
        unsigned getNodeID() const { return nodeID; };
        unsigned getBBID() const { return BBID; };
        int getIntegerID() const { return integerID; }
        int getFloatID() const { return floatID; }
        int getLongID() const { return longID; }
        int getMathID() const {return mathID;}
        int getDPASID() const { return DPASID; }
        unsigned short getDPASSize() const { return DPASSize; }

        void setIntegerID(int id) { integerID = id; }
        void setFloatID(int id) { floatID = id; }
        void setLongID(int id) { longID = id; }
        void setMathID(int id) { mathID = id; }
        void setDPASID(int id) { DPASID = id; }
        void addDPASSize(unsigned short size) { DPASSize += size; }

        unsigned getLiveStartID() const { return liveStartID; }
        unsigned getLiveEndID() const { return liveEndID; }

        unsigned getLiveStartBBID() const { return liveStartBBID; }
        unsigned getLiveEndBBID() const { return liveEndBBID; }

        void setLiveEarliestID(unsigned id, unsigned startBBID)
        {
            if (!liveStartID)
            {
                liveStartID = id;
                liveStartBBID = startBBID;
            }
            else if (liveStartID > id)
            {
                liveStartID = id;
                liveStartBBID = startBBID;
            }
        }
        void setLiveLatestID(unsigned id, unsigned endBBID)
        {
            if (!liveEndID)
            {
                liveEndID = id;
                liveEndBBID = endBBID;
            }
            else if (liveEndID < id)
            {
                liveEndID = id;
                liveEndBBID = endBBID;
            }
        }

        void setLiveEarliestID(unsigned id)
        {
            liveStartID = id;
        }

        void setLiveLatestID(unsigned id)
        {
            liveEndID = id;
        }

        void setInstKilled(bool value)
        {
            instKilled = value;
            if (value)
            {
                hasAW = true;
            }
        }

        bool isInstKilled() const
        {
            return instKilled;
        }

        void setSourceKilled(bool value)
        {
            sourceKilled = value;
            if (value)
            {
                hasAR = true;
            }
        }

        bool isSourceKilled() const
        {
            return sourceKilled;
        }

        void setAW()
        {
            hasAW = true;
        }

        void setAR()
        {
            hasAR = true;
        }

        void setDistOneAReg()
        {
            hasFollowDistOneAReg = true;
        }
        void setFollowDistOneAReg()
        {
            followDistOneAReg = true;
        }

        bool hasAWDep() const { return hasAW; }
        bool hasARDep() const { return hasAR; }
        bool hasDistOneAreg() const { return hasFollowDistOneAReg; }
        bool followDistOneAreg() const { return followDistOneAReg; }

        void setFootprint(SBFootprint *footprint, Gen4_Operand_Number opndNum)
        {
            if (footprints[opndNum] == nullptr)
            {
                footprints[opndNum] = footprint;
            }
            else
            {
                footprint->next = footprints[opndNum];
                footprints[opndNum] = footprint;
            }
        }

        SBFootprint* getFirstFootprint(Gen4_Operand_Number opndNum) const
        {
            return footprints[opndNum];
        }

        void setDistance(unsigned distance)
        {
            distance = std::min(distance, SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE);
            unsigned curDistance = (unsigned)instVec.front()->getDistance();
            if (curDistance == 0 ||
                curDistance > distance)
            {
                instVec.front()->setDistance((unsigned char)distance);
            }
        }

        void setAccurateDistType(SB_INST_PIPE depPipe)
        {
            switch (depPipe)
            {
            case PIPE_INT:
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTINT);
                break;
            case PIPE_FLOAT:
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTFLOAT);
                break;
            case PIPE_LONG:
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTLONG);
                break;
            case PIPE_MATH:
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTMATH);
                break;
            case PIPE_SEND:
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                break;
            default:
                assert(0 && "Wrong ALU PIPE");
                break;
            }
        }

        //This is to reduce the sync instructions.
        //When A@1 is used to replace I@1, make sure it will not cause the stall for float or long pipelines
        bool isClosedALUType(std::vector<unsigned>** latestInstID, unsigned distance, SB_INST_PIPE type)
        {
            int instDist = SWSB_MAX_MATH_DEPENDENCE_DISTANCE;
            SB_INST_PIPE curType = PIPE_NONE;

            for (int i = 1; i < PIPE_DPAS; i++)
            {
                int dist = SWSB_MAX_MATH_DEPENDENCE_DISTANCE + 1;
                if (i == PIPE_INT || i == PIPE_FLOAT)
                {
                    if (latestInstID[i]->size() >= distance)
                    {
                        dist = (int)nodeID - (*latestInstID[i])[latestInstID[i]->size() - distance] - SWSB_MAX_ALU_DEPENDENCE_DISTANCE;
                    }
                }
                if (i == PIPE_MATH)
                {
                    if (latestInstID[i]->size() >= distance)
                    {
                        dist = (int)nodeID - (*latestInstID[i])[latestInstID[i]->size() - distance] - SWSB_MAX_MATH_DEPENDENCE_DISTANCE;
                    }
                }
                if (i == PIPE_LONG)
                {
                    if (latestInstID[i]->size() >= distance)
                    {
                        dist = (int)nodeID - (*latestInstID[i])[latestInstID[i]->size() - distance] - SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT;
                    }
                }

                if (dist < instDist)
                {
                    instDist = dist;
                    curType = (SB_INST_PIPE)i;
                }
            }

            return type == curType;
        }

        void finalizeDistanceType1(IR_Builder& builder, std::vector<unsigned>** latestInstID)
        {
            if (instVec.front()->getDistanceTypeXe() != G4_INST::DistanceType::DIST_NONE)
            {  //Forced to a type already
                return;
            }

            if (builder.loadThreadPayload() && nodeID <= 8 &&
                GetInstruction()->isSend() && distDep.size())
            {
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                return;
            }

            if (builder.hasA0WARHWissue() && builder.hasFourALUPipes())
            {
                G4_INST* inst = GetInstruction();

                if (inst->getDst() && inst->getDst()->isA0())
                {
                    instVec.front()->setDistance(1);
                    instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);

                    return;
                }
            }

            unsigned curDistance = (unsigned)instVec.front()->getDistance();
            if (distDep.size())
            {
                SB_INST_PIPE depPipe = PIPE_NONE;
                SB_INST_PIPE sameOpndTypeDepPipe = PIPE_NONE;
                bool sameOperandType = true;
                int diffPipes = 0;
                bool depLongPipe = false;

                //Check if there is type conversion
                for (int i = 0; i < (int)(distDep.size()); i++)
                {
                    SBDISTDEP_ITEM& it = distDep[i];

                    if (it.operandType != it.liveNodePipe || it.dstDep)
                    {
                        sameOperandType = false;
                    }
                }

                //If the dependent instructions belong to different pipelines
                for (int i = 0; i < (int)(distDep.size()); i++)
                {
                    SBDISTDEP_ITEM& it = distDep[i];

                    if (it.liveNodePipe != it.nodePipe)
                    {
                        diffPipes++;
                        depPipe = it.liveNodePipe;
                    }

                    depLongPipe = it.liveNodePipe == PIPE_LONG;
                }

                //In some platforms, A@ need be used for following case when the regDist cannot be used (due to HW issue), and inst depends on different pipes
                //mov(8 | M0)               r63.0 < 2 > :ud   r15.0 < 1; 1, 0 > : ud{ I@7 }
                //...
                //add(8 | M0)               r95.0 < 1 > : q    r87.0 < 1; 1, 0 > : q    r10.0 < 0; 1, 0 > : q{ I@7 }
                //...
                //add(8 | M0)               r55.0 < 2 > : d    r95.0 < 1; 1, 0 > : q    r63.0 < 2; 1, 0 > : ud{ A@4 }
                //In some platforms I@ accurate dependence can used for following case when the regDist cannot be used (due to HW issue), because inst depends on same pipe
                //The example code piece :
                //mov(16 | M0) r88.0 < 2 > : d r84.0 < 1; 1, 0 > : d{ F@2 } // $79&103
                //mov(16 | M16) r90.0 < 2 > : d r85.0 < 1; 1, 0 > : d // $80&104
                //mov(16 | M0) r88.1 < 2 > : d r86.0 < 1; 1, 0 > : d{ F@1 } // $81&105
                //mov(16 | M16) r90.1 < 2 > : d r87.0 < 1; 1, 0 > : d // $82&106
                //mov(16 | M0) r32.0 < 1 > : df r88.0 < 1; 1, 0 > : q{ @2/I@2 } // $83&107
                //Since:q and : d all go to integer pipeline in, @2 should work.However, due to HW bug, I@2 is good
                bool mulitpleSamePipe = false;
                if (distDep.size() > 1 && sameOperandType)
                {
                    sameOpndTypeDepPipe = distDep[0].liveNodePipe;
                    for (int i = 1; i < (int)(distDep.size()); i++)
                    {
                        SBDISTDEP_ITEM& it = distDep[i];

                        if (it.liveNodePipe != sameOpndTypeDepPipe)
                        {
                            mulitpleSamePipe = true;
                        }
                    }
                }

                if (builder.hasLongOperandTypeDepIssue() && depLongPipe)
                {
                    sameOperandType = false;
                }

                if (!builder.getOptions()->getOption(vISA_disableRegDistDep) &&
                    !builder.hasRegDistDepIssue())
                {
                    instVec.front()->setOperandTypeIndicated(sameOperandType);
                }

                //Multiple dependences
                if (distDep.size() > 1)
                {
                    if (diffPipes) //Dependence instructions belong to different ALU pipelines.
                    {
                        if (sameOperandType) //But the operand type is from same instruction pipeline.
                        {
                            assert(!GetInstruction()->isSend()); //Send operand has no type, sameOperandType is always false
                            if (mulitpleSamePipe)
                            {
                                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                            }
                            else
                            {
                                setAccurateDistType(depPipe);
                            }
                            instVec.front()->setOperandTypeIndicated(false); //DPAS distance will be in sync inst
                        }
                        else // For send, we will set it to DISTALL if there are multiple dependences. For non-send, DIST
                        {
                            instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                            instVec.front()->setOperandTypeIndicated(false);
                        }
                    }
                    else //From same pipeline
                    {
                        setAccurateDistType(ALUPipe);
                        instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, ALUPipe));
                    }
                }
                else //Only one dependency
                {
                    if (depPipe != PIPE_NONE) // From different pipeline
                    {
                        setAccurateDistType(depPipe);
                        if (sameOperandType || GetInstruction()->isSend())
                        {
                            instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, depPipe));
                        }
                    }
                    else // From same pipeline
                    {
                        setAccurateDistType(ALUPipe);
                        if (sameOperandType)
                        {
                            instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, ALUPipe));
                        }
                    }
                }
            }
        }

        void finalizeDistanceType2(IR_Builder& builder, std::vector<unsigned> **latestInstID)
        {
            if (instVec.front()->getDistanceTypeXe() != G4_INST::DistanceType::DIST_NONE)
            {  //Forced to a type already
                return;
            }

            if (builder.loadThreadPayload() && nodeID <= 8 &&
                GetInstruction()->isSend() && distDep.size())
            {
                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                return;
            }

            if (builder.hasA0WARHWissue() && builder.hasFourALUPipes())
            {
                G4_INST* inst = GetInstruction();

                if (inst->getDst() && inst->getDst()->isA0())
                {
                    instVec.front()->setDistance(1);
                    instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);

                    return;
                }
            }

            unsigned curDistance = (unsigned)instVec.front()->getDistance();
            if (distDep.size())
            {
                SB_INST_PIPE depPipe = PIPE_NONE;
                SB_INST_PIPE sameOpndTypeDepPipe = PIPE_NONE;
                bool sameOperandType = true;
                bool depSamePipe = true;
                bool depLongPipe = false;

                //Check the potential to use regDist
                for (int i = 0; i < (int)(distDep.size()); i++)
                {
                    SBDISTDEP_ITEM& it = distDep[i];

                    //For regDist, the operand type is same as pipeline type of dependent instruction
                    if (it.operandType != it.liveNodePipe || it.dstDep)
                    {
                        sameOperandType = false;
                    }
                }

                //If the dependent instructions belong to different pipelines
                for (int i = 0; i < (int)(distDep.size()); i++)
                {
                    SBDISTDEP_ITEM& it = distDep[i];

                    if (it.liveNodePipe != it.nodePipe)
                    {
                        depSamePipe = false; //Current instruction and dependence instruction belong to different pipeline
                    }

                    if (depPipe == PIPE_NONE)
                    {
                        depPipe = it.liveNodePipe;
                    }

                    depLongPipe = it.liveNodePipe == PIPE_LONG;
                }

                //In some platforms, A@ need be used for following case when the regDist cannot be used (due to HW issue), and inst depends on different pipes
                //mov(8 | M0)               r63.0 < 2 > :ud   r15.0 < 1; 1, 0 > : ud{ I@7 }
                //...
                //add(8 | M0)               r95.0 < 1 > : q    r87.0 < 1; 1, 0 > : q    r10.0 < 0; 1, 0 > : q{ I@7 }
                //...
                //add(8 | M0)               r55.0 < 2 > : d    r95.0 < 1; 1, 0 > : q    r63.0 < 2; 1, 0 > : ud{ A@4 }
                //In some platforms I@ accurate dependence can used for following case when the regDist cannot be used (due to HW issue), because inst depends on same pipe
                //The example code piece :
                //mov(16 | M0) r88.0 < 2 > : d r84.0 < 1; 1, 0 > : d{ F@2 } // $79&103
                //mov(16 | M16) r90.0 < 2 > : d r85.0 < 1; 1, 0 > : d // $80&104
                //mov(16 | M0) r88.1 < 2 > : d r86.0 < 1; 1, 0 > : d{ F@1 } // $81&105
                //mov(16 | M16) r90.1 < 2 > : d r87.0 < 1; 1, 0 > : d // $82&106
                //mov(16 | M0) r32.0 < 1 > : df r88.0 < 1; 1, 0 > : q{ @2/I@2 } // $83&107
                //Since:q and : d all go to integer pipeline in, @2 should work.However, due to HW bug, we can only set to I@2
                //Depends on multiple pipelines but same pipeline
                bool mulitpleSamePipe = true;
                if (distDep.size() > 1)
                {
                    sameOpndTypeDepPipe = distDep[0].liveNodePipe;
                    for (int i = 1; i < (int)(distDep.size()); i++)
                    {
                        SBDISTDEP_ITEM& it = distDep[i];

                        if (it.liveNodePipe != sameOpndTypeDepPipe)
                        {
                            mulitpleSamePipe = false;
                        }
                    }
                }

                if (builder.hasLongOperandTypeDepIssue() && depLongPipe)
                {
                    sameOperandType = false;
                }

                if (!builder.getOptions()->getOption(vISA_disableRegDistDep) &&
                    !builder.hasRegDistDepIssue())
                {
                    instVec.front()->setOperandTypeIndicated(sameOperandType);
                }

                //Multiple dependences
                if (distDep.size() > 1)
                {
                    if (!depSamePipe) //Dependent instruction and current instruction belong to different ALU pipelines.
                    {
                        if (sameOperandType) //But the operand type and dependence instruction are from same instruction pipeline.
                        {
                            assert(!GetInstruction()->isSend()); //Send operand has no type, sameOperandType is always false
                            if (mulitpleSamePipe)
                            {
                                setAccurateDistType(depPipe);
                            }
                            else
                            {
                                instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                            }
                            instVec.front()->setOperandTypeIndicated(false); //DPAS distance will be in sync inst
                        }
                        else if (mulitpleSamePipe)
                        {
                            setAccurateDistType(depPipe);
                            instVec.front()->setOperandTypeIndicated(false);
                        }
                        else // set to DISTALL
                        {
                            instVec.front()->setDistanceTypeXe(G4_INST::DistanceType::DISTALL);
                            instVec.front()->setOperandTypeIndicated(false);
                        }
                    }
                    else //From same pipeline
                    {
                        setAccurateDistType(depPipe);
                        instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, depPipe));
                    }
                }
                else //Only one dependency
                {
                    if (depPipe != PIPE_NONE) // From different pipeline
                    {
                        setAccurateDistType(depPipe);
                        if (sameOperandType || GetInstruction()->isSend())
                        {
                            instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, depPipe));
                        }
                    }
                    else // From same pipeline
                    {
                        setAccurateDistType(ALUPipe);
                        if (sameOperandType)
                        {
                            instVec.front()->setIsClosestALUType(isClosedALUType(latestInstID, curDistance, ALUPipe));
                        }
                    }
                }
            }
        }

        int getMaxDepDistance() const
        {
            auto inst = GetInstruction();
            if (inst->getDst() && inst->getDst()->getTypeSize() == 8)
            {  // Note that for XeLP, there are no 8 bytes ALU instruction.
                return SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT;
            }
            else
            {
                return SWSB_MAX_ALU_DEPENDENCE_DISTANCE;
            }
        }

        void setDepToken(unsigned short token, SWSBTokenType type, SBNode *node)
        {
            for (DepToken& depToken : depTokens)
            {
                if (depToken.token == token)
                {
                    if (depToken.type == SWSBTokenType::AFTER_WRITE)
                    {
                        return;
                    }
                    if (type == SWSBTokenType::AFTER_WRITE)
                    {
                        depToken.type = type;
                        depToken.depNode = node;
                    }
                    return;
                }
            }

            struct DepToken dt;
            dt.token = token;
            dt.type = type;
            dt.depNode = node;
            depTokens.push_back(dt);
        }
        void eraseDepToken(unsigned i)
        {
            depTokens.erase(depTokens.begin() + i);
        }

        size_t getDepTokenNum() const { return depTokens.size(); }
        unsigned short getDepToken(unsigned int i, SWSBTokenType& type) const
        {
            type = depTokens[i].type;
            return depTokens[i].token;
        }

        unsigned short getDepTokenNodeID(unsigned int i) const
        {
            return depTokens[i].depNode->getNodeID();;
        }

        void setTokenReuseNode(SBNode *node)
        {
            tokenReusedNode = node;
        }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpInterval() const
        {
            std::cerr << "#" << nodeID << ": " << liveStartID << "-" << liveEndID << "\n";
        }
        void dumpAssignedTokens() const
        {
            std::cerr << "#" << nodeID << ": " << liveStartID << "-" << liveEndID << "\n";
            std::cerr << "Token: " << getLastInstruction()->getToken();
            if (tokenReusedNode != nullptr)
            {
                std::cerr << "\t\tReuse:" << tokenReusedNode->getNodeID();
            }
            std::cerr << "\n";
        }
    };
}
typedef std::vector<vISA::SBNode*>             SBNODE_VECT;
typedef std::vector<vISA::SBNode*>::iterator SBNODE_VECT_ITER;
typedef std::list<vISA::SBNode*>             SBNODE_LIST;
typedef std::list<vISA::SBNode*>::iterator     SBNODE_LIST_ITER;

namespace vISA
{
    //Similar as SBBucketNode, but it's used for the bucket descriptions got from each operands.
    struct SBBucketDesc {
        const int bucket;
        const Gen4_Operand_Number opndNum;
        const SBFootprint* footprint;

        SBBucketDesc(int Bucket, Gen4_Operand_Number opnd_num, const SBFootprint *f)
            : bucket(Bucket), opndNum(opnd_num), footprint(f) {
            ;
        }
    };

    //The node in the node vector of each bucket.
    struct SBBucketNode
    {
        SBNode*             node;
        Gen4_Operand_Number opndNum;
        int                 sendID = -1;
        const SBFootprint*  footprint;

        SBBucketNode(SBNode *node1, Gen4_Operand_Number opndNum1, const SBFootprint *f)
            : node(node1), opndNum(opndNum1), footprint(f)
        {
        }

        ~SBBucketNode()
        {
        }

        void setSendID(int ID)
        {
            sendID = ID;
        }

        int getSendID() const
        {
            return sendID;
        }

        void dump() const
        {
            std::cerr << "#" << node->getNodeID() << "-" << opndNum << ",";
        }
    };

    typedef std::vector<SBBucketNode *> SBBUCKET_VECTOR;
    typedef SBBUCKET_VECTOR::iterator SBBUCKET_VECTOR_ITER;

    // This class hides the internals of dependence tracking using buckets
    class LiveGRFBuckets
    {
        std::vector<SBBUCKET_VECTOR *> nodeBucketsArray;
        vISA::Mem_Manager &mem;
        const int numOfBuckets;

    public:
        LiveGRFBuckets(vISA::Mem_Manager& m, int TOTAL_BUCKETS)
            : nodeBucketsArray(TOTAL_BUCKETS), mem(m), numOfBuckets(TOTAL_BUCKETS)
        {
            // Initialize a vector for each bucket
            for (auto& bucket : nodeBucketsArray)
            {
                void* allocedMem = mem.alloc(sizeof(SBBUCKET_VECTOR));
                bucket = new (allocedMem) SBBUCKET_VECTOR();
            }
        }

        ~LiveGRFBuckets()
        {
            for (int i = 0; i < numOfBuckets; i++)
            {
                SBBUCKET_VECTOR *bucketVec = nodeBucketsArray[i];
                if (bucketVec)
                {
                    bucketVec->~SBBUCKET_VECTOR();
                }
            }
        }

        int getNumOfBuckets() const
        {
            return numOfBuckets;
        }

        //The iterator which is used to scan the node vector of each bucket
        class BN_iterator
        {
        public:
            const LiveGRFBuckets *LB;
            SBBUCKET_VECTOR_ITER node_it;
            int bucket;

            BN_iterator(const LiveGRFBuckets *LB1, SBBUCKET_VECTOR_ITER It, int Bucket)
                : LB(LB1), node_it(It), bucket(Bucket)
            {
            }

            BN_iterator &operator++()
            {
                ++node_it;
                return *this;
            }

            bool operator!=(const BN_iterator &it2) const
            {
                assert(LB == it2.LB);
                return (!(bucket == it2.bucket && node_it == it2.node_it));
            }

            SBBucketNode *operator*()
            {
                assert(node_it != LB->nodeBucketsArray[bucket]->end());
                return *node_it;
            }
        };

        BN_iterator begin(int bucket) const
        {
            return BN_iterator(this, nodeBucketsArray[bucket]->begin(), bucket);
        }

        BN_iterator end(int bucket) const
        {
            return BN_iterator(this, nodeBucketsArray[bucket]->end(), bucket);
        }

        //Scan the node vector of the bucket, and kill the bucket node with the specified node and operand
        void bucketKill(int bucket, SBNode *node, Gen4_Operand_Number opnd)
        {
            SBBUCKET_VECTOR &vec = *nodeBucketsArray[bucket];
            for (unsigned int i = 0; i < vec.size(); i++)
            {
                SBBucketNode *bNode = vec[i];

                //Same node and same operand
                if (bNode->node == node &&
                    bNode->opndNum == opnd)
                {
                    vec.erase(vec.begin() + i);
                    break;
                }
            }
        }
        //Kill the bucket node specified by bn_it
        void killSingleOperand(BN_iterator &bn_it)
        {
            SBBUCKET_VECTOR &vec = *nodeBucketsArray[bn_it.bucket];
            SBBUCKET_VECTOR_ITER &node_it = bn_it.node_it;

            //Kill current node
            if (*node_it == vec.back())
            {
                vec.pop_back();
                node_it = vec.end();
            }
            else
            {
                //Current node is assigned with the last one
                //For caller, same iterator position need be handled again,
                //Because a new node is copied here
                *node_it = vec.back();
                vec.pop_back();
            }
        }

        //Kill the bucket node specified by bn_it, also kill the same node in other buckets
        void killOperand(BN_iterator &bn_it)
        {
            SBBUCKET_VECTOR &vec = *nodeBucketsArray[bn_it.bucket];
            SBBUCKET_VECTOR_ITER &node_it = bn_it.node_it;
            SBBucketNode *bucketNode = *node_it; //Get the node before it is destroyed

            //Kill current node
            if (*node_it == vec.back())
            {
                vec.pop_back();
                node_it = vec.end();
            }
            else
            {
                //Current node is assigned with the last one
                //For caller, same iterator position need be handled again,
                //Because a new node is copied here
                *node_it = vec.back();
                vec.pop_back();
            }

            //Kill the same node in other bucket.
            for (const SBFootprint *footprint = bucketNode->node->getFirstFootprint(bucketNode->opndNum); footprint; footprint = footprint->next)
            {
                assert(footprint->inst != nullptr);
                const IR_Builder& irb = footprint->inst->getBuilder();
                unsigned int startBucket = footprint->LeftB / irb.numEltPerGRF<Type_UB>();
                unsigned int endBucket = footprint->RightB / irb.numEltPerGRF<Type_UB>();

                if (footprint->inst == bucketNode->footprint->inst)
                {
                    for (unsigned int i = startBucket; (i < endBucket + 1) && (i < nodeBucketsArray.size()); i++)
                    {
                        if (i == bn_it.bucket)
                        {
                            continue;
                        }
                        bucketKill(i, bucketNode->node, bucketNode->opndNum);
                    }
                }
            }
        }

        //Add a node into bucket
        void add(SBBucketNode *bucketNode, int bucket)
        {
            assert(nodeBucketsArray[bucket] != nullptr);
            SBBUCKET_VECTOR& nodeVec = *(nodeBucketsArray[bucket]);
            if (std::find(nodeVec.begin(), nodeVec.end(), bucketNode) == nodeVec.end())
            {
                nodeVec.push_back(bucketNode);
            }
        }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpLives() const
        {
            for (int curBucket = 0; curBucket < numOfBuckets; curBucket++)
            {
                if (nodeBucketsArray[curBucket]->size())
                {
                    std::cerr << " GRF" << curBucket << ":";
                    for (SBBucketNode *liveBN : *nodeBucketsArray[curBucket])
                    {
                        std::cerr << " " << liveBN->node->getNodeID() << "(" << liveBN->opndNum << ")";
                    }
                    std::cerr << "\t";
                    if ((curBucket + 1) % 8 == 0)
                    {
                        std::cerr << "\n";
                    }
                }
            }
            std::cerr << "\n";
        }
    };

    typedef std::list<G4_BB_SB *> BB_SWSB_LIST;
    typedef BB_SWSB_LIST::iterator BB_SWSB_LIST_ITER;

    typedef struct _SWSB_INDEXES {
        int instIndex = 0;
        int ALUIndex = 0;
        int integerIndex = 0;
        int floatIndex = 0;
        int longIndex = 0;
        int DPASIndex = 0;
        int mathIndex = 0;
        unsigned latestDepALUID[PIPE_DPAS] = {0};
        std::vector<unsigned> latestInstID[PIPE_DPAS];
    } SWSB_INDEXES;

    typedef struct _SWSB_LOOP {
        int entryBBID = -1 ;
        int endBBID = -1;
    } SWSB_LOOP;

    class G4_BB_SB {
    private:
        const SWSB& swsb;
        IR_Builder& builder;
        vISA::Mem_Manager &mem;
        G4_BB             *bb;
        G4_Label*         BBLabel;
        int nodeID;
        int ALUID;

        SBNode*           dpasNode = nullptr;
        SBNode*           lastDpasNode = nullptr;
        int integerID;
        int floatID;
        int longID;
        int mathID;
        int DPASID;
        unsigned latestDepALUID[PIPE_DPAS];
        std::vector<unsigned> *latestInstID[PIPE_DPAS];

        int totalGRFNum;
        int tokenAfterDPASCycle;

    public:
        LiveGRFBuckets *send_use_kills;
        BB_SWSB_LIST      Preds;
        BB_SWSB_LIST      Succs;

        BB_SWSB_LIST      domPreds;
        BB_SWSB_LIST      domSuccs;

        int first_node = -1;
        int last_node = -1;

        int first_send_node;
        int last_send_node;

        bool tokenAssigned = false;

        int send_start = -1;
        int send_end = -1;

        unsigned      loopStartBBID = -1;    // The start BB ID of live range
        unsigned      loopEndBBID = -1;    // The start BB ID of live range

        SBBitSets send_def_out;
        SBBitSets send_live_in;
        SBBitSets send_live_out;
        SBBitSets send_may_kill;
        SBBitSets send_live_in_scalar;
        SBBitSets send_live_out_scalar;
        SBBitSets send_kill_scalar;

        BitSet dominators;
        BitSet send_WAW_may_kill;

        //For token reduction
        BitSet   liveInTokenNodes;
        BitSet   liveOutTokenNodes;
        BitSet   killedTokens;
        std::vector<BitSet> tokeNodesMap;
        int first_DPASID = 0;
        int last_DPASID = 0;
        unsigned    *tokenLiveInDist;
        unsigned    *tokenLiveOutDist;
        SBBitSets localReachingSends;

        //BB local data dependence analysis
        G4_BB_SB(const SWSB& sb, IR_Builder& b, Mem_Manager &m, G4_BB *block, SBNODE_VECT *SBNodes, SBNODE_VECT* SBSendNodes,
            SBBUCKET_VECTOR *globalSendOpndList,  SWSB_INDEXES *indexes, uint32_t &globalSendNum, LiveGRFBuckets *lb,
            LiveGRFBuckets *globalLB, PointsToAnalysis& p,
            std::map<G4_Label*, G4_BB_SB*> *LabelToBlockMap, const unsigned dpasLatency) : swsb(sb), builder(b), mem(m), bb(block), tokenAfterDPASCycle(dpasLatency)
        {
            for (int i = 0; i < PIPE_DPAS; i++)
            {
                latestDepALUID[i] = 0;
                latestInstID[i] = nullptr;
            }
            first_send_node = -1;
            last_send_node = -1;
            totalGRFNum = block->getKernel().getNumRegTotal();
            SBDDD(bb, lb, globalLB, SBNodes, SBSendNodes, globalSendOpndList,  indexes, globalSendNum, p, LabelToBlockMap);
        }

        ~G4_BB_SB()
        {
        }

        G4_BB* getBB() const { return bb; }
        G4_Label* getLabel() const { return BBLabel; }

        static bool isGRFEdgeAdded(const SBNode* pred, const SBNode* succ, DepType d, SBDependenceAttr a);
        void createAddGRFEdge(SBNode* pred, SBNode* succ, DepType d, SBDependenceAttr a);

        //Bucket and range analysis
        SBFootprint* getFootprintForGRF(G4_Operand * opnd,
            Gen4_Operand_Number opnd_num,
            G4_INST *inst,
            int startingBucket,
            bool mustBeWholeGRF);
        SBFootprint * getFootprintForACC(G4_Operand * opnd,
            Gen4_Operand_Number opnd_num,
            G4_INST *inst);
        SBFootprint* getFootprintForFlag(G4_Operand* opnd,
            Gen4_Operand_Number opnd_num,
            G4_INST* inst);
        bool isLongPipeType(G4_Type type) const;
        bool isIntegerPipeType(G4_Type type) const;
        bool isLongPipeInstructionXe(const G4_INST* inst) const;
        bool isIntegerPipeInstructionXe(const G4_INST* inst) const;
        bool isFloatPipeInstructionXe(const G4_INST* inst) const;
        SB_INST_PIPE getInstructionPipeXe(const G4_INST* inst);
        bool getFootprintForOperand(SBNode *node,
            G4_INST *inst,
            G4_Operand* opnd,
            Gen4_Operand_Number opnd_num);
        void getGRFBuckets(const SBFootprint* footprint, Gen4_Operand_Number opndNum, std::vector<SBBucketDesc>& BDvec, bool GRFOnly);
        bool getGRFFootPrintOperands(SBNode *node,
            G4_INST *inst,
            Gen4_Operand_Number first_opnd,
            Gen4_Operand_Number last_opnd,
            PointsToAnalysis& p);
        void getGRFFootprintForIndirect(SBNode* node,
            Gen4_Operand_Number opnd_num,
            G4_Operand* opnd,
            PointsToAnalysis& p);
        void getGRFBucketsForOperands(SBNode *node,
            Gen4_Operand_Number first_opnd,
            Gen4_Operand_Number last_opnd,
            std::vector<SBBucketDesc>& BDvec,
            bool GRFOnly);

        bool hasIndirectSource(SBNode* node);

        bool getGRFFootPrint(SBNode *node,
            PointsToAnalysis &p);

        void getGRFBucketDescs(SBNode *node,
            std::vector<SBBucketDesc>& BDvec,
            bool GRFOnly);

        void clearSLMWARWAissue(SBNode* curNode, LiveGRFBuckets* LB);

        void setDistance(const SBFootprint * footprint, SBNode *node, SBNode *liveNode, bool dstDep);
        void setSpecialDistance(SBNode* node);
        static void footprintMerge(SBNode * node, const SBNode * nextNode);

        void pushItemToQueue(std::vector<unsigned>* nodeIDQueue, unsigned nodeID);

        bool hasInternalDependence(SBNode* nodeFirst, SBNode* nodeNext);

        bool is2xDPBlockCandidate(G4_INST* inst, bool accDST);
        //Local distance dependence analysis and assignment
        void SBDDD(G4_BB* bb,
            LiveGRFBuckets* &LB,
            LiveGRFBuckets* &globalSendsLB,
            SBNODE_VECT *SBNodes,
            SBNODE_VECT *SBSendNodes,
            SBBUCKET_VECTOR *globalSendOpndList,
            SWSB_INDEXES *indexes,
            uint32_t &globalSendNum,
            PointsToAnalysis& p,
            std::map<G4_Label*, G4_BB_SB*> *LabelToBlockMap);

        //Global SBID dependence analysis
        void setSendOpndMayKilled(LiveGRFBuckets *globalSendsLB, SBNODE_VECT *SBNodes, PointsToAnalysis &p);
        void dumpTokenLiveInfo(SBNODE_VECT * SBSendNodes);
        void getLiveBucketsFromFootprint(const SBFootprint *firstFootprint, SBBucketNode* sBucketNode, LiveGRFBuckets *send_use_kills) const;
        void addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR *globalSendOpndList, SBNODE_VECT *SBNodes, PointsToAnalysis &p, bool afterWrite);
        void clearKilledBucketNodeXeLP(LiveGRFBuckets * LB, int ALUID);

        void clearKilledBucketNodeXeHP(LiveGRFBuckets* LB, int integerID, int floatID, int longID, int mathID);

        bool hasInternalDependenceWithinDPAS(SBNode *node);
        bool hasDependenceBetweenDPASNodes(SBNode * node, SBNode * nextNode);
        bool src2FootPrintCachePVC(SBNode* curNode, SBNode* nextNode) const;
        bool src2SameFootPrintDiffType(SBNode* curNode, SBNode* nextNode) const;
        bool isLastDpas(SBNode * curNode, SBNode * nextNode);


        void getLiveOutToken(unsigned allSendNum, const SBNODE_VECT *SBNodes);


        unsigned getLoopStartBBID() const { return loopStartBBID; }
        unsigned getLoopEndBBID() const { return loopEndBBID; }

        void setLoopStartBBID(unsigned id) { loopStartBBID = id; }
        void setLoopEndBBID(unsigned id) { loopEndBBID = id; }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpLiveInfo(const SBBUCKET_VECTOR *globalSendOpndList, unsigned globalSendNum, const SBBitSets *send_kill) const;
    };

    typedef std::vector<G4_BB_SB *> BB_SWSB_VECTOR;

    class SWSB_TOKEN_PROFILE {
        uint32_t tokenInstructionCount = 0;
        uint32_t tokenReuseCount = 0;
        uint32_t AWTokenReuseCount = 0;
        uint32_t ARTokenReuseCount = 0;
        uint32_t AATokenReuseCount = 0;
        uint32_t mathInstCount = 0;
        uint32_t syncInstCount = 0;
        uint32_t mathReuseCount = 0;
        uint32_t ARSyncInstCount = 0;
        uint32_t AWSyncInstCount = 0;
        uint32_t ARSyncAllCount = 0;
        uint32_t AWSyncAllCount = 0;
        uint32_t prunedDepEdges = 0;
        uint32_t prunedGlobalEdgeNum = 0;
        uint32_t prunedDiffBBEdgeNum = 0;
        uint32_t prunedDiffBBSameTokenEdgeNum = 0;
    public:
        SWSB_TOKEN_PROFILE() {
            ;
        }

        ~SWSB_TOKEN_PROFILE()
        {
        }
        void* operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void setTokenInstructionCount(int count) { tokenInstructionCount = count; }
        uint32_t getTokenInstructionCount() const { return tokenInstructionCount; }

        void setTokenReuseCount(int count) { tokenReuseCount = count; }
        uint32_t getTokenReuseCount() const { return tokenReuseCount; }

        void setAWTokenReuseCount(int count) { AWTokenReuseCount = count; }
        uint32_t getAWTokenReuseCount() const { return AWTokenReuseCount; }

        void setARTokenReuseCount(int count) { ARTokenReuseCount = count; }
        uint32_t getARTokenReuseCount() const { return ARTokenReuseCount; }

        void setAATokenReuseCount(int count) { AATokenReuseCount = count; }
        uint32_t getAATokenReuseCount() const { return AATokenReuseCount; }

        void setMathInstCount(int count) { mathInstCount = count; }
        uint32_t getMathInstCount() const { return mathInstCount; }

        void setSyncInstCount(int count) { syncInstCount = count; }
        uint32_t getSyncInstCount() const { return syncInstCount; }

        void setMathReuseCount(int count) { mathReuseCount = count; }
        uint32_t getMathReuseCount() const { return mathReuseCount; }

        void setARSyncInstCount(int count) { ARSyncInstCount = count; }
        uint32_t getARSyncInstCount() const { return ARSyncInstCount; }

        void setAWSyncInstCount(int count) { AWSyncInstCount = count; }
        uint32_t getAWSyncInstCount() const { return AWSyncInstCount; }

        void setARSyncAllCount(int count) { ARSyncAllCount = count; }
        uint32_t getARSyncAllCount() const { return ARSyncAllCount; }

        void setAWSyncAllCount(int count) { AWSyncAllCount = count; }
        uint32_t getAWSyncAllCount() const { return AWSyncAllCount; }

        void setPrunedEdgeNum(int num) { prunedDepEdges = num; }
        uint32_t getPrunedEdgeNum() const { return prunedDepEdges; }

        void setPrunedGlobalEdgeNum(int num) { prunedGlobalEdgeNum = num; }
        uint32_t getPrunedGlobalEdgeNum() const { return prunedGlobalEdgeNum; }

        void setPrunedDiffBBEdgeNum(int num) { prunedDiffBBEdgeNum = num; }
        uint32_t getPrunedDiffBBEdgeNum() const { return prunedDiffBBEdgeNum; }

        void setPrunedDiffBBSameTokenEdgeNum(int num) { prunedDiffBBSameTokenEdgeNum = num; }
        uint32_t getPrunedDiffBBSameTokenEdgeNum() const { return prunedDiffBBSameTokenEdgeNum; }
    };

    class SWSB {
        G4_Kernel &kernel;
        FlowGraph&  fg;
        vISA::Mem_Manager& mem;

        BB_SWSB_VECTOR BBVector;    // The basic block vector, ordered with ID of the BB
        SBNODE_VECT SBNodes;        // All instruction nodes
        SBNODE_VECT SBSendNodes;    // All out-of-order instruction nodes
        SBNODE_VECT SBSendUses;    // All out-of-order instruction nodes
#ifdef DEBUG_VERBOSE_ON
        SBNODE_VECT globalSBNodes;        // All instruction nodes
#endif
        SWSB_INDEXES indexes;         // To pass ALU ID  from previous BB to current.
        uint32_t  globalSendNum = 0;  // The number of out-of-order instructions which generate global dependencies.
        SBBUCKET_VECTOR globalSendOpndList;  //All send operands which live out their instructions' BBs. No redundant.
        const uint32_t totalTokenNum;
        static constexpr unsigned TOKEN_AFTER_READ_CYCLE = 4;
        const unsigned tokenAfterWriteMathCycle;
        const unsigned tokenAfterWriteSendSlmCycle;
        const unsigned tokenAfterWriteSendMemoryCycle;
        const unsigned tokenAfterWriteSendSamplerCycle;
        int tokenAfterDPASCycle;

        //For profiling
        uint32_t syncInstCount = 0;
        uint32_t AWSyncInstCount = 0;
        uint32_t ARSyncInstCount = 0;
        uint32_t mathReuseCount = 0;
        uint32_t ARSyncAllCount = 0;
        uint32_t AWSyncAllCount = 0;
        uint32_t tokenReuseCount = 0;

        bool hasFCall = false;
        //Linear scan data structures for token allocation
        SBNODE_LIST linearScanLiveNodes;

        std::vector<SBNode *> freeTokenList;

        std::vector<SBNODE_VECT *> reachTokenArray;
        std::vector<SBNODE_VECT *> reachUseArray;
        SBNODE_VECT localTokenUsage;

        int topIndex = -1;

        std::map<G4_Label*, G4_BB_SB*> labelToBlockMap;

        // TokenAllocation uses a BitSet to track nodes assigned by marking the
        // send IDs of nodes, so that it's possible to get a SBNode using the
        // send ID to index into SBSendNodes.
        // Also add a filed to record the max send ID being assigned.
        struct TokenAllocation
        {
            BitSet bitset;
            int maxSendID = 0;
            void set(int sendID)
            {
                bitset.set(sendID, true);
                maxSendID = std::max(sendID, maxSendID);
            }
        };
        std::vector<TokenAllocation> allTokenNodesMap;
        SWSB_TOKEN_PROFILE tokenProfile;

        //Global dependence analysis
        bool globalDependenceDefReachAnalysis(G4_BB* bb);
        bool globalDependenceUseReachAnalysis(G4_BB* bb);
        void addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR *globalSendOpndList, SBNODE_VECT *SBNodes, PointsToAnalysis &p, bool afterWrite);
        void tokenEdgePrune(unsigned& prunedEdgeNum, unsigned& prunedGlobalEdgeNum, unsigned& prunedDiffBBEdgeNum, unsigned& prunedDiffBBSameTokenEdgeNum);
        void dumpTokenLiveInfo();

        void removePredsEdges(SBNode * node, SBNode * pred);

        void dumpImmDom(ImmDominator* dom) const;

        void setDefaultDistanceAtFirstInstruction();

        void quickTokenAllocation();

        //Token allocation
        void tokenAllocation();
        void buildLiveIntervals();
        void expireIntervals(unsigned startID);
        void addToLiveList(SBNode *node);

        std::pair<int /*reuseDelay*/, int /*curDistance*/> examineNodeForTokenReuse(unsigned nodeID, unsigned nodeDelay, const SBNode *curNode, unsigned char nestLoopLevel, unsigned curLoopStartBB, unsigned curLoopEndBB) const;
        SBNode * reuseTokenSelection(const SBNode * node) const;
        unsigned short reuseTokenSelectionGlobal(SBNode* node, G4_BB* bb, SBNode*& candidateNode, bool& fromUse);
        void addReachingDefineSet(SBNode* node, SBBitSets* globalLiveSet, SBBitSets* localLiveSet);
        void addReachingUseSet(SBNode* node, SBNode* use);
        void addGlobalDependenceWithReachingDef(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite);
        void expireLocalIntervals(unsigned startID, unsigned BBID);
        void assignTokenToPred(SBNode* node, SBNode* pred, G4_BB* bb);
        bool assignTokenWithPred(SBNode* node, G4_BB* bb);
        void allocateToken(G4_BB* bb);
        void tokenAllocationBB(G4_BB* bb);
        void buildExclusiveForCoalescing();
        void tokenAllocationGlobalWithPropogation();
        void tokenAllocationGlobal();
        bool propogateDist(G4_BB* bb);
        void tokenAllocationWithDistPropogationPerBB(G4_BB* bb);
        void tokenAllocationWithDistPropogation();
        void calculateDist();


        //Assign Token
        void assignToken(SBNode *node, unsigned short token, uint32_t &AWTokenReuseCount, uint32_t &ARTokenReuseCount, uint32_t &AATokenReuseCount);
        void assignDepToken(SBNode *node);
        void assignDepTokens();
        bool insertSyncTokenPVC(G4_BB * bb, SBNode * node, G4_INST * inst, INST_LIST_ITER inst_it, int newInstID, BitSet * dstTokens, BitSet * srcTokens, bool removeAllToken);
        bool insertSyncPVC(G4_BB * bb, SBNode * node, G4_INST * inst, INST_LIST_ITER inst_it, int newInstID, BitSet * dstTokens, BitSet * srcTokens);
        bool insertSyncXe(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens);
        void insertSync(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens);
        void insertTest();

        //Insert sync instructions
        G4_INST *insertTestInstruction(G4_BB *bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo, bool countSyns);  //FIXME: Please remove it when meta is not needed anymore
        G4_INST *insertSyncInstruction(G4_BB *bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST *insertSyncInstructionAfter(G4_BB * bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST* insertSyncAllRDInstruction(G4_BB *bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST *insertSyncAllWRInstruction(G4_BB *bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo);

        bool insertSyncToken(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens, bool& keepDst, bool removeAllToken);

        void SWSBDepDistanceGenerator(PointsToAnalysis& p, LiveGRFBuckets &LB, LiveGRFBuckets &globalSendsLB);
        void handleFuncCall();
        void SWSBGlobalTokenGenerator(PointsToAnalysis& p, LiveGRFBuckets &LB, LiveGRFBuckets &globalSendsLB);
        void SWSBBuildSIMDCFG();
        void addSIMDEdge(G4_BB_SB *pred, G4_BB_SB* succ);
        void SWSBGlobalScalarCFGReachAnalysis();
        void SWSBGlobalSIMDCFGReachAnalysis();

        void setTopTokenIndex();

        //Optimizations
        void tokenDepReduction(SBNode* node1, SBNode *node2);
        bool cycleExpired(const SBNode * node, int currentID) const;

        void shareToken(const SBNode *node, const SBNode *succ, unsigned short token);

        void SWSBGlobalTokenAnalysis();
        bool globalTokenReachAnalysis(G4_BB *bb);


        //Dump
        void dumpDepInfo() const;
        void dumpLiveIntervals() const;
        void dumpTokeAssignResult() const;
        void dumpSync(const SBNode * tokenNode, const SBNode * syncNode, unsigned short token, SWSBTokenType type) const;

        // Fast-composite support.
        void genSWSBPatchInfo();

        void getDominators(ImmDominator* dom);


    public:
        SWSB(G4_Kernel &k, vISA::Mem_Manager& m)
            : kernel(k), fg(k.fg), mem(m),
            totalTokenNum(k.fg.builder->kernel.getNumSWSBTokens()),
            tokenAfterWriteMathCycle(k.fg.builder->isXeLP() ? 20u : 17u),
            tokenAfterWriteSendSlmCycle(k.fg.builder->isXeLP() ? 33u : 25u), //unlocaled 25
            tokenAfterWriteSendMemoryCycle(k.fg.builder->getOptions()->getOption(vISA_USEL3HIT)
                ? (k.fg.builder->isXeLP() ? 106u : 150u) // TOKEN_AFTER_WRITE_SEND_L3_MEMORY_CYCLE
                : (k.fg.builder->isXeLP() ? 65u : 50u)), // TOKEN_AFTER_WRITE_SEND_L1_MEMORY_CYCLE
            tokenAfterWriteSendSamplerCycle(k.fg.builder->getOptions()->getOption(vISA_USEL3HIT)
                ? (k.fg.builder->isXeLP() ? 175u : 210u) // TOKEN_AFTER_WRITE_SEND_L3_SAMPLER_CYCLE
                : 60u)                                   // TOKEN_AFTER_WRITE_SEND_L1_SAMPLER_CYCLE
        {
            indexes.instIndex = 0;
            indexes.ALUIndex = 0;
            indexes.integerIndex = 0;
            indexes.floatIndex = 0;
            indexes.longIndex = 0;
            indexes.DPASIndex = 0;
            indexes.mathIndex = 0;
            LatencyTable LT(k.fg.builder);
            tokenAfterDPASCycle = LT.getDPAS8x8Latency();
        }
        ~SWSB()
        {
            for (SBNode *node : SBNodes)
            {
                node->~SBNode();
            }

            for (G4_BB_SB *bb : BBVector)
            {
                bb->~G4_BB_SB();
            }

            for (int i = 0; i != (int)reachTokenArray.size(); ++i)
            {
                 reachTokenArray[i]->~SBNODE_VECT();
                 reachUseArray[i]->~SBNODE_VECT();
            }
        }
        void SWSBGenerator();
        unsigned calcDepDelayForNode(const SBNode *node) const;
    };
}
#endif // _SWSB_H_
