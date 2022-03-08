/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LocalScheduler_G4IR.h"
#include "Dependencies_G4IR.h"
#include "../G4_Opcode.h"
#include "../Timer.h"
#include "visa_wa.h"

#include <fstream>
#include <functional>
#include <sstream>
#include <queue>

using namespace vISA;

/* Entry to the local scheduling. */
void LocalScheduler::localScheduling()
{
    // This is controlled by options for debugging
    if (!fg.getKernel()->isLocalSheduleable())
    {
        return;
    }

    DEBUG_VERBOSE("[Scheduling]: Starting...");
    BB_LIST_ITER ib(fg.begin()), bend(fg.end());
    MUST_BE_TRUE(ib != bend, ERROR_SCHEDULER);

    VISA_BB_INFO* bbInfo = (VISA_BB_INFO *)mem.alloc(fg.size() * sizeof(VISA_BB_INFO));
    memset(bbInfo, 0, fg.size() * sizeof(VISA_BB_INFO));
    int i = 0;

    const Options *m_options = fg.builder->getOptions();
    LatencyTable LT(fg.builder);

    PointsToAnalysis p(fg.getKernel()->Declares, fg.size());
    p.doPointsToAnalysis(fg);

    uint32_t totalCycles = 0;
    uint32_t scheduleStartBBId = m_options->getuInt32Option(vISA_LocalSchedulingStartBB);
    uint32_t shceduleEndBBId = m_options->getuInt32Option(vISA_LocalSchedulingEndBB);
    for (; ib != bend; ++ib)
    {
        if ((*ib)->getId() < scheduleStartBBId || (*ib)->getId() > shceduleEndBBId)
        {
            continue;
        }

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
                    G4_BB_Schedule schedule(fg.getKernel(), bbMem, tempBB, LT, p);
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
            G4_BB_Schedule schedule(fg.getKernel(), bbMem, *ib, LT, p);
            bbInfo[i].id = (*ib)->getId();
            bbInfo[i].staticCycle = schedule.sequentialCycle;
            bbInfo[i].sendStallCycle = schedule.sendStallCycle;
            bbInfo[i].loopNestLevel = (*ib)->getNestLevel();
            totalCycles += schedule.sequentialCycle;
        }

        i++;
    }
    FINALIZER_INFO* jitInfo = fg.builder->getJitInfo();
    jitInfo->BBInfo = bbInfo;
    jitInfo->BBNum = i;

    fg.builder->getcompilerStats().SetI64(CompilerStats::numCyclesStr(), totalCycles, fg.getKernel()->getSimdSize());
}

