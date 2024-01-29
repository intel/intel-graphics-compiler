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
#include "RPE.h"

#include <cmath>

using Scaled64 = llvm::ScaledNumber<uint64_t>;
namespace vISA {
FrequencyInfo::FrequencyInfo(IR_Builder *builder, G4_Kernel &k)
    : kernel(k), irb(builder), lastInstMarked(nullptr), liveAnalysis(nullptr) {
  dumpEnabled = irb->getOptions()->getuInt32Option(vISA_DumpFreqBasedSpillCost);
  enabledSteps = irb->getOptions()->getuInt32Option(vISA_FreqBasedSpillCost);
  GRFSpillFillFreq = Scaled64::getZero();
  freqScale = Scaled64::get(
      irb->getOptions()->getuInt32Option(vISA_FreqBasedSpillCostScale));
}

void FrequencyInfo::transferFreqToG4Inst(uint64_t digits, int16_t scale) {
  if (irb->instAllocList.empty())
    return;
  G4_INST *lastInst = irb->instAllocList.back();
  Scaled64 curFreq = llvm::ScaledNumber<uint64_t>(digits, scale);
  InstFreqInfo[lastInst] = curFreq;
  storeStaticFrequencyAsMetadata(lastInst,curFreq);
  for (auto itr = irb->instAllocList.rbegin();
    itr != irb->instAllocList.rend() && *itr != lastInstMarked; itr++)
  {
    G4_INST *inst = *itr;
    tailInsts[inst] = lastInst;
  }
  lastInstMarked =
      !irb->instAllocList.empty() ? irb->instAllocList.back() : nullptr;
  return;
}

void FrequencyInfo::storeStaticFrequencyAsMetadata(G4_INST *i,
                                                   Scaled64 curFreq) {
  MDNode *md_digits =
      irb->allocateMDString(std::to_string(curFreq.getDigits()));
  MDNode *md_scale = irb->allocateMDString(std::to_string(curFreq.getScale()));
  i->setMetadata("stats.blockFrequency.digits", md_digits);
  i->setMetadata("stats.blockFrequency.scale", md_scale);

  if (willDumpLLVMToG4()) {
    std::cerr << "LLVM to G4_Inst - Frequency data: ";
    std::cerr << curFreq.toString();
    std::cerr << " digits=";
    md_digits->dump();
    std::cerr << " scale=";
    md_scale->dump();
    std::cerr << " ";
    i->dump();
  }
  return;
}

void FrequencyInfo::updateStaticFrequencyForBasicBlock(G4_BB *bb) {
  G4_INST *lastInst =
      !bb->getInstList().empty() ? bb->getInstList().back() : nullptr;
  if (lastInst) {
    Scaled64 freq = getFreqInfoFromInst(lastInst);
    BlockFreqInfo[bb] = freq;
    if (willDumpLLVMToG4()) {
      std::cerr << "G4_Inst to G4_BB - Frequency data: ";
      std::cerr << freq.toString();
      std::cerr << " digits =" << freq.getDigits();
      std::cerr << " scale =" << freq.getScale();
      std::cerr << " ";
      bb->getInstList().back()->dump();
      std::cerr << "\n";
    }
  } else {
    // A block with no instructions have zero frequency
    // Fixme: infer frequency number from Pres
    BlockFreqInfo[bb] = Scaled64::getZero();
    if (willDumpLLVMToG4()) {
      std::cerr << "G4_Inst to G4_BB - No instructions in a basic block\n";
    }
  }
  return;
}

void FrequencyInfo::updateStaticFrequency(
  std::unordered_map<G4_Label*, std::vector<G4_BB*>>& subroutines)
{
  for (const auto& subroutine : subroutines)
    for (G4_BB *bb : subroutine.second)
      updateStaticFrequencyForBasicBlock(bb);
  return;
}

bool FrequencyInfo::underFreqSpillThreshold(
    const std::list<vISA::LiveRange *> &spilledLRs, int instNum,
    unsigned int legacySpillFillCount, bool legacyUnderThreshold) {

  int threshold = std::min(
      irb->getOptions()->getuInt32Option(vISA_AbortOnSpillThreshold), 200u);
  Scaled64 const_val = Scaled64::get(200);

  if (willDumpOnSpilThreshold()) {
    if (!isFreqBasedSpillSelectionEnabled()) {
      for (auto spilledLR : spilledLRs) {
        std::cerr << "Spill threshold - spilled LR";
        spilledLR->emit(std::cerr);
        std::cerr << " Ref Cnt: " << spilledLR->getRefCount() << "\n";
      }
    }
    std::cerr << "Spill threshold - Kernel " << kernel.getName() << std::endl;
    std::cerr << "Spill threshold - Spilled LR count: " << spilledLRs.size() << std::endl;
    std::cerr << "Spill threshold - Inst cnt: " << instNum
              << " abortThreshold: " << threshold << " cont(C): " << const_val.toString()
              << std::endl;
    if (legacyUnderThreshold) {
      std::cerr
          << "Spill threshold - (Legacy) Low total spill count(no retry): "
          << legacySpillFillCount << ", total_spill_count * C ("
          << legacySpillFillCount * const_val.toInt<uint32_t>() << ") < inst_count * threshold ("
          << instNum * threshold << ")\n";
    } else {
      std::cerr << "Spill threshold - (Legacy) high total spill count(retry): "
                << legacySpillFillCount << ", total_spill_count * C ("
                << legacySpillFillCount * const_val.toInt<uint32_t>()
                << ") > inst_count*threshold (" << instNum * threshold
                << ")\n";
    }
    std::cerr << std::endl;
  }

  if (!isFreqBasedSpillSelectionEnabled())
    return legacyUnderThreshold;

  for (auto spilled : spilledLRs) {
    GRFSpillFillFreq += getRefFreq(spilled);
    if (willDumpOnSpilThreshold()) {
      std::cerr << "Spill threshold - spilled LR";
      spilled->emit(std::cerr);
      std::cerr << " Ref Cnt: " << spilled->getRefCount();
      std::cerr << " Static Ref Cnt: " << getStaticRefCnt(spilled);
      std::cerr << " Ref Freq: " << getRefFreq(spilled).toString();
      std::cerr << " Freq Spill cost: " << getFreqSpillCost(spilled);
      std::cerr << " Total GRFSpillFillFreq: " << GRFSpillFillFreq.toString()
                << "\n";
    }
  }

  auto underSpillFreqThreshold = [&](Scaled64 &spillFreq, int asmCount) {
    return spillFreq * const_val * freqScale <
           Scaled64::get(threshold * asmCount);
  };
  bool isUnderThreshold = underSpillFreqThreshold(GRFSpillFillFreq, instNum);
  if (willDumpOnSpilThreshold()) {
    std::cerr << "Spill threshold - Scale * C: "
              << freqScale.toString()
              << "*" << const_val.toString()
              << " = " << (freqScale*const_val).toString() << "\n";
    if (isUnderThreshold) {
      std::cerr << "Spill threshold - (Static Profile) Low total spill "
                   "frequency(no retry): "
                << GRFSpillFillFreq.toString() << ", total_spill_freq * C * scale ("
                << (GRFSpillFillFreq * const_val * freqScale).toString()
                << ") < inst_count*threshold (" << instNum * threshold
                << ")\n";
    } else {
      std::cerr << "Spill threshold - (Static Profile) High total spill "
                   "frequency(retry): "
                << GRFSpillFillFreq.toString() << ", total_spill_freq * C * scale ("
                << (GRFSpillFillFreq * const_val * freqScale).toString()
                << ") > inst_count*threshold (" << instNum * threshold
                << ")\n";
    }
    std::cerr << std::endl;
  }
  return isUnderThreshold;
}

void FrequencyInfo::computeFreqSpillCosts(GlobalRA &gra,
                                          bool useSplitLLRHeuristic,
                                          const RPE *rpe) {
  LiveRangeVec addressSensitiveVars;
  float maxNormalFreqCost = 0;
  VarReferences directRefs(kernel, true, false);
  std::unordered_map<G4_Declare*, std::list<std::pair<G4_INST*, G4_BB*>>> indirectRefs;
  unsigned numVar = liveAnalysis->getNumSelectedVar();
  std::vector<LiveRange *> &lrs = liveAnalysis->gra.incRA.getLRs();

  bool useNewSpillCost =
      (irb->getOption(vISA_NewSpillCostFunctionISPC) ||
       irb->getOption(vISA_NewSpillCostFunction)) &&
      rpe &&
      !(gra.getIterNo() == 0 &&
        (float)rpe->getMaxRP() < (float)kernel.getNumRegTotal() * 0.80f);

  if (willDumpSpillCostAnalysis()) {
    //std::cerr << "Spill cost analysis - Norm factor: "
              //<< freqNormFactor.toString() << std::endl;
    std::cerr << "Spill cost analysis - Selected var cnt: " << numVar
              << std::endl;
    std::cerr << "Spill cost analysis - Live range cnt: " << lrs.size()
              << std::endl;
  }

  if (!isSpillCostComputationEnabled())
    return;


  if (useNewSpillCost && liveAnalysis->livenessClass(G4_GRF)) {
    // gather all instructions with indirect operands
    // for ref count computation once.
    for (auto bb : kernel.fg.getBBList()) {
      for (auto inst : bb->getInstList()) {
        auto dst = inst->getDst();
        if (dst && dst->isIndirect()) {
          auto pointsTo = liveAnalysis->getPointsToAnalysis().getAllInPointsTo(
              dst->getBase()
                  ->asRegVar()
                  ->getDeclare()
                  ->getRootDeclare()
                  ->getRegVar());
          if (pointsTo) {
            for (auto &pointee : *pointsTo)
              indirectRefs[pointee.var->getDeclare()->getRootDeclare()]
                  .push_back(std::make_pair(inst, bb));
          }
          continue;
        }

        for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
          auto src = inst->getSrc(i);
          if (!src || !src->isSrcRegRegion() ||
              !src->asSrcRegRegion()->isIndirect()) {
            continue;
          }
          auto pointsTo = liveAnalysis->getPointsToAnalysis().getAllInPointsTo(
              src->asSrcRegRegion()
                  ->getBase()
                  ->asRegVar()
                  ->getDeclare()
                  ->getRootDeclare()
                  ->getRegVar());
          if (pointsTo) {
            for (auto &pointee : *pointsTo)
              indirectRefs[pointee.var->getDeclare()->getRootDeclare()]
                  .push_back(std::make_pair(inst, bb));
          }
          continue;
        }
      }
    }
  }

