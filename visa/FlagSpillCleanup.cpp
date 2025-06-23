/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//
// Flag spill cleanup pass.
//
#include "FlagSpillCleanup.h"

using namespace vISA;

#define SPILL_MEMORY_OVERLAP(x, y)                                             \
  (!(x->leftOff > y->rightOff || y->leftOff > x->rightOff))

#define SPILL_MEMORY_OVERWRITE(target_memory, overwrite_memory)                \
  (target_memory->leftOff >= overwrite_memory->leftOff &&                      \
   overwrite_memory->rightOff >= target_memory->rightOff)

#define IS_FLAG_MOVE(inst)                                                     \
  (inst->opcode() == G4_mov && (inst->getDst() && inst->getSrc(0)) &&          \
   (inst->getDst()->getTopDcl() && inst->getSrc(0)->getTopDcl()) &&            \
   ((inst->getDst()->getTopDcl()->getRegFile() == G4_FLAG &&                   \
     inst->getSrc(0)->getTopDcl()->getRegFile() == G4_GRF) ||                  \
    (inst->getDst()->getTopDcl()->getRegFile() == G4_GRF &&                    \
     inst->getSrc(0)->getTopDcl()->getRegFile() == G4_FLAG)))

#define IS_SPILL_KILL_CANDIDATE(preScratchAccess)                              \
  (preScratchAccess->isSpill && !preScratchAccess->fillInUse)

#define IS_USE_KILL_CANDIDATE(preScratchAccess)                                \
  (!(preScratchAccess->regKilled || preScratchAccess->regPartialKilled ||      \
     preScratchAccess->scratchDefined))

#define IS_GRF_RANGE_OVERLAP(s1, e1, sa)                                       \
  (e1 >= sa->linearizedStart && sa->linearizedEnd >= s1)

#define IS_SCRATCH_RANGE_OVERLAP(s1, e1, sa)                                   \
  (!(e1 < sa->leftOff || sa->rightOff < s1))

#define IS_MERGEABLE_SCRATCH_RANGES(r1, r2)                                    \
  (!(((int)r1.leftOff - (int)r2.rightOff) > 1 ||                               \
     ((int)r2.leftOff - (int)r1.rightOff) > 1))

#define IS_MERGEABLE_GRF_RANGES(r1, r2)                                        \
  (!(((int)r1.linearizedStart - (int)r2.linearizedEnd) > 1 ||                  \
     ((int)r2.linearizedStart - (int)r1.linearizedEnd) > 1))

#define IS_GRF_RANGE_OVERWRITE(sa, s1, e1)                                     \
  (s1 <= sa->linearizedStart && sa->linearizedEnd <= e1)

#define IS_SCRATCH_RANGE_OVERWRITE(sa, s1, e1)                                 \
  (s1 <= sa->leftOff && sa->rightOff <= e1)

#define IS_FLAG_RANGE_OVERLAP(s1, e1, sa)                                      \
  (!(e1 < sa->linearizedStart || sa->linearizedEnd < s1))

#define IS_FLAG_RANGE_OVERWRITE(t, s, e)                                       \
  ((s <= t->linearizedStart && t->linearizedEnd <= e))

void FlagSpillCleanup::FlagLineraizedStartAndEnd(G4_Declare *topdcl,
                                                 unsigned &linearizedStart,
                                                 unsigned &linearizedEnd) {
  const G4_Areg *areg = topdcl->getRegVar()->getPhyReg()->asAreg();
  linearizedStart = areg->getFlagNum() * 4;
  linearizedStart +=
      topdcl->getRegVar()->getPhyRegOff() * topdcl->getElemSize();
  linearizedEnd = linearizedStart + topdcl->getByteSize();
  return;
}

/*
 * Reuse previous register
 */
