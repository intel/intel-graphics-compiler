/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __KERNELCOSTANALYSIS_HPP__
#define __KERNELCOSTANALYSIS_HPP__

#include "G4_BB.hpp"
#include "KernelCostInfo.h"
#include "JitterDataStruct.h"
#include "LoopAnalysis.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

namespace vISA {

class FlowGraph;
class G4_InstSend;

// Use fixed-point as probality type (better than float).
typedef uint32_t ProbType;

// Probability 1 is represented as 0x40000000 (bit 30 is 1).
//   Not using UINT_MAX as probability 1 is to make saturation of adding two
//   prob values easier (need satuation on the sum of prob values as prob
//   values are estimate and their sum could be more than 1).
// For example, prob value=10000 means probability is 10000/0x40000000.
constexpr uint32_t MAX_PROB_POINTS = (1u << 30);

struct BBCostInfo {
  uint32_t m_cycles; // total cycles taken by BB (taken from BB scheduler)
  ProbType m_prob;   // prob with which BB runs.
};

// Report of costs for code segments, such as loop/loop body/func/BB, etc.
// Cost value could be in symbolic form (based on kernel args).
class CostMetricsWrapper {
public:
  CostMetricsWrapper() : CM{0, 0, 0} {}

  uint32_t getCycles() const { return CM.cycles; }
  void setCycles(uint32_t v) { CM.cycles = v; }
  uint32_t getLoadBytes() const { return CM.loadBytes; }
  void setLoadBytes(uint32_t v) { CM.loadBytes = v; }
  uint32_t getStoreBytes() const { return CM.storeBytes; }
  void setStoreBytes(uint32_t v) { CM.storeBytes = v; }

  void add(CostMetricsWrapper& aCM, ProbType P = MAX_PROB_POINTS) {
    if (P != MAX_PROB_POINTS) {
      float factor = P / (float)MAX_PROB_POINTS;
      CM.cycles += (uint32_t)(aCM.getCycles() * factor);
      CM.loadBytes += (uint32_t)(aCM.getLoadBytes() * factor);
      CM.storeBytes += (uint32_t)(aCM.getStoreBytes() * factor);
    } else {
      CM.cycles += aCM.getCycles();
      CM.loadBytes += aCM.getLoadBytes();
      CM.storeBytes += aCM.getStoreBytes();
    }
  }

  void mul(uint32_t M) {
    CM.cycles *= M;
    CM.loadBytes *= M;
    CM.storeBytes *= M;
  }

  const CostMetrics &getCostMetrics() const { return CM; }

private:
  CostMetrics CM;
};

struct LoopCost;

// CostExprWrapper
//   cost for a kernel or a loop. It includes all its immediate
//   child loops (non-immediate nested loops are included in the
//   cost of its immediate child loops).
struct CostExprInternal {
  CostMetricsWrapper C;
  // One entry for each of all immediate child loops.
  std::vector<LoopCost *> LoopCosts;
};

// LoopCost: cost for a single loop
struct LoopCost {
  // loop id within a function (kernel, subroutine), starts from 0 for
  // each function and is in the increasing program order
  int m_loopId;
  // For matching loops b/w visa and igc
  int m_backedge_visaId;

  CostExprInternal m_loopBodyCost;
  // estimate cost (assuming LCE = 16)
  CostMetricsWrapper m_estimateCost;
};

struct FuncCost {
  CostExprInternal m_funcCost;
  CostMetricsWrapper m_estimateCost;

  // BB range for this func:  [m_startBB, m_endBB)
  FuncInfo *m_funcInfo;
  // All loops in this function, in program order.
  // m_loops[0] is the 1st loop and m_loops.back() is the last loop.
  std::vector<const Loop *> m_allLoopsInProgramOrder;
};

class KernelCost {
public:
  KernelCost(G4_Kernel *pK, std::vector<VISA_BB_INFO> &BBInfo);

  void run();

  FuncCost& getKernelCost() {
    if (m_metrics.empty()) {
      // sanity: create an empty FuncCost.
      m_metrics.push_back(FuncCost());
    }
    return m_metrics.back();
  }
  LoopCost& getLoopCost(const Loop *L) { return m_loopCosts[L]; }

private:
  G4_Kernel* m_kernel;
  LoopDetection& m_loops;
  std::unordered_map<G4_BB *, BBCostInfo> m_BBCostInfo;

  // Temporaries
  std::unordered_map<G4_BB *, int> visited;
  // Temporary : Reverse Post-Order traveral
  std::vector<G4_BB *> RPOT;

  // m_metrics.
  //   In reverse calling order. leaf subroutine appears first, kernel last.
  //   m_funcIndex[] is for mapping call site to its FuncCost.
  std::unordered_map<FuncInfo *, int> m_funcIndex;
  std::vector<FuncCost> m_metrics;

  // Metrics for all loops
  std::unordered_map<const Loop *, LoopCost> m_loopCosts;

  void updateBBProb(G4_BB *BB, ProbType P) {
    vISA_ASSERT(m_BBCostInfo.count(BB), "updateBBPro(): prob not set yet");
    BBCostInfo &BCI = m_BBCostInfo[BB];
    BCI.m_prob = std::min(MAX_PROB_POINTS, BCI.m_prob +  P);
  };

  ProbType getBBProb(G4_BB* BB) {
    vISA_ASSERT(m_BBCostInfo.count(BB), "getBBProb(): prob not set yet");
    BBCostInfo &BCI = m_BBCostInfo[BB];
    return BCI.m_prob;
  }

  void DFS_PO(G4_BB *BB);
  void doRPOT(G4_BB *EntryBB);

  // set up before calculating prob and collect metrics
  void init();

  void calculateProb();
  void propagateLoopProb(Loop *L, int RPOT_pos);
  void propagateBBProb(G4_BB* BB, Loop* L = nullptr);

  void getSuccEdgeProb(G4_BB *BB, std::vector<ProbType>& SuccEdgeProb);
  G4_INST* getBranchFlagLocalDef(G4_BB *BB, bool& DefByDst);

  void collectPerfMetrics();
  void calculateBBMetrics(CostMetricsWrapper &CM, G4_BB* BB);
  void collectLoopMetrics(Loop* L);
  void collectSendMetrics(G4_InstSend *SendI, uint32_t &ldBytes, uint32_t &stBytes);

  // helpers
  void print(std::ostream &OS);
  void printForLit(std::ostream &OS);
  void dump();
  void dump() const;
};

void collectKernelCostInfo(G4_Kernel* pK, std::vector<VISA_BB_INFO> &BBInfo);

} // namespace vISA
#endif
