/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "G4_Verifier.hpp"
#include "HWConformity.h"
#include "InstSplit.h"
#include "Optimizer.h"

using namespace vISA;

uint8_t HWConformity::checkMinExecSize(G4_opcode op) {
  if (op == G4_dp2 || op == G4_dp3 || op == G4_dp4 || op == G4_dph) {
    return 4;
  } else if (op == G4_line || op == G4_pln) {
    return 8;
  } else if (op == G4_sad2 || op == G4_sada2) {
    return 2;
  } else
    return 1;
}

void HWConformity::fixOpndTypeAlign(G4_BB *bb) {
  INST_LIST_ITER i = bb->begin();
  INST_LIST_ITER next_iter = i;
  bool needSplit = false;

  for (auto iEnd = bb->end(); i != iEnd; i = next_iter) {
    G4_INST *inst = *i;
    G4_opcode opcode = inst->opcode();
    if (opcode == G4_nop || opcode == G4_label || inst->isSend() ||
        inst->isDpas()) {
      next_iter++;
    } else if (fixInstOpndTypeAlign(i, bb)) {
      needSplit = true;
      next_iter = i;
      next_iter++;
    } else {
      next_iter++;
    }
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
  }

  if (needSplit) {
    // make sure updated insts and new moves don't cross 2 GRF
    InstSplitPass instSplitter(&builder);
    instSplitter.runOnBB(bb);
  }
}

// Fix instructions with vector immediate as source operands.
//    mov (8) r5.0<2>:uw 0xfdb97531:uv {Align1}
// becomes
//    mov (8) r6.0<1>:uw 0xfdb97531:uv {Align1}
//    mov (8) r5.0<2>:uw r6.0<8;8,1>:uw {Align1, Q1}
//
// When an immediate vector is used in an instruction, the destination must
// be 128-bit aligned with destination horizontal stride equivalent to a
// word for an immediate integer vector (v) and equivalent to a DWord for an
// immediate float vector (vf).
// For Xe2+, :vf datatype is not supported and :v and :uv must not be used
// when destination is any of the float datatypes.
bool HWConformity::fixDstAlignmentWithVectorImm(INST_LIST_ITER iter,
                                                G4_BB *bb) {
  bool changed = false;
  G4_INST *inst = *iter;
  G4_DstRegRegion *reg = inst->getDst();
  uint8_t execSize = inst->getExecSize();

  bool dstAligned = builder.tryToAlignOperand(reg, 16);

  unsigned hsInBytes = reg->getHorzStride() * reg->getTypeSize();
  for (int k = 0, e = inst->getNumSrc(); k < e; ++k) {
    G4_Operand *src = inst->getSrc(k);
    if (!src->isVectImm())
      continue;

    G4_Type ty = src->getType();
    if (!builder.hasPackedRestrictedFloatVector())
      vISA_ASSERT(ty != Type_VF,
                  ":vf datatype is not supported on this platform");
    G4_Type moveTy =
        (ty == Type_V) ? Type_W : (ty == Type_UV) ? Type_UW : Type_F;
    if (!dstAligned ||
        (builder.getPlatform() >= Xe2 && IS_TYPE_FLOAT_ALL(reg->getType()))) {
      inst->setSrc(insertMovBefore(iter, k, moveTy, bb), k);
      changed = true;
    } else if (hsInBytes != TypeSize(moveTy)) {
      if (hsInBytes == 4 && execSize < 8) {
        // for the case where dst is dword and execution size is < 8,
        // we can interleave the vector to avoid a move
        // e.g., mov (2) r1.0<1>:d 0x21:uv  -->
        //       mov (2) r1.0<1>:d 0x0201:uv
        uint64_t bitValue = 0;
        uint32_t immBits = static_cast<uint32_t>(src->asImm()->getImm());
        for (int i = 0; i < execSize; ++i) {
          uint64_t val = (immBits >> (i * 4)) & 0xF;
          bitValue |= val << (i * 8);
        }
        inst->setSrc(builder.createImm(bitValue, ty), k);
      } else {
        inst->setSrc(insertMovBefore(iter, k, moveTy, bb), k);
        changed = true;
      }
    }
  }

  return changed;
}

// Do basic HW conformity check related to operand type and dst alignment before
// resucing execution size to avoid splitting of the MOV inserted in this stage.
// This function is called for some instructions generated in later stages.
bool HWConformity::fixInstOpndTypeAlign(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  bool insertedInst = false;

  if (inst->opcode() == G4_srnd) {
    // Operands can be packed.
    return false;
  }

  int extypesize = 0;
  G4_Type extype = inst->getOpExecType(extypesize);

  if (extypesize == kernel.numEltPerGRF<Type_UB>() / 2 &&
      inst->opcode() != G4_mov) {
    fixPackedSource(i, bb);
    extype = inst->getOpExecType(extypesize);
  }

  // fixes opernds including
  // swapping sel,
  fixOpnds(i, bb, extype);

  extype = inst->getOpExecType(extypesize);
  if (inst->getDst() && !(inst->isSend()) && !(inst->isRawMov())) {
    if (extypesize < (int)kernel.numEltPerGRF<Type_UB>() / 2) {
      uint32_t dst_elsize = inst->getDst()->getTypeSize();
      if (dst_elsize < (unsigned int)extypesize ||
          // indirect float type needs to be handled as well.
          // See fixDstAlignment for detail
          ((extype == Type_F || extype == Type_HF) &&
           inst->getDst()->getRegAccess() != Direct)) {
        if (fixDstAlignment(i, bb, extype, dst_elsize)) {
          insertedInst = true;
        }
      }
    }

    // There are vector immediate source operands.
    if ((*i)->hasVectImm()) {
      if ((insertedInst = fixDstAlignmentWithVectorImm(i, bb))) {
        // Recompute the execution type size if there is some change.
        // This allows fixDstAlignment to fix possible conformity issues.
        extype = inst->getOpExecType(extypesize);
        uint32_t dst_elsize = inst->getDst()->getTypeSize();
        if (dst_elsize < unsigned(extypesize)) {
          if (fixDstAlignment(i, bb, extype, dst_elsize)) {
            insertedInst = true;
          }
        }
      }
    }
  }

  return insertedInst;
}