bool FlagSpillCleanup::replaceWithPreDcl(IR_Builder &builder,
                                         SCRATCH_ACCESS *scratchAccess,
                                         SCRATCH_ACCESS *preScratchAccess) {
  int preRegOff = 0;
  int payloadHeaderSize = 0;
  G4_Operand *reuseOpnd = NULL;
  G4_INST *preInst = *preScratchAccess->inst_it;

  // Get reuse operand
  if (preScratchAccess->isSpill) {
    reuseOpnd = preInst->getSrc(0);
    preRegOff = reuseOpnd->asSrcRegRegion()->getSubRegOff();
    reuseOpnd = preInst->getSrc(0);
  } else {
    reuseOpnd = preInst->getDst();
    preRegOff = reuseOpnd->asDstRegRegion()
                    ->getSubRegOff(); // For flag register, only subRegOff
  }
  G4_Declare *dcl = reuseOpnd->getBase()->asRegVar()->getDeclare();

  if (builder.WaDisableSendSrcDstOverlap()) {
    for (auto &renameOpnd : scratchAccess->renameOperandVec) {
      if (renameOpnd.second < -1) // Flag
      {
        break;
      }

      G4_INST *inst = renameOpnd.first;

      if (renameOpnd.second >= 0) {
        if (inst->isSend() && !inst->getDst()->isNullReg()) {
          G4_DstRegRegion *dst = inst->getDst();
          bool noOverlap =
              dst->getLinearizedEnd() < preScratchAccess->linearizedStart ||
              preScratchAccess->linearizedEnd < dst->getLinearizedStart();
          if (!noOverlap) {
            return false;
          }
        }
      }
    }
  }

  // Replace the declare for all operands assciated with this scratch fill.
  for (auto &renameOpnd : scratchAccess->renameOperandVec) {
    G4_INST *inst = renameOpnd.first;

    if (renameOpnd.second == -3) // Flag modifier
    {
      G4_CondMod *mod = inst->getCondMod();
      int regOff = preRegOff;
      G4_CondMod *mod_Opnd = builder.createCondMod(
          mod->getMod(), dcl->getRegVar(), (unsigned short)regOff);

      inst->setCondMod(mod_Opnd);

    } else if (renameOpnd.second == -2) // Flag predicate
    {
      G4_Predicate *predicate = inst->getPredicate();
      int regOff = preRegOff;
      G4_Predicate *pred_Opnd = builder.createPredicate(
          predicate->getState(), dcl->getRegVar(), (unsigned short)regOff,
          predicate->getControl());

      inst->setPredicate(pred_Opnd);
    } else if (renameOpnd.second == -1) // GRF dst
    {
      G4_DstRegRegion *orgDstRegion = inst->getDst();
      int regOff = preRegOff +
                   (scratchAccess->leftOff - preScratchAccess->leftOff) /
                       gra.kernel.numEltPerGRF<Type_UB>() +
                   payloadHeaderSize / gra.kernel.numEltPerGRF<Type_UB>();
      G4_DstRegRegion *dstOpnd = builder.createDst(
          dcl->getRegVar(), (short)regOff, orgDstRegion->getSubRegOff(),
          orgDstRegion->getHorzStride(), orgDstRegion->getType());
      inst->setDest(dstOpnd);
    } else // GRF src
    {
      G4_Operand *opnd = inst->getSrc(renameOpnd.second);
      G4_SrcRegRegion *orgSrcRegion = opnd->asSrcRegRegion();

      int regOff = preRegOff +
                   (scratchAccess->leftOff - preScratchAccess->leftOff) /
                       gra.kernel.numEltPerGRF<Type_UB>() +
                   payloadHeaderSize / gra.kernel.numEltPerGRF<Type_UB>();
      G4_Operand *srcOpnd = builder.createSrcRegRegion(
          orgSrcRegion->getModifier(), orgSrcRegion->getRegAccess(),
          dcl->getRegVar(), (short)regOff, orgSrcRegion->getSubRegOff(),
          orgSrcRegion->getRegion(), orgSrcRegion->getType());

      inst->setSrc(srcOpnd, renameOpnd.second);
    }
  }

  return true;
}

/*
 *  1) The reuse target register in pre scratch access may be partial killed,
 *  2) and the corresponding scracth memory range is overlap with the memory of
 * current scratch access. In both cases, the current fill can not be removed
 */
bool FlagSpillCleanup::scratchKilledByPartial(
    SCRATCH_ACCESS *scratchAccess, SCRATCH_ACCESS *preScratchAccess) {
  bool killed = false;

  for (auto &range : preScratchAccess->killedScratchRange) {
    if (!(scratchAccess->leftOff > range.rightOff ||
          range.leftOff > scratchAccess->rightOff)) {
      killed = true;
    }
  }

  for (auto &range : preScratchAccess->killedRegRange) {
    // Map the register kill to scratch kill
    unsigned leftOff =
        preScratchAccess->leftOff +
        (range.linearizedStart - preScratchAccess->linearizedStart);
    unsigned rightOff =
        preScratchAccess->leftOff +
        (range.linearizedEnd - preScratchAccess->linearizedStart);

    if (!(scratchAccess->leftOff > rightOff ||
          leftOff > scratchAccess->rightOff)) {
      killed = true;
    }
  }

  return killed;
}

/*
 *  Record all killed GRF ranges.
 *  do merging of ranges when possible.
 */