void G4_BB_Schedule::dumpSchedule(G4_BB *bb)
{
    const char *asmName = nullptr;
    getOptions()->getOption(VISA_AsmFileName, asmName);
    std::string dumpFileName = std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".schedule";
    std::ofstream ofile(dumpFileName, std::ios::out);
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
                ofile << std::setw(4) << cycle << " ";
                for (G4_INST *inst : (*nodeIT)->instVec) {
                    ofile << std::setw(5) << G4_Inst_Table[inst->opcode()].str;
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
                ofile << std::setw(4) << cycle << " " << std::endl;
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
    const LatencyTable& LT, PointsToAnalysis &p)
    : mem(m)
    , bb(block)
    , kernel(k)
    , pointsToAnalysis(p)
{
    // we use local id in the scheduler for determining two instructions' original ordering
    bb->resetLocalIds();

    DDD ddd(mem, bb, LT, k, p);
    // Generate pairs of TypedWrites
    bool doMessageFuse =
        (k->fg.builder->fuseTypedWrites() && k->getSimdSize() >= g4::SIMD16) ||
        k->fg.builder->fuseURBMessage();

    if (doMessageFuse)
    {
        ddd.pairTypedWriteOrURBWriteNodes(bb);
    }

    if ((ddd.getIs2xFPBlock())&&
        getOptions()->getOption(vISA_ScheduleFor2xSP))
    {
        lastCycle = ddd.listScheduleFor2xFP(this);
    }
    else if (getOptions()->getOption(vISA_ScheduleForReadSuppression) && ddd.getIsThreeSourceBlock())
    {
        lastCycle = ddd.listScheduleForSuppression(this);
    }
    else
    {
        lastCycle = ddd.listSchedule(this);
    }

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
    unsigned HWThreadsPerEU = k->getNumThreads();
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
            const RegionDesc *rd = srcOpnd->getRegion();
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

void DDD::getBucketsForIndirectOperand(G4_INST* inst,
    Gen4_Operand_Number opnd_num,
    G4_Operand* opnd,
    std::vector<BucketDescr>& BDvec)
{
    G4_Declare* addrdcl = nullptr;
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

    G4_RegVar* ptvar = nullptr;
    int vid = 0;
    unsigned char offset = 0;
    while ((ptvar = pointsToAnalysis.getPointsTo(addrdcl->getRegVar(), vid++, offset)) != nullptr)
    {

        uint32_t varID = ptvar->getId();
        G4_Declare* dcl = ptvar->getDeclare()->getRootDeclare();
        G4_RegVar* var = dcl->getRegVar();

        MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
        MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
        uint32_t regOff = var->getPhyRegOff();
        int linearizedStart = regNum * kernel->numEltPerGRF<Type_UB>() + regOff * TypeSize(dcl->getElemType());
        int linearizedEnd = linearizedStart + dcl->getByteSize() - 1;

        int startingBucket = linearizedStart / kernel->numEltPerGRF<Type_UB>();
        int endingBucket = linearizedEnd / kernel->numEltPerGRF<Type_UB>();
        Mask mask(linearizedStart, linearizedEnd, false, opnd->getAccRegSel());
        int numBuckets = endingBucket - startingBucket + 1;
        for (int j = startingBucket; j < (startingBucket + numBuckets); j++)
        {
            BDvec.push_back(BucketDescr(j, mask, opnd_num));
        }
    }
    return;
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
        startingBucket = opnd->getLinearizedStart() / kernel->numEltPerGRF<Type_UB>();
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
            unsigned divisor = kernel->numEltPerGRF<Type_UB>();
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
void DDD::getBucketDescrs(Node *node, std::vector<BucketDescr>& BDvec)
{
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
            if (hasIndirection(opnd, opndNum))
            {
                getBucketsForIndirectOperand(inst, opndNum, opnd, BDvec);
            }
        }

        // Sends need an additional bucket
        if (inst->isSend()) {
            if (inst->getMsgDesc()->isScratch()) {
                BDvec.push_back(BucketDescr(SCRATCH_SEND_BUCKET, Mask(), Opnd_dst));
            }
            else {
                BDvec.push_back(BucketDescr(SEND_BUCKET, Mask(), Opnd_dst));
            }
        }
    }

    return;
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

/*
Read suppression opportunity checking to group the three source instructions with read suppression into single node
1. per-source slot
2. No RAW and WAW dependence
3. Same opcode
4. acc define will seperate a new group
5. No other special register (including implicit) is used.
6. Only checking the suppression chance between adjacent instructions before scheduling.
7. Assumption, pre-RA scheduling will not do scheduling to the three source instructions

The group is built by scanning the code from back to front.
When tracking the dependence, we only care the dependence across different instructions.
So, in each time analysis, we record the source live of the second instruction and the destination live of the first instruction
dst1 inst1 src1_0 src1_1 src1_2
dst2 inst2 src2_0 src2_1 src2_2
In the dependence tracking,
RAW: when the destination register of the inst1 can be found in the liveSrc
WAW: when the destination register of the inst1 can be found in the liveDst
liveSrc record the source registers of all the instructions in the group.
liveDst record the destination registers of all the instructions in the group except the last one in the group.
*/
bool DDD::hasReadSuppression(G4_INST *prevInst, G4_INST *nextInst, BitSet &liveDst, BitSet &liveSrc)
{
    //Not three source
    if (nextInst->getNumSrc() != 3 || nextInst->isSend())
    {
        return false;
    }

    //Different opcode
    if (prevInst->opcode() != nextInst->opcode())
    {
        return false;
    }

    /*
    * No special registers except acc and GRF
    */
    for (Gen4_Operand_Number opndNum
        : { Opnd_src3,
        Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst})
    {
        G4_Operand *opnd = prevInst->getOperand(opndNum);
        // Skip if no operand or the operand is not touched by the instruction
        if (opnd) {
            return false;
        }
    }

    //Only support GRF and ACC registers in operands
    for (Gen4_Operand_Number opndNum
        : { Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2 })
    {
        G4_Operand *opnd = prevInst->getOperand(opndNum);

        // Skip if no operand or the operand is not touched by the instruction
        if (!opnd->isGreg() && !opnd->isImm()) {
            return false;
        }
    }

    int currInstRegs[2][G4_MAX_SRCS];
    int nextInstRegs[2][G4_MAX_SRCS];
    bool isCurrScalar[G4_MAX_SRCS] = { false, false, false, false };
    bool isNextScalar[G4_MAX_SRCS] = { false, false, false, false };
    int opndSize = 0;

    //Get the sources of previous instruction
    for (unsigned i = 0; i < 3; i++)
    {
        currInstRegs[0][i] = -1;
        currInstRegs[1][i] = -1;
        G4_Operand* srcOpnd = prevInst->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                !srcOpnd->asSrcRegRegion()->isIndirect() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                opndSize = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    currInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

                    if (opndSize > kernel->getGRFSize())
                    {
                        currInstRegs[1][i] = currInstRegs[0][i] + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
                    }
                    else if (srcOpnd->asSrcRegRegion()->isScalar()) //No Read suppression for SIMD 16/scalar src
                    {
                        isCurrScalar[i] = true;
                        currInstRegs[1][i] = currInstRegs[0][i];
                    }
                }
            }
        }
    }

    //Get the source of the next instruction
    for (unsigned i = 0; i < 3; i++)
    {
        nextInstRegs[0][i] = -1;
        nextInstRegs[1][i] = -1;
        G4_Operand* srcOpnd = nextInst->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                !srcOpnd->asSrcRegRegion()->isIndirect() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                opndSize = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    nextInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

                    liveSrc.set(nextInstRegs[0][i], true);  //Set live

                    if (opndSize > kernel->getGRFSize())
                    {
                        int reg = nextInstRegs[0][i] + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
                        nextInstRegs[1][i] =  reg;
                        liveSrc.set(reg, true);  //Set live
                    }
                    if (srcOpnd->asSrcRegRegion()->isScalar()) //No Read suppression for SIMD 16/scalar src
                    {
                        isNextScalar[i] = true;
                        nextInstRegs[1][i] =  nextInstRegs[0][i] ;
                    }
                }
            }
        }
    }

    bool curInstSimd8 = false;
    bool nextInstSimd8 = false;
    int dstReg0 = -1;
    int dstReg1 = -1;


    //Get Dst of the next instruction
    G4_DstRegRegion *nextDstOpnd = nextInst->getDst();

    if (nextDstOpnd &&
        !nextDstOpnd->isIndirect() &&
        nextDstOpnd->isGreg())
    {
        opndSize = nextDstOpnd->getLinearizedEnd() - nextDstOpnd->getLinearizedStart() + 1;
        uint32_t byteAddress = nextDstOpnd->getLinearizedStart();
        dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();
        liveDst.set(dstReg0, true);
        if (opndSize <= kernel->getGRFSize())
        {
            nextInstSimd8 = true;
        }
        else
        {
            dstReg1 = dstReg0 + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
            liveDst.set(dstReg1, true);
        }
    }

    //Get Dst of previous instruction
    G4_DstRegRegion *dstOpnd = prevInst->getDst();
    dstReg0 = -1;
    dstReg1 = -1;

    if (dstOpnd &&
        !dstOpnd->isIndirect() &&
        dstOpnd->isGreg())
    {
        opndSize = dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
        uint32_t byteAddress = dstOpnd->getLinearizedStart();
        dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();
        //If there is RAW and WAW dependence
        if (liveSrc.isSet(dstReg0) || liveDst.isSet(dstReg0))
        {
            return false;
        }

        if (opndSize <= kernel->getGRFSize())
        {
            curInstSimd8 = true;
        }
        else
        {
            dstReg1 = dstReg0 + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;

            //If there is RAW and WAW dependence
            if (liveSrc.isSet(dstReg1) || liveDst.isSet(dstReg1))
            {
                return false;
            }
        }
    }

    //Kill the suppression if the register is defined in the same instruction
    for (unsigned i = 0; i < 3; i++)
    {
        if ((dstReg1 != -1 &&  dstReg1 == currInstRegs[0][i]) || (dstReg0 != -1 && dstReg0 == currInstRegs[0][i]))
        {
            currInstRegs[0][i] = -1;
        }
        if ((dstReg1 != -1 &&  dstReg1 == currInstRegs[1][i]) || (dstReg0 != -1 && dstReg0 == currInstRegs[1][i]))
        {
            currInstRegs[1][i] = -1;
        }
    }

    //For TGL, src0 support read suppresion as well
    if (kernel->fg.builder->hasSrc0ReadSuppression())
    {
        if (currInstRegs[0][0] == nextInstRegs[0][0] && currInstRegs[0][0] != -1 && curInstSimd8 && nextInstSimd8)
        {
            //No scalar supperssion
            if (!isCurrScalar[0] && !isNextScalar[0])
            {
                return true;
            }
        }
    }

    //If there is read suppresion for src1
    //for src1 2 GRF suppression is supported.
    if (currInstRegs[0][1] == nextInstRegs[0][1] && currInstRegs[0][1] != -1 &&
        (( curInstSimd8 && nextInstSimd8) || ( !curInstSimd8 && !nextInstSimd8)))
    {
        //No scalar supperssion
        if (!isCurrScalar[1] && !isNextScalar[1])
        {
            return true;
        }
    }

    if (currInstRegs[0][2] == nextInstRegs[0][2] && currInstRegs[0][2] != -1 && curInstSimd8 && nextInstSimd8)
    {
        //No scalar supperssion
        if (!isCurrScalar[2] && !isNextScalar[2])
        {
            return true;
        }
    }

    return false;
}

