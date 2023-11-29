/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SplitAlignedScalars.h"
#include "G4_Opcode.h"

using namespace vISA;

bool SplitAlignedScalars::canReplaceDst(G4_INST *inst) {
  // acc has special rules, so skip optimizing instructions using acc
  if (inst->useAcc())
    return false;

  // dst must be 64 bit aligned for ternary instructions on certain platforms
  if (inst->getBuilder().kernel.getPlatform() < Xe_XeHPSDV &&
      inst->getNumSrc() == 3)
    return false;

  // whitelist of supported instructions
  if (inst->isMov() || inst->isMath() || inst->isArithmetic() ||
      inst->isLogic() || inst->isCompare() || inst->isPseudoKill()) {
    return true;
  }

  return false;
}

bool SplitAlignedScalars::canReplaceSrc(G4_INST *inst, unsigned int idx) {
  auto opcode = inst->opcode();

  if (inst->useAcc())
    return false;

  if (inst->isMov() || inst->isMath() || inst->isArithmetic() ||
      inst->isLogic() || inst->isCompare() || opcode == G4_mad ||
      inst->isPseudoUse()) {
    return true;
  }

  return false;
}

std::vector<G4_Declare *> SplitAlignedScalars::gatherCandidates() {
  std::vector<G4_Declare *> candidates;

  unsigned int lexId = 0;
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      inst->setLexicalId(lexId++);

      auto dst = inst->getDst();
      if (dst && dst->isDstRegRegion()) {
        auto dstTopDcl = dst->getTopDcl();
        if (dstTopDcl && isDclCandidate(dstTopDcl)) {
          auto &Data = dclData[dstTopDcl];
          Data.defs.push_back(std::pair(bb, inst));
          if (Data.firstDef == 0)
            Data.firstDef = lexId;

          // disallow cases where scalar def has predicate
          if (inst->getPredicate()) {
            Data.allowed = false;
          }

          // Disallow splitting any variable if it's used as send
          // dst because adding a copy after send causes a back-to-back
          // dependency. If scheduler cannot hide it, it can lead to
          // performance penalty.
          if (inst->isSend()) {
            Data.allowed = false;
          }

          if ((dst->getTypeSize() != dstTopDcl->getByteSize()) &&
              !(dst->getTypeSize() == 4 && dstTopDcl->getByteSize() == 8)) {
            // Disallow case where dst opnd does not write complete topdcl,
            // except QW opnd which may be A64 address computed separately
            // with low/hi DW opnds
            Data.allowed = false;
          }

          if (dst->getTypeSize() < 4) {
            // byte, word, half floats may have many hw restrictions
            // and these are usually not set as GRF aligned
            Data.allowed = false;
          }

          auto dstDcl =
              dst->asDstRegRegion()->getBase()->asRegVar()->getDeclare();
          if (dstDcl->getAliasDeclare()) {
            // Disallow case where dst alias dcl size is different from
            // top dcl, except QW opnd which may be A64 address computed
            // separately with low/hi DW opnds
            if ((dstDcl->getByteSize() != dstTopDcl->getByteSize()) &&
                !(dstDcl->getByteSize() == 4 && dstTopDcl->getByteSize() == 8))
              Data.allowed = false;
          }

          if (dst->getHorzStride() != 1) {
            // dst hstride != 1 to accommodate some alignment restriction
            Data.allowed = false;
          }

          if (!inst->isSend()) {
            if (inst->getExecSize() != 1) {
              Data.allowed = false;
            }
            // check whether dst type size != src type size
            // disallow optimization if dst type is different
            // than src as that entails alignmment requirements
            // that this pass may not honor.
            for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
              auto src = inst->getSrc(i);
              if (!src->isSrcRegRegion())
                continue;
              if (dst->getTypeSize() != src->getTypeSize())
                Data.allowed = false;
            }
          }
        }
      }

      for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
        auto src = inst->getSrc(i);
        if (src && src->isSrcRegRegion()) {
          auto srcTopDcl = src->getTopDcl();
          if (srcTopDcl && isDclCandidate(srcTopDcl)) {
            auto &Data = dclData[srcTopDcl];
            Data.uses.push_back(std::make_tuple(bb, inst, i));
            if (Data.lastUse < lexId)
              Data.lastUse = lexId;

            if (!inst->isSend()) {
              if (!(src->asSrcRegRegion()->isScalar())) {
                Data.allowed = false;
              }

              // mixed types have alignment requirements
              if (dst->getTypeSize() != src->getTypeSize())
                Data.allowed = false;
            }
          }
        }
      }
    }
  }

  for (auto dcl : kernel.Declares) {
    if (dcl != dcl->getRootDeclare())
      continue;
    auto dclDataIt = dclData.find(dcl);
    if (dclDataIt != dclData.end()) {
      if ((*dclDataIt).second.allowed)
        candidates.push_back(dcl);
    }
  }

  return candidates;
}

