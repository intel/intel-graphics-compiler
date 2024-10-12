/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "InstSplit.h"

using namespace vISA;

InstSplitPass::InstSplitPass(IR_Builder *builder) : m_builder(builder) {}

static bool DoNotSplit(G4_INST* inst) {
  if (inst->isDpas() || inst->isSend() || inst->opcode() == G4_label ||
      inst->opcode() == G4_pln || inst->opcode() == G4_return ||
      inst->isFlowControl() || inst->isPseudoLogic() ||
      inst->opcode() == G4_madw)
    return true;

  return false;
}

// This pass verifies instructions sizes with respect to SIMD width and
// operands' data type. Instructions that touch more than 2 GRFs are split
// evenly until they are within 2 GRFs. Instructions not considered for
// splitting:
//      - SIMD1, SIMD2, SIMD4 and SIMD8
//      - Send messages
//      - Plane
//      - Control flow, labels and return
//      - Dpas
//      - Instructions with indirect addressing other than 1x1 indirect region
//      - SIMD32 instructions with operands which have Q/DF datatypes and
//        direct addressing mode
void InstSplitPass::run() {
  for (INST_LIST_ITER it = m_builder->instList.begin(),
                      instlistEnd = m_builder->instList.end();
       it != instlistEnd; ++it) {
    G4_INST *inst = *it;

    if (inst->getExecSize() == g4::SIMD1) {
      continue;
    }

    if (DoNotSplit(inst)) {
      continue;
    }

    it = splitInstruction(it, m_builder->instList);
  }
}

void InstSplitPass::runOnBB(G4_BB *bb) {
  for (INST_LIST_ITER it = bb->begin(), instlistEnd = bb->end();
       it != instlistEnd; ++it) {
    G4_INST *inst = *it;

    if (inst->getExecSize() == g4::SIMD1) {
      continue;
    }

    if (DoNotSplit(inst)) {
      continue;
    }

    it = splitInstruction(it, bb->getInstList());
  }
}

