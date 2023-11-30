/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __REMAT_H__
#define __REMAT_H__

#include "FlowGraph.h"
#include "GraphColor.h"
#include "RPE.h"
#include <list>
#include <map>

namespace vISA {
// Remat will trigger only for vars that have less than following uses
#define MAX_USES_REMAT 6

// Minimum def-use distance for remat to trigger
#define MIN_DEF_USE_DISTANCE 20

// Distance in instructions to reuse rematted value in BB
#define MAX_LOCAL_REMAT_REUSE_DISTANCE 40

typedef std::pair<G4_INST *, G4_BB *> Reference;
class References {
public:
  std::vector<Reference> def;
  // std::vector<Reference> uses;

  unsigned int numUses = 0;

  // lastUseLexId provides a quick and dirty way
  // to determine end of liveness for a variable.
  // This is not always accurate due to holes
  // in live-ranges but should be fine most times.
  unsigned int lastUseLexId = 0;

  // Store number of times this var has been used in
  // a remat'd operation. This forms part of heuristic
  // that decides if it is profitable to remat an
  // operation inside a loop.
  unsigned int numRemats = 0;

  // Store set of rows of this variable ever used.
  // This is useful for samplers.
  std::unordered_set<unsigned int> rowsUsed;
};

class Rematerialization {
private:
  G4_Kernel &kernel;
  const LivenessAnalysis &liveness;
  GraphColor &coloring;
  GlobalRA &gra;
  G4_Declare *samplerHeader = nullptr;
  unsigned int numRematsInLoop = 0;
  bool IRChanged = false;
  bool samplerHeaderMapPopulated = false;
  unsigned int loopInstsBeforeRemat = 0;
  unsigned int totalInstsBeforeRemat = 0;
  RPE &rpe;

  static const unsigned int cRematLoopRegPressure128GRF = 85;
  static const unsigned int cRematRegPressure128GRF = 120;

  unsigned int rematLoopRegPressure = 0;
  unsigned int rematRegPressure = 0;

  std::vector<G4_Declare *> preDefinedVars;
  std::vector<G4_Declare *> spills;
  // For each top dcl, this map holds all defs
  std::unordered_map<G4_Declare *, References> operations;
  // This vector contains declares that could potentially save spill
  // if remat'd.
  std::vector<bool> rematCandidates;

  // Map each sampler instruction with instruction initializing
  // samplerHeader instruction. This is required when inserting
  // remat'd samplers.
  std::unordered_map<G4_INST *, G4_INST *> samplerHeaderMap;
  std::unordered_set<G4_BB *> deLVNedBBs;
  // Map BB->subroutine it belongs to
  // BBs not present are assumed to belong to main kernel
  std::unordered_map<G4_BB *, const FuncInfo *> BBPerSubroutine;
  bool cr0DefBB = false;

  void populateRefs();
  void populateSamplerHeaderMap();
  void deLVNSamplers(G4_BB *);
  bool usesNoMaskWA(const Reference *uniqueDef);
  bool canRematerialize(G4_SrcRegRegion *, G4_BB *, const Reference *&,
                        INST_LIST_ITER instIter);
  G4_SrcRegRegion *rematerialize(G4_SrcRegRegion *, G4_BB *, const Reference *,
                                 std::list<G4_INST *> &, G4_INST *&);
  G4_SrcRegRegion *createSrcRgn(G4_SrcRegRegion *, G4_DstRegRegion *,
                                G4_Declare *);
  const Reference *findUniqueDef(References &, G4_SrcRegRegion *);
  bool areInSameLoop(G4_BB *, G4_BB *, bool &);
  bool isRangeSpilled(G4_Declare *);
  bool areAllDefsInBB(G4_Declare *, G4_BB *, unsigned int);
  unsigned int getLastUseLexId(G4_Declare *);
  bool checkLocalWAR(G4_INST *, G4_BB *, INST_LIST_ITER);
  void updateSplitInfo(G4_INST *dstInst, int srcNum);
  void reduceNumUses(G4_Declare *dcl) {
    auto opIt = operations.find(dcl);
    if (opIt != operations.end()) {
      auto numUses = (*opIt).second.numUses;
      if (numUses > 0)
        opIt->second.numUses = numUses - 1;

      if (numUses == 1) {
        for (const auto &ref : opIt->second.def) {
          ref.second->remove(ref.first);
        }
        opIt->second.def.clear();
      }
    }
  }

