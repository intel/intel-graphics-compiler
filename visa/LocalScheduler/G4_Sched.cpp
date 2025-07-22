/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../GraphColor.h"
#include "../PointsToAnalysis.h"
#include "LocalScheduler_G4IR.h"
#include "Passes/AccSubstitution.hpp"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Allocator.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <optional>
#include <queue>

using namespace vISA;

static const unsigned SMALL_BLOCK_SIZE = 10;
static const unsigned LARGE_BLOCK_SIZE = 20000;
static const unsigned LARGE_BLOCK_SIZE_RPE = 32000;
static const unsigned PRESSURE_REDUCTION_MIN_BENEFIT = 5; // percentage
static const unsigned PRESSURE_REDUCTION_THRESHOLD = 110;
static const unsigned PRESSURE_LATENCY_HIDING_THRESHOLD = 104;
static const unsigned PRESSURE_HIGH_THRESHOLD = 128;
static const unsigned PRESSURE_REDUCTION_THRESHOLD_SIMD32 = 120;

namespace {

// Forward declaration.
class preNode;
struct RegisterPressure;
template<G4_RegFileKind Kind> class AregUsage;

using preNodeAlloc = llvm::SpecificBumpPtrAllocator<preNode>;

class preEdge {
public:
  preEdge(preNode *N, DepType Ty) : mNode(N), mType(Ty), mLatency(-1) {}

  preNode *getNode() const { return mNode; }
  DepType getType() const { return mType; }
  bool isDataDep() const {
    switch (mType) {
    case DepType::RAW:
    case DepType::WAR:
    case DepType::WAW:
    case DepType::RAW_MEMORY:
    case DepType::WAR_MEMORY:
    case DepType::WAW_MEMORY:
      return true;
    default:
      break;
    }
    return false;
  }
  bool isFlowDep() const {
    return (mType == DepType::RAW || mType == DepType::WAW);
  }

  void setLatency(int L) { mLatency = L; }
  int getLatency() { return isDataDep() ? mLatency : 0; }

private:
  // Node at the end of this edge.
  preNode *mNode;

  // Type of dependence (RAW, WAW, WAR, etc.).
  DepType mType;

  // data-dependence Latency used in Latency scheduling.
  // only exists (i.e. >=0) on succ-edge during latency scheduling.
  // set in LatencyQueue::calculatePriority
  int mLatency;
};

class preNode {
public:
  // A node for an instruction.
  preNode(G4_INST *Inst, unsigned ID) : Inst(Inst), ID(ID) {
    Barrier = checkBarrier(Inst);
  }

  // A special node without attaching to an instruction.
  preNode() { Barrier = checkBarrier(Inst); }

  ~preNode() = default;

  void *operator new(size_t sz, preNodeAlloc &Allocator) {
    return Allocator.Allocate(sz / sizeof(preNode));
  }

  DepType getBarrier() const { return Barrier; }
  void setBarrier(DepType d) { Barrier = d; }
  static DepType checkBarrier(G4_INST *Inst);
  static bool isBarrier(G4_INST *Inst) {
    switch (checkBarrier(Inst)) {
    case DepType::DEP_LABEL:
    case DepType::CONTROL_FLOW_BARRIER:
    case DepType::INDIRECT_ADDR_BARRIER:
    case DepType::MSG_BARRIER:
    case DepType::OPT_BARRIER:
    case DepType::SEND_BARRIER:
      return true;
    default:
      break;
    }
    return false;
  }

  vISA::G4_INST *getInst() const { return Inst; }
  unsigned getID() const { return ID; }

  typedef std::vector<preEdge>::iterator pred_iterator;
  typedef std::vector<preEdge>::const_iterator const_pred_iterator;
  pred_iterator pred_begin() { return Preds.begin(); }
  pred_iterator pred_end() { return Preds.end(); }
  bool pred_empty() const { return Preds.empty(); }
  unsigned pred_size() const { return (unsigned)Preds.size(); }
  std::vector<preEdge> &preds() { return Preds; }

  typedef std::vector<preEdge>::iterator succ_iterator;
  typedef std::vector<preEdge>::const_iterator const_succ_iterator;
  succ_iterator succ_begin() { return Succs.begin(); }
  succ_iterator succ_end() { return Succs.end(); }
  bool succ_empty() const { return Succs.empty(); }
  unsigned succ_size() const { return (unsigned)Succs.size(); }
  std::vector<preEdge> &succs() { return Succs; }

  void setTupleLead(preNode *Lead) {
    TupleLead = Lead;
    if (this == Lead)
      TupleParts = 1;
    else
      ++Lead->TupleParts;
  }
  preNode *getTupleLead() const { return TupleLead; }
  unsigned getTupleParts() const {
    if (this == TupleLead)
      return TupleParts;
    return TupleLead->TupleParts;
  }
  // Used in latency scheduling
  void setReadyCycle(unsigned cyc) { ReadyCycle = cyc; }
  unsigned getReadyCycle() { return ReadyCycle; }
  // Used in ACC scheduling
  void setACCCandidate() { ACCCandidate = true; }
  bool isACCCandidate() { return ACCCandidate; }

  void print(std::ostream &os) const;
  void dump() const;

private:
  /* The following data shall not be overwritten by a scheduler. */
  std::vector<preEdge> Succs;
  std::vector<preEdge> Preds;

  // The corresponding instruction to this node.
  G4_INST *Inst = nullptr;

  // The unique node ID.
  unsigned ID = 0xFFFFFFFF;

  // Indicates whether this node is a barrier (NONE, SEND, CONTROL)
  DepType Barrier;

  /* The following data may be overwritten by a scheduler. */

  // height by counting edges not by summing latency
  // not used as priority in latency-scheduling
  unsigned Height = 0;

  // Tuple node, which should be schedule in pair with this node.
  preNode *TupleLead = nullptr;
  unsigned TupleParts = 0;

  // # of preds not scheduled.
  unsigned NumPredsLeft = 0;

  // # of succs not scheduled.
  unsigned NumSuccsLeft = 0;

  // the earliest cycle for latency scheduling
  unsigned ReadyCycle = 0;

  // True once scheduled.
  bool isScheduled = false;
  bool ACCCandidate = false;

  friend class preDDD;
  friend class BB_Scheduler;
  friend class BB_ACC_Scheduler;
  friend class SethiUllmanACCQueue;
  friend class SethiUllmanQueue;
  friend class LatencyQueue;
  template <G4_RegFileKind Kind> friend class AregUsage;
};

// The dependency graph for a basic block.
class preDDD {
  G4_Kernel &kernel;

  // The basic block to be scheduled.
  G4_BB *m_BB;

  // If this DDD has been built.
  bool IsDagBuilt = false;

  // All nodes to be built and scheduled.
  preNodeAlloc preNodeAllocator;
  std::vector<preNode *> SNodes;

  // Special node for the schedule region.
  preNode EntryNode;
  preNode ExitNode;

  // New operands to be added into live ones.
  // This auxiliary vector is built while processing one node.
  std::vector<std::pair<preNode *, Gen4_Operand_Number>> NewLiveOps;

  bool BTIIsRestrict;

public:
  preDDD(G4_Kernel &kernel, G4_BB *BB) : kernel(kernel), m_BB(BB) {
    BTIIsRestrict = getOptions()->getOption(vISA_ReorderDPSendToDifferentBti);
  }
  ~preDDD() = default;

  G4_Kernel &getKernel() const { return kernel; }
  G4_BB *getBB() const { return m_BB; }
  Options *getOptions() const { return kernel.getOptions(); }
  preNode *getEntryNode() { return &EntryNode; }
  preNode *getExitNode() { return &ExitNode; }
  std::vector<preNode *> &getNodes() { return SNodes; }

  // Build the data dependency graph.
  void buildGraph();

  // Initialize or clear per node state so that data dependency graph
  // could be used for scheduling.
  void reset(bool ReassignNodeID = false);

  // Dump the DDD into a text file
  // need RegisterPressure to get LiveOut info
  void dumpDagTxt(RegisterPressure &rp);
  // Dump the DDD into a dot file.
  void dumpDagDot();

  void buildGraphForACC();

  // Each instruction creates live nodes for adding dependency edges.
  struct LiveNode {
    LiveNode(preNode *N, Gen4_Operand_Number OpNum) : N(N), OpNum(OpNum) {}

    // The DAG node that this node belongs to.
    preNode *N;

    // This indicates which operand this node is tracking.
    Gen4_Operand_Number OpNum;

    // Check if this is a read/write operand.
    bool isWrite() const {
      return OpNum == Gen4_Operand_Number::Opnd_dst ||
             OpNum == Gen4_Operand_Number::Opnd_condMod ||
             OpNum == Gen4_Operand_Number::Opnd_implAccDst;
    }
    bool isRead() const { return !isWrite(); }

    friend void swap(LiveNode &a, LiveNode &b) {
      std::swap(a.N, b.N);
      std::swap(a.OpNum, b.OpNum);
    }
  };

private:
  // Keep live nodes while scanning the block.
  // Each declare is associated with a list of live nodes.
  std::unordered_map<const G4_Declare *, std::vector<LiveNode>> LiveNodes;

  // Use an extra list to track physically assigned nodes, I.e. a0.2 etc.
  std::vector<LiveNode> LivePhysicalNodes;

  // Use an extra list to track send message dependency.
  std::vector<preNode *> LiveSends;

  // The most recent scheduling barrier.
  preNode *prevBarrier = nullptr;

  // The core function for building the DAG.
  // This adds node to the DAG and adds any required edges
  // by checking the dependencies against the live nodes.
  void addNodeToGraph(preNode *N);

  // Create a new edge from pred->succ of type D.
  void addEdge(preNode *pred, preNode *succ, DepType D) {
    auto fn = [=](const preEdge &E) { return E.getNode() == succ; };
    if (pred->succ_end() ==
        std::find_if(pred->succ_begin(), pred->succ_end(), fn)) {
      pred->Succs.emplace_back(succ, D);
      succ->Preds.emplace_back(pred, D);
    }
  }

  void processBarrier(preNode *curNode, DepType Dep);
  void processSend(preNode *curNode);
  void addSrcOpndDep(preNode *curNode, G4_Declare *dcl,
                     Gen4_Operand_Number OpNum);
  void processReadWrite(preNode *curNode);
  void prune();
};

// Track and recompute register pressure for a block.
struct RegisterPressure {
  PointsToAnalysis *p2a = nullptr;
  GlobalRA *gra = nullptr;
  LivenessAnalysis *liveness = nullptr;
  RPE *rpe = nullptr;
  G4_Kernel &kernel;

  RegisterPressure(G4_Kernel &kernel, RPE *rpe) : rpe(rpe), kernel(kernel) {
    // Initialize rpe if not available.
    if (rpe == nullptr) {
      init();
    } else {
      liveness = const_cast<LivenessAnalysis *>(rpe->getLiveness());
      rpe->run();
    }
  }

  ~RegisterPressure() {
    // Delete only if owns the following objects.
    if (p2a) {
      delete p2a;
      delete gra;
      delete liveness;
      delete rpe;
    }
  }

  RegisterPressure(const RegisterPressure &other) = delete;
  RegisterPressure &operator=(RegisterPressure &other) = delete;

  void init() {
    p2a = new PointsToAnalysis(kernel.Declares, kernel.fg.getNumBB());
    p2a->doPointsToAnalysis(kernel.fg);
    gra = new GlobalRA(kernel, kernel.fg.builder->phyregpool, *p2a);
    // To properly track liveness for partially-written local variables.
    gra->markGraphBlockLocalVars();
    liveness = new LivenessAnalysis(*gra, G4_GRF | G4_ADDRESS | G4_INPUT |
                                              G4_FLAG | G4_SCALAR);
    liveness->computeLiveness();
    rpe = new RPE(*gra, liveness);
    rpe->run();
  }

  bool isLiveOut(G4_BB *bb, G4_Declare *Dcl) const {
    G4_RegVar *V = Dcl->getRegVar();
    return liveness->isLiveAtExit(bb, V->getId());
  }

  void recompute(G4_BB *BB) { rpe->runBB(BB); }
  void recompute() {
    rpe->resetMaxRP();
    rpe->run();
  }

  // Return the register pressure in GRF for an instruction.
  unsigned getPressure(G4_INST *Inst) const {
    return rpe->getRegisterPressure(Inst);
  }

  // Return the max register pressure
  unsigned getMaxRP() const {
    return rpe->getMaxRP();
  }

  // Return the max pressure in GRFs for this block.
  unsigned getPressure(G4_BB *bb, std::vector<G4_INST *> *Insts = nullptr) {
    unsigned Max = 0;
    for (auto Inst : *bb) {
      if (Inst->isPseudoKill())
        continue;
      unsigned Pressure = rpe->getRegisterPressure(Inst);
      if (Pressure > Max) {
        Max = Pressure;
        if (Insts) {
          Insts->clear();
          Insts->push_back(Inst);
        }
      } else if (Pressure == Max && Insts) {
        Insts->push_back(Inst);
      }
    }

    return Max;
  }

  void dump(G4_BB *bb, const char *prefix = "") {
    unsigned Max = 0;
    std::vector<G4_INST *> Insts;
    for (auto Inst : *bb) {
      if (Inst->isPseudoKill()) {
        std::cerr << "[---] ";
        Inst->dump();
        continue;
      }
      unsigned Pressure = rpe->getRegisterPressure(Inst);
      if (Pressure > Max) {
        Max = Pressure;
        Insts.clear();
        Insts.push_back(Inst);
      } else if (Pressure == Max) {
        Insts.push_back(Inst);
      }
      std::cerr << "[" << Pressure << "] ";
      Inst->dump();
    }
    std::cerr << prefix << "the max pressure is " << Max << "\n";
    std::cerr << "Max pressure instructions are: \n";
    for (auto Inst : Insts) {
      Inst->dump();
    }
    std::cerr << "\n\n";
  }
};

struct SchedConfig {
  enum {
    MASK_DUMP = 1U << 0,
    MASK_LATENCY = 1U << 1,
    MASK_MIN_REG = 1U << 2,
    MASK_SKIP_CLUSTER = 1U << 3,
    MASK_SKIP_HOLD = 1U << 4,
    MASK_NOT_ITERATE = 1U << 5,
  };
  unsigned Dump : 1;
  unsigned UseLatency : 1;
  unsigned UseMinReg  : 1;
  unsigned SkipClustering : 1; // default 0 i.e. try min-reg with clustering
  unsigned SkipHoldList : 1; // default 0 i.e. use hold list in latency-hiding
  unsigned DoNotIterate : 1; // default 0 i.e. iterative latency-scheduling

