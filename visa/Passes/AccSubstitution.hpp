/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _ACCSUBSTITUION_H
#define _ACCSUBSTITUION_H

#include "../BuildIR.h"
#include "../FlowGraph.h"

// Replace local GRF variables with accumulator. This offers a number of
// benefits:
// -- avoid bank conflict
// -- reduce back to back dependency
// -- lower power consumption
// Note that this does not lower GRF pressure since we run this pass post-RA;
// this is because we only have a few accumulators available, and doing this
// before RA may introduce anti-dependencies and make scheduling less effective.

struct AccInterval;

namespace vISA {

class AccSubPass {

  IR_Builder &builder;
  G4_Kernel &kernel;

  bool replaceDstWithAcc(G4_INST *inst, int accNum);


public:
  AccSubPass(IR_Builder &B, G4_Kernel &K) : builder(B), kernel(K) {}

  AccSubPass(const AccSubPass &) = delete;

  virtual ~AccSubPass() = default;

  void run() {
    for (auto bb : kernel.fg) {
      accSub(bb);
    }
  }
  void accSub(G4_BB *bb);
  void multiAccSub(G4_BB *bb);
  void doAccSub(G4_BB *bb);

  void getInstACCAttributes(G4_INST *inst, int &lastUse, bool &isAllFloat);

  bool isAccCandidate(G4_INST *inst, int &lastUse, bool &mustBeAcc0,
                      bool &isAllFloat, int &readSuppressionSrcs, int &bundleBC,
                      int &bankBC, std::map<G4_INST *, unsigned int> *BCInfo,
                      std::vector<USE_DEF_NODE> *SwappableUses);

};

} // namespace vISA

#endif // _ACCSUBSTITUION_H