  auto getWeightedRefCount = [&](G4_Declare *dcl, unsigned int useWt = 1,
                                 unsigned int defWt = 1) {
    auto defs = directRefs.getDefs(dcl);
    auto uses = directRefs.getUses(dcl);
    auto &loops = kernel.fg.getLoops();
    unsigned int refCount = 0;
    const unsigned int assumeLoopIter = 10;

    if (defs) {
      for (auto &def : *defs) {
        auto *bb = std::get<1>(def);
        auto *innerMostLoop = loops.getInnerMostLoop(bb);
        if (innerMostLoop) {
          auto nestingLevel = innerMostLoop->getNestingLevel();
          refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
        } else
          refCount += defWt;
      }
    }

    if (uses) {
      for (auto &use : *uses) {
        auto *bb = std::get<1>(use);
        auto *innerMostLoop = loops.getInnerMostLoop(bb);
        if (innerMostLoop) {
          auto nestingLevel = innerMostLoop->getNestingLevel();
          refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
        } else
          refCount += useWt;
      }
    }

    if (dcl->getAddressed()) {
      auto indirectRefsIt = indirectRefs.find(dcl);
      if (indirectRefsIt != indirectRefs.end()) {
        auto &dclIndirRefs = (*indirectRefsIt).second;
        for (auto &item : dclIndirRefs) {
          auto bb = item.second;

          auto *innerMostLoop = loops.getInnerMostLoop(bb);
          if (innerMostLoop) {
            auto nestingLevel = innerMostLoop->getNestingLevel();
            refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
          } else
            refCount += useWt;
        }
      }
    }

    return refCount == 0 ? 1 : refCount;
  };

