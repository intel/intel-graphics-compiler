/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains the implementation of frequency info pass.

#include "FrequencyInfo.h"
#include "BuildIR.h"
#include "PointsToAnalysis.h"
#include "G4_IR.hpp"
#include "GraphColor.h"
#include "RegAlloc.h"

using Scaled64 = llvm::ScaledNumber<uint64_t>;
namespace vISA {
FrequencyInfo::FrequencyInfo(IR_Builder *builder, G4_Kernel &k)
    : irb(builder), curFreq(Scaled64::getZero()), kernel(k),
      lastInstMarked(nullptr), dumpEnabled(irb->getOption(vISA_DumpFreqBasedSpillCost)) {}

void FrequencyInfo::setCurrentStaticFreq(uint64_t digits, int16_t scale) {
  curFreq = llvm::ScaledNumber<uint64_t>(digits, scale);
  lastInstMarked = !irb->instAllocList.empty() ? irb->instAllocList.back() : nullptr;
  return;
}
void FrequencyInfo::transferFreqToG4Inst()
{
  for (auto itr = irb->instAllocList.rbegin();
    itr != irb->instAllocList.rend() || *itr != lastInstMarked; itr++)
  {
    if ((*itr)->isCFInst())
      storeStaticFrequencyAsMetadata(*itr);
  }
  return;
}

void FrequencyInfo::storeStaticFrequencyAsMetadata(G4_INST* i)
{
  MDNode *md_digits =
      irb->allocateMDString(std::to_string(curFreq.getDigits()));
  MDNode *md_scale = irb->allocateMDString(std::to_string(curFreq.getDigits()));
  i->setMetadata("stats.blockFrequency.digits", md_digits);
  i->setMetadata("stats.blockFrequency.scale", md_scale);

  if (dumpEnabled) {
    if (hasFreqMetaData(i)) {
      std::cerr << "G4_Inst - Frequency data: digits=";
      md_digits->dump();
      std::cerr << " scale=";
      md_scale->dump();
      std::cerr << " ";
      i->dump();
    } else {
      std::cerr << "G4_Inst - No frequency data available";
      i->dump();
    }
  }
}

void FrequencyInfo::updateStaticFrequencyForBasicBlock(G4_BB *bb) {
  G4_INST *lastInst =
      !bb->getInstList().empty() ? bb->getInstList().back() : nullptr;
  if (hasFreqMetaData(lastInst)) {
    MDNode *mn_digits = lastInst->getMetadata("stats.blockFrequency.digits");
    MDNode *mn_scale = lastInst->getMetadata("stats.blockFrequency.scale");
    Scaled64 freq =
        Scaled64(stoull((mn_digits->asMDString())->getData()),
                 (int16_t)stoi((mn_scale->asMDString())->getData()));
    BlockFreqInfo[bb] = freq;
    if (dumpEnabled) {
      std::cerr << "G4_BB - Frequency data: digits =";
      mn_digits->dump();
      std::cerr << " scale =";
      mn_scale->dump();
      std::cerr << " ";
      bb->getLabel()->dump();
      std::cerr << "\n";
    }
  } else {
    if (dumpEnabled) {
      std::cerr << "G4_BB - No frequency data available ";
      bb->getLabel()->dump();
      std::cerr << "\n";
    }
  }
  return;
}

void FrequencyInfo::updateStaticFrequency(
  std::unordered_map<G4_Label*, std::vector<G4_BB*>>& subroutines)
{
  for (auto subroutine : subroutines)
    for (G4_BB *bb : subroutine.second)
      updateStaticFrequencyForBasicBlock(bb);
  return;
}

bool FrequencyInfo::underSpillFreqThreshold(const std::list<vISA::LiveRange*> &spilledLRs, int instNum)
{
  Scaled64 GRFSpillFillFreq = Scaled64::getZero();
  for (auto spilled : spilledLRs) {
    GRFSpillFillFreq += freqSpillCost[spilled];
  }
  auto underSpillFreqThreshold = [this](Scaled64& spillFreq, int asmCount) {
    int threshold = std::min(
      irb->getOptions()->getuInt32Option(vISA_AbortOnSpillThreshold),
      200u);
    return (spillFreq * Scaled64::get(200)) < Scaled64::get(threshold * asmCount);
  };
  bool isUnderThreshold = underSpillFreqThreshold(GRFSpillFillFreq, instNum);
  if (dumpEnabled)
  {
    if (isUnderThreshold)
      std::cerr << "GraphColor - Low spill (not retry): " << GRFSpillFillFreq.toString() << "\n";
    else
      std::cerr << "GraphColor - High spill (retry): " << GRFSpillFillFreq.toString() << "\n";
  }
  return isUnderThreshold;
}

void FrequencyInfo::computeFreqSpillCosts(GlobalRA &gra, LivenessAnalysis &liveAnalysis, std::vector<LiveRange*> &lrs, unsigned int numVar) {
  LiveRangeVec addressSensitiveVars;
  Scaled64 maxNormalFreqCost = Scaled64::getZero();
  VarReferences directRefs(kernel, true, false);
  std::unordered_map<G4_Declare*, std::list<std::pair<G4_INST*, G4_BB*>>> indirectRefs;

  auto getWeightedRefFreq = [&](G4_Declare* dcl) {
      auto defs = directRefs.getDefs(dcl);
      auto uses = directRefs.getUses(dcl);
      Scaled64 refFreq = Scaled64::getZero();

      if (defs) {
        for (auto& def : *defs) {
          auto* bb = std::get<1>(def);
          refFreq += BlockFreqInfo[bb];
        }
      }

      if (uses) {
        for (auto& use : *uses) {
          auto* bb = std::get<1>(use);
          refFreq += BlockFreqInfo[bb];
        }
      }

      if (dcl->getAddressed()) {
        auto indirectRefsIt = indirectRefs.find(dcl);
        if (indirectRefsIt != indirectRefs.end()) {
          auto& dclIndirRefs = (*indirectRefsIt).second;
          for (auto& item : dclIndirRefs) {
            auto bb = item.second;
            refFreq += BlockFreqInfo[bb];
          }
        }
      }
      return refFreq == Scaled64::getZero() ? Scaled64::getOne() : refFreq;
  };

  std::unordered_map<const G4_Declare*, std::vector<G4_Declare*>>
    addrTakenMap;
  std::unordered_map<G4_Declare*, std::vector<const G4_Declare*>>
    revAddrTakenMap;
  bool addrMapsComputed = false;
  auto incSpillCostCandidate = [&](LiveRange* lr) {
    if (kernel.getOption(vISA_IncSpillCostAllAddrTaken))
      return true;
    if (!addrMapsComputed) {
      const_cast<PointsToAnalysis&>(liveAnalysis.getPointsToAnalysis())
        .getPointsToMap(addrTakenMap);
      const_cast<PointsToAnalysis&>(liveAnalysis.getPointsToAnalysis())
        .getRevPointsToMap(revAddrTakenMap);
      addrMapsComputed = true;
    }

    // this condition is a safety measure and isnt expected to be true.
    auto it = revAddrTakenMap.find(lr->getDcl());
    if (it == revAddrTakenMap.end())
      return true;

    for (auto& addrVar : (*it).second) {
      if (addrTakenMap.count(addrVar) > 1)
        return true;
    }
    return false;
  };

  for (unsigned i = 0; i < numVar; i++) {
    G4_Declare* dcl = lrs[i]->getDcl();

    if (dcl->getIsPartialDcl()) {
      continue;
    }
    //
    // The spill cost of pseudo nodes inserted to aid generation of save/restore
    // code must be the minimum so that such nodes go to the bootom of the color
    // stack.
    //
    if (kernel.fg.isPseudoDcl(dcl)) {
      if (kernel.fg.isPseudoVCADcl(dcl)) {
        freqSpillCost[lrs[i]] = Scaled64(1, INT16_MIN);
      }
      else {
        freqSpillCost[lrs[i]] = Scaled64::getZero();
      }
    }

    auto dclLR = gra.getLocalLR(dcl);
    if (dclLR != NULL && dclLR->getSplit()) {
        freqSpillCost[lrs[i]] = Scaled64(2, INT16_MIN);
    }
    else if (gra.isAddrFlagSpillDcl(dcl) || lrs[i]->isRetIp() ||
      lrs[i]->getIsInfiniteSpillCost() == true ||
      ((lrs[i]->getVar()->isRegVarTransient() == true ||
        lrs[i]->getVar()->isRegVarTmp() == true) &&
        lrs[i]->getVar()->isSpilled() == false) ||
      dcl == gra.getOldFPDcl() ||
      (!irb->canReadR0() && dcl == irb->getBuiltinR0())) {
      freqSpillCost[lrs[i]] = Scaled64::getLargest();
    }
    else if (dcl->isDoNotSpill()) {
      freqSpillCost[lrs[i]] = Scaled64::getLargest();
    }
    //
    // Calculate spill costs of regular nodes.
    //
    else {
      Scaled64 refFreq = getWeightedRefFreq(lrs[i]->getDcl());
      Scaled64 spillCost = refFreq * refFreq * refFreq /
        (Scaled64::get(lrs[i]->getDegree() + 1) * Scaled64::get(lrs[i]->getDegree() + 1));

      freqSpillCost[lrs[i]] = spillCost;
      if (dumpEnabled)
      {
        std::cerr << "GraphColor -  a live range "; lrs[i]->emit(std::cerr); std::cerr << "\n";
        std::cerr << "GraphColor - Freq Spill cost: " << spillCost.toString() << "\n";
      }

      // Track address sensitive live range.
      if (liveAnalysis.isAddressSensitive(i) && incSpillCostCandidate(lrs[i])) {
        addressSensitiveVars.push_back(lrs[i]);
      }
      else {
        // Set the spill cost of all other normal live ranges, and
        // track the max normal cost.
        if (maxNormalFreqCost < spillCost)
        {
          maxNormalFreqCost = spillCost;
        }
      }
    }
  }

  //
  // Set the spill cost of address sensitive live ranges above all the
  // normal live ranges, so that they get colored before all the normal
  // live ranges.
  //
  for (LiveRange* lr : addressSensitiveVars) {
    Scaled64 fsc = freqSpillCost[lr];
    if (fsc != Scaled64::getLargest())
    {
      fsc += maxNormalFreqCost;
      freqSpillCost[lr] = fsc;
    }
  }
}

void FrequencyInfo::sortBasedOnFreq(std::vector<LiveRange*>& lrs)
{
  std::sort(lrs.begin(), lrs.end(), [&](LiveRange* lr1, LiveRange* lr2) {
    Scaled64 sp1 = freqSpillCost[lr1];
    Scaled64 sp2 = freqSpillCost[lr2];
    return sp1 < sp2 ||
      (sp1 == sp2 &&
        lr1->getVar()->getId() < lr2->getVar()->getId());
    }
  );
  return;
}

bool FrequencyInfo::hasFreqMetaData(G4_INST *i) {
  MDNode *md_digits = i->getMetadata("stats.blockFrequency.digits");
  MDNode *md_scale = i->getMetadata("stats.blockFrequency.scale");
  return md_digits && md_scale && (md_digits->asMDString()->getData() != "") &&
         (md_scale->asMDString()->getData() != "");
}

} // namespace vISA