// Recursive function to split instructions that touch more than 2 GRF
// For example, with 32-byte GRF:
//    1 SIMD32 inst with 64-bit operand(s)
//    split into:
//                  -> 2 SIMD16 insts with 64-bit operand(s)
//    split again into:
//                  -> 4 SIMD8 insts with 64-bit operand(s)
INST_LIST_ITER InstSplitPass::splitInstruction(INST_LIST_ITER it,
                                               INST_LIST &instList) {
  G4_INST *inst = *it;
  bool doSplit = false;
  G4_ExecSize execSize = inst->getExecSize();

  auto cross2GRF = [inst, this](G4_Operand *opnd) {
    G4_SrcRegRegion *src = opnd->asSrcRegRegion();
    // A source cannot span more than 2 adjacent GRF registers, if the source
    // is in indirect 1x1 mode. vISA assumes that the subreg of indirect operand
    // is GRF-aligned.
    bool indirect1x1 = opnd->isIndirect() && !src->getRegion()->isRegionWH();
    if (indirect1x1) {
      uint16_t srcStride = 0;
      uint32_t execSize = inst->getExecSize();
      src->getRegion()->isSingleStride(execSize, srcStride);
      return (execSize * src->getTypeSize() * srcStride) >
             (m_builder->getGRFSize() * 2u);
    }
    uint32_t leftBound = 0, rightBound = 0;
    computeSrcBounds(src, leftBound, rightBound);
    return (rightBound - leftBound) > (m_builder->getGRFSize() * 2u);
  };

  auto cross2GRFDst = [inst, this](G4_DstRegRegion *dst) {
    // In Indirect Addressing mode, a destination cannot span more than 2
    // adjacent GRF registers. vISA assumes that the subreg of indirect
    // operand is GRF-aligned.
    if (dst->isNullReg() || dst->isIndirect()) {
      return ((unsigned)inst->getExecSize() * dst->getTypeSize() *
              dst->getHorzStride()) > (m_builder->getGRFSize() * 2u);
    }
    uint32_t leftBound = 0, rightBound = 0;
    computeDstBounds(dst, leftBound, rightBound);
    return (rightBound - leftBound) > (m_builder->getGRFSize() * 2u);
  };

  auto useTmpForSrc = [&](G4_SrcRegRegion *src) -> G4_SrcRegRegion * {
    // insert mov before current instruction
    G4_Declare *dcl = m_builder->createTempVar(execSize, src->getType(), Any);
    G4_SrcModifier modifier = src->getModifier();
    src->setModifier(Mod_src_undef);

    G4_INST *movInst =
        m_builder->createMov(execSize, m_builder->createDstRegRegion(dcl, 1),
                             src, inst->getOption(), false);
    movInst->inheritDIFrom(inst);

    INST_LIST_ITER newMovIter = instList.insert(it, movInst);

    // split new mov if needed
    splitInstruction(newMovIter, instList);

    G4_SrcRegRegion *tmpSrc = m_builder->createSrcRegRegion(
        modifier, Direct, dcl->getRegVar(), 0, 0, m_builder->getRegionStride1(),
        dcl->getElemType());
    return tmpSrc;
  };

  // Exception to allow to span 2 GRFs:
  // When ExecSize = 32 and Dtatype = DF or *Q, then dst/src operands cannot
  // span more than 4 ajacent GRF registers. This requires that the operand must
  // be GRF-aligned and have contiguous regions.
  auto AllowCross2GRF = [&](G4_Operand *opnd) {
    if (!m_builder->supportNativeSIMD32())
      return false;

    // Must be SIMD32 with 64b datatypes
    if (inst->getExecSize() != g4::SIMD32 || opnd->getTypeSize() != 8)
      return false;

    // Must be GRF-aligned
    if (!opnd->isScalarSrc() && !m_builder->tryToAlignOperand(
            opnd, m_builder->getGRFSize()))
      return false;

    // Must be scalar or contiguous regions
    if (opnd->isDstRegRegion()) {
      return (opnd->asDstRegRegion()->getHorzStride() == 1);
    } else {
      return (opnd->isScalarSrc() ||
             opnd->asSrcRegRegion()->getRegion()->isContiguous(
                 inst->getExecSize()));
    }
    return false;
  };

  // Check sources
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    if (!inst->getSrc(i)->isSrcRegRegion())
      continue;
    if (cross2GRF(inst->getSrc(i)) && !AllowCross2GRF(inst->getSrc(i))) {
      doSplit = true;
      break;
    }
    if (m_builder->getPlatform() >= Xe_XeHPSDV) {
      // Instructions whose operands are 64b and have 2D regioning need to be
      // split up front to help fixUnalignedRegions(..) covering 2D cases.
      G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
      if ((src->getType() == Type_DF || IS_QTYPE(src->getType())) &&
          !src->getRegion()->isSingleStride(execSize)) {
        // Try splitting the inst if it's a mov. Otherwise, legalize
        // the inst by inserting a mov for the src, and split the new
        // mov if needed.
        if (inst->opcode() == G4_mov) {
          doSplit = true;
          break;
        }

        auto tmpSrc = useTmpForSrc(src);
        vASSERT(tmpSrc->getRegion()->isSingleStride(execSize));
        inst->setSrc(tmpSrc, i);
      }
    }
  }

  // Check destination
  if (inst->getDst() && cross2GRFDst(inst->getDst()) &&
      !AllowCross2GRF(inst->getDst())) {
    doSplit = true;
  }

  // Handle split exceptions
  if (!doSplit) {
    if (inst->opcode() == G4_cmp && !m_builder->supportNativeSIMD32()) {
      // Due to a simulator quirk, we need to split cmp instruction even if the
      // dst operand of the compare is null, if it "looks" too large,
      // that is, if the execution size is 16 and the comparison type
      // is QW.
      if (needSplitByExecSize(execSize) && inst->getDst()->isNullReg() &&
          (inst->getSrc(0)->getTypeSize() > 4 ||
           inst->getSrc(1)->getTypeSize() > 4)) {
        doSplit = true;
      }
    }
  }

  if (!doSplit) {
    return it;
  }

  G4_opcode op = inst->opcode();
  G4_ExecSize newExecSize{execSize / 2};

  G4_DstRegRegion *dst = inst->getDst();
  bool nullDst = dst && inst->hasNULLDst();

  // Check src/dst dependency
  if (dst && !nullDst) {
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      bool useTmp = false;
      G4_Operand *src = inst->getSrc(i);
      G4_CmpRelation rel = compareSrcDstRegRegion(dst, src);
      if (rel != Rel_disjoint) {
        useTmp = (rel != Rel_eq) ||
                 src->asSrcRegRegion()->getRegion()->isRepeatRegion(
                     inst->getExecSize());
      }

      if (useTmp) {
        vISA_ASSERT(src != nullptr && src->isSrcRegRegion(),
                     "source must be a SrcRegRegion");
        auto tmpSrc = useTmpForSrc(src->asSrcRegRegion());
        inst->setSrc(tmpSrc, i);
      }
    }
  }

  // Special handling for vISA ADDC (SUBB) instruction,
  // which is translated to addc (subb)/mov acc pair
  // and should be splitted together.
  G4_INST *carryMovInst = nullptr;
  G4_Predicate* newCarryMovPred = NULL;
  G4_CondMod* newCarryMovCondMod = NULL;
  INST_LIST_ITER nextIter = std::next(it);
  if (inst->opcode() == G4_addc || inst->opcode() == G4_subb) {
    G4_INST *nextInst = *nextIter;
    if (nextInst->opcode() == G4_mov && nextInst->isAccSrcInst() &&
        nextInst->getExecSize() == inst->getExecSize()) {
      carryMovInst = nextInst;

      if (carryMovInst->getPredicate()) {
        newCarryMovPred = carryMovInst->getPredicate();
        newCarryMovPred->splitPred();
      }

      if (carryMovInst->getCondMod()) {
        newCarryMovCondMod = carryMovInst->getCondMod();
        newCarryMovCondMod->splitCondMod();
      }
    }
  }

  G4_SrcRegRegion* accSrcRegion = NULL;
  if (inst->getImplAccSrc()) {
    accSrcRegion = inst->getImplAccSrc()->asSrcRegRegion();
  }

  G4_DstRegRegion* accDstRegion = NULL;
  if (inst->getImplAccDst()) {
    accDstRegion = inst->getImplAccDst();
  }

  // Create new predicate
  G4_Predicate *newPred = NULL;
  if (inst->getPredicate()) {
    newPred = inst->getPredicate();
    newPred->splitPred();
  }

  // Create new condition modifier
  G4_CondMod *newCondMod = NULL;
  if (inst->getCondMod()) {
    newCondMod = inst->getCondMod();
    newCondMod->splitCondMod();
  }

  INST_LIST_ITER newInstIterator = it;
  for (int i = 0; i < execSize; i += newExecSize) {
    G4_INST *newInst = nullptr;

    // Create new destination
    G4_DstRegRegion *newDst;
    if (dst && !nullDst) {
      newDst = m_builder->createSubDstOperand(dst, (uint16_t)i, newExecSize);
    } else {
      newDst = dst;
    }

    // Create new split instruction
    newInst = m_builder->makeSplittingInst(inst, newExecSize);
    newInst->setDest(newDst);
    newInst->setPredicate(m_builder->duplicateOperand(newPred));
    newInst->setCondMod(m_builder->duplicateOperand(newCondMod));
    newInstIterator = instList.insert(it, newInst);

    // Set new sources
    for (int j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = inst->getSrc(j);
      if (!src)
        continue;

      // Src1 for single source math should be arc reg null.
      if (src->isImm() ||
          (inst->opcode() == G4_math && j == 1 && src->isNullReg())) {
        newInst->setSrc(src, j);
      } else if (src->asSrcRegRegion()->isScalar() ||
                 (j == 0 && op == G4_line)) {
        newInst->setSrc(m_builder->duplicateOperand(src), j);
      } else {
        newInst->setSrc(
            m_builder->createSubSrcOperand(
                src->asSrcRegRegion(), (uint16_t)i, newExecSize,
                (uint8_t)(src->asSrcRegRegion()->getRegion()->vertStride),
                (uint8_t)(src->asSrcRegRegion()->getRegion()->width)),
            j);
      }
    }

    // Set new mask
    // FIXME: To update the mask in a CM kernel, the inst's BB should be
    // divergent.
    //        However, at this stage BBs are not constructed yet.
    bool isCMKernel = m_builder->kernel.getInt32KernelAttr(
                          Attributes::ATTR_Target) == VISA_CM;
    bool needsMaskOffset =
        newCondMod || newPred || (!isCMKernel && !inst->isWriteEnableInst());
    if (needsMaskOffset) {
      int newMaskOffset = inst->getMaskOffset() + (i == 0 ? 0 : newExecSize);
      bool nibOk =
          m_builder->hasNibCtrl() && (inst->getDst()->getTypeSize() == 8 ||
                                      TypeSize(inst->getExecType()) == 8);
      G4_InstOption newMask =
          G4_INST::offsetToMask(newExecSize, newMaskOffset, nibOk);
      newInst->setMaskOption(newMask);
    }

    if (accDstRegion)
      newInst->setImplAccDst(m_builder->duplicateOperand(accDstRegion));

    if (accSrcRegion)
      newInst->setImplAccSrc(m_builder->duplicateOperand(accSrcRegion));

    if (carryMovInst) {
      G4_INST* newCarryMovInst = m_builder->makeSplittingInst(carryMovInst, newExecSize);

      G4_DstRegRegion* carryMovDst = carryMovInst->getDst();
      G4_DstRegRegion* newCarryMovDst;
      if (carryMovDst && !carryMovInst->hasNULLDst()) {
        newCarryMovDst = m_builder->createSubDstOperand(carryMovDst, (uint16_t)i, newExecSize);
      }
      else {
        newCarryMovDst = newCarryMovDst;
      }

      newCarryMovInst->setDest(newCarryMovDst);
      newCarryMovInst->setPredicate(m_builder->duplicateOperand(newCarryMovPred));
      newCarryMovInst->setCondMod(m_builder->duplicateOperand(newCarryMovCondMod));
      newCarryMovInst->setSrc(m_builder->duplicateOperand(carryMovInst->getSrc(0)), 0);
      auto prevInstIterator = newInstIterator;
      newInstIterator = instList.insert(it, newCarryMovInst);

      // Call recursive splitting function
      newInstIterator = splitInstruction(prevInstIterator, instList);
    }
    else {
      // Call recursive splitting function
      newInstIterator = splitInstruction(newInstIterator, instList);
    }
  }

  // remove original instruction
  instList.erase(it);

  if (carryMovInst) {
    instList.erase(nextIter);
  }

  return newInstIterator;
}

