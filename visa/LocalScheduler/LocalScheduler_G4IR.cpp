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
#include "LocalScheduler_G4IR.h"
#include "Dependencies_G4IR.h"
#include "../G4_Opcode.h"
#include "../Timer.h"
#include "visa_wa.h"
#include <queue>

using namespace std;
using namespace vISA;

/* Entry to the local scheduling. */
void LocalScheduler::localScheduling()
{
    DEBUG_VERBOSE("[Scheduling]: Starting...");
    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    MUST_BE_TRUE(ib != bend, ERROR_SCHEDULER);

    CM_BB_INFO* bbInfo = (CM_BB_INFO *)mem.alloc(fg.size() * sizeof(CM_BB_INFO));
    memset(bbInfo, 0, fg.size() * sizeof(CM_BB_INFO));
    int i = 0;

    const Options *m_options = fg.builder->getOptions();
    LatencyTable LT(fg.builder);

    for (; ib != bend; ++ib)
    {
        unsigned instCountBefore = (uint32_t)(*ib)->size();
        #define SCH_THRESHOLD 2
        if (instCountBefore < SCH_THRESHOLD)
        {
            continue;
        }

        Mem_Manager bbMem(4096);
        unsigned schedulerWindowSize = m_options->getuInt32Option(vISA_SchedulerWindowSize);
        if (schedulerWindowSize > 0 && instCountBefore > schedulerWindowSize)
        {
            // If BB has a lot of instructions then when recursively
            // traversing DAG in list scheduler, stack overflow occurs.
            // So artificially breakup inst list here to reduce size
            // of scheduler problem size.
            unsigned int count = 0;
            std::vector<G4_BB*> sections;

            for (INST_LIST_ITER inst_it = (*ib)->begin();
                ;
                inst_it++)
            {
                if (count == schedulerWindowSize ||
                    inst_it == (*ib)->end())
                {
                    G4_BB* tempBB = fg.createNewBB(false);
                    sections.push_back(tempBB);
                    tempBB->splice(tempBB->begin(),
                        (*ib), (*ib)->begin(), inst_it);
                    G4_BB_Schedule schedule(fg.getKernel(), bbMem, tempBB, LT);
                    count = 0;
                }
                count++;

                if (inst_it == (*ib)->end())
                {
                    break;
                }
            }

            for (unsigned int i = 0; i < sections.size(); i++)
            {
                (*ib)->splice((*ib)->end(), sections[i], sections[i]->begin(), sections[i]->end());
            }
        }
        else
        {
            G4_BB_Schedule schedule(fg.getKernel(), bbMem, *ib, LT);
            bbInfo[i].id = (*ib)->getId();
            bbInfo[i].staticCycle = schedule.sequentialCycle;
            bbInfo[i].sendStallCycle = schedule.sendStallCycle;
            bbInfo[i].loopNestLevel = (*ib)->getNestLevel();
        }

        i++;
    }
    FINALIZER_INFO* jitInfo = fg.builder->getJitInfo();
    jitInfo->BBInfo = bbInfo;
    jitInfo->BBNum = i;
}

void G4_BB_Schedule::dumpSchedule(G4_BB *bb)
{
    const char *asmName = nullptr;
    getOptions()->getOption(VISA_AsmFileName, asmName);
    char dumpFileName[MAX_OPTION_STR_LENGTH];
    SNPRINTF(dumpFileName, MAX_OPTION_STR_LENGTH, "%s.bb%d.schedule",
        asmName, bb->getId());
    fstream ofile(dumpFileName, ios::out);
    auto nodeIT = scheduledNodes.begin();
    Node *finalNode = scheduledNodes.back();
    int lastBusyCycle = 0;
    int GAP = 4;
    for (int cycle = 0, cycle_e = finalNode->schedTime + finalNode->getOccupancy();
        cycle != cycle_e; ++cycle)
    {
        if (cycle == (*nodeIT)->schedTime)
        {
            int latency = (*nodeIT)->getOccupancy();
            assert(latency > 0);
            int externCycle = cycle;
            for (int cy_e = cycle + latency; cycle != cy_e; ++cycle)
            {
                lastBusyCycle = cycle;
                ofile << setw(4) << cycle << " ";
                for (G4_INST *inst : (*nodeIT)->instVec) {
                    ofile << setw(5) << G4_Inst_Table[inst->opcode()].str;
                }
                if (externCycle == cycle) {
                    ofile << "[" << (*nodeIT)->nodeID << "]";
                    const std::list<G4_INST *> *instrs
                        = (*nodeIT)->getInstructions();
                    if (instrs->empty()) {
                        ofile << "I" << 0;
                    } else {
                        for (G4_INST *instr : *instrs) {
                            ofile << "I" << instr->getLocalId();
                        }
                    }
                    ofile << "L" << (*nodeIT)->getOccupancy()
                        << "E" << (*nodeIT)->getEarliest()
                        << "P" << (*nodeIT)->priority
                        << " ";
                    for (G4_INST *instr : *instrs) {
                        ofile << *instr << ", ";
                    }
                } else {
                    ofile << " ... ";
                }
                ofile << std::endl;
            }
            cycle -= 1; // Since the external for loop will ++ anyway
            ++nodeIT;
        }
        else
        {
            int nextBusyCycle = (*nodeIT)->schedTime;
            if (cycle > lastBusyCycle + GAP
                && cycle < nextBusyCycle - GAP)
            {
                ofile << "+++++++++++ "
                    << nextBusyCycle - (lastBusyCycle + 1) << " empty cycles "
                    << lastBusyCycle + 1 << "-" << nextBusyCycle
                    << " +++++++++++\n";
                cycle = nextBusyCycle - (GAP + 1);
            }
            else
            {
                ofile << setw(4) << cycle << " " << std::endl;
            }
        }
    }
    ofile.close();
}


/************************************************************************/
/* G4Sched::G4_BB_Schedule                                             */
/************************************************************************/

//
//  G4_BB_Schedule constructor:
//      - creates dependence DAG (can emit DAG by enabling a macro in DDD::DDD);
//      - re-schedules the code;
//      - dumps the DAG (optional)
//      - creates a new instruction listing within a BBB
//
G4_BB_Schedule::G4_BB_Schedule(G4_Kernel* k, Mem_Manager& m, G4_BB* block,
    const LatencyTable& LT)
    : mem(m)
    , bb(block)
    , kernel(k)

{
    // we use local id in the scheduler for determining two instructions' original ordering
    bb->resetLocalId();

    DDD ddd(mem, bb, LT, k);
    // Generate pairs of TypedWrites
    bool doMessageFuse = (k->fg.builder->fuseTypedWrites() && k->getSimdSize() >= 16) ||
        k->fg.builder->fuseURBMessage();

    if (doMessageFuse)
    {
        ddd.pairTypedWriteOrURBWriteNodes(bb);
    }

    lastCycle = ddd.listSchedule(this);

    if (getOptions()->getOption(vISA_DumpSchedule))
    {
        dumpSchedule(bb);
    }
    if (getOptions()->getOption(vISA_DumpDot))
    {
        std::stringstream sstr;
        sstr << "BB" << bb->getId();
        ddd.DumpDotFile(sstr.str().c_str(), "nodes");
    }

    // Update the listing of the basic block with the reordered code.
    INST_LIST_ITER inst_it = bb->begin();
    Node *prevNode = nullptr;
    unsigned HWThreadsPerEU = getBuilder()->getHWThreadNumberPerEU();
    size_t scheduleInstSize = 0;
    for (Node *currNode : scheduledNodes) {
        for (G4_INST *inst : *currNode->getInstructions()) {
            (*inst_it) = inst;
            ++scheduleInstSize;
            if (prevNode && !prevNode->isLabel()) {
                int32_t stallCycle = (int32_t)currNode->schedTime - (int32_t)prevNode->schedTime;
                if (stallCycle > 0 && stallCycle > int32_t(prevNode->getOccupancy() * HWThreadsPerEU)) {
                    sendStallCycle += (stallCycle + HWThreadsPerEU - 1) / HWThreadsPerEU;
                    sequentialCycle += (stallCycle + HWThreadsPerEU - 1) / HWThreadsPerEU;
                }
            }
            sequentialCycle += currNode->getOccupancy();
            prevNode = currNode;
            inst_it++;
        }
    }

    assert(scheduleInstSize == bb->size() &&
           "Size of inst list is different before/after scheduling");
}