bool FlagSpillCleanup::addKilledGRFRanges(unsigned linearizedStart,
                                          unsigned linearizedEnd,
                                          SCRATCH_ACCESS *scratchAccess,
                                          G4_Predicate *predicate) {
  REG_RANGE range;
  range.linearizedStart =
      std::max(scratchAccess->linearizedStart, linearizedStart);
  range.linearizedEnd = std::min(scratchAccess->linearizedEnd, linearizedEnd);
  range.predicate = predicate ? true : false;

  if (scratchAccess->killedRegRange.size() == 0) {
    scratchAccess->killedRegRange.push_back(range);
  } else {
    bool merged = false;
    REG_RANGE_VEC_ITER range_iter = scratchAccess->killedRegRange.begin();
    REG_RANGE_VEC_ITER range_iter_next;
    REG_RANGE *merged_range = NULL;
    while (range_iter != scratchAccess->killedRegRange.end()) {
      REG_RANGE &killedRange = *(range_iter);
      range_iter_next = range_iter;
      range_iter_next++;

      if (killedRange.predicate) // With predicate, the range can not be merged
                                 // with others
      {
        range_iter = range_iter_next;
        continue;
      }

      if (!merged && IS_MERGEABLE_GRF_RANGES(range, killedRange)) {
        killedRange.linearizedStart =
            std::min(killedRange.linearizedStart, range.linearizedStart);
        killedRange.linearizedEnd =
            std::max(killedRange.linearizedEnd, range.linearizedEnd);
        merged = true;
        merged_range = &killedRange;
      } else if (merged) {
        if (IS_MERGEABLE_GRF_RANGES((*merged_range), killedRange)) {
          merged_range->linearizedStart = std::min(
              killedRange.linearizedStart, merged_range->linearizedStart);
          merged_range->linearizedEnd =
              std::max(killedRange.linearizedEnd, merged_range->linearizedEnd);
        }
      }
      if (IS_GRF_RANGE_OVERWRITE(scratchAccess, killedRange.linearizedStart,
                                 killedRange.linearizedEnd)) {
        scratchAccess->regKilled = true;
        return true;
      }
      range_iter = range_iter_next;
    }
    if (!merged) {
      scratchAccess->killedRegRange.push_back(range);
    }
  }

  return false;
}

/*
 * Check if the register in previous scratch access is fully killed by current
 * register define
 */
bool FlagSpillCleanup::regFullyKilled(SCRATCH_ACCESS *scratchAccess,
                                      unsigned linearizedStart,
                                      unsigned linearizedEnd,
                                      unsigned short maskFlag) {

  if (IS_FLAG_RANGE_OVERWRITE(scratchAccess, linearizedStart, linearizedEnd)) {
    if (maskFlag & InstOpt_WriteEnable) // No mask == all range killed
    {
      return true;
    }

    if (linearizedStart == scratchAccess->linearizedStart &&
        linearizedEnd == scratchAccess->linearizedEnd &&
        scratchAccess->maskFlag == maskFlag) {
      return true;
    }
  }

  return false;
}

/*
 *  Check only part of scratch register is killed, at the same time no overlap.
 *  This is to make sure if the associated fill is removed, the define register
 * can be replaced with reuse register or not.
 */
bool FlagSpillCleanup::inRangePartialKilled(SCRATCH_ACCESS *scratchAccess,
                                            unsigned linearizedStart,
                                            unsigned linearizedEnd,
                                            unsigned short maskFlag) {
  if ((scratchAccess->linearizedStart <= linearizedStart &&
       scratchAccess->linearizedEnd >= linearizedEnd)) {
    if (maskFlag & InstOpt_WriteEnable) {
      return true;
    }

    if (scratchAccess->linearizedStart == linearizedStart &&
        scratchAccess->linearizedEnd == linearizedEnd &&
        scratchAccess->maskFlag == maskFlag) {
      return true;
    }
  }

  return false;
}

/*
 * Register kill analysis
 */
bool FlagSpillCleanup::regDefineAnalysis(SCRATCH_ACCESS *scratchAccess,
                                         unsigned linearizedStart,
                                         unsigned linearizedEnd,
                                         unsigned short maskFlag,
                                         G4_Predicate *predicate) {
  if (regFullyKilled(scratchAccess, linearizedStart, linearizedEnd, maskFlag)) {
    return true;
  } else if (!scratchAccess->regKilled) {
    // Handle partial overlap
    // What about the mask?
    if (addKilledGRFRanges(linearizedStart, linearizedEnd, scratchAccess,
                           predicate)) {
      // The register range is killed by accumulated partial range kills
      return true;
    }
    scratchAccess->regPartialKilled = true;
  }

  return false;
}

