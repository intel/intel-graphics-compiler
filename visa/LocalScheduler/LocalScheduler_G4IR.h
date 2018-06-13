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

#ifndef _LOCALSCHEDULER_H_
#define _LOCALSCHEDULER_H_

#include <string>
#include <set>
#include <bitset>
#include "../Mem_Manager.h"
#include "../FlowGraph.h"
#include "../BuildIR.h" // add IR_Builder and G4_Kernel objects to support the exit code patch and combined kernel
#include "../Gen4_IR.hpp"
#include "../Timer.h"
#include "../BitSet.h"
#include <vector>
#include "LatencyTable.h"
#include "Dependencies_G4IR.h"

//To be comptabile with send cycles, don't normalized them to 1
#define UNCOMPR_LATENCY  2    // Latency of an uncompressed instruction
#define COMPR_LATENCY    4    // Latency of a compressed instruction
#define ACC_BUBBLE       4    // Accumulator back-to-back stall
#define IVB_PIPELINE_LENGTH  14
#define EDGE_LATENCY_MATH 22
#define EDGE_LATENCY_MATH_TYPE2 30
#define EDGE_LATENCY_SEND_WAR 36

namespace vISA {

class Node;
class DDD;
class G4_BB_Schedule;
class LocalScheduler;
class RPE;

class Edge {
    // The node at the end of this edge
    Node* node;
    // Type of Dependence (RAW, WAW, WAR, etc.)
    DepType type;
    // The set of variables that are live across this edge
    uint32_t latency;
public:
    Edge(Node *Node, DepType Type, uint32_t Latency) {
        node = Node;
        type = Type;
        latency = Latency;
    }
    Node *getNode() const { return node; }
    DepType getType() const { return type; }
    void setType(DepType newType) { type = newType; }
    uint32_t getLatency(void) const { return latency; }
    void setLatency(uint32_t newLatency) { latency = newLatency; }
};

typedef std_arena_based_allocator<Edge> Edge_Allocator;
typedef std::vector<Edge> EdgeVector;

class Node
{
    // Unique ID of the node.
    unsigned nodeID;

    // LIR instruction pointer
    std::list<G4_INST *> instVec;

    // Longest distance to the end of the DAG.
    int priority;

    // Earliest time an instruction can be issue.
    uint32_t earliest;

    // Number of consecutive cycles this instruction occupies in the pipeline
    // (This is typically 2 for SIMD8 and 4 for SIMD16 8 for SIMD32)
    // This is *NOT* the dependency latency. That is part of the Edge.
    uint16_t occupancy;

    // Indicates whether or not this node is a barrier (NONE, SEND, CONTROL).
    DepType barrier;

    Node *lastSchedPred;

    // Indicates that this Node is writing to a subreg.
    // This is used to avoid WAW hazzards during scheduling.
    int wSubreg;

    // due to coalescing we may end up with dead node, and since we use a vector
    // it's not easy to delete it, so we just mark the node as dead.
    // it's not needed for correctness as a dead node has no pred/succ and won't be scheduled,
    // we just leave it for debugging
    bool m_isDead = false;

public:
    static const uint32_t SCHED_CYCLE_UNINIT = UINT_MAX;
    static const int NO_SUBREG = INT_MAX;
    static const int PRIORITY_UNINIT = -1;

    // WARNING!!!: hasPreds() will return the right value ONLY before
    // the node's predecessors get scheduled (we are reusing the element
    // predsNotScheduled for two things to save memory).
    bool hasPreds() { return predsNotScheduled != 0;  };
    unsigned getNodeID() const{ return nodeID; };

    bool isTransitiveDep(Node *edgeDst);
    bool hasTransitiveEdgeToBarrier;

    uint32_t schedTime;

    // Number of predecessor nodes not scheduled
    uint16_t predsNotScheduled;

    // A list of instruction's predecessors
    EdgeVector preds;