void Node::dump() {
    std::cerr << "Id:" << nodeID
        << " Prio:" << priority
        << " Earl:" << earliest
        << " Occu:" << occupancy
        << " ";
    for (G4_INST *inst : instVec) {
        inst->dump();
    }
    std::cerr << "SchedTime: " << schedTime << "\n";
    std::cerr << "predsNotScheduled: " << predsNotScheduled << "\n";
    std::cerr << "Preds: ";
    for (const Edge &predE : preds) {
        std::cerr << predE.getNode()->nodeID << ", ";
    }
    std::cerr << "\n";

    std::cerr << "Succs: ";
    for (const Edge &succE : succs) {
        std::cerr << succE.getNode()->nodeID << ", ";
    }
    std::cerr << "\n";
}

// Compute the range of registers touched by OPND.
static Mask getMaskForOp(G4_Operand * opnd, Gen4_Operand_Number opnd_num,
    unsigned execSize)
{
    unsigned short LB, RB;
    bool nonContiguousStride = false;

    auto getFlagBounds = [&](G4_Operand *opnd) {
        const unsigned BITS_PER_FLAG_REG = 16;
        bool valid = true;
        unsigned subRegOff = opnd->getBase()->ExSubRegNum(valid);
        LB = (unsigned short)(opnd->getLeftBound() + subRegOff * BITS_PER_FLAG_REG);
        RB = (unsigned short)(opnd->getRightBound() + subRegOff * BITS_PER_FLAG_REG);
    };

    switch (opnd_num) {
    case Opnd_src0:
    case Opnd_src1:
    case Opnd_src2:
    case Opnd_src3:
    case Opnd_pred:
    case Opnd_implAccSrc: {
        if (opnd->isFlag())
        {
            getFlagBounds(opnd);
        }
        else
        {
            LB = (unsigned short)opnd->getLinearizedStart();
            RB = (unsigned short)opnd->getLinearizedEnd();
            G4_SrcRegRegion *srcOpnd = opnd->asSrcRegRegion();
            RegionDesc *rd = srcOpnd->getRegion();
            nonContiguousStride = !rd->isContiguous(execSize);
        }
        break;
    }
    case Opnd_dst:
    case Opnd_condMod:
    case Opnd_implAccDst: {
        if (opnd->isFlag())
        {
            getFlagBounds(opnd);
        }
        else
        {
            LB = (unsigned short)opnd->getLinearizedStart();
            RB = (unsigned short)opnd->getLinearizedEnd();
            G4_DstRegRegion *dstOpnd = opnd->asDstRegRegion();
            nonContiguousStride = (execSize != 1
                && dstOpnd->getHorzStride() != 1);
        }
        break;
    }
    default:
        assert(0 && "Bad opnd");
    }
    return Mask(LB, RB, nonContiguousStride, opnd->getAccRegSel());
}

