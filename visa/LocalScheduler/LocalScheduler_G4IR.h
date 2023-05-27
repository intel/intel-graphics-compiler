/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _LOCALSCHEDULER_H_
#define _LOCALSCHEDULER_H_

#include "../BitSet.h"
#include "../BuildIR.h" // add IR_Builder and G4_Kernel objects to support the exit code patch and combined kernel
#include "../FlowGraph.h"
#include "../G4_IR.hpp"
#include "../Mem_Manager.h"
#include "../Timer.h"
#include "Dependencies_G4IR.h"
#include "LatencyTable.h"

#include "llvm/Support/Allocator.h"

#include <list>
#include <set>
#include <string>
#include <vector>

#define THREE_SOURCE_BLOCK_HERISTIC 0.5

#define FP_BLOCK_SEND_HERISTIC 0.1
#define FP_MIN_INST_NUM                                                        \
  12 // at least 3 blocks(4 instructions per block) for either 2XDP or 2XSP
namespace vISA {

class Node;
class DDD;
class G4_BB_Schedule;
class LocalScheduler;
class RPE;
class PointsToAnalysis;

class Edge {
  // The node at the end of this edge
  Node *node;
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
using NodeAlloc = llvm::SpecificBumpPtrAllocator<Node>;

class Node {
  // Unique ID of the node.
  unsigned nodeID;

  // LIR instruction pointer
  std::list<G4_INST *> instVec;

  // Longest distance to the end of the DAG.
  int priority = PRIORITY_UNINIT;

  // Earliest time an instruction can be issue.
  uint32_t earliest = 0;

  // Number of consecutive cycles this instruction occupies in the pipeline
  // (This is typically 2 for SIMD8 and 4 for SIMD16 8 for SIMD32)
  // This is *NOT* the dependency latency. That is part of the Edge.
  uint16_t occupancy = 0;

  // Indicates whether or not this node is a barrier (NONE, SEND, CONTROL).
  DepType barrier = DepType::NODEP;

  Node *lastSchedPred = nullptr;

  // Indicates that this Node is writing to a subreg.
  // This is used to avoid WAW hazzards during scheduling.
  int wSubreg = NO_SUBREG;

  bool hasTransitiveEdgeToBarrier = false;

public:
  static const uint32_t SCHED_CYCLE_UNINIT = UINT_MAX;
  static const int NO_SUBREG = INT_MAX;
  static const int PRIORITY_UNINIT = -1;

  unsigned getNodeID() const { return nodeID; };

  uint32_t schedTime = 0;

  // Number of predecessor nodes not scheduled
  uint16_t predsNotScheduled = 0;

  // A list of instruction's predecessors
  EdgeVector preds;

