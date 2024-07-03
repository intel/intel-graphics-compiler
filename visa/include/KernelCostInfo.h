/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __KERNELCOSTINFO_H__
#define __KERNELCOSTINFO_H__

#include <vector>

// Kernel cost info Provides cost metrics for Graph Compilers (GC) to choose
// a better pre-compiled kernel by fusing or splitting tensor operations of
// ML workloads. Unlike pure static cost metrics, the kernel cost metrics is
// represented as functions of loop counts, which in turn can be symbolic
// expressions based on kernel input arguments.
//
// The cost can be described in the following fomula:
//   kernelCost = costExpr
//   costExpr = C + LCE[0] * LB[0] + LCE[1] * LB[1] + ... + LCE[n]*LB[n]
//   LB[i] = costExpr
// where LCE[i] and LB[i] are the loop count expression and loop body cost for loop i,
// respectively. Each loop's cost is in turns represented as costExpr.
//
// Those expressions are written into zeinfo and GC will evaluate them with
// the actual value to replace LCE[i], thus calculate the cost.
// For details, see docs <place>
//

namespace vISA {

struct ArgSym {
  int  argNo;
  int  byteOffset;
  int  sizeInBytes;
  bool isIndirect;
};

struct LoopCountExpr {
  float factor;
  ArgSym*  sym;
  float C;
};

struct CostMetrics {
  uint32_t cycles;
  uint32_t loadBytes;
  uint32_t storeBytes;
};


struct LoopCostInfo;

struct CostExpr {
  // Non-loop parts, constant
  CostMetrics C;
  // All immediate loops in program order
  std::vector<LoopCostInfo *> loopCosts;
};

struct LoopCostInfo {
  // Start from 0, in the increasing program order
  int loopId;
  // May be needed for matching loops b/w visa and igc
  int backedge_visaId;
  // The number of immediate child loops
  int numChildLoops;
  // nesting level, top loop is 1
  int nestingLevel;

  CostExpr loopBodyCost;

  struct LoopCountExpr *LCE;
};

struct KernelCostInfo {
  CostExpr kernelCost;
  std::vector<LoopCostInfo> allLoopCosts;
};

} // namespace vISA
#endif
