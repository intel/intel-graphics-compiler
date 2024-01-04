/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _STATICPROFILING_H
#define _STATICPROFILING_H

#include "../BuildIR.h"
#include "../G4_IR.hpp"
#include "../FlowGraph.h"
#include "../LocalScheduler/LatencyTable.h"

namespace vISA {
class StaticProfiling {
  IR_Builder &builder;
  G4_Kernel &kernel;

public:
  StaticProfiling(IR_Builder &B, G4_Kernel &K)
      : builder(B), kernel(K) {}

  StaticProfiling(const StaticProfiling &) = delete;
  StaticProfiling& operator=(const StaticProfiling&) = delete;
  virtual ~StaticProfiling() = default;

  void ALUInstructionProfile(G4_INST *inst);

  void run() {
    for (auto bb : kernel.fg) {
      for (auto inst : *bb) {
        ALUInstructionProfile(inst);
      }
    }
  }
};

typedef std::pair<G4_INST*, unsigned int> InstCycle;
typedef std::vector<InstCycle> DistPipeInsts;

class StaticCycleProfiling {
  G4_Kernel &kernel;
  std::vector<InstCycle> tokenInsts;
  std::vector<DistPipeInsts> distInsts;
  LatencyTable *LT = nullptr;

  unsigned BBStaticCycleProfiling(G4_BB *bb);

public:
  StaticCycleProfiling(G4_Kernel &K)
      : kernel(K) {

    distInsts.resize(PIPE_DPAS);
  }
  StaticCycleProfiling(const StaticProfiling &) = delete;
  virtual ~StaticCycleProfiling() = default;

  void run() {
    auto ltable = LatencyTable::createLatencyTable(*kernel.fg.builder);
    LT = &(*ltable);
    for (auto bb : kernel.fg) {
      // Initialization
      tokenInsts.clear();
      tokenInsts.resize(kernel.getNumSWSBTokens());
      for (int i = 0; i < PIPE_DPAS; i++) {
        distInsts[i].clear();
      }
      BBStaticCycleProfiling(bb);
    }
  }
};

} // namespace vISA

#endif // _STATICPROFILING_H