bool InstSplitPass::needSplitByExecSize(G4_ExecSize execSize) const {
  if (m_builder->getGRFSize() == 64) {
    return execSize == g4::SIMD32;
  }
  return execSize == g4::SIMD16;
}

// Compare regRegion of source operand and destination.
// We put this in a separate function since compareOperand from G4_DstRegRegion
// and G4_SrcRegRegion don't handle regions that cross 2 GRFs.
G4_CmpRelation InstSplitPass::compareSrcDstRegRegion(G4_DstRegRegion *dstRegion,
                                                     G4_Operand *opnd) {

  G4_VarBase *dstBase = dstRegion->getBase();
  G4_VarBase *srcBase = opnd->getBase();
  bool dstIndirect = dstRegion->isIndirect();
  bool srcIndirect = opnd->isIndirect();
  G4_Declare *dstDcl = dstRegion->getTopDcl();
  G4_Declare *srcDcl = opnd->getTopDcl();

  if (!opnd->isSrcRegRegion() || dstBase == nullptr || srcBase == nullptr) {
    // a null base operand can never interfere with anything
    return Rel_disjoint;
  }

  if (dstDcl == srcDcl && srcDcl != nullptr) {
    // special checks for pseudo kills
    G4_INST *dstInst = dstRegion->getInst();
    G4_INST *srcInst = opnd->getInst();
    if (dstInst && (dstInst->isPseudoKill() || dstInst->isLifeTimeEnd())) {
      return Rel_interfere;
    }

    if (srcInst && (srcInst->isPseudoKill() || srcInst->isLifeTimeEnd())) {
      return Rel_interfere;
    }
  }

  if (srcIndirect && dstIndirect)
    // two indirect are assumed to interfere in the absence of pointer analysis
    return Rel_interfere;
  if (srcIndirect != dstIndirect) {
    // direct v. indirect
    auto mayInterfereWithIndirect = [](G4_Operand *direct,
                                       G4_Operand *indirect) {
      vISA_ASSERT((!direct->isIndirect() && indirect->isIndirect()),
             "first opereand should be direct and second indirect");
      return (direct->getTopDcl() && direct->getTopDcl()->getAddressed()) ||
             (direct->getBase()->isAddress() &&
              direct->getTopDcl() == indirect->getTopDcl());
    };

    if ((srcIndirect && mayInterfereWithIndirect(dstRegion, opnd)) ||
        (dstIndirect && mayInterfereWithIndirect(opnd, dstRegion))) {
      return Rel_interfere;
    }
    return Rel_disjoint;
  }

  // Check if both are physically assigned
  G4_VarBase *dstPhyReg =
      dstBase->isRegVar() ? dstBase->asRegVar()->getPhyReg() : dstBase;
  G4_VarBase *srcPhyReg =
      srcBase->isRegVar() ? srcBase->asRegVar()->getPhyReg() : srcBase;
  if (dstPhyReg && srcPhyReg) {
    vASSERT(dstPhyReg->isPhyReg() && srcPhyReg->isPhyReg());
    if (dstPhyReg->getKind() != srcPhyReg->getKind())
      return Rel_disjoint;

    if (dstPhyReg->isPhyAreg()) {
      if (dstPhyReg->asAreg()->getArchRegType() == AREG_NULL) {
        // like NaN, a null ARF is disjoint to everyone including itself
        return Rel_disjoint;
      }
      return (dstPhyReg->asAreg()->getArchRegType() ==
              srcPhyReg->asAreg()->getArchRegType())
                 ? Rel_eq
                 : Rel_disjoint;
    }
  }

  if (dstBase->getKind() != srcBase->getKind()) {
    return Rel_disjoint;
  }

  if (dstDcl != srcDcl) {
    return Rel_disjoint;
  }

  // Lastly, check byte footprint for exact relation
  uint32_t srcLeftBound = 0, srcRightBound = 0;
  int maskSize = 8 * m_builder->getGRFSize();
  BitSet srcBitSet(maskSize, false);
  computeSrcBounds(opnd->asSrcRegRegion(), srcLeftBound, srcRightBound);
  generateBitMask(opnd, srcBitSet);

  uint32_t dstLeftBound = 0, dstRightBound = 0;
  BitSet dstBitSet(maskSize, false);
  computeDstBounds(dstRegion, dstLeftBound, dstRightBound);
  generateBitMask(dstRegion, dstBitSet);

  if (dstRightBound < srcLeftBound || srcRightBound < dstLeftBound) {
    return Rel_disjoint;
  } else if (dstLeftBound == srcLeftBound && dstRightBound == srcRightBound &&
             dstBitSet == srcBitSet) {
    return Rel_eq;
  } else {

    BitSet tmp = dstBitSet;
    dstBitSet &= srcBitSet;
    if (dstBitSet.isEmpty()) {
      return Rel_disjoint;
    }

    dstBitSet = tmp;
    dstBitSet -= srcBitSet;
    if (dstBitSet.isEmpty()) {
      return Rel_lt;
    }
    srcBitSet -= tmp;
    return srcBitSet.isEmpty() ? Rel_gt : Rel_interfere;
  }
}