  explicit SchedConfig(unsigned Config)
      : Dump((Config & MASK_DUMP) != 0),
        UseLatency((Config & MASK_LATENCY) != 0),
        UseMinReg((Config & MASK_MIN_REG) != 0),
        SkipClustering((Config & MASK_SKIP_CLUSTER) != 0),
        SkipHoldList((Config & MASK_SKIP_HOLD) != 0),
        DoNotIterate((Config & MASK_NOT_ITERATE) != 0) {}
};

#define SCHED_DUMP(X)                                                          \
  do {                                                                         \
    if (config.Dump) {                                                         \
      X;                                                                       \
    }                                                                          \
  } while (0)

// Scheduler on a single block.
class BB_Scheduler {
  // The kernel this block belongs to.
  G4_Kernel &kernel;

  // The data dependency graph for this block.
  preDDD &ddd;

  // Register pressure estimation and tracking.
  RegisterPressure &rp;

  // The most recent schedule result.
  std::vector<G4_INST *> schedule;
  unsigned CycleEstimation = 0;
  // save the original list before any scheduling
  INST_LIST OrigInstList;

  // Options to customize scheduler.
  SchedConfig config;

  const LatencyTable &LT;

public:
  BB_Scheduler(G4_Kernel &kernel, preDDD &ddd, RegisterPressure &rp,
               SchedConfig config, const LatencyTable &LT)
      : kernel(kernel), ddd(ddd), rp(rp), config(config), LT(LT) {}
  ~BB_Scheduler() {
    schedule.clear();
    OrigInstList.clear();
  }
  BB_Scheduler(const BB_Scheduler&) = delete;
  BB_Scheduler& operator=(const BB_Scheduler&) = delete;

  G4_Kernel &getKernel() const { return kernel; }
  G4_BB *getBB() const { return ddd.getBB(); }

  // MaxPressure is the BB pressure before and after scheduling
  bool scheduleBlockForPressure(unsigned &MaxPressure, unsigned Threshold);

  // MaxPressure is the BB reg-pressure before and after scheduling
  // ReassignID of PreNodes when this is not 1st-round scheduling
  // UpperBoundGRF is the measure max reg-pressure of this kernel before scheduling
  bool scheduleBlockForLatency(unsigned &MaxPressure, bool ReassignID,
                               unsigned UpperBoundGRF);
  void SethiUllmanScheduling(bool DoClustering);

private:
  void LatencyScheduling(unsigned GroupingThreshold);
  bool verifyScheduling();

  // Relocate pseudo-kills right before its successors.
  void relocatePseudoKills();
  // Commit this scheduling if it reduces register pressure.
  bool commitIfBeneficial(unsigned &MaxRPE, bool IsTopDown = false,
                          unsigned NumGrfs = 128);
  // save the original inst list
  void saveOriginalList() {
    INST_LIST &CurInsts = getBB()->getInstList();
    OrigInstList.clear();
    OrigInstList.splice(OrigInstList.begin(), CurInsts, CurInsts.begin(),
                        CurInsts.end());
    vASSERT(CurInsts.empty());
  }
  // restore the original inst list
  void restoreOriginalList() {
    INST_LIST &CurInsts = getBB()->getInstList();
    vASSERT(CurInsts.size() == OrigInstList.size());
    CurInsts.clear();
    CurInsts.splice(CurInsts.begin(), OrigInstList, OrigInstList.begin(),
                    OrigInstList.end());
    rp.recompute(getBB());
  }
};

} // namespace

static bool isSlicedSIMD32(G4_Kernel &kernel) {
  // need special treatment with simd32-slicing during scheduling
  return (kernel.getSimdSize() == g4::SIMD32 &&
          kernel.fg.builder->getNativeExecSize() < g4::SIMD16);
}

static unsigned getRPReductionThreshold(G4_Kernel &kernel) {
  unsigned RPThreshold =
      kernel.getOptions()->getuInt32Option(vISA_preRA_MinRegThreshold);
  if (RPThreshold == 0) {
    // For SIMD32 prior to PVC, use a higher threshold for rp reduction,
    // as it may not be beneficial.
    RPThreshold = isSlicedSIMD32(kernel) ? kernel.getScaledGRFSize(PRESSURE_REDUCTION_THRESHOLD_SIMD32)
                                         : kernel.getScaledGRFSize(PRESSURE_REDUCTION_THRESHOLD);
  }
  return RPThreshold;
}

static unsigned getLatencyHidingThreshold(G4_Kernel &kernel, unsigned NumGrfs) {
  unsigned RPThreshold =
      kernel.getOptions()->getuInt32Option(vISA_preRA_ScheduleRPThreshold);
  if (RPThreshold == 0) {
    RPThreshold = PRESSURE_LATENCY_HIDING_THRESHOLD;
  }
  return unsigned(RPThreshold * (std::max(NumGrfs, 128u) - 32u) / 96u);
}

preRA_Scheduler::preRA_Scheduler(G4_Kernel &k) : kernel(k) {}

preRA_Scheduler::~preRA_Scheduler() {}