bool DDD::hasReadSuppression(G4_INST* prevInst, G4_INST* nextInst, bool multiSuppression)
{
    //Not three source
    if (nextInst->getNumSrc() != 3 || nextInst->isSend())
    {
        return false;
    }

    //Different opcode
    if (prevInst->opcode() != nextInst->opcode())
    {
        return false;
    }

    /*
    * No special registers except acc and GRF
    */
    for (Gen4_Operand_Number opndNum
        : { Opnd_src3,
        Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst})
    {
        G4_Operand* opnd = prevInst->getOperand(opndNum);
        // Skip if no operand or the operand is not touched by the instruction
        if (opnd) {
            return false;
        }
    }

    //Only support GRF and ACC registers in operands
    for (Gen4_Operand_Number opndNum
        : { Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2 })
    {
        G4_Operand* opnd = prevInst->getOperand(opndNum);

        // Skip if no operand or the operand is not touched by the instruction
        if (!opnd->isGreg() && !opnd->isImm()) {
            return false;
        }
    }

    int currInstRegs[2][G4_MAX_SRCS];
    int nextInstRegs[2][G4_MAX_SRCS];
    bool isCurrScalar[G4_MAX_SRCS] = { false, false, false, false };
    bool isNextScalar[G4_MAX_SRCS] = { false, false, false, false };
    int opndSize = 0;

    //Get the sources of previous instruction
    for (unsigned i = 0; i < 3; i++)
    {
        currInstRegs[0][i] = -1;
        currInstRegs[1][i] = -1;
        G4_Operand* srcOpnd = prevInst->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                !srcOpnd->asSrcRegRegion()->isIndirect() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                opndSize = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    currInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

                    if (opndSize > kernel->getGRFSize())
                    {
                        currInstRegs[1][i] = currInstRegs[0][i] + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
                    }
                    else if (srcOpnd->asSrcRegRegion()->isScalar()) //No Read suppression for SIMD 16/scalar src
                    {
                        isCurrScalar[i] = true;
                        currInstRegs[1][i] = currInstRegs[0][i];
                    }
                }
            }
        }
    }

    //Get the source of the next instruction
    for (unsigned i = 0; i < 3; i++)
    {
        nextInstRegs[0][i] = -1;
        nextInstRegs[1][i] = -1;
        G4_Operand* srcOpnd = nextInst->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                !srcOpnd->asSrcRegRegion()->isIndirect() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                opndSize = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    nextInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

                    if (opndSize > kernel->getGRFSize())
                    {
                        int reg = nextInstRegs[0][i] + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
                        nextInstRegs[1][i] = reg;
                    }
                    if (srcOpnd->asSrcRegRegion()->isScalar()) //No Read suppression for SIMD 16/scalar src
                    {
                        isNextScalar[i] = true;
                        nextInstRegs[1][i] = nextInstRegs[0][i];
                    }
                }
            }
        }
    }

    bool curInstSimd8 = false;
    bool nextInstSimd8 = false;
    int dstReg0 = -1;
    int dstReg1 = -1;


    //Get Dst of the next instruction
    G4_DstRegRegion* nextDstOpnd = nextInst->getDst();

    if (nextDstOpnd &&
        !nextDstOpnd->isIndirect() &&
        nextDstOpnd->isGreg())
    {
        opndSize = nextDstOpnd->getLinearizedEnd() - nextDstOpnd->getLinearizedStart() + 1;
        uint32_t byteAddress = nextDstOpnd->getLinearizedStart();
        dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();
        if (opndSize <= kernel->getGRFSize())
        {
            nextInstSimd8 = true;
        }
        else
        {
            dstReg1 = dstReg0 + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
        }
    }

    //Get Dst of previous instruction
    G4_DstRegRegion* dstOpnd = prevInst->getDst();
    dstReg0 = -1;
    dstReg1 = -1;

    if (dstOpnd &&
        !dstOpnd->isIndirect() &&
        dstOpnd->isGreg())
    {
        opndSize = dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
        uint32_t byteAddress = dstOpnd->getLinearizedStart();
        dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();

        if (opndSize <= kernel->getGRFSize())
        {
            curInstSimd8 = true;
        }
        else
        {
            dstReg1 = dstReg0 + (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) / kernel->numEltPerGRF<Type_UB>() - 1;
        }
    }

    //Kill the suppression if the register is defined in the same instruction
    for (unsigned i = 0; i < 3; i++)
    {
        if ((dstReg1 != -1 && dstReg1 == currInstRegs[0][i]) || (dstReg0 != -1 && dstReg0 == currInstRegs[0][i]))
        {
            currInstRegs[0][i] = -1;
        }
        if ((dstReg1 != -1 && dstReg1 == currInstRegs[1][i]) || (dstReg0 != -1 && dstReg0 == currInstRegs[1][i]))
        {
            currInstRegs[1][i] = -1;
        }
    }

    int  suppressionSrcs = 0;
    //For TGL, src0 support read suppresion as well
    if (kernel->fg.builder->hasSrc0ReadSuppression())
    {
        if (currInstRegs[0][0] == nextInstRegs[0][0] && currInstRegs[0][0] != -1 && curInstSimd8 && nextInstSimd8)
        {
            //No scalar supperssion
            if (!((!kernel->fg.builder->hasScalarReadSuppression()) && (isCurrScalar[0] || isNextScalar[0])))
            {
                if (!multiSuppression)
                {
                    return true;
                }
                suppressionSrcs++;
            }
        }
    }

    //If there is read suppresion for src1
    //for src1 2 GRF suppression is supported.
    if (currInstRegs[0][1] == nextInstRegs[0][1] && currInstRegs[0][1] != -1 &&
        ((curInstSimd8 && nextInstSimd8) || (!curInstSimd8 && !nextInstSimd8)))
    {
        //No scalar supperssion
        if (!((!kernel->fg.builder->hasScalarReadSuppression()) && (isCurrScalar[1] || isNextScalar[1])))
        {
            if (!multiSuppression)
            {
                return true;
            }
            suppressionSrcs++;
        }
    }

    if (currInstRegs[0][2] == nextInstRegs[0][2] && currInstRegs[0][2] != -1 && curInstSimd8 && nextInstSimd8)
    {
        //No scalar supperssion
        if (!((!kernel->fg.builder->hasScalarReadSuppression()) && (isCurrScalar[2] || isNextScalar[2])))
        {
            if (!multiSuppression)
            {
                return true;
            }
            suppressionSrcs++;
        }
    }

    return suppressionSrcs > 1;
}

