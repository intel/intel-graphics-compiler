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

#ifndef _SWSB_H_
#define _SWSB_H_
#include <string>
#include <set>
#include <bitset>
#include "../Mem_Manager.h"
#include "../FlowGraph.h"
#include "../Gen4_IR.hpp"
#include "../Timer.h"
#include "../RegAlloc.h"
#include <vector>
#include "../BitSet.h"
#include "LocalScheduler_G4IR.h"

namespace vISA
{
    class SBNode;
    struct SBBucketNode;
    class SBDDD;
    class G4_BB_SB;
    class SWSB;
}

//FIXME, build a table for different plaforms
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE 11
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT 15
#define SWSB_MAX_MATH_DEPENDENCE_DISTANCE 18
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE 7

#define TOKEN_AFTER_READ_CYCLE 4

#define TOKEN_AFTER_WRITE_MATH_CYCLE (fg.builder->isGen12LP() ? 20u : 17u)
#define TOKEN_AFTER_WRITE_SEND_SLM_CYCLE (fg.builder->isGen12LP() ? 33u : 25u)   //unlocaled 25
#define TOKEN_AFTER_WRITE_SEND_L1_MEMORY_CYCLE (fg.builder->isGen12LP() ? 65u : 50u)
#define TOKEN_AFTER_WRITE_SEND_L1_SAMPLER_CYCLE 60u
#define TOKEN_AFTER_WRITE_SEND_L3_MEMORY_CYCLE (fg.builder->isGen12LP() ? 106u : 150u)
#define TOKEN_AFTER_WRITE_SEND_L3_SAMPLER_CYCLE (fg.builder->isGen12LP() ? 175u : 210u)
#define TOKEN_AFTER_WRITE_SEND_MEMORY_CYCLE (fg.builder->getOptions()->getOption(vISA_USEL3HIT) ? TOKEN_AFTER_WRITE_SEND_L3_MEMORY_CYCLE : TOKEN_AFTER_WRITE_SEND_L1_MEMORY_CYCLE)
#define TOKEN_AFTER_WRITE_SEND_SAMPLER_CYCLE (fg.builder->getOptions()->getOption(vISA_USEL3HIT) ? TOKEN_AFTER_WRITE_SEND_L3_SAMPLER_CYCLE : TOKEN_AFTER_WRITE_SEND_L1_SAMPLER_CYCLE)
#define TOKEN_MAXIMAL_DELAY_CYCLE TOKEN_AFTER_WRITE_SEND_SAMPLER_CYCLE
#define TOKEN_REUSE_DISTANCE  4   //The node ID distance


#define DEPENCENCE_ATTR(DO) \
    DO(DEP_EXPLICT) \
    DO(DEP_IMPLICIT)

enum SBDependenceAttr
{
    DEPENCENCE_ATTR(MAKE_ENUM)
};


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


    using SWSBTokenType = G4_INST::SWSBTokenType;
    void singleInstStallSWSB(G4_Kernel *kernel, uint32_t instID, uint32_t endInstID, bool is_barrier);
    void forceDebugSWSB(G4_Kernel *kernel);
    void forceFCSWSB(G4_Kernel *kernel);
}

typedef vISA::SBDep SBDEP_ITEM;
typedef std::vector< SBDEP_ITEM> SBDEP_VECTOR;
typedef SBDEP_VECTOR::iterator SBDEP_VECTOR_ITER;


namespace vISA
{
    typedef enum  _FOOTPRINT_TYPE
    {
        GRF_T = 1,
        ACC_T = 2,
        FLAG_T = 4
    } FOOTPRINT_TYPE;

    struct SBFootprint {
        FOOTPRINT_TYPE fType;
        G4_Type type;
        unsigned short LeftB;
        unsigned short RightB;
        unsigned short offset;
        G4_INST*       inst;
        struct SBFootprint *next;             //The ranges linked together to represent the possible registser ranges may be occupied by an operand.
                                         //For indirect access, non-contigious ranges exist.

        SBFootprint(void) : fType(GRF_T), type (Type_UNDEF), LeftB(0), RightB(0), offset(0), inst(nullptr), next(nullptr) { ; }
        SBFootprint(FOOTPRINT_TYPE ft, G4_Type t, unsigned short LB, unsigned short RB, G4_INST *i)
            : fType(ft), type(t), LeftB(LB), RightB(RB), offset(0), inst(i), next(nullptr) {
            ;
        }
        ~SBFootprint()
        {
        }