// Simplified function to calculate left/right bounds.
// InstSplitPass calls this function since the operand's internal computeBound
// function carries several aditional calculations and asserts restricted to 2
// GRFs.
void InstSplitPass::computeDstBounds(G4_DstRegRegion *dstRegion,
                                     uint32_t &leftBound,
                                     uint32_t &rightBound) {
  unsigned short typeSize = dstRegion->getTypeSize();

  // Calculate left bound
  {
    G4_VarBase *base = dstRegion->getBase();
    G4_Declare *topDcl = NULL;
    uint32_t subRegOff = dstRegion->getSubRegOff();
    uint32_t regOff = dstRegion->getRegOff();
    uint32_t newregoff = regOff, offset = 0;
    if (base && base->isRegVar()) {
      topDcl = base->asRegVar()->getDeclare();
      if (!topDcl && base->asRegVar()->isGreg()) {
        newregoff = base->asRegVar()->asGreg()->getRegNum();
      }
    }

    if (topDcl) {
      while (topDcl->getAliasDeclare()) {
        offset += topDcl->getAliasOffset();
        topDcl = topDcl->getAliasDeclare();
      }
    }

    if (base != NULL && base->isAccReg()) {
      leftBound = subRegOff * typeSize;
      if (base->asAreg()->getArchRegType() == AREG_ACC1 || regOff == 1) {
        leftBound += m_builder->getGRFSize();
      }
    } else if (topDcl) {
      if (dstRegion->getRegAccess() == Direct) {
        leftBound = offset + newregoff * m_builder->numEltPerGRF<Type_UB>() +
                    subRegOff * typeSize;
      } else {
        leftBound = subRegOff * TypeSize(ADDR_REG_TYPE);
      }
    }
  }

  // Calculate right bound
  {
    if (dstRegion->getRegAccess() == Direct) {
      unsigned short s_size = dstRegion->getHorzStride() * typeSize;
      unsigned totalBytes =
          (dstRegion->getInst()->getExecSize() - 1) * s_size + typeSize;
      rightBound = leftBound + totalBytes - 1;
    } else {
      rightBound = leftBound + TypeSize(ADDR_REG_TYPE) - 1;
    }
  }
}