void DDD::getBucketsForOperand(G4_INST* inst, Gen4_Operand_Number opnd_num,
    G4_Operand* opnd,
    std::vector<BucketDescr>& BDvec)
{
    if (opnd->isLabel() || opnd->isImm()) {
        return;
    }
#define UNINIT_BUCKET -1
    int startingBucket = UNINIT_BUCKET;
    G4_VarBase* base = opnd->getBase();
    bool canSpanMultipleBuckets = false;
    assert(base && "If no base, then the operand is not touched by the instr.");

    // If a register allocated regvar, then get the physical register
    G4_VarBase *phyReg = (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

    switch (phyReg->getKind()) {
    case G4_VarBase::VK_phyGReg:
        startingBucket = opnd->getLinearizedStart() / G4_GRF_REG_NBYTES;
        canSpanMultipleBuckets = true;
        break;
    case G4_VarBase::VK_phyAReg: {
        G4_Areg *Areg = (G4_Areg *)phyReg;
        switch (Areg->getArchRegType()) {
        case AREG_NULL:
            break;
        case AREG_A0:
            startingBucket = A0_BUCKET;
            break;
        case AREG_F0:
            startingBucket = FLAG0_BUCKET;
            break;
        case AREG_F1:
            startingBucket = FLAG1_BUCKET;
            break;
        case AREG_ACC0:
        case AREG_ACC1:
            startingBucket = ACC_BUCKET;
            break;
        default:
            startingBucket = OTHER_ARF_BUCKET;
            break;
        }
        break;
    }
    case G4_VarBase::VK_regVar:
        assert(0 && "Should not be a regvar. PhyReg is extracted from regvar.");
        break;
    default:
        assert(0 && "Bad kind");
        break;
    }

    // Create one or more buckets and push them into the vector
    if (startingBucket != UNINIT_BUCKET) {
        if (canSpanMultipleBuckets) {
            assert(base->isGreg());
            unsigned divisor = G4_GRF_REG_NBYTES;
            int baseBucket = GRF_BUCKET;
            int endingBucket = baseBucket + opnd->getLinearizedEnd() / divisor;
            MUST_BE_TRUE(endingBucket >= startingBucket, "Ending bucket less than starting bucket");
            Mask mask = getMaskForOp(opnd, opnd_num, inst->getExecSize());
            int numBuckets = endingBucket - startingBucket + 1;
            for (int j = startingBucket;
                j < (startingBucket + numBuckets); j++) {
                BDvec.push_back(BucketDescr(j, mask, opnd_num));
            }
            // If this operand is a non-trivial special ACC operand, add
            // it to the other ARF bucket for tacking extra dependencies.
            G4_AccRegSel Acc = opnd->getAccRegSel();
            if (Acc != G4_AccRegSel::ACC_UNDEFINED && Acc != G4_AccRegSel::NOACC)
                BDvec.push_back(BucketDescr(OTHER_ARF_BUCKET, mask, opnd_num));
        } else {
            Mask mask = getMaskForOp(opnd, opnd_num, inst->getExecSize());
            BDvec.push_back(BucketDescr(startingBucket, mask, opnd_num));
        }
    }
}

// Return TRUE if opnd corresponding to opndNum has indirect access.
static inline bool hasIndirection(G4_Operand *opnd, Gen4_Operand_Number opndNum) {
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

// Given an inst with physical register assignment,
// return all bucket descriptors that the physical register can map
// to. This requires taking in to account exec size, data
// type, and whether inst is a send
bool DDD::getBucketDescrs(Node *node, std::vector<BucketDescr>& BDvec)
{
    bool hasIndir = false;
    for (G4_INST *inst : node->instVec)
    {
        // Iterate over all operands and create buckets.
        for (Gen4_Operand_Number opndNum
            : {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2, Opnd_src3,
            Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst}) {
            G4_Operand *opnd = inst->getOperand(opndNum);
            // Skip if no operand or the operand is not touched by the instruction
            if (!opnd || !opnd->getBase()) {
                continue;
            }
            // FIXME: This is to emulate the original code. Not sure if it is OK
            if (opndNum == Opnd_src3 && inst->isSplitSend()) {
                getBucketsForOperand(inst, Opnd_src2, opnd, BDvec);
            }
            else {
                getBucketsForOperand(inst, opndNum, opnd, BDvec);
            }
            // Check if this operand is an indirect access
            hasIndir |= hasIndirection(opnd, opndNum);
        }

        // Sends need an additional bucket
        if (inst->isSend()) {
            if (inst->getMsgDesc()->isScratchRW()) {
                BDvec.push_back(BucketDescr(SCRATCH_SEND_BUCKET, Mask(), Opnd_dst));
            }
            else {
                BDvec.push_back(BucketDescr(SEND_BUCKET, Mask(), Opnd_dst));
            }
        }
    }

    return hasIndir;
}

// This class hides the internals of dependence tracking using buckets
class LiveBuckets
{
    std::vector<BucketHeadNode> nodeBucketsArray;
    DDD *ddd;
    int firstBucket;
    int numOfBuckets;
    friend class BN_iterator;
    static const bool ALL_BUCKETS = true;

public:
    class BN_iterator
    {
    public:
        const LiveBuckets *LB;
        BUCKET_VECTOR_ITER node_it;
        int bucket;
        bool iterateAll;

        BN_iterator(const LiveBuckets *LB1, BUCKET_VECTOR_ITER It, int Bucket,
            bool All)
            : LB(LB1), node_it(It), bucket(Bucket), iterateAll(All) {
            ;
        }


        void skipEmptyBuckets() {
            // If at the end of the node vector, move to next bucket
            // Keep going until a non-empty vector is found
            while (bucket < LB->numOfBuckets
                && node_it == LB->nodeBucketsArray[bucket].bucketVec->end())
            {
                bucket++;
                if (bucket < LB->numOfBuckets)
                {
                    node_it = LB->nodeBucketsArray[bucket].bucketVec->begin();
                }
            }
        }

        // The iterator supports 2 modes of operation:
        // 1) Iterate across the vector nodes of a single bucket
        // 2) Iterate across all buckets and all vector nodes of each bucket
        BN_iterator &operator++() {
            // Mode 1
            if (iterateAll == LiveBuckets::ALL_BUCKETS)
            {
                auto node_ite = LB->nodeBucketsArray[bucket].bucketVec->end();
                // Increment node vector iterator
                if (node_it != node_ite)
                {
                    ++node_it;
                }
                skipEmptyBuckets();
            }
            // Mode 2
            else
            {
                ++node_it;
            }
            assert(!iterateAll
                || bucket == LB->numOfBuckets
                || node_it != LB->nodeBucketsArray[bucket].bucketVec->end());
            return *this;
        }
        bool operator==(const BN_iterator &it2) {
            assert(LB == it2.LB && iterateAll == it2.iterateAll);
            // NOTE: order of comparisons matters: if different buckets
            //       then node_its are of different vectors
            return (bucket == it2.bucket && node_it == it2.node_it);
        }
        bool operator!=(const BN_iterator &it2) {
            assert(LB == it2.LB && iterateAll == it2.iterateAll);
            return (!(*this == it2));
        }
        BucketNode *operator*() {
            assert(node_it != LB->nodeBucketsArray[bucket].bucketVec->end());
            return *node_it;
        }
    };

    LiveBuckets(DDD *Ddd, int GRF_BUCKET, int TOTAL_BUCKETS) {
        firstBucket = GRF_BUCKET;
        numOfBuckets = TOTAL_BUCKETS;
        ddd = Ddd;
        nodeBucketsArray.resize(numOfBuckets);

        // Initialize a vector for each bucket
        for (int bucket_i = 0; bucket_i != (int)numOfBuckets; ++bucket_i)
        {
            void* allocedMem = ddd->get_mem()->alloc(sizeof(BUCKET_VECTOR));
            nodeBucketsArray[bucket_i].bucketVec
                = new (allocedMem)BUCKET_VECTOR();
        }
    }

    ~LiveBuckets() {
        for (int i = 0; i < numOfBuckets; i++) {
            BucketHeadNode &BHN = nodeBucketsArray[i];
            if (BHN.bucketVec) {
                BHN.bucketVec->~BUCKET_VECTOR();
            }
        }
    }

    // Mode 1: Iterate across nodes in BUCKET
    BN_iterator begin(int bucket) const {
        return BN_iterator(this, nodeBucketsArray[bucket].bucketVec->begin(),
            bucket, !ALL_BUCKETS);
    }

    // Mode 1:
    BN_iterator end(int bucket) const {
        return BN_iterator(this, nodeBucketsArray[bucket].bucketVec->end(),
            bucket, !ALL_BUCKETS);
    }

    // Mode 2: Iterate across all nodes and all buckets
    BN_iterator begin() const {
        auto it = BN_iterator(this,
            nodeBucketsArray[firstBucket].bucketVec->begin(),
            firstBucket, ALL_BUCKETS);
        it.skipEmptyBuckets();
        return it;
    }

    // Mode 2:
    BN_iterator end() const {
        return BN_iterator(this,
            nodeBucketsArray[numOfBuckets - 1].bucketVec->end(),
            numOfBuckets, ALL_BUCKETS);
    }

    void clearLive(int bucket) {
        BucketHeadNode &BHNode = nodeBucketsArray[bucket];
        BHNode.bucketVec->clear();
    }

    void clearAllLive() {
        for (int bucket_i = 0; bucket_i < numOfBuckets; ++bucket_i) {
            clearLive(bucket_i);
        }
    }

    bool hasLive(const Mask &mask, int bucket) {
        BucketHeadNode &BHNode = nodeBucketsArray[bucket];
        auto BV = BHNode.bucketVec;
        assert(BV != nullptr && "vectors not initialized?");
        return (!BV->empty());
    }

    void kill(Mask mask, BN_iterator &bn_it) {
        BucketHeadNode &BHNode = nodeBucketsArray[bn_it.bucket];
        BUCKET_VECTOR &vec = *BHNode.bucketVec;
        BUCKET_VECTOR_ITER &node_it = bn_it.node_it;
        if (*node_it == vec.back()) {
            vec.pop_back();
            node_it = vec.end();
        } else {
            *node_it = vec.back();
            vec.pop_back();
        }
    }

    // Create a bucket node for NODE using the information in BD
    // and append it to the list of live nodes.
    void add(Node *node, const BucketDescr &BD) {
        BucketHeadNode &BHNode = nodeBucketsArray[BD.bucket];
        // Append the bucket node to the vector hanging from the header
        assert(BHNode.bucketVec != nullptr);
        BUCKET_VECTOR& nodeVec = *(BHNode.bucketVec);
        void *allocedMem = ddd->get_mem()->alloc(sizeof(BucketNode));
        BucketNode *newNode = new(allocedMem)BucketNode(node, BD.mask, BD.operand);
        nodeVec.push_back(newNode);
        // If it is a write to a subreg, mark the NODE accordingly
        if (BD.operand == Opnd_dst) {
            node->setWritesToSubreg(BD.bucket);
        }
    }
};

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


// Construct data dependencey DAG. The function constructs
// DAG using a bucket-based algorithm. The idea is to setup
// as many buckets as there are GRFs (and ARFs). Then when
// testing dependencies the DAG, we only need to look at buckets
// that current inst may fall into. This saves us from checking
// dependencies with all insts in live set. After analyzing
// dependencies and creating necessary edges, current inst
// is inserted in all buckets it touches.
DDD::DDD(Mem_Manager& m, G4_BB* bb, const LatencyTable& lt, G4_Kernel* k)
    : mem(m)
    , LT(lt)
    , kernel(k)
{
    Node* lastBarrier = nullptr;
    HWthreadsPerEU = getBuilder()->getHWThreadNumberPerEU();
    useMTLatencies = getBuilder()->useMultiThreadLatency();
    totalGRFNum = kernel->getNumRegTotal();
    bool BTIIsRestrict = getOptions()->getOption(vISA_ReorderDPSendToDifferentBti);

    GRF_BUCKET = 0;
    ACC_BUCKET = GRF_BUCKET + totalGRFNum;
    FLAG0_BUCKET = ACC_BUCKET + 1;
    FLAG1_BUCKET = FLAG0_BUCKET + 1;
    A0_BUCKET = FLAG1_BUCKET + 1;
    SEND_BUCKET = A0_BUCKET + 1;
    SCRATCH_SEND_BUCKET = SEND_BUCKET + 1;
    OTHER_ARF_BUCKET = SCRATCH_SEND_BUCKET + 1;
    TOTAL_BUCKETS = OTHER_ARF_BUCKET + 1;

    LiveBuckets LB(this, GRF_BUCKET, TOTAL_BUCKETS);

    // Building the graph in reverse relative to the original instruction
    // order, to naturally take care of the liveness of operands.
    std::list<G4_INST*>::reverse_iterator iInst(bb->rbegin()), iInstEnd(bb->rend());
    std::vector<BucketDescr> BDvec;

    for (int nodeId = (int)(bb->size() - 1); iInst != iInstEnd; ++iInst, nodeId--)
    {
        Node *node = nullptr;
        // If we have a pair of instructions to be mapped on a single DAG node:
        node = new (mem)Node(nodeId, *iInst, depEdgeAllocator, LT);
        allNodes.push_back(node);
        G4_INST *curInst = node->getInstructions()->front();
        bool hasIndir = false;
        BDvec.clear();

        // Get buckets for all physical registers assigned in curInst
        hasIndir = getBucketDescrs(node, BDvec);
        if (hasIndir || (curInst->isSend() && curInst->asSendInst()->isFence()))
        {
            // If inst has indirect src/dst then treat it as a barrier.
            node->MarkAsUnresolvedIndirAddressBarrier();
        }

        DepType  depType;
        if ((depType = node->isLabel()) || (depType = node->isBarrier()))
        {
            // Insert edge from current instruction
            // to all instructions live in every bucket
            for (auto it = LB.begin(), ite = LB.end(); it != ite; ++it) {
                BucketNode *BNode = *it;
                Node* liveNode = BNode->node;
                if (liveNode->preds.empty())
                {
                    createAddEdge(node, liveNode, depType);
                }
            }
            LB.clearAllLive();
            if (lastBarrier)
            {
                createAddEdge(node, lastBarrier, lastBarrier->isBarrier());
            }

            lastBarrier = node;
        } else {
            // Compute buckets for RAW
            bool transitiveEdgeToBarrier = false;

            // For all bucket descriptors of curInst
            for (const BucketDescr &BD : BDvec) {
                const int &curBucket = BD.bucket;
                const Gen4_Operand_Number &curOpnd = BD.operand;
                const Mask &curMask = BD.mask;
                if (!LB.hasLive(curMask, curBucket)) {
                    continue;
                }
                // Kill type 1: When the current destination region completely
                //              covers the whole register from the first bit
                //              to the last bit.
                bool curKillsBucket = curMask.killsBucket(curBucket);

                // For each live curBucket node:
                // i)  create edge if required
                // ii) kill bucket node if required
                for (LiveBuckets::BN_iterator bn_it = LB.begin(curBucket);
                    bn_it != LB.end(curBucket);) {
                    BucketNode *liveBN = (*bn_it);
                    Node *curLiveNode = liveBN->node;
                    Gen4_Operand_Number liveOpnd = liveBN->opndNum;
                    Mask &liveMask = liveBN->mask;

                    G4_INST *liveInst = *curLiveNode->getInstructions()->begin();
                    // Kill type 2: When the current destination region covers
                    //              the live node's region completely.
                    bool curKillsLive = curMask.kills(liveMask);
                    bool hasOverlap = curMask.hasOverlap(liveMask);

                    // 1. Find DEP type
                    DepType dep = DEPTYPE_MAX;
                    if (curBucket < ACC_BUCKET) {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                    } else if (curBucket == ACC_BUCKET
                        || curBucket == A0_BUCKET) {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                        curKillsBucket = false;
                    } else if (curBucket == SEND_BUCKET) {
                        dep = getDepSend(curInst, liveInst, getOptions(), BTIIsRestrict);
                        hasOverlap = (dep != NODEP);
                        curKillsBucket = false;
                        curKillsLive = (dep == WAW_MEMORY || dep == RAW_MEMORY);
                    } else if (curBucket == SCRATCH_SEND_BUCKET) {
                        dep = getDepScratchSend(curInst, liveInst);
                        hasOverlap = (dep != NODEP);
                        curKillsBucket = false;
                        curKillsLive = false; // Disable kill
                    } else if (curBucket == FLAG0_BUCKET
                        || curBucket == FLAG1_BUCKET) {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                        curKillsBucket = false;
                    } else if (curBucket == OTHER_ARF_BUCKET) {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                        hasOverlap = (dep != NODEP); // Let's be conservative
                        curKillsBucket = false;
                    } else {
                        assert(0 && "Bad bucket");
                    }

                    // 2. Create Edge if there is overlap and RAW/WAW/WAR
                    if (dep != NODEP && hasOverlap) {
                        createAddEdge(node, curLiveNode, dep);
                        transitiveEdgeToBarrier
                            |= curLiveNode->hasTransitiveEdgeToBarrier;
                    }

                    // 3. Kill if required
                    if ((dep == RAW || dep == RAW_MEMORY
                        || dep == WAW || dep == WAW_MEMORY)
                        && (curKillsBucket || curKillsLive)) {
                        LB.kill(curMask, bn_it);
                        continue;
                    }
                    assert(dep != DEPTYPE_MAX && "dep unassigned?");
                    ++bn_it;
                }
            }

            if (transitiveEdgeToBarrier == false && lastBarrier != nullptr)
            {
                // Insert edge to barrier and set flag
                createAddEdge(node, lastBarrier, lastBarrier->isBarrier());
                node->hasTransitiveEdgeToBarrier = true;
            }
        }

        // Add buckets of current instruction to bucket list
        for (const BucketDescr &BD : BDvec)
        {
            LB.add(node, BD);
        }

        // Insert this node into the graph.
        InsertNode(node);
    }
}

void Node::deletePred(Node* pred)
{
    for (int i = 0, size = (int)preds.size(); i < size; ++i)
    {
        auto&& edge = preds[i];
        if (edge.getNode() == pred)
        {
            if (i != size - 1)
            {
                preds[i] = preds.back();
            }
            preds.pop_back();
            predsNotScheduled--;
            return;
        }
    }
    assert(false && "trying to delete a non-predecessor node");
}

// Move all input and output dependences:
// from:  ...->fromNode->...
// to  :  ...->toNode->...
void DDD::moveDeps(Node *fromNode, Node *toNode)
{

    // 1. Move outgoing edges:
    //    from: fromNode->...
    //      to: toNode->...
    for (size_t i = 0, succEmax = fromNode->succs.size(); i != succEmax; ++i)
    {
        const Edge &succE = fromNode->succs[i];
        Node *destNode = succE.getNode();
        DepType type = succE.getType();
        createAddEdge(toNode, destNode, type);
        destNode->deletePred(fromNode);
    }
    fromNode->succs.clear();

    // 2. Move incoming edges:
    //    from: ...->fromNode
    //      to: ...->toNode
    for (size_t i = 0, predEmax = fromNode->preds.size(); i != predEmax; ++i)
    {
        const Edge &predE = fromNode->preds[i];
        // predFromNode->fromNode
        Node *predFromNode = predE.getNode();
        int maxI = (int)predFromNode->succs.size();

        // move all edges predFromNode->fromNode
        //            to: predFromNode->toNode
        // a) add new edges predFromNode->toNode
        if (predFromNode != toNode)
        {
            for (int i = 0; i != maxI; ++i) {
                Edge &edge = predFromNode->succs[i];
                DepType type = edge.getType();
                if (edge.getNode() == fromNode)
                {
                    createAddEdge(predFromNode, toNode, type);
                }
            }
        }
        // b) delete old edges predFromNode->fromNode
        //    no need to check after maxI as all those edges are new ones
        for (int i = 0; i != maxI;) {
            Edge &edge = predFromNode->succs[i];
            bool keep_iter = false;
            if (edge.getNode() == fromNode) {
                // Erase edge from predFromNode->succs[].
                // We use the swap with last element trick.
                predFromNode->succs[i] = predFromNode->succs.back();
                predFromNode->succs.pop_back();
                maxI--;
                keep_iter = true;
            }
            if (!keep_iter) {
                i++;
            }
        }
    }
    fromNode->preds.clear();
    fromNode->schedTime = 0;
}


// Return TRUE if there is no dependence chain connecting
// firstNode->...->secondNode
// That would result in a dependence cycle after pairing.
// Note that firstNode->secondNode is ok if there are no instructions in between
static bool canAvoidDepCycles(Node *firstNode, Node *secondNode, bool isFirstLevel)
{
    // assumption is firstNode is close to secondNode so this will be fast
    // and we don't need to cache intermediate nodes
    for (const Edge& succE : firstNode->succs)
    {
        auto&& node = succE.getNode();
        if (node == secondNode)
        {
            if (isFirstLevel)
            {
                continue;
            }
            return false;
        }

        if (node->getInstructions()->back()->getLocalId() > secondNode->getInstructions()->back()->getLocalId())
        {
            // stop if we've reached beyond the second node
            continue;
        }
        if (!canAvoidDepCycles(node, secondNode, false))
        {
            return false;
        }
    }
    return true;
}

// Return TRUE if INST is the the partN'th part {0,1,2,3} of a typedWrite
static bool isTypedWritePart(G4_INST* inst, int partN) {
    return inst->isSend()
        && inst->getExecSize() == 8
        && inst->getMsgDesc()->isHdcTypedSurfaceWrite()
        && inst->getMaskOffset() == partN * 8;
};

struct URBDesc
{
    uint32_t opcode : 4;
    uint32_t offset : 11;   // unit of 16 bytes
    uint32_t maskPresent : 1;
    uint32_t ignored : 1;
    uint32_t perSlotPresent : 1;
    uint32_t dontcare : 14;
};

union DescData {
    uint32_t value;
    URBDesc layout;
};

// leading URB write should have cache-aligned global offset
static bool isLeadingURBWrite(G4_INST* inst)
{
    if (inst->isSend() && inst->getMsgDesc()->getFuncId() == SFID::URB)
    {
        DescData desc;
        desc.value = inst->getMsgDesc()->getDesc();
        // ToDo: add support for per-slot offset and channel mask later if necessary
        if (desc.layout.opcode == URB_SIMD8_WRITE && !desc.layout.perSlotPresent &&
            !desc.layout.maskPresent && desc.layout.offset % 4 == 0)
        {
            return true;
        }
    }
    return false;
}

// precondition: both inst are URB messages
static bool canFuseURB(G4_INST* secondURB, G4_INST* firstURB)
{
    DescData firstDesc, secondDesc;
    firstDesc.value = firstURB->getMsgDesc()->getDesc();
    secondDesc.value = secondURB->getMsgDesc()->getDesc();
    if (firstDesc.layout.opcode == secondDesc.layout.opcode &&
        firstDesc.layout.maskPresent == secondDesc.layout.maskPresent &&
        firstDesc.layout.perSlotPresent == secondDesc.layout.maskPresent &&
        firstDesc.layout.offset + 2 == secondDesc.layout.offset)
    {
        if (firstURB->getMsgDesc()->MessageLength() == secondURB->getMsgDesc()->MessageLength() &&
            firstURB->getMsgDesc()->extMessageLength() == secondURB->getMsgDesc()->extMessageLength())
        {
            return true;
        }
    }

    return false;
}

// Join nodes that need to be paired into fat nodes.
void DDD::pairTypedWriteOrURBWriteNodes(G4_BB *bb) {
    // 1. Scan through the code for pairs
    //    A pair {A, B}  is set like this: instrPairs[A] = B
    //                                     instrPairs[B] = A
    instrPairVec_t instrPairs;
    // Go through the instructions in BB and find all possible pairs of typedWrites.
    Node *foundFirst = nullptr, *foundSecond = nullptr;
    Node *foundThird = nullptr, *foundFourth = nullptr;

    for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it)
    {
        Node *node = *it;
        G4_INST *inst = (*node->getInstructions()).front();
        // {0,1}
        if (!foundFirst && isTypedWritePart(inst, 0)) {
            foundFirst = node;
        }
        if (foundFirst  && isTypedWritePart(inst, 1)) {
            foundSecond = node;
            instrPairs.emplace_back(DDD::instrPair_t(foundFirst, foundSecond));
            foundFirst = nullptr;
            foundSecond = nullptr;
        }
        // {2,3}
        if (!foundThird && isTypedWritePart(inst, 2)) {
            foundThird = node;
        }
        if (foundThird && isTypedWritePart(inst, 3)) {
            foundFourth = node;
            instrPairs.emplace_back(DDD::instrPair_t(foundThird, foundFourth));
            foundThird = nullptr;
            foundFourth = nullptr;
        }
    }

    Node* leadingURB = nullptr;
    for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it)
    {
        Node *node = *it;
        G4_INST *inst = (*node->getInstructions()).front();
        if (!leadingURB && isLeadingURBWrite(inst))
        {
            leadingURB = node;
        }
        else if (leadingURB)
        {
            if (inst->isSend() && inst->getMsgDesc()->getFuncId() == SFID::URB)
            {
                if (canFuseURB(inst, (*leadingURB->getInstructions()).front()))
                {
                    instrPairs.emplace_back(DDD::instrPair_t(leadingURB, node));
                    leadingURB = nullptr;
                }
                else
                {
                    leadingURB = isLeadingURBWrite(inst) ? node : nullptr;
                }
            }
            else
            {
                // stop if this instruction depends on leadingURB
                // it's a conservative way to avoid cycles in the DAG when
                // fusing two URB sends (e.g., second URB's payload share register with first)
                Node *leadingURBNode = leadingURB;
                Node *curNode = node;
                for (auto&& succ : leadingURBNode->succs)
                {
                    if (succ.getNode() == curNode)
                    {
                        leadingURB = nullptr;
                    }
                }
            }
        }
    }

    // 2. Join nodes that need pairing
    for (auto&& pair : instrPairs) {
        Node *firstNode = pair.first;
        Node *secondNode = pair.second;
        G4_INST *firstInstr = (*firstNode->getInstructions()).front();
        G4_INST *secondInstr = (*secondNode->getInstructions()).front();
        assert(firstNode->getInstructions()->size() == 1);
        assert(secondNode->getInstructions()->size() == 1);
        assert(*firstNode->getInstructions()->begin() == firstInstr);
        assert(*secondNode->getInstructions()->begin() == secondInstr);
        if (canAvoidDepCycles(firstNode, secondNode, true))
        {
            // A. move the deps of seconde node to the first.
            moveDeps(secondNode, firstNode);

            // B. We add the second instruction to the first node.
            assert(firstNode->getInstructions()->size() == 1);
            firstNode->addPairInstr(*secondNode->getInstructions()->begin());
            if (!kernel->fg.builder->getOption(vISA_NoAtomicSend) &&
                firstInstr->isSend() && firstInstr->getMsgDesc()->getFuncId() == SFID::URB)
            {
                firstInstr->setOptionOn(InstOpt_Atomic);
            }

            // C. Cleanup the paired node.
            secondNode->clear();
        }
    }
}