        void setOffset(unsigned short o) { offset = o; }

        bool hasOverlap(SBFootprint *liveFootprint, unsigned short &internalOffset) const
        {
            SBFootprint *curFootprint2Ptr = liveFootprint;
            while (curFootprint2Ptr)
            {
                // Negative of no overlap: !(LeftB > curFootprint2Ptr->RightB || RightB < curFootprint2Ptr->LeftB)
                if (fType == curFootprint2Ptr->fType &&
                    LeftB <= curFootprint2Ptr->RightB && RightB >= curFootprint2Ptr->LeftB)
                {
                    internalOffset = curFootprint2Ptr->offset;
                    return true;
                }
                curFootprint2Ptr = curFootprint2Ptr->next;
            }

            //Overlap with other ranges.
            SBFootprint *curFootprintPtr = next;
            while (curFootprintPtr)
            {
                curFootprint2Ptr = liveFootprint;
                while (curFootprint2Ptr)
                {
                    if (fType == curFootprint2Ptr->fType &&
                           curFootprintPtr->LeftB <= curFootprint2Ptr->RightB && curFootprintPtr->RightB >= curFootprint2Ptr->LeftB)
                    {
                        internalOffset = curFootprint2Ptr->offset;
                        return true;
                    }
                    curFootprint2Ptr = curFootprint2Ptr->next;
                }
                curFootprintPtr = curFootprintPtr->next;
            }

            return false;
        }

        bool hasGRFGrainOverlap(SBFootprint *liveFootprint) const
        {
            SBFootprint *curFootprint2Ptr = liveFootprint;
            while (curFootprint2Ptr)
            {
                if (fType == curFootprint2Ptr->fType &&
                    ((LeftB / G4_GRF_REG_NBYTES) <= (curFootprint2Ptr->RightB / G4_GRF_REG_NBYTES)) &&
                    ((RightB / G4_GRF_REG_NBYTES) >= (curFootprint2Ptr->LeftB / G4_GRF_REG_NBYTES)))
                {
                    return true;
                }
                curFootprint2Ptr = curFootprint2Ptr->next;
            }

            //Overlap with other ranges.
            SBFootprint *curFootprintPtr = next;
            while (curFootprintPtr)
            {
                curFootprint2Ptr = liveFootprint;
                while (curFootprint2Ptr)
                {
                    if (fType == curFootprint2Ptr->fType &&
                        ((curFootprintPtr->LeftB  / G4_GRF_REG_NBYTES) <= (curFootprint2Ptr->RightB  / G4_GRF_REG_NBYTES)) &&
                        ((curFootprintPtr->RightB  / G4_GRF_REG_NBYTES) >= (curFootprint2Ptr->LeftB  / G4_GRF_REG_NBYTES)))
                    {
                        return true;
                    }
                    curFootprint2Ptr = curFootprint2Ptr->next;
                }
                curFootprintPtr = curFootprintPtr->next;
            }

            return false;
        }


        //Check if current footprint overlaps footprint2
        //FIXME: it's conservative. Because for the indirect, the ranges may be contigious?
        bool isWholeOverlap(SBFootprint *liveFootprint) const
        {
            SBFootprint *curFootprintPtr = nullptr;
            SBFootprint *footprint2Ptr = liveFootprint;

            while (footprint2Ptr)
            {
                if (fType == footprint2Ptr->fType &&
                    LeftB <= footprint2Ptr->LeftB && RightB >= footprint2Ptr->RightB)
                {
                    return true;
                }
                curFootprintPtr = next;
                while (curFootprintPtr)
                {
                    if (fType == footprint2Ptr->fType &&
                            curFootprintPtr->LeftB <= footprint2Ptr->LeftB && curFootprintPtr->RightB >= footprint2Ptr->RightB)
                    {
                        return true;
                    }
                    curFootprintPtr = curFootprintPtr->next;
                }
                footprint2Ptr = footprint2Ptr->next;
            }

            return false;
        }

    };

    typedef std::vector<vISA::SBFootprint*>::iterator SBMASK_VECT_ITER;

