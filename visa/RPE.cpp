/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains implementation of Register Pressure Estimator.

#include "GraphColor.h"
#include "LoopAnalysis.h"
#include "PointsToAnalysis.h"
#include "RPE.h"
#include "Timer.h"

namespace vISA {
RPE::RPE(const GlobalRA &g, const LivenessAnalysis *l, DECLARE_LIST *spills)
    : gra(g), fg(g.kernel.fg), liveAnalysis(l), live(), vars(l->vars)
{
  options = g.kernel.getOptions();
  if (spills) {
    std::for_each(spills->begin(), spills->end(),
                  [&](const G4_Declare *dcl) { spilledVars.insert(dcl); });
  }
}

void RPE::run() {
  TIME_SCOPE(RPE);
  if (!vars.empty()) {
    for (auto &bb : gra.kernel.fg) {
      runBB(bb);
    }
  }
}

void RPE::runBB(G4_BB *bb) {
  G4_Declare *topdcl = nullptr;
  unsigned int id = 0;
  TotalGRFAligned = 0;
  TotalSubGRF = 0;

  // Compute reg pressure at BB exit
  regPressureBBExit(bb);

  auto updateLivenessForLLR = [this](LocalLiveRange *LLR, bool val) {
    int numRows = LLR->getTopDcl()->getNumRows();
    int sreg;
    G4_VarBase *preg = LLR->getPhyReg(sreg);
    int startGRF = preg->asGreg()->getRegNum();
    for (int i = startGRF; i < startGRF + numRows; ++i) {
      G4_Declare *GRFDcl = gra.getGRFDclForHRA(i);
      updateLiveness(live, GRFDcl->getRegVar()->getId(), val);
    }
  };

  // Iterate in bottom-up order to analyze register usage (similar to intf graph
  // construction)
  for (auto rInst = bb->rbegin(), rEnd = bb->rend(); rInst != rEnd; rInst++) {
    auto inst = (*rInst);
    auto dst = inst->getDst();

    rp[inst] = (uint32_t)regPressure;
    LocalLiveRange *LLR = nullptr;
    if (dst && (topdcl = dst->getTopDcl())) {
      if (topdcl->getRegVar()->isRegAllocPartaker()) {
        // Check if dst is killed
        if (liveAnalysis->writeWholeRegion(bb, inst, dst) ||
            inst->isPseudoKill()) {
          id = topdcl->getRegVar()->getId();
          updateLiveness(live, id, false);
        }
      } else if ((LLR = gra.getLocalLR(topdcl)) && LLR->getAssigned()) {
        uint32_t firstRefIdx;
        if (LLR->getFirstRef(firstRefIdx) == inst ||
            liveAnalysis->writeWholeRegion(bb, inst, dst)) {
          updateLivenessForLLR(LLR, false);
        }
      }
    }

    for (unsigned int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      auto src = inst->getSrc(i);
      G4_RegVar *regVar = nullptr;

      if (!src)
        continue;

      if (!src->isSrcRegRegion() || !src->getTopDcl())
        continue;

      if (!src->asSrcRegRegion()->isIndirect()) {
        if ((regVar = src->getTopDcl()->getRegVar()) &&
            regVar->isRegAllocPartaker()) {
          unsigned int id = regVar->getId();
          updateLiveness(live, id, true);
        } else if ((LLR = gra.getLocalLR(src->getTopDcl())) &&
                   LLR->getAssigned()) {
          updateLivenessForLLR(LLR, true);
        }
      } else if (src->asSrcRegRegion()->isIndirect()) {
        // make every var in points-to set live
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(src,
                                                                          bb);
        for (const auto &pt : pointsToSet) {
          if (pt.var->isRegAllocPartaker()) {
            updateLiveness(live, pt.var->getId(), true);
          }
        }
      }
    }
  }
}

void RPE::regPressureBBExit(G4_BB *bb) {
  live.clear();
  live = liveAnalysis->use_out[bb->getId()];
  live &= liveAnalysis->def_out[bb->getId()];

  // Iterate over all live variables and add up numRows required
  // for each. For scalar variables, add them up separately.
  regPressure = 0;
  unsigned int numScalarBytes = 0;
  for (auto LI = live.begin(), LE = live.end(); LI != LE; ++LI) {
    unsigned i = *LI;
    {
      auto range = vars[i];
      G4_Declare *rootDcl = range->getDeclare()->getRootDeclare();
      if (isSpilled(rootDcl))
        continue;
      if (isStackPseudoVar(rootDcl))
        continue;
      if (rootDcl->getNumElems() > 1) {
        regPressure += rootDcl->getNumRows();
      } else {
        auto dclSize = rootDcl->getByteSize();
        auto alignBytes = static_cast<uint32_t>(rootDcl->getSubRegAlign()) * 2;
        if (dclSize < gra.builder.getGRFSize() && dclSize < alignBytes) {
          dclSize = handleSubGRFPressure(dclSize, alignBytes);
        }
        numScalarBytes += dclSize;
      }
    }
  }

  regPressure += (double)numScalarBytes / gra.builder.getGRFSize();
}

void RPE::updateLiveness(SparseBitVector &live, uint32_t id, bool val) {
  bool change = false;
  bool clean = false;
  if (val) { //true
    if (!live.test(id)) { //used to be false
      change = true;
      live.set(id);
    }
  } else {
    if (live.test(id)) { //
      change = true;
      clean = true;
      live.reset(id);
    }
  }
  updateRegisterPressure(change, clean, id);
}

void RPE::updateRegisterPressure(bool change, bool clean, unsigned int id) {
  if (change) {
    auto dcl = vars[id]->getDeclare();
    if (isSpilled(dcl))
      return;
    if (isStackPseudoVar(dcl))
      return;
    // For < 1 GRF variable we have to take alignment into consideration as well
    // when computing register pressure. For now we double each < 1 GRF
    // variable's size if its alignment also exceeds its size. Alternative is to
    // simply take the alignment as the size, but it might cause performance
    // regressions due to being too conservative (i.e., a GRF-aligned variable
    // may share physical GRF with several other
    auto dclSize = dcl->getByteSize();
    auto alignBytes = static_cast<uint32_t>(dcl->getSubRegAlign()) * 2;
    if (dclSize < gra.builder.getGRFSize() && dclSize < alignBytes) {
      dclSize = handleSubGRFPressure(dclSize, alignBytes);
    }

    double delta = dclSize < gra.builder.getGRFSize()
                       ? dclSize / (double)gra.builder.getGRFSize()
                       : (double)dcl->getNumRows();
    if (clean) {
      if (regPressure < delta) {
        regPressure = 0;
      } else {
        regPressure -= delta;
      }
    } else {
      regPressure += delta;
    }
  }
  maxRP = std::max(maxRP, (uint32_t)regPressure);
}

unsigned int RPE::handleSubGRFPressure(unsigned int dclSize,
                                       unsigned int alignBytes) {
  // If high % of < 1 GRF variables are GRF aligned then we
  // approximate their contribution to RPE as 1. This is because
  // we are unlikely to find other < 1 GRF variables that can
  // fit in free space left behind by GRF aligned < 1 GRF variables.
  // Currently, we use a constant of 99%. Using too low a value
  // means we overestimate impact of 1 GRF aligned variables.
  TotalSubGRF++;
  if (alignBytes == gra.builder.getGRFSize()) {
    TotalGRFAligned++;
  }
  if (TotalSubGRF > MinNumSubGRFVars &&
      (float)TotalGRFAligned / (float)TotalSubGRF > LowerThreshold)
    return gra.builder.getGRFSize();
  return std::min(dclSize * 2, alignBytes);
}

void RPE::recomputeMaxRP() {
  maxRP = 0;
  // Find max register pressure over all entries in map
  for (const auto &item : rp) {
    maxRP = std::max(maxRP, item.second);
  }
}

SparseBitVector RPE::computeLoopVars(Loop *loop) const {
  SparseBitVector loopVars;
  for (G4_BB *bb : loop->getBBs()) {
    for (G4_INST *inst : *bb) {
      auto dst = inst->getDst();
      if (dst && dst->getTopDcl()) {
        G4_RegVar *rv = dst->getTopDcl()->getRegVar();
        if (rv && rv->isRegAllocPartaker())
          loopVars.set(rv->getId());
      }
      for (unsigned i = 0, n = inst->getNumSrc(); i < n; i++) {
        auto src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion() || !src->getTopDcl())
          continue;
        if (src->asSrcRegRegion()->isIndirect()) {
          const REGVAR_VECTOR &pts =
              liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                  src, bb);
          for (const auto &pt : pts)
            if (pt.var->isRegAllocPartaker())
              loopVars.set(pt.var->getId());
        } else {
          G4_RegVar *rv = src->getTopDcl()->getRegVar();
          if (rv && rv->isRegAllocPartaker())
            loopVars.set(rv->getId());
        }
      }
    }
  }
  return loopVars;
}

