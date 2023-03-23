/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __RPE_H__
#define __RPE_H__

#include "RegAlloc.h"
#include <unordered_map>

namespace vISA {
class RPE {
public:
  RPE(const GlobalRA &, const LivenessAnalysis *,
      DECLARE_LIST *spills = nullptr);

  ~RPE() {}

  void run();
  void runBB(G4_BB *);
  unsigned int getRegisterPressure(G4_INST *inst) {
    auto it = rp.find(inst);
    if (it == rp.end())
      return 0;
    return it->second;
  }

  unsigned int getMaxRP() const { return maxRP; }

  const LivenessAnalysis *getLiveness() const { return liveAnalysis; }

  void recomputeMaxRP();

  void dump() const;

private:
  const GlobalRA &gra;
  const FlowGraph &fg;
  const LivenessAnalysis *const liveAnalysis;
  std::unordered_map<G4_INST *, unsigned int> rp;
  double regPressure = 0;
  uint32_t maxRP = 0;
  const Options *options;
  llvm_SBitVector live;
  const std::vector<G4_RegVar *> &vars;
  // Variables part of spilledVars set dont contribute to
  // program register pressure. This is useful to model
  // register pressure immediately after coloring (spill
  // iteration).
  std::unordered_set<const G4_Declare *> spilledVars;

  void regPressureBBExit(G4_BB *);
  void updateRegisterPressure(bool change, bool clean, unsigned int);
  void updateLiveness(llvm_SBitVector &, uint32_t, bool);

  bool isSpilled(const G4_Declare *dcl) const {
    auto it = spilledVars.find(dcl);
    if (it == spilledVars.end())
      return false;
    return true;
  }

  bool isStackPseudoVar(G4_Declare *dcl) const {
    bool stackCall = fg.getIsStackCallFunc() || fg.getHasStackCalls();
    if (stackCall) {
      if (fg.isPseudoDcl(dcl))
        return true;

      auto phyReg = dcl->getRegVar()->getPhyReg();
      if (phyReg && phyReg->isGreg()) {
        auto regNum = phyReg->asGreg()->getRegNum();
        // Pre-assigned variables like FP, SP from
        // reserved GRFs don't contribute to reg pressure.
        if (regNum >= fg.builder->kernel.getStackCallStartReg())
          return true;
      }
    }
    return false;
  }
};
} // namespace vISA
#endif
