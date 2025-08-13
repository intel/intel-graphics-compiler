/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __FREQUENCYINFO_H__
#define __FREQUENCYINFO_H__
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/ScaledNumber.h"
#include "common/LLVMWarningsPop.hpp"
#include <cstdint>
#include <list>
#include <vector>
#include <unordered_map>

namespace vISA {
class IR_Builder;
class LiveRange;
class LivenessAnalysis;
class GlobalRA;
class G4_Kernel;
class G4_BB;
class G4_INST;
class G4_Label;
class RPE;

class FrequencyInfo {

  typedef enum {
    PGSS_VISA_EMBED = 0x1C,
    PGSS_VISA_COMP = 0x18,
    PGSS_VISA_ENABLE = 0x10,
  } PGSS_STEP_t;

  typedef enum {
    PGSS_VISA_DUMP_TRANS = 0x10,
    PGSS_VISA_DUMP_ANAL = 0x20,
    PGSS_VISA_DUMP_COLOR = 0x40,
    PGSS_VISA_DUMP_THR = 0x80,
    PGSS_VISA_DUMP_NOFREQ = 0x100,
  } PGSS_DUMP_t;

public:
  FrequencyInfo(IR_Builder *builder, G4_Kernel &k);
  void transferFreqToG4Inst(uint64_t digits, int16_t scale);
  void storeStaticFrequencyAsMetadata(G4_INST *i,
                                      llvm::ScaledNumber<uint64_t> curFreq);
  void updateStaticFrequencyForBasicBlock(G4_BB *bb);
  void updateStaticFrequency(
      std::unordered_map<G4_Label *, std::vector<G4_BB *>> &subroutines);
  bool underFreqSpillThreshold(const std::list<vISA::LiveRange *> &spilledLRs,
                               int instNum, unsigned int legacySpillFillCount,
                               bool legacyUnderThreshold);
  void computeFreqSpillCosts(GlobalRA &gra, bool useSplitLLRHeuristic,
                             const RPE *rpe);
  void sortBasedOnFreq(std::vector<LiveRange *> &lrs);
  bool hasFreqMetaData(G4_INST *i);
  void deriveRefFreq(G4_BB *bb);
  void dump() const {}
  void initForRegAlloc(LivenessAnalysis *l);
  void initGRFSpillFillFreq() {
    GRFSpillFillFreq = llvm::ScaledNumber<uint64_t>::getZero();
  };

  bool isProfileEmbeddingEnabled() {
    return (enabledSteps & PGSS_VISA_EMBED) != 0;
  }

  bool isSpillCostComputationEnabled() {
    return (enabledSteps & PGSS_VISA_COMP) != 0;
  }

  bool isFreqBasedSpillSelectionEnabled() {
    return (enabledSteps & PGSS_VISA_ENABLE) != 0;
  }

  bool willDumpLLVMToG4() { return (dumpEnabled & PGSS_VISA_DUMP_TRANS) != 0; }

  bool willDumpSpillCostAnalysis() {
    return (dumpEnabled & PGSS_VISA_DUMP_ANAL) != 0;
  }

  bool willDumpColoringOrder() {
    return (dumpEnabled & PGSS_VISA_DUMP_COLOR) != 0;
  }

  bool willDumpOnSpilThreshold() {
    return (dumpEnabled & PGSS_VISA_DUMP_THR) != 0;
  }

  bool willDumpNoFreqReport() {
    return (dumpEnabled & PGSS_VISA_DUMP_NOFREQ) != 0;
  }

private:
  unsigned dumpEnabled;
  unsigned enabledSteps;
  G4_Kernel &kernel;
  IR_Builder *irb;
  G4_INST *lastInstMarked;
  std::unordered_map<G4_BB *, llvm::ScaledNumber<uint64_t>> BlockFreqInfo;
  std::unordered_map<G4_INST *, G4_INST *> tailInsts;
  std::unordered_map<G4_INST *, llvm::ScaledNumber<uint64_t>> InstFreqInfo;

  llvm::ScaledNumber<uint64_t> freqScale;

  llvm::ScaledNumber<uint64_t> GRFSpillFillFreq;

  LivenessAnalysis *liveAnalysis;
  std::unordered_map<LiveRange *, float> freqSpillCosts;
  std::unordered_map<LiveRange *, llvm::ScaledNumber<uint64_t>> refFreqs;
  std::unordered_map<LiveRange *, unsigned> staticRefCnts;


  llvm::ScaledNumber<uint64_t> getBlockFreqInfo(G4_BB *bb);

  void setBlockFreqInfo(G4_BB *bb, llvm::ScaledNumber<uint64_t> freq) {
    BlockFreqInfo[bb] = freq;
    return;
  };

  float getFreqSpillCost(LiveRange *lr) {
    return freqSpillCosts[lr];
  };
  void setFreqSpillCost(LiveRange *lr, float spillCost) {
    freqSpillCosts[lr] = spillCost;
    return;
  };

  llvm::ScaledNumber<uint64_t> getRefFreq(LiveRange *lr) {
    return refFreqs[lr];
  };

  void setRefFreq(LiveRange *lr, llvm::ScaledNumber<uint64_t> refFreq) {
    refFreqs[lr] = refFreq;
    return;
  };
  void addupRefFreq(LiveRange *lr, llvm::ScaledNumber<uint64_t> refFreq) {
    if (refFreqs.find(lr) == refFreqs.end())
      refFreqs[lr] = llvm::ScaledNumber<uint64_t>::getZero();
    refFreqs[lr] += refFreq;

    if (staticRefCnts.find(lr) == staticRefCnts.end())
      staticRefCnts[lr] = 0;
    staticRefCnts[lr] += 1;
    return;
  };

  unsigned getStaticRefCnt(LiveRange* lr) { return staticRefCnts[lr];}
  llvm::ScaledNumber<uint64_t> getFreqInfoFromInst(G4_INST *inst);
};
} // namespace vISA
#endif