// check Rule 2H
// VertStride must be used to cross GRF register boundaries. This rule implies
// that elements within a 'Width' cannot cross GRF boundaries. This is a
// separate function from fixSrcRegion because we may need to split the
// instruction to satisfy this rule
bool HWConformity::checkSrcCrossGRF(INST_LIST_ITER &iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion()) {
      G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
      bool widthCrossingGRF = false;
      const RegionDesc *srcRegion = src->getRegion();
      uint16_t vs = srcRegion->vertStride, wd = srcRegion->width,
               hs = srcRegion->horzStride;
      uint8_t exSize = inst->getExecSize();
      if (src->getRegAccess() == Direct && src->crossGRF(builder)) {
        int elementSize = src->getTypeSize();
        int startOffset = src->getLeftBound() % kernel.numEltPerGRF<Type_UB>();
        for (int row = 0; row < exSize / wd; row++) {
          int rowOffset = (startOffset + row * vs * elementSize) %
                          kernel.numEltPerGRF<Type_UB>();
          if (rowOffset + (wd - 1) * hs * elementSize >=
              (int)kernel.numEltPerGRF<Type_UB>()) {
            widthCrossingGRF = true;
            break;
          }
        }
      } else if (src->getRegAccess() == IndirGRF) {
        widthCrossingGRF = wd > 1 && hs != 0;
      }

      auto doSplit = [&](bool canCrossGRF) -> void {
        if (inst->usesFlag() ||
            (!bb->isAllLaneActive() && !inst->isWriteEnableInst())) {
          // splitting may be unsafe, insert a move then split the move
          G4_Operand *newSrc =
              insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb);
          inst->setSrc(newSrc, i);
          auto movIter = iter;
          --movIter;
          splitInstruction(movIter, bb, false, 0, false, canCrossGRF);
        } else {
          splitInstruction(iter, bb, false, 0, false, canCrossGRF);
        }
      };

      if (widthCrossingGRF) {
        uint16_t stride = 0;
        if (srcRegion->isSingleStride(exSize, stride)) {
          // replace <v;w,h> with <h;1,0>
          src->setRegion(builder, builder.createRegionDesc(stride, 1, 0), true);
        } else {
          doSplit(true);
          return true;
        }
      } else if (kernel.getKernelType() == VISA_CM &&
                 builder.no64bitRegioning() && src->getTypeSize() == 8) {
        // for CM, split non-scalar, non-contiguous source that cross GRF as HW
        // conformity may be not equipped to deal with them later
        const RegionDesc *region = src->getRegion();
        if (!region->isScalar() && !region->isContiguous(inst->getExecSize()) &&
            src->crossGRF(builder)) {
          doSplit(false);
          return true;
        }
      }
    }
  }

  return false;
}

void HWConformity::fixInstExecSize(G4_BB *bb) {
#ifdef _DEBUG
  verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

  INST_LIST_ITER i = bb->begin();
  INST_LIST_ITER next_iter = i;

  for (; i != bb->end(); i = next_iter) {
    next_iter++;
    G4_INST *inst = *i;
    G4_opcode opcode = inst->opcode();
    if (opcode == G4_nop || opcode == G4_label || inst->isSend() ||
        inst->isDpas()) {
      continue;
    }

    if (reduceExecSize(i, bb)) {
      next_iter = i;
      next_iter++;
    }
  }
}
// split CISA instructions to follow Gen register region restriction
// splitOp returns true if inst is split into more than instructions
bool HWConformity::reduceExecSize(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  // Madw can't be split in any pass except for fixMadwInst as it will cause the
  // dst(SOA layout) unexpected. For example:
  //    madw (M1, 16) dst(0,0)<1> src0(0,0)<1;1,0> 0x38:ud 0x0:ud
  // If split here, then low result is in dst(0,0) and dst(2,0), and high result
  // is in dst(1,0) and dst(3,0)
  //    madw (M1, 8) dst(0,0)<1> src0(0,0)<1;1,0> 0x38:ud 0x0:ud
  //    madw (M8, 8) dst(2,0)<1> src0(1,0)<1;1,0> 0x38:ud 0x0:ud
  // But expected dst is low result is in dst(0,0) and dst(1,0) and high result
  // is in dst(2,0) and dst(3,0)
  if (!inst || inst->isSend() || inst->getExecSize() == 1 ||
      inst->opcode() == G4_madw) {
    return false;
  }

  unsigned char execSize = inst->getExecSize();

  // rules specific to math instructions
  // INT DIV function does not support SIMD16
  if (inst->isMath() && inst->asMathInst()->isMathIntDiv() && execSize == 16) {
    return reduceExecSizeForMath(iter, bb);
  }

  // SKL removes rules for GRF alignments, so we don't have to check whether
  // the dst or src is evenly split anymore This means that any subreg of
  // source can move to any subreg of dst
  return checkSrcCrossGRF(iter, bb);
}

