/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _SWSB_H_
#define _SWSB_H_
#include "../BitSet.h"
#include "FastSparseBitVector.h"
#include "../FlowGraph.h"
#include "../G4_IR.hpp"
#include "LocalScheduler_G4IR.h"
#include "../Mem_Manager.h"
#include "../Timer.h"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Allocator.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <bitset>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace vISA {
class SBNode;
struct SBBucketNode;
class SBDDD;
class G4_BB_SB;
class SWSB;
class PointsToAnalysis;
} // namespace vISA

// FIXME, build a table for different platforms
#define SWSB_MAX_MATH_DEPENDENCE_DISTANCE 18
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE 7u

#define TOKEN_AFTER_READ_DPAS_CYCLE 8
#define SWSB_MAX_DPAS_DEPENDENCE_DISTANCE 4
#define FOUR_THREAD_TOKEN_NUM 32

#define DEPENCENCE_ATTR(DO)                                                    \
  DO(DEP_EXPLICT)                                                              \
  DO(DEP_IMPLICIT)

enum SBDependenceAttr { DEPENCENCE_ATTR(MAKE_ENUM) };

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

typedef enum _DPAS_DEP { REP_1 = 1, REP_2 = 2, REP_4 = 4, REP_8 = 8 } DPAS_DEP;

constexpr int INVALID_ID = -1;

#define UNKNOWN_TOKEN (unsigned short)-1
#define INVALID_GRF (unsigned short)-1
#define UNINIT_BUCKET -1

namespace vISA {

struct SBDep {
  SBDep(SBNode *SBNode, DepType Type, SBDependenceAttr Attr) {
    node = SBNode;
    type = Type;
    attr = Attr;
    exclusiveNodes.clear();
  }
  SBNode *node;
  DepType type;
  SBDependenceAttr attr;
  std::vector<vISA::SBNode *> exclusiveNodes;
};

struct SBDISTDep {
  SB_INST_PIPE liveNodePipe;
  SB_INST_PIPE nodePipe;
  SB_INST_PIPE operandType;
  bool dstDep;
};

struct DistDepValue {
  unsigned short intDist : 3;
  unsigned short floatDist : 3;
  unsigned short longDist : 3;
  unsigned short mathDist : 3;
  unsigned short scalarDist : 3;
};

using SWSBTokenType = G4_INST::SWSBTokenType;
void singleInstStallSWSB(G4_Kernel *kernel, uint32_t instID, uint32_t endInstID,
                         bool is_barrier);
void forceDebugSWSB(G4_Kernel *kernel);
void forceFCSWSB(G4_Kernel *kernel);
} // namespace vISA

typedef vISA::SBDep SBDEP_ITEM;
typedef std::vector<SBDEP_ITEM> SBDEP_VECTOR;
typedef SBDEP_VECTOR::iterator SBDEP_VECTOR_ITER;

typedef vISA::SBDISTDep SBDISTDEP_ITEM;
typedef std::vector<SBDISTDEP_ITEM> SBDISTDEP_VECTOR;

typedef vISA::DistDepValue DISTDEPINFO;

// TODO: we have to leave this in the header file right now due to the huge
//       amount of implementation code in the header that refer to it. We
//       need to move all of the implementation code out of header.
namespace SWSB_global {
void setAccurateDistType(vISA::G4_INST *inst, vISA::SB_INST_PIPE depPipe);
};
namespace vISA {
typedef enum _FOOTPRINT_TYPE {
  GRF_T = 1,
  ACC_T = 2,
  FLAG_T = 4,
} FOOTPRINT_TYPE;

struct SBFootprint {
  const FOOTPRINT_TYPE fType;
  const unsigned short type;
  const unsigned short LeftB;
  const unsigned short RightB;
  unsigned short offset = 0;
  bool isPrecision = false;
  G4_INST *inst;

  // FIXME: The choice of C-style linked list seems suspect given that there are
  // many higher-level data structures we could use instead.
  // The ranges linked together to represent the possible register
  // ranges may be occupied by an operand. For indirect access,
  // non-contiguous ranges exist.
  struct SBFootprint *next = nullptr;

  SBFootprint()
      : fType(GRF_T), type((unsigned short)Type_UNDEF), LeftB(0), RightB(0),
        inst(nullptr) {
    isPrecision = false;
  }
  SBFootprint(FOOTPRINT_TYPE ft, G4_Type t, unsigned short LB,
              unsigned short RB, G4_INST *i)
      : fType(ft), type((unsigned short)t), LeftB(LB), RightB(RB), inst(i) {
    isPrecision = false;
  }
  SBFootprint(FOOTPRINT_TYPE ft, GenPrecision p, unsigned short LB,
              unsigned short RB, G4_INST *i)
      : fType(ft), type((unsigned short)p), LeftB(LB), RightB(RB), inst(i), isPrecision(true) {
  }

  void setOffset(unsigned short o) { offset = o; }

  bool hasOverlap(const SBFootprint *liveFootprint,
                  unsigned short &internalOffset) const;
  bool hasOverlap(const SBFootprint *liveFootprint, bool &isRMWOverlap,
                  unsigned short &internalOffset) const;
  bool hasGRFGrainedOverlap(const SBFootprint *liveFootprint) const;
  bool isWholeOverlap(const SBFootprint *liveFootprint) const;
  bool isSameOrNoOverlap(const SBFootprint *liveFootprint) const;
  bool hasSameType(const SBFootprint *liveFootprint) const;
};

// Bit set which is used for global dependence analysis for SBID.
// Since dependencies may come from dst and src and there may be dependence kill
// between dst and src dependencies, we use internal bit set to track the live
// of dst and src separately. Each bit map to one global SBID node according to
// the node's global ID.
struct SBBitSets {
  SparseBitVector dst;
  SparseBitVector src;