bool DDD::hasSameSourceOneDPAS(G4_INST *curInst, G4_INST *nextInst, BitSet &liveDst, BitSet &liveSrc)
{
    G4_Type curTypes[4];
    G4_Type nextTypes[4];

    //Get Types
    for(int i = 0; i < 4; i++)
    {
        curTypes[i] = Type_UNDEF;
        nextTypes[i] = Type_UNDEF;
    }
    curTypes[0] = curInst->getDst()->getType();
    nextTypes[0] = nextInst->getDst()->getType();
    for (int i = 0; i < 3; i++)
    {
        curTypes[i + 1] = curInst->getSrc(i)->getType();
        nextTypes[i + 1] = nextInst->getSrc(i)->getType();
    }

    //Same type for all operands
    for(int i = 0; i < 4; i++)
    {
        if (curTypes[i] != nextTypes[i])
        {
            return false;
        }
    }

    G4_InstDpas* curDpasInst = curInst->asDpasInst();
    G4_InstDpas* nextDpasInst = nextInst->asDpasInst();
    uint8_t curr_D = curDpasInst->getSystolicDepth();
    uint8_t next_D = nextDpasInst->getSystolicDepth();

    //Same depth
    if (curr_D == next_D)
    {
        G4_Operand * c_dst = curInst->getDst();
        G4_Operand *n_dst = nextInst->getDst();
        unsigned short c_dstLB = c_dst->getLinearizedStart();
        unsigned short c_dstRB = c_dst->getLinearizedEnd();
        unsigned short n_dstLB = n_dst->getLinearizedStart();
        unsigned short n_dstRB = n_dst->getLinearizedEnd();
        int c_dstReg = c_dstLB / kernel->numEltPerGRF<Type_UB>();
        int n_dstReg = n_dstLB / kernel->numEltPerGRF<Type_UB>();

        //Set destination live for current instruction
        while (c_dstReg * kernel->numEltPerGRF<Type_UB>() < c_dstRB)
        {
            liveDst.set(c_dstReg, true);
            c_dstReg++;
        }

        for (int i = 0; i < 3; i++)
        {
            G4_Operand *c_src = curInst->getSrc(i);
            G4_Operand *n_src = nextInst->getSrc(i);
            unsigned short c_srcLB = c_src->getLinearizedStart();
            unsigned short c_srcRB = c_src->getLinearizedEnd();
            unsigned short n_srcLB = n_src->getLinearizedStart();

            //Set source live for current instruction
            int srcReg = c_srcLB / kernel->numEltPerGRF<Type_UB>();
            while (srcReg * kernel->numEltPerGRF<Type_UB>() < c_srcRB)
            {
                liveSrc.set(srcReg, true);
                srcReg++;
            }

            //Not same src1 register
            if (i == 1 && c_srcLB != n_srcLB)
            {
                return false;
            }
        }

        //If there is RAW, WAW dependence
        while (n_dstReg * kernel->numEltPerGRF<Type_UB>() < n_dstRB)
        {
            if (liveDst.isSet(n_dstReg) || liveSrc.isSet(n_dstReg))
            {
                return false;
            }
            n_dstReg++;
        }

        for (int i = 0; i < 3; i++)
        {
            G4_Operand *n_src = nextInst->getSrc(i);
            unsigned short n_srcLB = n_src->getLinearizedStart();
            unsigned short n_srcRB = n_src->getLinearizedEnd();
#if 0
            //There is overlap between source and destitation
            if (!(n_dstLB > n_srcRB || n_dstRB < n_srcLB))
            {
                if (i == 1)
                {
                    assert(0);
                }
                return false;
            }
#endif
            //If there is WAR dependence
            int srcReg = n_srcLB / kernel->numEltPerGRF<Type_UB>();
            while (srcReg * kernel->numEltPerGRF<Type_UB>() < n_srcRB)
            {
                if (liveDst.isSet(srcReg))
                {
                    return false;
                }
                srcReg++;
            }
        }

        return true;
    }

    return false;
}