void DDD::collectRoots()
{
    Roots.clear();
    for (auto N : allNodes) {
        if (N->preds.empty() && !N->getInstructions()->empty()) {
            Roots.push_back(N);
       }
    }
}

void DDD::setPriority(Node *pred, const Edge &edge)
{
    // Calculate PRED's priority (pred->priority), based on SUCC's priority
    Node *succ = edge.getNode();
    assert(succ->priority != Node::PRIORITY_UNINIT && "succ node has no priority?");
    int newPriority = succ->priority + edge.getLatency();
    pred->priority = (newPriority > pred->priority) ? newPriority : pred->priority;
}

// Create a new edge PRED->SUCC of type D.
// The edge latency is also attached.
void DDD::createAddEdge(Node* pred, Node* succ, DepType d)
{
    // Check whether an edge already exists
    for (int i = 0; i < (int)(pred->succs.size()); i++)
    {
        Edge& curSucc = pred->succs[i];
        // Keep the deptype that has the highest latency
        if (curSucc.getNode() == succ)
        {
            uint32_t newEdgeLatency = getEdgeLatency(pred, d);
            if (newEdgeLatency > curSucc.getLatency())
            {
                // Update with the dep type that causes the highest latency
                curSucc.setType(d);
                curSucc.setLatency(newEdgeLatency);
                // Set the node priority
                setPriority(pred, curSucc);
            }
            return;
        }
    }

    // No edge with the same successor exists. Append this edge.
    uint32_t edgeLatency = getEdgeLatency(pred, d);
    pred->succs.emplace_back(succ, d, edgeLatency);

    // Set the node priority
    setPriority(pred, pred->succs.back());
    // Count SUCC's predecessors (used to tell when SUCC is ready to issue)
    succ->predsNotScheduled++;

    succ->preds.emplace_back(pred, d, edgeLatency);
}