// Simplified function to calculate left/right bounds.
// InstSplitPass calls this function since the operand's internal computeBound
// function carries several aditional calculations and asserts restricted to 2
// GRFs.
void InstSplitPass::computeSrcBounds(G4_SrcRegRegion *srcRegion,
                                     uint32_t &leftBound,
                                     uint32_t &rightBound) {
  unsigned short typeSize = srcRegion->getTypeSize();

  // Calculate left bound
  {
    G4_VarBase *base = srcRegion->getBase();
    G4_Declare *topDcl = NULL;
    uint32_t subRegOff = srcRegion->getSubRegOff();
    uint32_t regOff = srcRegion->getRegOff();
    unsigned newregoff = regOff, offset = 0;

    if (base) {
      if (base->isRegVar()) {
        topDcl = base->asRegVar()->getDeclare();
        if (!topDcl && base->asRegVar()->isGreg()) {
          newregoff = base->asRegVar()->asGreg()->getRegNum();
        }
      }
    }

    if (topDcl) {
      while (topDcl->getAliasDeclare()) {
        offset += topDcl->getAliasOffset();
        topDcl = topDcl->getAliasDeclare();
      }
    }

    if (base != NULL && base->isAccReg()) {
      leftBound = subRegOff * typeSize;
      if (base->asAreg()->getArchRegType() == AREG_ACC1) {
        leftBound += m_builder->getGRFSize();
      }
    } else if (topDcl) {
      if (srcRegion->getRegAccess() == Direct) {
        leftBound = offset + newregoff * m_builder->numEltPerGRF<Type_UB>() +
                    subRegOff * typeSize;
      } else {
        leftBound = subRegOff * TypeSize(ADDR_REG_TYPE);
      }
    }
  }

  // Calculate right bound
  {
    if (srcRegion->getRegAccess() == Direct) {
      unsigned short hs = srcRegion->getRegion()->isScalar()
                              ? 1
                              : srcRegion->getRegion()->horzStride;
      unsigned short vs = srcRegion->getRegion()->isScalar()
                              ? 0
                              : srcRegion->getRegion()->vertStride;

      if (srcRegion->getRegion()->isScalar()) {
        rightBound = leftBound + typeSize - 1;
      } else {
        int numRows =
            srcRegion->getInst()->getExecSize() / srcRegion->getRegion()->width;
        if (numRows > 0) {
          rightBound = leftBound + (numRows - 1) * vs * typeSize +
                       hs * (srcRegion->getRegion()->width - 1) * typeSize +
                       typeSize - 1;
        } else {
          rightBound =
              leftBound +
              hs * (srcRegion->getInst()->getExecSize() - 1) * typeSize +
              typeSize - 1;
        }
      }
    } else {
      unsigned short numAddrSubReg = 1;
      if (srcRegion->getRegion()->isRegionWH()) {
        numAddrSubReg =
            srcRegion->getInst()->getExecSize() / srcRegion->getRegion()->width;
      }
      rightBound = leftBound + TypeSize(ADDR_REG_TYPE) * numAddrSubReg - 1;
    }
  }
}