// return number of movs needed when applying optimization on dcl
// this method returns estimate of dynamic movs by considering
// loops. each loop nest N add is expected to run const# of iterations.
unsigned int SplitAlignedScalars::computeNumMovs(G4_Declare *dcl) {
  unsigned int numMovsNeeded = 0;
  auto &Data = dclData[dcl];
  for (const auto &def : Data.defs) {
    if (!canReplaceDst(def.second)) {
      auto bb = def.first;
      auto innerMostLoop = kernel.fg.getLoops().getInnerMostLoop(bb);
      if (innerMostLoop)
        numMovsNeeded +=
            innerMostLoop->getNestingLevel() * EstimatedLoopTripCount;
      else
        numMovsNeeded++;
    }
  }
  for (const auto &use : Data.uses) {
    if (!canReplaceSrc(std::get<1>(use), std::get<2>(use))) {
      auto bb = std::get<0>(use);
      auto innerMostLoop = kernel.fg.getLoops().getInnerMostLoop(bb);
      if (innerMostLoop)
        numMovsNeeded +=
            innerMostLoop->getNestingLevel() * EstimatedLoopTripCount;
      else
        numMovsNeeded++;
    }
  }

  return numMovsNeeded;
}

void SplitAlignedScalars::pruneCandidates(
    std::vector<G4_Declare *> &candidates) {
  // This method is a cost function that decides which aligned scalar variables
  // from candidates list get split. This method returns list of final
  // candidates in passed-by-ref argument. We first sort candidates in
  // descending order of spill cost. Next, we determine number of movs needed if
  // a given candidate is split. This is an estimate that takes in to account
  // loop nesting levels. If accumulated mov count is below a threshold then
  // splitting is allowed. Note that we always split a candidate if it was
  // spilled by current RA iteration or if the variable was marked as callee
  // saved (in case it crossed fcall boundary).

  auto compareSpillCost = [&](G4_Declare *dcl1, G4_Declare *dcl2) {
    auto it1 = dclSpillCost.find(dcl1);
    auto it2 = dclSpillCost.find(dcl2);

    float cost1 = 0, cost2 = 0;
    if (it1 != dclSpillCost.end())
      cost1 = (*it1).second;
    if (it2 != dclSpillCost.end())
      cost2 = (*it2).second;

    return cost1 < cost2;
  };

  std::list<G4_Declare *> candidateList;
  std::for_each(candidates.begin(), candidates.end(),
                [&](G4_Declare *dcl) { candidateList.push_back(dcl); });

  // First re-order candidates based on spill cost in ascending order
  candidateList.sort(compareSpillCost);

  for (auto it = candidateList.begin(); it != candidateList.end();) {
    auto dcl = *it;
    auto dclDataIt = dclData.find(dcl);
    bool erase = true;
    if (dclDataIt != dclData.end()) {
      auto &Data = dclData[dcl];
      if (heuristic(*it, Data))
        erase = false;
    }
    if (erase) {
      it = candidateList.erase(it);
      continue;
    }
    ++it;
  }

  auto estimateInstCount = [&]() {
    auto instCount = 0;
    for (auto bb : kernel.fg.getBBList()) {
      auto loop = kernel.fg.getLoops().getInnerMostLoop(bb);
      if (loop)
        instCount +=
            (loop->getNestingLevel() * EstimatedLoopTripCount) * bb->size();
      else
        instCount += bb->size();
    }
    return instCount;
  };

  candidates.clear();
  unsigned int totalMovsNeeded = 0;
  unsigned int estimatedInstCount = estimateInstCount();

  for (auto ci = candidateList.rbegin(); ci != candidateList.rend(); ++ci) {
    const auto &candidate = *ci;
    bool isCandidate = false;
    auto numMovsNeeded = computeNumMovs(candidate);

    // Allow any candidate that was marked as spill or if the candidate needs no
    // movs or if candidate was marked as requiring callee save allocation.
    if (spilledDclSet.find(candidate) != spilledDclSet.end() ||
        calleeSaveBiased.find(candidate) != calleeSaveBiased.end() ||
        numMovsNeeded == 0)
      isCandidate = true;
    else {
      // Mark as candidate for splitting only if doing so doesnt increase mov
      // count above a set threshold.
      if ((totalMovsNeeded + numMovsNeeded) <
          (unsigned int)(((float)estimatedInstCount * BloatAllowed)))
        isCandidate = true;
    }

    if (isCandidate) {
      candidates.push_back(candidate);
      totalMovsNeeded += numMovsNeeded;
    }
  }
}