  // A list of instruction's successors
  EdgeVector succs;
  void dump();

public:
  /* Constructor */
  Node(unsigned, G4_INST *, Edge_Allocator &depEdgeAllocator,
       const LatencyTable &LT);
  ~Node() {}
  void *operator new(size_t sz, NodeAlloc &Allocator) {
    return Allocator.Allocate(sz / sizeof(Node));
  }
  const std::list<G4_INST *> *getInstructions() const { return &instVec; }
  DepType isBarrier() const { return barrier; }
  void MarkAsUnresolvedIndirAddressBarrier() {
    barrier = INDIRECT_ADDR_BARRIER;
  }
  DepType isLabel() const {
    for (G4_INST *inst : instVec) {
      if (inst->isLabel()) {
        vISA_ASSERT(instVec.size() == 1,
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
  void clear() { instVec.clear(); }
  void deletePred(Node *pred);
  // Xe check BC between adjacent instructions
  bool hasConflict(Node *node2);

  friend class DDD;
  friend class G4_BB_Schedule;
};

using NODE_VECT = std::vector<Node *>;
using NODE_VECT_ITER = NODE_VECT::iterator;
using NODE_LIST = std::list<Node *>;
using NODE_LIST_ITER = NODE_LIST::iterator;

// The mask describes the range of data touched by a bucket.
struct Mask {
  unsigned short LeftB;
  unsigned short RightB;
  bool nonContiguousStride = false;
  G4_AccRegSel specialAcc = G4_AccRegSel::ACC_UNDEFINED;

  Mask() : LeftB(0), RightB(0) {}
  Mask(unsigned short LB, unsigned short RB, bool NCS, G4_AccRegSel Acc)
      : LeftB(LB), RightB(RB), nonContiguousStride(NCS), specialAcc(Acc) {}

  bool killsBucket(unsigned int bucket, const IR_Builder &irb) const {
    unsigned short bucketLB = bucket * irb.numEltPerGRF<Type_UB>();
    unsigned short bucketRB = (bucket + 1) * irb.numEltPerGRF<Type_UB>() - 1;
    return !nonContiguousStride && !withSpecialAcc() && LeftB <= bucketLB &&
           RightB >= bucketRB;
  }

  bool kills(const Mask &mask2) const {
    return LeftB <= mask2.LeftB && RightB >= mask2.RightB &&
           specialAcc == mask2.specialAcc;
  }

  bool withSpecialAcc() const {
    return specialAcc != G4_AccRegSel::ACC_UNDEFINED;
  }

  bool hasOverlap(const Mask &mask2) const {
    return (LeftB <= mask2.RightB && RightB >= mask2.LeftB) ||
           (mask2.withSpecialAcc() && mask2.specialAcc == specialAcc);
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
      : node(node1), opndNum(opndNum1), mask(mask1) {}
};

typedef std::vector<BucketNode *> BUCKET_VECTOR;
typedef BUCKET_VECTOR::iterator BUCKET_VECTOR_ITER;

// This is the head node from which the list of live nodes hangs from.
// There is a single head node per bucket.
struct BucketHeadNode {
  // The list of live nodes hanging from this head node.
  BUCKET_VECTOR bucketVec;
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
      : bucket(Bucket), operand(Operand), mask(Mask) {
    ;
  }
};

class DDD {
  std::vector<Node *> allNodes;
  // Used to allocate BucketNode, which is POD.
  // TODO: investigate whether dynamic allocation is actually necessary.
  Mem_Manager DDDMem;
  Edge_Allocator depEdgeAllocator;
  int HWthreadsPerEU;
  bool useMTLatencies;
  bool isThreeSouceBlock;
  bool is_2XFP_Block;
  const LatencyTable &LT;

  int GRF_BUCKET;
  int ACC_BUCKET;
  int FLAG0_BUCKET;
  int FLAG1_BUCKET;
  int FLAG2_BUCKET;
  int FLAG3_BUCKET;
  int A0_BUCKET;
  int SEND_BUCKET;
  int SCRATCH_SEND_BUCKET;
  int OTHER_ARF_BUCKET;
  int TOTAL_BUCKETS;
  int totalGRFNum;
  int totalACCNum;
  G4_Kernel *kernel;
  PointsToAnalysis &pointsToAnalysis;

  // Gather all initial ready nodes.
  void collectRoots();

public:
  typedef std::pair<Node *, Node *> instrPair_t;
  typedef std::vector<instrPair_t> instrPairVec_t;
  NODE_LIST Roots;
  NodeAlloc NodeAllocator;
  void moveDeps(Node *fromNode, Node *toNode);
  void pairTypedWriteOrURBWriteNodes(G4_BB *bb);

  bool hasReadSuppression(G4_INST *curInst, G4_INST *nextInst, BitSet &liveDst,
                          BitSet &liveSrc);
  bool hasReadSuppression(G4_INST *prevInst, G4_INST *nextInst,
                          bool multipSuppression);

private:
  bool hasSameSourceOneDPAS(G4_INST *curInst, G4_INST *nextInst,
                            BitSet &liveDst, BitSet &liveSrc);
  bool hsaSameTypesAllOperands(const G4_INST &curInst,
                               const G4_INST &nextInst) const;

public:
  DDD(G4_BB *bb, const LatencyTable &lt, G4_Kernel *k, PointsToAnalysis &p);
  ~DDD() = default;
  void dumpNodes(G4_BB *bb);
  void dumpDagDot(G4_BB *bb);
  uint32_t listScheduleForSuppression(G4_BB_Schedule *schedule);
  uint32_t listScheduleFor2xFP(G4_BB_Schedule *schedule);
  uint32_t listSchedule(G4_BB_Schedule *);
  void setPriority(Node *pred, const Edge &edge);
  void createAddEdge(Node *pred, Node *succ, DepType d);
  void DumpDotFile(G4_BB *bb);

  void getBucketsForOperand(G4_INST *inst, Gen4_Operand_Number opnd_num,
                            std::vector<BucketDescr> &buckets);
  void getBucketsForIndirectOperand(G4_INST *inst,
                                    Gen4_Operand_Number opnd_num,
                                    std::vector<BucketDescr> &BDvec);
  void getBucketDescrs(Node *inst, std::vector<BucketDescr> &bucketDescrs);

  uint32_t getEdgeLatency_old(Node *node, DepType depT);
  uint32_t getEdgeLatency(Node *node, DepType depT);
  Mem_Manager *get_mem() { return &DDDMem; }
  IR_Builder *getBuilder() const { return kernel->fg.builder; }
  const Options *getOptions() const { return kernel->getOptions(); }
  bool getIsThreeSourceBlock() { return isThreeSouceBlock; }
  bool getIs2xFPBlock() { return is_2XFP_Block; }
};

class G4_BB_Schedule {
  G4_BB *bb;
  DDD *ddd;
  G4_Kernel *kernel;
  PointsToAnalysis &pointsToAnalysis;

public:
  std::vector<Node *> scheduledNodes;
  unsigned lastCycle = 0;
  unsigned sendStallCycle = 0;
  unsigned sequentialCycle = 0;

  G4_BB_Schedule(G4_Kernel *kernel, G4_BB *bb, const LatencyTable &LT,
                 PointsToAnalysis &p);
  // Dumps the schedule
  void emit(std::ostream &);
  void dumpSchedule(G4_BB *bb);
  G4_BB *getBB() const { return bb; };
  G4_Kernel *getKernel() const { return kernel; }
  IR_Builder *getBuilder() const { return kernel->fg.builder; }
  const Options *getOptions() const { return kernel->getOptions(); }
};

class LocalScheduler {
  FlowGraph &fg;

  // send latencies are now defined in FFLatency in LIR.cpp
  void EmitNode(Node *);

public:
  LocalScheduler(FlowGraph &flowgraph) : fg(flowgraph) {}
  void localScheduling();
};

class preRA_Scheduler {
public:
  preRA_Scheduler(G4_Kernel &k, RPE *rpe);
  ~preRA_Scheduler();
  bool run();

private:
  G4_Kernel &kernel;
  RPE *rpe;
  Options *m_options;
};

class preRA_ACC_Scheduler {
public:
  preRA_ACC_Scheduler(G4_Kernel &k);
  ~preRA_ACC_Scheduler();
  bool run();

private:
  G4_Kernel &kernel;
  RPE *rpe;
  Options *m_options;
};

class preRA_RegSharing {
public:
  preRA_RegSharing(G4_Kernel &k, RPE *rpe);
  ~preRA_RegSharing();
  bool run();

private:
  G4_Kernel &kernel;
  RPE *rpe;
};
// Restrictions of candidate for 2xDP:
//    1, Only support SIMD16 DF mad with M0
//    2, Inst must not have predicate/cond mod
//    3, The first use of the dst must have DF type as well
static bool instCanUse2xDP(G4_INST *inst) {
  if (inst->opcode() == G4_mad && (((inst->getExecSize() == g4::SIMD16) &&
      (inst->getDst()->getType() == Type_DF)) || ((inst->getExecSize() == g4::SIMD32) &&
      (inst->getDst()->getType() == Type_F))) &&
      ((inst->getMaskOption() == InstOpt_M0) ||
       (inst->getMaskOption() == InstOpt_NoOpt)) &&
      inst->getPredicate() == nullptr &&
      inst->getCondMod() == nullptr) {
    return true;
  }
  return false;
}


class window2xSP {
public:
  std::vector<Node *> instList;
  std::vector<int> dstGRF;
  std::vector<int> srcGRF;
  int instGRFSize = 0;
  G4_Type instType = Type_UNDEF;
  G4_Kernel *kernel;
  window2xSP *preBlock = nullptr;

  window2xSP(G4_Kernel *k) : kernel(k) {
    instGRFSize = 0;
    instType = Type_UNDEF;
    preBlock = nullptr;
    dstGRF.clear();
    srcGRF.clear();
    instList.clear();
  }

  ~window2xSP() {
    instGRFSize = 0;
    instType = Type_UNDEF;
    preBlock = nullptr;
    dstGRF.clear();
    srcGRF.clear();
    instList.clear();
  }

  void clean() {
    instGRFSize = 0;
    instType = Type_UNDEF;
    preBlock = nullptr;
    dstGRF.clear();
    srcGRF.clear();
    instList.clear();
  }

  void setPreBlock(window2xSP *block) {
    vISA_ASSERT(block && block->instList.size() != 0,
           "previous block has no instructions");
    preBlock = block;
  }

  // Push new instruction into block, update the register size of the block at
  // the same time
  void push(Node *node) {
    G4_INST *inst = node->getInstructions()->front();
    unsigned grfSize = inst->getBuilder().getGRFSize();
    int dstReg = int(inst->getDst()->getLinearizedStart() / grfSize);
    int src0Reg = int(inst->getSrc(0)->getLinearizedStart() / grfSize);

    // set instruction GRF size
    if (instGRFSize == 0) {
      instGRFSize = (inst->getDst()->getLinearizedEnd() -
                     inst->getDst()->getLinearizedStart() + 1) /
                    grfSize;
    }

    // set instruction type
    if (instType == Type_UNDEF) {
      instType = inst->getDst()->getType();
    }

    if (!preBlock) { // First block in group, keep the original instruction
                     // order
      instList.push_back(node);
      for (int i = 0; i < instGRFSize; i++) {
        dstGRF.push_back(dstReg);
        srcGRF.push_back(src0Reg);
        dstReg++;
        src0Reg++;
      }
    } else // Non-first block in group
    {
      // First instruction in current block
      // Currnt block and previous block may have different instruction GRF
      // size. For example:
      //     For K=0
      //     mad (16 | M0) r50.0<1>:f  r34.0<1,0>:f   r10.0<1,0>:f   r77.0<0>:f
      //     { Align1, Q1 } mad (16 | M0) r51.0<1>:f  r35.0<1,0>:f r11.0<1,0>:f
      //     r77.0<0>:f   { Align1, Q1 } mad (16 | M0) r52.0<1>:f  r36.0<1,0>:f
      //     r10.0<1,0>:f   r77.1<0>:f   { Align1, Q1 } mad (16 | M0) r53.0<1>:f
      //     r37.0<1,0>:f   r11.0<1,0>:f   r77.1<0>:f   { Align1, Q1 } mad (16 |
      //     M0) r54.0<1>:f  r38.0<1,0>:f   r10.0<1,0>:f   r77.2<0>:f   {
      //     Align1, Q1 } mad (16 | M0) r55.0<1>:f  r39.0<1,0>:f   r11.0<1,0>:f
      //     r77.2<0>:f   { Align1, Q1 } mad (16 | M0) r56.0<1>:f  r40.0<1,0>:f
      //     r10.0<1,0>:f   r77.3<0>:f   { Align1, Q1 } mad (16 | M0) r57.0<1>:f
      //     r41.0<1,0>:f   r11.0<1,0>:f   r77.3<0>:f   { Align1, Q1 } For K=1
      //     mad (16  |M0) 60.0<1>:df  50.0<1,0>:df  r12.0<1,0>:df   r78.0<0>:df
      //     { Align1, Q1 } mad (16 | M0) 62.0<1>:df  52.0<1,0>:df r12.0<1,0>:df
      //     r78.1<0>:df   { Align1, Q1 } mad (16 | M0) 64.0<1>:df  54.0<1,0>:df
      //     r12.0<1,0>:df   r78.2<0>:df   { Align1, Q1 } mad (16 |
      //     M0) 66.0<1>:df  56.0<1,0>:df  r12.0<1,0>:df   r78.3<0>:df   {
      //     Align1, Q1 } For K=2 mad (16 | M0) r34.0<1>:f  60.0<1,0>:f
      //     r14.0<1,0>:f   r79.0<0>:f   { Align1, Q1 } mad (16 | M0) r35.0<1>:f
      //     61.0<1,0>:f   r15.0<1,0>:f   r79.0<0>:f   { Align1, Q1 } mad (16 |
      //     M0) r36.0<1>:f  62.0<1,0>:f   r14.0<1,0>:f   r79.1<0>:f   { Align1,
      //     Q1 } mad (16 | M0) r37.0<1>:f  63.0<1,0>:f   r15.0<1,0>:f
      //     r79.1<0>:f   { Align1, Q1 } mad (16 | M0) r38.0<1>:f  64.0<1,0>:f
      //     r14.0<1,0>:f   r79.2<0>:f   { Align1, Q1 } mad (16 | M0) r39.0<1>:f
      //     65.0<1,0>:f   r15.0<1,0>:f   r79.2<0>:f   { Align1, Q1 } mad (16 |
      //     M0) r40.0<1>:f  66.0<1,0>:f   r14.0<1,0>:f   r79.3<0>:f   { Align1,
      //     Q1 } mad (16 | M0) r41.0<1>:f  67.0<1,0>:f   r15.0<1,0>:f
      //     r79.3<0>:f   { Align1, Q1 }
      vISA_ASSERT((std::max(instGRFSize, preBlock->instGRFSize) %
                  std::min(instGRFSize, preBlock->instGRFSize) ==
              0),
             "instructions in ajacent blocks can't be aligned");
      auto indexInterval = std::max(instGRFSize, preBlock->instGRFSize) /
                           std::min(instGRFSize, preBlock->instGRFSize);
      if (instList.size() == 0) {
        auto instListSize = (instGRFSize > preBlock->instGRFSize)
                                ? (preBlock->instList.size() / indexInterval)
                                : (preBlock->instList.size() * indexInterval);
        instList.resize(instListSize, nullptr);
        dstGRF.resize(preBlock->dstGRF.size(), -1);
        srcGRF.resize(preBlock->srcGRF.size(), -1);
      }

      // For non-first block, the instruction order is decided by previous block
      // The instruction whose src0 in current block and the instruction whose
      // dst in previous block have the same registers, and these two
      // instructions should have the same order in their own blocks
      unsigned grfSize = inst->getBuilder().getGRFSize();
      int src0RegStart = int(inst->getSrc(0)->getLinearizedStart() / grfSize);
      std::vector<int> src0Regs;
      for (int src0Reg = src0RegStart; src0Reg < (src0RegStart + instGRFSize);
           src0Reg++) {
        src0Regs.push_back(src0Reg);
      }
      auto it = std::search(preBlock->dstGRF.begin(), preBlock->dstGRF.end(),
                            src0Regs.begin(), src0Regs.end());
      vISA_ASSERT(it != preBlock->dstGRF.end(),
             "the src0 is not the same as the dst of previous block");
      auto dstGRFIndexPreBlock = it - preBlock->dstGRF.begin();
      auto instIndexPreBlock = dstGRFIndexPreBlock / preBlock->instGRFSize;
      auto instIndex = (instGRFSize > preBlock->instGRFSize)
                           ? (instIndexPreBlock / indexInterval)
                           : (instIndexPreBlock * indexInterval);
      this->instList[instIndex] = node;
      for (int i = 0; i < instGRFSize; i++) {
        srcGRF[dstGRFIndexPreBlock] = src0Reg;
        dstGRF[dstGRFIndexPreBlock] = dstReg;
        src0Reg++;
        dstReg++;
        dstGRFIndexPreBlock++;
      }
    }
  }

  // Check GRF size to see if the instruction can be added the block or not
  bool checkGRFSize(Node *node) {
    G4_INST *inst = node->getInstructions()->front();
    G4_Operand *newOpnd = inst->getDst();
    unsigned platGRFSize = inst->getBuilder().getGRFSize();
    unsigned GRFSize =
        (newOpnd->getLinearizedEnd() - newOpnd->getLinearizedStart() + 1) /
        platGRFSize;

    // 2xSP/2xDP instruction should be 1 or 2 GRF size in destination
    // GRF aligned
    if (!GRFSize || newOpnd->getLinearizedStart() % platGRFSize) {
      return false;
    }

    // Check if the first block is less than 2xSP register size requirement.
    // For non-first block, the size is the same as previous block, so no need
    // to check here.
    if (!preBlock && (int)(dstGRF.size() + GRFSize) > kernel->getNumAcc()) {
      return false;
    }

    if (instGRFSize != 0 && GRFSize != instGRFSize) {
      return false;
    }

    return true;
  }

  // Check if the instruction can be added into 2xDP block
  bool canAddToBlock2xDP(Node *node) {
    G4_INST *inst = node->getInstructions()->front();

    if (!instCanUse2xDP(inst)) {
      return false;
    }

    // All instructions in the block have the same GRF size
    if (!checkGRFSize(node)) {
      return false;
    }

    // For non-first blocks, the inst's src0 should have the same registers as
    // the dst of instructions in previous block
    if (preBlock && !hasSameOperand(inst)) {
      return false;
    }

    return true;
  }


  // Check if block is good or not:
  //     GRF size should be <= ACC number
  //     GRF size should be larger than 4
  //     For the non-first block, the src0 of the block should have the same
  //     registers as the dst of previous block
  bool isGoodBlock() {
    return (int)dstGRF.size() <= kernel->getNumAcc() && dstGRF.size() >= 4 &&
           checkAllInstsSrc0();
  }

  // Check if the src0 of the instruction has the same register as the dst of
  // any instruction in previous block
  bool hasSameOperand(G4_INST *inst) {
    unsigned grfSize = inst->getBuilder().getGRFSize();
    int src0RegStart = int(inst->getSrc(0)->getLinearizedStart() / grfSize);
    int tmpInstGRFSize = (this->instGRFSize == 0)
                             ? int((inst->getDst()->getLinearizedEnd() -
                                    inst->getDst()->getLinearizedStart() + 1) /
                                   grfSize)
                             : this->instGRFSize;
    std::vector<int> src0Regs;
    for (auto src0Reg = src0RegStart; src0Reg < (src0RegStart + tmpInstGRFSize);
         src0Reg++) {
      src0Regs.push_back(src0Reg);
    }

    auto itDst = std::search(preBlock->dstGRF.begin(), preBlock->dstGRF.end(),
                             src0Regs.begin(), src0Regs.end());
    auto itSrc = std::search(this->srcGRF.begin(), this->srcGRF.end(),
                             src0Regs.begin(), src0Regs.end());
    if (itDst != preBlock->dstGRF.end() && itSrc == this->srcGRF.end()) {
      return true;
    }
    return false;
  }

  // Check if the src0 of all instructions in the current block and the dst of
  // all instructions in the previous block have same registers.
  bool checkAllInstsSrc0() {
    // No need to check operand if the current block is the first block
    if (preBlock) {
      std::vector<int>::iterator itDst = preBlock->dstGRF.begin();
      std::vector<int>::iterator itSrc = this->srcGRF.begin();
      while (itDst != preBlock->dstGRF.end() && itSrc != this->srcGRF.end()) {
        auto dstReg = (*itDst);
        auto srcReg = (*itSrc);

        if (dstReg != srcReg) {
          return false;
        }
        itDst++;
        itSrc++;
      }

      if (itDst != preBlock->dstGRF.end() || itSrc != this->srcGRF.end()) {
        return false;
      }
    }

    return true;
  }

  void dump() {
    for (auto inst : instList) {
      inst->dump();
    }
  }

  // Check if block is full
  bool isBlockFull() {
    for (auto const reg : dstGRF) {
      if (reg == -1)
        return false;
    }

    return (dstGRF.size() >= kernel->getNumAcc());
  }
};

class windowWriteCombine {
  std::vector<Node *> instList;
  G4_Type dstType = Type_UNDEF;
  G4_Type srcType = Type_UNDEF;
  int dstGRF = -1;
  bool hasMixedExecSize = false;
  std::bitset<64> bytesWritten{0x0};

public:
  std::vector<Node *> &getInstList() { return instList; }

  bool isWriteCombineCandidate(Node *node, G4_BB *bb) {
    auto inst = node->getInstructions()->front();
    if (inst->opcode() != G4_mov) {
      return false;
    }

    auto dst = inst->getDst();
    // 1, dst cannot be null and cannot cross GRF boundary
    if (!dst || dst->asDstRegRegion()->isCrossGRFDst(inst->getBuilder())) {
      return false;
    }

    // 2, check dst data type: dst can only be byte type, and all insts in the
    // block have the same dst type Todo: support word and dword dst types.
    if ((dstType == Type_UNDEF && !IS_BTYPE(dst->getType())) ||
        (dstType != Type_UNDEF && dst->getType() != dstType)) {
      return false;
    }

    // 3, should have same dst GRF number.
    unsigned grfSize = inst->getBuilder().getGRFSize();
    int dstReg = int(dst->getLinearizedStart() / grfSize);
    if (dstGRF != -1 && dstReg != dstGRF) {
      return false;
    }

    // 4, for byte write combine atomic sequence, instruction must have no mask
    // as we cannot gurantee that both bytes in a word are written.
    bool isNoMaskInst = !inst->getPredicate() &&
                        (inst->isWriteEnableInst() || bb->isAllLaneActive());
    if ((IS_BTYPE(dstType) ||
         (dstType == Type_UNDEF && IS_BTYPE(dst->getType()))) &&
        !isNoMaskInst) {
      return false;
    }

    // 5, check src data type: must be byte/word/dword/float types, and all
    // insts in the block have same src type.
    auto src = inst->getSrc(0);
    if ((srcType == Type_UNDEF && !IS_BTYPE(src->getType()) &&
         !IS_WTYPE(src->getType()) && !IS_DTYPE(src->getType()) &&
         src->getType() != Type_F) ||
        (srcType != Type_UNDEF && src->getType() != srcType)) {
      return false;
    }

    // 6, check exec size: mixed exec size is OK for dw-aligned cases only
    auto execSize = inst->getExecSize();
    if (hasMixedExecSize ||
        (!instList.empty() &&
         execSize !=
             instList.back()->getInstructions()->front()->getExecSize())) {
      if (dst->getLinearizedStart() % 4 || dst->getExecTypeSize() % 4) {
        return false;
      }
    }

    // 7, no performance advantage if the dst is packed
    if (dst->getHorzStride() == 1 && execSize != 1) {
      return false;
    }

    return true;
  }

  // push the node to the end
  void push(Node *node) {
    auto inst = node->getInstructions()->front();
    auto dst = inst->getDst();
    auto src = inst->getSrc(0);
    auto grfSize = inst->getBuilder().getGRFSize();

    // set dst data type of the block
    if (dstType == Type_UNDEF) {
      dstType = dst->getType();
    }

    // set dst GRF number
    if (dstGRF == -1) {
      dstGRF = int(dst->getLinearizedStart() / grfSize);
    }

    // set src data type of the block
    if (srcType == Type_UNDEF) {
      srcType = src->getType();
    }

    // update hasMixedExecSize
    if (!hasMixedExecSize && !instList.empty()) {
      hasMixedExecSize =
          inst->getExecSize() !=
          instList.back()->getInstructions()->front()->getExecSize();
    }

    // record written bytes in the GRF
    for (int offset = dst->getLinearizedStart() % grfSize, i = 0;
         i < inst->getExecSize(); i++, offset += dst->getExecTypeSize()) {
      for (int elementOffset = 0; elementOffset < dst->getElemSize();
           elementOffset++) {
        bytesWritten.set(offset + elementOffset);
      }
    }

    instList.push_back(node);
    return;
  }

  // remove the last element
  void pop() {
    auto inst = instList.back()->getInstructions()->front();
    auto dst = inst->getDst();
    auto grfSize = inst->getBuilder().getGRFSize();

    // clear written bytes in the GRF
    for (int offset = dst->getLinearizedStart() % grfSize, i = 0;
         i < inst->getExecSize(); i++, offset += dst->getExecTypeSize()) {
      for (int elementOffset = 0; elementOffset < dst->getElemSize();
           elementOffset++) {
        bytesWritten.set(offset + elementOffset, false);
      }
    }

    instList.pop_back();
    return;
  }

  // check if current block meets below conditions:
  // 1, block size >= 2
  // 2, both bytes in a word need to be written
  bool isGoodBlock() {
    if (instList.size() < 2) {
      return false;
    }

    // both bytes in a word need to be written
    for (std::size_t i = 0; i < bytesWritten.size(); i += 2) {
      if (bytesWritten[i] ^ bytesWritten[i + 1]) {
        return false;
      }
    }

    return true;
  }
};
} // namespace vISA

#endif // _LOCALSCHEDULER_H_