    // Bit set which is used for global dependence analysis for SBID.
    // Since dependencies may come from dst and src and there may be dependence kill between dst and src depencencies,
    // we use internal bit set to track the live of dst and src seperately.
    // Each bit map to one global SBID node according to the node's global ID.
    struct SBBitSets {
        BitSet dst;
        BitSet src;

        SBBitSets(vISA::Mem_Manager& mem, unsigned size) : dst(size, false), src(size, false)
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

        bool isEmpty()
        {
            return dst.isEmpty() && src.isEmpty();
        }

        bool isDstEmpty()
        {
            return dst.isEmpty();
        }

        bool isSrcEmpty()
        {
            return src.isEmpty();
        }

        bool isDstSet(unsigned i)
        {
            return dst.isSet(i);
        }

        bool isSrcSet(unsigned i)
        {
            return src.isSet(i);
        }

        SBBitSets& operator= (const SBBitSets& other)
        {
            dst = other.dst;
            src = other.src;
            return *this;
        }

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

        bool operator!= (const SBBitSets& other)
        {
            return (dst != other.dst) || (src != other.src);
        }

        void* operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }
    };

    class SBNode {
    private:
        std::vector<SBFootprint*>  footprints;  // The coarse grained footprint of operands
        unsigned      nodeID;          // Unique ID of the node
        unsigned      BBID;           // ID of basic block
        int      ALUID;          // The ID for in-order instructions. The out-of-order instructions are not counted.
        unsigned      liveStartID;    // The start ID of live range
        unsigned      liveEndID;      // The end ID of live range
        unsigned      liveStartBBID;    // The start BB ID of live range
        unsigned      liveEndBBID;    // The start BB ID of live range
        bool          instKilled;     // Used for global analysis, if the node generated dependencies have all been resolved.
        bool          sourceKilled;   // If the dependencies caused by source operands have all been resolved.
        bool          hasAW;          // Used for global analysis, if has AW (RAW or WAW) dependencies from the node
        bool          hasAR;          // Used for global analysis, if has AR (WAR) dependencies from the node
        bool          hasFollowDistOneAReg;
        bool          followDistOneAReg;

    public:
        std::vector<G4_INST *> instVec;
        SBDEP_VECTOR   succs;          // A list of node's successors in dependence graph
        SBDEP_VECTOR   preds;          // A list of node's predecessors in dependence graph
        int            globalID;      // ID of global send instructions
        int            sendID;
        int            sendUseID;
        SBNode *       tokenReusedNode;  // For global token reuse optimization, the node whose token is reused by current one.
        SBBitSets* reachingSends;
        SBBitSets* reachedUses;
        unsigned reuseOverhead;

        /* Constructor */
        SBNode(uint32_t id, int ALUId, uint32_t BBId, G4_INST *i)
            : nodeID(id), ALUID(ALUId), BBID(BBId),
            liveStartID(0), liveStartBBID(-1), liveEndID(0), liveEndBBID(-1), instKilled(false), sourceKilled(false), hasAW(false), hasAR(false), hasFollowDistOneAReg(false), footprints(Opnd_total_num, nullptr), globalID(-1)
        {

            sendID = -1;
            sendUseID = -1;
            instVec.push_back(i);
            tokenReusedNode = nullptr;
            reachingSends = nullptr;
            reachedUses = nullptr;
            reuseOverhead = 0;
        }

        ~SBNode()
        {
            for (auto sm_it = footprints.begin();
                sm_it != footprints.end();
                sm_it++)
            {
                SBFootprint* sm = *sm_it;
                sm->~SBFootprint();
            }
        }
        void setSendID(int ID)
        {
            sendID = ID;
            return;
        }

        void setSendUseID(int ID)
        {
            sendUseID = ID;
            return;
        }

        int getSendID()
        {
            return sendID;
        }

        int getSendUseID()
        {
            return sendUseID;
        }

        /* Member functions */
        G4_INST*  GetInstruction(void) const { return instVec.front(); }
        void addInstruction(G4_INST *i) { instVec.push_back(i); }
        G4_INST*  getLastInstruction(void) const { return instVec.back(); }

        int getALUID() { return ALUID; }
        unsigned getNodeID(void) { return nodeID; };
        unsigned getBBID(void) { return BBID; };


        unsigned getLiveStartID() { return liveStartID; }
        unsigned getLiveEndID() { return liveEndID; }

        unsigned getLiveStartBBID() { return liveStartBBID; }
        unsigned getLiveEndBBID() { return liveEndBBID; }

        void setLiveEarliesID(unsigned id, unsigned startBBID)
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
            return;
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
            return;
        }

        void setLiveEarliesID(unsigned id)
        {
            liveStartID = id;
            return;
        }

        void setLiveLatestID(unsigned id)
        {
            liveEndID = id;

            return;
        }

        void setInstKilled(bool value)
        {
            instKilled = value;
            if (value)
            {
                hasAW = true;
            }
        }

        bool isInstKilled()
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

        bool isSourceKilled()
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

        bool hasAWDep() { return hasAW; }
        bool hasARDep() { return hasAR; }
        bool hasDistOneAreg() { return hasFollowDistOneAReg; }
        bool followDistOneAreg() { return followDistOneAReg; }

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

        SBFootprint *getFootprint(Gen4_Operand_Number opndNum)
        {
            return footprints[opndNum];
        }

        void setDistance(unsigned distance)
        {
            if (distance > SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE)
            {
                distance = SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE;
            }
            unsigned curDistance = (unsigned)instVec.front()->getDistance();
            if (curDistance == 0 ||
                curDistance > distance)
            {
                instVec.front()->setDistance((unsigned char)distance);
            }
        }


        int getMaxDepDistance() const
        {
            auto inst = GetInstruction();
            if (inst->getDst() && getTypeSize(inst->getDst()->getType()) == 8)
            {  //Note that for Gen12lp, there is no 8 bytes ALU instruction.
                return SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT;
            }
            else
            {
                return SWSB_MAX_ALU_DEPENDENCE_DISTANCE;
            }
        }

        void setTokenReuseNode(SBNode *node)
        {
            tokenReusedNode = node;
        }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpInterval()
        {
            std::cerr << "#" << nodeID << ": " << liveStartID << "-" << liveEndID << "\n";
        }
        void dumpAssignedTokens()
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
    struct SBBucketDescr {
        int bucket;
        Gen4_Operand_Number opndNum;
        SBNode*  node;
        G4_INST* inst;

        SBBucketDescr(int Bucket, Gen4_Operand_Number opnd_num, SBNode *sNode, G4_INST *i)
            : bucket(Bucket), opndNum(opnd_num), inst(i), node(sNode) {
            ;
        }
    };

    //The node in the node vector of each bucket.
    struct SBBucketNode
    {
        SBNode*             node;
        Gen4_Operand_Number opndNum;
        int                 sendID;
        G4_INST*            inst;

        SBBucketNode(SBNode *node1, Gen4_Operand_Number opndNum1, G4_INST *i)
            : node(node1), opndNum(opndNum1), sendID(-1), inst(i)
        {
        }

        ~SBBucketNode()
        {
        }

        void setSendID(int ID)
        {
            sendID = ID;
            return;
        }

        int getSendID()
        {
            return sendID;
        }

        void dump()
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
        G4_Kernel &k;
        vISA::Mem_Manager &mem;
        int numOfBuckets;

    public:
        LiveGRFBuckets(vISA::Mem_Manager& m, int TOTAL_BUCKETS, G4_Kernel& k)
            : k(k)
            , mem(m)
        {
            numOfBuckets = TOTAL_BUCKETS;
            nodeBucketsArray.resize(numOfBuckets);

            // Initialize a vector for each bucket
            for (int bucket_i = 0; bucket_i != (int)numOfBuckets; ++bucket_i)
            {
                void* allocedMem = mem.alloc(sizeof(SBBUCKET_VECTOR));
                nodeBucketsArray[bucket_i] = new (allocedMem) SBBUCKET_VECTOR();
            }
        }

        ~LiveGRFBuckets(void)
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

        int getNumOfBuckets()
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

            bool operator!=(const BN_iterator &it2)
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
                //For caller, same iterator postion need be handled again,
                //Beause a new node is copied here
                *node_it = vec.back();
                vec.pop_back();
            }
        }

        //Kill the bucket node specified by bn_it, also kill the same node in other buckets
        void killOperand(BN_iterator &bn_it)
        {
            SBBUCKET_VECTOR &vec = *nodeBucketsArray[bn_it.bucket];
            SBBUCKET_VECTOR_ITER &node_it = bn_it.node_it;
            SBBucketNode *bucketNode = *node_it; //Get the node before it is destroied
            int aregOffset = k.getNumRegTotal();

            //Kill current node
            if (*node_it == vec.back())
            {
                vec.pop_back();
                node_it = vec.end();
            }
            else
            {
                //Current node is assigned with the last one
                //For caller, same iterator postion need be handled again,
                //Beause a new node is copied here
                *node_it = vec.back();
                vec.pop_back();
            }

            //Kill the same node in other bucket.
            SBFootprint *footprint = bucketNode->node->getFootprint(bucketNode->opndNum);
            while (footprint)
            {
                unsigned int startBucket = footprint->LeftB / G4_GRF_REG_NBYTES;
                unsigned int endBucket = footprint->RightB / G4_GRF_REG_NBYTES;
                if (footprint->fType == ACC_T)
                {
                    startBucket = startBucket + aregOffset;
                    endBucket = endBucket + aregOffset;
                }

                if (footprint->inst == bucketNode->inst)
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
                footprint = footprint->next;
            }
        }

        //Add a node into bucket
        void add(SBBucketNode *bucketNode, int bucket)
        {
            assert(nodeBucketsArray[bucket] != nullptr);
            SBBUCKET_VECTOR& nodeVec = *(nodeBucketsArray[bucket]);
            nodeVec.push_back(bucketNode);
        }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpLives()
        {
            for (int curBucket = 0; curBucket < numOfBuckets; curBucket++)
            {
                if (nodeBucketsArray[curBucket]->size())
                {
                    std::cerr << " GRF" << curBucket << ":";
                    for (auto it = nodeBucketsArray[curBucket]->begin(), ite = nodeBucketsArray[curBucket]->end(); it != ite; ++it)
                    {
                        SBBucketNode *liveBN = (*it);
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
    } SWSB_INDEXES;

    typedef struct _SWSB_LOOP {
        int entryBBID = -1 ;
        int endBBID = -1;
    } SWSB_LOOP;

    class G4_BB_SB {
    private:
        IR_Builder& builder;
        vISA::Mem_Manager &mem;
        G4_BB             *bb;
        G4_Label*         BBLabel;
        int nodeID;
        int ALUID;


        int totalGRFNum;

    public:
        LiveGRFBuckets *send_use_kills;
        BB_SWSB_LIST      Preds;
        BB_SWSB_LIST      Succs;

        BB_SWSB_LIST      domPreds;
        BB_SWSB_LIST      domSuccs;

        int first_node;
        int last_node;

        int first_send_node;
        int last_send_node;

        bool tokenAssigned;

        int send_start;
        int send_end;

        unsigned      loopStartBBID;    // The start BB ID of live range
        unsigned      loopEndBBID;    // The start BB ID of live range
        // send_live_in(BBi) = U(send_live_out(BBj)), BBj is the predecessor of BBi
        // send_live_out = (send_live_in - send_may_kill) + send_live_out
        // send_kill = send_live_in - send_live_out;
        SBBitSets *send_def_out;
        SBBitSets *send_live_in;
        SBBitSets *send_live_out;
        SBBitSets *send_may_kill;
        SBBitSets *send_live_in_scalar;
        SBBitSets *send_live_out_scalar;
        SBBitSets *send_kill_scalar;
        BitSet* send_WAW_may_kill;

        BitSet* dominators;

        //For token reduction
        BitSet   *liveInTokenNodes;
        BitSet   *liveOutTokenNodes;
        BitSet   *killedTokens;
        BitSet   **tokeNodesMap;
        unsigned    *tokenLiveInDist;
        unsigned    *tokenLiveOutDist;
        SBBitSets* localReachingSends;

        //BB local data dependence analysis
        G4_BB_SB(IR_Builder& b, Mem_Manager &m, G4_BB *block, SBNODE_VECT *SBNodes, SBNODE_VECT* SBSendNodes,
            SBBUCKET_VECTOR *globalSendOpndList,  SWSB_INDEXES *indexes, uint32_t &globalSendNum, LiveGRFBuckets *lb,
            LiveGRFBuckets *globalLB, PointsToAnalysis& p,
            std::map<G4_Label*, G4_BB_SB*> *LabelToBlockMap) : builder(b), mem(m), bb(block),
            first_node(-1), last_node(-1), send_start(-1), send_end(-1),
            send_live_in(nullptr), send_live_out(nullptr), send_may_kill(nullptr), send_live_in_scalar(nullptr), send_live_out_scalar(nullptr), send_kill_scalar(nullptr), send_WAW_may_kill(nullptr),
            liveInTokenNodes(nullptr), liveOutTokenNodes(nullptr), killedTokens(nullptr), tokeNodesMap(nullptr), loopStartBBID(-1), loopEndBBID(-1),
            send_def_out(nullptr), tokenAssigned(false), localReachingSends(nullptr)
        {
            first_send_node = -1;
            last_send_node = -1;
            totalGRFNum = block->getKernel().getNumRegTotal();
            SBDDD(bb, lb, globalLB, SBNodes, SBSendNodes, globalSendOpndList,  indexes, globalSendNum, p, LabelToBlockMap);
        }

        ~G4_BB_SB()
        {
            send_def_out->~SBBitSets();
            send_live_in->~SBBitSets();
            send_live_out->~SBBitSets();
            send_may_kill->~SBBitSets();
            send_live_in_scalar->~SBBitSets();
            send_live_out_scalar->~SBBitSets();
            send_kill_scalar->~SBBitSets();
            send_WAW_may_kill->~BitSet();
            liveInTokenNodes->~BitSet();
            liveOutTokenNodes->~BitSet();
            killedTokens->~BitSet();

            if (tokeNodesMap != nullptr)
            {
                for (uint32_t i = 0; i < builder.kernel.getNumSWSBTokens(); i++)
                {
                    tokeNodesMap[i]->~BitSet();
                }
            }
        }

        G4_BB* getBB() { return bb; }
        G4_Label* getLabel() { return BBLabel; }

        bool isGRFEdgeAdded(SBNode* pred, SBNode* succ, DepType d, SBDependenceAttr a);
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
        void getGRFBuckets(SBNode *node,
            SBFootprint* footprint,
            Gen4_Operand_Number opndNum,
            std::vector<SBBucketDescr>& BDvec);
        bool getFootprintForOperand(SBNode *node,
            G4_INST *inst,
            G4_Operand* opnd,
            Gen4_Operand_Number opnd_num);
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
            std::vector<SBBucketDescr>& BDvec,
            bool GRFOnly);

        bool getGRFFootPrint(SBNode *node,
            PointsToAnalysis &p);

        void getGRFBucketDescrs(SBNode *node,
            std::vector<SBBucketDescr>& BDvec,
            bool GRFOnly);

        void setDistance(SBFootprint * footprint, SBNode *node, SBNode *liveNode);
        void footprintMerge(SBNode * node, SBNode * nextNode);

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
        void getLiveBucketsFromFootprint(SBFootprint *firstFootprint, SBBucketNode* sBucketNode, LiveGRFBuckets *send_use_kills);
        void addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR *globalSendOpndList, SBNODE_VECT *SBNodes, PointsToAnalysis &p, bool afterWrite);
        void clearKilledBucketNodeGen12LP(LiveGRFBuckets * LB, int ALUID);



        void getLiveOutToken(unsigned allSendNum, SBNODE_VECT *SBNodes);


        unsigned getLoopStartBBID() { return loopStartBBID; }
        unsigned getLoopEndBBID() { return loopEndBBID; }

        void setLoopStartBBID(unsigned id) { loopStartBBID = id; }
        void setLoopEndBBID(unsigned id) { loopEndBBID = id; }

        void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

        void dumpLiveInfo(SBBUCKET_VECTOR *globalSendOpndList, unsigned globalSendNum, SBBitSets *send_kill);
    };

    typedef std::vector<G4_BB_SB *> BB_SWSB_VECTOR;
    typedef BB_SWSB_VECTOR::iterator BB_SWSB_VECTOR_ITER;
    typedef std::vector<SWSB_LOOP> LOOP_SWSB_VECTOR;
    typedef LOOP_SWSB_VECTOR::iterator LOOP_SWSB_VECTOR_ITER;

    class Dom
    {
    public:
        std::vector<G4_BB*> iDoms;
        Dom(G4_Kernel& k, vISA::Mem_Manager& m) : kernel(k), mem(m)
        {
        }

        ~Dom()
        {
        };

        std::unordered_set<G4_BB*>& getDom(G4_BB*);
        std::vector<G4_BB*>& getImmDom(G4_BB*);
        G4_BB* getCommonImmDom(std::unordered_set<G4_BB*>& bbs);
        void runDOM();
        G4_BB* InterSect(G4_BB* bb, int i, int k);
        void runIDOM();
        void dumpImmDom();

    private:
        vISA::Mem_Manager& mem;
        G4_Kernel& kernel;
        G4_BB* entryBB = nullptr;
        std::vector<std::unordered_set<G4_BB*>> Doms;
        std::vector<std::vector<G4_BB*>> immDoms;

        void updateImmDom();
    };

    class SWSB {
        G4_Kernel &kernel;
        FlowGraph&  fg;
        vISA::Mem_Manager& mem;

        BB_SWSB_VECTOR BBVector;    // The basic block vector, ordered with ID of the BB
        LOOP_SWSB_VECTOR loopVector;
        SBNODE_VECT SBNodes;        // All instruction nodes
        SBNODE_VECT SBSendNodes;    // All out-of-order instruction nodes
        SBNODE_VECT SBSendUses;    // All out-of-order instruction nodes
#ifdef DEBUG_VERBOSE_ON
        SBNODE_VECT globalSBNodes;        // All instruction nodes
#endif
        SWSB_INDEXES indexes;         // To pass ALU ID  from previous BB to current.
        uint32_t  globalSendNum;    // The number of out-of-order instructions which generate global dependencies.
        SBBUCKET_VECTOR globalSendOpndList;  //All send operands which live out their instructions' BBs. No redundant.
        uint32_t totalTokenNum;

        //For profiling
        uint32_t syncInstCount;
        uint32_t AWSyncInstCount;
        uint32_t ARSyncInstCount;
        uint32_t mathReuseCount;
        uint32_t ARSyncAllCount;
        uint32_t AWSyncAllCount;
        uint32_t tokenReuseCount;

        //Linear scan data structures for token allocation
        SBNODE_LIST linearScanLiveNodes;

        std::vector<SBNode *> freeTokenList;

        std::vector<SBNODE_VECT *> reachTokenArray;
        std::vector<SBNODE_VECT *> reachUseArray;
        SBNODE_VECT localTokenUsage;

        SBNODE_LIST sameTokenNodes[32];
        int topIndex;

        std::map<G4_Label*, G4_BB_SB*> labelToBlockMap;
        BitSet   **allTokenNodesMap;

        //Global dependence analysis
        bool globalDependenceDefReachAnalysis(G4_BB* bb);
        bool globalDependenceUseReachAnalysis(G4_BB* bb);
        void addGlobalDependence(unsigned globalSendNum, SBBUCKET_VECTOR *globalSendOpndList, SBNODE_VECT *SBNodes, PointsToAnalysis &p, bool afterWrite);
        void tokenEdgePrune(unsigned& prunedEdgeNum, unsigned& prunedGlobalEdgeNum, unsigned& prunedDiffBBEdgeNum, unsigned& prunedDiffBBSameTokenEdgeNum);
        void dumpTokenLiveInfo();

        void removePredsEdges(SBNode * node, SBNode * pred);

        void dumpImmDom(Dom* dom);

        void setDefaultDistanceAtFirstInstruction();

        //Token allocation
        void tokenAllocation();
        void buildLiveIntervals();
        void expireIntervals(unsigned startID);
        void addToLiveList(SBNode *node);

        unsigned short reuseTokenSelectionGlobal(SBNode* node, G4_BB* bb, SBNode*& candidateNode, bool& fromUse);
        void addReachingDefineSet(SBNode* node, SBBitSets* globalLiveSet, SBBitSets* localLiveSet);
        void addReachingUseSet(SBNode* node, SBNode* use);
        void addGlobalDependenceWithReachingDef(unsigned globalSendNum, SBBUCKET_VECTOR* globalSendOpndList, SBNODE_VECT* SBNodes, PointsToAnalysis& p, bool afterWrite);
        void expireLocalIntervals(unsigned startID, unsigned BBID);
        void assignTokenToPred(SBNode* node, SBNode* pred, G4_BB* bb);
        void assignTokenToSucc(SBNode* node, G4_BB* bb);
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
        bool insertSyncToken(G4_BB *bb, SBNode *node, G4_INST *inst, INST_LIST_ITER inst_it, int newInstID, BitSet *dstTokens, BitSet *srcTokens, bool removeAllTokens);
        void insertSync(G4_BB* bb, SBNode* node, G4_INST* inst, INST_LIST_ITER inst_it, int newInstID, BitSet* dstTokens, BitSet* srcTokens);
        void insertTest();

        //Insert sync instructions
        G4_INST *insertTestInstruction(G4_BB *bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo, bool countSyns);  //FIXME: Please remove it when meta is not needed anymore
        G4_INST *insertSyncInstruction(G4_BB *bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST * insertSyncInstructionAfter(G4_BB * bb, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST* insertSyncAllRDInstruction(G4_BB *bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo);
        G4_INST *insertSyncAllWRInstruction(G4_BB *bb, unsigned int SBIDs, INST_LIST_ITER nextIter, int CISAOff, int lineNo);

        void SWSBDepDistanceGenerator(PointsToAnalysis& p, LiveGRFBuckets &LB, LiveGRFBuckets &globalSendsLB);
        void SWSBGlobalTokenGenerator(PointsToAnalysis& p, LiveGRFBuckets &LB, LiveGRFBuckets &globalSendsLB);
        void SWSBBuildSIMDCFG();
        void addSIMDEdge(G4_BB_SB *pred, G4_BB_SB* succ);
        void SWSBGlobalScalarCFGReachAnalysis();
        void SWSBGlobalSIMDCFGReachAnalysis();

        void setTopTokenIndex();

        //Optimizations
        void tokenDepReduction(SBNode* node1, SBNode *node2);
        bool  cycleExpired(SBNode * node, int currentID);

        void shareToken(SBNode *node, SBNode *succ, unsigned short token);

        void SWSBGlobalTokenAnalysis();
        bool globalTokenReachAnalysis(G4_BB *bb);


        //Dump
        void dumpDepInfo();
        void dumpLiveIntervals();
        void dumpTokeAssignResult();
        void dumpSync(SBNode * tokenNode, SBNode * syncNode, unsigned short token, SWSBTokenType type);

        // Fast-composite support.
        void genSWSBPatchInfo();

        void getDominators(Dom* dom);


    public:
        SWSB(G4_Kernel &k, vISA::Mem_Manager& m)
            : kernel(k), fg(k.fg), mem(m)
        {
            globalSendNum = 0;
            syncInstCount = 0;
            mathReuseCount = 0;
            ARSyncInstCount = 0;
            AWSyncInstCount = 0;
            ARSyncAllCount = 0;
            AWSyncAllCount = 0;
            tokenReuseCount = 0;
            indexes.instIndex = 0;
            indexes.ALUIndex = 0;
            topIndex = -1;

            allTokenNodesMap = nullptr;
            totalTokenNum = fg.builder->kernel.getNumSWSBTokens();
        }
        ~SWSB()
        {
            for (SBNODE_VECT_ITER node_it = SBNodes.begin();
                node_it != SBNodes.end();
                node_it++)
            {
                SBNode *node = *node_it;
                node->~SBNode();
            }

            for (BB_SWSB_VECTOR_ITER bb_it = BBVector.begin();
                bb_it != BBVector.end();
                bb_it++)
            {
                G4_BB_SB *bb = *bb_it;
                bb->~G4_BB_SB();
            }
            if (allTokenNodesMap != nullptr)
            {
                for (size_t i = 0; i < fg.builder->kernel.getNumSWSBTokens(); i++)
                {
                    allTokenNodesMap[i]->~BitSet();
                }
            }
        }
        void SWSBGenerator();
        SBNode * reuseTokenSelection(SBNode * node);
        unsigned getDepDelay(SBNode *node);
    };
}
#endif // _SWSB_H_