// Debug function for generating the dependency graph in dot format
void DDD::dumpDagDot(G4_BB *bb)
{
    const char *asmName = nullptr;
    getOptions()->getOption(VISA_AsmFileName, asmName);
    char dumpFileName[MAX_OPTION_STR_LENGTH];
    SNPRINTF(dumpFileName, MAX_OPTION_STR_LENGTH, "%s.bb%d.dag.dot",
        asmName, bb->getId());
    fstream ofile(dumpFileName, ios::out);
    ofile << "digraph DAG {" << std::endl;

    // 1. Get an ordering of the nodes
    std::vector<Node *> DFSordering;
    std::vector<Node *> dfs_stack;
    std::set<Node *> markedSet;
    for (Node *root : Roots)
    {
        dfs_stack.push_back(root);
    }
    while (!dfs_stack.empty())
    {
        Node *node = dfs_stack.back();
        markedSet.insert(node);
        dfs_stack.pop_back();
        DFSordering.push_back(node);
        for (Edge &dep : node->succs)
        {
            Node *succNode = dep.getNode();
            if (!markedSet.count(succNode))
            {
                dfs_stack.push_back(succNode);
            }
        }
    }

    // 2. Generate the .dot file for the DAG
    for (Node *node : allNodes) {
        // G4_INST &g4Inst = const_cast<G4_INST&>(*node->getInstruction());
        // std::stringstream ss;
        // g4Inst.emit(ss, false, false);
        // 1. NODE
        const char *fillColor = (node->schedTime != UINT_MAX) ? "#CCCCCC"
            : (node->getInstructions()->size() > 1) ? "#FFEE99"
            : "#FFFFFF";
        ofile << node->nodeID << "[label=\"";
        for (G4_INST *inst : node->instVec) {
            ofile << G4_Inst_Table[inst->opcode()].str << ", ";
        }
        ofile << "[" << node->nodeID << "]";
        const std::list<G4_INST *> *instrs = node->getInstructions();
        if (instrs->empty()) {
            ofile << "I" << 0;
        } else {
            for (G4_INST *instr : *instrs) {
                ofile << "I" << instr->getLocalId();
            }
        }
        ofile << "O" << node->getOccupancy()
            << "E" << node->getEarliest()
            << "P" << node->priority
            << "\""
            << ", style=\"filled\", fillcolor=\"" << fillColor << "\""
            << "]"
            << std::endl;
        // 2. EDGES
        for (Edge &succDep : node->succs)
        {
            Node *succNode = succDep.getNode();
            if (node->nodeID > succNode->nodeID) {
                fprintf(stderr, "%d->%d\n", node->nodeID, succNode->nodeID);
            }
            auto depType = succDep.getType();
            const char *depColor
                = (depType == RAW || depType == RAW_MEMORY) ? "black"
                : (depType == WAR || depType == WAR_MEMORY) ? "red"
                : (depType == WAW || depType == WAW_MEMORY) ? "orange"
                : "grey";
            uint32_t edgeLatency = getEdgeLatency(node, depType);
            // Example: 30->34[label="4",color="{red|black|yellow}"];
            ofile << node->nodeID << "->" << succNode->nodeID
                << "[label=\"" << edgeLatency << "\""
                << ",color=\"" << depColor << "\""
                << "];" << std::endl;
        }
    }
    ofile << "}" << std::endl;
    ofile.close();
}


