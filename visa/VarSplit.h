/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _VARSPLIT_H_
#define _VARSPLIT_H_

#include "BuildIR.h"
#include "FlowGraph.h"
#include "RPE.h"

namespace vISA {
class LiveRange;
class GraphColor;
class RPE;
class GlobalRA;

// store mapping of a split variable to original variable. if any split
// variable is spilled, we can reuse spill location of original variable.
// also store all instructions emitted in preheader, loop exit for each
// split variable. this helps eliminate those instruction in case the
// split variable itself spills.
class SplitResults {
public:
  G4_Declare *origDcl = nullptr;
  std::unordered_map<G4_BB *, std::unordered_set<G4_INST *>> insts;
};

class CComparator {
public:
  bool operator()(const G4_Declare *first, const G4_Declare *second) const {
    return first->getDeclId() < second->getDeclId();
  }
};

class LoopVarSplit {
public:
  LoopVarSplit(G4_Kernel &k, GraphColor *c,
               const LivenessAnalysis *liveAnalysis);
  ~LoopVarSplit() { delete rpe; }
  LoopVarSplit(const LoopVarSplit&) = delete;
  LoopVarSplit& operator=(const LoopVarSplit&) = delete;

  void run();

  std::vector<G4_SrcRegRegion *> getReads(G4_Declare *dcl, Loop &loop);
  std::vector<G4_DstRegRegion *> getWrites(G4_Declare *dcl, Loop &loop);
  unsigned int getMaxRegPressureInLoop(Loop &loop);
  void dump(std::ostream &of = std::cerr);

  static void removeAllSplitInsts(GlobalRA *gra, G4_Declare *dcl);
  static void removeSplitInsts(GlobalRA *gra, G4_Declare *spillDcl, G4_BB *bb);
  static bool removeFromPreheader(GlobalRA *gra, G4_Declare *spillDcl,
                                  G4_BB *bb, INST_LIST_ITER filledInstIter);
  static bool removeFromLoopExit(GlobalRA *gra, G4_Declare *spillDcl, G4_BB *bb,
                                 INST_LIST_ITER filledInstIter);
  static const std::unordered_set<G4_INST *> getSplitInsts(GlobalRA *gra,
                                                           G4_BB *bb);

private:
  const unsigned int cLargeLoop = 500;

  bool split(G4_Declare *dcl, Loop &loop);
  void copy(G4_BB *bb, G4_Declare *dst, G4_Declare *src,
            SplitResults *splitData, bool isDefault32bMask,
            bool isDefault64bMask, bool pushBack = true);
  void replaceSrc(G4_SrcRegRegion *src, G4_Declare *dcl, const Loop &loop);
  void replaceDst(G4_DstRegRegion *dst, G4_Declare *dcl, const Loop &loop);
  G4_Declare *getNewDcl(G4_Declare *dcl1, G4_Declare *dcl2, const Loop &loop);
  std::vector<Loop *> getLoopsToSplitAround(G4_Declare *dcl);
  void adjustLoopMaxPressure(Loop &loop, unsigned int numRows);

  G4_Kernel &kernel;
  GraphColor *coloring = nullptr;
  RPE *rpe = nullptr;
  VarReferences references;

  // store set of dcls marked as spill in current RA iteration
  std::unordered_set<G4_Declare *> spilledDclSet;

  // store spill cost for each dcl
  std::map<G4_Declare *, float, CComparator> dclSpillCost;

  std::unordered_map<const Loop *,
                     std::unordered_map<G4_Declare *, G4_Declare *>>
      oldNewDclPerLoop;

  std::unordered_map<Loop *, std::unordered_set<G4_Declare *>> splitsPerLoop;

  std::unordered_map<Loop *, unsigned int> maxRegPressureCache;

  std::unordered_map<Loop *, unsigned int> scalarBytesSplit;

  // a spilled dcl may be split multiple times, once per loop
  // store this information to uplevel to GlobalRA class so
  // anytime we spill a split variable, we reuse spill location.
  // Orig dcl, vector<Tmp Dcl, Loop>
  std::unordered_map<G4_Declare *, std::vector<std::pair<G4_Declare *, Loop *>>>
      splitResults;

  class Size {
  public:
    enum class State {
      Undef = 0,
      Small = 1,
      Large = 2,
    };
    State state = State::Undef;
  };

  std::unordered_map<Loop *, Size> loopSizeCache;

  bool isLargeLoop(Loop &loop) {
    auto &size = loopSizeCache[&loop];
    if (size.state != Size::State::Undef)
      return size.state == Size::State::Large;
    unsigned int instCount = 0;
    std::for_each(loop.getBBs().begin(), loop.getBBs().end(),
                  [&instCount](G4_BB *bb) { instCount += bb->size(); });
    if (instCount > cLargeLoop)
      size.state = Size::State::Large;
    return size.state == Size::State::Large;
  }
};

class VarProperties {
public:
  enum class AccessGranularity { OneGrf = 1, TwoGrf = 2, Unknown = 3 };

  AccessGranularity ag = AccessGranularity::Unknown;
  unsigned int numDefs = 0;
  std::pair<G4_DstRegRegion *, G4_BB *> def;
  std::vector<std::pair<G4_SrcRegRegion *, G4_BB *>> srcs;
  bool candidateDef = false;
  bool legitCandidate = true;

  // API to check whether variable is local or global
  bool isDefUsesInSameBB() {
    auto defBB = def.second;
    for (auto src : srcs) {
      if (src.second != defBB)
        return false;
    }
    return true;
  }

  bool isPartDclUsed(unsigned int lb, unsigned int rb) {
    // Return true if lb/rb is part of any src regions
    for (auto &src : srcs) {
      if (src.first->getLeftBound() >= lb && src.first->getRightBound() <= rb)
        return true;
    }
    return false;
  }
};
}; // namespace vISA
#endif