// split a SIMD32 inst into two SIMD16.
// there is predicate/conditional modifier used in this inst.
//
// Result:
//    Inst refered to by 'iter' is split into two simd16 instrcutions.
//    One is inserted right before 'iter', the other is to reuse 'iter'.
// And the caller of this function can access two new instructions via '--iter'
// and 'iter' !
void HWConformity::splitSIMD32Inst(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  G4_opcode op = inst->opcode();
  G4_Operand *srcs[3] = {nullptr};
  int numSrc = inst->getNumSrc();

  // check dst/src dependency
  checkSrcDstOverlap(iter, bb, false);
  for (int i = 0; i < numSrc; i++) {
    srcs[i] = inst->getSrc(i);
  }

  // compute max exeuction size.
  // boundary is GRF-boundary and HS change, but for Dst, elements should be
  // symetric if half-GRF boundary is crossed.
  G4_DstRegRegion *dst = inst->getDst();
  vASSERT(nullptr != dst);
  bool nullDst = inst->hasNULLDst();
  G4_ExecSize instExSize = inst->getExecSize(),
              currExSize = G4_ExecSize(instExSize / 2);
  for (int i = 0; i < instExSize; i += currExSize) {
    // create new Oprands. Acc should not be split since we generate it in
    // jitter and can control this. create new condMod and predicate
    G4_CondMod *newCondMod = inst->getCondMod();
    if (newCondMod) {
      newCondMod = builder.createCondMod(newCondMod->getMod(),
                                         newCondMod->getBase(), i == 0 ? 0 : 1);
    }

    G4_Predicate *newPredOpnd = inst->getPredicate();
    if (newPredOpnd) {
      newPredOpnd = builder.createPredicate(
          newPredOpnd->getState(), newPredOpnd->getBase(), i == 0 ? 0 : 1,
          newPredOpnd->getControl());
    }

    G4_DstRegRegion *newDst;
    if (!nullDst) {
      newDst = builder.createSubDstOperand(dst, (uint16_t)i, currExSize);
    } else {
      newDst = dst;
    }
    // generate new inst
    G4_INST *newInst;
    if ((i + currExSize) < instExSize) {
      // 1st simd16 (M0)
      newInst = builder.makeSplittingInst(inst, currExSize);
      newInst->setDest(newDst);
      newInst->setPredicate(newPredOpnd);
      newInst->setCondMod(newCondMod);
      bb->insertBefore(iter, newInst);
    } else {
      // 2nd simd16 (M16). reuse the original inst and may need to reset mask
      // offset.
      if (!inst->isWriteEnableInst() || newPredOpnd || newCondMod) {
        inst->setMaskOption(InstOpt_M16);
      }
      newInst = inst;
      newInst->setExecSize(currExSize);
      newInst->setDest(newDst);
      newInst->setPredicate(newPredOpnd);
      newInst->setCondMod(newCondMod);
    }

    for (int j = 0; j < numSrc; j++) {
      if (srcs[j]) {
        // src1 for single source math should be arc reg null.
        if (srcs[j]->isImm() ||
            (inst->opcode() == G4_math && j == 1 && srcs[j]->isNullReg())) {
          newInst->setSrc(srcs[j], j);
        } else if (srcs[j]->asSrcRegRegion()->isScalar() ||
                   (j == 0 && op == G4_line)) {
          newInst->setSrc(builder.duplicateOperand(srcs[j]), j);
        } else {
          newInst->setSrc(
              builder.createSubSrcOperand(
                  srcs[j]->asSrcRegRegion(), (uint16_t)i, currExSize,
                  (uint8_t)(srcs[j]->asSrcRegRegion()->getRegion()->vertStride),
                  (uint8_t)(srcs[j]->asSrcRegRegion()->getRegion()->width)),
              j);
        }
      }
    }

    // maintain def-use chain
    if (newInst == inst) {
      newInst->trimDefInstList();
    } else {
      // Defs (uses) of this new instruction will be a subset of the
      // original instruction's defs (uses).
      inst->copyDefsTo(newInst, true);
      inst->copyUsesTo(newInst, true);
    }
  }
}