  auto getWeightedStaticFreq = [&](G4_Declare *dcl) {
    auto defs = directRefs.getDefs(dcl);
    auto uses = directRefs.getUses(dcl);
    Scaled64 refFreq = Scaled64::getZero();

    if (defs) {
      for (auto &def : *defs) {
        auto *bb = std::get<1>(def);
        refFreq += getBlockFreqInfo(bb);
      }
    }

    if (uses) {
      for (auto &use : *uses) {
        auto *bb = std::get<1>(use);
        refFreq += getBlockFreqInfo(bb);
      }
    }

    if (dcl->getAddressed()) {
      auto indirectRefsIt = indirectRefs.find(dcl);
      if (indirectRefsIt != indirectRefs.end()) {
        auto &dclIndirRefs = (*indirectRefsIt).second;
        for (auto &item : dclIndirRefs) {
          auto bb = item.second;
          refFreq += getBlockFreqInfo(bb);
        }
      }
    }
    refFreq += Scaled64::getOne();
    double refFreqDouble =
        refFreq.getDigits() * (double)std::pow(2, refFreq.getScale());
    return refFreqDouble;
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
      const_cast<PointsToAnalysis&>(liveAnalysis->getPointsToAnalysis())
        .getPointsToMap(addrTakenMap);
      const_cast<PointsToAnalysis&>(liveAnalysis->getPointsToAnalysis())
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
    LiveRange *lr = lrs[i];
    G4_Declare *dcl = lr->getDcl();
    Scaled64 refFreq = getRefFreq(lr);
    double refFreqDouble = refFreq.getDigits() * (double)std::pow(2,refFreq.getScale());
    if (willDumpSpillCostAnalysis()) {
      std::cerr << "Spill cost analysis - reference frequency: "
                << refFreqDouble
                << std::endl;
    }
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
        setFreqSpillCost(lr, MINSPILLCOST + 1);
      } else {
        setFreqSpillCost(lr, MINSPILLCOST);
      }
    }

    auto dclLR = gra.getLocalLR(dcl);
    if (dclLR != NULL && dclLR->getSplit()) {
      setFreqSpillCost(lr, MINSPILLCOST + 2);
    } else if (gra.isAddrFlagSpillDcl(dcl) || lrs[i]->isRetIp() ||
               lrs[i]->getIsInfiniteSpillCost() == true ||
               ((lrs[i]->getVar()->isRegVarTransient() == true ||
                 lrs[i]->getVar()->isRegVarTmp() == true) &&
                lrs[i]->getVar()->isSpilled() == false) ||
               dcl == gra.getOldFPDcl() ||
               (!irb->canReadR0() && dcl == irb->getBuiltinR0())) {
      setFreqSpillCost(lr, MAXSPILLCOST);
    } else if (dcl->isDoNotSpill()) {
      setFreqSpillCost(lr, MAXSPILLCOST);
    }
    //
    // Calculate spill costs of regular nodes.
    //
    else {
      float spillCost = 0.0f;
      unsigned refCount = lrs[i]->getRefCount();
      unsigned degree = lrs[i]->getDegree() + 1;
      unsigned int byteSize = lrs[i]->getDcl()->getByteSize();
      unsigned short numRows = lrs[i]->getDcl()->getNumRows();
      unsigned short metric = -1;
      // NOTE: Add 1 to degree to avoid divide-by-0, as a live range may have no
      // neighbors
      if (irb->kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D) {
        if (useSplitLLRHeuristic) {
          metric = 1;
          spillCost =
              (float)refFreqDouble * refCount / degree; // 1.0f * refCount / degree;
        } else {
          vASSERT(lrs[i]->getDcl()->getTotalElems() > 0);
          if (!liveAnalysis->livenessClass(G4_GRF) || !useNewSpillCost) {
            metric = 2;
            // address or flag variables
            spillCost = (float)std::sqrt(refFreqDouble) * refCount * refCount *
                        byteSize * // 1.0f * refCount * refCount * byteSize *
                        (float)sqrt(byteSize) /
                        ((float)sqrt(degree) * (float)(sqrt(sqrt(numRows))));
          } else {
            metric = 3;
            refCount = getWeightedRefCount(lrs[i]->getDcl());
            refFreqDouble = getWeightedStaticFreq(lrs[i]->getDcl());
            spillCost = (float)refFreqDouble * refCount * refCount *
                        refCount / // 1.0f * refCount * refCount * refCount /
                        ((float)degree * (float)degree);
          }
        }
      } else {
        if (!useNewSpillCost) {
          metric = 4;
          spillCost = liveAnalysis->livenessClass(G4_GRF)
                          ? lrs[i]->getDegree()
                          : (float)refFreqDouble * refCount * refCount /
                                degree; // 1.0f * refCount * refCount / degree;
        } else {
          metric = 5;
          refCount = getWeightedRefCount(lrs[i]->getDcl());
          refFreqDouble = getWeightedStaticFreq(lrs[i]->getDcl());
          spillCost = (float)refFreqDouble * refCount * refCount *
                      refCount / // 1.0f * refCount * refCount * refCount /
                      ((float)degree * (float)degree);
        }
      }
      setFreqSpillCost(lr, spillCost);

      if (willDumpSpillCostAnalysis()) {
        std::cerr << "Spill cost analysis - ";
        lr->emit(std::cerr);
        std::cerr << " Metric: " << metric;
        std::cerr << " Ref Cnt: " << lr->getRefCount();
        std::cerr << " Static Ref Cnt: " << getStaticRefCnt(lr);
        std::cerr << " Ref Freq: " << getRefFreq(lr).toString();
        std::cerr << " Freq Spill cost: " << spillCost << "\n";
      }

      // Track address sensitive live range.
      if (liveAnalysis->isAddressSensitive(i) && incSpillCostCandidate(lr)) {
        addressSensitiveVars.push_back(lr);
      } else {
        // Set the spill cost of all other normal live ranges, and
        // track the max normal cost.
        if (maxNormalFreqCost < spillCost)
          maxNormalFreqCost = spillCost;
      }
    }
    if (willDumpSpillCostAnalysis()) {
      std::cerr << "\n";
    }
  }

  //
  // Set the spill cost of address sensitive live ranges above all the
  // normal live ranges, so that they get colored before all the normal
  // live ranges.
  //
  for (LiveRange* asv : addressSensitiveVars) {
    float fsc = getFreqSpillCost(asv);
    if (fsc != MAXSPILLCOST) {
      fsc += maxNormalFreqCost;
      setFreqSpillCost(asv, fsc);
    }
  }
}