  SBBitSets() {}
  SBBitSets(SBBitSets&& other) = delete;
  SBBitSets& operator=(SBBitSets&& other) = delete;
  ~SBBitSets() {}

  void setDst(int ID, bool value) {
    if (value)
      dst.set(ID);
    else
      dst.reset(ID);
  }
  void setSrc(int ID, bool value) {
    if (value)
      src.set(ID);
    else
      src.reset(ID);
  }

  bool isEmpty() const { return dst.empty() && src.empty(); }

  bool isDstEmpty() const { return dst.empty(); }

  bool isSrcEmpty() const { return src.empty(); }

  bool isDstSet(unsigned i) const { return dst.test(i); }

  bool isSrcSet(unsigned i) const { return src.test(i); }

  SBBitSets &operator=(const SBBitSets &other) {
    dst = other.dst;
    src = other.src;
    return *this;
  }

  SBBitSets &operator=(SBBitSets &other) {
    dst = other.dst;
    src = other.src;
    return *this;
  }

  SBBitSets &operator|=(const SBBitSets &other) {
    dst |= other.dst;
    src |= other.src;
    return *this;
  }

  SBBitSets &operator&=(const SBBitSets &other) {
    dst &= other.dst;
    src &= other.src;
    return *this;
  }

  SBBitSets &operator-=(const SBBitSets &other) {
    dst = dst - other.dst;
    src = src - other.src;
    return *this;
  }

  bool operator!=(const SBBitSets &other) const {
    return (dst != other.dst) || (src != other.src);
  }
};

using SBNodeAlloc = llvm::SpecificBumpPtrAllocator<SBNode>;

class SBNode {
private:
  std::vector<SBFootprint *>
      footprints;       // The coarse grained footprint of operands
  unsigned nodeID = -1; // Unique ID of the node
  unsigned BBID = 0;    // ID of basic block
  int ALUID = 0;        // The ID for in-order instructions. The out-of-order
                        // instructions are not counted.
  unsigned liveStartID = 0;    // The start ID of live range
  unsigned liveEndID = 0;      // The end ID of live range
  int integerID = -1;
  int floatID = -1;
  int longID = -1;
  int mathID = -1;
  int DPASID = -1;
  unsigned short DPASSize = 0;
  bool instKilled = false;   // Used for global analysis, if the node generated
                             // dependencies have all been resolved.
  bool sourceKilled = false; // If the dependencies caused by source operands
                             // have all been resolved.
  bool hasAW = false;        // Used for global analysis, if has AW (RAW or WAW)
                             // dependencies from the node
  bool hasAR = false; // Used for global analysis, if has AR (WAR) dependencies
                      // from the node
  bool hasFollowDistOneAReg = false;
  bool followDistOneAReg = false;
  unsigned depDelay = 0;

  struct DepToken {
    unsigned short token;
    SWSBTokenType type;
    SBNode *depNode;
  };
  std::vector<DepToken> depTokens;

public:
  std::vector<G4_INST *> instVec;
  SBDEP_VECTOR succs; // A list of node's successors in dependence graph
  SBDEP_VECTOR preds; // A list of node's predecessors in dependence graph
  int globalID = -1;  // ID of global send instructions
  int sendID = -1;
  int sendUseID = -1;
  SBNode *tokenReusedNode =
      nullptr; // For global token reuse optimization, the node whose token is
               // reused by current one.
  SB_INST_PIPE ALUPipe = PIPE_NONE;
  SBDISTDEP_VECTOR distDep;
  union {
    DISTDEPINFO info;
    unsigned short value;
  } distInfo;

  SBBitSets reachingSends;
  SBBitSets reachedUses;
  unsigned reuseOverhead = 0;

  // FIXME: This needs to be removed, default ctor doesn't make sense at all for
  // this class.
  SBNode()
  {
    clearAllDistInfo();
  }

  SBNode(uint32_t id, int ALUId, uint32_t BBId, G4_INST *i)
      : nodeID(id), ALUID(ALUId), BBID(BBId),
        footprints(Opnd_total_num, nullptr) {
    clearAllDistInfo();
    if (i->isDpas()) {
      G4_InstDpas *dpasInst = i->asDpasInst();
      DPASSize =
          dpasInst
              ->getRepeatCount(); // Set the size of the current dpas inst, if
                                  // there is a macro, following will be added.
    }

    instVec.push_back(i);
  }

  void setSendID(int ID) { sendID = ID; }

  void setSendUseID(int ID) { sendUseID = ID; }

  int getSendID() const { return sendID; }

  int getSendUseID() const { return sendUseID; }

  /* Member functions */
  G4_INST *GetInstruction() const { return instVec.front(); }
  void addInstruction(G4_INST *i) { instVec.push_back(i); }
  void addInstructionAtBegin(G4_INST *i) { instVec.insert(instVec.begin(), i); }
  G4_INST *getLastInstruction() const { return instVec.back(); }
  void setDepDelay(unsigned val) { depDelay = val; }
  unsigned getDepDelay() const { return depDelay; }