bool SplitAlignedScalars::isDclCandidate(G4_Declare *dcl) {
  if (dcl->getRegFile() == G4_RegFileKind::G4_GRF &&
      (dcl->getNumElems() == 1 ||
       (dcl->getNumElems() == 2 && dcl->getElemType() == Type_D)) &&
      !dcl->getAddressed() && !dcl->getIsPartialDcl() &&
      !dcl->getRegVar()->isPhyRegAssigned() && !dcl->getAliasDeclare() &&
      !dcl->isInput() && !dcl->isOutput() && !dcl->isPayloadLiveOut() &&
      !dcl->isDoNotSpill() && gra.getSubRegAlign(dcl) == kernel.getGRFAlign())
    return true;
  return false;
}

bool SplitAlignedScalars::heuristic(G4_Declare *dcl, Data &d) {
  if (d.getDUMaxDist() < MinOptDist)
    return false;

  if (!d.allowed) {
    return false;
  }

  return true;
}

// T is either G4_SrcRegRegion or G4_DstRegRegion
template <class T>
G4_Declare *SplitAlignedScalars::getDclForRgn(T *rgn, G4_Declare *newTopDcl) {
  // newTopDcl is the replacement dcl computed by this pass to replace rgn
  // If rgn's dcl is not aliased then return newTopDcl as new rgn can
  // directly use this.
  // If rgn's dcl is aliased, create a new alias dcl based off newTopDcl
  // with correct offset and return it.
  auto rgnDcl = rgn->getBase()->asRegVar()->getDeclare();

  if (rgn->getTopDcl() == rgnDcl)
    return newTopDcl;

  auto newAliasDcl = kernel.fg.builder->createTempVar(
      rgnDcl->getNumElems(), rgnDcl->getElemType(), rgnDcl->getSubRegAlign());
  newAliasDcl->setAliasDeclare(newTopDcl, rgnDcl->getOffsetFromBase());

  return newAliasDcl;
}