void HWConformity::splitInstruction(INST_LIST_ITER iter, G4_BB *bb,
                                    bool compOpt, uint8_t numInFirstMov,
                                    bool rule4_11, bool canSrcCrossGRF) {
  G4_INST *inst = *iter;
  G4_opcode op = inst->opcode();
  G4_Operand *srcs[3] = {nullptr};

  // check dst/src dependency
  checkSrcDstOverlap(iter, bb, compOpt);

  int numSrcs = inst->getNumSrc();

  for (int i = 0; i < numSrcs; i++) {
    srcs[i] = inst->getSrc(i);
  }

  uint8_t minExSize = checkMinExecSize(op);
  // compute max exeuction size.
  // boundary is GRF-boundary and HS change, but for Dst, elements should be
  // symetric if half-GRF boundary is crossed.
  G4_DstRegRegion *dst = inst->getDst();
  bool nullDst = inst->hasNULLDst();
  G4_ExecSize instExSize = inst->getExecSize();
  G4_ExecSize currExSize;
  uint16_t vs[3] = {0}, wd[3] = {0};

  G4_Predicate *instPred = inst->getPredicate();

  // first, produce mask if needed
  // mov (16) r2.0<1>:uw 0:uw {Align1, NoMask}   // 0:uw
  // mov (16) r2.0<1>:uw 0x1:uw {Align1}   // 1:uw
  // this part is currently not used since we do not split inst with predicate
  // or emask
  bool isSIMDCFInst = !bb->isAllLaneActive() && !inst->isWriteEnableInst();
  G4_Declare *maskDcl = NULL;
  if (instPred || isSIMDCFInst) {
    maskDcl = builder.createTempVar(instExSize, Type_UW, Eight_Word);
    G4_DstRegRegion *tmpMaskOpnd =
        builder.createDst(maskDcl->getRegVar(), 0, 0, 1, Type_UW);

    G4_INST *firstMov = builder.createMov(instExSize, tmpMaskOpnd,
                                          builder.createImm(0, Type_UW),
                                          inst->getOption(), false);

    G4_Predicate *pred = builder.duplicateOperand(inst->getPredicate());
    auto movInst = builder.createMov(instExSize, tmpMaskOpnd,
                                     builder.createImm(1, Type_UW),
                                     inst->getOption(), false);
    movInst->setPredicate(pred);

    if (isSIMDCFInst) {
      firstMov->setNoMask(true);
    }
  }

  bool needsMaskOffset =
      instPred || isSIMDCFInst || inst->getCondMod() != nullptr;

  for (uint8_t i = 0; i < instExSize; i += currExSize) {
    if (compOpt && i == 0) {
      currExSize = G4_ExecSize(numInFirstMov);
      G4_INST *newInst = builder.makeSplittingInst(inst, instExSize);
      newInst->setDest(builder.duplicateOperand(inst->getDst()));
      newInst->setPredicate(builder.duplicateOperand(inst->getPredicate()));
      newInst->setCondMod(builder.duplicateOperand(inst->getCondMod()));
      for (int j = 0; j < inst->getNumSrc(); j++) {
        newInst->setSrc(builder.duplicateOperand(srcs[j]), j);
      }
      // update def-use chain
      inst->copyDefsTo(newInst, true);
      inst->copyDefsTo(newInst, true);
      bb->insertBefore(iter, newInst);
      continue;
    }

    // this stores the max allowed exec size for each operand (0 -- dst, 1 --
    // src0, and so on)
    uint8_t opndExSize[4] = {0, 0, 0, 0};
    currExSize = G4_ExecSize(roundDownPow2(instExSize - i));

    bool crossGRFsrc = false;
    for (int j = 0; j < numSrcs; j++) {
      if (!srcs[j] || !srcs[j]->isSrcRegRegion() || srcs[j]->isNullReg() ||
          (j == 0 && op == G4_line)) {
        opndExSize[j + 1] = currExSize;
        continue;
      }
      bool twoGRFsrc = false;
      opndExSize[j + 1] = srcs[j]->asSrcRegRegion()->getMaxExecSize(
          builder, i, currExSize, canSrcCrossGRF, vs[j], wd[j], twoGRFsrc);

      if (opndExSize[j + 1] > 8 && rule4_11) {
        opndExSize[j + 1] = 8;
      }

      crossGRFsrc |= twoGRFsrc;
      if (minExSize == 1) {
        currExSize = G4_ExecSize(opndExSize[j + 1]);
      }
    }

    vASSERT(dst);
    if (!nullDst) {
      opndExSize[0] = dst->getMaxExecSize(builder, i, currExSize, crossGRFsrc);

      if (opndExSize[0] > 8 && rule4_11)
        opndExSize[0] = 8;
    } else {
      // dst essentially does not affect the splitting decision
      opndExSize[0] = currExSize;
    }

    if (minExSize == 1) {
      currExSize = G4_ExecSize(opndExSize[0]);
    }

    bool needMov = false;
    if (minExSize > 1) {
      // find minimal execsize. if it is not less than minExSize, use it
      // to avoid dependency
      // FIXME: optimize this part by avoiding MOVs
      uint8_t currMinExSize = 64;
      currExSize = G4_ExecSize(0);
      for (int j = 0; j <= numSrcs; j++) {
        // use max possible exsize
        if (opndExSize[j] > currExSize) {
          currExSize = G4_ExecSize(opndExSize[j]);
        }
        if (opndExSize[j] != 0 && opndExSize[j] < currMinExSize) {
          currMinExSize = opndExSize[j];
        }
      }

      if (currMinExSize >= minExSize) {
        currExSize = G4_ExecSize(currMinExSize);
      } else {
        for (int j = 0; j <= numSrcs; j++) {
          if (opndExSize[j] != 0 && opndExSize[j] < currExSize) {
            needMov = true;
          }
        }
      }
    }

    vISA_ASSERT(currExSize != 0,
                "illegal execution size in instruction splitting");
    // create new Oprands. Acc should not be split since we generate it in
    // jitter and can control this.
    G4_DstRegRegion *newDst =
        !nullDst ? builder.createSubDstOperand(dst, (uint16_t)i, currExSize)
                 : dst;

    // generate new inst
    G4_INST *newInst;
    INST_LIST_ITER newInstIter;
    if ((i + currExSize) < instExSize) {
      newInst = builder.makeSplittingInst(inst, currExSize);
      newInst->setDest(newDst);
      newInst->setPredicate(builder.duplicateOperand(inst->getPredicate()));
      newInst->setCondMod(builder.duplicateOperand(inst->getCondMod()));
      bb->insertBefore(iter, newInst);
      newInstIter = iter;
      newInstIter--;
    } else {
      // reuse the original inst
      newInst = inst;
      newInst->setDest(newDst);
      newInst->setExecSize(currExSize);
      newInstIter = iter;
    }

    for (int j = 0; j < inst->getNumSrc(); j++) {
      if (srcs[j]) {
        // src1 for single source math should be arc reg null.
        if (srcs[j]->isImm() ||
            (inst->opcode() == G4_math && j == 1 && srcs[j]->isNullReg())) {
          newInst->setSrc(srcs[j], j);
        } else if (srcs[j]->asSrcRegRegion()->isScalar() ||
                   (j == 0 && op == G4_line)) {
          newInst->setSrc(builder.duplicateOperand(srcs[j]), j);
        } else {
          if (srcs[j]->isAddrExp()) {
            G4_AddrExp *addExp =
                builder.createAddrExp(srcs[j]->asAddrExp()->getRegVar(),
                                      srcs[j]->asAddrExp()->getOffset(),
                                      srcs[j]->asAddrExp()->getType());
            newInst->setSrc(addExp, j);
          } else {
            uint16_t start = i;
            newInst->setSrc(
                builder.createSubSrcOperand(srcs[j]->asSrcRegRegion(), start,
                                            currExSize, vs[j], wd[j]),
                j);
          }
        }
      }
    }

    if (instExSize == 16 && currExSize == 8 && needsMaskOffset) {
      if (instPred) {
        G4_Predicate *tPred = builder.duplicateOperand(instPred);
        tPred->setInst(newInst);
        newInst->setPredicate(tPred);
      }

      if (newInst->getMaskOffset() == 0) {
        newInst->setMaskOption(i == 0 ? InstOpt_M0 : InstOpt_M8);
      } else {
        newInst->setMaskOption(i == 0 ? InstOpt_M16 : InstOpt_M24);
      }
    }

    // maintain def-use chain
    if (newInst == inst) {
      newInst->trimDefInstList();
    } else {
      inst->copyDefsTo(newInst, /*checked*/ true);
      inst->copyUsesTo(newInst, /*checked*/ true);
    }

    // the following code is to keep minimal execution size for some opcode, for
    // example, DP4 insert mov if needed
    if (needMov) {
      for (int j = 0; j < inst->getNumSrc(); j++) {
        if (opndExSize[j + 1] < currExSize) {
          newInst->setSrc(
              insertMovBefore(newInstIter, j, srcs[j]->getType(), bb), j);
          // reducing exec size for new MOV
          INST_LIST_ITER newMovIter = newInstIter;
          newMovIter--;
          reduceExecSize(newMovIter, bb);
        }
      }
    }
    // dst
    if (needMov && opndExSize[0] < currExSize) {
      (*newInstIter)
          ->setDest(insertMovAfter(newInstIter, inst->getDst(),
                                   inst->getDst()->getType(), bb));
      INST_LIST_ITER newMovIter = newInstIter;
      newMovIter++;
      reduceExecSize(newMovIter, bb);
    }
  }
}