  int getALUID() const { return ALUID; }
  unsigned getNodeID() const { return nodeID; };
  unsigned getBBID() const { return BBID; };
  int getIntegerID() const { return integerID; }
  int getFloatID() const { return floatID; }
  int getLongID() const { return longID; }
  int getMathID() const { return mathID; }
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

  void setLiveEarlierID(unsigned id) {
    if (!liveStartID) {
      liveStartID = id;
    } else if (liveStartID > id) {
      liveStartID = id;
    }
  }
  void setLiveLaterID(unsigned id) {
    if (!liveEndID) {
      liveEndID = id;
    } else if (liveEndID < id) {
      liveEndID = id;
    }
  }

  void setLiveEarliestID(unsigned id) { liveStartID = id; }

  void setLiveLatestID(unsigned id) { liveEndID = id; }

  void setInstKilled(bool value) {
    instKilled = value;
    if (value) {
      hasAW = true;
    }
  }

  bool isInstKilled() const { return instKilled; }

  void setSourceKilled(bool value) {
    sourceKilled = value;
    if (value) {
      hasAR = true;
    }
  }

  bool isSourceKilled() const { return sourceKilled; }

  void setAW() { hasAW = true; }

  void setAR() { hasAR = true; }

  void setDistOneAReg() { hasFollowDistOneAReg = true; }
  void setFollowDistOneAReg() { followDistOneAReg = true; }

  bool hasAWDep() const { return hasAW; }
  bool hasARDep() const { return hasAR; }
  bool hasDistOneAreg() const { return hasFollowDistOneAReg; }
  bool followDistOneAreg() const { return followDistOneAReg; }

  void setFootprint(SBFootprint *footprint, Gen4_Operand_Number opndNum) {
    if (footprints[opndNum] == nullptr) {
      footprints[opndNum] = footprint;
    } else {
      footprint->next = footprints[opndNum];
      footprints[opndNum] = footprint;
    }
  }

  SBFootprint *getFirstFootprint(Gen4_Operand_Number opndNum) const {
    return footprints[opndNum];
  }

  unsigned setDistance(unsigned s) {
    unsigned distance = std::min(s, SWSB_MAX_ALU_DEPENDENCE_DISTANCE_VALUE);
    unsigned curDistance = (unsigned)instVec.front()->getDistance();
    if (curDistance == 0 || curDistance > distance) {
      instVec.front()->setDistance((unsigned char)distance);
    }
    return distance;
  }

  void setAccurateDistType(SB_INST_PIPE depPipe) {
    SWSB_global::setAccurateDistType(instVec.front(), depPipe);
  }

  bool hasDistInfo() const { return distInfo.value != 0; }

  void clearAllDistInfo() { distInfo.value = 0; }

  void eraseDepToken(unsigned i) { depTokens.erase(depTokens.begin() + i); }

  size_t getDepTokenNum() const { return depTokens.size(); }
  unsigned short getDepToken(unsigned int i, SWSBTokenType &type) const {
    type = depTokens[i].type;
    return depTokens[i].token;
  }

  unsigned short getDepTokenNodeID(unsigned int i) const {
    return depTokens[i].depNode->getNodeID();
  }

  void setTokenReuseNode(SBNode *node) { tokenReusedNode = node; }

  void *operator new(size_t sz, SBNodeAlloc &Allocator) {
    return Allocator.Allocate(sz / sizeof(SBNode));
  }

  void dumpInterval() const {
    std::cerr << "#" << nodeID << ": " << liveStartID << "-" << liveEndID
              << "\n";
  }
  void dumpAssignedTokens() const {
    std::cerr << "#" << nodeID << ": " << liveStartID << "-" << liveEndID
              << "\n";
    std::cerr << "Token: " << getLastInstruction()->getToken();
    if (tokenReusedNode != nullptr) {
      std::cerr << "\t\tReuse:" << tokenReusedNode->getNodeID();
    }
    std::cerr << "\n";
  }

  unsigned getDistInfo(SB_INST_PIPE depPipe);
  void setDistInfo(SB_INST_PIPE depPipe, unsigned distance);
  void setDepToken(unsigned short token, SWSBTokenType type, SBNode *node);
  void clearDistInfo(SB_INST_PIPE depPipe);
  int calcDiffBetweenInstDistAndPipeDepDist(
      std::vector<unsigned> **latestInstID, unsigned distance,
      SB_INST_PIPE type);
  std::pair<SB_INST_PIPE, int>
  calcClosestALUAndDistDiff(std::vector<unsigned> **latestInstID,
                            unsigned distance);
  std::pair<SB_INST_PIPE, int>
  calcClosestALUAndDistDiff(std::vector<unsigned> **latestInstID);
  bool isClosestALUType(std::vector<unsigned> **latestInstID, unsigned distance,
                        SB_INST_PIPE type);
  void finalizeDistanceType1(IR_Builder &builder,
                             std::vector<unsigned> **latestInstID);
  void finalizeDistanceType2(IR_Builder &builder,
                             std::vector<unsigned> **latestInstID);
  void finalizeDistanceType3(IR_Builder &builder,
                             std::vector<unsigned> **latestInstID);
};
} // namespace vISA
typedef std::vector<vISA::SBNode *> SBNODE_VECT;
typedef std::vector<vISA::SBNode *>::iterator SBNODE_VECT_ITER;
typedef std::list<vISA::SBNode *> SBNODE_LIST;
typedef std::list<vISA::SBNode *>::iterator SBNODE_LIST_ITER;