void FlagSpillCleanup::regDefineFlag(SCRATCH_PTR_LIST *scratchTraceList,
                                     G4_INST *inst, G4_Operand *opnd) {
  // Get the linearized address in GRF register file
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  G4_Predicate *predicate = inst->getPredicate();
  G4_Declare *topdcl = opnd->getTopDcl();

  FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

  // Impact on previous scratch access
  SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

  if (it != itEnd && inst == *(scratchTraceList->back()->inst_it)) {
    itEnd--;
  }

  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *scratchAccess = *it;

    // Not instruction itself, def->use can not happen in single instruction.
    if (scratchAccess->regKilled) {
      it = kt;
      continue;
    }

    // Checked if the registers used in the previous scratch accesses (both
    // spill and fill) are killed (redefined).
    if (linearizedEnd &&
        IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess)) {
      // E mask
      unsigned maskFlag = (inst->getOption() & 0xFFF010C);

      if (regDefineAnalysis(scratchAccess, linearizedStart, linearizedEnd,
                            (unsigned short)maskFlag, predicate)) {
        // Fully killed
        scratchAccess->regKilled = true;
        if (scratchAccess->evicted) // Not in use
        {
          scratchTraceList->erase(
              it); // The previous one is not candidate for future use
        }
      }

      // For prefill and associated define and spill instructions
      // 1. Same dcl is used
      // 2. If the prefill register is fulled killed,
      //     a. The prefill instruction can be removed.
      //     b. But the define and instruction's registers are kept and will not
      //     reuse previous one.
      // 3. If the prefill register is partial killed, and the killed register
      // region is part of prefill region.
      //     a. The prefill instruction can be removed.
      //     b. and the register in define and spill instruction can reuse
      //     previous one.
      // 4. Otherwise, the (pre)fill instruction can not be removed, and no
      // reuse will happen.
      // 5. For pure fill, it's no killed by same declare
      G4_Declare *preDcl = scratchAccess->flagOpnd->getTopDcl();

      if (topdcl == preDcl) {
        if (inRangePartialKilled(scratchAccess, linearizedStart, linearizedEnd,
                                 (unsigned short)maskFlag)) {
          scratchAccess->renameOperandVec.emplace_back(inst, -1);
          scratchAccess->inRangePartialKilled = true;
        } else {
          scratchAccess->removeable = false;
        }
      }
    }

    it = kt;
  }
}

/*
 *  Analysis the use of register to determine if the scratchAccess can be
 * removed or not
 *
 */
bool FlagSpillCleanup::regUseAnalysis(SCRATCH_ACCESS *scratchAccess,
                                      unsigned linearizedStart,
                                      unsigned linearizedEnd) {
  // GRF in previous fill is used as part of current reg,
  // In this case, the fill can not be removed since the reuse can not happen.
  // Caller gauranteed the overlap of the registers
  if (linearizedEnd > scratchAccess->linearizedEnd ||
      linearizedStart < scratchAccess->linearizedStart) {
    return true;
  }

  // Can not be removed when the previous scratch access is killed or partial
  // killed before the use of current scratch access register
  // b
  SCRATCH_ACCESS *preScratchAccess = scratchAccess->preScratchAccess;
  if (preScratchAccess &&
      (preScratchAccess->regKilled ||
       scratchKilledByPartial(scratchAccess, preScratchAccess))) {
    return true;
  }

  // Back trace to update the reachable scratch accesses
  if (preScratchAccess && scratchAccess->prePreScratchAccess) {
    SCRATCH_ACCESS *prePreScratchAccess = preScratchAccess;
    preScratchAccess = scratchAccess;

    do {
      if ((prePreScratchAccess->regKilled ||
           scratchKilledByPartial(scratchAccess, prePreScratchAccess))) {
        scratchAccess->prePreScratchAccess = preScratchAccess;
        break;
      }
      preScratchAccess = prePreScratchAccess;
      prePreScratchAccess = preScratchAccess->preScratchAccess;
    } while (prePreScratchAccess &&
             preScratchAccess != scratchAccess->prePreScratchAccess);
  }

  return false;
}