// Debug function.
// Dump all nodes in a text form and show their dependences with arrows.
void DDD::dumpNodes(G4_BB *bb)
{
    const char *asmName = nullptr;
    getOptions()->getOption(VISA_AsmFileName, asmName);
    char dumpFileName[MAX_OPTION_STR_LENGTH];
    SNPRINTF(dumpFileName, MAX_OPTION_STR_LENGTH, "%s.bb%d.nodes",
        asmName, bb->getId());
    fstream ofile(dumpFileName, ios::out);

    // 2. Generate the .dot file for the DAG
    for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
        Node *node = *it;
        for (G4_INST *inst : *node->getInstructions()) {
            std::stringstream ss;
            inst->emit(ss, false, false);
            // 1. NODE
            ofile << node->nodeID << " "
                << " Occu:" << node->getOccupancy()
                << " Earl:" << node->getEarliest()
                << " Prio:" << node->priority
                << "  " << *inst;
        }
        ofile << std::endl;
    }

    for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
        Node *node = *it;
        // 2. EDGES
        for (Edge &succDep : node->succs)
        {
            Node *succNode = succDep.getNode();
            if (node->nodeID > succNode->nodeID) {
                fprintf(stderr, "%d->%d\n", node->nodeID, succNode->nodeID);
            }
            auto depType = succDep.getType();
            const char *depTypeStr
                = (depType == RAW || depType == RAW_MEMORY) ? "RAW"
                : (depType == WAR || depType == WAR_MEMORY) ? "WAR"
                : (depType == WAW || depType == WAW_MEMORY) ? "WAW"
                : "grey";
            uint32_t edgeLatency = getEdgeLatency(node, depType);
            // Example: 30->34[label="4",color="{red|black|yellow}"];
            ofile << node->nodeID << "->" << succNode->nodeID
                << "[label=\"" << edgeLatency << "\""
                << ",type=\"" << depTypeStr << "\""
                << "];" << std::endl;
        }
    }
    ofile.close();
}