namespace vISA {
// Similar as SBBucketNode, but it's used for the bucket descriptions got from
// each operands.
struct SBBucketDesc {
  const int bucket;
  const Gen4_Operand_Number opndNum;
  const SBFootprint *footprint;

  SBBucketDesc(int Bucket, Gen4_Operand_Number opnd_num, const SBFootprint *f)
      : bucket(Bucket), opndNum(opnd_num), footprint(f) {
    ;
  }
};

// The node in the node vector of each bucket.
struct SBBucketNode {
  SBNode *node;
  Gen4_Operand_Number opndNum;
  int sendID = -1;
  const SBFootprint *footprint;

  SBBucketNode(SBNode *node1, Gen4_Operand_Number opndNum1,
               const SBFootprint *f)
      : node(node1), opndNum(opndNum1), footprint(f) {}

  void setSendID(int ID) { sendID = ID; }

  int getSendID() const { return sendID; }

  void dump() const {
    std::cerr << "#" << node->getNodeID() << "-" << (int)opndNum << ",";
  }
};

typedef std::vector<SBBucketNode *> SBBUCKET_VECTOR;
typedef SBBUCKET_VECTOR::iterator SBBUCKET_VECTOR_ITER;

// This class hides the internals of dependence tracking using buckets
class LiveGRFBuckets {
  std::vector<SBBUCKET_VECTOR> nodeBucketsArray;
  // FIXME: Why do we need this? Do we ever change the size of nodeBucketsArray?
  const int numOfBuckets;
  bool empty;

public:
  LiveGRFBuckets(int TOTAL_BUCKETS)
      : nodeBucketsArray(TOTAL_BUCKETS), numOfBuckets(TOTAL_BUCKETS) {
    empty = true;
  }

  ~LiveGRFBuckets() {}
  bool isEmpty() {return empty;}
  int getNumOfBuckets() const { return numOfBuckets; }

  // The iterator which is used to scan the node vector of each bucket
  class BN_iterator {
  public:
    const LiveGRFBuckets *LB;
    SBBUCKET_VECTOR_ITER node_it;
    int bucket;

    BN_iterator(const LiveGRFBuckets *LB1, SBBUCKET_VECTOR_ITER It, int Bucket)
        : LB(LB1), node_it(It), bucket(Bucket) {}

    BN_iterator &operator++() {
      ++node_it;
      return *this;
    }

    bool operator!=(const BN_iterator &it2) const {
      vASSERT(LB == it2.LB);
      return (!(bucket == it2.bucket && node_it == it2.node_it));
    }

    SBBucketNode *operator*() {
      vASSERT(node_it != LB->nodeBucketsArray[bucket].end());
      return *node_it;
    }
  };

  BN_iterator begin(int bucket) {
    return BN_iterator(this, nodeBucketsArray[bucket].begin(), bucket);
  }

  BN_iterator end(int bucket) {
    return BN_iterator(this, nodeBucketsArray[bucket].end(), bucket);
  }

  void add(SBBucketNode *bucketNode, int bucket);
  void bucketKill(int bucket, SBNode *node, Gen4_Operand_Number opnd);
  void killSingleOperand(BN_iterator &bn_it);
  void killOperand(BN_iterator &bn_it);
  void dumpLives() const;
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
  int entryBBID = -1;
  int endBBID = -1;
} SWSB_LOOP;

class G4_BB_SB {
private:
  const SWSB &swsb;
  IR_Builder &builder;
  // Used for POD objects that don't need a dtor.
  // TODO: May want to change these to use llvm allocator as well.
  vISA::Mem_Manager &mem;
  G4_BB *bb;
  G4_Label *BBLabel = nullptr;
  int nodeID;
  int ALUID;

  int integerID;
  int floatID;
  int longID;
  int mathID;
  int DPASID;
  unsigned latestDepALUID[PIPE_DPAS];
  std::vector<unsigned> *latestInstID[PIPE_DPAS];

  int totalGRFNum;
  int tokenAfterDPASCycle;

private:
  // dpas read suppression buffer size
  unsigned short getDpasSrcCacheSize(Gen4_Operand_Number opNum) const;

public:
  LiveGRFBuckets *send_use_kills = nullptr;
  BB_SWSB_LIST Preds;
  BB_SWSB_LIST Succs;

  BB_SWSB_LIST domPreds;
  BB_SWSB_LIST domSuccs;

  int first_node = -1;
  int last_node = -1;

  int first_send_node;
  int last_send_node;

  bool tokenAssigned = false;

  int send_start = -1;
  int send_end = -1;

  unsigned loopStartBBID = -1; // The start BB ID of live range
  unsigned loopEndBBID = -1;   // The start BB ID of live range

  SBBitSets send_def_out;
  SBBitSets send_live_in;
  SBBitSets send_live_out;
  SBBitSets send_may_kill;
  SBBitSets send_live_in_scalar;
  SBBitSets send_live_out_scalar;
  SBBitSets send_kill_scalar;

  BitSet dominators;
  SparseBitVector send_WAW_may_kill;