bool preRA_Scheduler::run(unsigned &KernelPressure) {
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D) {
    // Do not run pre-RA scheduler for CM unless user forces it.
    if (!kernel.getOption(vISA_preRA_ScheduleForce))
      return false;
  }

  unsigned Threshold = getRPReductionThreshold(kernel);
  unsigned SchedCtrl = kernel.getuInt32Option(vISA_preRA_ScheduleCtrl);

  auto LT = LatencyTable::createLatencyTable(*kernel.fg.builder);
  SchedConfig config(SchedCtrl);
  RegisterPressure rp(kernel, nullptr);
  // skip extreme test cases that scheduling does not good
  // if (kernel.fg.getNumBB() >= 10000 && rp.rpe->getMaxRP() >= 800)
  //   return false;

  bool Changed = false;
  for (auto bb : kernel.fg) {
    if (bb->size() < SMALL_BLOCK_SIZE || bb->size() > LARGE_BLOCK_SIZE) {
      SCHED_DUMP(std::cerr << "Skip block with instructions " << bb->size()
                           << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleStartBBID) &&
        (bb->getId() <
         kernel.getuInt32Option(vISA_ScheduleStartBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleEndBBID) &&
        (bb->getId() >
         kernel.getuInt32Option(vISA_ScheduleEndBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    unsigned MaxPressure = rp.getPressure(bb);
    if (MaxPressure <= Threshold && !config.UseLatency) {
      SCHED_DUMP(std::cerr << "Skip block with rp " << MaxPressure << "\n");
      continue;
    }

    SCHED_DUMP(rp.dump(bb, "Before scheduling, "));
    preDDD ddd(kernel, bb);
    BB_Scheduler S(kernel, ddd, rp, config, *LT);

    Changed |= S.scheduleBlockForPressure(MaxPressure, Threshold);
    Changed |= S.scheduleBlockForLatency(MaxPressure, Changed, 0);
  }

  if (Changed)
    rp.recompute();
  KernelPressure = rp.getMaxRP();

  return Changed;
}

bool preRA_Scheduler::runWithGRFSelection(unsigned &KernelPressure) {
  // General algorithm for GRF selection in VRT:
  //  1. Schedule for reg pressure reduction if needed
  //      - Reg pressure might be reduced in this step
  //  2. Re-compute reg pressure if there were any changes in step 1
  //  3. Adjust kernel's GRF based on reg pressure
  //  4. Schedule for latency hiding if needed
  //      - If GRF was lowered in step 3, we try a larger GRF mode
  //        as upper bound for latency hiding scheduling (+32).
  //      - Different schedules are computed considering GRF modes
  //        up to the upper bound. The best schedule is committed.
  //      - Reg pressue might be increased in this step (bounded by
  //        current upper bound).
  //  5. Adjust kernel's GRF based on reg pressure
  //      - In order to avoid spills, extra registers are added to
  //        the reg pressure when GRF selected has the same number
  //        of threads as the next larger one.

  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D) {
    // Do not run pre-RA scheduler for CM unless user forces it.
    if (!kernel.getOption(vISA_preRA_ScheduleForce))
      return false;
  }

  bool Changed = false;

  unsigned SchedCtrl = kernel.getuInt32Option(vISA_preRA_ScheduleCtrl);
  SchedConfig config(SchedCtrl);
  RegisterPressure rp(kernel, nullptr);
  KernelPressure = rp.getMaxRP();
  unsigned RPReductionThreshold = getRPReductionThreshold(kernel);
  auto LT = LatencyTable::createLatencyTable(*kernel.fg.builder);

  // Schedule for reg pressure reduction if needed
  for (auto bb : kernel.fg) {
    // Skip BBs:
    if (bb->size() < SMALL_BLOCK_SIZE || bb->size() > LARGE_BLOCK_SIZE) {
      SCHED_DUMP(std::cerr << "Skip block with instructions " << bb->size()
                           << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleStartBBID) &&
        (bb->getId() < kernel.getuInt32Option(vISA_ScheduleStartBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleEndBBID) &&
        (bb->getId() > kernel.getuInt32Option(vISA_ScheduleEndBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    // Schedule:
    SCHED_DUMP(rp.dump(bb, "Before scheduling for pressure reduction, "));
    preDDD ddd(kernel, bb);
    BB_Scheduler S(kernel, ddd, rp, config, *LT);
    unsigned BBRP = rp.getPressure(bb);
    Changed |= S.scheduleBlockForPressure(BBRP, RPReductionThreshold);
  }

  if (Changed) {
    // Re-compute register pressure estimation
    rp.recompute();
    KernelPressure = rp.getMaxRP();
  }

  // Adjust GRF based on register pressure
  unsigned oldGRFNum = kernel.getNumRegTotal();
  kernel.updateKernelByRegPressure(KernelPressure);
  bool GRFdecreased = kernel.getNumRegTotal() < oldGRFNum;
  Changed = false;

  // Schedule for latency hiding if needed
  for (auto bb : kernel.fg) {
    // Skip BBs:
    if (bb->size() < SMALL_BLOCK_SIZE || bb->size() > LARGE_BLOCK_SIZE) {
      SCHED_DUMP(std::cerr << "Skip block with instructions " << bb->size()
                           << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleStartBBID) &&
        (bb->getId() < kernel.getuInt32Option(vISA_ScheduleStartBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    if (kernel.getuInt32Option(vISA_ScheduleEndBBID) &&
        (bb->getId() > kernel.getuInt32Option(vISA_ScheduleEndBBID))) {
      SCHED_DUMP(std::cerr << "Skip BB" << bb->getId() << "\n");
      continue;
    }

    // Schedule:
    SCHED_DUMP(rp.dump(bb, "Before scheduling for latency hiding, "));
    preDDD ddd(kernel, bb);
    BB_Scheduler S(kernel, ddd, rp, config, *LT);
    unsigned BBRP = rp.getPressure(bb);

    unsigned UpperBoundGRF = 0;
    if (GRFdecreased && KernelPressure < kernel.grfMode.getMaxGRF())
      UpperBoundGRF = kernel.grfMode.getLargerGRF();
    Changed |= S.scheduleBlockForLatency(BBRP, Changed, UpperBoundGRF);
  }

  if (Changed) {
    rp.recompute();
    KernelPressure = rp.getMaxRP();
  }

  kernel.updateKernelByRegPressure(KernelPressure, true);

  return Changed;
}

[[maybe_unused]] bool BB_Scheduler::verifyScheduling() {
  std::set<G4_INST *> Insts;
  for (auto Inst : *(getBB()))
    Insts.insert(Inst);

  for (auto Inst : schedule) {
    if (Insts.count(Inst) != 1) {
      Inst->dump();
      return false;
    }
  }

  return true;
}

namespace {

// The base class that implements common functionalities used in
// different scheduling algorithms.
class QueueBase {
protected:
  // The data-dependency graph.
  preDDD &ddd;

  // Register pressure related data.
  RegisterPressure &rp;

  // Options to customize scheduler.
  SchedConfig config;

  QueueBase(preDDD &ddd, RegisterPressure &rp, SchedConfig config)
      : ddd(ddd), rp(rp), config(config) {}

  virtual ~QueueBase() {}

public:
  preNode *getCurrTupleLead() const { return TheCurrTupleLead; }
  void setCurrTupleLead(preNode *N) {
    vASSERT(N->getInst()->getExecSize() == g4::SIMD8 ||
            N->getInst()->getExecSize() == g4::SIMD16);
    TheCurrTupleLead = N->getTupleLead();
    TheCurrTupleParts = N->getTupleParts();
  }
  void updateCurrTupleLead(preNode *N) {
    vASSERT(TheCurrTupleLead != nullptr);
    vASSERT(N->getTupleLead() == TheCurrTupleLead);
    TheCurrTupleParts--;
    if (TheCurrTupleParts == 0)
      TheCurrTupleLead = nullptr;
  }
  virtual void push(preNode *N) = 0;
  virtual preNode *pickNode() = 0;

protected:
  // The current (send) tuple lead.
  preNode *TheCurrTupleLead = nullptr;
  unsigned TheCurrTupleParts = 0;
};

// Queue for Sethi-Ullman scheduling to reduce register pressure.
class SethiUllmanQueue : public QueueBase {
  // Ready nodes.
  std::vector<preNode *> Q;

  // Sethi-Ullman numbers.
  // max-reg-pressure for the sub-exp-tree starting from a node
  std::vector<int> MaxRegs;
  std::vector<int> DstSizes;
  // the max time-stamp among node uses
  std::vector<unsigned> LiveTS;

public:
  SethiUllmanQueue(preDDD &ddd, RegisterPressure &rp, SchedConfig config)
      : QueueBase(ddd, rp, config) {
    init();
  }

  // Add a new ready node.
  void push(preNode *N) override {
    Q.push_back(N);
  }

  // Schedule the top node.
  preNode *pickNode() override { return select(); }

  bool empty() const { return Q.empty(); }

  friend void BB_Scheduler::SethiUllmanScheduling(bool);

private:
  // Initialize Sethi-Ullman numbers.
  void init();
  // Check if the Tuple restriction is obeyed.
  bool willNotBreakTupleRestriction(preNode *N) const;
  // Select next ready node to schedule.
  preNode *select();
  // In clustering mode
  void formWorkingSet(preNode *seed, std::vector<preNode *> &W);

  // Compare two ready nodes and decide which one should be scheduled first.
  // Return true if N2 has a higher priority than N1, false otherwise.
  bool compare(preNode *N1, preNode *N2);

  // Compute the Sethi-Ullman number for a node.
  void calculateSethiUllmanNumber(preNode *N);
};

} // namespace

// This implements the idea in the paper by Appel & Supowit:
//
// Generalizations of the Sethi-Ullman algorithm for register allocation
//
void SethiUllmanQueue::calculateSethiUllmanNumber(preNode *N) {
  auto getDstByteSize = [&](preNode *Node) -> int {
    G4_INST *Inst = Node->getInst();
    if (!Inst)
      return 0;
    G4_DstRegRegion *Dst = Inst->getDst();
    if (Dst && Dst->getTopDcl()) {
      // If a variable lives out, then there is no extra cost to hold the
      // result.
      if (rp.isLiveOut(ddd.getBB(), Dst->getTopDcl()))
        return 0;
      auto rootDcl = Dst->getTopDcl();
      auto dclSize = rootDcl->getByteSize();
      auto alignBytes = static_cast<uint32_t>(rootDcl->getSubRegAlign()) * 2;
      if (dclSize < alignBytes) {
        dclSize = std::min(dclSize * 2, alignBytes);
      }
      return dclSize;
    }
    return 0;
  };

  vASSERT(N->getID() < MaxRegs.size());
  vASSERT(N->getID() < DstSizes.size());
  auto CurNum = MaxRegs[N->getID()];
  if (CurNum != 0 || DstSizes[N->getID()] != 0)
    return;
  // record the destination register requirement
  DstSizes[N->getID()] = getDstByteSize(N);
  // compute max-reg
  std::vector<std::pair<preNode *, int>> Preds;
  for (auto I = N->pred_begin(), E = N->pred_end(); I != E; ++I) {
    auto &Edge = *I;
    auto DepType = Edge.getType();
    if (DepType != RAW && DepType != WAW)
      continue;

    // Skip pseudo-kills as they are lifetime markers.
    auto DefInst = Edge.getNode()->getInst();
    if (DefInst && DefInst->isPseudoKill())
      continue;

    // Recurse on the predecessors.
    calculateSethiUllmanNumber(Edge.getNode());
    auto MaxReg = MaxRegs[Edge.getNode()->getID()];
    auto DstSize = DstSizes[Edge.getNode()->getID()];
    Preds.emplace_back(Edge.getNode(), MaxReg - DstSize);
  }

  vASSERT(CurNum == 0);
  if (Preds.size() > 0) {
    std::sort(Preds.begin(), Preds.end(),
              [](std::pair<preNode *, int> lhs, std::pair<preNode *, int> rhs) {
                return lhs.second < rhs.second;
              });
    for (unsigned i = 0, e = (unsigned)Preds.size(); i < e; ++i) {
      auto PN = Preds[i].first;
      auto DstSize = DstSizes[PN->getID()];
      auto MaxReg = MaxRegs[PN->getID()];
      CurNum = std::max(MaxReg, CurNum + DstSize);
    }
  }
  MaxRegs[N->getID()] = CurNum;
  return;
}

void SethiUllmanQueue::init() {
  auto &Nodes = ddd.getNodes();
  unsigned N = (unsigned)Nodes.size();
  MaxRegs.resize(N, 0);
  DstSizes.resize(N, 0);
  LiveTS.resize(N, N);
  for (auto I = Nodes.rbegin(); I != Nodes.rend(); ++I) {
    calculateSethiUllmanNumber((*I));
  }

#if 0
    std::cerr << "\n\n";
    for (auto I = Nodes.rbegin(); I != Nodes.rend(); ++I) {
        std::cerr << "MaxRegs[" << MaxRegs[(*I)->getID()] << "] ";
        (*I)->getInst()->dump();
    }
    std::cerr << "\n\n";
#endif
}

// Compare two ready nodes and decide which one should be scheduled first.
// Return true if N2 has a higher priority than N1, false otherwise.
bool SethiUllmanQueue::compare(preNode *N1, preNode *N2) {
  // TODO. Introduce heuristics before comparing SU numbers.
  vASSERT(N1->getID() < MaxRegs.size());
  vASSERT(N2->getID() < MaxRegs.size());
  vASSERT(N1->getID() != N2->getID());

  // Pseudo kill always has higher priority.
  if (N1->getInst()->isPseudoKill())
    return false;

  auto SU1 = MaxRegs[N1->getID()] - DstSizes[N1->getID()];
  auto SU2 = MaxRegs[N2->getID()] - DstSizes[N2->getID()];

  // This is a bottom-up scheduling. Smaller SU number means higher priority.
  if (SU1 < SU2)
    return false;

  if (SU1 > SU2)
    return true;

  // Otherwise, break tie with their IDs. Smaller ID means higher priority.
  return N1->getID() > N2->getID();
}

bool SethiUllmanQueue::willNotBreakTupleRestriction(preNode *N) const {
  return (!N->getInst() || !N->getInst()->isSend() || !TheCurrTupleLead ||
          (N->getTupleLead() == TheCurrTupleLead));
};

preNode *SethiUllmanQueue::select() {
  vASSERT(!Q.empty());
  auto TopIter = Q.end();
  for (auto I = Q.begin(), E = Q.end(); I != E; ++I) {
    preNode *N = *I;
    // If there's a node to be paired, skip send not in pair.
    if (!willNotBreakTupleRestriction(N))
      continue;
    if (TopIter == Q.end() || compare(*TopIter, *I))
      TopIter = I;
  }

  // In rare cases, there is a cycle due to send pairing.
  // Stop this heuristics if it happens.
  if (TopIter == Q.end()) {
    TheCurrTupleLead = nullptr;
    TheCurrTupleParts = 0;
    for (auto I = Q.begin(), E = Q.end(); I != E; ++I) {
      if (TopIter == Q.end() || compare(*TopIter, *I))
        TopIter = I;
    }
  }

  vASSERT(TopIter != Q.end());
  preNode *Top = *TopIter;
  std::swap(*TopIter, Q.back());
  Q.pop_back();

  return Top;
}

void SethiUllmanQueue::formWorkingSet(preNode *seed,
                                      std::vector<preNode *> &W) {
  if (!seed->getInst() || seed->getInst()->isSend()) {
    W.push_back(seed);
    return;
  }

  std::vector<preNode *> Cluster;
  Cluster.push_back(seed);
  preNode *ClusterWait = nullptr;
  unsigned MaxHeight = seed->Height;
  // looking for cluster by checking source-operand sharing
  for (auto I = seed->pred_begin(), E = seed->pred_end(); I != E; ++I) {
    preNode *Pred = I->getNode();
    // not looking for cluster for instructions sharing tiny-size def
    if (Pred->getInst() == nullptr || DstSizes[Pred->getID()] < 4)
      continue;
    if (I->isFlowDep()) {
      unsigned UseCnt = 0;
      for (auto J = Pred->succ_begin(), JE = Pred->succ_end(); J != JE; ++J) {
        if (J->isFlowDep()) {
          UseCnt++;
        }
      }
      //  not looking for cluster on operand with too many sharing
      if (UseCnt > 8)
        continue;
      // adding the other uses of Pred to the cluster
      for (auto J = Pred->succ_begin(), JE = Pred->succ_end(); J != JE; ++J) {
        preNode *Succ = J->getNode();
        if (!J->isFlowDep())
          continue;
        if (Succ->isScheduled)
          continue;
        // this use is not in cluster
        if (std::find(Cluster.begin(), Cluster.end(), Succ) == Cluster.end()) {
          Cluster.push_back(Succ);
          // Succ is not in ready list
          if (Succ->NumSuccsLeft > 0) {
            if (ClusterWait == nullptr)
              ClusterWait = Succ;
            else if (Succ->Height > ClusterWait->Height)
              ClusterWait = Succ;
            MaxHeight = std::max(MaxHeight, ClusterWait->Height);
          }
        }
      }
    }
  }
  // when the entire cluster is ready or the entire cluster is too small
  // too large, just schedule all ready nodes in the cluster
  if (!ClusterWait || Cluster.size() <= 2 || Cluster.size() > 5) {
    W.push_back(seed);
    unsigned idx = 0;
    while (idx < Q.size()) {
      preNode *Tmp = Q[idx];
      if (std::find(Cluster.begin(), Cluster.end(), Tmp) != Cluster.end()) {
        // For send instruction which is not part of TheCurrTupleLead, the send
        // instruction cannot be inserted into W. Which will be scheduled before
        // the other parts of TheCurrTupleLead.
        // Such as in following 4 send instructions, first two and last two send
        // instructions must be schedule together, cannot interleave between
        // them.
        //
        // sample_l.RGB (M1, 16)  0x0:uw S31 %bss V0626.0 %null.0 V0628.0
        // CCTuple_1.0 CCTuple_1.64
        // sample_l.RGB(M5, 16) 0x0 : uw S31 %bss V0627.0 %null.0 V0629.0
        // CCTuple_2.0 CCTuple_2.64
        //
        // sample_l.RGB(M1, 16) 0x0 : uw S31 %bss V0640.0 %null.0 V0642.0
        // CCTuple_3.0 CCTuple_3.64
        // sample_l.RGB(M5, 16) 0x0 : uw S31 %bss V0641.0 %null.0 V0643.0
        // CCTuple_4.0 CCTuple_4.64
        if (willNotBreakTupleRestriction(Tmp)) {
          W.push_back(Tmp);
          Q[idx] = Q.back();
          Q.pop_back();
        } else {
          idx++;
        }
      } else
        idx++;
    }
    return;
  }
  // cluster is not ready, find a ready node that can enable the cluster
  auto Top = ClusterWait;
  bool Searching = (MaxHeight - seed->Height <= 20);
  while (Searching) {
    Searching = false;
    for (auto I = Top->succ_begin(), E = Top->succ_end(); I != E; ++I) {
      auto Node = I->getNode();
      if (!Node->isScheduled) {
        if (Node->NumSuccsLeft > 0) {
          Top = Node; // continue search
          Searching = true;
          break;
        } else if (willNotBreakTupleRestriction(Node)) {
          if (Node != seed) {
            Q.erase(std::remove(Q.begin(), Q.end(), Node), Q.end());
            Q.push_back(seed);
          }
          W.push_back(Node);
          return;
        }
      }
    }
  }
  if (W.empty()) {
    W.push_back(seed);
    return;
  }
}

// The basic idea is...
//
bool BB_Scheduler::scheduleBlockForPressure(unsigned &MaxPressure,
                                            unsigned Threshold) {
  auto tryRPReduction = [this, MaxPressure, Threshold]() {
    if (!config.UseMinReg)
      return false;
    if (MaxPressure < Threshold)
      return false;
    // MaxRP at the block entry or exit, cannot change that
    if (MaxPressure == rp.getPressure(ddd.getBB()->front()) ||
        MaxPressure == rp.getPressure(ddd.getBB()->back()))
      return false;
    return true;
  };

  bool Changed = false;
  if (tryRPReduction()) {
    ddd.buildGraph();
    if (kernel.getOptions()->getOption(vISA_DumpDagTxt)) {
      ddd.dumpDagTxt(rp);
    }
    if (!config.SkipClustering) {
      // try clustering first
      SethiUllmanScheduling(true);
      if (commitIfBeneficial(MaxPressure)) {
        SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for presssure, "));
        kernel.fg.builder->getJitInfo()->statsVerbose.minRegClusterCount++;
        Changed = true;
      } else {
        ddd.reset(false);
      }
    }
    if (!Changed) {
      // try not-clustering
      SethiUllmanScheduling(false);
      if (commitIfBeneficial(MaxPressure)) {
        SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for presssure, "));
        kernel.fg.builder->getJitInfo()->statsVerbose.minRegSUCount++;
        Changed = true;
      }
    }
  }
  if (!Changed)
    kernel.fg.builder->getJitInfo()->statsVerbose.minRegRestCount++;
  return Changed;
}

void BB_Scheduler::SethiUllmanScheduling(bool DoClustering) {
  schedule.clear();
  SethiUllmanQueue Q(ddd, rp, config);
  Q.push(ddd.getExitNode());

  while (!Q.empty()) {
    preNode *N = Q.pickNode();
    std::vector<preNode *> W;
    if (DoClustering)
      Q.formWorkingSet(N, W);
    else
      W.push_back(N);
    while (!W.empty()) {
      N = W.back();
      W.pop_back();
      vASSERT(!N->isScheduled && N->NumSuccsLeft == 0);
      if (N->getInst() != nullptr) {
        // std::cerr << "emit: "; N->getInst()->dump();
        if (N->getInst()->isSend() && N->getTupleLead()) {
          // If it's the pair of the current node, reset the node to be
          // paired. If it's send with pair, ensure its pair is scheduled
          // before other sends by setting the current node to be paired.
          if (!Q.getCurrTupleLead())
            Q.setCurrTupleLead(N);
          if (Q.getCurrTupleLead())
            Q.updateCurrTupleLead(N);
        }
        schedule.push_back(N->getInst());
        N->isScheduled = true;
      }
      // update LiveTS, which is used both in priority-comparision
      // and register-releasing scheduling
      for (auto I = N->pred_begin(), E = N->pred_end(); I != E; ++I) {
        preNode *Node = I->getNode();
        vASSERT(!Node->isScheduled && Node->NumSuccsLeft);
        if (I->isFlowDep()) {
          Q.LiveTS[Node->getID()] =
              std::min(Q.LiveTS[Node->getID()], (unsigned)schedule.size());
        }
      }
      for (auto I = N->pred_begin(), E = N->pred_end(); I != E; ++I) {
        preNode *Node = I->getNode();
        --Node->NumSuccsLeft;
        if (Node->NumSuccsLeft == 0) {
          if (Node->getInst() == nullptr) {
            W.push_back(Node);
          } else if (Node->getInst()->isSend() && Node->getTupleLead()) {
            Q.push(Node);
          } else {
            // if newly available node does not increase pressure,
            // schedule it immediately
            int SzDelta = (Q.LiveTS[Node->getID()] <= schedule.size())
                              ? (int)Q.DstSizes[Node->getID()]
                              : 0;
            for (auto J = Node->pred_begin(), JE = Node->pred_end(); J != JE;
                 ++J) {
              if (J->isFlowDep()) {
                auto NodeJ = J->getNode();
                if (Q.LiveTS[NodeJ->getID()] > schedule.size())
                  SzDelta = SzDelta - (int)Q.DstSizes[NodeJ->getID()];
              }
            }
            if (SzDelta >= 0)
              W.push_back(Node);
            else
              Q.push(Node);
          }
        }
      }
    }
  }

  vASSERT(verifyScheduling());
}

namespace {

template<G4_RegFileKind Kind>
class AregUsage {
  struct Write {
    preNode *N = nullptr;
    unsigned NumRegsUsed = 0;
  };
  // The list is to use to track active scheduled nodes that write Aregs.
  std::list<Write> Writes;
  // Numbers of physical architecture registers
  const unsigned PhyRegNum;
  // The total Aregs used by active Areg writes.
  unsigned NumRegsUsed = 0;

public:
  AregUsage(unsigned N) : PhyRegNum(N) {}

  bool isPressureHigh() const { return NumRegsUsed >= PhyRegNum; }

  unsigned numRegsUsed() const { return NumRegsUsed; }

  unsigned numPhysicalRegs() const { return PhyRegNum; }

  void update(preNode *N);

  unsigned numActiveRegWritten(const G4_INST *Inst) const;

  unsigned numActiveRegRead(const G4_INST *Inst) const;
};

template<>
void AregUsage<G4_FLAG>::update(preNode *N) {
  vASSERT(N && N->isScheduled);
  // Only update flag usage when the scheduled node uses flag through condition
  // modifier and predicate. Currently assume the register allocator could do
  // the optimal assignment and simply add the numbers of flag registers used
  // by active flag writes to represent the total active flag registers.
  // TODO: Consider tracking flag uses through src and dst as well.
  G4_INST *NInst = N->getInst();
  if (!NInst || !NInst->usesFlag())
    return;

  auto allFlagUsesScheduled = [](preNode *Wr) -> bool {
    return std::all_of(Wr->Succs.begin(), Wr->Succs.end(), [Wr](preEdge &E) {
      preNode *Succ = E.getNode();
      G4_INST *SuccInst = Succ->getInst();
      return !SuccInst ||
          SuccInst->getPredicateBase() != Wr->getInst()->getCondModBase() ||
          Succ->isScheduled;
    });
  };

  // Erase a flag write if
  // 1. the new node that has the same cond mod, or
  // 2. all of its successors that use the flag are scheduled
  bool WriteFlag = NInst->getCondMod() && NInst->opcode() != G4_sel;
  for (auto IT = Writes.begin(), IE = Writes.end(); IT != IE; ) {
    Write &Wr = *IT;
    if (WriteFlag &&
        NInst->getCondModBase() == Wr.N->getInst()->getCondModBase()) {
      NumRegsUsed -= Wr.NumRegsUsed;
      IT = Writes.erase(IT);
    } else if (allFlagUsesScheduled(Wr.N)) {
      NumRegsUsed -= Wr.NumRegsUsed;
      IT = Writes.erase(IT);
    } else
      ++IT;
  }
  // Append the new flag write to the end.
  if (WriteFlag) {
    unsigned NumRegsNeeded =
        NInst->getCondMod()->getTopDcl()->getNumRegNeeded();
    vASSERT(NumRegsNeeded == 1 || NumRegsNeeded == 2);
    NumRegsUsed += NumRegsNeeded;
    Writes.push_back({N, NumRegsNeeded});
  }
}

template<>
unsigned AregUsage<G4_FLAG>::numActiveRegWritten(const G4_INST *Inst) const {
  if (!Inst || !Inst->getCondMod() || Inst->opcode() == G4_sel)
    return 0;

  for (const Write &Wr : Writes) {
    if (Inst->getCondModBase() == Wr.N->getInst()->getCondModBase())
      return Wr.NumRegsUsed;
  }
  return 0;
}

template<>
unsigned AregUsage<G4_FLAG>::numActiveRegRead(const G4_INST *Inst) const {
  if (!Inst || !Inst->getPredicate())
    return 0;

  for (const Write &Wr : Writes) {
    if (Inst->getPredicateBase() == Wr.N->getInst()->getCondModBase())
      return Wr.NumRegsUsed;
  }
  return 0;
}

// Queue for scheduling to hide latency.
class LatencyQueue : public QueueBase {
  // Represent the type of SchedCandidate picked from ready-list. The types are
  // listed in decreasing priority.
  enum CandReason : uint8_t {
    NoCand,
    PseudoKill,
    AregPhyLimit,
    NodeGroup,
    SendNoReturn,
    NodePriority,
    Send,
    // NodeID (order) is the lowest priority
    NodeID = std::numeric_limits<uint8_t>::max(),
  };

  const char *getReasonStr(CandReason Reason) const {
    switch (Reason) {
    case NoCand:         return "NOCAND";
    case PseudoKill:     return "KILL";
    case AregPhyLimit:   return "AREGPHYLIMIT";
    case NodeGroup:      return "GROUP";
    case SendNoReturn:   return "SENDNORET";
    case NodePriority:   return "PRIORITY";
    case Send:           return "SEND";
    case NodeID:         return "ID";
    };
    vISA_ASSERT_UNREACHABLE("Unknown reason!");
    return "UNKNOWN";
  }

  struct SchedCandidate {
    preNode* Node = nullptr;
    CandReason Reason = NoCand;
    // Number of flag register added if selecting the candidate. A negative
    // number means the candidate reads an active flag and could release the
    // flag potentially.
    int NumFlagRegsAdded = 0;

    bool isValid() const { return Node; }
  };

  // Assign a priority to each node.
  std::vector<unsigned> Priorities;

  // Map each instruction to a group ID. Instructions in the same
  // group will be scheduled for latency.
  std::map<G4_INST *, unsigned> GroupInfo;

  // Instruction latency information.
  const LatencyTable &LT;

  // nodes with all predecessors scheduled and ready-cycle <= current-cycle for
  // topdown scheduling
  std::vector<preNode *> ReadyList;
  // nodes with all predecessors scheduled and ready-cycle > current-cycle for
  // topdown scheduling
  std::priority_queue<preNode *, std::vector<preNode *>,
                      std::function<bool(preNode *, preNode *)>>
      HoldList;
  // The register-pressure limit we use to decide sub-blocking
  unsigned GroupingPressureLimit;

  AregUsage<G4_FLAG> FlagUsage;

public:
  LatencyQueue(preDDD &ddd, RegisterPressure &rp, SchedConfig config,
               const LatencyTable &LT, unsigned GroupingThreshold)
      : QueueBase(ddd, rp, config), LT(LT),
        HoldList([this](preNode *a, preNode *b) { return compareHold(a, b); }),
        GroupingPressureLimit(GroupingThreshold),
        FlagUsage(ddd.getKernel().fg.builder->getNumFlagRegisters()) {
    init();
  }

  // Add a new node to queue.
  void push(preNode *N) override {
    // Always add pseudo_kill to ready-list directly so that group info
    // advancing won't be affected when moving pseudo_kill from host-list to
    // ready-list.
    if (config.SkipHoldList)
      ReadyList.push_back(N);
    else if (N->getInst() && N->getInst()->isPseudoKill())
      ReadyList.push_back(N);
    else
      HoldList.push(N);
  }

  // Pick a node based on heuristics.
  preNode *pickNode() override {
    vASSERT(!ReadyList.empty());
    SchedCandidate Cand;
    auto IT = ReadyList.begin(), IE = ReadyList.end(), IC = IE;
    for (; IT != IE; ++IT) {
      SchedCandidate TryCand = {*IT};
      initCandidate(TryCand);
      if (tryCandidate(Cand, TryCand)) {
        Cand = TryCand;
        IC = IT;
      }
    }
    vASSERT(IC != IE && *IC == Cand.Node && Cand.Reason != NoCand);
    std::swap(*IC, ReadyList.back());
    ReadyList.pop_back();
    SCHED_DUMP({
        std::cerr << "Picking a node based on reason "
                  << getReasonStr(Cand.Reason) << ": ";
        Cand.Node->dump();
    });

    return Cand.Node;
  }

  bool empty() const { return ReadyList.empty(); }

  // move instruction from HoldList to ReadyList,
  // also update current-cycle and current-group
  void advance(unsigned &CurCycle, unsigned &CurGroup) {
    if (config.SkipHoldList) {
      vASSERT(HoldList.empty());
      return;
    }
    GroupInfo[nullptr] = CurGroup;
    // move inst out of hold-list based on current group and cycle
    while (!HoldList.empty()) {
      preNode *N = HoldList.top();
      if (GroupInfo[N->getInst()] <= CurGroup &&
          N->getReadyCycle() <= CurCycle) {
        HoldList.pop();
        ReadyList.push_back(N);
      } else
        break;
    }
    // ready-list is still empty, then we need to move forward to
    // the next group or the next cycle so that some instructions
    // can come out of the hold-list.
    if (ReadyList.empty() && !HoldList.empty()) {
      preNode *N = HoldList.top();
      CurCycle = std::max(CurCycle, N->getReadyCycle());
      CurGroup = std::max(CurGroup, GroupInfo[N->getInst()]);
      do {
        preNode *N = HoldList.top();
        if (GroupInfo[N->getInst()] <= CurGroup &&
            N->getReadyCycle() <= CurCycle) {
          HoldList.pop();
          ReadyList.push_back(N);
        } else
          break;
      } while (!HoldList.empty());
    }
  }

  void updateAregUsage(preNode* N) {
    FlagUsage.update(N);
  }

private:
  void init();
  unsigned calculatePriority(preNode *N);

  // Return which candidate is better (less) if the heuristic determines
  // a less-than order. Return no candidate if the order can't be determined.
  std::optional<SchedCandidate *> tryLess(int TryVal, int CandVal,
      SchedCandidate &TryCand, SchedCandidate &Cand, CandReason Reason) const {
    // 1. Return TryCand as it is better. Also update its reason.
    if (TryVal < CandVal) {
      TryCand.Reason = Reason;
      return std::make_optional(&TryCand);
    }
    // 2. Return Cand as it is better. Update its reason if the priority
    //    (reason) of the heuristic is higher (smaller).
    if (TryVal > CandVal) {
      if (Cand.Reason > Reason)
        Cand.Reason = Reason;
      return std::make_optional(&Cand);
    }
    return std::nullopt;
  }

  // Return which candidate is better (greater) if the heuristic determines
  // a greater-than order. Return no candidate if the order can't be
  // determined.
  std::optional<SchedCandidate *> tryGreater(int TryVal, int CandVal,
      SchedCandidate &TryCand, SchedCandidate &Cand, CandReason Reason) const {
    // 1. Return TryCand as it is better. Also update its reason.
    if (TryVal > CandVal) {
      TryCand.Reason = Reason;
      return std::make_optional(&TryCand);
    }
    // 2. Return Cand as it is better. Update its reason if the priority
    //    (reason) of the heuristic is higher (smaller).
    if (TryVal < CandVal) {
      if (Cand.Reason > Reason)
        Cand.Reason = Reason;
      return std::make_optional(&Cand);
    }
    return std::nullopt;
  }

  // Return the candidate that won't increase flag pressure above the physical
  // limit and could avoid flag spills potentially. Note that currently this
  // heuristic only considers flag uses in condition modifier and predicate,
  // and does not consider flag in src or dst.
  std::optional<SchedCandidate *> tryCandidateToAvoidFlagSpill(
      SchedCandidate &Cand, SchedCandidate &TryCand) const {
    // If both won't cause spill, they are equally good. Otherwise, favor the
    // one that has lower pressure.
    int PhyRegNum = FlagUsage.numPhysicalRegs();
    int TryCandPressure = TryCand.NumFlagRegsAdded + FlagUsage.numRegsUsed();
    int CandPressure = Cand.NumFlagRegsAdded + FlagUsage.numRegsUsed();
    int TryCandVal = TryCandPressure > PhyRegNum ? TryCandPressure : PhyRegNum;
    int CandVal = CandPressure > PhyRegNum ? CandPressure : PhyRegNum;
    if (auto Res = tryLess(TryCandVal, CandVal, TryCand, Cand, AregPhyLimit))
      return Res;

    return std::nullopt;
  }

  void initCandidate(SchedCandidate &TryCand) {
    G4_INST *Inst = TryCand.Node->getInst();
    // Only record the flag stats for the candidate when it uses flag.
    if (Inst && Inst->usesFlag()) {
      // Update NumFlagRegsAdded if the TryCand writes to a different flag than
      // active flag writes.
      if (Inst->getCondMod() && Inst->opcode() != G4_sel &&
          FlagUsage.numActiveRegWritten(Inst) == 0) {
        TryCand.NumFlagRegsAdded +=
            Inst->getCondMod()->getTopDcl()->getNumRegNeeded();
        vASSERT(TryCand.NumFlagRegsAdded == 1 || TryCand.NumFlagRegsAdded == 2);
      }
      // Update NumFlagRegsAdded if the TryCand reads any active flag register
      // and does not overwrite the same flag.
      // TODO: Consider something like assigning a fractional weight if the
      // flag has multiple uses to favor the only use or the last use in some
      // way. Currently treat all uses as the only use to simplify the
      // implementation.
      if (Inst->getPredicateBase() != Inst->getCondModBase())
        TryCand.NumFlagRegsAdded -= FlagUsage.numActiveRegRead(Inst);
    }
  }

  // Return true if TryCand is better than current Cand. The heuristics should
  // be ordered based on their priority.
  bool tryCandidate(SchedCandidate &Cand, SchedCandidate &TryCand) {
    // Initialize the candidate. If Cand is invalid, just use the first TryCand
    // and set the reason to the lowest priority.
    if (!Cand.isValid()) {
      TryCand.Reason = NodeID;
      return true;
    }

    preNode *CandNode = Cand.Node;
    preNode *TryCandNode = TryCand.Node;
    G4_INST *CandInst = CandNode->getInst();
    G4_INST *TryCandInst = TryCandNode->getInst();

    // Try scheduling pseudo kill first.
    bool IsCandPseudoKill = CandInst && CandInst->isPseudoKill();
    bool IsTryCandPseudoKil = TryCandInst && TryCandInst->isPseudoKill();
    if (auto Res = tryGreater(IsTryCandPseudoKil, IsCandPseudoKill, TryCand,
                              Cand, PseudoKill))
      return Res.value() == &TryCand;

    // Avoid increasing the flag pressure.
    if (auto Res = tryCandidateToAvoidFlagSpill(Cand, TryCand))
      return Res.value() == &TryCand;

    // Group ID has higher priority, smaller ID means higher priority.
    unsigned CandGID = GroupInfo[Cand.Node->getInst()];
    unsigned TryCandGID = GroupInfo[TryCand.Node->getInst()];
    if (auto Res = tryLess(TryCandGID, CandGID, TryCand, Cand, NodeGroup))
      return Res.value() == &TryCand;

    // Favor sends without return such as stores or urb-writes because they
    // likely release source registers.
    // FIXME: Check why the heuristic is good for latency scheduling.
    auto isSendNoReturn = [](G4_INST *Inst) -> bool {
      return Inst && Inst->isSend() && Inst->getDst()->isNullReg();
    };
    bool IsCandSendNoRet = isSendNoReturn(CandInst);
    bool IsTryCandSendNoRet = isSendNoReturn(TryCandInst);
    if (auto Res = tryGreater(IsTryCandSendNoRet, IsCandSendNoRet, TryCand,
                              Cand, SendNoReturn))
      return Res.value() == &TryCand;

    // Within the same group, compare their priority.
    unsigned CandPriority = Priorities[CandNode->getID()];
    unsigned TryCandPriority = Priorities[TryCandNode->getID()];
    if (auto Res = tryGreater(TryCandPriority, CandPriority, TryCand, Cand,
                              NodePriority))
      return Res.value() == &TryCand;

    // Favor sends.
    bool IsCandSend = CandInst && CandInst->isSend();
    bool IsTryCandSend = TryCandInst && TryCandInst->isSend();
    if (auto Res = tryGreater(IsTryCandSend, IsCandSend, TryCand, Cand, Send))
      return Res.value() == &TryCand;

    // Otherwise, break tie on ID. Larger ID means higher priority.
    if (auto Res = tryGreater(TryCandNode->getID(), CandNode->getID(), TryCand,
                              Cand, NodeID))
      return Res.value() == &TryCand;

    return false;
  }

  bool compareHold(preNode *N1, preNode *N2);
};

} // namespace

bool BB_Scheduler::scheduleBlockForLatency(unsigned &MaxPressure,
                                           bool ReassignID,
                                           unsigned UpperBoundGRF) {
  auto tryLatencyHiding = [this, UpperBoundGRF, MaxPressure](unsigned nr) {
    if (!config.UseLatency)
      return false;

    // UpperBoundGRF == 0 means we are scheduling for the fixed number of GRF
    if (UpperBoundGRF == 0 &&
        MaxPressure >= getLatencyHidingThreshold(kernel, nr))
      return false;

    // simple ROI check.
    unsigned NumOfHighLatencyInsts = 0;
    for (auto Inst : *(ddd.getBB())) {
      if (Inst->isSend()) {
        G4_SendDesc *MsgDesc = Inst->getMsgDesc();
        if (MsgDesc->isRead() || MsgDesc->isSampler() || MsgDesc->isAtomic())
          NumOfHighLatencyInsts++;
      }
    }

    return NumOfHighLatencyInsts >= 2;
  };

  unsigned NumGrfs =
      kernel.getNumRegTotal() +
      kernel.getOptions()->getuInt32Option(vISA_preRA_ScheduleExtraGRF);

  if (!tryLatencyHiding(NumGrfs))
    return false;

  // UpperBoundGRF == 0 means we only schedule under single NumGRF
  // setting for this block instead of trying to find the best schedule
  // among multiple NumGRF settings.
  if (UpperBoundGRF == 0)
    UpperBoundGRF = NumGrfs;
  unsigned SavedEstimation = 0;
  std::vector<G4_INST *> SavedSchedule;

  for (; NumGrfs <= UpperBoundGRF; NumGrfs += 32) {
    // try grouping-threshold decremently until we find a schedule likely won't
    // spill
    unsigned Thresholds[] = {144, 128, 112, 104, 96};
    unsigned Iterations = 5;
    float Ratio = (std::max(NumGrfs, 128u) - 48u) / 80.0f;

    if (kernel.fg.builder
            ->hasFiveALUPipes()) { // One more iteration on new platforms
      Iterations++;
    }
    // limit the iterative approach to certain platforms for now
    if (config.DoNotIterate) {
      Iterations = 1;
    }

    for (unsigned i = 0; i < Iterations; ++i) {
      unsigned GroupingThreshold = 0;
      if (config.DoNotIterate) {
        GroupingThreshold = getLatencyHidingThreshold(kernel, NumGrfs);
        Ratio = 1.0f; // already adjusted inside getLatencyHidingThreshold
      } else if (kernel.fg.builder->hasFiveALUPipes() &&
                 i == Iterations - 1) {
        // the last iteration uses the threshold from getLatencyHidingThreshold
        GroupingThreshold = getLatencyHidingThreshold(kernel, NumGrfs);
        if (GroupingThreshold * 1.0 > Thresholds[i - 1] * Ratio) {
          break;
        }
        Ratio = 1.0f;
      } else {
        GroupingThreshold = Thresholds[i];
      }

      ddd.reset(ReassignID);
      ReassignID = false; // only reassign inst-local-id at most once
      LatencyScheduling(unsigned(GroupingThreshold * Ratio));
      if (commitIfBeneficial(MaxPressure, /*IsTopDown*/ true, NumGrfs)) {
        if (NumGrfs >= UpperBoundGRF && SavedEstimation == 0) {
          SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for latency, "));
          return true;
        }
        // if this schedule does not provide expected gain from
        // previous schedule, stop searching
        if (SavedEstimation > 0 && SavedSchedule.size() == schedule.size() &&
            CycleEstimation * 4 > SavedEstimation * 3) {
          // commit the previous schedule as the best
          INST_LIST &CurInsts = getBB()->getInstList();
          CurInsts.clear();
          for (auto Inst : SavedSchedule)
            CurInsts.push_back(Inst);
          rp.recompute(getBB());
          SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for latency, "));
          return true;
        }
        if (NumGrfs >= UpperBoundGRF) {
          // best schedule is found with the max GRF setting
          SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for latency, "));
          return true;
        }
        // save the current schedule as the potential choice
        // but do not commit it
        SavedSchedule.swap(schedule);
        SavedEstimation = CycleEstimation;
        schedule.clear();
        // restore the original instruction order
        restoreOriginalList();
        // try the next GRF level
        break;
      }
    }
  }
  if (SavedEstimation > 0 && SavedSchedule.size() > 0) {
    // commit the previous schedule as the best
    INST_LIST &CurInsts = getBB()->getInstList();
    vASSERT(SavedSchedule.size() == CurInsts.size());
    CurInsts.clear();
    for (auto Inst : SavedSchedule)
      CurInsts.push_back(Inst);
    rp.recompute(getBB());
    SCHED_DUMP(rp.dump(ddd.getBB(), "After scheduling for latency, "));
    return true;
  }
  return false;
}

// Scheduling block to hide latency (top down).
void BB_Scheduler::LatencyScheduling(unsigned GroupingThreshold) {
  schedule.clear();
  LatencyQueue Q(ddd, rp, config, LT, GroupingThreshold);
  Q.push(ddd.getEntryNode());

  unsigned CurrentCycle = 0;
  unsigned CurrentGroup = 0;
  Q.advance(CurrentCycle, CurrentGroup);
  while (!Q.empty()) {
    preNode *N = Q.pickNode();
    vASSERT(N->NumPredsLeft == 0);
    // Set NextCycle as max of CurrentCycle and N->getReadyCycle() to include
    // the stall.
    unsigned NextCycle = std::max(CurrentCycle, N->getReadyCycle());
    if (N->getInst()) {
      schedule.push_back(N->getInst());
      NextCycle += LT.getOccupancy(N->getInst());
    }
    N->isScheduled = true;
    // Update AReg usage after scheduling a node.
    Q.updateAregUsage(N);

    for (auto I = N->succ_begin(), E = N->succ_end(); I != E; ++I) {
      preNode *Node = I->getNode();
      vASSERT(!Node->isScheduled && Node->NumPredsLeft);
      int L = (*I).getLatency();
      vASSERT(L >= 0);
      if (Node->getReadyCycle() < CurrentCycle + (unsigned)L)
        Node->setReadyCycle(CurrentCycle + (unsigned)L);
      --Node->NumPredsLeft;
      if (Node->NumPredsLeft == 0) {
        Q.push(Node);
      }
    }
    CurrentCycle = NextCycle;
    Q.advance(CurrentCycle, CurrentGroup);
  }
  CycleEstimation = CurrentCycle;
  relocatePseudoKills();
  vASSERT(verifyScheduling());
}

static void mergeSegments(const std::vector<unsigned> &RPtrace,
                          const std::vector<unsigned> &Max,
                          const std::vector<unsigned> &Min,
                          std::vector<unsigned> &Segments, unsigned Threshold) {
  unsigned n = std::min<unsigned>((unsigned)Max.size(), (unsigned)Min.size());
  vASSERT(n >= 2);

  // Starts with a local minimum.
  /*
      C        /\
              /  \
      A \    /    \
      D  \  /      \
      B   \/
  */
  if (Min[0] < Max[0]) {
    unsigned Hi = RPtrace[0];
    unsigned Lo = RPtrace[Min[0]];

    for (unsigned i = 1; i < n; ++i) {
      unsigned Hi2 = RPtrace[Max[i - 1]];
      unsigned Lo2 = RPtrace[Min[i]];

      if ((Hi2 - Lo + Hi) <= Threshold) {
        Hi = Hi2 - Lo + Hi;
        Lo = Lo2;
      } else {
        // Do not merge two segments, and end the last group.
        Segments.push_back(Min[i - 1]);
        Hi = Hi2;
        Lo = Lo2;
      }
    }

    // If ends with a local maximal, then Hi2 is the last max;
    // otherwise it is the end pressure.
    unsigned Hi2 =
        (Min.back() > Max.back()) ? RPtrace.back() : RPtrace[Max.back()];
    if ((Hi2 - Lo + Hi) > Threshold)
      Segments.push_back(Min.back());
    return;
  }

  // Starts with local maximum.
  /*
      D             /\
                   /  \
      B     /\    /    \
           /  \  /      \
      C   /    \/
         /
      A /
  */
  unsigned Hi = RPtrace[Max[0]];
  unsigned Lo = RPtrace[Min[0]];
  for (unsigned i = 1; i < n; ++i) {
    unsigned Hi2 = RPtrace[Max[i]];
    unsigned Lo2 = RPtrace[Min[i]];

    // Merge two segments
    if ((Hi2 - Lo + Hi) <= Threshold) {
      Hi = Hi2 - Lo + Hi;
      Lo = Lo2;
    } else {
      // Do not merge two segments, and end the last group.
      Segments.push_back(Min[i - 1]);
      Hi = Hi2;
      Lo = Lo2;
    }
  }

  // If ends with a local maximal, then Hi2 is the last max;
  // otherwise it is the end pressure.
  unsigned Hi2 =
      (Min.back() > Max.back()) ? RPtrace.back() : RPtrace[Max.back()];
  if ((Hi2 - Lo + Hi) > Threshold)
    Segments.push_back(Min.back());
}

void LatencyQueue::init() {
  G4_BB *BB = ddd.getBB();

  // Scan block forward and group instructions, without causing
  // excessive pressure increase. First collect the register
  // pressure trace in this block.
  std::vector<unsigned> RPtrace;
  RPtrace.reserve(BB->size());
  for (auto Inst : *BB) {
    // Ignore pseudo-kills as they often introduce inaccurate rp bumps.
    if (Inst->isPseudoKill()) {
      unsigned prevRP = RPtrace.empty() ? 0 : RPtrace.back();
      RPtrace.push_back(prevRP);
    } else
      RPtrace.push_back(rp.getPressure(Inst));
  }

  // Collect all local maximum and minimum.
  std::vector<unsigned> Max, Min;
  bool IsIncreasing = true;
  for (unsigned i = 1; i + 1 < RPtrace.size(); ++i) {
    if (RPtrace[i] > RPtrace[i - 1])
      IsIncreasing = true;
    else if (RPtrace[i] < RPtrace[i - 1])
      IsIncreasing = false;

    if (RPtrace[i] > RPtrace[i - 1] && RPtrace[i] > RPtrace[i + 1])
      Max.push_back(i);
    else if (IsIncreasing && RPtrace[i] == RPtrace[i - 1] &&
             RPtrace[i] > RPtrace[i + 1])
      Max.push_back(i);
    else if (RPtrace[i] < RPtrace[i - 1] && RPtrace[i] < RPtrace[i + 1])
      Min.push_back(i);
    else if (!IsIncreasing && RPtrace[i] == RPtrace[i - 1] &&
             RPtrace[i] < RPtrace[i + 1])
      Min.push_back(i);
  }

  if (Max.size() <= 1 || Min.size() <= 1) {
    // Simple case, there is only a single group.
    for (auto Inst : *BB)
      GroupInfo[Inst] = 0;
  } else {
    // Multiple high/low pressure segments. We merge consective segments
    // without breaking rpe limit. E.g. given [A, B, C, D, E] with A < B >
    // C and C < D > E, B, D are local maximum and C is a local minimum.
    // We merge [A, B, C] with [C, D, E] when B + D - C <= THRESHOLD,
    // resulting a larger segment [A, B + D - C, E]. Approximately,
    // B + D - C bounds the maximal pressure in this merged segment [A, E].
    // Otherwise, do not merge [A, C] with [C, E], C ends the current group,
    // and starts a new group.
    //
    std::vector<unsigned> Segments;
    mergeSegments(RPtrace, Max, Min, Segments, GroupingPressureLimit);

    // Iterate segments and assign a group id to each insstruction.
    unsigned i = 0;
    unsigned j = 0;
    for (auto Inst : *BB) {
      if (j >= Segments.size()) {
        GroupInfo[Inst] = j;
      } else if (i < Segments[j]) {
        GroupInfo[Inst] = j;
      } else {
        GroupInfo[Inst] = j;
        ++j;
      }
      ++i;
    }
  }

  auto &Nodes = ddd.getNodes();
  unsigned N = (unsigned)Nodes.size();
  Priorities.resize(N, 0);
  for (unsigned i = 0; i < N; ++i)
    Priorities[i] = calculatePriority(Nodes[i]);

#if 0
    std::cerr << "\n\n";
    for (auto I = Nodes.rbegin(); I != Nodes.rend(); ++I) {
        std::cerr << "(GroupId, Priority, RPE) = ("
                  << std::setw(2)<< GroupInfo[(*I)->getInst()]
                  << ", " << std::setw(4) << Priorities[(*I)->getID()]
                  << ", " << std::setw(3)<< rp.getPressure((*I)->getInst())
                  << ") ";
        (*I)->getInst()->dump();
    }
    std::cerr << "\n\n";
#endif
}

unsigned LatencyQueue::calculatePriority(preNode *N) {
  G4_INST *Inst = N->getInst();
  if (!Inst)
    return 0;

  vASSERT(N->getID() < Priorities.size());
  unsigned CurPriority = Priorities[N->getID()];
  if (CurPriority > 0)
    return CurPriority;

  // Check if an edge is setting a0 operand for a send.
  auto isHeaderOnAddr = [](preNode *N, preEdge &E) {
    // Check if N is writing to address.
    G4_INST *Inst = N->getInst();
    if (!Inst || !Inst->getDst() || !Inst->getDst()->isDirectAddress())
      return false;

    // Check if this use is on send.
    preNode *T = E.getNode();
    if (T->getInst() && T->getInst()->isSend())
      return true;

    // By default.
    return false;
  };

  unsigned Priority = 0;
  for (auto I = N->succ_begin(), E = N->succ_end(); I != E; ++I) {
    // Recurse on the successors.
    auto &Edge = *I;
    unsigned SuccPriority = calculatePriority(Edge.getNode());
    unsigned Latency = 0;

    if (Inst && !Inst->isPseudoKill() && Edge.isDataDep()) {
      switch (Edge.getType()) {
      case RAW:
        // By setting Latency to 0, this moves address initializations
        // close to sends.
        if (isHeaderOnAddr(N, Edge))
          break;
        // fall through
      case RAW_MEMORY:
      case WAW:
        Latency = LT.getLatency(Inst);
        break;
      default:
        break;
      }
    }
    Edge.setLatency(Latency);
    Priority = std::max(Priority, SuccPriority + Latency);
  }

  return std::max(1U, Priority);
}

// hold-list is sorted by nodes' ready cycle
bool LatencyQueue::compareHold(preNode *N1, preNode *N2) {
  vASSERT(N1->getID() != N2->getID());
  vASSERT(N1->getInst() && N2->getInst());
  G4_INST *Inst1 = N1->getInst();
  G4_INST *Inst2 = N2->getInst();

  // Group ID has higher priority, smaller ID means higher priority.
  unsigned GID1 = GroupInfo[Inst1];
  unsigned GID2 = GroupInfo[Inst2];
  if (GID1 > GID2)
    return true;
  if (GID1 < GID2)
    return false;

  // compare ready cycle, smaller ready cycle means higher priority
  unsigned cyc1 = N1->getReadyCycle();
  unsigned cyc2 = N2->getReadyCycle();
  if (cyc1 > cyc2)
    return true;
  if (cyc1 < cyc2)
    return false;

  // Otherwise, break tie on ID.
  // Larger ID means higher priority.
  return N2->getID() > N1->getID();
}

// Find the edge with smallest ID.
static preNode *minElt(const std::vector<preEdge> &Elts) {
  vASSERT(!Elts.empty());
  if (Elts.size() == 1)
    return Elts.front().getNode();
  auto Cmp = [](const preEdge &E1, const preEdge &E2) {
    G4_INST *Inst1 = E1.getNode()->getInst();
    G4_INST *Inst2 = E2.getNode()->getInst();
    return Inst1 && Inst2 && Inst1->getLocalId() < Inst2->getLocalId();
  };
  auto E = std::min_element(Elts.begin(), Elts.end(), Cmp);
  return E->getNode();
}

// Given an instruction stream [p1 p2 A1 A2 A4 A3]
// with dependency p1 -> {A1, A2}, p2-> {A3, A4},
// for p in {p1, p2}, we compute the earliest location to insert
// the pseudo kill p, p1<-A1, p2<-A4, and shuffle the stream to
// [p1 A1 A2 p2 A4 A3].
void BB_Scheduler::relocatePseudoKills() {
  // Reset local id after scheduling and build the location map.
  // Multiple pseudo-kills may be placed before a single instruction.
  std::unordered_map<G4_INST *, std::vector<G4_INST *>> LocMap;
  std::vector<G4_INST *> KillsWithoutUse;
  int i = 0;
  for (auto Inst : schedule) {
    Inst->setLocalId(i++);
  }

  G4_INST *LastBarrier = nullptr;
  for (auto N : ddd.getNodes()) {
    G4_INST *Inst = N->getInst();
    // All dangling pseudo-kills shall be placed before a barrier.
    if (preNode::isBarrier(Inst)) {
      if (LastBarrier && !KillsWithoutUse.empty()) {
        LocMap[LastBarrier].swap(KillsWithoutUse);
        vASSERT(KillsWithoutUse.empty());
      }
      LastBarrier = Inst;
    }

    if (Inst && Inst->isPseudoKill()) {
      preNode *Pos = minElt(N->Succs);
      while (Pos->getInst() && Pos->getInst()->isPseudoKill())
        Pos = minElt(Pos->Succs);

      if (Pos->getInst() == nullptr)
        KillsWithoutUse.push_back(Inst);
      else
        LocMap[Pos->getInst()].push_back(Inst);
    }
  }

  // Do nothing if there is no pseudo-kill.
  if (LocMap.empty() && KillsWithoutUse.empty())
    return;

  // Do relocation.
  std::vector<G4_INST *> relocated;
  relocated.reserve(schedule.size());
  for (auto Inst : schedule) {
    // pseudo-kills will be relocated.
    if (Inst->isPseudoKill())
      continue;
    auto I = LocMap.find(Inst);
    if (I != LocMap.end())
      relocated.insert(relocated.end(), I->second.begin(), I->second.end());
    relocated.push_back(Inst);
  }

  // Put remaining dangling pseudo-kills at the end of the block.
  if (!KillsWithoutUse.empty())
    relocated.insert(relocated.end(), KillsWithoutUse.begin(),
                     KillsWithoutUse.end());

  std::swap(schedule, relocated);
}

// Commit this scheduling if it is better.
bool BB_Scheduler::commitIfBeneficial(unsigned &MaxRPE, bool IsTopDown,
                                      unsigned NumGrfs) {
  INST_LIST &CurInsts = getBB()->getInstList();
  if (schedule.size() != CurInsts.size()) {
    SCHED_DUMP(std::cerr << "schedule reverted due to mischeduling.\n\n");
    return false;
  }
  if (IsTopDown) {
    if (std::equal(CurInsts.begin(), CurInsts.end(), schedule.begin())) {
      SCHED_DUMP(std::cerr << "schedule not committed due to no change.\n\n");
      return false;
    }
  } else if (std::equal(CurInsts.begin(), CurInsts.end(), schedule.rbegin())) {
    SCHED_DUMP(std::cerr << "schedule not committed due to no change.\n\n");
    return false;
  }
  saveOriginalList();
  // evaluate this scheduling.
  if (IsTopDown)
    for (auto Inst : schedule)
      CurInsts.push_back(Inst);
  else
    for (auto Inst : schedule)
      CurInsts.push_front(Inst);

  rp.recompute(getBB());
  unsigned NewRPE = rp.getPressure(getBB());
  unsigned LatencyPressureThreshold =
      getLatencyHidingThreshold(kernel, NumGrfs);
  if (config.UseLatency && IsTopDown) {
    // For hiding latency.
    if (NewRPE <= LatencyPressureThreshold) {
      SCHED_DUMP(std::cerr << "schedule committed for latency.\n\n");
      MaxRPE = NewRPE;
      return true;
    } else {
      SCHED_DUMP(std::cerr << "the pressure is increased to " << NewRPE
                           << "\n");
    }
  } else {
    // For reducing rpe.
    if (NewRPE < MaxRPE &&
        (MaxRPE - NewRPE) * 100 >= PRESSURE_REDUCTION_MIN_BENEFIT * MaxRPE) {
      bool AbortOnSpill = kernel.getOptions()->getOption(vISA_AbortOnSpill);
      if (isSlicedSIMD32(kernel) && AbortOnSpill) {
        // It turns out that simd32 kernels may be scheduled like slicing, which
        // in general hurts latency hidding. If not insist to compile for
        // simd32, make rp reduction conservative.
        //
        if (NewRPE < LatencyPressureThreshold) {
          SCHED_DUMP(std::cerr
                     << "schedule committed with reduced pressure.\n\n");
          MaxRPE = NewRPE;
          return true;
        }
      } else {
        SCHED_DUMP(std::cerr
                   << "schedule committed with reduced pressure.\n\n");
        MaxRPE = NewRPE;
        return true;
      }
    } else if (NewRPE < MaxRPE) {
      SCHED_DUMP(std::cerr << "the reduced pressure is " << MaxRPE - NewRPE
                           << "\n");
    }
  }

  SCHED_DUMP(rp.dump(getBB(), "schedule reverted, "));
  restoreOriginalList();
  return false;
}

// Implementation of preNode.
DepType preNode::checkBarrier(G4_INST *Inst) {
  // Check if there is an indirect operand in this instruction.
  auto hasIndirectOpnd = [=]() {
    G4_DstRegRegion *dst = Inst->getDst();
    if (dst && dst->isIndirect())
      return true;
    for (auto opNum : {Opnd_src0, Opnd_src1, Opnd_src2}) {
      G4_Operand *opnd = Inst->getOperand(opNum);
      if (opnd && opnd->isSrcRegRegion() &&
          opnd->asSrcRegRegion()->isIndirect())
        return true;
    }
    return false;
  };

  if (Inst == nullptr)
    return DepType::OPT_BARRIER;
  else if (Inst->isLabel())
    return DepType::DEP_LABEL;
  else if (hasIndirectOpnd())
    return DepType::INDIRECT_ADDR_BARRIER;
  else if (Inst->isSend() && Inst->asSendInst()->isFence())
    return DepType::OPT_BARRIER;
  else if (Inst->opcode() == G4_madm)
    return DepType::OPT_BARRIER;
  else if (Inst->isDpas())
    return DepType::OPT_BARRIER;
  else
    return CheckBarrier(Inst);
}

void preNode::print(std::ostream &os) const {
  os << "ID: " << this->ID << "";
  if (Inst)
    Inst->emit(os);

  os << "Preds: ";
  for (auto &E : this->Preds)
    os << E.getNode()->ID << ",";
  os << "";

  os << "Succs: ";
  for (auto &E : this->Succs)
    os << E.getNode()->ID << ",";
  os << "\n";
}

void preNode::dump() const { print(std::cerr); }

// Implementation of preDDD.

// Build the data dependency bottom up with two simple
// special nodes.
void preDDD::buildGraph() {
  vASSERT(!IsDagBuilt);

  // Starts with the exit node.
  addNodeToGraph(&ExitNode);

  unsigned NumOfInsts = (unsigned)m_BB->size();
  SNodes.reserve(NumOfInsts);

  auto I = m_BB->rbegin(), E = m_BB->rend();
  for (unsigned i = 0; I != E; ++I) {
    preNode *N = new (preNodeAllocator) preNode(*I, i++);
    SNodes.push_back(N);
    addNodeToGraph(N);
  }

  // Ends with the entry node.
  addNodeToGraph(&EntryNode);

  // prune the graph.
  prune();

  // Set DAG is complete.
  IsDagBuilt = true;

  // Initialize perNode data.
  reset();
}

void preDDD::addNodeToGraph(preNode *N) {
  NewLiveOps.clear();
  DepType Dep = N->getBarrier();
  if (Dep != DepType::NODEP) {
    processBarrier(N, Dep);
  } else {
    vISA_ASSERT(N->Inst, "not an instruction");
    processSend(N);
    processReadWrite(N);
  }

  // Adding live node should happen in the end, as illustrated below:
  // add X X 1
  // add Y X 2
  for (auto &Item : NewLiveOps) {
    preNode *N = std::get<0>(Item);
    G4_INST *inst = N->getInst();
    Gen4_Operand_Number OpNum = std::get<1>(Item);
    G4_Operand *Opnd = inst->getOperand(OpNum);
    vASSERT(Opnd != nullptr);
    G4_Declare *Dcl = Opnd->getTopDcl();
    if (inst->isPseudoAddrMovIntrinsic() &&
        OpNum != Gen4_Operand_Number::Opnd_dst) {
      Dcl = Opnd->asAddrExp()->getRegVar()->getDeclare();
    }
    vASSERT(Dcl || Opnd->isPhysicallyAllocatedRegVar());
    if (Dcl) {
      LiveNodes[Dcl].emplace_back(N, OpNum);
    }

    if (Opnd->isPhysicallyAllocatedRegVar())
      LivePhysicalNodes.emplace_back(N, OpNum);
  }

  // Update live nodes on sends.
  G4_INST *Inst = N->getInst();
  if (Inst && Inst->isSend()) {
    vASSERT(!Inst->getMsgDesc()->isScratch());
    LiveSends.push_back(N);
  }

  // No explicit dependency found, so it depends on previous barrier.
  if (N->succ_empty() && N != prevBarrier) {
    vISA_ASSERT(prevBarrier, "out of sync");
    addEdge(N, prevBarrier, prevBarrier->getBarrier());
  }
}

void preDDD::processBarrier(preNode *curNode, DepType Dep) {
  // A barrier kills all live nodes, so add dependency edge to all live
  // nodes and clear.
  for (auto &Nodes : LiveNodes) {
    for (LiveNode &X : Nodes.second) {
      if (X.N->pred_empty()) {
        addEdge(curNode, X.N, Dep);
      }
    }
    Nodes.second.clear();
  }

  for (auto &X : LivePhysicalNodes) {
    if (X.N->pred_empty()) {
      addEdge(curNode, X.N, Dep);
    }
  }
  LivePhysicalNodes.clear();

  for (auto N : LiveSends) {
    if (N->pred_empty()) {
      addEdge(curNode, N, Dep);
    }
  }
  LiveSends.clear();

  // Add an edge when there is no edge on previous barrier.
  if (prevBarrier != nullptr && prevBarrier->pred_empty())
    addEdge(curNode, prevBarrier, Dep);
  prevBarrier = curNode;

  G4_INST *Inst = curNode->getInst();
  if (Inst == nullptr)
    return;

  for (auto OpNum :
       {Gen4_Operand_Number::Opnd_dst, Gen4_Operand_Number::Opnd_src0,
        Gen4_Operand_Number::Opnd_src1, Gen4_Operand_Number::Opnd_src2,
        Gen4_Operand_Number::Opnd_src3, Gen4_Operand_Number::Opnd_src4,
        Gen4_Operand_Number::Opnd_pred, Gen4_Operand_Number::Opnd_condMod,
        Gen4_Operand_Number::Opnd_implAccSrc,
        Gen4_Operand_Number::Opnd_implAccDst}) {
    G4_Operand *opnd = Inst->getOperand(OpNum);
    if (opnd == nullptr || opnd->getBase() == nullptr || opnd->isNullReg())
      continue;
    NewLiveOps.emplace_back(curNode, OpNum);
  }
}

// - Remove one element from vector if pred is true.
// - Return the iterator to the next element.
template <typename T, typename AllocTy>
static typename std::vector<T, AllocTy>::iterator
kill_if(bool pred, std::vector<T, AllocTy> &Elts,
        typename std::vector<T, AllocTy>::iterator Iter) {
  if (!pred)
    return std::next(Iter);

  vASSERT(Iter != Elts.end());
  vASSERT(!Elts.empty());
  // This is the last element so the next element is none.
  if (&*Iter == &Elts.back()) {
    Elts.pop_back();
    return Elts.end();
  }

  // This is not the last element, swap with the tail.
  // Keep the iterator unchanged.
  std::swap(*Iter, Elts.back());
  Elts.pop_back();
  return Iter;
}

// Compute {RAW,WAW,WAR,NODEP} for given operand to a live node.
static DepType getDep(G4_Operand *Opnd, const preDDD::LiveNode &LN) {
  DepType Deps[] = {DepType::NODEP, DepType::RAW, DepType::WAR, DepType::WAW};
  int i = int(LN.isWrite());
  int j = int(Opnd->isDstRegRegion() || Opnd->isCondMod());
  return Deps[i * 2 + j];
}

// Compute relation for given operand to a live node. This function may return
// a different dependency when checking acc dependency.
static std::pair<DepType, G4_CmpRelation>
getDepAndRel(G4_Operand *Opnd, const preDDD::LiveNode &LN, DepType Dep) {
  G4_CmpRelation Rel = G4_CmpRelation::Rel_undef;
  G4_Operand *Other = LN.N->getInst()->getOperand(LN.OpNum);
  vASSERT(Other != nullptr);

  if (Other) {
    const IR_Builder &builder = LN.N->getInst()->getBuilder();
    if (Opnd->isDstRegRegion())
      Rel = Opnd->asDstRegRegion()->compareOperand(Other, builder);
    else if (Opnd->isCondMod())
      Rel = Opnd->asCondMod()->compareOperand(Other, builder);
    else if (Opnd->isSrcRegRegion())
      Rel = Opnd->asSrcRegRegion()->compareOperand(Other, builder);
    else if (Opnd->isPredicate())
      Rel = Opnd->asPredicate()->compareOperand(Other, builder);
    else
      Rel = Opnd->compareOperand(Other, builder);

    if (Rel == G4_CmpRelation::Rel_disjoint) {
      // Check if there is any acc dependency on acc registers.
      G4_AccRegSel AccOpnd = Opnd->getAccRegSel();
      G4_AccRegSel AccOther = Other->getAccRegSel();

      // Normalize NOACC to ACC_UNDEFINED
      if (AccOpnd == G4_AccRegSel::NOACC)
        AccOpnd = G4_AccRegSel::ACC_UNDEFINED;
      if (AccOther == G4_AccRegSel::NOACC)
        AccOther = G4_AccRegSel::ACC_UNDEFINED;

      if (AccOther == AccOpnd && AccOther != G4_AccRegSel::ACC_UNDEFINED) {
        // While comparing V3:Acc2 to V4:Acc2, we cannot kill this live
        // node, as there is no overlap on V3 and V4. So only returns
        // Rel_interfere relation, not Rel_eq.
        //
        if (LN.isWrite() && Opnd->isDstRegRegion())
          return std::make_pair(DepType::WAW, Rel_interfere);
        if (LN.isWrite())
          return std::make_pair(DepType::WAR, Rel_interfere);
        if (Opnd->isDstRegRegion())
          return std::make_pair(DepType::RAW, Rel_interfere);
      }

      // No dependency.
      return std::make_pair(DepType::NODEP, Rel);
    }
  }
  return std::make_pair(Dep, Rel);
}

//Add the dependence edge for source operand
//Also, add the node to the active live list
void preDDD::addSrcOpndDep(preNode *curNode, G4_Declare *Dcl,
                           Gen4_Operand_Number OpNum) {
  G4_Operand *opnd = curNode->getInst()->getOperand(OpNum);

  if (Dcl) {
    auto &Nodes = LiveNodes[Dcl];
    // Iterate all live nodes associated to the same declaration.
    for (auto &liveNode : Nodes) {
      // Skip read live nodes.
      if (liveNode.isRead())
        continue;

      DepType Dep = getDep(opnd, liveNode);
      if (Dep == DepType::NODEP)
        continue;
      std::pair<DepType, G4_CmpRelation> DepRel =
          getDepAndRel(opnd, liveNode, Dep);
      if (DepRel.first != DepType::NODEP)
        addEdge(curNode, liveNode.N, DepRel.first);
    }
  }

  // If this is a physically allocated regvar, then check dependency on the
  // physically allocated live nodes. This should be a cold path.
  if (opnd->isPhysicallyAllocatedRegVar()) {
    for (auto &liveNode : LivePhysicalNodes) {
      // Skip read live nodes.
      if (liveNode.isRead())
        continue;
      DepType Dep = getDep(opnd, liveNode);
      if (Dep == DepType::NODEP)
        continue;
      std::pair<DepType, G4_CmpRelation> DepRel =
          getDepAndRel(opnd, liveNode, Dep);
      if (DepRel.first != DepType::NODEP)
        addEdge(curNode, liveNode.N, DepRel.first);
    }
  }

  NewLiveOps.emplace_back(curNode, OpNum);
}

// This is not a label nor a barrier and check the dependency
// introduced by this node.
void preDDD::processReadWrite(preNode *curNode) {
  G4_INST *Inst = curNode->getInst();
  for (auto OpNum :
       {Gen4_Operand_Number::Opnd_dst, Gen4_Operand_Number::Opnd_condMod,
        Gen4_Operand_Number::Opnd_implAccDst}) {
    G4_Operand *opnd = Inst->getOperand(OpNum);
    if (opnd == nullptr || opnd->getBase() == nullptr || opnd->isNullReg())
      continue;
    vASSERT(opnd->getTopDcl() || opnd->isPhysicallyAllocatedRegVar());
    if (G4_Declare *Dcl = opnd->getTopDcl()) {
      auto &Nodes = LiveNodes[Dcl];
      // Iterate all live nodes associated to the same declaration.
      for (auto Iter = Nodes.begin(); Iter != Nodes.end(); /*empty*/) {
        LiveNode &liveNode = *Iter;
        DepType Dep = getDep(opnd, liveNode);
        if (Dep == DepType::NODEP) {
          ++Iter;
        } else {
          auto DepRel = getDepAndRel(opnd, liveNode, Dep);
          if (DepRel.first != DepType::NODEP) {
            addEdge(curNode, liveNode.N, Dep);
            // Check if this kills current live node. If yes, remove it.
            bool pred = DepRel.second == G4_CmpRelation::Rel_eq ||
                        DepRel.second == G4_CmpRelation::Rel_gt;
            Iter = kill_if(pred, Nodes, Iter);
          } else
            ++Iter;
        }
      }
    }

    // If this is a physically allocated regvar, then check dependency on the
    // physically allocated live nodes. This should be a cold path.
    if (opnd->isPhysicallyAllocatedRegVar()) {
      for (auto Iter = LivePhysicalNodes.begin();
           Iter != LivePhysicalNodes.end();
           /*empty*/) {
        LiveNode &liveNode = *Iter;
        DepType Dep = getDep(opnd, liveNode);
        if (Dep == DepType::NODEP) {
          ++Iter;
        } else {
          auto DepRel = getDepAndRel(opnd, liveNode, Dep);
          if (DepRel.first != DepType::NODEP) {
            addEdge(curNode, liveNode.N, Dep);
            // Check if this kills current live node. If yes, remove it.
            bool pred = DepRel.second == G4_CmpRelation::Rel_eq ||
                        DepRel.second == G4_CmpRelation::Rel_gt;
            Iter = kill_if(pred, LivePhysicalNodes, Iter);
          } else
            ++Iter;
        }
      }
    }

    NewLiveOps.emplace_back(curNode, OpNum);
  }

  if (Inst->isPseudoAddrMovIntrinsic()) {
    for (auto OpNum :
         {Gen4_Operand_Number::Opnd_src0, Gen4_Operand_Number::Opnd_src1,
          Gen4_Operand_Number::Opnd_src2, Gen4_Operand_Number::Opnd_src3,
          Gen4_Operand_Number::Opnd_src4, Gen4_Operand_Number::Opnd_src5,
          Gen4_Operand_Number::Opnd_src6, Gen4_Operand_Number::Opnd_src7}) {
      G4_Operand *opnd = curNode->getInst()->getOperand(OpNum);
      if (opnd == nullptr || opnd->isNullReg())
        continue;
      G4_Declare *Dcl = opnd->asAddrExp()->getRegVar()->getDeclare();
      addSrcOpndDep(curNode, Dcl, OpNum);
    }
  } else {
    for (auto OpNum :
         {Gen4_Operand_Number::Opnd_src0, Gen4_Operand_Number::Opnd_src1,
          Gen4_Operand_Number::Opnd_src2, Gen4_Operand_Number::Opnd_src3,
          Gen4_Operand_Number::Opnd_src4, Gen4_Operand_Number::Opnd_pred,
          Gen4_Operand_Number::Opnd_implAccSrc}) {
      G4_Operand *opnd = curNode->getInst()->getOperand(OpNum);
      if (opnd == nullptr || opnd->getBase() == nullptr || opnd->isNullReg())
        continue;
      G4_Declare *Dcl = opnd->getTopDcl();
      vASSERT(Dcl || opnd->isPhysicallyAllocatedRegVar());
      addSrcOpndDep(curNode, Dcl, OpNum);
    }
  }
}

void preDDD::processSend(preNode *curNode) {
  G4_INST *Inst = curNode->getInst();
  if (!Inst->isSend())
    return;

  vISA_ASSERT(!Inst->getMsgDesc()->isScratch(), "not expected");
  for (auto Iter = LiveSends.begin(); Iter != LiveSends.end(); /*empty*/) {
    preNode *liveN = *Iter;
    DepType Dep = getDepSend(Inst, liveN->getInst(), BTIIsRestrict);
    if (Dep != DepType::NODEP) {
      addEdge(curNode, liveN, Dep);
      // Check if this kills current live send. If yes, remove it.
      bool pred = (Dep == DepType::WAW_MEMORY || Dep == DepType::RAW_MEMORY);
      Iter = kill_if(pred, LiveSends, Iter);
    } else
      ++Iter;
  }
}

void preDDD::prune() {
  auto removeEdge = [=](preNode *pred, preNode *succ) {
    auto Iter = std::find_if(pred->succ_begin(), pred->succ_end(),
                             [=](preEdge &E) { return E.getNode() == succ; });
    if (Iter == pred->succ_end())
      return;
    kill_if(true, pred->Succs, Iter);
    Iter = std::find_if(succ->pred_begin(), succ->pred_end(),
                        [=](preEdge &E) { return E.getNode() == pred; });
    vASSERT(Iter != succ->pred_end());
    kill_if(true, succ->Preds, Iter);
  };

  // Currently only prune up to two levels.
  for (auto N : SNodes) {
    std::set<preNode *> Seen;
    for (auto &E1 : N->Succs)
      for (auto &E2 : E1.getNode()->Succs)
        Seen.insert(E2.getNode());

    for (auto T : Seen)
      removeEdge(N, T);
  }
}

// Reset states that a scheduler may overwrite.
void preDDD::reset(bool ReassignNodeID) {
  if (!IsDagBuilt)
    buildGraph();

  // When instructcions are reordered, the node IDs may become
  // inconsistent. This is to ensure the following internal consistency:
  //
  // I0     N2
  // I1 ==> N1
  // I2     N0
  //
  // SNodes = { N0, N1, N2}
  //
  // as IDs may be used in node comparison.
  //
  if (ReassignNodeID) {
    m_BB->resetLocalIds();
    auto Cmp = [](const preNode *LHS, const preNode *RHS) {
      return LHS->Inst->getLocalId() > RHS->Inst->getLocalId();
    };
    std::sort(SNodes.begin(), SNodes.end(), Cmp);
    unsigned Id = 0;
    for (auto N : SNodes) {
      N->ID = Id++;
    }
  }

  auto isHalfN = [](G4_INST *Inst, unsigned N) -> bool {
    return Inst->isSend() && Inst->getExecSize() == g4::SIMD16 &&
           Inst->getMaskOffset() == N * 16;
  };

  auto isQuadN = [](G4_INST *Inst, unsigned N) -> bool {
    return Inst->isSend() && Inst->getExecSize() == g4::SIMD8 &&
           Inst->getMaskOffset() == N * 8;
  };

  auto isTupleLead = [&isHalfN, &isQuadN](G4_INST *Inst) -> bool {
    return isHalfN(Inst, 1) || isQuadN(Inst, 3);
  };

  auto isTuplePart = [&isHalfN, &isQuadN](G4_INST *Inst) -> bool {
    return isHalfN(Inst, 0) || isHalfN(Inst, 1) || isQuadN(Inst, 0) ||
           isQuadN(Inst, 1) || isQuadN(Inst, 2) || isQuadN(Inst, 3);
  };

  // Mark SIMD32 send tuples.
  preNode *Lead = nullptr;
  for (auto N : SNodes) {
    G4_INST *Inst = N->getInst();
    if (!Inst)
      continue;
    if (isTupleLead(Inst)) {
      // In case, send tuples are interleaved, bail out.
      if (Lead)
        break;
      Lead = N;
      N->setTupleLead(Lead);
      continue;
    }
    if (Lead && isTuplePart(Inst)) {
      N->setTupleLead(Lead);
      if (Inst->getMaskOffset() == 0)
        Lead = nullptr;
      continue;
    }
    // This send is neither a lead nor a part, which means this block is
    // already sliced. Bail out.
    if (Lead && Inst->isSend())
      break;
  }

  for (auto N : SNodes) {
    N->NumPredsLeft = unsigned(N->Preds.size());
    N->NumSuccsLeft = unsigned(N->Succs.size());
    N->isScheduled = false;
    N->setReadyCycle(0);
  }

  EntryNode.NumPredsLeft = 0;
  EntryNode.NumSuccsLeft = unsigned(EntryNode.Succs.size());
  EntryNode.isScheduled = false;
  EntryNode.setReadyCycle(0);

  ExitNode.NumPredsLeft = unsigned(ExitNode.Preds.size());
  ExitNode.NumSuccsLeft = 0;
  ExitNode.isScheduled = false;
  ExitNode.setReadyCycle(0);
  // compute height
  // sort its successor in height-descending order
  for (auto N : SNodes) {
    std::sort(N->Succs.begin(), N->Succs.end(), [](preEdge &A, preEdge &B) {
      auto *AN = A.getNode();
      auto *BN = B.getNode();
      return (AN->Height > BN->Height) ||
             (AN->Height == BN->Height && AN->getID() > BN->getID());
    });
    if (N->succs().size())
      N->Height = N->succs().front().getNode()->Height + 1;
  }
}

void preDDD::dumpDagTxt(RegisterPressure &rp) {
  const char *asmFileName = "nullasm";
  getOptions()->getOption(VISA_AsmFileName, asmFileName);
  std::string fileName(asmFileName);
  fileName.append(".bb")
      .append(std::to_string(getBB()->getId()))
      .append(".preDDD.txt");
  std::fstream ofile(fileName, std::ios::out);

  std::vector<unsigned> LiveOutNodeIDs;
  std::set<vISA::G4_Declare *> LiveOutSet;
  // 1) dump node-id, dst-size, instruction, ...
  // nodes are ordered bottom-up from block exit
  for (auto N : SNodes) {
    // Node
    ofile << "NodeInfo, " << N->ID << ", ";
    if (N->getInst()) {
      // dst-size
      unsigned dclSize = 0;
      G4_INST *Inst = N->getInst();
      G4_DstRegRegion *Dst = Inst->getDst();
      if (!Inst->isPseudoKill() && Dst && Dst->getTopDcl()) {
        auto rootDcl = Dst->getTopDcl();
        dclSize = rootDcl->getByteSize();
        auto alignBytes = static_cast<uint32_t>(rootDcl->getSubRegAlign()) * 2;
        if (dclSize < alignBytes) {
          dclSize = std::min(dclSize * 2, alignBytes);
        }
        // first time seeing a a live-out variable, record the node-id
        if (rp.isLiveOut(m_BB, rootDcl) && !LiveOutSet.count(rootDcl)) {
          LiveOutNodeIDs.push_back(N->ID);
          LiveOutSet.insert(rootDcl);
        }
      }
      ofile << dclSize << ", ";
      // inst text
      N->getInst()->emit(ofile);
      ofile << "\n";
    } else
      ofile << "0, "
            << "null\n";
  }
  // 2) dump node-id then predecessor-ids
  for (auto N : SNodes) {
    // Node
    ofile << "PredInfo, " << N->ID;
    // Edge
    for (auto &E : N->Preds) {
      ofile << ", " << E.getNode()->ID;
    }
    ofile << "\n";
  }
  // 3) dump node-id then use-ids
  for (auto N : SNodes) {
    // Node
    ofile << "UseInfo, " << N->ID;
    if (N == &ExitNode) {
      for (auto OutID : LiveOutNodeIDs)
        ofile << ", " << OutID;
      ofile << "\n";
      continue;
    }
    // Edge
    for (auto &E : N->Preds) {
      auto DefInst = E.getNode()->getInst();
      if (DefInst && !DefInst->isPseudoKill() && E.isFlowDep())
        ofile << ", " << E.getNode()->ID;
    }
    ofile << "\n";
  }
  ofile.close();
}

[[maybe_unused]] void preDDD::dumpDagDot() {
  const char *asmFileName = "nullasm";
  getOptions()->getOption(VISA_AsmFileName, asmFileName);
  std::string fileName(asmFileName);
  fileName.append(".bb")
      .append(std::to_string(getBB()->getId()))
      .append(".preDDD.dot");

  std::fstream ofile(fileName, std::ios::out);
  ofile << "digraph DAG {"
        << "\n";

  for (auto N : SNodes) {
    // Node
    ofile << N->ID << "[label=\"";
    N->getInst()->emit(ofile);
    ofile << ", I" << N->getInst()->getLocalId() << "\"]\n";
    // Edge
    for (auto &E : N->Succs) {
      DepType depType = E.getType();
      const char *depColor, *depStr;
      std::tie(depColor, depStr) = (depType == RAW || depType == RAW_MEMORY)
                                       ? std::make_tuple("black", "RAW")
                                   : (depType == WAR || depType == WAR_MEMORY)
                                       ? std::make_tuple("red", "WAR")
                                   : (depType == WAW || depType == WAW_MEMORY)
                                       ? std::make_tuple("orange", "WAW")
                                       : std::make_tuple("grey", "other");

      // Example: 30->34[label="RAW",color="{red|black|yellow}"];
      ofile << N->ID << "->" << E.getNode()->ID << "[label=\"" << depStr << "\""
            << ",color=\"" << depColor << "\""
            << "];\n";
    }
  }

  ofile << "}\n";
  ofile.close();
}

namespace {
// Queue for Sethi-Ullman scheduling to reduce register pressure.
class SethiUllmanACCQueue {
  preDDD &ddd;

  // Sethi-Ullman numbers.
  std::vector<unsigned> Numbers;

  std::vector<preNode *> Q;

public:
  SethiUllmanACCQueue(preDDD &ddd, G4_Kernel *kernel) : ddd(ddd) {
    init(kernel);
  }

  // Add a new ready node.
  void push(preNode *N) { Q.push_back(N); }

  // Schedule the top node.
  preNode *pop() { return select(); }

  bool empty() const { return Q.empty(); }

  unsigned getNumber(unsigned i) { return Numbers[i]; }

private:
  // Compute the Sethi-Ullman number for a node.
  unsigned calculateSethiUllmanNumberForACC(preNode *N, G4_Kernel *kernel);

  // Initialize Sethi-Ullman numbers.
  void init(G4_Kernel *kernel);

  // Select next ready node to schedule.
  preNode *select();

  // Compare two ready nodes and decide which one should be scheduled first.
  // Return true if N2 has a higher priority than N1, false otherwise.
  bool compare(preNode *N1, preNode *N2);
};

// Scheduler instruction to increase the ACC substitution ratio on a single
// block.
class BB_ACC_Scheduler {
  // The kernel this block belongs to.
  G4_Kernel &kernel;

  // The data dependency graph for this block.
  preDDD &ddd;

  // The schedule result.
  std::vector<G4_INST *> schedule;

public:
  BB_ACC_Scheduler(G4_Kernel &kernel, preDDD &ddd) : kernel(kernel), ddd(ddd) {}

  G4_Kernel &getKernel() const { return kernel; }
  G4_BB *getBB() const { return ddd.getBB(); }

  // Run Sethi-Ullman scheduling.
  void scheduleBlockForACC() { SethiUllmanACCScheduling(); }

  // Commit the scheduling result.
  void commit();

private:
  void SethiUllmanACCScheduling();
  bool verifyScheduling();
};

} // namespace

//
// Basic generalized SU scheduling algorithm
//
void BB_ACC_Scheduler::SethiUllmanACCScheduling() {
  schedule.clear();
  SethiUllmanACCQueue Q(ddd, &kernel);
  Q.push(ddd.getExitNode());

  while (!Q.empty()) {
    preNode *N = Q.pop();
    vASSERT(!N->isScheduled && N->NumSuccsLeft == 0);
    if (N->getInst()) {
      VISA_DEBUG_VERBOSE({
        std::cerr << "SU[" << Q.getNumber(N->getID()) << "]:";
        N->dump();
      });
      schedule.push_back(N->getInst());
      N->isScheduled = true;
    }

    for (auto I = N->pred_begin(), E = N->pred_end(); I != E; ++I) {
      preNode *Node = I->getNode();
      vASSERT(!Node->isScheduled && Node->NumSuccsLeft);
      --Node->NumSuccsLeft;
      if (Node->NumSuccsLeft == 0)
        Q.push(Node);
    }
  }

  vASSERT(verifyScheduling());
}

void BB_ACC_Scheduler::commit() {
  INST_LIST &CurInsts = getBB()->getInstList();
  CurInsts.clear();

  // move the scheduled instruction to the instruction list.
  for (auto Inst : schedule) {
    CurInsts.push_front(Inst);
  }

  return;
}

[[maybe_unused]] bool BB_ACC_Scheduler::verifyScheduling() {
  std::set<G4_INST *> Insts;
  for (auto Inst : *(getBB()))
    Insts.insert(Inst);

  if (Insts.size() != schedule.size()) {
    return false;
  }

  for (auto Inst : schedule) {
    if (Insts.count(Inst) != 1) {
      Inst->dump();
      return false;
    }
  }

  return true;
}

preRA_ACC_Scheduler::preRA_ACC_Scheduler(G4_Kernel &k)
    : kernel(k), m_options(kernel.getOptions()) {}

preRA_ACC_Scheduler::~preRA_ACC_Scheduler() {}

bool preRA_ACC_Scheduler::run() {
  AccSubPass accSub(*kernel.fg.builder, kernel);

  for (auto bb : kernel.fg) {
    if (bb->size() < SMALL_BLOCK_SIZE || bb->size() > LARGE_BLOCK_SIZE) {
      // Skip small and large blocks.
      continue;
    }

    preDDD ddd(kernel, bb);
    SchedConfig config(0);
    BB_ACC_Scheduler S(kernel, ddd);

    ddd.buildGraphForACC();
    S.scheduleBlockForACC();
    S.commit();
    accSub.doAccSub(bb);
  }

  return true;
}

#define ACC_DEF_NODE_DEGREE 1
#define ACC_USE_NODE_DEGREE 5
#define NONE_ACC_NODE_DEGREE 20

//
// Generalizations of the Sethi-Ullman algorithm for register allocation
//
unsigned
SethiUllmanACCQueue::calculateSethiUllmanNumberForACC(preNode *N,
                                                      G4_Kernel *kernel) {
  vASSERT(N->getID() < Numbers.size());
  unsigned CurNum = Numbers[N->getID()];
  if (CurNum != 0)
    return CurNum;

  // Get the number of Pred nodes
  unsigned accPredNum = 0;
  std::vector<std::pair<preNode *, unsigned>> Preds;
  for (auto I = N->pred_begin(), E = N->pred_end(); I != E; ++I) {
    auto &Edge = *I;

    // Skip pseudo-kills as they are lifetime markers.
    auto predNode = Edge.getNode();
    auto type = Edge.getType();
    auto DefInst = predNode->getInst();
    if (!DefInst)
      continue;

    if (predNode->isACCCandidate() && type == DepType::RAW) {
      accPredNum++;
    }
    // Recurse on the predecessors.
    unsigned Num = calculateSethiUllmanNumberForACC(Edge.getNode(), kernel);
    Preds.emplace_back(Edge.getNode(), Num);
  }

  // If current node is not ACC candidate, but it's predecessor is, reduce the
  // degree to be scheduled first.
  CurNum = N->isACCCandidate() ? ACC_DEF_NODE_DEGREE
           : accPredNum        ? ACC_USE_NODE_DEGREE
                               : NONE_ACC_NODE_DEGREE;

  if (Preds.size() > 0) {
    std::sort(Preds.begin(), Preds.end(),
              [](std::pair<preNode *, unsigned> lhs,
                 std::pair<preNode *, unsigned> rhs) {
                return lhs.second < rhs.second;
              });
    // Add the minimal degree of the pred nodes
    CurNum = CurNum + Preds[0].second;
  }

  return CurNum;
}

void SethiUllmanACCQueue::init(G4_Kernel *kernel) {
  auto &Nodes = ddd.getNodes();
  unsigned N = (unsigned)Nodes.size();
  Numbers.resize(N, 0);
  for (unsigned i = 0; i < N; ++i) {
    unsigned j = N - 1 - i;
    Numbers[j] = calculateSethiUllmanNumberForACC(Nodes[j], kernel);
  }

  VISA_DEBUG_VERBOSE({
    std::cerr << "\n\n";
    for (auto I = Nodes.rbegin(); I != Nodes.rend(); ++I) {
      std::cerr << "SU[" << Numbers[(*I)->getID()] << "] "
                << ((*I)->isACCCandidate() ? "ACC " : "GRF ");
      (*I)->dump();
    }
    std::cerr << "\n\n";
  });
}

// Compare two ready nodes and decide which one should be scheduled first.
// Return true if N2 has a higher priority than N1, false otherwise.
bool SethiUllmanACCQueue::compare(preNode *N1, preNode *N2) {
  // TODO. Introduce heuristics before comparing SU numbers.
  vASSERT(N1->getID() < Numbers.size());
  vASSERT(N2->getID() < Numbers.size());
  vASSERT(N1->getID() != N2->getID());

  // Pseudo kill always has higher priority.
  if (N1->getInst()->isPseudoKill())
    return false;

  unsigned SU1 = Numbers[N1->getID()];
  unsigned SU2 = Numbers[N2->getID()];

  // This is a bottom-up scheduling. Smaller SU number means higher priority.
  if (SU1 < SU2)
    return false;

  if (SU1 > SU2)
    return true;

  // Otherwise, break tie with their IDs. Smaller ID means higher priority.
  return N1->getID() > N2->getID();
}

preNode *SethiUllmanACCQueue::select() {
  vASSERT(!Q.empty());
  auto TopIter = Q.end();
  for (auto I = Q.begin(), E = Q.end(); I != E; ++I) {
    if (TopIter == Q.end() || compare(*TopIter, *I))
      TopIter = I;
  }

  vASSERT(TopIter != Q.end());
  preNode *Top = *TopIter;
  std::swap(*TopIter, Q.back());
  Q.pop_back();

  return Top;
}

// Build the data dependency bottom up with two simple
// special nodes.
void preDDD::buildGraphForACC() {
  vASSERT(!IsDagBuilt);

  // Starts with the exit node.
  addNodeToGraph(&ExitNode);

  unsigned NumOfInsts = (unsigned)m_BB->size();
  SNodes.reserve(NumOfInsts);

  auto I = m_BB->rbegin(), E = m_BB->rend();
  for (unsigned i = 0; I != E; ++I) {
    preNode *N = new (preNodeAllocator) preNode(*I, i++);
    SNodes.push_back(N);
    if ((*I)->isSend()) {
      N->setBarrier(DepType::SEND_BARRIER);
    }
    addNodeToGraph(N);
    if ((*I)->canInstBeAcc(&kernel.fg.globalOpndHT)) {
      N->setACCCandidate();
    }
  }

  // Ends with the entry node.
  addNodeToGraph(&EntryNode);

  // prune the graph.
  prune();

  // Set DAG is complete.
  IsDagBuilt = true;

  // Initialize perNode data.
  reset();
}