void FlagSpillCleanup::regUseFlag(SCRATCH_PTR_LIST *scratchTraceList,
                                  G4_INST *inst, G4_Operand *opnd,
                                  int opndIndex) {
  // Get the linearized address in GRF register file
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  G4_Declare *topdcl = NULL;

  topdcl = opnd->getTopDcl();
  FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

  // Impact on previous scratch access
  for (SCRATCH_ACCESS *scratchAccess : *scratchTraceList) {
    if (linearizedEnd &&
        IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess)) {
      // Not handle indirect GRF
      if (inst->isEOT() || inst->isPseudoUse()) {
        scratchAccess->removeable = false;
        continue;
      }

      if (scratchAccess->flagOpnd->getTopDcl() == topdcl) // Same declare
      {
        if (regUseAnalysis(scratchAccess, linearizedStart, linearizedEnd)) {
          // The filled register is in use
          scratchAccess->removeable = false;
        } else if (scratchAccess->inRangePartialKilled ||
                   !scratchAccess->regKilled) {
          // can reuse previous register
          scratchAccess->renameOperandVec.emplace_back(inst, opndIndex);
        }
      }
    }
  }
}

void FlagSpillCleanup::regUseScratch(SCRATCH_PTR_LIST *scratchTraceList,
                                     G4_INST *inst, G4_Operand *opnd,
                                     Gen4_Operand_Number opndNum) {
  const G4_Declare *topdcl = opnd->getTopDcl();

  // Impact on previous scratch access
  for (SCRATCH_ACCESS *scratchAccess : *scratchTraceList) {
    if (topdcl == scratchAccess->scratchDcl) {
      if (opndNum == Opnd_dst) {
        scratchAccess->scratchDefined = true;
      } else {
        scratchAccess->removeable = false;
      }
    }
  }
}

void FlagSpillCleanup::initializeScratchAccess(SCRATCH_ACCESS *scratchAccess,
                                               INST_LIST_ITER inst_it) {
#ifdef _DEBUG
  scratchAccess->regNum = -1;
#endif
  scratchAccess->scratchDcl = NULL;
  scratchAccess->flagOpnd = NULL;

  scratchAccess->linearizedStart = 0;
  scratchAccess->linearizedEnd = 0;
  scratchAccess->leftOff = 0;
  scratchAccess->rightOff = 0;
  scratchAccess->useCount = 0;

  scratchAccess->isSpill = false;
  scratchAccess->isBlockLocal = false;
  scratchAccess->directKill = false;

  scratchAccess->regKilled = false;
  scratchAccess->regPartialKilled = false;
  scratchAccess->regOverKilled = false;
  scratchAccess->inRangePartialKilled = false;
  scratchAccess->regInUse = false;

  scratchAccess->fillInUse = false;
  scratchAccess->removeable = true;
  scratchAccess->instKilled = false;
  scratchAccess->evicted = false;
  scratchAccess->scratchDefined = false;

  scratchAccess->preScratchAccess = NULL;
  scratchAccess->prePreScratchAccess = NULL;
  scratchAccess->preFillAccess = NULL;

  scratchAccess->inst_it = inst_it;
  G4_INST *inst = *inst_it;
  scratchAccess->maskFlag = (inst->getOption() & 0xFFF010C);

  return;
}

bool FlagSpillCleanup::initializeFlagScratchAccess(
    SCRATCH_PTR_VEC *scratchAccessList, SCRATCH_ACCESS *&scratchAccess,
    INST_LIST_ITER inst_it) {
  G4_INST *inst = (*inst_it);

  G4_DstRegRegion *dst = inst->getDst();
  G4_Operand *src = inst->getSrc(0);
  G4_Declare *topDcl_1 = dst->getTopDcl();
  G4_Declare *topDcl_2 = src->getTopDcl();

  // Create the spill/fill description
  if (topDcl_1->getRegFile() == G4_FLAG && topDcl_2->getRegFile() == G4_GRF) {
    if (src->asSrcRegRegion()->getBase()->isRegVar() &&
        src->asSrcRegRegion()->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
      scratchAccess = new SCRATCH_ACCESS;
      scratchAccessList->push_back(scratchAccess);
      initializeScratchAccess(scratchAccess, inst_it);
      // Fill
#ifdef _DEBUG
      scratchAccess->regNum =
          topDcl_1->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
      scratchAccess->scratchDcl = topDcl_2; // Spill location

      if (gra.isBlockLocal(topDcl_2)) {
        scratchAccess->isBlockLocal = true;
      }
      FlagLineraizedStartAndEnd(topDcl_1, scratchAccess->linearizedStart,
                                scratchAccess->linearizedEnd);
      scratchAccess->flagOpnd = dst;
      if (inst->getPredicate()) {
        scratchAccess->removeable = false; // Partil spill/fill cannot be
                                           // removed
        scratchAccess->instKilled =
            true; // Not really killed, mark so that the instruction depends on
                  // current one will not be removed.
      }

      return true;
    }
  } else { // Spill
    if (dst->getBase()->isRegVar() &&
        dst->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
      scratchAccess = new SCRATCH_ACCESS;
      scratchAccessList->push_back(scratchAccess);
      initializeScratchAccess(scratchAccess, inst_it);
#ifdef _DEBUG
      scratchAccess->regNum =
          topDcl_2->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
      scratchAccess->scratchDcl = topDcl_1;

      if (gra.isBlockLocal(topDcl_1)) {
        scratchAccess->isBlockLocal = true;
      }

      scratchAccess->isSpill = true;
      FlagLineraizedStartAndEnd(topDcl_2, scratchAccess->linearizedStart,
                                scratchAccess->linearizedEnd);
      scratchAccess->flagOpnd = src;
      if (inst->getPredicate()) {
        scratchAccess->removeable = false; // Partil spill/fill cannot be
                                           // removed
        scratchAccess->instKilled =
            true; // Not really killed, mark so that the instruction depends on
                  // current one will not be removed.
      }

      return true;
    }
  }

  return false;
}