// evenly split an inst into two instructions with half execution size.
// this is used to split a simd16 math into two simd8 before other reducing
// exeuction size actions
//
// This will has two instructions: one is right before "iter", the other is to
// re-use "iter". The caller is safe to use "--iter" and "iter" to refer those
// two instructions.
bool HWConformity::evenlySplitInst(INST_LIST_ITER iter, G4_BB *bb,
                                   bool checkOverlap) {
  G4_INST *inst = *iter;
  G4_opcode op = inst->opcode();
  G4_Operand *srcs[3];
  int origMaskOffset = inst->getMaskOffset();
  bool extraMov = false;
  const int numSrc = inst->getNumSrc();

  // check dst/src dependency
  if (checkOverlap) {
    extraMov = checkSrcDstOverlap(iter, bb, false);
  }

  bool useARF = false;
  for (int i = 0; i < numSrc; i++) {
    srcs[i] = inst->getSrc(i);
  }

  // compute max exeuction size.
  // boundary is GRF-boundary and HS change, but for Dst, elements should be
  // symetric if half-GRF boundary is crossed.

  G4_DstRegRegion *dst = inst->getDst();
  vASSERT(nullptr != dst);
  bool nullDst = inst->hasNULLDst();
  G4_ExecSize instExSize = inst->getExecSize(),
              currExSize = G4_ExecSize(instExSize / 2);

  G4_Predicate *newPred = NULL;
  if (inst->getPredicate()) {
    newPred = inst->getPredicate();
    newPred->splitPred();
  }

  G4_CondMod *newCond = NULL;
  if (inst->getCondMod()) {
    newCond = inst->getCondMod();
    newCond->splitCondMod();
  }

  G4_SrcRegRegion *accSrcRegion = NULL;
  if (inst->getImplAccSrc()) {
    accSrcRegion = inst->getImplAccSrc()->asSrcRegRegion();
  }

  G4_DstRegRegion *accDstRegion = NULL;
  if (inst->getImplAccDst()) {
    accDstRegion = inst->getImplAccDst();
  }

  if (accSrcRegion || accDstRegion || newPred || newCond) {
    useARF = true;
  }

  for (int i = 0; i < instExSize; i += currExSize) {
    // create new Oprands.
    G4_DstRegRegion *newDst;
    if (!nullDst) {
      newDst = builder.createSubDstOperand(dst, (uint16_t)i, currExSize);
    } else {
      newDst = dst;
    }
    // generate new inst
    G4_INST *newInst;
    if ((i + currExSize) < instExSize) {
      newInst = builder.makeSplittingInst(inst, currExSize);
      if (accDstRegion)
        newInst->setImplAccDst(builder.duplicateOperand(accDstRegion));
      if (accSrcRegion)
        newInst->setImplAccSrc(builder.duplicateOperand(accSrcRegion));
      newInst->setDest(newDst);
      newInst->setPredicate(builder.duplicateOperand(newPred));
      newInst->setCondMod(builder.duplicateOperand(newCond));
      newInst->setEvenlySplitInst(true);
      bb->insertBefore(iter, newInst);
    } else {
      // reuse the original inst
      newInst = inst;
      newInst->setExecSize(currExSize);
      newInst->setDest(newDst);
      if (newPred) {
        inst->setPredicate(builder.duplicateOperand(newPred));
      }
      if (newCond) {
        inst->setCondMod(builder.duplicateOperand(newCond));
      }
      if (accSrcRegion) {
        newInst->setImplAccSrc(builder.createSrcRegRegion(*accSrcRegion));
      }
      if (accDstRegion) {
        newInst->setImplAccDst(builder.createDstRegRegion(*accDstRegion));
      }
    }

    for (int j = 0; j < numSrc; j++) {
      if (srcs[j]) {
        if (srcs[j]->isImm() || srcs[j]->isNullReg()) {
          newInst->setSrc(srcs[j], j);
        } else if (srcs[j]->isScalarSrc() || (j == 0 && op == G4_line)) {
          // no need to split, but need to duplicate
          newInst->setSrc(builder.duplicateOperand(srcs[j]), j);
        } else if (op == G4_movi) {
       // we create temp region which is in VxH format
          RegionDesc VxHregionDesc = RegionDesc(UNDEFINED_SHORT, 1, 0);

          auto tempRegion = G4_SrcRegRegion(*srcs[j]->asSrcRegRegion());
          tempRegion.setRegion(builder, &VxHregionDesc);

          newInst->setSrc(builder.createSubSrcOperand(
                              &tempRegion, (uint16_t)i, currExSize,
                              (uint8_t)(tempRegion.getRegion()->vertStride),
                              (uint8_t)(tempRegion.getRegion()->width)),
                          j);

          // restore the original region
          const RegionDesc *rd = builder.getRegionStride1();
          newInst->getSrc(j)->asSrcRegRegion()->setRegion(builder, rd);
        } else {
          newInst->setSrc(
              builder.createSubSrcOperand(
                  srcs[j]->asSrcRegRegion(), (uint16_t)i, currExSize,
                  (uint8_t)(srcs[j]->asSrcRegRegion()->getRegion()->vertStride),
                  (uint8_t)(srcs[j]->asSrcRegRegion()->getRegion()->width)),
              j);
        }
      }
    }

    // set mask
    bool needsMaskOffset =
        useARF || (!bb->isAllLaneActive() && !inst->isWriteEnableInst());
    if (needsMaskOffset) {
      int newMaskOffset = origMaskOffset + (i == 0 ? 0 : currExSize);
      bool nibOk =
          builder.hasNibCtrl() && (inst->getDst()->getTypeSize() == 8 ||
                                   TypeSize(inst->getExecType()) == 8);
      G4_InstOption newMask =
          G4_INST::offsetToMask(currExSize, newMaskOffset, nibOk);
      if (newMask == InstOpt_NoOpt) {
        [[maybe_unused]] bool useMask =
            inst->getPredicate() || inst->getCondModBase() ||
            (!bb->isAllLaneActive() && !inst->isWriteEnableInst());
        vISA_ASSERT(!useMask, "no legal emask found for the split instruction");
      } else {
        newInst->setMaskOption(newMask);
      }
    }

    // maintain def-use chain
    if (newInst == inst) {
      newInst->trimDefInstList();
    } else {
      inst->copyDefsTo(newInst, /*checked*/ true);
      inst->copyUsesTo(newInst, /*checked*/ true);
    }
  }

  return extraMov;
}