void RPE::runBBForLoop(G4_BB *bb, const SparseBitVector &loopVars, Loop *loop) {
  G4_Declare *topdcl = nullptr;
  TotalGRFAligned = 0;
  TotalSubGRF = 0;

  // Initialise filtered live set at BB exit: only variables in loopVars.
  live.clear();
  live = liveAnalysis->use_out[bb->getId()];
  live &= liveAnalysis->def_out[bb->getId()];
  live &= loopVars;

  // Compute initial register pressure from the filtered live set.
  regPressure = 0;
  unsigned int numScalarBytes = 0;
  for (auto LI = live.begin(), LE = live.end(); LI != LE; ++LI) {
    unsigned i = *LI;
    G4_Declare *rootDcl = vars[i]->getDeclare()->getRootDeclare();
    if (isSpilled(rootDcl) || isStackPseudoVar(rootDcl))
      continue;
    if (rootDcl->getNumElems() > 1) {
      regPressure += rootDcl->getNumRows();
    } else {
      auto dclSize = rootDcl->getByteSize();
      auto alignBytes = static_cast<uint32_t>(rootDcl->getSubRegAlign()) * 2;
      if (dclSize < gra.builder.getGRFSize() && dclSize < alignBytes)
        dclSize = handleSubGRFPressure(dclSize, alignBytes);
      numScalarBytes += dclSize;
    }
  }
  regPressure += (double)numScalarBytes / gra.builder.getGRFSize();

  auto updateLivenessForLLR = [this](LocalLiveRange *LLR, bool val) {
    int numRows = LLR->getTopDcl()->getNumRows();
    int sreg;
    G4_VarBase *preg = LLR->getPhyReg(sreg);
    int startGRF = preg->asGreg()->getRegNum();
    for (int i = startGRF; i < startGRF + numRows; ++i) {
      G4_Declare *GRFDcl = gra.getGRFDclForHRA(i);
      updateLiveness(live, GRFDcl->getRegVar()->getId(), val);
    }
  };

  // Walk instructions bottom-up, mirroring runBB but:
  //   - only tracking variables present in loopVars
  //   - storing per-instruction filtered pressure in loopInstRP[loop]
  auto &instMap = loopInstRP[loop];
  for (auto rInst = bb->rbegin(), rEnd = bb->rend(); rInst != rEnd; ++rInst) {
    auto inst = *rInst;
    instMap[inst] = (uint32_t)regPressure;

    auto dst = inst->getDst();
    LocalLiveRange *LLR = nullptr;
    if (dst && (topdcl = dst->getTopDcl())) {
      if (topdcl->getRegVar()->isRegAllocPartaker()) {
        unsigned id = topdcl->getRegVar()->getId();
        if (loopVars.test(id) &&
            (liveAnalysis->writeWholeRegion(bb, inst, dst) ||
             inst->isPseudoKill()))
          updateLiveness(live, id, false);
      } else if ((LLR = gra.getLocalLR(topdcl)) && LLR->getAssigned()) {
        uint32_t firstRefIdx;
        if (LLR->getFirstRef(firstRefIdx) == inst ||
            liveAnalysis->writeWholeRegion(bb, inst, dst))
          updateLivenessForLLR(LLR, false);
      }
    }

    for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
      auto src = inst->getSrc(j);
      if (!src || !src->isSrcRegRegion() || !src->getTopDcl())
        continue;
      if (!src->asSrcRegRegion()->isIndirect()) {
        G4_RegVar *rv = src->getTopDcl()->getRegVar();
        if (rv && rv->isRegAllocPartaker()) {
          if (loopVars.test(rv->getId()))
            updateLiveness(live, rv->getId(), true);
        } else if ((LLR = gra.getLocalLR(src->getTopDcl())) &&
                   LLR->getAssigned()) {
          updateLivenessForLLR(LLR, true);
        }
      } else {
        const REGVAR_VECTOR &pts =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(src,
                                                                          bb);
        for (const auto &pt : pts)
          if (pt.var->isRegAllocPartaker() && loopVars.test(pt.var->getId()))
            updateLiveness(live, pt.var->getId(), true);
      }
    }
  }
}