void FlagSpillCleanup::freeScratchAccess(SCRATCH_PTR_VEC *scratchAccessList) {
  for (SCRATCH_ACCESS *scratchAccess : *scratchAccessList) {
    delete scratchAccess;
  }

  scratchAccessList->clear();

  return;
}

// Check the flag define instruction.
void FlagSpillCleanup::flagDefine(SCRATCH_PTR_LIST &scratchTraceList,
                                  G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();

  if (dst) {
    G4_Declare *topdcl = NULL;
    topdcl = GetTopDclFromRegRegion(dst);

    if (topdcl && topdcl->getRegFile() == G4_FLAG) {
      // Flag register define
      regDefineFlag(&scratchTraceList, inst, dst);
    }
  }

  G4_CondMod *mod = inst->getCondMod();
  if (!mod) {
    return;
  }

  // ConMod, handled as register define
  unsigned maskFlag = (inst->getOption() & 0xFFF010C);

  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;

  G4_VarBase *flagReg = mod->getBase();
  if (!flagReg) {
    return;
  }

  G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
  FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

  SCRATCH_PTR_LIST_ITER it = scratchTraceList.begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList.end();
  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *preScratchAccess = *it;
    if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd,
                              preScratchAccess)) {
      G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();

      if (regDefineAnalysis(preScratchAccess, linearizedStart, linearizedEnd,
                            (unsigned short)maskFlag, NULL)) {
        preScratchAccess->regKilled = true;
        if (preScratchAccess->evicted) // Not in use
        {
          scratchTraceList.erase(
              it); // The previous one is not candidate for reuse
        }
      }
      if (topdcl == preDcl) {
        if (preScratchAccess->inRangePartialKilled) {
          preScratchAccess->renameOperandVec.emplace_back(inst, -3);
        } else {
          preScratchAccess->removeable = false;
        }
      }
    }
    it = kt;
  }

  return;
}

void FlagSpillCleanup::scratchUse(SCRATCH_PTR_LIST &scratchTraceList,
                                  G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();

  if (dst) {
    G4_Declare *topdcl = NULL;
    topdcl = GetTopDclFromRegRegion(dst);

    if (topdcl && topdcl->getRegFile() == G4_GRF) {
      // Flag scratch variable is redefined
      regUseScratch(&scratchTraceList, inst, dst, Opnd_dst);
    }
  }

  for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src && src->isSrcRegRegion()) {
      G4_Declare *topdcl = NULL;

      if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar()) {
        topdcl = GetTopDclFromRegRegion(src);
      }

      if (!topdcl || (topdcl->getRegFile() == G4_FLAG)) {
        continue;
      }

      regUseScratch(&scratchTraceList, inst, src, Opnd_src0);
    }
  }
}

