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

class FrequencyInfo {
public:
  FrequencyInfo(IR_Builder *builder, G4_Kernel &k);
  void setCurrentStaticFreq(uint64_t digits, int16_t scale);
  void transferFreqToG4Inst();
  void storeStaticFrequencyAsMetadata(G4_INST *i);
  void updateStaticFrequencyForBasicBlock(G4_BB *bb);
  void updateStaticFrequency(
      std::unordered_map<G4_Label *, std::vector<G4_BB *>> &subroutines);
  bool underSpillFreqThreshold(const std::list<vISA::LiveRange *> &spilledLRs,
                               int instNum);
  void computeFreqSpillCosts(GlobalRA &gra, LivenessAnalysis &liveAnalysis,
                             std::vector<LiveRange *> &lrs,
                             unsigned int numVar);
  void sortBasedOnFreq(std::vector<LiveRange *> &lrs);
  bool hasFreqMetaData(G4_INST *i);
  ~FrequencyInfo() {}
  void dump() const {};

private:
  G4_Kernel &kernel;
  llvm::ScaledNumber<uint64_t> curFreq;
  IR_Builder *irb;
  G4_INST *lastInstMarked;
  std::unordered_map<G4_BB *, llvm::ScaledNumber<uint64_t>> BlockFreqInfo;
  std::unordered_map<LiveRange *, llvm::ScaledNumber<uint64_t>> freqSpillCost;
  bool dumpEnabled;
};
} // namespace vISA
#endif