// Construct data dependencey DAG. The function constructs
// DAG using a bucket-based algorithm. The idea is to setup
// as many buckets as there are GRFs (and ARFs). Then when
// testing dependencies the DAG, we only need to look at buckets
// that current inst may fall into. This saves us from checking
// dependencies with all insts in live set. After analyzing
// dependencies and creating necessary edges, current inst
// is inserted in all buckets it touches.
DDD::DDD(Mem_Manager& m, G4_BB* bb, const LatencyTable& lt, G4_Kernel* k, PointsToAnalysis& p)
    : mem(m)
    , LT(lt)
    , kernel(k)
    , pointsToAnalysis(p)
{
    Node* lastBarrier = nullptr;
    HWthreadsPerEU = k->getNumThreads();
    useMTLatencies = getBuilder()->useMultiThreadLatency();
    totalGRFNum = kernel->getNumRegTotal();
    isThreeSouceBlock = false;
    is_2XFP_Block = false;
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

    int threeSrcInstNUm = 0;
    int FP_InstNum = 0;
    int sendInstNum = 0;
    for (int nodeId = (int)(bb->size() - 1); iInst != iInstEnd; ++iInst, nodeId--)
    {
        Node *node = nullptr;
        // If we have a pair of instructions to be mapped on a single DAG node:
        node = new (mem)Node(nodeId, *iInst, depEdgeAllocator, LT);
        allNodes.push_back(node);
        G4_INST *curInst = node->getInstructions()->front();
        BDvec.clear();

        if (curInst->getNumSrc() == 3)
        {
            threeSrcInstNUm ++;
        }
        G4_DstRegRegion* dstOpnd = curInst->getDst();
        sendInstNum += curInst->isSend() ? 1 : 0;
        if (dstOpnd &&
            dstOpnd->getTopDcl() &&
            dstOpnd->getTopDcl()->getRegFile() == G4_GRF)
        {
            bool isCounted = false;
            if (getBuilder()->has2xDP() && instCanUse2xDP(curInst))
            {
                FP_InstNum++;
                isCounted = true;
            }

        }
        if (getBuilder()->hasReadSuppression() &&
            getOptions()->getOption(vISA_EnableGroupScheduleForBC))
        {
            //FIXME: we can extended to all 3 sources
            if (curInst->opcode() == G4_mad || curInst->opcode() == G4_dp4a)
            {
                 std::list<G4_INST*>::reverse_iterator iNextInst = iInst;
                 iNextInst ++;
                 if (iNextInst != iInstEnd)
                 {
                     G4_INST *nextInst = *iNextInst;
                     BitSet liveSrc(totalGRFNum, false);
                     BitSet liveDst(totalGRFNum, false);
                     liveSrc.clear();
                     liveDst.clear();
                     while (hasReadSuppression(nextInst, curInst, liveDst, liveSrc))
                     {
                         //Pushed to the same node
                         node->instVec.push_front(nextInst);
                         nodeId--;
                         curInst = nextInst;
                         iInst = iNextInst;
                         iNextInst++;
                         if (iNextInst == iInstEnd)
                         {
                             break;
                         }
                         nextInst = *iNextInst;
                     }
                 }
            }
        }

        if (curInst->isDpas())
        {
             std::list<G4_INST*>::reverse_iterator iNextInst = iInst;
             iNextInst ++;
             if (iNextInst != iInstEnd)
             {
                 G4_INST *nextInst = *iNextInst;
                 BitSet liveSrc(totalGRFNum, false);
                 BitSet liveDst(totalGRFNum, false);
                 liveSrc.clear();
                 liveDst.clear();

                 while (nextInst->isDpas() &&
                        hasSameSourceOneDPAS(curInst, nextInst, liveDst, liveSrc))
                 {
                     //Pushed to the same node
                     node->instVec.insert(node->instVec.begin(), nextInst);
                     nodeId--;
                     curInst = nextInst;
                     iInst = iNextInst;
                     iNextInst++;
                     if (iNextInst == iInstEnd)
                     {
                         break;
                     }
                     nextInst = *iNextInst;
                 }
             }
        }
        // Get buckets for all physical registers assigned in curInst
        getBucketDescrs(node, BDvec);
        if (curInst->isSend() && curInst->asSendInst()->isFence())
        {
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
                bool curKillsBucket = curMask.killsBucket(curBucket, *kernel->fg.builder);

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

                    //Acc1 and Acc3 may crash acc0 data
                    if (curBucket == ACC_BUCKET)
                    {
                        hasOverlap = true;
                    }
                    // 1. Find DEP type
                    DepType dep = DEPTYPE_MAX;
                    if (curBucket < ACC_BUCKET) {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                    } else if (curBucket == ACC_BUCKET
                        || curBucket == A0_BUCKET)
                    {
                        dep = getDepForOpnd(curOpnd, liveOpnd);
                        curKillsBucket = false;
                    } else if (curBucket == SEND_BUCKET) {
                        dep = getDepSend(curInst, liveInst, BTIIsRestrict);
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

    if (Nodes.size())
    {
        isThreeSouceBlock = ((float)threeSrcInstNUm / Nodes.size()) > THREE_SOURCE_BLOCK_HERISTIC;
        is_2XFP_Block = (FP_InstNum >= FP_MIN_INST_NUM) &&
            (((float)FP_InstNum / Nodes.size()) > THREE_SOURCE_BLOCK_HERISTIC) &&
            (((float)sendInstNum / FP_InstNum) < FP_BLOCK_SEND_HERISTIC);
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
        && inst->getExecSize() == g4::SIMD8
        && inst->getMsgDescRaw()
        && inst->getMsgDescRaw()->isHdcTypedSurfaceWrite()
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
    if (inst->isSend() &&
        inst->getMsgDescRaw() &&
        inst->getMsgDesc()->getSFID() == SFID::URB)
    {
        DescData desc;
        desc.value = inst->getMsgDescRaw()->getDesc();
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
static bool canFuseURB(const G4_INST* secondURB, const G4_INST* firstURB)
{
    if (!firstURB->getMsgDescRaw() || !secondURB->getMsgDescRaw()) {
        return false;
    }
    DescData firstDesc, secondDesc;
    firstDesc.value = firstURB->getMsgDescRaw()->getDesc();
    secondDesc.value = secondURB->getMsgDescRaw()->getDesc();
    if (firstDesc.layout.opcode == secondDesc.layout.opcode &&
        firstDesc.layout.maskPresent == secondDesc.layout.maskPresent &&
        firstDesc.layout.perSlotPresent == secondDesc.layout.maskPresent &&
        firstDesc.layout.offset + 2 == secondDesc.layout.offset)
    {
        if (firstURB->getMsgDescRaw()->MessageLength() == secondURB->getMsgDescRaw()->MessageLength() &&
            firstURB->getMsgDescRaw()->extMessageLength() == secondURB->getMsgDescRaw()->extMessageLength())
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
            if (inst->isSend() && inst->getMsgDesc()->getSFID() == SFID::URB)
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
                firstInstr->isSend() &&
                (firstInstr->getMsgDesc()->getSFID() == SFID::URB ||
                (kernel->fg.builder->fuseTypedWrites() &&
                (isTypedWritePart(firstInstr, 0) ||
                isTypedWritePart(firstInstr, 2)))))
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
    std::string dumpFileName = std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".dag.dot";
    std::ofstream ofile(dumpFileName, std::ios::out);
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
    std::string dumpFileName = std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".nodes";
    std::ofstream ofile(dumpFileName, std::ios::out);

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

struct criticalCmpForMad
{
    criticalCmpForMad() = default;
    bool operator()(const Node* n1, const Node* n2)
    {
        return (n1->getInstructions()->front()->getCISAOff() > n2->getInstructions()->front()->getCISAOff());
    }
};

// 1).The priority queue is ordered as original sequence order.
// 2).If there is a mad instruction be scheduled, trying to search the candidate which has read suppression in src1and src2.
// 3).The scheduling is only applied to the BB whose instructions are mostly mad.
// 4).This scheduling is not for general instruction scheduling, it's controlled by option vISA_ScheduleForReadSuppression
uint32_t DDD::listScheduleForSuppression(G4_BB_Schedule* schedule)
{
    if (getOptions()->getOption(vISA_DumpDagDot))
    {
        dumpDagDot(schedule->getBB());
        dumpNodes(schedule->getBB());
    }

    // All nodes in root set have no dependency
    // so they can be immediately scheduled,
    // and hence are added to readyList.
    std::priority_queue<Node*, std::vector<Node*>, criticalCmpForMad> readyList;

    // Nodes with their predecessors scheduled are pushed into the preReadyQueue
    // They only get pushed into the real readyList if they are ready to issue,
    // that is their earliest cycle is >= than the current schedule cycle.
    std::priority_queue<Node*, std::vector<Node*>, earlyCmp> preReadyQueue;

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
    Node* lastScheduled = nullptr;

    // Keep scheduling until both readyList or preReadyQueue contain instrs.
    while (!(readyList.empty() && preReadyQueue.empty()))
    {
        Node* readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
        // While non empty, move nodes from preReadyQueue to readyList
        while (readyCandidate)
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
        Node* scheduled = readyList.top();
        readyList.pop();
        if (lastScheduled &&
            kernel->fg.builder->hasReadSuppression() &&
            (readyList.size() > 0))
        {
            if (lastScheduled->getInstructions()->size() == 1)
            {
                G4_INST* inst = lastScheduled->getInstructions()->front();
                if (inst->opcode() == G4_mad ||
                    inst->opcode() == G4_dp4a)
                {
                    std::vector<Node*> popped;
                    const int searchSize = (int)readyList.size();

                    G4_INST* scheduledInst = scheduled->getInstructions()->front();
                    if (!((scheduledInst->opcode() == G4_mad ||
                        scheduledInst->opcode() == G4_dp4a) &&
                        hasReadSuppression(inst, scheduledInst, inst->getExecSize() == g4::SIMD8)))
                    {
                        for (int i = 0; i < searchSize; ++i)
                        {
                            Node* next = readyList.top();
                            if ((next->getInstructions()->size() == 1))
                            {
                                G4_INST* nextInst = next->getInstructions()->front();
                                readyList.pop();
                                if ((nextInst->opcode() == G4_mad ||
                                    nextInst->opcode() == G4_dp4a) &&
                                    hasReadSuppression(inst, nextInst, inst->getExecSize() == g4::SIMD8))
                                {
                                    readyList.push(scheduled);
                                    scheduled = next;
                                    break;
                                }
                                else
                                {
                                    popped.push_back(next);
                                }
                            }
                        }

                        for (auto nodes : popped)
                        {
                            readyList.push(nodes);
                        }
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

        for (auto& curSucc : scheduled->succs)
        {
            Node* succ = curSucc.getNode();
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

//Scheduling for the 2xDP pipeline
uint32_t DDD::listScheduleFor2xFP(G4_BB_Schedule* schedule)
{
    if (getOptions()->getOption(vISA_DumpDagDot))
    {
        dumpDagDot(schedule->getBB());
        dumpNodes(schedule->getBB());
    }

    // All nodes in root set have no dependency
    // so they can be immediately scheduled,
    // and hence are added to readyList.
    // 2xSP specific, the original order will be kept.
    std::priority_queue<Node*, std::vector<Node*>, criticalCmpForMad> readyList;

    // Nodes with their predecessors scheduled are pushed into the preReadyQueue
    // They only get pushed into the real readyList if they are ready to issue,
    // that is their earliest cycle is >= than the current schedule cycle.
    std::priority_queue<Node*, std::vector<Node*>, earlyCmp> preReadyQueue;

    auto updateForSucc = [&](Node* scheduled, std::priority_queue<Node*, std::vector<Node*>, earlyCmp>* preReadyQueue)
    {
        for (auto& curSucc : scheduled->succs)
        {
            Node* succ = curSucc.getNode();
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
                uint32_t latencyToAdd = curSucc.getLatency();
                uint32_t earliestNew = 0;
                latencyToAdd = latencyToAdd > scheduled->getOccupancy() ? latencyToAdd : scheduled->getOccupancy();
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
                preReadyQueue->push(succ);
            }
        }
    };

    auto scheduleSingleInst = [&](window2xSP* curBlock, std::vector<Node*> &popped, uint32_t *currCycle)
    {
        // Push back the nodes in current block to ready list
        for (auto node : curBlock->instList)
        {
            if (node != nullptr)
            {
                readyList.push(node);
            }
        }

        // push back the nodes in popped list to ready list
        for (auto node : popped)
        {
            readyList.push(node);
        }

        //Only schedule single instruction
        Node* scheduled = readyList.top();
        readyList.pop();
        schedule->scheduledNodes.push_back(scheduled);
        //Update succ
        updateForSucc(scheduled, &preReadyQueue);
        scheduled->schedTime = *currCycle;
        *currCycle += scheduled->getOccupancy();
    };

    collectRoots();
    for (auto N : Roots) {
        preReadyQueue.push(N);
    }

    // The scheduler's clock.
    // This counter is updated in each loop iteration and its
    // final value represents number of cycles the kernel would
    // take to execute on the GPU as per the model.
    uint32_t currCycle = 0;

    //The blocks are used for the 2xDP and 2xSP instruction block, which will be scheduled at the same time
    //Since the atomic is set according to the dependence of two contigious blocks,
    //we keep two blocks with two block pointers to use them alternatively.
    window2xSP madBlock1(kernel);
    window2xSP madBlock2(kernel);
    window2xSP *curBlock = &madBlock1;
    window2xSP *lastBlock = &madBlock2;

    // Keep scheduling until both readyList or preReadyQueue contain instrs.
    while (!(readyList.empty() && preReadyQueue.empty()))
    {
        Node* readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
        // While non empty, move nodes from preReadyQueue to readyList
        while (readyCandidate)
        {
            readyList.push(readyCandidate);
            preReadyQueue.pop();
            readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
        }
        // Nothing to issue at this cycle, increment scheduler clock :)
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

        // Store the node popped from readyList but not in block
        std::vector<Node*> popped;

        // Traverse the readyList to try to build the 2xDP/2xSP block
        int searchSize = (int)readyList.size();
        for (int i = 0; i < searchSize; ++i)
        {
            bool isAddedToBlock = false;  // Used to see if the node has been added into block
            Node* curNode = readyList.top();

            // On PVC_XT+ platforms, check if the instruction can be added into the block
            if ((curNode->getInstructions()->size() == 1) && getBuilder()->has2xDP() && curBlock->canAddToBlock2xDP(curNode))
            {
                readyList.pop();
                curBlock->push(curNode);
                isAddedToBlock = true;

                // Current block is full
                if (curBlock->isBlockFull())
                {
                    break;
                }
            }


            // Current instruction can't be added into current block, add it into the popped
            if (!isAddedToBlock)
            {
                readyList.pop();
                popped.push_back(curNode);
            }
        }

        // Check if current block is a good block
        if (curBlock->isGoodBlock())
        {
            // Try to see if we can schedule current block. If pre-ready queue updated by the block scheduling is empty, which means
            // some instructions in popped list depeneded by next block should be scheduled firstly. Then schedule current block.
            // Considering below case:
            //    mad (16)             r5.0<1>:df  r92.0<1;0>:df  r45.0<1;0>:df  r84.0<0;0>:df
            //    mad (16)             r7.0<1>:df  r96.0<1;0>:df  r45.0<1;0>:df  r84.1<0;0>:df
            //    mov (32)             r86.0<1>:d  r17.0<1;1,0>:d
            //    mad (16)             r9.0<1>:df  r100.0<1;0>:df  r45.0<1;0>:df  r84.2<0;0>:df
            //    mad (16)             r11.0<1>:df  r104.0<1;0>:df  r45.0<1;0>:df  r84.3<0;0>:df
            //
            //    mad (16)             r5.0<1>:df  r5.0<1;0>:df  r108.0<1;0>:df  r86.0<0;0>:df
            //    mad (16)             r7.0<1>:df  r7.0<1;0>:df  r108.0<1;0>:df  r86.1<0;0>:df
            //    mad (16)             r9.0<1>:df  r9.0<1;0>:df  r108.0<1;0>:df  r86.2<0;0>:df
            //    mad (16)             r11.0<1>:df  r11.0<1;0>:df  r108.0<1;0>:df  r86.3<0;0>:df
            // The last 4 mad depends on previous 4 mad and mov. In current block, we have built the block successfully
            // which includes the first 4 instructions, and the popped list has the mov instruction. If shcedule
            // the block now, then the next sheduled instruction will be the mov as the last 4 mad wouldn't be
            // sheduled before all the previous 5 instructions scheduled. So the group of blocks will be broken.
            // Todo: Is there better solution here?

            // Schedule instructions in popped list
            for (auto node : popped)
            {
                schedule->scheduledNodes.push_back(node);
                updateForSucc(node, &preReadyQueue);
                node->schedTime = currCycle;
                currCycle += node->getOccupancy();
            }

            // Schedule instructions in block together
            for (auto node : curBlock->instList)
            {
                schedule->scheduledNodes.push_back(node);
                updateForSucc(node, &preReadyQueue);
                node->schedTime = currCycle;
                currCycle += node->getOccupancy();
            }

            window2xSP* tmpBlock = lastBlock;
            lastBlock = curBlock;
            curBlock = tmpBlock;
            curBlock->clean();
            curBlock->setPreBlock(lastBlock);
        }
        else // Not good block
        {
            scheduleSingleInst(curBlock, popped, &currCycle);

            // clean the blocks
            lastBlock->clean();
            curBlock->clean();
        }
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

    auto updateForSucc = [&](Node* scheduled, std::priority_queue<Node*, std::vector<Node*>, earlyCmp>* preReadyQueue)
    {
        for (auto& curSucc : scheduled->succs)
        {
            Node* succ = curSucc.getNode();
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
                uint32_t latencyToAdd = curSucc.getLatency();
                uint32_t earliestNew = 0;
                latencyToAdd = latencyToAdd > scheduled->getOccupancy() ? latencyToAdd : scheduled->getOccupancy();
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
                preReadyQueue->push(succ);
            }
        }
    };

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
        if (lastScheduled &&
            kernel->fg.builder->hasReadSuppression() &&
            getOptions()->getuInt32Option(vISA_ReadSuppressionDepth) != 0 &&
            (readyList.size() > 0))
        {
            if (lastScheduled->getInstructions()->size() == 1)
            {
                G4_INST* inst = lastScheduled->getInstructions()->front();
                if (inst->opcode() == G4_mad ||
                    inst->opcode() == G4_dp4a)
                {
                    std::vector<Node*> popped;
                    const int searchSize = std::min((int)getOptions()->getuInt32Option(vISA_ReadSuppressionDepth), (int)readyList.size());
                    for (int i = 0; i < searchSize; ++i)
                    {
                        Node* next = readyList.top();
                        if ((next->getInstructions()->size() == 1))
                        {
                            G4_INST* nextInst = next->getInstructions()->front();
                            readyList.pop();
                            if ((nextInst->opcode() == G4_mad ||
                                nextInst->opcode() == G4_dp4a) &&
                                hasReadSuppression(inst, nextInst, false))
                            {
                                readyList.push(scheduled);
                                scheduled = next;
                                break;
                            }
                            else
                            {
                                popped.push_back(next);
                            }
                        }
                    }
                    for (auto nodes : popped)
                    {
                        readyList.push(nodes);
                    }
                }
            }
        }
        // try to avoid bank conflict for Xe
        // Such as in following case:
        // r40 and r56 are from same bundle and have conflict
        // scheduling next instruction up can avoid the conflit.
        //mad r10   r20  r33 r40        mad r10   r20  r33 r40
        //mad r12   r56  r61  r70  -->  mad r14   r58 r 63  r72
        //mad r14   r58 r 63  r72       mad r12   r56  r61  r70
        else if (kernel->fg.builder->hasEarlyGRFRead() &&
            lastScheduled && lastScheduled->hasConflict(scheduled) &&
            (readyList.size() > 0))
        {
            std::vector<Node*> popped;
            const int searchSize = std::min(3, (int)readyList.size());
            for (int i = 0; i < searchSize; ++i)
            {
                Node* next = readyList.top();
                readyList.pop();
                if (!lastScheduled->hasConflict(next))
                {
                    readyList.push(scheduled);
                    scheduled = next;
                    break;
                }
                else
                {
                    popped.push_back(next);
                }
            }
            for (auto nodes : popped)
            {
                readyList.push(nodes);
            }
        }
        else

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
        // For write combine feature
        else if (kernel->fg.builder->hasWriteCombine() &&
            getOptions()->getOption(vISA_writeCombine) == true &&
            scheduled && scheduled->getInstructions()->front()->opcode() == G4_mov &&
            readyList.size() > 0)
        {
            windowWriteCombine block;
            if (block.isWriteCombineCandidate(scheduled, schedule->getBB()))
            {
                block.push(scheduled);

                const int searchSize = (int)readyList.size();

                // build the write combine block from other nodes of readyList
                for (int i = 0; i < searchSize; i++)
                {
                    Node* next = readyList.top();
                    if (next->getInstructions()->size() == 1 && block.isWriteCombineCandidate(next, schedule->getBB()))
                    {
                        readyList.pop();
                        block.push(next);
                    }
                    else
                    {
                        break;
                    }
                }

                bool isBlockScheduled = false;
                while (block.getInstList().size() > 1)
                {
                    if (block.isGoodBlock())
                    {
                        // add atomic to the instructions except for the last one in the block
                        std::vector<Node*> instList = block.getInstList();
                        for (size_t index = 0; index < instList.size() - 1; index++)
                        {
                            instList[index]->getInstructions()->front()->setOptionOn(InstOpt_Atomic);
                        }

                        // schedule together
                        for (auto node : instList)
                        {
                            schedule->scheduledNodes.push_back(node);
                            updateForSucc(node, &preReadyQueue);
                            scheduled = node;
                            scheduled->schedTime = currCycle;
                            currCycle += scheduled->getOccupancy();
                            lastScheduled = scheduled;
                        }
                        isBlockScheduled = true;
                        break;
                    }
                    else // not a good block
                    {
                        // pop up the last node from block, and try to see if it is a good block
                        Node* tmpNode = block.getInstList().back();
                        block.pop();
                        readyList.push(tmpNode);
                        continue;
                    }
                }

                if (isBlockScheduled)
                {
                    continue;
                }
            }
        }

        assert(scheduled && "Must have found an instruction to schedule by now");

        // Append the scheduled node to the end of the schedule.
        schedule->scheduledNodes.push_back(scheduled);
        lastScheduled = scheduled;

        // Set the cycle at which this node is scheduled.
        scheduled->schedTime = currCycle;

        updateForSucc(scheduled, &preReadyQueue);

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
        G4_SendDesc *msgDesc = inst->getMsgDesc();
        if (msgDesc->isRead())
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
            inst->asSendInst()->emit_send(std::cerr);
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
    std::string fileName = std::string(name) + "." + appendix + ".dot";
    std::ofstream ofile(fileName, std::ios::out);
    if (!ofile)
    {
        MUST_BE_TRUE(false, "[Scheduling]:ERROR: Cannot open file " <<
            fileName << ", dump failed." << std::endl);
    }

    ofile << "digraph " << name << " {" << std::endl;
    ofile << "\n" << "\t// Setup\n";
    ofile << "\tsize = \"80, 100\";\n";
    ofile << "\n" << "\t// Nodes\n";
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
            std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1,'<'), '[');
            std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1, '>'), ']');
            std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1,'{'), '[');
            std::replace_if(dotStr.begin(), dotStr.end(), std::bind(std::equal_to<char>(), std::placeholders::_1,'}'), ']');

            ofile << dotStr;
            ofile << "\\l";
            ofile << "} \"];" << std::endl;
        }
    }
    ofile << "\n" << "\t// Edges\n";

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
        out << "\n";
        for (uint32_t i = 0; i < (uint32_t)scheduledNodes.size(); i++) {
            for (G4_INST *inst : *scheduledNodes[i]->getInstructions()) {
                out << scheduledNodes[i]->schedTime << ":\t";
                if (!inst->isSend()) {
                    inst->emit(out);
                } else {
                    inst->asSendInst()->emit_send(out);
                }
                out << "\n";
            }
        }
    }
}

//Check conflicts according to registers read in same cycle
//Two kinds conflict:
//1. multiple registers from same bundle
//2. all three souces from same bank
//Input: regCandidates are the registers which will be read in same cycle
static bool hasConflictForSchedule(int *regCandidates)
{
    int bundles[3];
    int bankSrcs[2];

    for (int i = 0; i < 2; i++)
    {
        bankSrcs[i] = 0;
    }
    for (int i = 0; i < 3; i++)
    {
        bundles[i] = -1;
    }

    for (int i = 0; i < 3; i++)
    {
        if (regCandidates[i] != -1)
        {
            int bundleID = regCandidates[i] % 16;  //8 bundles for each bank
            int bankID = regCandidates[i] % 2;

            bundles[i] = bundleID;
            bankSrcs[bankID]++;
        }
    }

    //If there is bundle conflict between the sources from different instructions
    //Note that the instruction internal conflict is not considered here.
    if (bundles[2] != -1 &&
        (bundles[2] == bundles[0] ||
        bundles[2] == bundles[1]))
    {
        return true;
    }

    //If all three sources are from same bank
    if ((bankSrcs[0] > 2 ||
        bankSrcs[1] > 2))
    {
        return true;
    }

    return false;
}

//Xe check BC between adjacent instructions (this, node2)
//Scheduling can only solve the bank conflict between adjacent instructions
//The src0 of second instruction is read together with src1 (and src2) of the first instruction.
//We only handle the situation that first instruction is a three source instruction.
bool Node::hasConflict(Node* node2)
{
    G4_INST *inst1 = getInstructions()->front();
    G4_INST *inst2 = node2->getInstructions()->front();

    if (inst1->isSend() || inst2->isSend())
    {
        return false;
    }

    //Don't consider two source instructions
    if (inst1->getNumSrc() != 3)
    {
        return false;
    }

    int prevInstRegs[2][G4_MAX_SRCS];
    int currInstReg;
    int prevInstExecSize[G4_MAX_SRCS];
    int firstRegCandidate[G4_MAX_SRCS];
    int candidateNum = 0;

    //Get the sources of current instruction
    bool instSplit = false;
    const IR_Builder& irb = inst1->getBuilder();
    for (int i = 0; i < inst1->getNumSrc(); i++)
    {
        prevInstRegs[0][i] = -1;
        prevInstRegs[1][i] = -1;
        G4_Operand* srcOpnd = inst1->getSrc(i);
        if (srcOpnd)
        {
            if (srcOpnd->isSrcRegRegion() &&
                srcOpnd->asSrcRegRegion()->getBase() &&
                srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                prevInstExecSize[i] = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
                if (baseVar->isGreg()) {
                    uint32_t byteAddress = srcOpnd->getLinearizedStart();
                    prevInstRegs[0][i] = byteAddress / irb.numEltPerGRF<Type_UB>();

                    if (prevInstExecSize[i] > 32)
                    {
                        prevInstRegs[1][i] = prevInstRegs[0][i] + (prevInstExecSize[i] + irb.numEltPerGRF<Type_UB>() - 1) / irb.numEltPerGRF<Type_UB>() - 1;
                        instSplit = true;
                    }
                    else
                    {
                        //The same regsiter is reused in both SIMD8 instructions
                        prevInstRegs[1][i] = prevInstRegs[0][i];
                    }
                }
            }
        }
    }

    if (!instSplit)
    {
        if (prevInstRegs[0][1] != -1)
        {
            firstRegCandidate[candidateNum] = prevInstRegs[0][1];
            candidateNum++;
        }
        if (prevInstRegs[0][2] != -1)
        {
            firstRegCandidate[candidateNum] = prevInstRegs[0][2];
            candidateNum++;
        }
    }
    else    //For SIMD16 and SIMD32, if the GRF1 of src1 or src2 of inst 1 is GRF regsiter
    {
        if (prevInstRegs[1][1] != -1)
        {
            firstRegCandidate[candidateNum] = prevInstRegs[1][1];
            candidateNum++;
        }
        if (prevInstRegs[1][2] != -1)
        {
            firstRegCandidate[candidateNum] = prevInstRegs[1][2];
            candidateNum++;
        }
    }

    //Get the src0 of the second instruction node2
    currInstReg = -1;
    G4_Operand * srcOpnd = inst2->getSrc(0);
    if (srcOpnd)
    {
        if (srcOpnd->isSrcRegRegion() &&
            srcOpnd->asSrcRegRegion()->getBase() &&
            srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
        {
            G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
            if (baseVar->isGreg()) {
                uint32_t byteAddress = srcOpnd->getLinearizedStart();
                firstRegCandidate[candidateNum] = byteAddress / irb.numEltPerGRF<Type_UB>();
                candidateNum++;
            }
        }
    }
    else
    {
        return false;
    }

    return hasConflictForSchedule(firstRegCandidate);
}