void FlagSpillCleanup::flagUse(SCRATCH_PTR_LIST &scratchTraceList,
                               G4_INST *inst) {
  for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src && src->isSrcRegRegion()) {
      G4_Declare *topdcl = NULL;

      if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar()) {
        topdcl = GetTopDclFromRegRegion(src);
      }

      if (!topdcl || (topdcl->getRegFile() != G4_FLAG)) {
        continue;
      }

      regUseFlag(&scratchTraceList, inst, src, i);
    }
  }

  // Flag register is used as predicate
  G4_Predicate *predicate = inst->getPredicate();
  if (!predicate) {
    return;
  }

  G4_VarBase *flagReg = predicate->getBase();
  if (!flagReg) {
    return;
  }

  G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

  for (SCRATCH_ACCESS *preScratchAccess : scratchTraceList) {
    if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd,
                              preScratchAccess)) {
      G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();
      // Use should have same top declare
      if (preDcl == topdcl) {
        if (regUseAnalysis(preScratchAccess, linearizedStart, linearizedEnd)) {
          preScratchAccess->removeable = false;
        } else if (preScratchAccess->inRangePartialKilled ||
                   !preScratchAccess->regKilled) {
          // can reuse previous register
          preScratchAccess->renameOperandVec.emplace_back(inst, -2);
        }
      }
    }
  }

  return;
}

bool FlagSpillCleanup::flagScratchDefineUse(
    G4_BB *bb, SCRATCH_PTR_LIST *scratchTraceList,
    SCRATCH_PTR_VEC *candidateList, SCRATCH_ACCESS *scratchAccess,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *preScratchAccess = *it;

    // Evicted
    if (preScratchAccess->evicted) {
      it = kt;
      continue;
    }

    // Same scratch declare
    if (preScratchAccess->scratchDcl ==
        scratchAccess->scratchDcl) // Same scratch location
    {
      if (scratchAccess->isSpill) // Current is spill
      {
        if (IS_SPILL_KILL_CANDIDATE(
                preScratchAccess)) // previoius is spill as well and previous
                                   // spill is not used
        {
          // kill the previous spill
          bb->erase(preScratchAccess->inst_it);
          preScratchAccess->instKilled = true;
          clean_num_profile->spill_clean_num[0]++;
          scratchTraceList->erase(
              it); // The previous one is not candidate for reuse
          it = kt;

          continue;
        }

        preScratchAccess->evicted = true;
        scratchTraceList->erase(
            it); // The previous one is not a good candidate for reuse any more
      } else     // Current is fill
      {
        preScratchAccess->fillInUse = true;
        preScratchAccess->useCount++;

        if (IS_USE_KILL_CANDIDATE(preScratchAccess)) // Is not used before
        {
          scratchAccess->preScratchAccess =
              preScratchAccess; // set previous scrach location define
          candidateList->push_back(scratchAccess); // Add to candidate list
          if (IS_FLAG_RANGE_OVERWRITE(scratchAccess,
                                      preScratchAccess->linearizedStart,
                                      preScratchAccess->linearizedEnd)) {
            // Exactly same GRF, it's useless fill, since prevous fill or spill
            // not been killed
            scratchAccess->directKill = true;
            scratchTraceList->push_back(scratchAccess);
            return true;
          }
        }
      }
    }
    it = kt;
  }

  scratchTraceList->push_back(scratchAccess);

  return false;
}

void FlagSpillCleanup::flagSpillFillClean(
    G4_BB *bb, INST_LIST_ITER inst_it, SCRATCH_PTR_VEC &scratchAccessList,
    SCRATCH_PTR_LIST &scratchTraceList, SCRATCH_PTR_VEC &candidateList,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  G4_INST *inst = (*inst_it);
  if (inst->isPseudoKill()) {
    return;
  }

  bool noDefineAnalysis = false;

  // Check if there is flag use
  flagUse(scratchTraceList, inst);

  // Check if it's spill/fill of the flag
  if (IS_FLAG_MOVE(inst)) {
    SCRATCH_ACCESS *scratchAccess = NULL;

    if (initializeFlagScratchAccess(&scratchAccessList, scratchAccess,
                                    inst_it)) {
      // Build the trace list and the candidate list
      // Trace list includes all spill/fill
      // Candidate includues ??
      // Checking if the spill/fill can be removed at the same time by comparing
      // previous one.
      noDefineAnalysis =
          flagScratchDefineUse(bb, &scratchTraceList, &candidateList,
                               scratchAccess, clean_num_profile);
    }
  } else {
    scratchUse(scratchTraceList, inst);
  }

  // Check if there is flag define
  if (!noDefineAnalysis) {
    flagDefine(scratchTraceList, inst);
  }

  return;
}

#ifdef _DEBUG
#define FILL_DEBUG_THRESHOLD 0xffffffff
#define SPILL_DEBUG_THRESHOLD 0xffffffff // 25
#endif