void RPE::runLoop(Loop *loop) {
  SparseBitVector loopVars = computeLoopVars(loop);

  for (G4_BB *bb : loop->getBBs())
    runBBForLoop(bb, loopVars, loop);

  // Derive the loop-wide max from the per-instruction data.
  uint32_t loopMaxRP = 0;
  for (auto &[inst, pressure] : loopInstRP[loop])
    loopMaxRP = std::max(loopMaxRP, pressure);
  loopRP[loop] = loopMaxRP;
}

void RPE::runLoopHierarchy(Loop *loop) {
  runLoop(loop);
  for (Loop *nested : *loop)
    runLoopHierarchy(nested);
}

void RPE::runLoops() {
  if (vars.empty())
    return;
  auto &loopDetection = gra.kernel.fg.getLoops();
  loopDetection.recomputeIfStale();
  for (Loop *top : loopDetection.getTopLoops())
    runLoopHierarchy(top);
}

unsigned int RPE::getLoopMaxRP(const Loop *loop) const {
  auto it = loopRP.find(loop);
  if (it == loopRP.end())
    return 0;
  return it->second;
}

unsigned int RPE::getLoopInstRP(const Loop *loop, G4_INST *inst) const {
  auto loopIt = loopInstRP.find(loop);
  if (loopIt == loopInstRP.end())
    return 0;
  auto instIt = loopIt->second.find(inst);
  if (instIt == loopIt->second.end())
    return 0;
  return instIt->second;
}

void RPE::dump() const {
  std::cerr << "Max pressure = " << maxRP << "\n";
  for (auto &bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      std::cerr << "[";
      if (rp.count(inst)) {
        std::cerr << rp.at(inst);
      } else {
        std::cerr << "??";
      }
      std::cerr << "]";
      inst->dump();
    }
    std::cerr << "\n";
  }
}

void RPE::dumpLoops(std::ostream &os) const {
  os << "\nLoop Register Pressure:\n";

  std::function<void(Loop *, unsigned)> dumpLoop = [&](Loop *loop,
                                                        unsigned indent) {
    auto it = loopRP.find(loop);
    unsigned pressure = (it != loopRP.end()) ? it->second : 0;
    for (unsigned i = 0; i < indent; i++)
      os << "  ";
    os << "Loop (header BB" << loop->getHeader()->getId()
       << ", nesting=" << loop->getNestingLevel()
       << "): max RP = " << pressure << "\n";
    for (Loop *nested : *loop)
      dumpLoop(nested, indent + 1);
  };

  for (Loop *top : gra.kernel.fg.getLoops().getTopLoops())
    dumpLoop(top, 0);
}
} // namespace vISA