// Helper comparator for priority queue. Queue top is for lowest earliest cycle.
struct earlyCmp {
    earlyCmp() = default;
    bool operator()(const Node *n1, const Node *n2)
    {
        return n1->getEarliest() > n2->getEarliest();
    }
};

// Helper comparator for priority queue. Top has highest priority (critical path).
struct criticalCmp
{
    criticalCmp() = default;
    bool operator()(const Node *n1, const Node *n2)
    {
        // Critical Path Heuristic
        int prio1 = n1->getPriority();
        int prio2 = n2->getPriority();
        if (prio1 != prio2)
        {
            // Favor node with highest priority
            return prio1 < prio2;
        }
        // Break Ties
        else
        {
            // 1. Favor sends over non-sends
            bool n1_isSend = (*n1->getInstructions()).front()->isSend();
            bool n2_isSend = (*n2->getInstructions()).front()->isSend();
            if (n1_isSend != n2_isSend)
            {
                return n1_isSend < n2_isSend;
            }
            // 2. Favor instruction with lower earliest cycle
            //    This usually maintains instruction order
            int n1_earliest = n1->getEarliest();
            int n2_earliest = n2->getEarliest();
            if (n1_earliest != n2_earliest)
            {
                return n1_earliest > n2_earliest;
            }
            // 3. Favor instructions earliest in the code
            //    NOTE: This is new.
            else
            {
                return (*n1->getInstructions()).front()->getLocalId()
                     > (*n2->getInstructions()).front()->getLocalId();
            }
        }
    }
};

// Perform local list scheduling
uint32_t DDD::listSchedule(G4_BB_Schedule *schedule)
{
    if (getOptions()->getOption(vISA_DumpDagDot))
    {
        dumpDagDot(schedule->getBB());
        dumpNodes(schedule->getBB());
    }

    // All nodes in root set have no dependency
    // so they can be immediately scheduled,
    // and hence are added to readyList.
    std::priority_queue<Node *, std::vector<Node *>, criticalCmp> readyList;

    // Nodes with their predecessors scheduled are pushed into the preReadyQueue
    // They only get pushed into the real readyList if they are ready to issue,
    // that is their earliest cycle is >= than the current schedule cycle.
    std::priority_queue<Node *, std::vector<Node *>, earlyCmp> preReadyQueue;

    collectRoots();
    for (auto N : Roots) {
        preReadyQueue.push(N);
    }

    // The scheduler's clock.
    // This counter is updated in each loop iteration and its
    // final value represents number of cycles the kernel would
    // take to execute on the GPU as per the model.
    uint32_t currCycle = 0;

    // Used to avoid WAW subreg hazards
    Node *lastScheduled = nullptr;

    // Keep scheduling until both readyList or preReadyQueue contain instrs.
    while (!(readyList.empty() && preReadyQueue.empty()))
    {
        Node *readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
        // While non empty, move nodes from preReadyQueue to readyList
        while (readyCandidate && readyCandidate->earliest <= currCycle)
        {
            readyList.push(readyCandidate);
            preReadyQueue.pop();
            readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
        }
        // Nothing to issue at this cycle, increment scheduler clock
        if (readyList.empty())
        {
            // readyCandidate is not nullptr. Proof: If it was nullptr, then both
            // preReadyQueue and readyList would be empty. But this cannot
            // happen because the while() loop will only enter if at least
            // one of them is non-empty.
            assert(readyCandidate && "Both readyList and preReadyQ empty!");
            currCycle = readyCandidate->earliest;
            continue;
        }

        // Pointer to node to be scheduled.
        Node *scheduled = readyList.top();
        readyList.pop();

        // try to avoid b2b math if possible as there are pipeline stalls
        if (scheduled->getInstructions()->front()->isMath() &&
            lastScheduled && lastScheduled->getInstructions()->front()->isMath())
        {
            // pick another node on the ready list if it's not math and won't cause a longer stall
            // to save compile time we currently limit search size to 2
            if (readyList.size() > 0)
            {
                std::vector<Node*> popped;
                const int searchSize = std::min(2, (int) readyList.size());
                for (int i = 0; i < searchSize; ++i)
                {
                    Node* next = readyList.top();
                    readyList.pop();
                    if (!next->getInstructions()->front()->isMath() &&
                        next->getEarliest() < lastScheduled->schedTime + lastScheduled->getOccupancy())
                    {
                        readyList.push(scheduled);
                        scheduled = next;
                        break;
                    }
                    else
                    {
                        // keep searching
                        popped.push_back(next);
                    }
                }
                for (auto nodes : popped)
                {
                    readyList.push(nodes);
                }
            }
        }
        // Avoid WAW subreg hazard by skipping nodes that cause a WAW subreg
        // hazard with the lastScheduled instruction.
        else if (kernel->fg.builder->avoidWAWSubRegHazard())
        {
            // Do not try to fix the hazard for ever (maintain low complexity)
#define WAW_SUBREG_ATTEMPTS 2
            if (lastScheduled) {
                // If only 1 instruction ready, then there is nothing we can do
                if (readyList.size() > 1) {
                    Node *extScheduled = scheduled;
                    std::vector<Node *> skippedNodes;
                    int lastReg = lastScheduled->writesToSubreg();
                    // If lastScheduled writes to a subreg
                    int attempts = 0;
                    if (lastReg != Node::NO_SUBREG) {
                        // While there is a WAW subreg hazard with
                        // the scheduled node, get the next from readyList
                        while (scheduled->writesToSubreg() == lastReg
                            && !readyList.empty()
                            && ++attempts < WAW_SUBREG_ATTEMPTS) {
                            skippedNodes.push_back(scheduled);
                            scheduled = readyList.top();
                            readyList.pop();
                        }
                    }

                    // Re-insert the skipped nodes into the readyList
                    for (Node *poppedNode : skippedNodes) {
                        readyList.push(poppedNode);
                    }

                    // If we have reached the end of the readyList but we still
                    // get a WAW subreg hazard, choose the top of readyList
                    if (scheduled != extScheduled
                        && scheduled->writesToSubreg() == lastReg
                        && (readyList.empty() || attempts >= WAW_SUBREG_ATTEMPTS)) {
                        // Re-insert the scheduled node into the readyList
                        readyList.push(scheduled);
                        // Get the highest priority node
                        scheduled = readyList.top();
                        readyList.pop();
                    }
                }
            }
        }

        assert(scheduled && "Must have found an instruction to schedule by now");

        // Append the scheduled node to the end of the schedule.
        schedule->scheduledNodes.push_back(scheduled);
        lastScheduled = scheduled;

        // Set the cycle at which this node is scheduled.
        scheduled->schedTime = currCycle;

        for (auto &curSucc : scheduled->succs)
        {
            Node *succ = curSucc.getNode();
            // Recompute the earliest time for each successor.
            if (scheduled->isLabel())
            {
                succ->earliest = 0;
            }
            else
            {
                // Update the earliest time of the successor and set its last scheduled
                // predecessor with the largest latency to the currently scheduled node
                // if the latency incurred by scheduling the successor right after the
                // "scheduled" node is larger than successor's earliest time.
                uint32_t latencyToAdd = 0;
                uint32_t earliestNew = 0;
                latencyToAdd = curSucc.getLatency() > scheduled->getOccupancy() ? curSucc.getLatency() : scheduled->getOccupancy();
                earliestNew = scheduled->schedTime + latencyToAdd;

                if (succ->earliest <= earliestNew || !succ->lastSchedPred)
                {
                    succ->lastSchedPred = scheduled;
                }
                succ->earliest = (succ->earliest > earliestNew) ? succ->earliest : earliestNew;
            }
            // Decrease the number of predecessors not scheduled for the successor node.
            if ((--(succ->predsNotScheduled)) == 0)
            {
                preReadyQueue.push(succ);
            }
        }

        // Increment the scheduler's clock after each scheduled node
        currCycle += scheduled->getOccupancy();
    }

    // Sanity check: Make sure all nodes are scheduled
#if defined(_DEBUG)
    for (auto node : allNodes)
    {
        assert(node->schedTime != UINT_MAX && "Node not scheduled!");
    }
#endif
    return currCycle;
}