void FlagSpillCleanup::regFillClean(IR_Builder &builder, G4_BB *bb,
                                    SCRATCH_PTR_VEC &candidateList,
                                    CLEAN_NUM_PROFILE *clean_num_profile) {
  for (SCRATCH_ACCESS *scratchAccess : candidateList) {
    SCRATCH_ACCESS *preScratchAccess = scratchAccess->preScratchAccess;

    // Since the reuse happens from front to end.
    // If the pre scratchAccess is killed, current candidate can not reuse
    // previous register any more
    if (!scratchAccess->instKilled &&
        (scratchAccess->removeable && scratchAccess->directKill)) {
      if (scratchAccess->prePreScratchAccess) {
        while (preScratchAccess && preScratchAccess->preScratchAccess &&
               preScratchAccess != scratchAccess->prePreScratchAccess) {
          // If possible, propagate to previous scratchAccess
          if (preScratchAccess->preFillAccess) {
            // to jump over prefill.
            if (preScratchAccess->isSpill && preScratchAccess->preFillAccess &&
                preScratchAccess->preFillAccess->instKilled &&
                preScratchAccess->preScratchAccess) {
              preScratchAccess = preScratchAccess->preScratchAccess;
            } else {
              break;
            }
          } else {
            if (!preScratchAccess->instKilled) {
              break;
            }
            preScratchAccess = preScratchAccess->preScratchAccess;
          }
        }

        if (preScratchAccess) {
          if (preScratchAccess->isSpill && preScratchAccess->preFillAccess &&
              preScratchAccess->preFillAccess->instKilled) {
          } else if (!preScratchAccess->instKilled) {
            if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess)) {
              gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
              bb->erase(scratchAccess->inst_it);
              scratchAccess->instKilled = true;
              scratchAccess->preScratchAccess->useCount--;
              clean_num_profile->fill_clean_num[0]++;
            }
          }
        }
      } else {
        if (preScratchAccess && !preScratchAccess->instKilled) {
          if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess)) {
            gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
            bb->erase(scratchAccess->inst_it);
            scratchAccess->instKilled = true;
            scratchAccess->preScratchAccess->useCount--;
            clean_num_profile->fill_clean_num[0]++;
          }
        }
      }
    }
#ifdef _DEBUG
    if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
      return;
#endif
  }

  return;
}

void FlagSpillCleanup::regSpillClean(IR_Builder &builder, G4_BB *bb,
                                     SCRATCH_PTR_VEC &candidateList,
                                     CLEAN_NUM_PROFILE *clean_num_profile) {
  for (SCRATCH_ACCESS *scratchAccess : candidateList) {
    if (scratchAccess->instKilled) {
      continue;
    }
    if (!scratchAccess->instKilled && scratchAccess->isSpill &&
        scratchAccess->removeable && scratchAccess->evicted &&
        scratchAccess->useCount == 0) {
      gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
      bb->erase(scratchAccess->inst_it);
      scratchAccess->instKilled = true;
      clean_num_profile->spill_clean_num[0]++;
#ifdef _DEBUG
      if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD) {
        return;
      }
#endif
    }
  }

  return;
}

// Replace Scratch Block Read/Write message with OWord Block Read/Write message
// For spill code clean up, clean target may exist in all WAW, RAR, RAW, WAR.
void FlagSpillCleanup::spillFillCodeCleanFlag(
    IR_Builder &builder, G4_Kernel &kernel,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  SCRATCH_PTR_VEC scratchAccessList;
  SCRATCH_PTR_LIST scratchTraceList;
  SCRATCH_PTR_VEC candidateList;
  FlowGraph &fg = kernel.fg;

  [[maybe_unused]] int candidate_size = 0;
  for (auto bb : fg) {
    INST_LIST_ITER inst_it = bb->begin();

    scratchTraceList.clear();
    candidateList.clear();
    freeScratchAccess(&scratchAccessList);

    // Top down scan within BB
    while (inst_it != bb->end()) {
      INST_LIST_ITER inst_it_next = inst_it;
      inst_it_next++;

      flagSpillFillClean(bb, inst_it, scratchAccessList, scratchTraceList,
                         candidateList, clean_num_profile);

      inst_it = inst_it_next;
    }

#ifdef _DEBUG
    candidate_size += (int)candidateList.size();
#endif
    // Clean the fills.
    regFillClean(builder, bb, candidateList, clean_num_profile);

#ifdef _DEBUG
    if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
      return;
#endif
    // Clean the spills
    regSpillClean(builder, bb, scratchAccessList, clean_num_profile);

#ifdef _DEBUG
    if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD) {
      return;
    }
#endif
  }

  freeScratchAccess(&scratchAccessList);
  scratchTraceList.clear();
  candidateList.clear();

  VISA_DEBUG_VERBOSE(printf("Candidate size: %d\n", candidate_size));

  return;
}