  unsigned int getNumUses(G4_Declare *dcl) const {
    auto opIt = operations.find(dcl);
    if (opIt != operations.end())
      return opIt->second.numUses;

    return 0;
  }

  void incNumRemat(G4_Declare *dcl) {
    auto opIt = operations.find(dcl);
    if (opIt != operations.end())
      opIt->second.numRemats += 1;
  }

  unsigned int getNumRemats(G4_Declare *dcl) const {
    auto opIt = operations.find(dcl);
    if (opIt != operations.end())
      return opIt->second.numRemats;

    return 0;
  }

  bool isRematCandidateOp(G4_INST *inst) const {
    if (inst->isFlowControl() || inst->isWait() ||
        (inst->isSend() && inst->asSendInst()->isFence()) ||
        inst->isLifeTimeEnd() || inst->isAccDstInst() || inst->isAccSrcInst() ||
        inst->getImplAccDst() || inst->getImplAccSrc() ||
        inst->isRelocationMov()) {
      return false;
    }

    if (inst->isCallerRestore() || inst->isCallerSave() ||
        inst->isCalleeRestore() || inst->isCalleeSave()) {
      return false;
    }

    G4_Declare *dcl = nullptr;
    if (inst->getDst() && inst->getDst()->getTopDcl())
      dcl = inst->getDst()->getTopDcl();

    if (kernel.fg.builder->isPreDefArg(dcl) ||
        kernel.fg.builder->isPreDefRet(dcl) ||
        kernel.fg.builder->isPreDefFEStackVar(dcl))
      return false;

    return true;
  }

  void cleanRedundantSamplerHeaders();

  unsigned int getNumRematsInLoop() const { return numRematsInLoop; }
  void incNumRematsInLoop() { numRematsInLoop++; }
  bool inSameSubroutine(G4_BB *, G4_BB *);

  bool isPartGRFBusyInput(G4_Declare *inputDcl, unsigned int atLexId);

public:
  Rematerialization(G4_Kernel &k, const LivenessAnalysis &l, GraphColor &c,
                    RPE &r, GlobalRA &g)
      : kernel(k), liveness(l), coloring(c), gra(g), rpe(r) {
    unsigned numGRFs = k.getNumRegTotal();
    auto scale = [=](unsigned threshold) -> unsigned {
      float ratio = 1.0f - (128 - threshold) / 128.0f;
      return static_cast<unsigned>(numGRFs * ratio);
    };
    rematLoopRegPressure = scale(cRematLoopRegPressure128GRF);
    rematRegPressure = scale(cRematRegPressure128GRF);

    rematCandidates.resize(l.getNumSelectedVar(), false);

    for (auto &&lr : coloring.getSpilledLiveRanges()) {
      auto dcl = lr->getDcl()->getRootDeclare();
      if (!dcl->isSpilled()) {
        spills.push_back(dcl);
        dcl->setSpillFlag();
      }

      // Mark all simultaneously live variables as remat candidates
      unsigned int spillId = dcl->getRegVar()->getId();
      const auto &intfVec = coloring.getIntf()->getSparseIntfForVar(spillId);

      for (auto intfId : intfVec) {
        rematCandidates[intfId] = true;
      }
    }

    for (auto &bb : kernel.fg) {
      if (bb->getNestLevel() != 0) {
        for (auto &inst : *bb) {
          if (!inst->isLabel() && !inst->isPseudoKill()) {
            loopInstsBeforeRemat++;
          }
        }
      }
    }

    // Map BBs in subroutines
    for (auto curFuncInfo : kernel.fg.funcInfoTable) {
      const auto &bbList = curFuncInfo->getBBList();
      for (auto bb : bbList) {
        BBPerSubroutine.insert(std::make_pair(bb, curFuncInfo));
      }
    }
  }

  ~Rematerialization() {
    for (auto &&dcl : spills) {
      dcl->resetSpillFlag();
    }
  }

  Rematerialization(const Rematerialization&) = delete;
  Rematerialization& operator=(const Rematerialization&) = delete;

  bool getChangesMade() const { return IRChanged; }

  void run();
};
} // namespace vISA
#endif