// Generates the byte footprint of an instruction's operand
void InstSplitPass::generateBitMask(G4_Operand *opnd, BitSet &footprint) {
  uint64_t bitSeq = TypeFootprint(opnd->getType());
  unsigned short typeSize = opnd->getTypeSize();

  if (opnd->isDstRegRegion()) {
    if (!opnd->isIndirect()) {
      G4_DstRegRegion *dst = opnd->asDstRegRegion();
      unsigned short horzStride = dst->getHorzStride();
      unsigned short s_size = horzStride * typeSize;
      for (uint8_t i = 0; i < opnd->getInst()->getExecSize(); ++i) {
        int eltOffset = i * s_size;
        for (uint8_t j = 0; j < typeSize; j++) {
          footprint.set(eltOffset + j, true);
        }
      }
    } else {
      footprint.set(0, true);
      footprint.set(1, true);
    }
  } else if (opnd->isSrcRegRegion()) {
    G4_SrcRegRegion *src = opnd->asSrcRegRegion();
    const RegionDesc *srcReg = src->getRegion();
    if (!opnd->isIndirect()) {
      if (srcReg->isScalar()) {
        uint64_t mask = bitSeq;
        for (unsigned i = 0; i < typeSize; ++i) {
          if (mask & (1ULL << i)) {
            footprint.set(i, true);
          }
        }
      } else {
        for (int i = 0,
                 numRows = opnd->getInst()->getExecSize() / srcReg->width;
             i < numRows; ++i) {
          for (int j = 0; j < srcReg->width; ++j) {
            int eltOffset = i * srcReg->vertStride * typeSize +
                            j * srcReg->horzStride * typeSize;
            for (uint8_t k = 0; k < typeSize; k++) {
              footprint.set(eltOffset + k, true);
            }
          }
        }
      }
    } else {
      unsigned short numAddrSubReg = 1;
      if (srcReg->isRegionWH()) {
        numAddrSubReg = opnd->getInst()->getExecSize() / srcReg->width;
      }
      uint64_t mask = 0;
      for (unsigned i = 0; i < numAddrSubReg; i++) {
        mask |= ((uint64_t)0x3) << (i * 2);
      }
      for (unsigned i = 0; i < 64; ++i) {
        if (mask & (1ULL << i)) {
          footprint.set(i, true);
        }
      }
    }
  }
}