  // For token reduction
  BitSet liveInTokenNodes;
  BitSet liveOutTokenNodes;
  BitSet killedTokens;
  std::vector<BitSet> tokeNodesMap;
  int first_DPASID = 0;
  int last_DPASID = 0;
  unsigned *tokenLiveInDist = nullptr;
  unsigned *tokenLiveOutDist = nullptr;
  SBBitSets localReachingSends;
  SBBitSets
      BBGRF; // Is used to record the GRF registers accessed by each basic block

  SBBUCKET_VECTOR
  globalARSendOpndList; // All send source operands which live out their
                        // instructions' BB. No redundant.

  // BB local data dependence analysis
  G4_BB_SB(const SWSB &sb, IR_Builder &b, Mem_Manager &m, G4_BB *block,
           SBNODE_VECT *SBNodes, SBNODE_VECT *SBSendNodes,
           SBBUCKET_VECTOR *globalSendOpndList, SWSB_INDEXES *indexes,
           uint32_t &globalSendNum, LiveGRFBuckets *lb,
           LiveGRFBuckets *globalLB, LiveGRFBuckets *GRFAlignedGlobalSendsLB,
           PointsToAnalysis &p,
           std::map<G4_Label *, G4_BB_SB *> *LabelToBlockMap,
           const unsigned dpasLatency)
      : swsb(sb), builder(b), mem(m), bb(block),
        tokenAfterDPASCycle(dpasLatency) {
    for (int i = 0; i < PIPE_DPAS; i++) {
      latestDepALUID[i] = 0;
      latestInstID[i] = nullptr;
    }
    first_send_node = -1;
    last_send_node = -1;
    totalGRFNum = block->getKernel().getNumRegTotal();
    SBDDD(bb, lb, globalLB, GRFAlignedGlobalSendsLB, SBNodes, SBSendNodes,
          globalSendOpndList, indexes,
          globalSendNum, p, LabelToBlockMap);
  }

  ~G4_BB_SB() {}

  G4_BB *getBB() const { return bb; }
  G4_Label *getLabel() const { return BBLabel; }

  static bool isGRFEdgeAdded(const SBNode *pred, const SBNode *succ, DepType d,
                             SBDependenceAttr a);
  void createAddGRFEdge(SBNode *pred, SBNode *succ, DepType d,
                        SBDependenceAttr a);

  // Bucket and range analysis
  SBFootprint *getFootprintForGRF(G4_Operand *opnd,
                                  Gen4_Operand_Number opnd_num, G4_INST *inst,
                                  int startingBucket, bool mustBeWholeGRF);
  SBFootprint *getFootprintForACC(G4_Operand *opnd,
                                  Gen4_Operand_Number opnd_num, G4_INST *inst);
  SBFootprint *getFootprintForFlag(G4_Operand *opnd,
                                   Gen4_Operand_Number opnd_num, G4_INST *inst);
  bool getFootprintForOperand(SBNode *node, G4_INST *inst, G4_Operand *opnd,
                              Gen4_Operand_Number opnd_num);
  void getGRFBuckets(const SBFootprint *footprint, Gen4_Operand_Number opndNum,
                     std::vector<SBBucketDesc> &BDvec, bool GRFOnly);
  bool getGRFFootPrintOperands(SBNode *node, G4_INST *inst,
                               Gen4_Operand_Number first_opnd,
                               Gen4_Operand_Number last_opnd,
                               PointsToAnalysis &p);
  void getGRFFootprintForIndirect(SBNode *node, Gen4_Operand_Number opnd_num,
                                  G4_Operand *opnd, PointsToAnalysis &p);
  void getGRFBucketsForOperands(SBNode *node, Gen4_Operand_Number first_opnd,
                                Gen4_Operand_Number last_opnd,
                                std::vector<SBBucketDesc> &BDvec, bool GRFOnly);

  bool hasIndirectSource(SBNode *node);

  bool getGRFFootPrint(SBNode *node, PointsToAnalysis &p);

  void getGRFBucketDescs(SBNode *node, std::vector<SBBucketDesc> &BDvec,
                         bool GRFOnly);

  void clearSLMWARWAissue(SBNode *curNode, LiveGRFBuckets *LB);

  void setDistance(const SBFootprint *footprint, SBNode *node, SBNode *liveNode,
                   bool dstDep);
  void setSpecialDistance(SBNode *node);
  static void footprintMerge(SBNode *node, const SBNode *nextNode);

  void pushItemToQueue(std::vector<unsigned> *nodeIDQueue, unsigned nodeID);

  bool hasInternalDependence(SBNode *nodeFirst, SBNode *nodeNext);

  bool is2xFPBlockCandidate(G4_INST *inst, bool accDST);
  bool hasExtraOverlap(G4_INST *liveInst, G4_INST *curInst,
                       const SBFootprint *liveFootprint,
                       const SBFootprint *curFootprint,
                       const Gen4_Operand_Number curOpnd, IR_Builder *builder);
  bool hasIntraReadSuppression(SBNode *node, Gen4_Operand_Number opndNum,
                               const SBFootprint *curFP) const;
  // Local distance dependence analysis and assignment
  void SBDDD(G4_BB *bb, LiveGRFBuckets *&LB, LiveGRFBuckets *&globalSendsLB,
             LiveGRFBuckets *&GRFAlignedGlobalSendsLB,
             SBNODE_VECT *SBNodes, SBNODE_VECT *SBSendNodes,
             SBBUCKET_VECTOR *globalSendOpndList, SWSB_INDEXES *indexes,
             uint32_t &globalSendNum, PointsToAnalysis &p,
             std::map<G4_Label *, G4_BB_SB *> *LabelToBlockMap);