void FrequencyInfo::sortBasedOnFreq(std::vector<LiveRange*>& lrs)
{
  if (willDumpColoringOrder()) {
    std::cerr << "Sort based on freq - Kernel " << kernel.getName() << std::endl;
    for (auto lr : lrs) {
      std::cerr << "Sort based on freq - (Legacy) ";
      lr->emit(std::cerr);
      std::cerr << " Ref Cnt: " << lr->getRefCount();
      std::cerr << "\n";
    }
    std::cerr << std::endl;
  }

  if (!isFreqBasedSpillSelectionEnabled()) {
    return;
  }
  std::sort(lrs.begin(), lrs.end(), [&](LiveRange* lr1, LiveRange* lr2) {
    float sp1 = getFreqSpillCost(lr1);
    float sp2 = getFreqSpillCost(lr2);
    return sp1 < sp2 ||
      (sp1 == sp2 &&
        lr1->getVar()->getId() < lr2->getVar()->getId());
    }
  );

  if (willDumpColoringOrder()) {

    for (auto lr : lrs) {
        std::cerr << "Sort based on freq - (Frequency) ";
        lr->emit(std::cerr);
        std::cerr << " Ref Cnt: " << lr->getRefCount();
        std::cerr << " Ref Freq: " << getRefFreq(lr).toString();
        std::cerr << " Freq Spill cost: "
                  << getFreqSpillCost(lr) << "\n";
    }
    std::cerr << std::endl;
  }

  return;
}