// This comment is moved from DDD::Latency()
// Given two instructions, this function returns latency
// in number of cycles. If there is a RAW dependency
// between the two instructions then modeled latency
// is higher because second instruction needs to wait
// till first instruction execution is complete to
// receive operands. In case of false dependencies or
// no dependencies, second instruction need not wait
// for completion of execution of first instruction.
// So modeled latency is lower and equal to latency
// of first instruction (which is initialized to either
// 1 or 2 depending on compression attribute).
uint32_t DDD::getEdgeLatency_old(Node *node, DepType depT)
{
    // This is a prefetch read only depending on the terminator.
    // We pessimistically assume that it will be used right after the branch.
    G4_INST *inst = *node->getInstructions()->begin();
    if (depT == DepType::CONTROL_FLOW_BARRIER && inst->isSend())
    {
        G4_SendMsgDescriptor *msgDesc = inst->getMsgDesc();
        if (msgDesc->isDataPortRead())
        {
            return LT.getLatency(inst);
        }
    }

    if (depT <= NODEP || depT >= CONTROL_FLOW_BARRIER)
    {
        return node->getOccupancy();
    }

    uint32_t latency = IVB_PIPELINE_LENGTH;
    switch (depT)
    {
    case RAW:
    case RAW_MEMORY:
        latency = LT.getLatency(inst);
        break;

    case WAR:
    case WAR_MEMORY:
    case WAW:
    case WAW_MEMORY:  //?? WAW have the same cycle as RAW?
        latency = UNCOMPR_LATENCY;  //Used as edge dependence latency also.
        break;

    default:
        break;
    }
    latency = latency > node->getOccupancy() ? latency : node->getOccupancy();
    return latency;
}

uint32_t DDD::getEdgeLatency(Node *node, DepType depT) {
    uint32_t latency = getEdgeLatency_old(node, depT);
    if (useMTLatencies)
    {
        float scale = float(HWthreadsPerEU) / getBuilder()->getCoIssueUints();
        latency = int(latency / scale);
    }
    return latency;
}

Node::Node(uint32_t id, G4_INST* inst, Edge_Allocator& depEdgeAllocator,
    const LatencyTable& LT)
    : nodeID(id)
{
    instVec.push_back(inst);
    occupancy = LT.getOccupancy(inst);

    // Set the initial node priority
    priority = occupancy;

    barrier = CheckBarrier(inst);
}

void LocalScheduler::EmitNode(Node *node) {
    for (G4_INST *inst : *node->getInstructions()) {
        if (inst->isSend())
        {
            inst->asSendInst()->emit_send(cerr);
        } else {
            DEBUG_EMIT(inst);
        }
        DEBUG_MSG("");
    }
}

/************************************************************************/
/* ================ Below are printout methods only  ================   */
/************************************************************************/

void DDD::DumpDotFile(const char *name, const char* appendix){

    MUST_BE_TRUE(name && strlen(name) < 220 && strlen(appendix) < 30,
        ERROR_SCHEDULER);
    char fileName[256];
    SNPRINTF(fileName, 256, "%s.%s.dot", name, appendix);

    fstream ofile(fileName, ios::out);
    if (!ofile)
    {
        MUST_BE_TRUE(false, "[Scheduling]:ERROR: Cannot open file " <<
            fileName << ", dump failed." << std::endl);
    }

    ofile << "digraph " << name << " {" << std::endl;
    ofile << endl << "\t// Setup" << endl;
    ofile << "\tsize = \"80, 100\";\n";
    ofile << endl << "\t// Nodes" << endl;
    std::list<Node*>::iterator iNode(Nodes.begin()), endNodes(Nodes.end());
    for (; iNode != endNodes; ++iNode) {
        for (G4_INST *inst : *(*iNode)->getInstructions()) {

            ofile << "\tID_" << (*iNode)->nodeID
                << "\t[shape=record, label=\"{ID : " << (*iNode)->nodeID
                << " PRIORITY : " << (*iNode)->priority << " | "
                << " ETIME : " << (*iNode)->earliest << " | ";
            ofile << ((inst->isLabel()) ? "Label: " : "");
            std::ostringstream os;
            if (inst->isSend())
            {
                inst->asSendInst()->emit_send(os);
            }
            else
            {
                inst->emit(os);
            }

            std::string dotStr(os.str());
            //TODO: dot doesn't like '<', '>', '{', or '}' this code below is a hack. need to replace with delimiters.
            std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '<'), '[');
            std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '>'), ']');
            std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '{'), '[');
            std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '}'), ']');
            ofile << dotStr;
            ofile << "\\l";
            ofile << "} \"];" << std::endl;
        }
    }
    ofile << endl << "\t// Edges" << endl;

    for (iNode = Nodes.begin(); iNode != endNodes; ++iNode)
    {
        Node *node = *iNode;
        EdgeVector::iterator iEdge(node->succs.begin()), endEdges(node->succs.end());
        for (; iEdge != endEdges; ++iEdge)
        {
            ofile << "\tID_" << node->nodeID << " -> " << "ID_" << (*iEdge).getNode()->nodeID;
        }
    }
    ofile << " }" << std::endl;
    ofile.close();
}

void G4_BB_Schedule::emit(std::ostream &out) {
    if (!scheduledNodes.empty()) {
        out << endl;
        for (uint32_t i = 0; i < (uint32_t)scheduledNodes.size(); i++) {
            for (G4_INST *inst : *scheduledNodes[i]->getInstructions()) {
                out << scheduledNodes[i]->schedTime << ":\t";
                if (!inst->isSend()) {
                    inst->emit(out);
                } else {
                    inst->asSendInst()->emit_send(out);
                }
                out << endl;
            }
        }
    }
}