    // A list of instruction's successors
    EdgeVector succs;
    void dump();

public:
    /* Constructor */
    Node(unsigned, G4_INST*, Edge_Allocator& depEdgeAllocator,
        const LatencyTable &LT);
    ~Node()
    {
        if (succs.size() > 0)
        {
            succs.clear();
        }
    }
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
    const std::list<G4_INST *> *getInstructions() const { return &instVec; }
    DepType isBarrier() const { return barrier; }
    void MarkAsUnresolvedIndirAddressBarrier() {
      barrier = INDIRECT_ADDR_BARRIER;
    }
    DepType isLabel() const {
        for (G4_INST *inst : instVec) {
            if (inst->isLabel()) {
                assert(instVec.size() == 1 &&
                    "Should not have label + other instr in one node");
                return DEP_LABEL;
            }
        }
        return NODEP;
    }

    uint16_t getOccupancy() { return occupancy; }
    uint32_t getEarliest() const { return earliest; }
    int getPriority() const { return priority; }
    void setWritesToSubreg(int reg) { wSubreg = reg; }
    int writesToSubreg() { return wSubreg; }
    void addPairInstr(G4_INST *inst) { instVec.push_back(inst); }
    void deletePred(Node *pred);
    bool isDead() const { return m_isDead; }
    void setDead() { m_isDead = true; }

    friend class DDD;
    friend class G4_BB_Schedule;
};

typedef std::vector<Node *> NODE_VECT;
typedef std::vector<Node *>::iterator NODE_VECT_ITER;
typedef std::list<Node *> NODE_LIST;
typedef std::list<Node *>::iterator NODE_LIST_ITER;

// The mask describes the range of data touched by a bucket.
struct Mask {
    unsigned short LeftB;
    unsigned short RightB;
    bool nonContiguousStride;
    Mask() : LeftB(0), RightB(0), nonContiguousStride(false) { ; }
    Mask(unsigned short LB, unsigned short RB, bool NCS)
        : LeftB(LB), RightB(RB), nonContiguousStride(NCS) {}

    bool killsBucket(unsigned int bucket) const {
        unsigned short bucketLB = bucket * G4_GRF_REG_NBYTES;
        unsigned short bucketRB = (bucket + 1) * G4_GRF_REG_NBYTES - 1;
        return !nonContiguousStride && LeftB <= bucketLB && RightB >= bucketRB;
    }

    bool kills(const Mask &mask2) const {
        return LeftB <= mask2.LeftB && RightB >= mask2.RightB;
    }

    bool hasOverlap(const Mask &mask2) const {
        return LeftB <= mask2.RightB && RightB >= mask2.LeftB;
    }
};

// This is a live node hanging from the bucket array.
// It represents an access to the bucket of type described by opndNum (R/W)
struct BucketNode {
    // The DAG node that this access belongs to
    Node *node;
    // Used to track whether it is read/write
    Gen4_Operand_Number opndNum;
    // The mask helps us track the dependences at a byte granularity.
    Mask mask;
    BucketNode(Node *node1, const Mask &mask1, Gen4_Operand_Number opndNum1)
        : node(node1), mask(mask1), opndNum(opndNum1) {}
};

typedef std::vector<BucketNode *> BUCKET_VECTOR;
typedef BUCKET_VECTOR::iterator BUCKET_VECTOR_ITER;

// This is the head node from which the list of live nodes hangs from.
// There is a single head node per bucket.
struct BucketHeadNode {
    // The list of live nodes hanging from this head node.
    BUCKET_VECTOR *bucketVec;
    // This is for future use. We can use it as an aggregate mask to avoid
    // searching through the list.
    Mask mask;
};

// Describes a single bucket access
struct BucketDescr {
    // This is the index into the bucket array. For GRFs it is the GRF num.
    int bucket;
    // This is used to tell whether this bucket is read/written
    Gen4_Operand_Number operand;
    // The mask helps us track the dependences at a byte granularity.
    Mask mask;
    BucketDescr(int Bucket, const Mask &Mask, Gen4_Operand_Number Operand)
        : bucket(Bucket), mask(Mask), operand(Operand) { ; }
};

class DDD {
    std::vector<Node *> allNodes;
    Mem_Manager &mem;
    Edge_Allocator depEdgeAllocator;
    int HWthreadsPerEU;
    const LatencyTable LT;

    // Counter that holds num of sends scheduled by
    // list scheduler just before current instruction.
    uint32_t numSendsScheduled;