bool FrequencyInfo::hasFreqMetaData(G4_INST* i) {
  MDNode* md_digits = i->getMetadata("stats.blockFrequency.digits");
  MDNode* md_scale = i->getMetadata("stats.blockFrequency.scale");
  return md_digits && md_scale && (md_digits->asMDString()->getData() != "") &&
    (md_scale->asMDString()->getData() != "");
}

Scaled64 FrequencyInfo::getFreqInfoFromInst(G4_INST *inst) {
  if (tailInsts.find(inst) == tailInsts.end()) {
    if (willDumpNoFreqReport()) {
      std::cerr << "The instruction doesn't have a registered tail "
                   "instruction, possible generateed after encoding\n";
    }
    return Scaled64::getZero(); // return 0 freqeuncy for unknown frequency
  }

  G4_INST *lastInst = tailInsts[inst];
  vISA_ASSERT(InstFreqInfo.find(lastInst) != InstFreqInfo.end(),
              "last instruction should have frequency data");
  return InstFreqInfo[lastInst];
}

Scaled64 FrequencyInfo::getBlockFreqInfo(G4_BB *bb) {
  if (BlockFreqInfo.find(bb) == BlockFreqInfo.end()) {
    if (willDumpNoFreqReport()) {
      std::cerr << "The basicblock doesn't have frequency information, "
                   "possibly generated after flow graph construction\n";
      bb->dump();
    }
    return Scaled64::getZero(); // return 0 freqeuncy for unknown frequency
  }
  return BlockFreqInfo[bb];
};