// this is specifically for math instruction
// assumption: the input math function is a compressed instruction and need
// split
bool HWConformity::reduceExecSizeForMath(INST_LIST_ITER iter, G4_BB *bb) {
  // split the instruction into two first
  evenlySplitInst(iter, bb);
  // fix execution size for each one
  INST_LIST_ITER firstIter = iter;
  firstIter--;
  reduceExecSize(firstIter, bb);
  return reduceExecSize(iter, bb);
}
// check overlap between src and dst
// if overlap exists, insert to MOV to eliminate it
// how about replicate regions?<0;4,1>
bool HWConformity::checkSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb,
                                      bool compOpt) {
  G4_INST *inst = *iter;
  G4_Operand *srcs[3];
  bool hasOverlap = false;

  for (int i = 0; i < inst->getNumSrc(); i++) {
    srcs[i] = inst->getSrc(i);
  }
  // check dst/src dependency
  // how about replicate regions?<0;4,1>
  if (inst->getDst() && !inst->hasNULLDst()) {
    for (int i = 0; i < inst->getNumSrc(); i++) {
      bool useTmp = false;
      if (srcs[i] &&
          (IS_VINTTYPE(srcs[i]->getType()) || IS_VFTYPE(srcs[i]->getType()))) {
        useTmp = true;
      } else {
        G4_CmpRelation rel = inst->getDst()->compareOperand(srcs[i], builder);
        if (rel != Rel_disjoint) {
          useTmp = (rel != Rel_eq) || compOpt ||
                   srcs[i]->asSrcRegRegion()->getRegion()->isRepeatRegion(
                       inst->getExecSize());
        }
      }
      if (useTmp) {
        if (inst->opcode() == G4_movi) {
          // movi requires an indirect src operand. Inserting mov on src
          // would strip the indirect addressing and break the instruction.
          // Instead, redirect the dst to a tmp and insert "mov real_dst, tmp"
          // after the movi to eliminate the overlap.
          insertMovAfter(iter, inst->getDst()->getHorzStride(), bb);
          hasOverlap = true;
          break;
        }
        // insert mov
        inst->setSrc(
            insertMovBefore(iter, i,
                            G4_Operand::GetNonVectorImmType(srcs[i]->getType()),
                            bb),
            i);
        srcs[i] = inst->getSrc(i);
        // reducing exec size for new MOV
        INST_LIST_ITER newMovIter = iter;
        newMovIter--;
        reduceExecSize(newMovIter, bb);
        hasOverlap = true;
      }
    }
  }

  return hasOverlap;
}