  G4_INST *createADummyDpasRSWAInst(LiveGRFBuckets *LB, SBNode *curDpasNode,
                                    G4_InstDpas *curInst, SBNode *lastDpasNode,
                                    bool &sameDstSrc);

  // Global SBID dependence analysis
  void setSendOpndMayKilled(LiveGRFBuckets *globalSendsLB, SBNODE_VECT &SBNodes,
                            PointsToAnalysis &p);
  void
  setSendGlobalIDMayKilledByCurrentBB(std::vector<SparseBitVector> &dstTokenBit,
                                  std::vector<SparseBitVector> &srcTokenBit,
                                  SBNODE_VECT &SBNodes, PointsToAnalysis &p);

  void dumpTokenLiveInfo(SBNODE_VECT *SBSendNodes);
  void getLiveBucketsFromFootprint(const SBFootprint *firstFootprint,
                                   SBBucketNode *sBucketNode,
                                   LiveGRFBuckets *send_use_kills) const;
  void clearKilledBucketNodeXeLP(LiveGRFBuckets *LB, int ALUID);

  void clearKilledBucketNodeXeHP(LiveGRFBuckets *LB, int integerID, int floatID,
                                 int longID, int mathID);

  bool hasInternalDependenceWithinDPAS(SBNode *node) const;
  bool hasRAWDependenceBetweenDPASNodes(SBNode *node, SBNode *nextNode) const;
  // check if the given src can be cached (by src suppression buffer)
  bool dpasSrcFootPrintCache(Gen4_Operand_Number opNum, SBNode *curNode,
                             SBNode *nextNode) const;
  bool src2SameFootPrintDiffType(SBNode *curNode, SBNode *nextNode) const;
  bool isLastDpas(SBNode *curNode, SBNode *nextNode);

  void getLiveOutToken(unsigned allSendNum, const SBNODE_VECT &SBNodes);

  unsigned getLoopStartBBID() const { return loopStartBBID; }
  unsigned getLoopEndBBID() const { return loopEndBBID; }

  void setLoopStartBBID(unsigned id) { loopStartBBID = id; }
  void setLoopEndBBID(unsigned id) { loopEndBBID = id; }

  void *operator new(size_t sz,
                     llvm::SpecificBumpPtrAllocator<G4_BB_SB> &Allocator) {
    return Allocator.Allocate(sz / sizeof(G4_BB_SB));
  }

  void dumpLiveInfo(const SBBUCKET_VECTOR *globalSendOpndList,
                    unsigned globalSendNum, const SBBitSets *send_kill) const;
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
  SWSB_TOKEN_PROFILE() {}

  ~SWSB_TOKEN_PROFILE() {}

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

  void setPrunedDiffBBSameTokenEdgeNum(int num) {
    prunedDiffBBSameTokenEdgeNum = num;
  }
  uint32_t getPrunedDiffBBSameTokenEdgeNum() const {
    return prunedDiffBBSameTokenEdgeNum;
  }
};

using FIFOQueueType = std::pair<unsigned short, unsigned short>;

class SWSB {
  G4_Kernel &kernel;
  FlowGraph &fg;
  vISA::Mem_Manager SWSBMem;
  // Type-specific bump pointer allocators so we don't have to explicitly call
  // their dtor.
  llvm::SpecificBumpPtrAllocator<G4_BB_SB> BB_SWSBAllocator;
  SBNodeAlloc SBNodeAllocator;

  BB_SWSB_VECTOR BBVector; // The basic block vector, ordered with ID of the BB
  SBNODE_VECT SBNodes;     // All instruction nodes
  SBNODE_VECT SBSendNodes; // All out-of-order instruction nodes
  SBNODE_VECT SBSendUses;  // All out-of-order instruction nodes
  std::vector<SBBUCKET_VECTOR>
      globalDstSBSendNodes; // globalID indexed global dst token nodes
  std::vector<SBBUCKET_VECTOR>
      globalSrcSBSendNodes; // globalID indexed global src token nodes,
                            // use SBBUCKET_VECTOR vector because one
                            // instruction may have multiple source operands.
#ifdef DEBUG_VERBOSE_ON
  SBNODE_VECT globalSBNodes; // All instruction nodes
#endif
  SWSB_INDEXES indexes;       // To pass ALU ID  from previous BB to current.
  uint32_t globalSendNum = 0; // The number of out-of-order instructions which
                              // generate global dependencies.
  SBBUCKET_VECTOR globalSendOpndList; // All send operands which live out their
                                      // instructions' BBs. No redundant.
  const uint32_t totalTokenNum;
  static constexpr unsigned TOKEN_AFTER_READ_CYCLE = 4;
  const unsigned tokenAfterWriteMathCycle;
  const unsigned tokenAfterWriteSendSlmCycle;
  const unsigned tokenAfterWriteSendMemoryCycle;
  const unsigned tokenAfterWriteSendSamplerCycle;
  int tokenAfterDPASCycle;

  // For profiling
  uint32_t syncInstCount = 0;
  uint32_t AWSyncInstCount = 0;
  uint32_t ARSyncInstCount = 0;
  uint32_t mathReuseCount = 0;
  uint32_t ARSyncAllCount = 0;
  uint32_t AWSyncAllCount = 0;
  uint32_t tokenReuseCount = 0;