void FrequencyInfo::deriveRefFreq(G4_BB *bb) {
  Scaled64 blockFreq = getBlockFreqInfo(bb);
  std::vector<LiveRange *> &lrs = liveAnalysis->gra.incRA.getLRs();
  bool incSpillCostAddrTaken = kernel.getOption(vISA_IncSpillCostAllAddrTaken);

  for (auto i = bb->rbegin(); i != bb->rend(); i++) {
    G4_INST *inst = (*i);
    auto dst = inst->getDst();
    if (dst) {
      if (dst->getBase()->isRegAllocPartaker()) {
        unsigned id = dst->getBase()->asRegVar()->getId();
        LiveRange * lr = lrs[id];
        if (!inst->isPseudoKill() && !inst->isLifeTimeEnd()) {
          addupRefFreq(lr, blockFreq); //Update reference frequency
        }
      } else if (dst->isIndirect() && liveAnalysis->livenessClass(G4_GRF)) {
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(dst,
                                                                          bb);
        for (auto &pt : pointsToSet) {
          if (!pt.var->isRegAllocPartaker() || !incSpillCostAddrTaken)
            continue;
          unsigned id = pt.var->getId();
          LiveRange *lr = lrs[id];
          addupRefFreq(lr, blockFreq);
        }
      }
    }

    if (inst->opcode() == G4_pseudo_fcall &&
        liveAnalysis->livenessClass(G4_GRF)) {
      auto fcall = kernel.fg.builder->getFcallInfo(bb->back());
      G4_Declare *ret = kernel.fg.builder->getStackCallRet();
      vISA_ASSERT(fcall != std::nullopt, "fcall info not found");
      uint16_t retSize = fcall->getRetSize();
      if (ret && retSize > 0 && ret->getRegVar() &&
          ret->getRegVar()->isRegAllocPartaker()) {
        unsigned id = ret->getRegVar()->getId();
        LiveRange *lr = lrs[id];
        addupRefFreq(lr, blockFreq);
      }
    }

    //
    // process each source operand
    //
    for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = inst->getSrc(j);
      if (!src || !src->isSrcRegRegion())
        continue;

      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      if (srcRegion->getBase()->isRegAllocPartaker()) {
        unsigned id = srcRegion->getBase()->asRegVar()->getId();
        LiveRange *lr = lrs[id];
        addupRefFreq(lr, blockFreq);
      } else if (srcRegion->isIndirect() &&
                 liveAnalysis->livenessClass(G4_GRF)) {
        // make every var in points-to set live
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                srcRegion, bb);
        for (auto &pt : pointsToSet) {
          if (!pt.var->isRegAllocPartaker() || !incSpillCostAddrTaken)
            continue;
          unsigned id = pt.var->getId();
          LiveRange *lr = lrs[id];
          addupRefFreq(lr, blockFreq);
        }
      }
    }

    //
    // Process condMod
    //
    if (auto mod = inst->getCondMod()) {
      G4_VarBase *flagReg = mod->getBase();
      if (flagReg) {
        if (flagReg->asRegVar()->isRegAllocPartaker()) {
          unsigned id = flagReg->asRegVar()->getId();
          LiveRange *lr = lrs[id];
          addupRefFreq(lr, blockFreq);
        }
      } else {
        vISA_ASSERT((inst->opcode() == G4_sel || inst->opcode() == G4_csel) &&
                        inst->getCondMod() != NULL,
                    "Invalid CondMod");
      }
    }

    //
    // Process predicate
    //
    if (auto predicate = inst->getPredicate()) {
      G4_VarBase *flagReg = predicate->getBase();
      if (flagReg->asRegVar()->isRegAllocPartaker()) {
        unsigned id = flagReg->asRegVar()->getId();
        LiveRange *lr = lrs[id];
        addupRefFreq(lr, blockFreq);
      }
    }
  }
  return;
}
void FrequencyInfo::initForRegAlloc(LivenessAnalysis  *l) {
  freqSpillCosts.clear();
  refFreqs.clear();
  liveAnalysis = l;
  if (!isSpillCostComputationEnabled())
    return;
  for (auto bb : kernel.fg)
    deriveRefFreq(bb);
  return;
}

} // namespace vISA