// move source operand to one or two GRF
// tmp dst use the same type as source.
// this MOV does not need further resucing execsize
void HWConformity::moveSrcToGRF(INST_LIST_ITER it, uint32_t srcNum,
                                uint16_t numGRF, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_ExecSize execSize = inst->getExecSize();

  G4_Operand *src = inst->getSrc(srcNum);
  uint32_t srcTypeSize = src->getTypeSize();
  uint16_t dclSize = (kernel.numEltPerGRF<Type_UB>() * numGRF) / srcTypeSize;
  uint16_t hs = dclSize / execSize;
  uint16_t wd = execSize;
  uint16_t vs = hs * wd;
  const RegionDesc *region = builder.createRegionDesc(vs, wd, hs);

  // look up in MOV table to see if there is already inserted MOV for this
  // source.
  G4_INST *def_inst = NULL;
  def_inst = checkSrcDefInst(inst, def_inst, srcNum);

  G4_Type tmpType = G4_Operand::GetNonVectorImmType(src->getType());

  if (def_inst && def_inst->getDst()->getType() == tmpType &&
      (def_inst->getExecSize() == execSize) &&
      def_inst->getDst()->coverGRF(builder, numGRF, execSize) &&
      def_inst->getDst()->checkGRFAlign(builder) &&
      (bb->isAllLaneActive() || def_inst->isWriteEnableInst())) {
    G4_DstRegRegion *existing_def = def_inst->getDst();
    G4_SrcRegRegion *newSrc =
        builder.createSrc(existing_def->getBase(), existing_def->getRegOff(),
                          existing_def->getSubRegOff(), region, src->getType());
    inst->setSrc(newSrc, srcNum);
  }

  G4_Declare *dcl =
      builder.createTempVar(dclSize, src->getType(), builder.getGRFAlign());
  G4_DstRegRegion *dstRegion =
      builder.createDst(dcl->getRegVar(), 0, 0, hs, dcl->getElemType());
  G4_INST *newInst = builder.createMov(
      execSize, dstRegion, src,
      (!bb->isAllLaneActive() ? InstOpt_WriteEnable : InstOpt_NoOpt), false);

  // insert instruction and maintain def-use chain
  bb->insertBefore(it, newInst);
  inst->transferDef(newInst, Gen4_Operand_Number(srcNum + 1), Opnd_src0);
  newInst->addDefUse(inst, Gen4_Operand_Number(srcNum + 1));

  G4_SrcRegRegion *newSrc =
      builder.createSrc(dcl->getRegVar(), 0, 0, region, dcl->getElemType());
  inst->setSrc(newSrc, srcNum);
}