    int GRF_BUCKET;
    int ACC_BUCKET;
    int FLAG0_BUCKET;
    int FLAG1_BUCKET;
    int A0_BUCKET;
    int SEND_BUCKET;
    int SCRATCH_SEND_BUCKET;
    int OTHER_ARF_BUCKET;
    int TOTAL_BUCKETS;
    int totalGRFNum;
    bool useMTLatencies;
    G4_Kernel* kernel;

public:
    typedef std::pair<G4_INST *, G4_INST *> instrPair_t;
    typedef std::vector<instrPair_t> instrPairVec_t;
    NODE_LIST Nodes, Roots;
    NODE_VECT pstOrder, originalOrder;
    void moveDeps(Node *fromNode, Node *toNode);
    uint32_t numOfPairs;
    void pairTypedWriteOrURBWriteNodes(G4_BB *bb);

    DDD(Mem_Manager &m, G4_BB *bb, const Options *options,
        const LatencyTable &lt, G4_Kernel *k);
    ~DDD()
    {
        if (Nodes.size())
        {
            for (NODE_LIST_ITER nIter = Nodes.begin(); nIter != Nodes.end(); nIter++)
            {
                Node *n = (*nIter);
                n->~Node();
            }
            Nodes.clear();
        }

        pstOrder.clear();
        originalOrder.clear();
    }
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
    void InsertRoot(Node *root) { Roots.push_back(root); }
    void InsertNode(Node *node) { Nodes.push_back(node); }
    void dumpNodes(G4_BB *bb);
    void dumpDagDot(G4_BB *bb);
    uint32_t listSchedule(G4_BB_Schedule*);
    void setPriority(Node *pred, const Edge &edge);
    void createAddEdge(Node* pred, Node* succ, DepType d);
    void  DumpDotFile(const char*, const char*);

    void getBucketsForOperand(G4_INST *inst, Gen4_Operand_Number opnd_num,
                              G4_Operand *opnd,
                              std::vector<BucketDescr> &buckets);
    // Returns true if instruction has any indirect operands (dst or src)
    bool getBucketDescrs(G4_INST *inst, std::vector<BucketDescr> &bucketDescrs);

    const Options *m_options;

    uint32_t getNumSendsScheduled() { return numSendsScheduled; }
    void recordConsecutiveSendsScheduled(uint32_t howmany) {
      numSendsScheduled = howmany;
    }

    uint32_t getEdgeLatency_old(Node *node, DepType depT);
    uint32_t getEdgeLatency_new(Node *node, DepType depT);
    uint32_t getEdgeLatency(Node *node, DepType depT);
    Mem_Manager *get_mem() { return &mem; }
};

class G4_BB_Schedule {
    Mem_Manager &mem;
    G4_BB *bb;
    DDD *ddd;
    const Options *m_options;
    G4_Kernel *kernel;
    uint32_t optimumConsecutiveSends;
    void setOptimumConsecutiveSends();

public:
    std::vector<Node *> scheduledNodes;
    unsigned curINum;
    unsigned lastCycle;
    unsigned sendStallCycle;
    unsigned sequentialCycle;

    // Constructor
    G4_BB_Schedule(G4_Kernel *kernel, Mem_Manager &m, G4_BB *bb, int t1,
                   int t2, uint32_t &totalCycle, const Options *options,
                   const LatencyTable &LT);
    void *operator new(size_t sz, Mem_Manager &m){ return m.alloc(sz); }
    // Dumps the schedule
    void emit(std::ostream &);
    uint32_t getOptimumConsecutiveSends() { return optimumConsecutiveSends; }
    void dumpSchedule(G4_BB *bb);
    G4_BB *getBB() const { return bb; };
    G4_Kernel *getKernel() const { return kernel; }
};

class LocalScheduler {
    FlowGraph &fg;
    Mem_Manager &mem;

    // send latencies are now defined in FFLatency in LIR.cpp
    void EmitNode(Node *);
    void isolateBarrierBBs();

public:
    LocalScheduler(FlowGraph &flowgraph, Mem_Manager &m)
        : fg(flowgraph), mem(m) {}
    void localScheduling();
};

class preRA_Scheduler {
public:
    preRA_Scheduler(G4_Kernel& k, Mem_Manager& m, RPE* rpe);
    ~preRA_Scheduler();
    bool run();

private:
    G4_Kernel& kernel;
    Mem_Manager& mem;
    RPE* rpe;
    Options* m_options;
};

} // namespace vISA

#endif // _LOCALSCHEDULER_H_