  // Linear scan data structures for token allocation
  SBNODE_LIST linearScanLiveNodes;

  std::vector<SBNode *> freeTokenList;

  std::vector<SBNODE_VECT> reachTokenArray;
  std::vector<SBNODE_VECT> reachUseArray;
  SBNODE_VECT localTokenUsage;

  int topIndex = -1;

  std::map<G4_Label *, G4_BB_SB *> labelToBlockMap;

  // TokenAllocation uses a BitSet to track nodes assigned by marking the
  // send IDs of nodes, so that it's possible to get a SBNode using the
  // send ID to index into SBSendNodes.
  // Also add a filed to record the max send ID being assigned.
  struct TokenAllocation {
    BitSet bitset;
    int maxSendID = 0;
    void set(int sendID) {
      bitset.set(sendID, true);
      maxSendID = std::max(sendID, maxSendID);
    }
  };
  std::vector<TokenAllocation> allTokenNodesMap;
  SWSB_TOKEN_PROFILE tokenProfile;

  // Global dependence analysis
  bool globalDependenceDefReachAnalysis(G4_BB *bb);
  bool globalDependenceUseReachAnalysis(G4_BB *bb);
  void addGlobalDependence(unsigned globalSendNum,
                           SBBUCKET_VECTOR *globalSendOpndList,
                           SBNODE_VECT &SBNodes, PointsToAnalysis &p,
                           bool afterWrite);
  void tokenEdgePrune(unsigned &prunedEdgeNum, unsigned &prunedGlobalEdgeNum,
                      unsigned &prunedDiffBBEdgeNum,
                      unsigned &prunedDiffBBSameTokenEdgeNum);
  void dumpTokenLiveInfo();

  void removePredsEdges(SBNode *node, SBNode *pred);

  void dumpImmDom(ImmDominator *dom) const;

  void setDefaultDistanceAtFirstInstruction();

  void quickTokenAllocation();

  // Token allocation
  void tokenAllocation();
  void buildLiveIntervals();
  void expireIntervals(unsigned startID);
  void addToLiveList(SBNode *node);

  void updateTokensForNodeSuccs(SBNode *node, unsigned short token);
  std::pair<int /*reuseDelay*/, int /*curDistance*/>
  examineNodeForTokenReuse(unsigned nodeID, unsigned nodeDelay,
                           const SBNode *curNode, unsigned char nestLoopLevel,
                           unsigned curLoopStartBB,
                           unsigned curLoopEndBB) const;
  SBNode *reuseTokenSelection(const SBNode *node) const;
  unsigned short reuseTokenSelectionGlobal(SBNode *node, G4_BB *bb,
                                           SBNode *&candidateNode,
                                           bool &fromUse);
  void addReachingDefineSet(SBNode *node, SBBitSets *globalLiveSet,
                            SBBitSets *localLiveSet);
  void addReachingUseSet(SBNode *node, SBNode *use);
  void addGlobalDependenceWithReachingDef(unsigned globalSendNum,
                                          SBBUCKET_VECTOR *globalSendOpndList,
                                          SBNODE_VECT &SBNodes,
                                          PointsToAnalysis &p, bool afterWrite);
  void expireLocalIntervals(unsigned startID, unsigned BBID);
  void assignTokenToPred(SBNode *node, SBNode *pred, G4_BB *bb);
  bool assignTokenWithPred(SBNode *node, G4_BB *bb);
  void allocateToken(G4_BB *bb);
  void tokenAllocationBB(G4_BB *bb);
  void buildExclusiveForCoalescing();
  void tokenAllocationGlobalWithPropogation();
  void tokenAllocationGlobal();
  bool propogateDist(G4_BB *bb);
  void tokenAllocationWithDistPropogationPerBB(G4_BB *bb);
  void tokenAllocationWithDistPropogation();
  void calculateDist();

  // Assign Token
  void assignToken(SBNode *node, unsigned short token,
                   uint32_t &AWTokenReuseCount, uint32_t &ARTokenReuseCount,
                   uint32_t &AATokenReuseCount);
  void assignDepToken(SBNode *node);
  void assignDepTokens();
  bool insertSyncTokenPVC(G4_BB *bb, SBNode *node, G4_INST *inst,
                          INST_LIST_ITER inst_it, int newInstID,
                          BitSet *dstTokens, BitSet *srcTokens,
                          bool removeAllToken);
  bool insertDistSyncPVC(G4_BB *bb, SBNode *node, G4_INST *inst,
                         INST_LIST_ITER inst_it);
  bool insertSyncPVC(G4_BB *bb, SBNode *node, G4_INST *inst,
                     INST_LIST_ITER inst_it, int newInstID, BitSet *dstTokens,
                     BitSet *srcTokens);
  bool insertSyncXe(G4_BB *bb, SBNode *node, G4_INST *inst,
                    INST_LIST_ITER inst_it, int newInstID, BitSet *dstTokens,
                    BitSet *srcTokens);
  void insertSync(G4_BB *bb, SBNode *node, G4_INST *inst,
                  INST_LIST_ITER inst_it, int newInstID, BitSet *dstTokens,
                  BitSet *srcTokens);
  void insertTokenSync();