/*
 *  create a new mov instruction and insert it before iter
 *  mov (esize) tmpDst dst (nomask)
 *  add (esize) tmpDst ...
 *  where esize is "inst"'s execution size
 *
 */
void HWConformity::saveDst(INST_LIST_ITER &it, uint8_t stride, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  G4_ExecSize execSize = inst->getExecSize();
  G4_Type dstType = dst->getType();
  uint16_t dstWidthBytes = execSize * TypeSize(dstType) * stride;

  G4_SubReg_Align subAlign =
      getDclAlignment(dstWidthBytes, inst, execSize == 1);

  uint32_t numElt = execSize == 1 ? 1 : execSize * stride;
  G4_Declare *dcl = builder.createTempVar(numElt, dstType, subAlign);

  uint16_t hs = dst->getHorzStride();
  const RegionDesc *region =
      builder.createRegionDesc(hs * execSize, execSize, hs);
  G4_SrcRegRegion *srcRegion = builder.createSrc(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff(), region, dstType);

  G4_DstRegRegion *tmpDstOpnd = builder.createDstRegRegion(dcl, stride);

  unsigned int new_option = inst->getOption();

  G4_INST *newInst =
      builder.createMov(execSize, tmpDstOpnd, srcRegion, new_option, false);
  newInst->setNoMask(true);

  bb->insertBefore(it, newInst);
  inst->setDest(builder.duplicateOperand(tmpDstOpnd));
}

void HWConformity::restoreDst(INST_LIST_ITER &it, G4_DstRegRegion *origDst,
                              G4_BB *bb) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  G4_ExecSize execSize = inst->getExecSize();

  uint16_t hs = dst->getHorzStride();
  const RegionDesc *region =
      builder.createRegionDesc(hs * execSize, execSize, hs);
  G4_SrcRegRegion *srcRegion =
      builder.createSrc(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                        region, dst->getType());

  unsigned int new_option = inst->getOption();

  G4_INST *newInst =
      builder.createMov(execSize, origDst, srcRegion, new_option, false);
  newInst->setNoMask(true);

  INST_LIST_ITER iter = it;
  iter++;
  bb->insertBefore(iter, newInst);

  // how about def-use?
  inst->transferUse(newInst);
  inst->addDefUse(newInst, Gen4_Operand_Number::Opnd_src0);
}

/*
 *  create a new mov instruction and insert it after iter
 *  mov (esize) dst tmp:dst_type
 *  where esize is "inst"'s execution size and insert it after "inst"
 *  dst of inst is replaced with the tmp dst using the same type
 */
void HWConformity::insertMovAfter(INST_LIST_ITER &it, uint16_t stride,
                                  G4_BB *bb) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  G4_ExecSize execSize = inst->getExecSize();
  G4_Type execType = inst->getExecType(), dstType = dst->getType();
  uint16_t opExecWidthBytes = execSize * TypeSize(execType);
  uint16_t dstWidthBytes = execSize * TypeSize(dstType) * stride;

  G4_SubReg_Align subAlign = getDclAlignment(
      opExecWidthBytes > dstWidthBytes ? opExecWidthBytes : dstWidthBytes, inst,
      execSize == 1);

  G4_Declare *dcl = builder.createTempVar(execSize * stride, dstType, subAlign);

  const RegionDesc *region = builder.createRegionDesc(stride, 1, 0);
  G4_SrcRegRegion *srcRegion = builder.createSrcRegRegion(dcl, region);
  G4_DstRegRegion *tmpDstOpnd = builder.createDstRegRegion(dcl, stride);

  G4_Predicate *pred = NULL;
  if (inst->opcode() != G4_sel) {
    pred = inst->getPredicate();
    inst->setPredicate(NULL);
  }
  unsigned int new_option = inst->getOption();

  G4_INST *newInst =
      builder.createMov(execSize, dst, srcRegion, new_option, false);
  newInst->setPredicate(pred);

  INST_LIST_ITER iter = it;
  iter++;
  bb->insertBefore(iter, newInst);
  // change dst of inst
  inst->setDest(tmpDstOpnd);

  // update propagation info
  if (pred) {
    inst->transferDef(newInst, Opnd_pred, Opnd_pred);
  }

  inst->transferUse(newInst);
  inst->addDefUse(newInst, Opnd_src0);
}

void HWConformity::removeBadSrc(INST_LIST_ITER &iter, G4_BB *bb,
                                bool crossGRFDst, bool oneGRFSrc[3],
                                bool badTwoGRFSrc[3]) {
  G4_INST *inst = *iter;
  G4_Operand *dst = inst->getDst();
  // check source and dst region together
  // get rid of bad two-GRF source
  for (int i = 0; i < inst->getNumSrc(); i++) {

    if (badTwoGRFSrc[i]) {
      if (!crossGRFDst || (dst && IS_DTYPE(dst->getType()) &&
                           IS_WTYPE(inst->getSrc(i)->getType()))) {
        inst->setSrc(insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb),
                     i);
      } else {
        moveSrcToGRF(iter, i, 2, bb);
      }
      badTwoGRFSrc[i] = false;
      INST_LIST_ITER tmpIter = iter;
      tmpIter--;
      reduceExecSize(tmpIter, bb);
    }
  }
}