void SplitAlignedScalars::run() {
  auto getNewDcl = [&](G4_Declare *oldDcl) {
    auto newTopDcl = oldNewDcls[oldDcl];
    if (newTopDcl == nullptr) {
      auto builder = kernel.fg.builder;
      auto numElems = oldDcl->getNumElems();
      auto elemType = oldDcl->getElemType();
      auto subAlign = Get_G4_SubRegAlign_From_Size(
          numElems * TypeSize(elemType), builder->getPlatform(),
          builder->getGRFAlign());
      newTopDcl = builder->createTempVar(numElems, elemType, subAlign);
      oldNewDcls[oldDcl] = newTopDcl;
    }
    return newTopDcl;
  };

  auto getTypeExecSize = [&](G4_Type type, bool isScalar, G4_ExecSize execSize) {
    if (TypeSize(type) < 8) {
      if (execSize == g4::SIMD1 || isScalar)
        return std::make_tuple(1, type);
      else {
        vISA_ASSERT(execSize == g4::SIMD2, "invalid execution size");
        return std::make_tuple(2, type);
      }
    }

    if (kernel.fg.builder->noInt64())
      return std::make_tuple(2, Type_UD);

    return std::make_tuple(1, Type_UQ);
  };

  auto candidates = gatherCandidates();

  pruneCandidates(candidates);

  for (auto dcl : candidates) {
    auto dclDataIt = dclData.find(dcl);
    if (dclDataIt != dclData.end()) {
      oldNewDcls[dcl] = nullptr;
      numDclsReplaced++;
    }
    gra.incRA.markForIntfUpdate(dcl);
  }

  for (auto bb : kernel.fg) {
    for (auto instIt = bb->begin(), instItEnd = bb->end();
         instIt != instItEnd;) {
      auto newInstIt = instIt;
      auto inst = (*instIt);

      auto dst = inst->getDst();
      if (dst && oldNewDcls.find(dst->getTopDcl()) != oldNewDcls.end()) {
        auto oldTopDcl = dst->getTopDcl();
        auto newTopDcl = getNewDcl(oldTopDcl);
        auto newDcl = getDclForRgn(dst, newTopDcl);

        if (canReplaceDst(inst)) {
          auto newDstRgn = kernel.fg.builder->createDst(
              newDcl->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
              dst->getHorzStride(), dst->getType());
          inst->setDest(newDstRgn);
        } else {
          // found an instruction where dst has to be GRF aligned,
          // so we keep old dst but we insert a copy in new dcl
          auto newAlignedTmpTopDcl = kernel.fg.builder->createTempVar(
              oldTopDcl->getNumElems(), oldTopDcl->getElemType(),
              oldTopDcl->getSubRegAlign());
          newAlignedTmpTopDcl->copyAlign(oldTopDcl);
          auto newAlignedVar = getDclForRgn(dst, newAlignedTmpTopDcl);
          auto dstRgn = kernel.fg.builder->createDst(
              newAlignedVar->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
              dst->getHorzStride(), dst->getType());
          inst->setDest(dstRgn);

          // emit copy to store data to original non-aligned scalar
          unsigned int execSize = 1;
          auto oldExecSize = inst->getExecSize();
          G4_Type typeToUse = Type_UD;
          std::tie(execSize, typeToUse) =
              getTypeExecSize(dst->getType(), false, oldExecSize);

          auto src = kernel.fg.builder->createSrc(
              dstRgn->getBase(), dstRgn->getRegOff(), dstRgn->getSubRegOff(),
              execSize == g4::SIMD1 ? kernel.fg.builder->getRegionScalar()
                                    : kernel.fg.builder->getRegionStride1(),
              typeToUse);
          auto dstRgnOfCopy = kernel.fg.builder->createDst(
              newDcl->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
              dst->getHorzStride(), typeToUse);

          G4_Predicate *dupPred = nullptr;
          if (inst->getPredicate())
            dupPred = kernel.fg.builder->createPredicate(*inst->getPredicate());
          auto newInst = kernel.fg.builder->createInternalInst(
              dupPred, G4_mov, nullptr, g4::NOSAT, G4_ExecSize(execSize),
              dstRgnOfCopy, src, nullptr, InstOpt_WriteEnable);
          newInstIt = bb->insertAfter(instIt, newInst);

          if (gra.EUFusionNoMaskWANeeded()) {
            gra.addEUFusionNoMaskWAInst(bb, newInst);
          }

          gra.addNoRemat(newInst);

          numMovsAdded++;
        }
        changesMade = true;
      }

      for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
        auto src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion())
          continue;

        auto srcRgn = src->asSrcRegRegion();

        if (oldNewDcls.find(srcRgn->getTopDcl()) != oldNewDcls.end()) {
          auto oldTopDcl = srcRgn->getTopDcl();
          auto newTopDcl = getNewDcl(oldTopDcl);
          auto newDcl = getDclForRgn(srcRgn, newTopDcl);

          if (canReplaceSrc(inst, i)) {
            auto newSrcRgn = kernel.fg.builder->createSrcRegRegion(
                srcRgn->getModifier(), srcRgn->getRegAccess(),
                newDcl->getRegVar(), srcRgn->getRegOff(),
                srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
            inst->setSrc(newSrcRgn, i);
          } else {
            // create a new aligned tmp
            auto newAlignedTmpTopDcl = kernel.fg.builder->createTempVar(
                oldTopDcl->getNumElems(), oldTopDcl->getElemType(),
                oldTopDcl->getSubRegAlign());
            newAlignedTmpTopDcl->copyAlign(oldTopDcl);

            unsigned int execSize = 1;
            auto oldExecSize = inst->getExecSize();
            G4_Type typeToUse = Type_UD;
            G4_Type srcType = srcRgn->getType();
            bool isScalar = srcRgn->isScalar();
            if (inst->isSend() && isScalar && oldTopDcl->getNumElems() == 2 &&
                TypeSize(oldTopDcl->getElemType()) == 4) {
              // For SIMD1 send with :d type data payload (e.g., d32x2t),
              // create a move to copy both dwords when replacing source
              srcType = Type_UQ;
            }
            std::tie(execSize, typeToUse) =
                getTypeExecSize(srcType, isScalar, oldExecSize);

            // copy oldDcl in to newAlignedTmpTopDcl
            auto tmpDst = kernel.fg.builder->createDst(
                newAlignedTmpTopDcl->getRegVar(), typeToUse);
            auto src = kernel.fg.builder->createSrc(
                newTopDcl->getRegVar(), 0, 0,
                execSize == 1 ? kernel.fg.builder->getRegionScalar()
                              : kernel.fg.builder->getRegionStride1(),
                typeToUse);
            auto copy = kernel.fg.builder->createMov(
                G4_ExecSize(execSize), tmpDst, src, InstOpt_WriteEnable, false);

            // now create src out of tmpDst
            auto dclToUse = getDclForRgn(srcRgn, newAlignedTmpTopDcl);

            auto newAlignedSrc = kernel.fg.builder->createSrcRegRegion(
                srcRgn->getModifier(), srcRgn->getRegAccess(),
                dclToUse->getRegVar(), srcRgn->getRegOff(),
                srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
            inst->setSrc(newAlignedSrc, i);
            bb->insertBefore(instIt, copy);

            if (gra.EUFusionNoMaskWANeeded()) {
              gra.addEUFusionNoMaskWAInst(bb, copy);
            }

            // this copy shouldnt be rematerialized
            gra.addNoRemat(copy);

            numMovsAdded++;
          }
          changesMade = true;
        }
      }

      instIt = ++newInstIt;
    }
  }
}

void SplitAlignedScalars::dump(std::ostream &os) {
  os << "# GRF aligned scalar dcls replaced: " << numDclsReplaced << "\n";
  os << "# movs added: " << numMovsAdded << "\n";
}