  void getLastTwoGRFsOfSend(FIFOQueueType &GRFs, FIFOQueueType &token,
                            G4_INST *inst);
  void insertDummyImmMov(G4_BB *bb, unsigned token, INST_LIST_ITER inst_iter);
  void insertDummyGRFMov(G4_BB *bb, unsigned short grf, INST_LIST_ITER inst_iter);
  void insertSyncInt1(G4_BB *bb, INST_LIST_ITER inst_iter);
  bool insertDummyMovs(G4_BB *bb, INST_LIST_ITER inst_it, unsigned short token,
                       std::vector<FIFOQueueType> &lastTwoGRFsOfToken,
                       std::vector<FIFOQueueType> &nonLSCLastTwoGRFsOfToken);

  void insertPVCWA();

  // Insert sync instructions
  G4_INST *insertSyncInstruction(G4_BB *bb, INST_LIST_ITER nextIter);
  G4_INST *insertSyncInstructionAfter(G4_BB *bb, INST_LIST_ITER nextIter);
  G4_INST *insertSyncAllRDInstruction(G4_BB *bb, unsigned int SBIDs,
                                      INST_LIST_ITER nextIter);
  G4_INST *insertSyncAllWRInstruction(G4_BB *bb, unsigned int SBIDs,
                                      INST_LIST_ITER nextIter);

  bool insertSyncToken(G4_BB *bb, SBNode *node, G4_INST *inst,
                       INST_LIST_ITER inst_it, int newInstID, BitSet *dstTokens,
                       BitSet *srcTokens, bool &keepDst, bool removeAllToken);

  void
  SWSBInitializeGlobalNodesInBuckets(std::vector<SparseBitVector> &dstGlobalIDs,
                                     std::vector<SparseBitVector> &srcGlobalIDs,
                                     LiveGRFBuckets &globalSendsLB);

  void SWSBDepDistanceGenerator(PointsToAnalysis &p, LiveGRFBuckets &LB,
                                LiveGRFBuckets &globalSendsLB,
                                LiveGRFBuckets &GRFAlignedGlobalSendsLB);
  void handleFuncCall();
  void SWSBGlobalTokenGenerator(PointsToAnalysis &p, LiveGRFBuckets &LB,
                                LiveGRFBuckets &globalSendsLB,
                                LiveGRFBuckets &GRFAlignedGlobalSendsLB);
  void SWSBBuildSIMDCFG();
  void addSIMDEdge(G4_BB_SB *pred, G4_BB_SB *succ);
  void SWSBGlobalScalarCFGReachAnalysis();
  void SWSBGlobalSIMDCFGReachAnalysis();

  void setTopTokenIndex();

  // Optimizations
  void tokenDepReduction(SBNode *node1, SBNode *node2);
  bool cycleExpired(const SBNode *node, int currentID) const;

  void shareToken(const SBNode *node, const SBNode *succ, unsigned short token);

  void SWSBGlobalTokenAnalysis();
  bool globalTokenReachAnalysis(G4_BB *bb);

  // Dump
  void dumpDepInfo() const;
  void dumpLiveIntervals() const;
  void dumpTokeAssignResult() const;
  void dumpSync(const SBNode *tokenNode, const SBNode *syncNode,
                unsigned short token, SWSBTokenType type) const;

  // Fast-composite support.
  void genSWSBPatchInfo();

  void getDominators(ImmDominator *dom);

public:
  SWSB(G4_Kernel &k)
      : kernel(k), fg(k.fg), SWSBMem(4096),
        totalTokenNum(k.fg.builder->kernel.getNumSWSBTokens()),
        tokenAfterWriteMathCycle(k.fg.builder->isXeLP() ? 20u : 17u),
        tokenAfterWriteSendSlmCycle(
            k.fg.builder->isXeLP() ? 33u : 25u), // unlocaled 25
        tokenAfterWriteSendMemoryCycle(
            k.fg.builder->getOptions()->getOption(vISA_USEL3HIT)
                ? (k.fg.builder->isXeLP()
                       ? 106u
                       : 150u) // TOKEN_AFTER_WRITE_SEND_L3_MEMORY_CYCLE
                : (k.fg.builder->isXeLP()
                       ? 65u
                       : 50u)), // TOKEN_AFTER_WRITE_SEND_L1_MEMORY_CYCLE
        tokenAfterWriteSendSamplerCycle(
            k.fg.builder->getOptions()->getOption(vISA_USEL3HIT)
                ? (k.fg.builder->isXeLP()
                       ? 175u
                       : 210u) // TOKEN_AFTER_WRITE_SEND_L3_SAMPLER_CYCLE
                : 60u)         // TOKEN_AFTER_WRITE_SEND_L1_SAMPLER_CYCLE
  {
    indexes.instIndex = 0;
    indexes.ALUIndex = 0;
    indexes.integerIndex = 0;
    indexes.floatIndex = 0;
    indexes.longIndex = 0;
    indexes.DPASIndex = 0;
    indexes.mathIndex = 0;
    tokenAfterDPASCycle =
        LatencyTable::createLatencyTable(*k.fg.builder)->getDPASLatency(8);
  }
  ~SWSB() {}
  void SWSBGenerator();
  unsigned calcDepDelayForNode(const SBNode *node) const;
  // SBNodes need to be live for the entire SWSB pass but they are
  // allocated by each BB, so G4_BB_SB needs access to the allocator.
  SBNodeAlloc &getSBNodeAlloc() { return SBNodeAllocator; }
};
} // namespace vISA
#endif // _SWSB_H_
