/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "DebugInfo.h"
#include "G4_Verifier.hpp"
#include "HWConformity.h"
#include "HWConformityPro.h"
#include "Optimizer.h"
#include "PointsToAnalysis.h"
#include "visa_wa.h"

#include <cmath>
#include <unordered_set>

using namespace vISA;

static G4_CondModifier getReverseCondMod(G4_CondModifier mod) {
  switch (mod) {
  case Mod_z:
    return Mod_z;
  case Mod_e:
    return Mod_e;
  case Mod_nz:
    return Mod_nz;
  case Mod_ne:
    return Mod_ne;
  case Mod_g:
    return Mod_l;
  case Mod_ge:
    return Mod_le;
  case Mod_l:
    return Mod_g;
  case Mod_le:
    return Mod_ge;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid conditional modifier input for reversed "
                    "conditional modifier.");
  }
  return Mod_cond_undef;
}

static bool isCompressedInst(G4_INST *inst) { return inst->isComprInst(); }

#define isUnitRegionRow(opnd, execSize)                                        \
  (opnd->isImm() ||                                                            \
   opnd->isSrcRegRegion() &&                                                   \
       opnd->asSrcRegRegion()->getRegion()->width == execSize ||               \
   opnd->isSrcRegRegion() &&                                                   \
       opnd->asSrcRegRegion()->getRegion()->vertStride == 0)

G4_SubReg_Align HWConformity::getDclAlignment(int opndBytes, G4_INST *inst,
                                              bool isScalar) {
  auto subAlign = Get_G4_SubRegAlign_From_Size(
      (uint16_t)opndBytes, builder.getPlatform(), builder.getGRFAlign());
  bool hasAccSrc = inst->hasACCSrc();

  if (hasAccSrc && subAlign < builder.getGRFAlign()) {
    subAlign = builder.getGRFAlign();
  }

  if (!isScalar) {
    // certain instructions have additional alignment requirements for
    // non-scalar sources
    if (!builder.hasAlign1Ternary() && inst->getNumSrc() == 3 &&
        !inst->isSend() && subAlign < Eight_Word) {
      subAlign = Eight_Word;
    }
    if (inst->isMath()) {
      subAlign = builder.getGRFAlign();
    }
  }

  return subAlign;
}
/*
 *  create a new mov instruction and insert it after "it"
 *  mov (esize) dst tmp:type
 *  where esize is "inst"'s execution size and insert it after "inst"
 *  return value is the new temp variable as a dst operand
 *  If dstAlign is specified, the new temp will at least be aligend to that size
 *
 *  The new mov instruction is inserted right after "it", and caller is safe to
 *  access it via "++it".
 */
G4_DstRegRegion *HWConformity::insertMovAfter(INST_LIST_ITER &it,
                                              G4_DstRegRegion *dst,
                                              G4_Type type, G4_BB *bb,
                                              G4_SubReg_Align dstAlign) {
  return insertMovAfter(it, dst, type, bb, /*stride*/0, dstAlign);
}

G4_DstRegRegion *HWConformity::insertMovAfter(INST_LIST_ITER &it,
                                              G4_DstRegRegion *dst,
                                              G4_Type type, G4_BB *bb,
                                              uint16_t stride,
                                              G4_SubReg_Align dstAlign) {
  G4_INST *inst = *it;

  if (!dst) {
    return dst;
  }

  if (inst->hasNULLDst()) {
    return builder.createDst(dst->getBase(), 0, 0, 1, type);
  }

  G4_ExecSize exec_size = inst->getExecSize();
  G4_Type execType = inst->isRawMov() ? dst->getType() : inst->getExecType();
  bool scalarSrc = true;

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);
    if (!src->isImm()) {
      if (!(inst->isMath() && i == 1 && src->isNullReg()) &&
          (src->isSrcRegRegion() && !src->asSrcRegRegion()->isScalar())) {
        scalarSrc = false;
      }
    } else if (IS_VINTTYPE(src->getType()) || IS_VFTYPE(src->getType())) {
      scalarSrc = false;
    }
  }

  // fcvt/srnd do not support simd1
  const bool sameExecSize =
      (inst->opcode() == G4_fcvt || inst->opcode() == G4_srnd);
  G4_ExecSize newExecSize = ((inst->opcode() == G4_sel || sameExecSize ||
                              inst->getImplAccSrc() || !scalarSrc)
                                 ? exec_size
                                 : g4::SIMD1);

  uint32_t opExecWidthBytes = newExecSize * TypeSize(execType);
  if (execType == Type_DF && IS_BTYPE(type)) {
    type = (type == Type_UB ? Type_UW : Type_W);
  }
  uint16_t dstWidthBytes = newExecSize * TypeSize(type);
  uint16_t scale = stride ? stride : TypeSize(execType) / TypeSize(type);
  /*   so according to comments in function that call it MAD needs to have
     packed format. It ends up with hStride 2, due to DefHoisting. So it is
     trying to undo it. For every other type if srcType > dstCype we need to
     adjust regions. This is not necessary for HF. It's already packed.

      The src region of move is wrong. Since for HF it is packed, unlike other
     data types.mad (8) r56.0.xyzw:hf -r37.0.xyzw:f r59.0.xyzw:hf r58.0.xyzw:hf
     {Align16, NoMask} mov (16) r44.0<2>:hf r56.0<16;8,2>:hf {Align1, H1} //
     #??:$39:%66
  */
  // fcvt/srnd need to be packed, so scale should be 1
  const bool isPacked =
      !(builder.hasSimplifiedRegions() ||
        builder.getOption(vISA_EURegionRemoval)) &&
      (inst->opcode() == G4_fcvt || inst->opcode() == G4_srnd);
  if (scale == 0 || isPacked ||
      (builder.getPlatform() >= GENX_CHV && execType == Type_F &&
       !(builder.hasSimplifiedRegions() ||
         builder.getOption(vISA_EURegionRemoval)) &&
       type == builder.getMixModeType())) {
    scale = 1;
  }

  G4_SubReg_Align subAlign = getDclAlignment(
      opExecWidthBytes > dstWidthBytes ? opExecWidthBytes : dstWidthBytes, inst,
      newExecSize == 1);

  if (subAlign < dstAlign) {
    subAlign = dstAlign;
  }

  const RegionDesc *region = newExecSize > 1
                                 ? builder.createRegionDesc(scale, 1, 0)
                                 : builder.getRegionScalar();

  G4_Declare *dcl = builder.createTempVar(
      newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);

  G4_SrcRegRegion *srcRegion = builder.createSrcRegRegion(dcl, region);
  G4_Predicate *pred = NULL;

  if (inst->opcode() != G4_sel) {
    pred = inst->getPredicate();
    inst->setPredicate(NULL);
    // maintainDU4TempMov will update def-use
  }

  unsigned int new_option = inst->getMaskOption();
  G4_INST *newInst =
      builder.createMov(exec_size, dst, srcRegion, new_option, false);
  newInst->setPredicate(pred);
  newInst->setSaturate(inst->getSaturate());
  bb->insertAfter(it, newInst);

  // update propagation info
  maintainDU4TempMov(inst, newInst);

  if (type == dst->getType()) {
    newInst->setSaturate(g4::NOSAT);
  } else if (type == Type_F || type == Type_DF) {
    inst->setSaturate(g4::NOSAT);
  }

  inst->setExecSize(newExecSize);

  // If the inst has predicate, we shouldn't set NoMask for it as NoMask impacts
  // predicate control. For example:
  // (p0.0) sel (1|M8)  v1(0,0)<1>:ub  v2(0,0)<0;1,0>:uw  v3(0,0)<0;1,0>:uw
  // After inserting mov instruction =>
  // (p0.0) sel (1|M8)  TV1(0,0)<2>:ub  v2(0,0)<0;1,0>:uw  v3(0,0)<0;1,0>:uw
  //        mov (1|M8)  v1(0,0)<1>:ub  TV1(0,0)<0;1,0>:ub
  // The predicate of all instructions except for G4_sel is always moved
  // to the newly inserted MOV instruction. And newExecSize always equals its
  // origal execution size for G4_sel. In other words, we must not set NoMask
  // for SIMD1 G4_sel inst after inserting MOV inst.
  if (newExecSize == 1 && !inst->getPredicate()) {
    inst->setNoMask(true);
  }

  return builder.createDstRegRegion(dcl, scale);
}

//
// replace instruction (*it)' source srcPos, which must be a scalar/immediate,
// with a temp variable after inserting
// mov (esize) tmp<1>:type imm/scalar {options}
// before the instruction
// This is like insertMovBefore(), except that the latter will always use
// simd1 move for scalar/imm values, which may not be what we want
// NOTE: This does not check for redundant moves.  We are counting on a later
// LVN pass to clean them up
//
void HWConformity::broadcast(G4_BB *bb, INST_LIST_ITER it, int srcPos,
                             G4_SubReg_Align align) {
  G4_INST *inst = *it;
  G4_Operand *src = inst->getSrc(srcPos);
  vISA_ASSERT(src->isImm() ||
                   (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar()),
               "source must be an immediate or scalar");
  G4_Type type = src->getType();

  G4_ExecSize execSize = inst->getExecSize();
  uint32_t instMask = inst->getMaskOption();

  // avoid simd16 Qword moves
  vISA_ASSERT((unsigned)execSize * TypeSize(type) <=
                   2u * kernel.numEltPerGRF<Type_UB>(),
               "move can't exceed 2 GRFs");

  G4_Declare *dcl = builder.createTempVar(execSize, type, align);
  G4_DstRegRegion *dst = builder.createDst(dcl->getRegVar(), 0, 0, 1, type);
  G4_INST *newInst = builder.createMov(execSize, dst, src, instMask, false);

  bb->insertBefore(it, newInst);

  const RegionDesc *srcRegion = builder.getRegionStride1();
  G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(dcl, srcRegion);
  inst->setSrc(newSrc, srcPos);
  newInst->addDefUse(inst, inst->getSrcOperandNum(srcPos));
}

//
// A simplified version of insertMovBefore(), this copies raw bytes from source
// to a temp and replaces the original source with tmp.  This is primarily used
// to ensure operand alignment and region restrictions op (esize) ... (mod)
// src<region>:type
// -->
// mov (esize) tmp<1>:type src<region>:type
// op (esize) ... (mod) tmp<1;1,0>:type
//
// source must be a G4_SrcRegRegion (direct or indirect), immediates are not
// supported note that modifier is propagated from source to tmp, but region is
// not
//
//
G4_SrcRegRegion *HWConformity::insertCopyBefore(INST_LIST_ITER it,
                                                uint32_t srcNum,
                                                G4_SubReg_Align tmpAlign,
                                                G4_BB *bb) {
  G4_INST *inst = *it;
  G4_Operand *src = inst->getSrc(srcNum);
  vISA_ASSERT(src != nullptr && src->isSrcRegRegion(),
               "source must be a SrcRegRegion");
  G4_SrcRegRegion *origSrc = src->asSrcRegRegion();

  G4_ExecSize newExecSize =
      origSrc->isScalar() ? g4::SIMD1 : inst->getExecSize();
  G4_Declare *dcl =
      builder.createTempVar(newExecSize, origSrc->getType(), tmpAlign);
  G4_SrcModifier modifier = origSrc->getModifier();
  origSrc->setModifier(Mod_src_undef);
  G4_DstRegRegion *dst = builder.createDstRegRegion(dcl, 1);

  G4_INST *movInst =
      builder.createMov(newExecSize, dst, origSrc, InstOpt_WriteEnable, false);

  bb->insertBefore(it, movInst);
  G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(
      modifier, Direct, dcl->getRegVar(), 0, 0,
      newExecSize == 1 ? builder.getRegionScalar() : builder.getRegionStride1(),
      dcl->getElemType());

  return newSrc;
}

G4_SrcRegRegion *HWConformity::insertCopyAtBBEntry(G4_BB *bb,
                                                   G4_ExecSize execSize,
                                                   G4_Operand *src) {
  vISA_ASSERT(src != nullptr && src->isSrcRegRegion(),
               "source must be a SrcRegRegion");
  G4_SrcRegRegion *origSrc = src->asSrcRegRegion();
  auto lb = src->getLinearizedStart();
  auto rb = src->getLinearizedEnd();

  unsigned int regNum = lb / kernel.numEltPerGRF<Type_UB>();
  unsigned int numRegs = (rb + kernel.numEltPerGRF<Type_UB>() - 1 - lb) /
                         kernel.numEltPerGRF<Type_UB>();
  if (regNum == -1 || numRegs == 0) {
    return nullptr;
  }

  G4_Declare *dcl = builder.createTempVar(execSize, origSrc->getType(),
                                          builder.getGRFAlign());
  dcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(regNum), 0);
  G4_SrcModifier modifier = origSrc->getModifier();
  origSrc->setModifier(Mod_src_undef);
  G4_DstRegRegion *dst = builder.createDstRegRegion(dcl, 1);

  G4_INST *movInst =
      builder.createMov(execSize, dst, origSrc, InstOpt_WriteEnable, false);

  for (auto it = bb->begin(); it != bb->end(); it++) {
    if (!(*it)->isLabel()) {
      bb->insertBefore(it, movInst);
      break;
    }
  }

  G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(
      modifier, Direct, dcl->getRegVar(), 0, 0,
      execSize == 1 ? builder.getRegionScalar() : builder.getRegionStride1(),
      dcl->getElemType());
  return newSrc;
}

/*
 *  create a new mov instruction
 *  mov (esize) tmp<1>:type src
 *  where esize is "inst"'s execution size and insert it before "inst"
 *  return value is the new temp variable as a source operand.
 *
 *  "inst" is pointed by "it", and the new mov inst is inserted right
 *  before "it", so that caller can safely use "--it" to access the new
 *  mov instruction.
 */
G4_Operand *HWConformity::insertMovBefore(INST_LIST_ITER it, uint32_t srcNum,
                                          G4_Type type, G4_BB *bb,
                                          G4_SubReg_Align tmpAlign) {
  return insertMovBefore(it, srcNum, type, bb, 0, tmpAlign);
}

G4_Operand *HWConformity::insertMovBefore(INST_LIST_ITER it, uint32_t srcNum,
                                          G4_Type type, G4_BB *bb,
                                          uint16_t tmpStride,
                                          G4_SubReg_Align tmpAlign) {
  G4_INST *inst = *it;
  G4_SubReg_Align subAlign;
  const RegionDesc *region = nullptr;
  G4_ExecSize execSize = inst->getExecSize();
  G4_Operand *src = inst->getSrc(srcNum);
  unsigned short scale =
      IS_BTYPE(src->getType()) && src->getType() == type ? 2 : 1;

  G4_ExecSize newExecSize =
      (src->isImm() && !IS_VTYPE(src->getType())) ||
              (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar())
          ? g4::SIMD1
          : execSize;

  if (newExecSize > 1) {
    if (tmpStride) {
      scale = tmpStride;
    } else {
      if (scale == 1 && !IS_VTYPE(src->getType())) {
        scale = (uint16_t)(TypeSize(src->getType()) / TypeSize(type));
      }
      if (scale == 0) {
        scale = 1;
      }
    }
    region = builder.createRegionDesc(scale, 1, 0);
  } else {
    scale = src->getTypeSize() / TypeSize(type);
    if (scale == 0) {
      scale = 1;
    }
    region = builder.getRegionScalar();
  }

  int opExecWidthBytes =
      IS_VINTTYPE(src->getType())
          ? kernel.numEltPerGRF<Type_UB>() / 2 *
                (execSize > 8 ? execSize / 8 : 1)
          : (src->getType() == Type_VF ? kernel.numEltPerGRF<Type_UB>() / 2 *
                                             (execSize > 4 ? execSize / 4 : 1)
                                       : newExecSize * TypeSize(type) * scale);

  subAlign = getDclAlignment(opExecWidthBytes, inst, newExecSize == 1);

  if (subAlign < tmpAlign) {
    subAlign = tmpAlign;
  }

  uint32_t newInstEMask =
      newExecSize == 1 ? InstOpt_WriteEnable : inst->getMaskOption();

  // due to old BDW regioning rule we need NoMask inst here so they can be split
  if (kernel.getKernelType() == VISA_CM && builder.getPlatform() == GENX_BDW) {
    if (!bb->isAllLaneActive()) {
      newInstEMask = InstOpt_WriteEnable;
    }
  }

  G4_Declare *dcl = builder.createTempVar(
      newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);
  G4_DstRegRegion *dstRegion = builder.createDstRegRegion(dcl, scale);
  G4_INST *newInst =
      builder.createMov(newExecSize, dstRegion, builder.duplicateOperand(src),
                        newInstEMask, false);
  bb->insertBefore(it, newInst);
  inst->transferDef(newInst, Gen4_Operand_Number(srcNum + 1), Opnd_src0);
  newInst->addDefUse(inst, Gen4_Operand_Number(srcNum + 1));

  G4_SrcModifier modifier = Mod_src_undef;
  if (src->isSrcRegRegion()) {
    G4_SrcModifier srcMod = src->asSrcRegRegion()->getModifier();
    if (srcMod == Mod_Not) {
      // mov doesn't support logic modifiers, so we keep it on the new source
      modifier = Mod_Not;
      newInst->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
    } else if (src->getType() == Type_BF) {
      // bf mov does not support src mod as it is changed to shl or uw mov.
      // Keep it on the new source.
      modifier = srcMod;
      newInst->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
    }
  }

  return builder.createSrcRegRegion(modifier, Direct, dcl->getRegVar(), 0, 0,
                                    region, dcl->getElemType());
}

void HWConformity::fixPackedSource(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  bool nonTypeWFound = false, nonTypeFFound = false,
       incompatibleTypeFound = false;

  for (int i = 0; i < inst->getNumSrc(); i++) {
    auto src = inst->getSrc(i);
    if (!src) {
      continue;
    }
    if (!IS_VTYPE(src->getType())) {
      // Make sure other src operands are of word type only as this is a HW
      // requirement
      if (src->getType() != Type_W && src->getType() != Type_UW) {
        nonTypeWFound = true;
      }
      if (src->getType() != Type_F) {
        nonTypeFFound = true;
      }
      continue;
    }
    G4_Type target_type = src->getType() == Type_VF ? Type_F : Type_W;
    if (target_type == Type_W && (nonTypeWFound || !builder.hasByteALU())) {
      // non-word type src is not allowed to co-exist with :v src
      // also if platform lacks byte regioning :v src may be incompatible with
      // later legalization
      incompatibleTypeFound = true;
    } else if (target_type == Type_F && nonTypeFFound == true) {
      // non-float type src is not allowed to co-exist with :vf src
      incompatibleTypeFound = true;
    }

    // Insert a move only if immediate operand is not last src operand
    if (i != inst->getNumSrc() - 1 || incompatibleTypeFound == true) {
      inst->setSrc(insertMovBefore(it, i, target_type, bb), i);
    }
  }
}
/*
 * fixMathInst() checks the following:
 * The math instruction can only use GRF registers as source(s) and destination.
 * The math instruction does not support indirect addressing modes.
 * source horizontal stride must be 1 with the exception of scalar sources and
 * destination horizontal stride must be always 1. Source and destination offset
 * must be the same, except the case of scalar source DW and UD is the only
 * source format supported for INT DIV, FP16/FP32 is the only source format
 * supported for all the other functions. Mixed DW and UD sources are not
 * allowed for the INT DIV function. For single source math function, <src1>
 * must be programmed as ARF-NULL register.
 */
bool HWConformity::fixMathInst(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  G4_Operand *src0 = inst->getSrc(0), *src1 = inst->getSrc(1);
  bool mov_dst = false;

  vISA_ASSERT(inst->isMath(), "Expect math instruction");
  G4_InstMath *mathInst = inst->asMathInst();

  if (mathInst->getMathCtrl() == MATH_INVM ||
      mathInst->getMathCtrl() == MATH_RSQRTM) {
    // split two GRF math macros. This should only happen for FP64
    if (!builder.hasTwoGRFMathMacro() && IS_DFTYPE(inst->getDst()->getType()) &&
        ((uint32_t)(inst->getExecSize() * 2)) > builder.getNativeExecSize()) {
      evenlySplitInst(it, bb);
      return true;
    }
    // math macros are constructed internally and should already conform to all
    // other HW rules
    return false;
  }

  if (builder.getOption(vISA_DisableHFMath)) {
    auto src0 = inst->getSrc(0);
    auto src1 = inst->getSrc(1);
    auto dst = inst->getDst();
    if (src0 && src0->getType() == Type_HF) {
      replaceSrc(it, 0, Type_F, bb);
    }

    if (src1 && src1->getType() == Type_HF) {
      replaceSrc(it, 1, Type_F, bb);
    }

    if (dst && dst->getType() == Type_HF) {
      replaceDst(it, Type_F);
    }
  }

  // covers MATH_INT_DIV, MATH_INT_DIV_QUOT, MATH_INT_DIV_REM
  bool isIntDivide = inst->asMathInst()->isMathIntDiv();
  bool hasSameOffset = hasSameSubregOffset(inst);

  auto hasModMinus = [](G4_Operand *SrcOprd) {
    if (SrcOprd->isSrcRegRegion()) {
      G4_SrcModifier mod = SrcOprd->asSrcRegRegion()->getModifier();
      return (mod == Mod_Minus || mod == Mod_Minus_Abs);
    }
    return false;
  };

  // check if the source needs a move and if so the new move type
  auto needsMove = [this, inst, isIntDivide, hasSameOffset,
                    hasModMinus](int srcID, G4_Type &newType) {
    vISA_ASSERT((srcID == 0 || srcID == 1), "math can have at most two sources");
    G4_Operand *src = inst->getSrc(srcID);
    newType = src->getType();
    if (isIntDivide) {
      // case 1: Perform a signed division if there's any minus src modifier.
      //   math.quot  r10:w   r20:ub   -r30:ub
      // Make sure newType is D, not UD. The correct code is:
      //   mov  r22:d  r20:ub
      //   mov  r32:d  -r30:ub
      //   math.quot r10:w  r22:d  r32:d
      // case 2: Perform an appropriate type conversion based on the type ranks
      // of both sources.
      //   math.quot  r6:ud  r3:b  r4:ud
      // Make sure it's still an unsigned division.
      //   mov  r11:ud  r3:b
      //   math.quot  r6:ud  r11:ud  r4:ud
      G4_Type src0Type = inst->getSrc(0)->getType();
      G4_Type src1Type = inst->getSrc(1)->getType();
      G4_Type divType = Type_UNDEF;
      if (hasModMinus(inst->getSrc(0)) || hasModMinus(inst->getSrc(1))) {
        // If there's any minus source modifier, do a signed division.
        divType = Type_D;
      } else if (TypeSize(src0Type) != TypeSize(src1Type)) {
        // If src0 and src1 have different ranks, get the signedness of the
        // division from the higher rank src.
        G4_Type higherRankType =
            TypeSize(src0Type) > TypeSize(src1Type) ? src0Type : src1Type;
        divType = IS_SIGNED_INT(higherRankType) ? Type_D : Type_UD;
      } else {
        // If both sources have the same rank, do a signed division only
        // when both are signed. Otherwise, do an unsigned division.
        divType = IS_SIGNED_INT(src0Type) && IS_SIGNED_INT(src1Type) ? Type_D
                                                                     : Type_UD;
      }
      vASSERT(divType == Type_D || divType == Type_UD);
      if (newType != divType) {
        newType = divType;
        return true;
      }
    } else if ((src->getType() != Type_F && src->getType() != Type_VF) &&
               (builder.getPlatform() == GENX_BDW ||
                src->getType() != Type_HF)) {
      // CHV+ supports F/HF math, while BDW only supports F math
      // mix mode math is handled in fixMixedHFInst()
      newType = Type_F;
      return true;
    }

    if (src->isImm()) {
      if (srcID == 0 && inst->asMathInst()->getMathCtrl() >= MATH_FDIV) {
        return true;
      }
    } else if (src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      const RegionDesc *rd = srcRegion->getRegion();
      if (srcRegion->getModifier() != Mod_src_undef && isIntDivide) {
        // no source modifier for int divide
        return true;
      } else if (srcRegion->getRegAccess() != Direct) {
        return true;
      } else if (!srcRegion->isScalar()) {
        if (!hasSameOffset &&
            !builder.tryToAlignOperand(srcRegion, kernel.numEltPerGRF<Type_UB>())) {
          return true;
        } else if (!rd->isContiguous(inst->getExecSize())) {
          return true;
        }
      }
    } else {
      vISA_ASSERT_UNREACHABLE("Unexpected math source!");
    }
    return false;
  };

  auto isPureHF = [](const G4_INST *aI) {
    G4_DstRegRegion *aD = aI->getDst();
    G4_Operand *S0 = aI->getSrc(0);
    G4_Operand *S1 = aI->getSrc(1);
    if ((aD && !aD->isNullReg() && aD->getType() != Type_HF) ||
        (S0 && !S0->isNullReg() && S0->getType() != Type_HF) ||
        (S1 && !S1->isNullReg() && S1->getType() != Type_HF)) {
      return false;
    }
    return true;
  };

  // For packed HF math,  it must be simd8/simd16. (see math instruction for
  // detail)
  G4_ExecSize nativeES = builder.getNativeExecSize();
  if (isPureHF(inst) && inst->getExecSize() < nativeES) {
    // math:  src0 and src1.
    // Given (PVC):
    //       math.inv (8|M0)          r4.0<1>:hf    r1.0<1;1,0>:hf
    // changed to:
    //       mov (16|M0)              r6.0<1>:uw    0x7C00:uw
    //       mov (8|M0)               r6.0<1>:hf    r1.0<1;1,0>:hf
    //       math.inv (16|M0)         r7.0<1>:hf    r6.0<1;1,0>:hf
    //       mov (8|M0)               r4.0<1>:hf    r7.0<1;1,0>:hf
    //
    // If src is scalar such as:
    //       math.inv (1|M0)          r4.8<1>:hf    r1.8<0;1,0>:hf
    // Changed it to:
    //       math.inv (16|M0)         r6.0<1>:hf    r1.8<0;1,0>:hf
    //       mov (1|M0)               r4.8<1>:hf    r6.0<0;1,0>:hf
    G4_ExecSize currES = inst->getExecSize();

    for (int i = 0, sz = 2; i < sz; ++i) {
      G4_Operand *S = inst->getSrc(i);
      if (!S || S->isNullReg() || !S->isSrcRegRegion()) {
        continue;
      }
      G4_SrcRegRegion *rS = S->asSrcRegRegion();
      if (rS->getRegion()->isScalar()) {
        continue;
      }

      G4_Declare *sDcl =
          builder.createTempVar(nativeES, Type_HF, builder.getGRFAlign());
      G4_DstRegRegion *tD0 =
          builder.createDst(sDcl->getRegVar(), 0, 0, 1, Type_UW);
      G4_Imm *inf = builder.createImm(0x7C00, Type_UW);
      G4_INST *I0 =
          builder.createMov(nativeES, tD0, inf, inst->getOption(), false);

      G4_DstRegRegion *tD1 =
          builder.createDst(sDcl->getRegVar(), 0, 0, 1, Type_HF);
      G4_INST *I1 =
          builder.createMov(currES, tD1, rS, inst->getOption(), false);

      G4_SrcRegRegion *nS0 =
          builder.createSrcRegRegion(sDcl, builder.getRegionStride1());
      inst->setSrc(nS0, i);

      bb->insertBefore(it, I0);
      bb->insertBefore(it, I1);
    }

    G4_Declare *dDcl =
        builder.createTempVar(nativeES, Type_HF, builder.getGRFAlign());
    G4_DstRegRegion *nD = builder.createDstRegRegion(dDcl, 1);
    inst->setDest(nD); // dst: still original
    inst->setExecSize(nativeES);

    G4_SrcRegRegion *nSrc =
        builder.createSrcRegRegion(dDcl, builder.getRegionStride1());
    G4_INST *nMov =
        builder.createMov(currES, dst, nSrc, inst->getOption(), false);
    bb->insertAfter(it, nMov);

    // Update dst/src0/src1 as it needs further check on other restrictions.
    dst = inst->getDst();
    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    mov_dst = true;
  }

  if (src0) {
    G4_Type src0_type = src0->getType();
    bool needsSrc0Mov = needsMove(0, src0_type);
    if (needsSrc0Mov) {
      inst->setSrc(
          insertMovBefore(it, 0,
                          src0->isImm()
                              ? G4_Operand::GetNonVectorImmType(src0_type)
                              : src0_type,
                          bb),
          0);
      src0 = inst->getSrc(0);
    }
  }

  bool nullSrc1 = src1 && src1->isNullReg();
  if (!nullSrc1 && src1) {
    G4_Type src1_type = src1->getType();
    bool needsSrc1Move = needsMove(1, src1_type);

    if (needsSrc1Move) {
      if (isIntDivide && src1->isImm() && !IS_VINTTYPE(src1->getType())) {
        // just change the immediate's type
        uint32_t immVal = (uint32_t)src1->asImm()->getImm();
        inst->setSrc(builder.createImm(immVal, src1_type), 1);
      } else {
        inst->setSrc(
            insertMovBefore(it, 1,
                            src1->isImm()
                                ? G4_Operand::GetNonVectorImmType(src1_type)
                                : src1_type,
                            bb),
            1);
      }
      src1 = inst->getSrc(1);
    }
  }

  if (nullSrc1 && src0 && src1->getType() != src0->getType()) {
    G4_SrcRegRegion *src1_opnd =
        builder.createNullSrc(inst->getSrc(0)->getType());
    inst->setSrc(src1_opnd, 1);
  }

  // recompute as src0 and src1 may have been modified
  hasSameOffset = hasSameSubregOffset(inst);
  G4_Type extype = inst->getExecType2();
  bool cond1 = (dst->getType() != extype &&
                !(dst->getType() == Type_UD && extype == Type_D));
  if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 || cond1 ||
      (!hasSameOffset && inst->getExecSize() != g4::SIMD1 &&
       !builder.tryToAlignOperand(dst, kernel.numEltPerGRF<Type_UB>()))) {
    mov_dst = true;
    replaceDst(it, extype);
  }

  if ((builder.hasHFMathSrcBroadCast() ||
       VISA_WA_CHECK(builder.getPWaTable(), Wa_22016140776)) &&
      (inst->getExecSize() != g4::SIMD1)) {
    auto src0 = inst->getSrc(0);
    auto src1 = inst->getSrc(1);

    if (src0 && !src0->isNullReg() && (src0->getType() == Type_HF) &&
        ((src0->isSrcRegRegion() && src0->asSrcRegRegion()->isScalar()) ||
         src0->isImm())) {
      broadcast(bb, it, 0, builder.getGRFAlign());
    }
    if (src1 && !src1->isNullReg() && (src1->getType() == Type_HF) &&
        ((src1->isSrcRegRegion() &&
        src1->asSrcRegRegion()->isScalar()) || src1->isImm())) {
      broadcast(bb, it, 1, builder.getGRFAlign());
    }
  }

  if (builder.hasHFMathGRFAlign()) {
    auto src0 = inst->getSrc(0);
    auto src1 = inst->getSrc(1);
    auto dst = inst->getDst();

    if (dst && !dst->isNullReg() && dst->getType() == Type_HF &&
        dst->getHorzStride() == 1) {
      if (!builder.tryToAlignOperand(dst, kernel.numEltPerGRF<Type_UB>())) {
        mov_dst = true;
        replaceDst(it, dst->getType(), builder.getGRFAlign());
      }
      if (src0 && !src0->isNullReg() && src0->getType() == Type_HF) {
        if (!builder.tryToAlignOperand(src0, kernel.numEltPerGRF<Type_UB>())) {
          G4_Operand *newSrc0 = insertMovBefore(it, 0, src0->getType(), bb,
                                                builder.getGRFAlign());
          inst->setSrc(newSrc0, 0);
        }
      }
      if (src1 && !src1->isNullReg() && src1->getType() == Type_HF) {
        if (!builder.tryToAlignOperand(src1, kernel.numEltPerGRF<Type_UB>())) {
          G4_Operand *newSrc1 = insertMovBefore(it, 1, src1->getType(), bb,
                                                builder.getGRFAlign());
          inst->setSrc(newSrc1, 1);
        }
      }
    }
  }

  return mov_dst;
}

bool HWConformity::hasSameSubregOffset(G4_INST *inst) const {
  uint32_t offset;
  return hasSameSubregOffset(inst, offset);
}

//
// returns true if all sources and dst in this inst have the same fixed subreg
// offset null src/dst, scalar sources and immediates are excluded from the
// check If true, return the common byte offset in byteOffset
//
bool HWConformity::hasSameSubregOffset(G4_INST *inst,
                                       uint32_t &byteOffset) const {
  bool anyOffset = true; // true means offset is not fixed yet
  byteOffset = 0;
  if (inst->getDst()) {
    G4_DstRegRegion *dst = inst->getDst();
    if (dst->isNullReg()) {
      // do nothing
    } else if (dst->hasFixedSubregOffset(builder, byteOffset)) {
      anyOffset = false;
    } else {
      return false;
    }
  }

  for (int i = 0; i < inst->getNumSrc(); ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isSrcRegRegion()) {
      uint32_t srcOffset = 0;
      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      if (srcRegion->isNullReg() || srcRegion->isScalar()) {
        continue;
      } else if (srcRegion->hasFixedSubregOffset(builder, srcOffset)) {
        if (anyOffset) {
          byteOffset = srcOffset;
          anyOffset = false;
        } else if (srcOffset != byteOffset) {
          return false;
        }
      } else {
        return false;
      }
    }
  }

  return true;
}

//Src1 regioning pattern must be flat, offset and channel aligned with destination.
//But for fp8 output, src1 is 8bits, only even offset supported, destination offset could be the same offset or plus 1
bool HWConformity::hasSameSubregOffsetSrnd(G4_DstRegRegion *dst,
                                       G4_SrcRegRegion *src) const {
  uint32_t dstOffset = 0;
  if (!dst->hasFixedSubregOffset(builder, dstOffset)) {
    return false;
  }

  uint32_t srcOffset = 0;
  G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
  if (srcRegion->hasFixedSubregOffset(builder, srcOffset)) {
    if ((dst->getType() == Type_UB && (srcOffset / 2) != (dstOffset / 2)) ||
        (dst->getType() != Type_UB && srcOffset != dstOffset) || (src->getType() == Type_UB && srcOffset % 2)) {
      return false;
    }
  }

  return true;
}
// Check the following rules
// -- src0 in 2 source instructions may not be immediate.  We try to swap for
// src0 and src1 for
//    commutative instructions in such cases
// -- ARF may not be in src1
void HWConformity::fixImmAndARFSrc(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  if (inst->isSend() || inst->isDpas()) {
    return;
  }

  G4_Operand *src0, *src1, *src2;
  src0 = inst->getSrc(0);
  src1 = inst->getSrc(1);
  src2 = inst->getSrc(2);

  /* Check for usage of two constants in binary operations */
  if (src0 && (src0->isImm() || src0->isAddrExp()) && inst->getNumSrc() == 2) {
    if (INST_COMMUTATIVE(inst->opcode()) && !src1->isImm()) {
      // all commutative inst must have 2 sources
      if (inst->opcode() == G4_mul) {
        bool needConstMov;
        // for DW and W mul, src0 must be DW and src1 W
        needConstMov = IS_DTYPE(src0->getType()) && !IS_DTYPE(src1->getType());

        if (needConstMov) {
          G4_Type tmpType = G4_Operand::GetNonVectorImmType(src0->getType());

          G4_Operand *newSrc0 = insertMovBefore(it, 0, tmpType, bb);
          inst->setSrc(newSrc0, 0);
        } else {
          // swap operands
          inst->swapSrc(0, 1);
          inst->swapDefUse();
        }
      } else {
        // swap operands
        inst->swapSrc(0, 1);
        inst->swapDefUse();
      }
    }
    /*
     * A select operation isn't commutative, but we may commute the
     * operands provided we perform a predicate inversion as well.
     * (v0)  sel ... const V1
     *    =>
     * (-v0) sel ... V1 const
     */
    else if (inst->opcode() == G4_sel && !src1->isImm()) {
      G4_CondMod *cond = inst->getCondMod();
      if (cond) {
        switch (cond->getMod()) {
        case Mod_ne:
          inst->setCondMod(builder.createCondMod(Mod_e, cond->getBase(), 0));
          break;
        case Mod_e:
          inst->setCondMod(builder.createCondMod(Mod_ne, cond->getBase(), 0));
          break;
        default:
          break;
        }
      } else {
        G4_Predicate *pred = inst->getPredicate();
        vISA_ASSERT(pred != NULL, "predicate must not be null");
        G4_PredState reverse = pred->getState() == PredState_Minus
                                   ? PredState_Plus
                                   : PredState_Minus;
        inst->setPredicate(builder.createPredicate(reverse, pred->getBase(),
                                                   pred->getSubRegOff(),
                                                   pred->getControl()));
      }
      inst->swapSrc(0, 1);
      inst->swapDefUse();
    } else if (!inst->isMath()) {
      // math immediate src0 is handled separately in fixMathInst()
      // If src0 is not 64-bit, src1 is 64-bit, swap them to save one move.
      if (INST_COMMUTATIVE(inst->opcode()) && src0->isImm() && src1->isImm() &&
          src0->getTypeSize() != 8 && src1->getTypeSize() == 8) {
        inst->swapSrc(0, 1);
        inst->swapDefUse();
        src0 = inst->getSrc(0);
        src1 = inst->getSrc(1);
        // this needs to fall through as we still need move for src0
      }

      if (INST_COMMUTATIVE(inst->opcode()) && src0->isAddrExp() &&
          src1->isImm()) {
        // The original IR has both addr expr and immediate
        //   add(8) A0(0, 0)<1>:uw &V36 + 0 0xeca86420 : uv{ Align1, Q1 }
        // We insert a move for src1 which is an immediate
        //   mov(8) TV0(0, 0)<1> : uw 0xeca86420 : uv{ Align1 }
        //   add(8) A0(0, 0)<1> : uw &V36 + 0 TV0(0, 0)<8; 8, 1> : uw{ Align1,
        //   Q1 }
        G4_Type type = src1->getType();
        inst->setSrc(
            insertMovBefore(it, 1, G4_Operand::GetNonVectorImmType(type), bb),
            1);
        // And we swap addr expr and the new variable
        //   add(8) A0(0, 0)<1> : uw TV0(0, 0)<8; 8, 1> : uw &V36 + 0 {Align1,
        //   Q1}
        // The final code sequence is
        //   mov(8) r13.0<1>:uw 0xeca86420 : uv{ Align1 } // #26:$9:%79
        //   add(8) a0.0<1> : uw r13.0<8; 8, 1> : uw 0x60 : uw{ Align1, Q1 }
        inst->setSrc(inst->getSrc(1), 0);
        inst->setSrc(src0, 1);
        inst->swapDefUse();
      } else {
        G4_Type newSrcType =
            inst->needsDWType()
                ? (IS_UNSIGNED_INT(src0->getType()) ? Type_UD : Type_D)
                : G4_Operand::GetNonVectorImmType(src0->getType());
        inst->setSrc(insertMovBefore(it, 0, newSrcType, bb), 0);
      }
    }
  }

  src0 = inst->getSrc(0);
  src1 = inst->getSrc(1);
  src2 = inst->getSrc(2);

  // lzd only supports UD datatype
  if (inst->opcode() == G4_lzd && src0 && src0->isImm() &&
      src0->getType() != Type_UD) {
    uint32_t immVal = (uint32_t)src0->asImm()->getImm();
    inst->setSrc(builder.createImm(immVal, Type_UD), 0);
  }

  // check for non-mad 3src inst

  if (inst->opcode() == G4_madw) {
    // src0 can not be immediate.
    if (src0 && src0->isImm()) {
      // swap src0 and src1 if src0 is immediate but src1 is not immediate
      if (src1 && !src1->isImm()) {
        inst->swapSrc(0, 1);
        inst->swapDefUse();
        src0 = inst->getSrc(0);
        src1 = inst->getSrc(1);
      } else {
        inst->setSrc(
            insertMovBefore(
                it, 0, IS_UNSIGNED_INT(src0->getType()) ? Type_UD : Type_D, bb),
            0);
        src0 = inst->getSrc(0);
      }
    }

    // fixe immediate type of G4_madw as it can only support D/UD types
    if (src1 && src1->isImm()) {
      uint32_t immVal = (uint32_t)src1->asImm()->getImm();
      inst->setSrc(builder.createImm(immVal, IS_SIGNED_INT(src1->getType())
                                                 ? Type_D
                                                 : Type_UD),
                   1);
      src1 = inst->getSrc(1);
    }

    if (src2 && src2->isImm()) {
      uint32_t immVal = (uint32_t)src2->asImm()->getImm();
      inst->setSrc(builder.createImm(immVal, IS_SIGNED_INT(src2->getType())
                                                 ? Type_D
                                                 : Type_UD),
                   2);
      src2 = inst->getSrc(2);
    }
  }

  // madw can have src1 as immediate
  if (inst->getNumSrc() == 3 && src1->isImm() && inst->opcode() != G4_madw) {
    inst->setSrc(
        insertMovBefore(
            it, 1,
            INST_FLOAT_SRC_ONLY(inst->opcode()) ? Type_F : src1->getType(), bb),
        1);
  }

  // Architecture registers may not appear as src1.
  auto isARF = [](G4_Operand *opnd) {
    return opnd->isAreg() || opnd->isFlag();
  };
  if (src1 != nullptr && isARF(src1) && !src1->isNullReg()) {
    /* See if we can swap the src1 */
    if (INST_COMMUTATIVE(inst->opcode()) && !isARF(src0)) {
      inst->swapSrc(0, 1);
      inst->swapDefUse();
    } else {
      /* Otherwise introduce a tmp */
      inst->setSrc(insertMovBefore(it, 1,
                                   INST_FLOAT_SRC_ONLY(inst->opcode())
                                       ? Type_F
                                       : src1->getType(),
                                   bb),
                   1);
    }
  }

  src2 = inst->getSrc(2);

  // 3 src instructions except madw can't have any constants
  if (!builder.hasAlign1Ternary() && src2 != nullptr && src2->isImm() &&
      inst->opcode() != G4_madw) {
    inst->setSrc(insertMovBefore(it, 2, src2->getType(), bb), 2);
  }
}

bool HWConformity::fixLine(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (inst->opcode() == G4_line) {
    bool badRegion = false;
    G4_Operand *src0 = inst->getSrc(0);
    // assumption: there are 4 elements in src0
    if (src0->isSrcRegRegion()) {
      const RegionDesc *rd = src0->asSrcRegRegion()->getRegion();
      badRegion =
          (rd->vertStride != 0 || rd->width != 4 || rd->horzStride != 1);
    }
    if (!IS_FTYPE(src0->getType()) || src0->isImm() || badRegion ||
        !builder.tryToAlignOperand(src0, kernel.numEltPerGRF<Type_UB>() / 2)) {
      // insertMovBefore()  is not used here
      // due to the special region <0;4,1> of src0 of line
      G4_Declare *src0_dcl;
      G4_DstRegRegion *new_dst_opnd;
      G4_SrcRegRegion *new_src0_opnd;
      unsigned char mov_size = 4;

      src0_dcl = builder.createTempVar(mov_size, Type_F, Eight_Word);
      /* Create temporary variable */
      // Actully we set region to be <0;4,1> directly here.
      const RegionDesc *rd = builder.createRegionDesc(0, 4, 1);
      new_src0_opnd = builder.createSrcRegRegion(src0_dcl, rd);
      new_dst_opnd = builder.createDstRegRegion(src0_dcl, 1);

      G4_INST *newInst = builder.createMov(G4_ExecSize(mov_size), new_dst_opnd,
                                           src0, InstOpt_NoOpt, false);
      newInst->setNoMask(true);

      bb->insertBefore(it, newInst);
      inst->setSrc(new_src0_opnd, 0);
      return true;
    }
  }
  return false;
}

bool HWConformity::fixOpndType(INST_LIST_ITER it, G4_BB *bb) {
  /*
   * Check for instruction that only accept float/int operands, as well as
   * instruction with mixed operand types.  Even though vISA itself forbids
   * mixed type instructions, optimizations such as copy propagation
   * may reintroduce them and so we do the checks here
   */
  G4_INST *inst = *it;
  bool changed = false;
  int numSrc = inst->getNumSrc();
  bool has_float = false;
  bool has_int = false;

  if (inst->isSend() || inst->isDpas() || inst->opcode() == G4_smov) {
    // skip special instructions
    return false;
  }

  for (int i = 0; i < numSrc; i++) {
    if (!inst->getSrc(i)) {
      continue;
    }
    G4_Type ty = inst->getSrc(i)->getType();
    if (IS_TYPE_FLOAT_ALL(ty)) {
      has_float = true;
    } else {
      has_int = true;
    }
  }

  if (has_float && has_int && (inst->opcode() != G4_srnd)) {
    for (int i = 0; i < numSrc; i++) {
      if (inst->getSrc(i) && !IS_FTYPE(inst->getSrc(i)->getType()) &&
          !IS_DFTYPE(inst->getSrc(i)->getType())) {
        // FIXME: we should probably either get rid of this or assert,
        // it's unlikely that blinding casting int to float is the right thing
        // here
        inst->setSrc(insertMovBefore(it, i, Type_F, bb), i);
        changed = true;
      }
    }
  }

  if (builder.noSrc1Byte()) {
    if (numSrc > 1) {
      G4_Operand *src0 = inst->getSrc(0);
      G4_Operand *src1 = inst->getSrc(1);
      if (src0 != nullptr && src1 != nullptr &&
          (IS_BTYPE(src1->getType()) &&
           !(inst->opcode() == G4_srnd && builder.enableMovCvt()))) {
        if (!IS_BTYPE(src0->getType()) && inst->canSwapSource()) {
          inst->swapSrc(0, 1);
        } else {
          bool hasModMinus = false;
          if (src1->isSrcRegRegion()) {
            G4_SrcModifier mod = src1->asSrcRegRegion()->getModifier();
            hasModMinus = (mod == Mod_Minus || mod == Mod_Minus_Abs);
          }
          // If minus modifier is present, need signed type.
          G4_Type Ty = (IS_SIGNED_INT(src1->getType()) || hasModMinus)
                           ? Type_W
                           : Type_UW;
          inst->setSrc(insertMovBefore(it, 1, Ty, bb), 1);
          changed = true;
        }
      }
    }
  }

  return changed;
}

/*
 * fixOpnds() looks for operands conformity:
 * 1. checks can operand be a constant.
 * 2. checks if operand's type is conformant to operation.
 * 3. check if only src0 uses VxH
 * 4. check if indirect scalar is used in compressed inst
 * It tries to fix these cases by changing operands order if possible
 * or by insertion if temporary location with appropriate conversion.
 */
void HWConformity::fixOpnds(INST_LIST_ITER it, G4_BB *bb, G4_Type &exType) {
  G4_INST *inst = *it;
  if (inst->isSend()) {
    return;
  }

  G4_Operand *src0, *src1, *src2;

  src0 = inst->getSrc(0);
  src1 = inst->getSrc(1);
  src2 = inst->getSrc(2);

  if (inst->opcode() == G4_mul) {
    if (IS_DTYPE(src1->getType()) &&
        !(IS_DTYPE(src0->getType()) || IS_FTYPE(src0->getType()))) {
      // check if src0 uses VxH
      bool src0_use_VxH = false;

      if (src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->getRegAccess() != Direct &&
          src0->asSrcRegRegion()->getRegion()->isRegionWH()) // is this safe?
      {
        src0_use_VxH = true;
      }
      if (src0_use_VxH) {
        src0 = insertMovBefore(it, 0, src0->getType(), bb);
      }
      inst->setSrc(src0, 1);
      inst->setSrc(src1, 0);
      inst->swapDefUse();
      src0 = inst->getSrc(0);
      src1 = inst->getSrc(1);
    }

    if (src1->isSrcRegRegion() &&
        src1->asSrcRegRegion()->getRegAccess() != Direct &&
        src1->asSrcRegRegion()->getRegion()->isRegionWH()) {
      if (IS_DTYPE(src0->getType()) &&
          !(IS_DTYPE(src1->getType()) || IS_FTYPE(src1->getType()))) {
        inst->setSrc(insertMovBefore(it, 1, src1->getType(), bb), 1);
      } else {
        inst->swapSrc(0, 1);
        inst->swapDefUse();
      }
    }
  }

  fixImmAndARFSrc(it, bb);

  src0 = inst->getSrc(0);
  src1 = inst->getSrc(1);
  src2 = inst->getSrc(2);

  // Vx1 and VxH can only be used for src0
  bool src0_use_VxH = false, src1_use_VxH = false;

  if (src2 && src2->isSrcRegRegion() &&
      src2->asSrcRegRegion()->getRegion()->isRegionWH()) {
    inst->setSrc(insertMovBefore(it, 2, exType, bb), 2);
  }

  if (src0 != NULL && src0->isSrcRegRegion() &&
      src0->asSrcRegRegion()->getRegion()->isRegionWH()) {
    src0_use_VxH = true;
  }

  if (src1 != NULL && src1->isSrcRegRegion() &&
      src1->asSrcRegRegion()->getRegion()->isRegionWH()) {
    src1_use_VxH = true;
  }

  if (src1_use_VxH) {
    if ((INST_COMMUTATIVE(inst->opcode()) || inst->opcode() == G4_cmp) &&
        !src0_use_VxH &&
        !(inst->opcode() == G4_mul && src0 && IS_DTYPE(src0->getType()))) {
      inst->swapSrc(0, 1);
      if (inst->opcode() == G4_cmp) {
        // change condMod
        G4_CondMod *condMod = inst->getCondMod();
        if (condMod) {
          G4_CondMod *newCondModOpnd = builder.createCondMod(
              getReverseCondMod(condMod->getMod()), condMod->getBase(),
              condMod->getSubRegOff());
          inst->setCondMod(newCondModOpnd);
        }
      }
    } else {
      inst->setSrc(insertMovBefore(it, 1, exType, bb), 1);
    }
  }

  // at this point only src0 may be VxH
  // VxH regioning and conditional modifiers may not co-exist
  if (builder.getPlatform() >= GENX_ICLLP) {
    src0 = inst->getSrc(0);
    if (src0 && src0->isSrcRegRegion() &&
        src0->asSrcRegRegion()->getRegion()->isRegionWH()) {
      if (inst->getCondMod()) {
        inst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
      }
    }
  }
}

void HWConformity::fixAlign13SrcInst(INST_LIST_ITER iter, G4_BB *bb) {
  // again mad should already conform by construction
  G4_INST *inst = *iter;
  vISA_ASSERT(inst->getNumSrc() == 3 && !inst->isSend(), "expect 3src inst");

  if (inst->opcode() != G4_mad && inst->opcode() != G4_madw) {
    G4_DstRegRegion *dst = inst->getDst();
    if (!isGoodAlign1TernaryDst(inst)) {
      auto alignment =
          builder.noSrc2Regioning() ? builder.getGRFAlign() : Four_Word;
      replaceDst(iter, dst->getType(), alignment);
    }

    auto canSwapSrc1Src2{[&inst]() {
      if (inst->opcode() == G4_add3 || inst->opcode() == G4_mad)
        return true;

      if (inst->opcode() == G4_bfn) {
        uint8_t funcCtrl = inst->asBfnInst()->getBooleanFuncCtrl();
        for (int k = 0; k < 8; k++) {
          auto calculateIdx{[](const int &k) {
            int b1 = (k >> 1) & 0x1;
            int b2 = (k >> 2) & 0x1;
            int x = b1 ^ b2;
            x = (x << 1) | (x << 2);
            return (k ^ x);
          }};
          if (((funcCtrl >> k) ^ (funcCtrl >> calculateIdx(k))) & 0x1) {
            return false;
          }
        }
        return true;
      }

      return false;
    }};

    bool canBeImm = true;
    for (int i = 0; i < inst->getNumSrc(); ++i) {
      if (!isGoodAlign1TernarySrc(inst, i, canBeImm)) {
        if (i == 2 && builder.noSrc2Regioning()) {
          if (canSwapSrc1Src2()) {
            // Try swapping src1 and src2 to avoid extra mov
            inst->swapSrc(1, 2);
            inst->swapDefUse(Opnd_src1, Opnd_src2);
            if (!isGoodAlign1TernarySrc(inst, 1, canBeImm) ||
                !isGoodAlign1TernarySrc(inst, 2, canBeImm)) {
              inst->swapSrc(1, 2);
              inst->swapDefUse(Opnd_src1, Opnd_src2);
              // some additional handling for src2 when src2 regioning is not
              // available
              fixSrc2(iter, bb, false);
            }
          } else {
            fixSrc2(iter, bb, false);
          }
        } else {
          G4_SubReg_Align subalign = (i == 2) ? Four_Word : Any;
          inst->setSrc(insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb,
                                       subalign),
                       i);
        }
      } else {
        if (inst->getSrc(i)->isImm() &&
            !builder.hasImmOnSrc0Src2ForAlign1Ternary()) {
          canBeImm = false;
        }
      }
    }
  }
}

void HWConformity::fix3SrcInst(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  if (inst->getNumSrc() != 3 || inst->isSend() || inst->isDpas() ||
      inst->opcode() == G4_madm) {
    return;
  }

  if (builder.hasAlign1Ternary()) {
    fixAlign13SrcInst(iter, bb);
    return;
  }

  if (inst->opcode() != G4_mad && inst->opcode() != G4_madw) {
    // check that dst and srcs are legal for 3src.  We do not check
    // mad and madw since they should already conform by construction
    uint8_t execSize = inst->getExecSize();
    G4_DstRegRegion *dst = inst->getDst();
    if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 ||
        !builder.tryToAlignOperand(dst, (execSize >= 8) ? 32 : execSize * 4)) {
      replaceDst(iter, dst->getType());
    }
    for (int i = 0; i < 3; i++) {
      if (!isGoodAlign16Src(inst, i)) {
        inst->setSrc(insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb),
                     i);
      }
    }
  }

  // When it is set (Align16), the instruction uses 16-byte-aligned addressing
  // for source and destination operands.
  if ((inst->getExecSize() == g4::SIMD1)) {
    if (inst->getDst() && inst->getDst()->getBase()->isRegVar()) {
      if (!builder.tryToAlignOperand(inst->getDst(), 16)) {
        replaceDst(iter, inst->getDst()->getType(), Eight_Word);
      }
    }
  }

  if (inst->getExecSize() == g4::SIMD16) {
    bool wa3rc =
        (VISA_WA_CHECK(builder.getPWaTable(), WaDisableSIMD16On3SrcInstr) &&
         !(inst->getExecType() == Type_HF &&
           inst->getOperand(Opnd_src1)->isSrcRegRegion() &&
           inst->getOperand(Opnd_src1)->getType() == Type_HF &&
           !inst->getOperand(Opnd_src1)->asSrcRegRegion()->crossGRF(builder)));

    if (wa3rc) {
      evenlySplitInst(iter, bb);
    }
  }
}

void HWConformity::fixCompareInst(INST_LIST_ITER i, G4_BB *bb, G4_Type exType,
                                  int dst_elsize) {
  G4_INST *inst = *i;
  G4_Operand *dst = inst->getDst();

  if (dst && dst->isNullReg()) {
    // change dst hstride if necessary
    if (TypeSize(exType) != dst->getTypeSize()) {
      // create a new dst with new stride
      G4_DstRegRegion *new_null = builder.createNullDst(exType);
      inst->setDest(new_null);
    }
  }
}

// For integer packing moves, we can replace the src type with the dst type
// instead of inserting a new move to satisfy dst alignment, since integer down
// conversion is based on truncation an inst has to satisfy the following
// properties:
// -- is a move (duh) and does not have conditional modifiers or saturation
// -- dst must be a direct DstRegRegion that is GRF-aligned
// -- src must be a direct SrcRegRegion with GRF base, no modifiers, and
// packed/scalar region
// -- both dst and src have integer type, with source stride > dst stride
// returns true if we have successfully down cast the src type
static bool canReplaceMovSrcType(IR_Builder &builder, G4_INST *inst,
                                 uint32_t extypesize) {

  if (inst->opcode() != G4_mov || inst->getCondMod() != NULL ||
      inst->getSaturate()) {
    return false;
  }
  if (!inst->getSrc(0)->isSrcRegRegion()) {
    return false;
  }

  G4_DstRegRegion *dst = inst->getDst();
  G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
  int dstByteOffset = dst->getByteOffset();
  if (dstByteOffset % extypesize != 0 || dst->getRegAccess() != Direct) {
    // don't do this if dst is not GRF aligned, since we have to fix it later
    // anyway
    return false;
  }

  if (src0->getRegAccess() != Direct || src0->getModifier() != Mod_src_undef ||
      (src0->getTopDcl() == NULL ||
       src0->getTopDcl()->getRegFile() != G4_GRF)) {
    return false;
  }

  bool isIntPackingMove = false;
  if (IS_TYPE_INT(dst->getType()) && IS_TYPE_INT(src0->getType())) {
    uint32_t dstAlign = dst->getTypeSize() * dst->getHorzStride();
    if (dstAlign < src0->getTypeSize()) {
      isIntPackingMove = true;
    }
  }

  if (!isIntPackingMove) {
    return false;
  }

  // we only handle direct contiguous and scalar source region for now,
  // as VxH and strided regions are a bit harder to update
  if (src0->getRegion()->isContiguous(inst->getExecSize())) {
    uint16_t newHS = extypesize / dst->getTypeSize();
    if (newHS > 4) {
      // rule out Q -> B moves if Q is not scalar
      return false;
    }
  } else if (!src0->isScalar()) {
    // only handle scalar and contiguous regions for now
    return false;
  }

  // instead of inserting a move, we change src's type to be same as dst type
  // e.g.,
  // mov (8) r1.0<1>:b r2.4<8;8,1>:d
  // becomes
  // mov (8) r1.0<1>:b r2.16<32;8,4>:b
  // This is safe since integer down conversion is based on truncation
  uint32_t typeSizeRatio = extypesize / dst->getTypeSize();
  uint32_t numElt = src0->isScalar() ? 1 : inst->getExecSize() * typeSizeRatio;
  G4_Declare *newDcl = builder.createTempVar(numElt, dst->getType(), Any);
  newDcl->setAliasDeclare(src0->getBase()->asRegVar()->getDeclare(), 0);
  auto region = src0->isScalar()
                    ? builder.getRegionScalar()
                    : builder.createRegionDesc(
                          (uint16_t)inst->getExecSize(),
                          (uint16_t)inst->getExecSize() * typeSizeRatio,
                          inst->getExecSize(), (uint16_t)typeSizeRatio);
  G4_SrcRegRegion *newSrc = builder.createSrc(
      newDcl->getRegVar(), src0->getRegOff(),
      src0->getSubRegOff() * typeSizeRatio, region, dst->getType());
  inst->setSrc(newSrc, 0);
  return true;
}

// implement HW restrictions on mov
// -- There is no direct conversion from B/UB to DF or DF to B/UB.
//    Use two instructions and a word or DWord intermediate type.
// -- There is no direct conversion from B/UB to Q/UQ or Q/UQ to B/UB.
//    Use two instructions and a word or DWord intermediate integer type.
// -- There is no direct conversion from HF to DF or DF to HF.
//    Use two instructions and F (Float) as an intermediate type.
// -- There is no direct conversion from HF to Q/UQ or Q/UQ to HF.
//    Use two instructions and F (Float) or a word integer type or a DWord
//    integer type as an intermediate type.
// -- There is no direct scalar conversion from B/UB to HF or F.
//    Use two instructions and a WORD or DWORD intermediate type respectively.
// -- There is no direct conversion from HF to Integer (DWORD or WORD).
//    Use two instructions and F (Float) as an intermediate type.
// -- There is no direct conversion from DF with conditional modifier.
//    Instead convert from DF to dst type and then compare with zero with
//    conditional modifier in another mov instruction.
// returns true if a move is inserted
bool HWConformity::fixMov(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;

  if (inst->opcode() != G4_mov) {
    return false;
  }

  G4_Type dstType = inst->getDst()->getType();
  G4_Type srcType = inst->getSrc(0)->getType();
  auto src = inst->getSrc(0);

  bool scalarByteToFloat =
      builder.noScalarByteToFloat() && IS_BTYPE(srcType) &&
      (IS_FTYPE(dstType) || IS_HFTYPE(dstType)) &&
      (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar());
  bool dstByteSrc64b =
      IS_BTYPE(dstType) && (IS_DFTYPE(srcType) || IS_QTYPE(srcType));

  if (scalarByteToFloat || dstByteSrc64b) {
    replaceDst(i, Type_W);
    return true;
  }
  if (IS_BTYPE(srcType) && (IS_DFTYPE(dstType) || IS_QTYPE(dstType))) {
    // mov Q/DF B
    replaceDst(i, Type_W);
    return true;
  }
  if (isLowPrecisionFloatTy(dstType) &&
      (IS_DFTYPE(srcType) || IS_QTYPE(srcType))) {
    // mov HF Q/DF
    replaceDst(i, Type_F);
    return true;
  }
  if (isLowPrecisionFloatTy(srcType) &&
      (IS_DFTYPE(dstType) || IS_QTYPE(dstType))) {
    // mov Q/DF HF
    replaceDst(i, Type_F);
    return true;
  }
  const bool noHFToInteger = builder.noHFToInteger() && IS_HFTYPE(srcType) &&
                             (dstType == Type_D || dstType == Type_W);
  if (noHFToInteger) {
    // mov W/DW HF
    replaceDst(i, Type_F);
    return true;
  }

  // mov (16|M0)    (eq)f0.0   r10.0<1>:q     r20.0<1;1,0>:df
  // =>
  // mov (16|M0)    r10.0<1>:q     r20.0<1;1,0>:df
  // mov (16|M0)    (eq)f0.0   null.0<1>:q     r10.0<1;1,0>:q
  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_16011698357) &&
      srcType == Type_DF && inst->getCondMod()) {
    auto newMovSrc = builder.createSrc(
        inst->getDst()->getBase(), inst->getDst()->getRegOff(),
        inst->getDst()->getSubRegOff(),
        inst->getExecSize() == g4::SIMD1
            ? builder.getRegionScalar()
            : builder.createRegionDesc(inst->getDst()->getHorzStride(), 1, 0),
        dstType);
    G4_INST *newMov =
        builder.createMov(inst->getExecSize(), builder.createNullDst(dstType),
                          newMovSrc, inst->getOption(), false);
    newMov->setPredicate(inst->getPredicate());
    newMov->setCondMod(inst->getCondMod());
    inst->setCondMod(nullptr);
    bb->insertAfter(i, newMov);
    return true;
  }
  return false;
}

bool HWConformity::fixRotate(INST_LIST_ITER i, G4_BB *bb) {

  // rotate requires src0 and dst to have the same datatype precision
  // It also does not support *B and *Q(for pre-pvc) types, but that should be
  // enforced at the vISA level returns true if new instruction is inserted
  bool changed = false;
  G4_INST *inst = *i;
  if (inst->opcode() != G4_rol && inst->opcode() != G4_ror) {
    return false;
  }

  G4_DstRegRegion *dst = inst->getDst();
  G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
  auto src1 = inst->getSrc(1);
  auto dstTy = dst->getType();
  auto src0Ty = src0->getType();
  auto src1Ty = src1->getType();

  vISA_ASSERT(IS_WTYPE(dstTy) || IS_DTYPE(dstTy) ||
                  (IS_QTYPE(dstTy) && builder.getPlatform() >= Xe_PVC),
              "dst type must be *W or *D or *Q(for PVC+)");
  vISA_ASSERT(IS_WTYPE(src0Ty) || IS_DTYPE(src0Ty) ||
                  (IS_QTYPE(src0Ty) && builder.getPlatform() >= Xe_PVC),
              "src0 type must be *W or *D or *Q(for PVC+)");
  vISA_ASSERT(IS_WTYPE(src1Ty) || IS_DTYPE(src1Ty) ||
                  (IS_QTYPE(src1Ty) && builder.getPlatform() >= Xe_PVC),
              "src1 type must be *W or *D or *Q(for PVC+)");

  if (dst->getTypeSize() != src0->getTypeSize()) {
    // Expect rotate has the same size for its src0 and its dst.
    // But visa could change the src0 to a different type
    // Use the larger of src0 and dst as rotation type and keep exec type same.
    if (dst->getTypeSize() > src0->getTypeSize()) {
      // use dst type as rotation type
      G4_Operand *newSrc = insertMovBefore(i, 0, dst->getType(), bb);
      inst->setSrc(newSrc, 0);
      vASSERT(newSrc->isSrcRegRegion());
      src0 = newSrc->asSrcRegRegion();
    } else {
      // use src type as rotation type. (note: can this happen ?)
      G4_DstRegRegion *newDst = insertMovAfter(i, dst, src0->getType(), bb);
      inst->setDest(newDst);
      dst = newDst;
    }
    // let it fall-thru
  }

  // dst must be UW/UD/UQ
  dstTy = dst->getType();
  if (IS_SIGNED_INT(dstTy))
    dst->setType(builder, getUnsignedType(TypeSize(dstTy)));

  // src0 must be UW/UD/UQ
  src0Ty = src0->getType();
  if (IS_SIGNED_INT(src0Ty))
    src0->setType(builder, getUnsignedType(TypeSize(src0Ty)));

  // src1 can only be UW/UD/UQ
  if (IS_SIGNED_INT(src1Ty)) {
    auto newSrc1Ty = getUnsignedType(TypeSize(src1Ty));
    if (src1->isImm()) {
      uint32_t immVal = (uint32_t)src1->asImm()->getImm();
      // Can not encode imm64, so truncate to UD as rotate will takes the lower
      // rotation count (5bits for UD).
      inst->setSrc(
          builder.createImm(immVal, newSrc1Ty == Type_UQ ? Type_UD : newSrc1Ty),
          1);
    } else {
      src1->asSrcRegRegion()->setType(builder, newSrc1Ty);
    }
  }

  return changed;
}

bool HWConformity::fixDstAlignment(INST_LIST_ITER i, G4_BB *bb, G4_Type extype,
                                   unsigned int dst_elsize) {
  G4_INST *inst = *i;
  bool insertMOV = false;

  unsigned char exec_size = inst->getExecSize();
  G4_DstRegRegion *dst = inst->getDst();
  G4_Operand *src0 = inst->getSrc(0);
  unsigned h_stride = dst->getHorzStride();
  unsigned int extypesize = TypeSize(extype);

  if (hasDedicateAlignRegionConformity(i)) {
    return insertMOV;
  }

  if (inst->hasNULLDst()) {
    if (dst_elsize * h_stride < extypesize) {
      uint16_t newHStride = extypesize / dst_elsize;
      if (newHStride == 8) {
        vISA_ASSERT(dst_elsize == 1, "expect B/UB dst");
        if (inst->opcode() == G4_mov && exec_size == 1 &&
            src0->isSrcRegRegion() && !src0->asSrcRegRegion()->hasModifier()) {
          // Just set src to be the same type as dst
          src0->asSrcRegRegion()->setType(builder, dst->getType());
        } else {
          replaceDst(i, Type_W);
          return true;
        }
      } else {
        vISA_ASSERT(newHStride <= 4, "horizontal stride must be <=4");
        dst->setHorzStride(newHStride);
      }
    }

    return insertMOV;
  }

  // optimize initialization instructions
  if (inst->opcode() == G4_mov && src0->isImm() &&
      (bb->isAllLaneActive() || inst->isWriteEnableInst()) &&
      !inst->getPredicate() && dst->getRegAccess() == Direct &&
      dst->getHorzStride() == 1 && inst->getSaturate() == false &&
      IS_BTYPE(dst->getType()) && !IS_TYPE_F32_F64(src0->getType()) &&
      builder.tryToAlignOperand(dst, src0->getTypeSize())) {
    // inst is a mov with packed byte dst and int imm source
    int64_t value = src0->asImm()->getInt();
    uint64_t new_value = (value & 0xFF) | (value << 0x8);
    int scale = 2;

    if (IS_DTYPE(src0->getType())) {
      scale = 4;
      new_value = (new_value & 0xFFFF) | (new_value << 0x10);
    }

    if (exec_size >= scale) {
      G4_Type new_type = (scale == 2) ? Type_UW : Type_UD;
      auto newDst = builder.createDst(dst->getBase(), dst->getRegOff(),
                                      dst->getSubRegOff() / scale, 1, new_type,
                                      dst->getAccRegSel());
      inst->setDest(newDst);
      inst->setSrc(builder.createImm(new_value, new_type), 0);
      inst->setExecSize(G4_ExecSize(exec_size / scale));
      return insertMOV;
    }
  }

  bool byteDst = IS_BTYPE(dst->getType());

  // Byte can not be used as dstination of INT*INT
  if ((byteDst && inst->opcode() == G4_mul &&
       IS_TYPE_INT(inst->getSrc(0)->getType()) &&
       IS_TYPE_INT(inst->getSrc(1)->getType()))) {
    // change dst type to W
    replaceDst(i, Type_W);
    return true;
  }

  if (byteDst && extypesize == 8) {
    // Gen doesn't support hstride 8, so we add a W move here
    replaceDst(i, Type_W);
    return true;
  }

  if (builder.hasBFMixMode() && extype == Type_F &&
      inst->getDst()->getType() == Type_BF && !inst->isDpas()) {
    // For now, BF mixed mode should not need this check.
    // If visa may allow any region as input under bf mixed mode, we need to
    // change this.
    return false;
  }

  auto allScalarSrcs = [](G4_INST *I) {
    // return true If all of I's srcs are scalar; false otherwise.
    bool allScalarSrc = true;
    for (int i = 0, nsrc = I->getNumSrc(); i < nsrc; i++) {
      G4_Operand *src = I->getSrc(i);
      if (src && !src->isNullReg() && src->isSrcRegRegion() &&
          !(src->asSrcRegRegion()->getRegion()->isScalar())) {
        allScalarSrc = false;
        break;
      }
    }
    return allScalarSrc;
  };

  // float operation requires that operands of every channel to start on the
  // same offset except for scalar operands. If dst is indirect and not all srcs
  // are scalar, assume it is not aligned.
  bool FPDstUnaligned_restriction9_7 =
      ((extype == Type_F || extype == Type_HF) &&
       dst->getRegAccess() != Direct && !allScalarSrcs(inst));

  bool dstHFMixModeInst =
      inst->getDst()->getType() == builder.getMixModeType() && extype == Type_F;
  bool dstNotAlignedToExecType = exec_size > 1 &&
                                 (dst_elsize * h_stride) < extypesize &&
                                 !(builder.hasMixMode() && dstHFMixModeInst);
  unsigned short dst_byte_offset;
  builder.tryToAlignOperand(dst, dst_byte_offset, extypesize);
  if (!((dst_byte_offset % extypesize == 0) ||
       /*
        * If destination is UB or B, the destination subregnum can be aligned
        * to byte 0 or 1 of the word execution channel.
        */
        (byteDst && (dst_byte_offset % extypesize == 1)) ||
       /*
        * If destination is UB or B, the destination subregnum can be aligned
        * to byte 0, 1, 2 or 3 of the dword execution channel.
        */
        (byteDst && extypesize == 4 && builder.supportsByteDestinationSubreg2Or3())) ||
      /*
       * Dynamic offset can be odd for serialized instructions
       * or when horizontal offset is dynamic.
       * Probably we need the same for any dst with dynamic offsets.
       */
      (dst_elsize < extypesize && dst->getRegAccess() != Direct &&
       !(byteDst && extypesize == 2 && exec_size == 1)) ||
      dstNotAlignedToExecType || FPDstUnaligned_restriction9_7) {
    /*
     * 10.3
     * For byte dst type:
     * 1. no 1 horstride
     * 2. no odd start subreg
     * There is only one excpetion - raw mov op
     * Raw means src operand has no attribute.
     *
     * Note: Actually all these cases are now controlled
     *       by extypesize value.
     */

    if (inst->isRawMov() && (dst_byte_offset % extypesize == 0 ||
                             (byteDst && dst_byte_offset % extypesize == 1))) {
      return insertMOV;
    }

    if (canReplaceMovSrcType(builder, inst, extypesize)) {
      return false;
    }

    if (inst->opcode() == G4_mov) {
      bool intHFConversion = false;
      G4_Operand *src0 = inst->getSrc(0);
      if (isLowPrecisionFloatTy(dst->getType()) &&
          IS_TYPE_INT(src0->getType())) {
        intHFConversion = true;
      } else if (isLowPrecisionFloatTy(src0->getType()) &&
                 IS_TYPE_INT(dst->getType())) {
        intHFConversion = true;
      }
      // F to packed HF operations are handled specially later
      bool FtoHFMov = dst->getType() == Type_HF && src0->getType() == Type_F;
      if (builder.getPlatform() >= GENX_CHV && !intHFConversion &&
          (inst->isMixedMode() ||
           (builder.hasFtoPackedHFMove() && FtoHFMov &&
            inst->getExecSize() >= builder.getNativeExecSize()))) {
        return insertMOV;
      }
    }

    if (splitInstListForByteDst(i, bb, (uint16_t)extypesize)) {
      return true;
    }

    inst->setDest(insertMovAfter(i, dst, dst->getType(), bb));
    insertMOV = true;
  }

  return insertMOV;
}

void HWConformity::fixPredicateIndirectInst(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = (*it);
  if (inst->getPredicate() && inst->getDst() && !inst->getDst()->isNullReg() &&
      inst->getDst()->getRegAccess() == Direct) {
    bool hasIndirectSource = false;
    for (int i = 0; i < inst->getNumSrc(); i++) {
      G4_Operand *opnd = inst->getSrc(i);

      if (opnd && opnd->isSrcRegRegion() &&
          opnd->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        if (inst->opcode() == G4_sel) {
          replaceSrc(it, i, opnd->getType(), bb);
        } else {
          hasIndirectSource = true;
          break;
        }
      }
    }

    if (hasIndirectSource) {
      replaceDst(it, inst->getDst()->getType());
    }
  }
}

// Workaround for compressed instructions with indirect src0:
// For a compressed instruction with 2 passes, if src0 have indirect 1x1
// addressing mode then src0 must not have GRF crossover in the first pass. A
// move must be introduced with GRF offset 0 to avoid src0 and destination
// crossover in the same instruction.
bool HWConformity::fixIndirectSrcForCompressedInst(INST_LIST_ITER i, G4_BB *bb) {
  if (!builder.needIndirectSrcForCompressedInstWA())
    return false;

  G4_INST *inst = *i;
  if (inst->isSend() || inst->isDpas() || inst->opcode() == G4_nop ||
      inst->opcode() == G4_madm || inst->isLabel() ||
      inst->isIntrinsic())
    return false;

  // apply WA to inst with 1x1 indirect src0
  G4_Operand *src0 = inst->getSrc(0);
  if (!src0 || !src0->isIndirect())
    return false;

  auto src0Region = src0->asSrcRegRegion()->getRegion();
  if (src0Region->isRegionWH() || src0Region->isRegionV() ||
      src0Region->isScalar())
    return false;

  G4_Operand *dst = inst->getDst();
  if (!dst || dst->isNullReg())
      return false;

  // apply WA to inst with indirect or cross-grf access dst
  if (!dst->isIndirect() && !dst->asDstRegRegion()->crossGRF(builder))
    return false;

  // 1. move dst to a grf-aligned reg
  //    Do the copy whenever dst stride is not 1 to avoid getting
  //    an invalid Execution Mask Offset during "evenlySplitInst" (that is,
  //    avoid splitting simd8 instructions), with the assumption that this WA
  //    won't apply to GRF size 32 platforms.
  if (dst->isIndirect() || dst->asDstRegRegion()->getHorzStride() != 1 ||
      !builder.tryToAlignOperand(dst, builder.getGRFSize()))
    replaceDst(i, dst->getType(), builder.getGRFAlign());

  // 2. evenly split the instruction so the dst of splitted instructions
  //    can't cross grf
  if (inst->getDst()->asDstRegRegion()->crossGRF(builder)) {
    // Set checkOverlap to false since compressed instructions cannot have
    // overlapped operands. Hence the splitted result must not have extraMov
    evenlySplitInst(i, bb, false);
  }
  return true;
}

/*
 * This function evenly splits movi from simd16 to simd8.
 */
bool HWConformity::fixIndirectMoviSimd16ToSimd8(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->opcode() != G4_movi)
    return false;
  if (inst->getExecSize() == g4::SIMD16) {
    // split the instruction
    evenlySplitInst(i, bb);
  }
  return true;
}

/*
 * This function checks to see if the instruction's indirect operands
 * potentially require totally more than 8 distinct addr reg sub-registers, and
 * then determines which of the operands to spill into temporary GRFs so
 * as to limit total number of distinct sub-registers used by the instruction
 * to 8. This is a requirement imposed by the CM register allocator.
 */

bool HWConformity::fixIndirectOpnd(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;

  G4_Operand *src0 = inst->getSrc(0), *src1 = inst->getSrc(1);
  G4_DstRegRegion *dst = inst->getDst();
  bool null_dst = (!dst || inst->hasNULLDst());

  bool null_src0 = !src0;
  bool null_src1 = !src1 || (inst->isMath() && src1->isNullReg());

  const int addr_reg_max_count = 16;
  int src_uniq_count = 0;
  int src1_count = 0;
  int src0_count = 0;
  int dst_uniq_count = 0;
  int dst_count = 0;
  bool nospill_src1 = false;
  bool nospill_src0 = false;
  [[maybe_unused]] bool nospill_dst = false;
  bool spill_src1 = false;
  bool spill_src0 = false;
  bool spill_dst = false;
  G4_Declare *addr_dcl0 = NULL, *addr_dcl1 = NULL, *addr_dcl2 = NULL;
  if (!null_src0 && src0->isSrcRegRegion() && src0->isIndirect() &&
      src0->asSrcRegRegion()->getBase()->isRegVar()) {
    addr_dcl0 = src0->asSrcRegRegion()
                    ->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getRootDeclare();
    // is the following precise?
    src0_count = addr_dcl0->getTotalElems();
    vISA_ASSERT(src0_count <= addr_reg_max_count,
                 "More than 8 address subregisters required for one operand.");
    src_uniq_count += src0_count;
  }

  if (!null_src1 && src1->isSrcRegRegion() && src1->isIndirect() &&
      src1->asSrcRegRegion()->getBase()->isRegVar()) {
    addr_dcl1 = src1->asSrcRegRegion()
                    ->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getRootDeclare();
    src1_count = addr_dcl1->getTotalElems();
    vISA_ASSERT(src1_count <= addr_reg_max_count,
                 "More than 8 address subregisters required for one operand.");
    if (addr_dcl1 != addr_dcl0) {
      // should we use top level dcl here?
      src_uniq_count += src1_count;
    } else {
      nospill_src1 = true;
      nospill_src0 = true;
    }
  }

  if (!null_dst && dst->getRegAccess() != Direct &&
      dst->getBase()->isRegVar()) {
    addr_dcl2 = dst->getBase()->asRegVar()->getDeclare()->getRootDeclare();
    dst_count = addr_dcl2->getTotalElems();
    vISA_ASSERT(dst_count <= addr_reg_max_count,
                 "More than 8 address subregisters required for one operand.");
    if (addr_dcl2 != addr_dcl0 && addr_dcl2 != addr_dcl1) {
      dst_uniq_count += dst_count;
    } else if (addr_dcl2 != addr_dcl0) {
      nospill_dst = true;
      nospill_src0 = true;
    } else {
      nospill_dst = true;
      nospill_src1 = true;
    }
  }

  if (src_uniq_count > addr_reg_max_count) {
    if (src0_count > src1_count || nospill_src1) {
      vISA_ASSERT(nospill_src0 == false,
                   "Address of source0 should be spilled.");
      spill_src0 = true;
      src_uniq_count -= src0_count;
    } else {
      vISA_ASSERT(nospill_src1 == false,
                   "Address of source1 should be spilled.");
      spill_src1 = true;
      src_uniq_count -= src1_count;
    }
  }

  if (src_uniq_count + dst_uniq_count > addr_reg_max_count) {
    vISA_ASSERT(nospill_dst == false, "Address of dst should be spilled.");

    if (nospill_src1 && nospill_src0) {
      spill_dst = true;
      dst_uniq_count = 0;
    } else if (dst_uniq_count > src0_count && dst_uniq_count > src1_count) {
      spill_dst = true;
      dst_uniq_count = 0;
    } else if (spill_src0) {
      spill_src1 = true;
      src_uniq_count -= src1_count;
    } else if (spill_src1) {
      spill_src0 = true;
      src_uniq_count -= src0_count;
    } else if (src0_count > src1_count) {
      spill_src0 = true;
      src_uniq_count -= src0_count;
    } else {
      spill_src1 = true;
      src_uniq_count -= src1_count;
    }
  }

  vISA_ASSERT(src_uniq_count + dst_uniq_count <= addr_reg_max_count,
               "Remianed number of address registers should be no more than 8 "
               "after spill.");

  // Is this only for iselect?
  // What if a scalar with indirect addressing is used?
  if (spill_src0) {
    G4_Operand *new_src0 = insertMovBefore(i, 0, src0->getType(), bb);
    inst->setSrc(new_src0, 0);
  }

  if (spill_src1 && src1) {
    G4_Operand *new_src1 = insertMovBefore(i, 1, src1->getType(), bb);
    inst->setSrc(new_src1, 1);
  }

  if (spill_dst && dst) {
    G4_DstRegRegion *new_dst = insertMovAfter(i, dst, dst->getType(), bb);
    inst->setDest(new_dst);
    if (dst != new_dst &&
        (IS_FTYPE(dst->getType()) || IS_DFTYPE(dst->getType()))) {
      inst->setSaturate(g4::NOSAT);
    }
  }
  return spill_dst;
}

// If an accumulator is a implicilit src/dst or explicit src operand, its
// register region must match that of the destination register, which means
// dst must be GRF-aligned and hstride must be 1 since we always GRF-align
// Acc and use <1;1,0> for Acc source.
// Also check for restrictions on explicit acc dst.
bool HWConformity::fixAcc(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;

  bool changed = false;
  auto dst = inst->getDst();
  if ((dst && dst->isAccReg()) || inst->opcode() == G4_mach) {
    if (!builder.accDstforIndirectSrc()) {
      if (inst->getSrc(0)->isSrcRegRegion() &&
          inst->getSrc(0)->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        inst->setSrc(insertMovBefore(iter, 0, inst->getSrc(0)->getType(), bb),
                     0);
        changed = true;
      }
    }
  }

  // Implicit acc src/dst and explicit acc src must be aligned with dst GRF
  bool useAcc = inst->hasImplicitAccSrc() || inst->hasImplicitAccDst();
  if (!useAcc) {
    for (int i = 0; i < inst->getNumSrc(); ++i) {
      G4_Operand *src = inst->getSrc(i);
      if (src && src->isAccReg()) {
        useAcc = true;
        break;
      }
    }
  }

  if (useAcc && dst && dst->getBase() && dst->getBase()->isRegVar()) {
    if (!builder.tryToAlignOperand(dst, kernel.numEltPerGRF<Type_UB>()) ||
        dst->getHorzStride() != 1) {
      inst->setDest(insertMovAfter(iter, dst, dst->getType(), bb, /*stride*/ 1,
                                   builder.getGRFAlign()));
      changed = true;
    }
  }

  return changed;
}

/*
 * When operation execution size is 1, destination horizontal stride is set
 * according to rule 10.2:
 *
 * 10.1.2. If ExecSize is greater than 1, dst.HorzStride*sizeof(dst.Type) must
 *         be equal to or greater than the size of the execution data type.
 * 10.2. If ExecSize is 1, dst.HorzStride must not be 0. Note that this is
 *       relaxed from rule 10.1.2. Also note that this rule for destination
 *       horizontal stride is different from that for source as stated
 *       in rule #7.
 *
 * There are some instructions which work unpredictably if both ExecSize
 * and dst.HorzStride are 1. But they work fine if dst.HorzStride is set
 * according to rule 10.1.2. So we have to correct all such cases.
 *
 * This supposed to be the last operation before emitting final assembly code.
 */
void HWConformity::fixDstHstride(INST_LIST_ITER i, int extypesize) {
  G4_INST *inst = *i;
  G4_DstRegRegion *dst = inst->getDst();

  if (dst) {
    int dst_elsize = dst->getTypeSize();
    unsigned short hs = dst->getHorzStride();
    if (hs * dst_elsize < extypesize) {
      dst->setHorzStride((unsigned short)(extypesize / dst_elsize));
    }
  }
}

template <class T> bool isPreAssignedRegOffsetNonZero(T *region) {
  // T is non-NULL and either
  // G4_SrcRegRegion or G4_DstRegRegion
  bool ret = false;

  if ((region->isSrcRegRegion() || region->isDstRegRegion()) &&
      region->getBase() && region->getBase()->isRegVar() &&
      region->getBase()->asRegVar()->isPhyRegAssigned() &&
      region->getBase()->asRegVar()->getPhyRegOff() != 0) {
    ret = true;
  }

  return ret;
}

void HWConformity::generateMacl(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *mulInst = *it;
  vISA_ASSERT(mulInst->opcode() == G4_mul, "expect mul instruction");
  if (mulInst->getExecSize() > builder.getNativeExecSize()) {
    auto startIter = it;
    bool isFirstInst = startIter == bb->begin();
    if (!isFirstInst) {
      --startIter;
    }
    evenlySplitInst(it, bb);
    if (!isFirstInst) {
      ++startIter;
    }
    // startIter now points to first mul created by split
    auto endIter = it;
    ++endIter;
    // endIter points to the first inst after the original mul
    for (auto iter = startIter; iter != endIter;) {
      auto nextIter = iter;
      ++nextIter;
      G4_INST *currInst = *iter;
      if (currInst->opcode() == G4_mul) {
        doGenerateMacl(iter, bb);
      }
      iter = nextIter;
    }
  } else {
    doGenerateMacl(it, bb);
  }
}

// convert vISA mul (8) dst src0 src1 into
// mul (8) acc0.0<1>:d src0:d src1:w
// mach (8) dst:d src0:d src1:d
//
void HWConformity::doGenerateMacl(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *mulInst = *it;
  vISA_ASSERT(mulInst->opcode() == G4_mul, "expect mul instruction");
  vISA_ASSERT(mulInst->getExecSize() <= builder.getNativeExecSize(),
         "expect single register inst");

  G4_Operand *src0 = mulInst->getSrc(0);
  G4_Operand *src1 = mulInst->getSrc(1);
  vISA_ASSERT(IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()),
               "both sources must have dword type");

  // src1 does not support modifier
  checkSrcMod(it, bb, 1);
  // fix src1 region: stride can't exceed 4, otherwise the stride of src1 in the
  // expanded mul will be invalid mulh dst:d src0:d src1:d
  //  =>
  // mul acc0:d src0:d src1:uw
  // mach dst:d src0:d src1:d
  fixSrc1Region(it, bb);
  src1 = mulInst->getSrc(1);

  if (!builder.supportSrcModforMul()) {
    checkSrcMod(it, bb, 0);
    src0 = mulInst->getSrc(0);
  }

  // sat cannot be used at all in the macro sequence
  // this effectivly means sat is broken for mul D D D
  mulInst->setSaturate(g4::NOSAT);

  G4_DstRegRegion *origDst = mulInst->getDst();
  G4_Type tmpType =
      (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()))
          ? Type_UD
          : Type_D;
  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14013677893) &&
      builder.getOption(vISA_expandMulPostSchedule)) {
       // Here just create tmp variables to fix srcMod, cond modifier, saturate,
       // etc. And Mul->Mul+Macl expanding will be done in expandMulPostSchedule
       // pass.

    // Need extra mov if dst is acc and src0 is indirect
    if (!builder.accDstforIndirectSrc()) {
      if (src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        mulInst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
      }
    }

    // need extra move for dst
    if (!IS_DTYPE(origDst->getType()) || origDst->getHorzStride() != 1 ||
        !builder.tryToAlignOperand(origDst, builder.getGRFSize())) {
      // macl dst must be grf-aligned, packed D/UD as it is also used for the
      // implicit acc source's region
      G4_DstRegRegion *tmpDst =
          insertMovAfter(it, origDst, tmpType, bb, builder.getGRFAlign());
      mulInst->setDest(tmpDst);
    }

    // set implicit acc dst to the mul instruction as acc will be used as dst of
    // the expanded mul after local scheduling. it is a must to fix the WAR/WAW
    // issue of acc in local scheduling.
    G4_DstRegRegion *accDstOpnd = builder.createDst(
        builder.phyregpool.getAcc0Reg(), 0, 0, 1, mulInst->getDst()->getType());
    mulInst->setImplAccDst(accDstOpnd);
  } else {
    G4_DstRegRegion *accDstOpnd =
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
    mulInst->setDest(accDstOpnd);

    uint32_t origOptions = mulInst->getOption();
    fixMulSrc1(it, bb);
    mulInst->setOptionOn(InstOpt_WriteEnable);

    G4_Predicate *predicate = mulInst->getPredicate();
    if (predicate != nullptr) {
      // move pred to mach
      mulInst->setPredicate(nullptr);
    }
    if (mulInst->getCondMod() != nullptr) {
      // conditional modifier cannot be used
      // when the MUL source operand is of dword type.
      vISA_ASSERT(false, "Dw multiply does not support conditional modifiers");
      mulInst->setCondMod(nullptr);
    }

    // create a macl inst
    G4_INST *maclInst = builder.createMacl(
        mulInst->getExecSize(), origDst, builder.duplicateOperand(src0),
        builder.duplicateOperand(src1), origOptions, tmpType);
    maclInst->setPredicate(predicate);

    // maintain du chain as fixAccDst uses it later
    mulInst->addDefUse(maclInst, Opnd_implAccSrc);

    INST_LIST_ITER machIter = it;
    machIter = bb->insertBefore(++machIter, maclInst);

    if (!IS_DTYPE(origDst->getType()) || origDst->getHorzStride() != 1 ||
        !builder.tryToAlignOperand(origDst, builder.getGRFSize())) {
      // macl dst must be grf-aligned, packed D/UD as it is also used for the
      // implicit acc source's region
      G4_DstRegRegion *tmpDst =
          insertMovAfter(machIter, origDst, tmpType, bb, builder.getGRFAlign());
      maclInst->setDest(tmpDst);
    }
  }
}

// get rid of source modifiers on this inst[srcPos]
bool HWConformity::checkSrcMod(INST_LIST_ITER it, G4_BB *bb, int srcPos) {
  bool changed = false;
  G4_INST *inst = *it;
  vISA_ASSERT(srcPos < inst->getNumSrc(), "invalid srcPos");
  auto src = inst->getSrc(srcPos);
  if (src->isSrcRegRegion()) {
    G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
    if (srcRegion->getModifier() != Mod_src_undef) {
      G4_Type type = IS_DTYPE(src->getType()) ? src->getType() : Type_D;
      src = insertMovBefore(it, srcPos, type, bb);
      inst->setSrc(src, srcPos);
      changed = true;
    }
  }
  return changed;
}

// If both source operands of an MUL instruction are of dword integer type,
// only the lower 16 bits of data elements in src0 are used.
// The full precision multiplication results can be only produced together
// with the mach and mov instructions.

bool HWConformity::fixMULInst(INST_LIST_ITER &i, G4_BB *bb) {
  bool insertedInst = false;
  G4_INST *inst = *i;
  G4_DstRegRegion *dst = inst->getDst();
  G4_ExecSize execSize = inst->getExecSize();

  if (dst->isAccReg()) {
    return false;
  }

  uint32_t inst_opt = inst->getOption();
  G4_Operand *src0 = inst->getSrc(0), *src1 = inst->getSrc(1);

  // MUL is commutative and only
  // allows src1 to be a constant.
  // If src0 is a constant and src1
  // is not, they are swapped here.
  // If both are constants, they
  // will be fixed in checking HW conformity.
  // this is fixed in fixOpnd.

  if (src0->isImm() && !src1->isImm()) {
    inst->swapSrc(0, 1);
  }

  if (!builder.supportSrcModforMul() &&
      (IS_DTYPE(src0->getType()) || IS_DTYPE(src1->getType())) &&
      ((src0->getTypeSize()) < 4 || (src1->getTypeSize()) < 4))

  {
    checkSrcMod(i, bb, 0);
    checkSrcMod(i, bb, 1);
  }

  src0 = inst->getSrc(0);
  src1 = inst->getSrc(1);
  // Q dst needs 64-bit support regardless of src type
  bool isDMul = IS_QTYPE(dst->getType()) ||
                (IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()));

  if (!isDMul) {
    return false;
  }

  if (builder.hasMacMacl() && !IS_QTYPE(dst->getType()) &&
      !builder.getOption(vISA_EnableInt32DstMulMad) &&
      !builder.hasSimplifiedRegions() &&
      (builder.noDwDstForDwordMul() || inst->getExecSize() > g4::SIMD1)) {
    // use macl for D = D x D. We use macl when possible
    // except on scalar inst on platforms that support native DMul
    generateMacl(i, bb);
    return true;
  }

  bool doNativeMul = false;
  if (builder.hasQDDMul()) {
    // platform natively supports DW-DW multiply, no need to generate
    // mul/mach/mov sequence
    doNativeMul = true;
  } else {
    if ((builder.getPlatform() == GENX_CHV ||
         builder.getPlatform() == GENX_BXT)) {
      if (inst->getExecSize() == g4::SIMD1) {
        // scalar insts are a-ok
        return false;
      }
      // ok if source is scalar or qword-aligned
      doNativeMul = dst->getTypeSize() * dst->getHorzStride() == 8;
      auto isQWordStride = [inst, this](G4_SrcRegRegion *src) {
        const RegionDesc *region = src->getRegion();
        if (!region->isScalar()) {
          uint16_t stride = 0;
          (void)region->isSingleNonUnitStride(inst->getExecSize(), stride);
          if (stride != 2) {
            return false;
          }
          // check that source is GRF-aligned to ensure that every element is
          // qword-aligned
          return builder.tryToAlignOperand(src, 32);
        }
        return true;
      };
      if (doNativeMul && src0->isSrcRegRegion()) {
        doNativeMul = isQWordStride(src0->asSrcRegRegion());
      }
      if (doNativeMul && src1->isSrcRegRegion()) {
        doNativeMul = isQWordStride(src1->asSrcRegRegion());
      }
    }
  }

  if (doNativeMul) {
    // promote source to D type if necessary
    if (IS_QTYPE(dst->getType())) {
      G4_Type newTy;
      G4_Operand *newOpnd;
      if (!IS_DTYPE(src0->getType())) {
        newTy = IS_SIGNED_INT(src0->getType()) ? Type_D : Type_UD;
        newOpnd = insertMovBefore(i, 0, newTy, bb);
        inst->setSrc(newOpnd, 0);
        insertedInst = true;
      }

      if (!IS_DTYPE(src1->getType())) {
        newTy = IS_SIGNED_INT(src1->getType()) ? Type_D : Type_UD;
        if (src1->isImm()) {
          newOpnd = builder.createImm(src1->asImm()->getImm(), newTy);
        } else {
          newOpnd = insertMovBefore(i, 1, newTy, bb);
        }
        inst->setSrc(newOpnd, 1);
        insertedInst = true;
      }
    }
    return insertedInst;
  }

  // both sources are dword, replace with mul/mach/mov sequence
  // At this point, src0 and src1 are both DW, so we simply make
  // acc's type (i.e. dst_type) be DW/UD

  G4_CondMod *condmod = builder.duplicateOperand(inst->getCondMod());
  G4_Predicate *pred = builder.duplicateOperand(inst->getPredicate());

  G4_Type tmp_type =
      (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()))
          ? Type_UD
          : Type_D;

  // src1 does not support modifier
  checkSrcMod(i, bb, 1);
  src1 = inst->getSrc(1);

  if (!builder.supportSrcModforMul()) {
    checkSrcMod(i, bb, 0);
    src0 = inst->getSrc(0);
  }

  auto satMod = inst->getSaturate();
  inst->setSaturate(g4::NOSAT);

  G4_DstRegRegion *acc_dst_opnd =
      builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmp_type);
  inst->setDest(acc_dst_opnd);
  fixMulSrc1(i, bb);

  inst->setNoMask(true);

  if (pred) {
    // conditional modifier cannot be used
    // when the MUL source operand is of dword type.
    inst->setCondMod(nullptr);
  }

  // Dst is either null, or a temp D if the original dst is Q/UQ
  G4_DstRegRegion *machDst = NULL;
  G4_Declare *high32BitDcl = NULL;
  if (IS_QTYPE(dst->getType())) {
    high32BitDcl = builder.createTempVar(execSize, Type_D, Any);
    machDst = builder.createDstRegRegion(high32BitDcl, 1);
  } else {
    machDst = builder.createNullDst(Type_D);
  }

  // create a mach inst
  G4_INST *newInst =
      builder.createMach(execSize, machDst, builder.duplicateOperand(src0),
                         builder.duplicateOperand(src1), inst_opt, tmp_type);

  INST_LIST_ITER iter = i;
  iter++;
  bb->insertBefore(iter, newInst);

  inst->setPredicate(nullptr);

  inst->copyDef(newInst, Opnd_src0, Opnd_src0);
  inst->copyDef(newInst, Opnd_src1, Opnd_src1);
  inst->transferUse(newInst);
  inst->addDefUse(newInst, Opnd_implAccSrc);

  // create an explciit acc source for later use
  const RegionDesc *rd =
      execSize > 1 ? builder.getRegionStride1() : builder.getRegionScalar();
  G4_SrcRegRegion *acc_src_opnd =
      builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0, rd, tmp_type);

  insertedInst = true;

  if (IS_QTYPE(dst->getType())) {
    // we have to produce two additional moves to form the Q/UQ:
    // mul (8) acc0:d r2.0<8;8,1>:d r3.0<16;8,2>:uw
    // mach (8) r5.0<1>:d r2.0<8;8,1>:d r3.0<8;8,1>:d
    // mov (8) r6.0<1>:d acc0:d  // Low 32 bits.
    // mov (8) dst.0<2>:d r6.0<1>:d
    // mov (8) dst.1<2>:d r5.0<1>:d
    // Note that we don't try to combine the moves because of the HW restriction
    // that "If an accumulator is an explicit source operand, its register
    // region must match that of the destination register"

    G4_Declare *low32BitDcl = builder.createTempVar(execSize, Type_D, Any);
    G4_INST *movInst = builder.createMov(
        execSize, builder.createDstRegRegion(low32BitDcl, 1),
        builder.createSrcRegRegion(*acc_src_opnd), inst_opt, false);
    bb->insertBefore(iter, movInst);

    G4_DstRegRegion *origDst = dst;
    bool needsExtraMov =
        origDst->getHorzStride() > 1 || condmod != NULL || satMod;

    G4_Declare *dstAlias = builder.createTempVar(execSize * 2, Type_D, Any);
    if (!needsExtraMov) {
      uint32_t aliasOffset =
          origDst->getRegOff() * kernel.numEltPerGRF<Type_UB>() +
          origDst->getSubRegOff() * 8;
      dstAlias->setAliasDeclare(origDst->getBase()->asRegVar()->getDeclare(),
                                aliasOffset);
    }
    G4_INST *lowMove = builder.createMov(
        execSize, builder.createDstRegRegion(dstAlias, 2),
        builder.createSrcRegRegion(low32BitDcl, builder.getRegionStride1()),
        inst_opt, false);
    lowMove->setPredicate(pred);

    bb->insertBefore(iter, lowMove);

    vISA_ASSERT(high32BitDcl != NULL, "mach dst must not be null");
    G4_INST *highMove = builder.createMov(
        execSize,
        builder.createDst(dstAlias->getRegVar(), 0, 1, 2,
                          dstAlias->getElemType()),
        builder.createSrcRegRegion(high32BitDcl, builder.getRegionStride1()),
        inst_opt, false);
    highMove->setPredicate(pred);
    bb->insertBefore(iter, highMove);

    if (needsExtraMov) {
      // this will take care of non-packed dst/cond mod/saturate
      G4_Declare *dstAliasAsQ = builder.createTempVar(execSize, Type_Q, Any);
      dstAliasAsQ->setAliasDeclare(dstAlias, 0);
      G4_INST *moveInst = builder.createMov(
          execSize, dst,
          builder.createSrcRegRegion(dstAliasAsQ, builder.getRegionStride1()),
          inst_opt, false);
      moveInst->setCondMod(condmod);
      moveInst->setSaturate(satMod);
      bb->insertBefore(iter, moveInst);
    }

    return true;
  }

  INST_LIST_ITER last_iter;
  // create a mov inst
  if (satMod == g4::NOSAT) {
    bool extra_mov = dst && dst->getExecTypeSize() > TypeSize(Type_D);
    extra_mov |= isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst);

    G4_INST *movInst = builder.createMov(
        execSize, dst, builder.createSrcRegRegion(*acc_src_opnd), inst_opt,
        false);
    movInst->setPredicate(pred);
    movInst->setCondMod(condmod);

    newInst->transferUse(movInst);
    newInst->addDefUse(movInst, Opnd_src0);

    bb->insertBefore(iter, movInst);
    last_iter = iter;
    last_iter--;
    if (extra_mov) {
      // add a tmp mov
      iter--;
      G4_DstRegRegion *new_next_dst =
          insertMovAfter(iter, dst, dst->getType(), bb);
      movInst->setDest(new_next_dst);
      movInst->setPredicate(NULL);
    }
  } else {
    // create an extra mov inst
    G4_Declare *dcl =
        builder.createTempVar(execSize, tmp_type, builder.getGRFAlign());

    G4_DstRegRegion *tmp_dst_opnd =
        builder.createDst(dcl->getRegVar(), 0, 0, 1, tmp_type);
    G4_INST *movInst = builder.createMov(
        execSize, tmp_dst_opnd, builder.createSrcRegRegion(*acc_src_opnd),
        InstOpt_NoOpt, false);
    movInst->setCondMod(condmod);
    bb->insertBefore(iter, movInst);

    last_iter = iter;
    last_iter--;

    G4_SrcRegRegion *tmp_src_opnd =
        builder.createSrc(dcl->getRegVar(), 0, 0, rd, tmp_type);

    G4_INST *newInst2 =
        builder.createInternalInst(pred, G4_mov, condmod, satMod, execSize, dst,
                                   tmp_src_opnd, NULL, inst_opt);

    newInst->transferUse(newInst2);
    newInst->addDefUse(movInst, Opnd_src0);
    movInst->addDefUse(newInst2, Opnd_src0);
    bb->insertBefore(iter, newInst2);
    iter++;
  }

  if (execSize > builder.getNativeExecSize()) {
    splitDWMULInst(i, last_iter, bb);
  }

  return insertedInst;
}

// Translate MULH into
// MUL acc src0 src1
// MACH dst src0 src1
void HWConformity::fixMULHInst(INST_LIST_ITER &i, G4_BB *bb) {
  G4_INST *inst = *i;
  G4_ExecSize execSize = inst->getExecSize();

  int inst_opt = inst->getOption();

  G4_Operand *src0 = inst->getSrc(0), *src1 = inst->getSrc(1);

  if (src0->isImm() && !src1->isImm()) {
    inst->swapSrc(0, 1);
    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
  }

  if (builder.hasSimplifiedRegions() ||
      builder.getOption(vISA_EURegionRemoval)) {
    // Use mullh to get high 32-bit results
    //   mullh  (16) tmp<1>:d   src0<1;1,0>:d    src1<1;1,0>:d
    //   mov    (16) dst<1>:d   tmp_hi32<1;1,0>:d
    G4_DstRegRegion *dst = inst->getDst();
    vISA_ASSERT(IS_DTYPE(dst->getType()), "mulh dst only supports DW type");

    // Fix immediate type as mulh can only support D/UD types
    if (src1->isImm()) {
      uint32_t immVal = (uint32_t)(src1->asImm()->getImm());
      inst->setSrc(builder.createImm(immVal, IS_SIGNED_INT(src1->getType())
                                     ? Type_D : Type_UD),
                   1);
    }

    auto doTranslateMulh{[&](INST_LIST_ITER it) {
      G4_INST *inst = *it;
      auto execSize = inst->getExecSize();

      // Create mullh instruction:
      //   mullh (16)  tmp<1>:d  src0<1;1,0>:d    src1<1;1,0>:d
      uint32_t origOptions = inst->getOption();
      auto origPredicate = inst->getPredicate();
      auto originSat = inst->getSaturate();
      auto dst = inst->getDst();
      auto src0 = inst->getSrc(0);
      auto src1 = inst->getSrc(1);

      G4_Declare *tmpDcl =
          builder.createTempVar(builder.numEltPerGRF(dst->getType()) * 2,
                                dst->getType(), builder.getGRFAlign());
      G4_DstRegRegion *tmpDst = builder.createDstRegRegion(tmpDcl, 1);
      auto mullhInst = builder.createBinOp(
          G4_mullh, execSize, tmpDst, builder.duplicateOperand(src0),
          builder.duplicateOperand(src1), origOptions, false);
      mullhInst->setVISAId(inst->getVISAId());
      (*it) = mullhInst;

      // Create mov instruction:
      //   mov    (16) dst<1>:d   tmp_hi32<1;1,0>:d
      G4_Declare *tmpHiDcl =
          builder.createTempVar(builder.numEltPerGRF(dst->getType()),
                                dst->getType(), builder.getGRFAlign());
      tmpHiDcl->setAliasDeclare(tmpDcl, builder.getGRFSize());
      G4_SrcRegRegion *tmpHiSrc =
          builder.createSrcRegRegion(tmpHiDcl, builder.getRegionStride1());
      G4_INST *hiMovInst = builder.createMov(execSize, dst, tmpHiSrc,
                                             inst->getMaskOption(), false);
      hiMovInst->setPredicate(origPredicate);
      hiMovInst->setSaturate(originSat);
      bb->insertAfter(it, hiMovInst);
    }};

    if (inst->getExecSize() > builder.getNativeExecSize()) {
      auto startIter = i;
      bool isFirstInst = startIter == bb->begin();
      if (!isFirstInst) {
        --startIter;
      }
      evenlySplitInst(i, bb);
      if (!isFirstInst) {
        ++startIter;
      }
      // startIter now points to first mulh created by split
      auto endIter = i;
      ++endIter;
      // endIter points to the first inst after the original mulh
      for (auto iter = startIter; iter != endIter;) {
        auto nextIter = iter;
        ++nextIter;
        G4_INST *currInst = *iter;
        if (currInst->opcode() == G4_mulh) {
          doTranslateMulh(iter);
        }
        iter = nextIter;
      }
    } else {
      doTranslateMulh(i);
    }

    return;
  }

  bool useMulQDD = false;
  if (execSize <= builder.getNativeExecSize() && builder.hasQDDMul() &&
      builder.supportFloatOr64bRegioning()) {
    useMulQDD = true;
    if (!IS_DTYPE(src0->getType()) || !IS_DTYPE(src1->getType())) {
      if (src1->isImm() && IS_DTYPE(src0->getType()) &&
          (IS_WTYPE(src1->getType()) || IS_BTYPE(src1->getType()))) {
        // Ensure src1 has the same type size as src0.
        const G4_Imm *oldImm = src1->asImm();
        G4_Imm *newImm = builder.createImm(oldImm->getInt(), src0->getType());
        inst->setSrc(newImm, 1);
      } else {
        useMulQDD = false;
      }
    }
  }
  if (useMulQDD) {
    // use mul Q D D to get the upper 32-bit
    // note that we don't do this for CHV/BXT due to the 64-bit type
    // restrictions
    inst->setOpcode(G4_mul);
    G4_DstRegRegion *dst = inst->getDst();
    G4_Type dstType = dst->getType();

    if (dstType == Type_UD)
      dstType = Type_UQ;
    else
      dstType = Type_Q;
    G4_Declare *dstDcl = dst->getBase()->asRegVar()->getDeclare();
    G4_Declare *tmpDcl = builder.createTempVar(execSize, dstType, Any, "TV");
    tmpDcl->copyAlign(dstDcl);

    G4_DstRegRegion *tmpDst = builder.createDstRegRegion(tmpDcl, 1);
    inst->setDest(tmpDst);

    // need move to cast back to D/UD type
    G4_SrcRegRegion *tmpSrc = builder.createSrc(
        tmpDcl->getRegVar(), 0, 1,
        execSize > 1 ? builder.getRegionStride2() : builder.getRegionScalar(),
        dst->getType());

    G4_INST *tmpMov =
        builder.createMov(execSize, dst, tmpSrc, inst->getOption(), false);
    tmpMov->setPredicate(builder.duplicateOperand(inst->getPredicate()));

    bb->insertAfter(i, tmpMov);

    // Check the new inserted mov inst
    i++;

    // Need to remove dst from uses list of mulh, and add them to movInst
    // useList add movInst to uselist of mulh. Add mulh to def instruction list
    // of movInst
    inst->transferUse(tmpMov);
    inst->addDefUse(tmpMov, Opnd_src0);
    return;
  }

  // src1 does not support modifier
  checkSrcMod(i, bb, 1);
  // fix src1 region: stride can't exceed 4, otherwise the stride of src1 in the
  // expanded mul will be invalid mulh dst:d src0:d src1:d
  //  =>
  // mul acc0:d src0:d src1:uw
  // mach dst:d src0:d src1:d
  fixSrc1Region(i, bb);
  src1 = inst->getSrc(1);

  if (!builder.supportSrcModforMul()) {
    checkSrcMod(i, bb, 0);
    src0 = inst->getSrc(0);
  }

  G4_Type tmp_type =
      (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()))
          ? Type_UD
          : Type_D;

  vISA_ASSERT(IS_DTYPE(src0->getType()), "src0 must be DW type");

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14013677893) &&
      builder.getOption(vISA_expandMulPostSchedule)) {
       // Here just create tmp variables to fix srcMod, cond modifier, saturate,
       // etc. And Mul->Mul + Macl expanding will be done in
       // expandMulPostSchedule pass.

    if (src1->isImm() && src0->getType() != src1->getType()) {
      G4_Imm *oldImm = src1->asImm();
      // Ensure src1 has the same type as src0.
      inst->setSrc(builder.createImm(oldImm->getInt(), src0->getType()), 1);
    } else if (!IS_DTYPE(src1->getType())) {
      // this can happen due to vISA opt, convert them to src0 type which should
      // be D/UD We use D as the tmp type to make sure we can represent all src1
      // values
      inst->setSrc(insertMovBefore(i, 1, Type_D, bb), 1);
    }

    // Need extra mov if dst is acc and src0 is indirect
    if (!builder.accDstforIndirectSrc()) {
      if (src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        inst->setSrc(insertMovBefore(i, 0, src0->getType(), bb), 0);
      }
    }

    INST_LIST_ITER end_iter = i;
    // this mul will be expanded into mul+macl in expandMulPostSchedule pass.
    // Since expanded macl must be grf-aligned, so need to make mul to be
    // grf-aligned.
    G4_DstRegRegion *dst = inst->getDst();
    if (inst->getSaturate() || dst->getExecTypeSize() > TypeSize(Type_D) ||
        isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst) ||
        !builder.tryToAlignOperand(dst, builder.getGRFSize())) {
      // add a tmp mov
      inst->setDest(
          insertMovAfter(i, dst, dst->getType(), bb, builder.getGRFAlign()));
      end_iter++;
    }

    // sat cannot be used at all in the macro sequence
    // this effectivly means sat is broken for mul D D D
    inst->setSaturate(g4::NOSAT);

    // set implicit acc dst to the mulh instruction as acc will be used as dst
    // of the expanded mul after local scheduling. it is a must to fix the
    // WAR/WAW issue of acc in local scheduling.
    G4_DstRegRegion *accDstOpnd = builder.createDst(
        builder.phyregpool.getAcc0Reg(), 0, 0, 1, inst->getDst()->getType());
    inst->setImplAccDst(accDstOpnd);

    if (execSize > builder.getNativeExecSize()) {
      auto start_iter = i;
      splitDWMULInst(start_iter, end_iter, bb);
      // start_iter points to the first half of mulh. Need double check this new
      // inserted mulh to see if need split again
      i = start_iter;
    } else {
      i++;
    }
  } else {
    G4_DstRegRegion *acc_dst_opnd =
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmp_type);

    G4_INST *newMul = builder.createBinOp(
        G4_mul, execSize, acc_dst_opnd, builder.duplicateOperand(src0),
        builder.duplicateOperand(src1), inst_opt, false);

    bb->insertBefore(i, newMul);
    inst->copyDefsTo(newMul, false);

    fixMulSrc1(std::prev(i), bb);
    newMul->setNoMask(true);

    auto machSrc1 = inst->getSrc(1);
    if (src1->isImm() && src0->getType() != src1->getType()) {
      G4_Imm *oldImm = src1->asImm();
      // Ensure src1 has the same type as src0.
      machSrc1 = builder.createImm(oldImm->getInt(), src0->getType());
    } else if (!IS_DTYPE(src1->getType())) {
      // this can happen due to vISA opt, convert them to src0 type which should
      // be D/UD We use D as the tmp type to make sure we can represent all src1
      // values
      machSrc1 = insertMovBefore(i, 1, Type_D, bb);
    }

    // We don't duplicate the operands here as original inst is unlinked
    // ToDo: this invalidate du-chain, do we still need to maintain it?
    auto machInst =
        builder.createMach(inst->getExecSize(), inst->getDst(), inst->getSrc(0),
                           machSrc1, inst_opt, tmp_type);
    machInst->setPredicate(inst->getPredicate());
    machInst->setCondMod(inst->getCondMod());
    *i = machInst;
    inst->transferUse(machInst);
    inst->removeAllDefs();
    newMul->addDefUse(machInst, Opnd_implAccSrc);

    INST_LIST_ITER end_iter = i;
    // check if the ACC source is aligned to mach dst
    // ToDo: this should be checked by fixAcc?
    G4_DstRegRegion *dst = inst->getDst();
    if (inst->getSaturate() || dst->getExecTypeSize() > TypeSize(Type_D) ||
        isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst)) {
      // add a tmp mov
      machInst->setDest(insertMovAfter(i, dst, dst->getType(), bb));
      end_iter++;
    }

    if (execSize > builder.getNativeExecSize()) {
      auto start_iter = std::prev(i);
      splitDWMULInst(start_iter, end_iter, bb);
      // start_iter ponits to the first half of mul. Need to check the new
      // inserted mul/mach instructions
      i = start_iter;
    } else {
      // i points to mach, and need to check the new inserted mul before mach
      i = std::prev(i);
    }
  }
  return;
}

//
// insert move instructions to copy numDwords dwords from src to dst at the
// specified location a NoMask UD move is used. dst and src must be
// dword-aligned. srcOffset and dstOffset are in bytes numDwords must be one of
// {1,2,4,8,16} ToDo: may want to generalize this into a copyBytes function that
// selects the appropriate move type based on dst and src type
//
void HWConformity::copyDwords(G4_Declare *dst, int dstOffset, G4_Declare *src,
                              int srcOffset, int numDwords, G4_BB *bb,
                              INST_LIST_ITER iter) {

  vISA_ASSERT(numDwords == 1 || numDwords == 2 || numDwords == 4 ||
                   numDwords == 8 || numDwords == 16 || numDwords == 32,
               "invalid number of dwords to copy");

  G4_Declare *newDst = dst;

  if (dst->getElemType() != Type_UD) {
    // create an alias with type UD
    newDst = builder.createTempVar(numDwords, Type_UD, Any);
    newDst->setAliasDeclare(dst, 0);
  }

  G4_Declare *newSrc = src;
  if (src->getElemType() != Type_UD) {
    // create an alias with type UD
    newSrc = builder.createTempVar(numDwords, Type_UD, Any);
    newSrc->setAliasDeclare(src, 0);
  }

  G4_SrcRegRegion *srcOpnd = builder.createSrc(
      newSrc->getRegVar(), srcOffset / kernel.numEltPerGRF<Type_UB>(),
      (srcOffset % kernel.numEltPerGRF<Type_UB>()) / TypeSize(Type_UD),
      builder.getRegionStride1(), Type_UD);
  G4_DstRegRegion *dstOpnd = builder.createDst(
      newDst->getRegVar(), dstOffset / kernel.numEltPerGRF<Type_UB>(),
      (dstOffset % kernel.numEltPerGRF<Type_UB>()) / TypeSize(Type_UD), 1,
      Type_UD);

  G4_INST *movInst = builder.createMov(G4_ExecSize(numDwords), dstOpnd, srcOpnd,
                                       InstOpt_WriteEnable, false);

  INST_LIST_ITER movPos = bb->insertBefore(iter, movInst);

  if (numDwords == kernel.numEltPerGRF<Type_UD>() * 2 &&
      ((dstOffset % kernel.numEltPerGRF<Type_UB>()) != 0 ||
       (srcOffset % kernel.numEltPerGRF<Type_UB>()) != 0)) {
    // move crosses 2 GRF boundary, needs splitting
    evenlySplitInst(movPos, bb);
  }
}

// like the above, but source is an indirect 64-bit source and dst offset is
// always 0 If source is Indirect 1x1, we generate
//  mov (esize*2) tmp<1>:ud r[A0]<1;1,0>:ud
//  ...     tmpSrc<region>:q
// If source is VxH indirect, we have to generate instead
//  mov (esize*2) tmp<1>:ud r[A0]<2,1>:ud
//  ...     tmpSrc<1;1,0>:q
// as we can't have the indirect region on the 64-bit type operand
// A0 is not changed otherwise
void HWConformity::copyDwordsIndirect(G4_Declare *dst, G4_SrcRegRegion *src,
                                      int numDwords, G4_BB *bb,
                                      INST_LIST_ITER iter) {
  vISA_ASSERT(TypeSize(dst->getElemType()) >= 4 && src->getTypeSize() >= 4,
               "dst and src must have dword or qword type");

  vISA_ASSERT(src->getRegAccess() == IndirGRF, "source must be indirect GRF");

  G4_Declare *newDst = dst;

  if (dst->getElemType() != Type_UD) {
    // create an alias with type UD
    newDst = builder.createTempVar(numDwords, Type_UD, Any);
    newDst->setAliasDeclare(dst, 0);
  }

  G4_SrcRegRegion *newSrc = builder.duplicateOperand(src);
  vISA_ASSERT(newSrc->getTypeSize() == 8,
               "only support 64-bit type source so far");
  newSrc->setType(builder, Type_UD);
  newSrc->setModifier(Mod_src_undef);
  if (newSrc->getRegion()->isRegionWH()) {
    vISA_ASSERT(newSrc->getRegion()->width == 1,
                 "only handle <1,0> region for now");
    newSrc->setRegion(builder, builder.createRegionDesc(UNDEFINED_SHORT, 2, 1));
  } else {
    newSrc->setRegion(builder, builder.getRegionStride1());
  }

  G4_DstRegRegion *dstOpnd =
      builder.createDst(newDst->getRegVar(), 0, 0, 1, Type_UD);

  G4_INST *movInst = builder.createMov(G4_ExecSize(numDwords), dstOpnd, newSrc,
                                       InstOpt_WriteEnable, false);

  bb->insertBefore(iter, movInst);
}

// copy numRegs GRFs from src[srcOffset] to dst[dstOffset]
// dst[dstOffset] and src[srcOffset] are both GRF-aligned
void HWConformity::copyRegs(G4_Declare *dst, int dstOffset, G4_Declare *src,
                            int srcOffset, int numRegs, G4_BB *bb,
                            INST_LIST_ITER iter) {
  int numByteCopied = 0;
  for (; numRegs >= 2;
       numRegs -= 2, numByteCopied += kernel.numEltPerGRF<Type_UB>() * 2) {
    copyDwords(dst, dstOffset + numByteCopied, src, srcOffset + numByteCopied,
               kernel.numEltPerGRF<Type_UD>() * 2, bb, iter);
  }
  if (numRegs != 0) {
    copyDwords(dst, dstOffset + numByteCopied, src, srcOffset + numByteCopied,
               kernel.numEltPerGRF<Type_UD>(), bb, iter);
  }
}

//
// Note that this function may invalidate <iter>
//
bool HWConformity::emulate64bMov(INST_LIST_ITER iter, G4_BB *bb) {
  auto inst = (*iter);
  auto origIter = iter;
  auto dst = inst->getDst();
  auto src0 = inst->getSrc(0);

  vISA_ASSERT(!inst->getCondMod(), "cant handle cond mod");
  auto dstHS = dst->getHorzStride();

  if (src0->isSrcRegRegion()) {
    auto src0RR = src0->asSrcRegRegion();
    vISA_ASSERT(src0RR->getModifier() == Mod_src_undef,
                 "cannot handle saturation");

    const RegionDesc *rgnToUse = nullptr;

    if (src0RR->getRegion()->isScalar())
      rgnToUse = builder.getRegionScalar();
    else if (!src0RR->isIndirect()) {
      uint16_t stride = 0;
      [[maybe_unused]] bool legal =
          src0RR->getRegion()->isSingleStride(inst->getExecSize(), stride);
      vISA_ASSERT(legal, "unsupported region");
      if (stride == 1)
        rgnToUse = builder.getRegionStride2();
      else if (stride == 2)
        rgnToUse = builder.getRegionStride4();
      else
        vISA_ASSERT(false, "unsupported stride");
    } else {
      if (src0RR->getTypeSize() < 8)
        rgnToUse = src0RR->getRegion();
      else {
        // this will be broken up in to 2 instructions
        auto factor = src0RR->getTypeSize() / dst->getTypeSize();
        if (src0RR->getRegion()->isContiguous(inst->getExecSize()) &&
            dst->getTypeSize() == src0RR->getTypeSize())
          factor *= 2;
        auto vs = src0RR->getRegion()->vertStride * factor;
        auto w = src0RR->getRegion()->width;
        auto hs = src0RR->getRegion()->horzStride * factor;
        rgnToUse = builder.createRegionDesc(vs, w, hs);
      }
    }

    if (dst->getTypeSize() == 8) {
      if (src0->getTypeSize() == 8) {
        // may be q->uq or uq->q or raw mov
        // safe to do raw copy for all 3 cases

        bool isNoMaskInst =
            !inst->getPredicate() &&
            (inst->isWriteEnableInst() || bb->isAllLaneActive());
        if (isNoMaskInst && inst->getExecSize() == g4::SIMD1 &&
            src0->asSrcRegRegion()->isScalar()) {
          // For SIMD1 case that is not under divergent CF, we can change to UD
          // type directly: mov (1) r10.1<1>:uq   r20.0<0;1,0>:uq
          // =>
          // mov (2) r10.2<1>:ud   r20.0<1;1,0>:ud
          G4_DstRegRegion *newDst = nullptr;
          if (dst->isIndirect()) {
            newDst = builder.createIndirectDst(
                dst->getBase(), dst->getSubRegOff(), dst->getHorzStride(),
                Type_UD, dst->getAddrImm());
          } else {
            newDst = builder.createDst(
                dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2,
                dst->getHorzStride(), Type_UD, dst->getAccRegSel());
          }

          G4_SrcRegRegion *newSrc = nullptr;
          if (!src0->isIndirect()) {
            newSrc = builder.createSrcRegRegion(
                src0RR->getModifier(), Direct, src0RR->getBase(),
                src0RR->getRegOff(), src0RR->getSubRegOff() * 2,
                builder.getRegionStride1(), Type_UD);
          } else {
            newSrc = builder.createIndirectSrc(
                src0RR->getModifier(), src0RR->getBase(), src0RR->getRegOff(),
                src0RR->getSubRegOff(), builder.getRegionStride1(), Type_UD,
                src0RR->getAddrImm());
          }

          inst->setSrc(newSrc, 0);
          inst->setDest(newDst);
          inst->setExecSize(G4_ExecSize(inst->getExecSize() * 2u));
          inst->setOptionOn(InstOpt_WriteEnable);
          inst->setMaskOption(InstOpt_M0);

          return true;
        } else {
          // mov (8) r10.0<1>:uq   r20.0<1;1,0>:uq
          // =>
          // mov (8) r10.0<2>:ud   r20.0<2;1,0>:ud
          // mov (8) r10.1<2>:ud   r20.1<2;1,0>:ud

          // 1st half
          auto newDst =
              dst->isIndirect()
                  ? (builder.createIndirectDst(dst->getBase(),
                                               dst->getSubRegOff(), 2 * dstHS,
                                               Type_UD, dst->getAddrImm()))
                  : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff() * 2, 2 * dstHS,
                                       Type_UD));
          auto newSrc = builder.createSrcRegRegion(
              Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(),
              src0RR->getRegOff(),
              src0RR->isIndirect() ? src0RR->getSubRegOff()
                                   : (src0RR->getSubRegOff() * 2),
              rgnToUse, Type_UD);
          if (newSrc->isIndirect())
            newSrc->setImmAddrOff(src0RR->getAddrImm());
          auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc,
                                           inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          // second half
          newDst = dst->isIndirect()
                       ? (builder.createIndirectDst(
                             dst->getBase(), dst->getSubRegOff(), 2 * dstHS,
                             Type_UD, 4 + dst->getAddrImm()))
                       : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                            dst->getSubRegOff() * 2 + 1,
                                            2 * dstHS, Type_UD));
          newSrc = builder.createSrcRegRegion(
              Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(),
              src0RR->getRegOff(),
              src0RR->isIndirect() ? src0RR->getSubRegOff()
                                   : (src0RR->getSubRegOff() * 2 + 1),
              rgnToUse, Type_UD);
          if (newSrc->isIndirect())
            newSrc->setImmAddrOff(4 + src0RR->getAddrImm());
          newInst = builder.createMov(inst->getExecSize(), newDst, newSrc,
                                      inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          bb->erase(origIter);

          return true;
        }
      } else if (dst->getTypeSize() == 8 && src0->getTypeSize() < 8) {
        // d/ud/w/uw/b/ub -> q/uq
        if (IS_SIGNED_INT(src0->getType())) {
          // when src is signed, sign extend
          // b/w/d -> q/uq
          //
          // dst<2>.0:d = src:[d|w|b]
          // dst<2>.1:d = asr dst<2>.0:d 31
          auto newDst =
              dst->isIndirect()
                  ? (builder.createIndirectDst(dst->getBase(),
                                               dst->getSubRegOff(), 2 * dstHS,
                                               Type_D, dst->getAddrImm()))
                  : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff() * 2, 2 * dstHS,
                                       Type_D));
          auto newSrc = builder.createSrcRegRegion(*src0RR);
          auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc,
                                           inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          newDst = dst->isIndirect()
                       ? (builder.createIndirectDst(
                             dst->getBase(), dst->getSubRegOff(), 2 * dstHS,
                             Type_D, 4 + dst->getAddrImm()))
                       : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                            dst->getSubRegOff() * 2 + 1,
                                            2 * dstHS, Type_D));
          if (dst->isIndirect()) {
            newSrc = builder.createSrcRegRegion(
                Mod_src_undef, IndirGRF, dst->getBase(), dst->getRegOff(),
                dst->getSubRegOff(), rgnToUse, Type_D);
            newSrc->setImmAddrOff(newDst->getAddrImm());
          } else
            newSrc = builder.createSrc(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff() * 2,
                                       builder.getRegionStride2(), Type_D);
          auto imm31 = builder.createImm(31, Type_W);
          newInst =
              builder.createBinOp(G4_asr, inst->getExecSize(), newDst, newSrc,
                                  imm31, inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          bb->erase(origIter);

          return true;
        } else {
          // when src is unsigned, zero extend
          // ub/uw/ud -> q/uq
          //
          // dst<2>.0:ud = src:[ud|uw|ub]
          // dst<2>.1:ud = 0

          auto newDst =
              dst->isIndirect()
                  ? (builder.createIndirectDst(dst->getBase(),
                                               dst->getSubRegOff(), 2 * dstHS,
                                               Type_UD, dst->getAddrImm()))
                  : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff() * 2, 2 * dstHS,
                                       Type_UD));
          auto newSrc = builder.createSrcRegRegion(*src0RR);
          auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc,
                                           inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          newDst = dst->isIndirect()
                       ? (builder.createIndirectDst(
                             dst->getBase(), dst->getSubRegOff(), 2 * dstHS,
                             Type_UD, 4 + dst->getAddrImm()))
                       : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                            dst->getSubRegOff() * 2 + 1,
                                            2 * dstHS, Type_UD));
          auto imm0 = builder.createImm(0);
          newInst = builder.createMov(inst->getExecSize(), newDst, imm0,
                                      inst->getOption(), false);
          newInst->setPredicate(
              inst->getPredicate()
                  ? builder.createPredicate(*inst->getPredicate())
                  : nullptr);
          iter = bb->insertBefore(origIter, newInst);

          bb->erase(origIter);

          return true;
        }
      }
    } else if (dst->getTypeSize() < 8 && src0->getTypeSize() == 8) {
      // truncate
      // q/uq -> d/ud/w/uw/b/ub
      // 1. mov(8) r10.0<1>:d   r20.0<1;1,0>:uq
      // =>
      // mov(8) r10.0<1>:d   r20.0<2;1,0>:d
      //
      // 2. mov(8) r10.0<1>:d   r20.1<2;1,0>:uq
      // =>
      // mov(8) r10.0<1>:d   r20.2<4;1,0>:d

      unsigned int factor = src0->getTypeSize() / dst->getTypeSize();
      auto newDst = builder.createDstRegRegion(*dst);
      auto newSrc = builder.createSrcRegRegion(
          Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(),
          src0RR->getRegOff(),
          src0RR->isIndirect() ? src0RR->getSubRegOff()
                               : (src0RR->getSubRegOff() * factor),
          rgnToUse, dst->getType());
      newSrc->setImmAddrOff(src0RR->getAddrImm());
      auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc,
                                       inst->getOption(), false);
      newInst->setPredicate(inst->getPredicate()
                                ? builder.createPredicate(*inst->getPredicate())
                                : nullptr);
      iter = bb->insertBefore(origIter, newInst);

      bb->erase(origIter);

      return true;
    }
  } else if (src0->isImm()) {
    auto imm = src0->asImm()->getInt();
    int low = imm & 0xffffffff;
    int high = (imm >> 32) & 0xffffffff;

    // low
    auto newDst =
        dst->isIndirect()
            ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(),
                                         2 * dstHS, Type_D, dst->getAddrImm()))
            : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                 dst->getSubRegOff() * 2, 2 * dstHS, Type_D));
    auto immLowSrc = builder.createImm(low, Type_D);
    auto newInst = builder.createMov(inst->getExecSize(), newDst, immLowSrc,
                                     inst->getOption(), false);
    newInst->setPredicate(inst->getPredicate()
                              ? builder.createPredicate(*inst->getPredicate())
                              : nullptr);
    iter = bb->insertBefore(origIter, newInst);

    // high
    newDst = dst->isIndirect()
                 ? (builder.createIndirectDst(
                       dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_D,
                       4 + dst->getAddrImm()))
                 : (builder.createDst(dst->getBase(), dst->getRegOff(),
                                      dst->getSubRegOff() * 2 + 1, 2 * dstHS,
                                      Type_D));
    auto immHigh = builder.createImm(high, Type_D);
    newInst = builder.createMov(inst->getExecSize(), newDst, immHigh,
                                inst->getOption(), false);
    newInst->setPredicate(inst->getPredicate()
                              ? builder.createPredicate(*inst->getPredicate())
                              : nullptr);
    iter = bb->insertBefore(origIter, newInst);

    bb->erase(origIter);

    return true;
  }

  return false;
}

bool HWConformity::fix64bInst(INST_LIST_ITER iter, G4_BB *bb) {

  // HW restrictions:
  // [DevCHV, DevBXT]: When source or destination datatype is 64b, indirect
  // addressing must not be used. the region rules are: Source and Destination
  // horizontal stride must be aligned to the execution datatype. Example: mov
  // (4) r10.0:df r11.0<16;8,2>:f // Source stride must be 2 since datatype is
  // smaller move (4) r10.0<2>:f r11.0<4;4,1>:df // Destination stride must be 2
  // since datatype is smaller. as this would require splitting in some cases
  // Regioning must ensure Src.Vstride = Src.Width * Src.Hstride
  // Source and Destination offset must be the same, except the case of scalar
  // source [DevCHV, DevBXT]: When source or destination datatype is 64b,
  // indirect addressing must not be used. [DevCHV, DevBXT]: ARF registers must
  // never be used with 64b datatype.

  if (!builder.no64bitRegioning()) {
    return false;
  }

  G4_INST *inst = *iter;
  bool uses64BitType = false;
  bool isDWMultiply = false;
  uint8_t execSize = inst->getExecSize();

  if (inst->isSend() || inst->isDpas()) {
    return false;
  }
  if (inst->getDst() && inst->getDst()->getTypeSize() == 8) {
    uses64BitType = true;
  }
  for (int i = 0, size = inst->getNumSrc(); !uses64BitType && i < size; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src && src->getTypeSize() == 8) {
      uses64BitType = true;
    }
  }
  if (inst->opcode() == G4_mul && IS_DTYPE(inst->getSrc(0)->getType()) &&
      IS_DTYPE(inst->getSrc(1)->getType())) {
    // WA: dw*dw multiply is considered to use 64bit data type since the result
    // is 64-bit
    uses64BitType = true;
    isDWMultiply = true;
  }

  if (uses64BitType) {
    if (builder.noInt64()) {
      // handle i64 mov/add/cmp/sel
      // ToDo: move it to its own pass
      if (inst->opcode() == G4_mov && IS_INT(inst->getDst()->getType()) &&
          IS_INT(inst->getSrc(0)->getType())) {
        if (emulate64bMov(iter, bb))
          return true;
      }
    }

    int numSrc = inst->getNumSrc();

    // handle indirect sources first
    for (int i = 0; i < numSrc; ++i) {
      G4_Operand *src = inst->getSrc(i);
      if (src != nullptr && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        G4_SrcRegRegion *srcAsRegion = src->asSrcRegRegion();
        const RegionDesc *region = srcAsRegion->getRegion();
        int byteSize = srcAsRegion->getTypeSize();
        if (byteSize == 8) {
          // right bound is not available for indirect operands
          // FIXME: this code should be moved to getRightBound()
          int rightBound = 0;
          // we must change move type to UD

          if (region->isScalar()) {
            rightBound = byteSize;
          } else if (region->isRegionWH()) {
            rightBound = inst->getExecSize() * byteSize;
          } else {
            int num_rows = inst->getExecSize() / region->width;
            rightBound = (num_rows - 1) * region->vertStride * byteSize +
                         region->horzStride * (region->width - 1) * byteSize +
                         byteSize;
          }

          int numDwords = rightBound / TypeSize(Type_UD);
          numDwords = Round_Up_Pow2(numDwords);
          G4_Declare *tmpSrc = builder.createTempVar(
              numDwords / 2, src->getType(), builder.getGRFAlign());
          // new source's region varies depending on whether it's VxH or 1x1
          const RegionDesc *newRegion =
              region->isRegionWH() ? builder.getRegionStride1() : region;
          copyDwordsIndirect(tmpSrc, srcAsRegion, numDwords, bb, iter);
          G4_SrcRegRegion *tmpSrcOpnd = builder.createSrcRegRegion(
              srcAsRegion->getModifier(), Direct, tmpSrc->getRegVar(), 0, 0,
              newRegion, tmpSrc->getElemType());
          inst->setSrc(tmpSrcOpnd, i);
        } else {
          // use the good ol' insertMovBefore
          G4_Operand *tmpSrc = insertMovBefore(iter, i, src->getType(), bb);
          G4_Declare *tmpSrcDcl = tmpSrc->getTopDcl();
          tmpSrcDcl->setSubRegAlign(builder.getGRFAlign());
          inst->setSrc(tmpSrc, i);
        }
      }
    }

    // now handle direct sources with bad region/alignment
    bool hasSameOffset = hasSameSubregOffset(inst);
    for (int i = 0; i < numSrc; i++) {
      G4_Operand *src = inst->getSrc(i);
      if (src != NULL && src->isSrcRegRegion()) {
        G4_SrcRegRegion *srcAsRegion = src->asSrcRegRegion();
        const RegionDesc *region = srcAsRegion->getRegion();
        int byteSize = srcAsRegion->getTypeSize();

        if (!isDWMultiply && !region->isScalar() &&
            (byteSize != 8 && (byteSize * region->horzStride) < 8)) {
          // source is not 8 byte aligned
          // this can happen e.g. for
          // mov (8) r1.0<1>:df (mod)r3<8;8,1>:f
          // which we'd need to change to
          // mov (8) r10.0<2>:f (mod)r3.0<8;8,1>:f
          // mov (8) r1.0<1>:df r10.0<8;4,2>:f
          // to satisfy rule 1
          uint8_t exSize = inst->getExecSize();
          uint16_t multFactor = (uint16_t)(8 / byteSize);
          G4_Type tmpType = srcAsRegion->getType();
          if (multFactor == 8) {
            // byte type needs special handling since we can't have stride 8
            tmpType = (tmpType == Type_B) ? Type_W : Type_UW;
            multFactor = 4;
          }
          vISA_ASSERT(multFactor != 8,
                       "does not support 64b operation with byte source");
          G4_Declare *tmp = builder.createTempVar(exSize * multFactor, tmpType,
                                                  builder.getGRFAlign());
          G4_DstRegRegion *tmpDst = builder.createDstRegRegion(tmp, multFactor);
          G4_INST *movInst = builder.createMov(inst->getExecSize(), tmpDst, src,
                                               inst->getOption(), false);
          bb->insertBefore(iter, movInst);
          uint16_t width = exSize;
          if (width * 8u > kernel.numEltPerGRF<Type_UB>()) {
            // can't have width cross GRF
            width = 4;
          }
          G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(
              tmp, builder.createRegionDesc((uint16_t)multFactor * width, width,
                                            multFactor));
          inst->setSrc(newSrc, i);
        } else if (region->isScalar()) {
#if 0
                    // scalar region still must be aligned to qword, though it can be any qword
                    if (byteSize < 8 && !builder.tryToAlignOperand(srcAsRegion, 8))
                    {
                        G4_Operand* tmpSrc = insertCopyBefore(iter, i, Four_Word, bb);
                        inst->setSrc(tmpSrc, i);
                    }
#endif
        } else if (!hasSameOffset) {
          // we need a temp src that is GRF-aligned
          if (byteSize == 8) {
            // the same src/dst offset restriction applies to move as well, so
            // we have to generate a packed move with UD type to work around the
            // restriction e.g., for add (2) ... r1.1<4;2,2>:q we turn it into
            // mov (8) r10.0<1>:ud r1.2<1;1,0>:ud {NoMask}
            // add (2) ... r10.0<4;2,2>:q
            int numDwords = (src->getRightBound() - src->getLeftBound() + 1) /
                            TypeSize(Type_UD);
            numDwords = Round_Up_Pow2(numDwords);
            G4_Declare *tmpSrc = builder.createTempVar(
                numDwords / 2, src->getType(), builder.getGRFAlign());
            copyDwords(tmpSrc, 0, src->getTopDcl(), src->getLeftBound(),
                       numDwords, bb, iter);
            G4_SrcRegRegion *tmpSrcOpnd = builder.createSrcRegRegion(
                srcAsRegion->getModifier(), Direct, tmpSrc->getRegVar(), 0, 0,
                srcAsRegion->getRegion(), tmpSrc->getElemType());
            inst->setSrc(tmpSrcOpnd, i);
          } else {
            // use the good ol' insertMovBefore
            G4_Operand *tmpSrc = insertMovBefore(iter, i, src->getType(), bb);
            G4_Declare *tmpSrcDcl = tmpSrc->getTopDcl();
            tmpSrcDcl->setSubRegAlign(builder.getGRFAlign());
            inst->setSrc(tmpSrc, i);
          }
        }
      }
    }

    for (int i = 0; i < numSrc; i++) {
      // rewrite <1;1,0> to <2;2,1> since HW does not like the former
      G4_Operand *src = inst->getSrc(i);
      if (src != nullptr && src->isSrcRegRegion()) {
        G4_SrcRegRegion *srcAsRegion = src->asSrcRegRegion();
        const RegionDesc *region = srcAsRegion->getRegion();
        if (!region->isRegionWH() &&
            region->vertStride != region->horzStride * region->width) {
          // see if we can fix the region to satisfy VS = W * HS
          if (region->width == inst->getExecSize()) {
            // vs is a don't care, change to <w*hs, w, hz>
            srcAsRegion->setRegion(
                builder,
                builder.createRegionDesc(region->width * region->horzStride,
                                         region->width, region->horzStride));
          } else if (region->width == 1) {
            // hs is a don't care, change it to <esize*vs, esize, vs>
            vISA_ASSERT(region->vertStride <= 4, "illegal vertical stride");

            uint16_t wd = inst->getExecSize();
            uint16_t hs = region->vertStride;
            if (src->crossGRF(builder)) {
              // Make sure the new hs does not cross GRF
              uint32_t nbytesIn1stGRF =
                  kernel.numEltPerGRF<Type_UB>() -
                  (src->getLeftBound() % kernel.numEltPerGRF<Type_UB>());
              uint32_t eltBytes = srcAsRegion->getTypeSize();
              uint32_t neltsIn1stGRF = nbytesIn1stGRF / eltBytes;

              vISA_ASSERT((nbytesIn1stGRF % eltBytes) == 0,
                           "Bad region with element crossing GRF");
              vISA_ASSERT((neltsIn1stGRF % hs) == 0, "hs cannot cross GRF");

              wd = neltsIn1stGRF / hs;
              // Get the largest powOfTwo that can divide wd
              wd = wd & (-wd);
              // vISA_ASSERT(wd > 1, "Cannot select non-1 width w/o crossing
              // GRF");
            }
            srcAsRegion->setRegion(builder,
                                   builder.createRegionDesc(wd * hs, wd, hs));
          }

          else {
            // FIXME: Both VS and HS are used by the region, so we have to
            // either split inst or insert multiple moves to pack the source
            // both are painful, so we assert for now and fix later if we
            // encounter such a case
            vISA_ASSERT(false, "Unhandled bad 64b region on CHV/BXT");
          }
        }
      }
    }
    G4_DstRegRegion *dst = inst->getDst();
    if (dst != NULL && !dst->isNullReg()) {
      bool needsTmpDst = dst->getRegAccess() != Direct ||
                         (execSize > 1 && !hasSameOffset) || dst->isAreg();
      if (needsTmpDst) {
        // we need to have a temp dst that is direct and GRF-aligned
        if (dst->getRegAccess() == Direct && dst->getTypeSize() == 8) {
          // the same src/dst offset restriction applies to move as well, so we
          // have to generate a move with UD type to work around the restriction
          // e.g., for
          // add (2) r1.2<1>:q ...
          // we generate
          // add (2) r3.0<1>:q ...
          // mov (4) r1.4<1>:ud r3.0<1;1,0>:ud {NoMask}
          // If dst is not contiguous, we additionally add a move to pre-load
          // the old values: add (2) r1.2<2>:q ... becomes mov (8) r3.0<1>:ud
          // r1.4<1;1,0>:ud {NoMask} add (2) r3.0<2>:q ... mov (8) r1.4<1>:ud
          // r3.0<1;1,0>:ud {NoMask}
          int numDwords = (dst->getRightBound() - dst->getLeftBound() + 1) /
                          TypeSize(Type_UD);
          numDwords = Round_Up_Pow2(numDwords);
          G4_Declare *tmpDst = builder.createTempVar(
              numDwords / 2, dst->getType(), builder.getGRFAlign());
          if (numDwords > execSize * 2) {
            // dst is not packed, need a move to pre-load the dst value into tmp
            copyDwords(tmpDst, 0, dst->getTopDcl(), dst->getLeftBound(),
                       numDwords, bb, iter);
          }
          INST_LIST_ITER next = iter;
          ++next;
          copyDwords(dst->getTopDcl(), dst->getLeftBound(), tmpDst, 0,
                     numDwords, bb, next);
          inst->setDest(
              builder.createDstRegRegion(tmpDst, dst->getHorzStride()));
        } else {
          // use the good ol' insertMoveAfter
          G4_DstRegRegion *tmpDst =
              insertMovAfter(iter, dst, dst->getType(), bb);
          G4_Declare *tmpDstDcl = tmpDst->getTopDcl();
          tmpDstDcl->setSubRegAlign(builder.getGRFAlign());
          inst->setDest(tmpDst);
          if (dst->getTypeSize() == 8) {
            // tmpDst is indirect and thus still does not conform
            // we rewrite
            // mov (e) r[a0.0]<1>:q src<1;1,0>:q
            // into
            // mov (e*2) r[a0.0]<1>:ud src<1;1,0>:ud {NoMask}
            ++iter;
            G4_INST *movInst = *iter;
            vISA_ASSERT(movInst->opcode() == G4_mov &&
                             movInst->getDst() == dst &&
                             movInst->getSrc(0)->isSrcRegRegion(),
                         "unexpected instruction created by insertMovAfter");
            vISA_ASSERT(dst->getHorzStride() == 1,
                         "only stride 1 is supported for now");
            dst->setType(builder, Type_UD);
            G4_SrcRegRegion *src = movInst->getSrc(0)->asSrcRegRegion();
            G4_Declare *tmpAsUD = builder.createTempVar(
                tmpDstDcl->getNumElems() * 2, Type_UD, Any);
            tmpAsUD->setAliasDeclare(tmpDstDcl, 0);
            const RegionDesc *newRegion =
                src->getRegion()->isScalar() ? builder.createRegionDesc(0, 2, 1)
                                             : builder.getRegionStride1();
            G4_SrcRegRegion *srcAsUD = builder.createSrcRegRegion(
                src->getModifier(), src->getRegAccess(), tmpAsUD->getRegVar(),
                src->getRegOff(), src->getSubRegOff() * 2, newRegion,
                tmpAsUD->getElemType());
            movInst->setSrc(srcAsUD, 0);
            movInst->setExecSize(G4_ExecSize(inst->getExecSize() * 2u));

            // NoMask is set on the mov instruction, but if we fall outside of
            // the new execution size, it won't be executed fully e.g., we have
            // to change (W) mov (16|M24) r[a0.0,64]<1>:ud r67.0<8;8,1>:ud into
            // (W) mov (16|M0) r[a0.0,64]<1>:ud r67.0<8;8,1>:ud
            movInst->setMaskOption(InstOpt_M0);

            // mov saturate/pred to the original inst
            movInst->setOptionOn(InstOpt_WriteEnable);
            if (movInst->getSaturate()) {
              movInst->setSaturate(g4::NOSAT);
              inst->setSaturate(g4::SAT);
            }
            G4_Predicate *pred = movInst->getPredicate();
            if (pred) {
              vISA_ASSERT(inst->getPredicate() == nullptr,
                           "both inst and movInst have predicates");
              movInst->setPredicate(nullptr);
              inst->setPredicate(pred);
            }
          }
        }
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
//
//  For BDW, 32 bits integer multiply is implemented as the following macro
//
//  mul (8) acc0:d     r2.0<8;8,1>d   r3.0<16;8,2>:uw
//  mach (8) rTemp<1>:d r2.0<8;8,1>d   r3.0<8;8,1>:d
//  mov (8) r5.0<1>:d   rTemp:d // hi-32bits
//  mov (8) r6.0<1>:d acc0:d // lo-32bits
//
//  Note that this only changes the mul instruction's src1, mach and mov is
//  generated elsewhere
//------------------------------------------------------------------------------
void HWConformity::fixMulSrc1(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  G4_Operand *src1 = inst->getSrc(1);

  if (!IS_DTYPE(src1->getType())) {
    // this could happen if dst is Q
    return;
  }

  if (src1->isImm()) {
    uint64_t truncVal = src1->asImm()->getImm() & 0xFFFF;
    G4_Imm *new_src1 = builder.createImm(truncVal, Type_UW);
    inst->setSrc(new_src1, 1);
  } else {
    vISA_ASSERT(src1->isSrcRegRegion(), "region expected");
    G4_SrcRegRegion *srcRegion = src1->asSrcRegRegion();
    const RegionDesc *rd = srcRegion->getRegion();

    // create a new opnd with type UW
    unsigned short scale = TypeSize(Type_D) / TypeSize(Type_UW);
    unsigned short newHS = rd->horzStride * scale;
    unsigned short newVS = rd->vertStride * scale;
    const RegionDesc *new_rd =
        builder.createRegionDesc(newVS, rd->width, newHS);
    short subRegOff = srcRegion->getSubRegOff();
    if (srcRegion->getRegAccess() == Direct) {
      subRegOff *= scale;
    }
    auto new_src1 = builder.createSrcRegRegion(
        srcRegion->getModifier(), srcRegion->getRegAccess(),
        srcRegion->getBase(), srcRegion->getRegOff(), subRegOff, new_rd,
        Type_UW);
    inst->setSrc(new_src1, 1);
    if (srcRegion->getRegAccess() != Direct) {
      new_src1->setImmAddrOff(srcRegion->getAddrImm());
    }
  }
}

/*
 *  only acc0 may be used in DWord operations, so we have to break a
 *  SIMD16 DWord multiply into two mul-mach-mov sequences.
 *
 *  Input:
 *  (f0) mul (16) dst:d  src0:d  src1:d
 *
 *  Output:
 *  mul (8) acc0:d  src0:d  src1:d
 *  mach    (8) null:d  src0:d  src1:d
 *  (f0) mov (8) dst:d acc0:d
 *  mul (8) acc0:d  src0+1:d  src1+1:d
 *  mach    (8) null:d  src0+1:d    src1+1:d
 *  (f1) mov (8) dst+1:d acc0:d
 *
 */
void HWConformity::splitDWMULInst(INST_LIST_ITER &start, INST_LIST_ITER &end,
                                  G4_BB *bb) {
  // split simd16 inst into SIMD8 ones, since D is not supported for acc1
  INST_LIST_ITER iter = start, last_iter = end;
  // iter--;
  last_iter++;
  INST_LIST_ITER curr_iter;
  while (iter != end) {
    curr_iter = iter;
    evenlySplitInst(curr_iter, bb);
    // curr_iter points to the second half after instruction splitting
    G4_INST *expand_sec_half_op = *curr_iter;
    iter++;

    bb->insertBefore(last_iter, expand_sec_half_op);
    if (curr_iter == start) {
      start--;
    }
    bb->erase(curr_iter);
  }
  // handle the last inst
  if (iter == end) {
    evenlySplitInst(iter, bb);
    G4_INST *expand_sec_half_op = *iter;
    bb->insertBefore(last_iter, expand_sec_half_op);
    // For the case that only one instruction needed to split, that is to say
    // start equals to end
    if (start == end) {
      start--;
    }
    end--;
    bb->erase(iter);
  }
}

static bool isGoodMadType(G4_Type type) {
  switch (type) {
  case Type_F:
  case Type_HF:
  case Type_DF:
  case Type_BF:
    return true;
  default:
    return false;
  }
}

bool HWConformity::isGoodAlign1TernaryDst(G4_INST *inst) const {
  // Align1 MAD requirements:
  // -- dst must be direct GRF/acc with horizontal stride 1 or 2
  G4_Type execType = inst->getExecType();
  G4_DstRegRegion *dst = inst->getDst();

  // XE3P supports Q dst types for mad instruction
  if (builder.hasWideMulMadOpsEnabled() && inst->opcode() == G4_pseudo_mad) {
    vISA_ASSERT(!IS_BTYPE(dst->getType()),
                 "3Src inst don't support B dst types");
  } else {
    vISA_ASSERT(!IS_QTYPE(dst->getType()) && !IS_BTYPE(dst->getType()),
                 "3Src inst don't support Q and B dst types");
  }

  if (!builder.hasMixMode() && isLowPrecisionFloatTy(dst->getType()) &&
      !isLowPrecisionFloatTy(execType)) {
    return false;
  }

  auto dstTySize = dst->getTypeSize();

  int alignInBytes = std::max((int)dstTySize, builder.get3SrcDstAlign());

  if (builder.noSrc2Regioning()) {
    // src2 is required to have the same subreg as dst if src2 is not a scalar
    // If we can't guarantee this we have to align both of them to GRF
    unsigned src2Pos = inst->opcode() == G4_pseudo_mad ? 0 : 2;
    auto src2 = inst->getSrc(src2Pos);
    if (src2->isSrcRegRegion() && !src2->asSrcRegRegion()->isScalar()) {
      alignInBytes = builder.getGRFSize();
    }
  }

  if (!builder.tryToAlignOperand(dst, alignInBytes)) {
    // dst may have special alignment due to encoding issues
    return false;
  }

  uint32_t effectiveStride = dst->getHorzStride();
  if (dstTySize < TypeSize(execType)) {
    if (IS_TYPE_INT(dst->getType())) {
      effectiveStride *= TypeSize(execType) / dstTySize;
    } else {
      // we have mixed HF and F inst
      // dst can be packed HF, but then it must be oword aligned
      // this should be checked later for mixed mode inst
    }
  }

  return dst->getRegAccess() == Direct && effectiveStride <= 2;
}

//
// check for legal align1 ternary inst sources
//
bool HWConformity::isGoodAlign1TernarySrc(G4_INST *inst, int srcPos,
                                          bool canBeImm) {
  vISA_ASSERT(srcPos >= 0 && srcPos < 3, "illegal source pos");

  uint8_t execSize = inst->getExecSize();
  G4_Operand *src = inst->getSrc(srcPos);
  // for pseudo_mad we have to swap src0 and src2
  // isSrc2 variable should be interpreted as isSrc2 in GEN
  bool isSrc2 = inst->opcode() == G4_pseudo_mad ? srcPos == 0 : srcPos == 2;

  if (!builder.hasMixMode()) {
    G4_Type execType = inst->getExecType();
    if (isLowPrecisionFloatTy(src->getType()) &&
        !isLowPrecisionFloatTy(execType)) {
      return false;
    }
  }

  if (IS_QTYPE(src->getType())) {
    // XE3P accepts src2 in vISA (src0 in gen) to be qword for mad instruction
    if (builder.hasWideMulMadOpsEnabled() && inst->opcode() == G4_pseudo_mad) {
      // for any other source operand positions, do not accept qword operands
      if (srcPos != 2) {
        return false;
      }
    } else {
      return false;
    }
  }

  // mad specific checks
  if (inst->opcode() == G4_pseudo_mad) {
    if (isSrc2) {
      if (IS_DTYPE(src->getType())
          // no need of check for dtype src
          // XE3P mad operation accepts src0 (src2 in gen) to be dtype
          && !(builder.hasWideMulMadOpsEnabled() &&
               IS_QTYPE(inst->getDst()->getType()))
          && !(builder.getOption(vISA_EnableInt32DstMulMad) ||
               builder.hasSimplifiedRegions())
      ) {
        return false;
      }
      if (builder.noSrc2Regioning() && IS_BTYPE(src->getType())) {
        return false;
      }
    } else if (srcPos == 1) {
      if (IS_DTYPE(src->getType()) && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getModifier() != Mod_src_undef) {
        // no source modifier for DW multiply
        return false;
      }
    }
  }

  if (builder.hasWideMulMadOpsEnabled() && inst->opcode() == G4_pseudo_mad &&
      IS_QTYPE(inst->getDst()->getType())) {
    if (isSrc2 || srcPos == 1) // src1, src2: d/ud
    {
      if (!IS_DTYPE(src->getType())) {
        return false;
      }
    } else // src0: q/uq or d/ud
    {
      if (!IS_QTYPE(src->getType()) && !IS_DTYPE(src->getType())) {
        return false;
      }
    }
  }
  if (src->isImm()) {
    // either src0 or src2 can be 16b imm, but not both
    // permanent WA: simd16 inst can't have src0 imm.
    // Instead of splitting, we just add a move

    bool isSigned = IS_SIGNED_INT(src->getType());
    bool isLETwoBytes =
        (src->getTypeSize() <= 2 ||
         G4_Imm::isInTypeRange(src->asImm()->getImm(),
                               isSigned ? G4_Type::Type_W : G4_Type::Type_UW));
    if (canBeImm && (srcPos == 0 || srcPos == 2) && isLETwoBytes) {
      bool canInlineImm = true;
      if (VISA_WA_CHECK(builder.getPWaTable(), WaNoSimd16TernarySrc0Imm)) {
        canInlineImm = !isSrc2 && inst->getExecSize() != g4::SIMD16;
      }
      if (canInlineImm && src->getTypeSize() > 2) {
        // replace imm to 2-byte type
        auto *shoterImm = builder.createImmWithLowerType(src->asImm()->getImm(),
                                                        src->getType());
        inst->setSrc(shoterImm, srcPos);
      }
      return canInlineImm;
    }
    return false;
  } else if (src->isSrcRegRegion()) {
    if (src->asSrcRegRegion()->getRegAccess() != Direct) {
      return false;
    }

    auto checkSingleStrideRegion = [](G4_SrcRegRegion *src, int stride,
                                      uint8_t execSize, IR_Builder &builder) {
      const RegionDesc *srcRegion = src->getRegion();

      if (stride > 4) {
        return false;
      } else if (srcRegion->isContiguous(execSize)) {
        // Normalize the region if it is not.
        if (srcRegion->width != 1) {
          src->setRegion(builder, builder.getRegionStride1(),
                         /*invariant*/ true);
        }
        if (!builder.encodeUnitStrideTernary()) {
          // we have to make sure width is not being used to cross GRF, as
          // <1;1,0> is not a legal region for align1 ternary source (vs 1 not
          // supported) mad doesn't support <1;1,0>, the width is at least 2
          int minAlignment = src->getTypeSize() * 2;
          return builder.tryToAlignOperand(src, minAlignment);
        }
      } else if (src->asSrcRegRegion()->getRegAccess() == Direct &&
                 src->crossGRF(builder)) {
        // Make sure only VertStride is used to cross GRF register boundaries
        int width = 2;
        int vStride = stride * 2;
        int elementSize = src->getTypeSize();
        int startOffset = src->getLeftBound() % builder.kernel.numEltPerGRF<Type_UB>();
        for (int row = 0; row < execSize / width; row++) {
          int rowOffset = (startOffset + row * vStride * elementSize) %
                          builder.numEltPerGRF<Type_UB>();
          if (rowOffset + (width - 1) * stride * elementSize >=
              (int)builder.kernel.numEltPerGRF<Type_UB>()) {
            return false;
          }
        }
      }
      return true;
    };

    // the following regions are supported:
    // <N;N,0>
    // <0;1,0>
    // <W*H;W,H>
    const RegionDesc *srcRegion = src->asSrcRegRegion()->getRegion();
    if (srcRegion->isScalar()) {
      return true;
    }

    // src0 and src1 (for psuedo-mad, it's src1 and src2) may use the <N;N,0>
    // region as they come with a vStride in encoding
    // TODO: we may consider swapping src1 and src2 to catch more regions
    if (!isSrc2) {
      uint16_t stride = 0;
      if (srcRegion->isSingleStride(execSize, stride)) {
        return checkSingleStrideRegion(src->asSrcRegRegion(), stride, execSize,
                                       builder);
      }

      if (builder.encodeUnitStrideTernary()) {
        // <4;4,0> and <8;8,0> are ok
        return srcRegion->vertStride == srcRegion->width &&
               srcRegion->horzStride == 0 &&
               (srcRegion->width == 4 || srcRegion->width == 8);
      } else {
        // <2;2,0>, <4;4,0> and <8;8,0> are ok
        return srcRegion->vertStride == srcRegion->width &&
               srcRegion->horzStride == 0 && srcRegion->width <= 8;
      }
    } else {
      if (!builder.noSrc2Regioning()) {
        // src2 (src0 for pseudo-mad) is without vstride, and its region must be
        // <esize*H;esize,H>, with vstride derived from exSize and hstride
        uint16_t stride = 0;
        if (srcRegion->isSingleStride(execSize, stride)) {
          return checkSingleStrideRegion(src->asSrcRegRegion(), stride,
                                         execSize, builder);
        }
      } else {
        if (src->isSrcRegRegion() && inst->getDst()->isDstRegRegion()) {
          auto dstRgn = inst->getDst()->asDstRegRegion();
          auto dstTy = dstRgn->getType();
          auto dstStride = dstRgn->getHorzStride();
          auto dstOff = dstRgn->getSubRegOff();
          auto srcTy = src->getType();
          auto srcRgn = src->asSrcRegRegion();
          auto srcOff = srcRgn->getSubRegOff();

          if ((builder.hasSimplifiedRegions() ||
               builder.getOption(vISA_RelaxQWRegion)) &&
              dstStride * TypeSize(dstTy) == 8 && !IS_TYPE_FLOAT_ALL(dstTy) &&
              ((inst->opcode() == G4_mul && (srcPos == 0 || srcPos == 1)) ||
               (inst->opcode() != G4_mul && (srcPos == 1 || srcPos == 2)))) {
            // Regioning for instructions with qword aligned destination
            // Mul instructions apply the following on src0 and src1
            // Non-mul instructions apply the following on src1 and src2
            // - dwords on source must be strided to all align to either
            //   dword0 or dword1 of the qword aligned destination.
            // - words on source must be strided to all align to either
            //   word0 or word2 of the qword aligned destination.
            if (dstOff == 0) {
              if ((TypeSize(srcTy) == 4 && (srcOff == 0 || srcOff == 1)) ||
                  (TypeSize(srcTy) == 2 && (srcOff == 0 || srcOff == 2))) {
                return true;
              }
            }
          }
        }
        // not a scalar, src2 must be GRF aligned.
        if (!builder.tryToAlignOperand(src, kernel.numEltPerGRF<Type_UB>())
            && (!(inst->opcode() == G4_pseudo_mad &&
                  (src->getType() == G4_Type::Type_D ||
                   src->getType() == G4_Type::Type_UD) &&
                  IS_QTYPE(inst->getDst()->getType()) &&
                  builder.hasWideMulMadOpsEnabled()))) {
          return false;
        }

        uint16_t stride = 0;
        if (srcRegion->isSingleStride(execSize, stride)) {
          unsigned short dstExecSize = inst->getDst()->getExecTypeSize();
          unsigned short srcExecSize =
              stride * src->asSrcRegRegion()->getElemSize();
          // Source 2 and destination stride must be aligned to the same
          // execution type. E.g. mad (4) r10.0<1>:hf src0 src1 r13.0<1>:hf
          //      mad (4) r10.0<2>:hf src0 src1 r13.0<1>:f
          //      mad (4) r10.0<1>:f  src0 src1 r13.0<2>:hf
          // this rule is relaxed if mix mode is enabled (packed HF ok)
          if (dstExecSize == srcExecSize) {
            return true;
          }
          if (builder.hasPartialMixMode() && inst->isMixedMode()) {
            return true;
          }
        }
      }

      return false;
    }
  }

  return true;
}

//
// a source is good for align16 if:
// -- it is a direct srcRegRegion
// -- it has contiguous region and can be made either GRF-aligned (for exec size
// >= 8)
//    or oword aligned (for exec size == 4)
// -- or it has scalar region and is not non-simd1 double
bool HWConformity::isGoodAlign16Src(G4_INST *inst, int srcPos) {
  vISA_ASSERT(srcPos >= 0 && srcPos < 3, "illegal source pos");

  uint8_t execSize = inst->getExecSize();
  G4_Operand *src = inst->getSrc(srcPos);
  G4_Type opnd_type = src->getType();

  // Constants are not allowed as MAD opnds.
  if (src->isSrcRegRegion()) {
    const RegionDesc *region = src->asSrcRegRegion()->getRegion();
    G4_RegAccess regAcc = src->asSrcRegRegion()->getRegAccess();

    if (regAcc != Direct) {
      return false;
    }

    if (region->isContiguous(execSize)) {
      if (builder.getPlatform() == GENX_BDW && TypeSize(opnd_type) < 4) {
        // BDW HF has to be 32-byte aligned
        if (!builder.tryToAlignOperand(src, 32)) {
          return false;
        }
      } else {
        if (execSize >= 8) {
          // operand must be GRF aligned, or oword aligned for HF/W
          uint32_t align =
              std::min<uint32_t>(execSize * src->getTypeSize(), 32);
          if (!builder.tryToAlignOperand(src, align)) {
            return false;
          }
        } else if (execSize == 4 || execSize == 2) {
          // operand must be oword-aligned
          if (!builder.tryToAlignOperand(src, 16)) {
            return false;
          }
        }
      }
    } else if (src->asSrcRegRegion()->isScalar()) {
      if (opnd_type == Type_DF && execSize != 1) {
        // scalar region is illegal for DF since replicate is not supported
        return false;
      }

      if (opnd_type == Type_HF && builder.getPlatform() == GENX_BDW) {
        return false;
      }
    } else {
      // all other regions are illegal
      return false;
    }

    return true;
  } else {
    return false;
  }
}

//
// Move modifiers of src2 in pseudo_mad to its defining instruction.
//
// mul (16) V66(0,0)<1>:d V46(23,0)<16;16,1>:w 0x39db:w {Align1, H1}
// psuedo_mad (16) V67(0,0)<1>:d V469,0)<8;8,1>:w 0x1b5d:w -V66(0,0)<16;16,1>:d
//
// becomes
//
// mul (16) V66(0,0)<1>:d -V46(23,0)<16;16,1>:w 0x39db:w {Align1, H1}
// psuedo_mad (16) V67(0,0)<1>:d V469,0)<8;8,1>:w 0x1b5d:w V66(0,0)<16;16,1>:d
//
static void tryTransferSrcModifier(IR_Builder &builder, G4_INST *def,
                                   G4_Operand *src) {
  // Only when def has no other users.
  if (!def->hasOneUse())
    return;

  // Only transfer for integer types.
  if (!IS_SIGNED_INT(src->getType()))
    return;

  // In case the use type is different from the def type.
  if (!def->getDst() || (def->getDst()->getType() != src->getType()))
    return;

  switch (def->opcode()) {
  default:
    break;

    // Probably this is the only interesting op, since G4_math will not be
    // used to generate mac.
  case G4_mul: {
    // Chances are src1 is an immediate.
    G4_Operand *defSrc1 = def->getSrc(1);
    if (!IS_SIGNED_INT(defSrc1->getType()))
      return;

    if (defSrc1->isImm()) {
      G4_Imm *val = defSrc1->asImm();
      // Mod_Minus is assumed.
      G4_Imm *newVal = builder.createImm(-val->getInt(), val->getType());
      def->setSrc(newVal, 1);
      src->asSrcRegRegion()->setModifier(Mod_src_undef);
    } else if (defSrc1->isSrcRegRegion()) {
      G4_SrcRegRegion *reg = defSrc1->asSrcRegRegion();
      if (reg->getModifier() == Mod_src_undef) {
        reg->setModifier(src->asSrcRegRegion()->getModifier());
        src->asSrcRegRegion()->setModifier(Mod_src_undef);
      } else if (reg->getModifier() == Mod_Minus) {
        reg->setModifier(Mod_src_undef);
        src->asSrcRegRegion()->setModifier(Mod_src_undef);
      }
    }
  } break;
  }
}

// Try to move source modifiers on MAD's src2 into its defintion. This allows
// pseudo_mad ops to be translated into mac ops.
void HWConformity::tryEliminateMadSrcModifier(IR_Builder &builder,
                                              G4_INST *inst) {
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "not a speudo-mad");

  // For pseudo_mad, src2 is the major source operand to be examined later.
  // If there is no modifier on src2, then nothing to do.
  G4_Operand *src2 = inst->getSrc(2);
  if (!src2->isSrcRegRegion())
    return;

  // Currently, only handle modifier minus. To handle others, we may need
  // to insert extra instructions.
  if (src2->asSrcRegRegion()->getModifier() != Mod_Minus)
    return;

  // Only when src2 has a single definition.
  if (G4_INST *def = inst->getSingleDef(Opnd_src2, true)) {
    tryTransferSrcModifier(builder, def, src2);
  }
}

/// Heuristic to decide whether this fp pseudo-mad should be lowered into a
/// GEN mad or not. Returns true if mad is preferred, false otherwise.
///
/// We flavor generating non-mad when this vISA mad is part of b2b mads that
/// share the same dst.
///
bool HWConformity::isFpMadPreferred(G4_BB *bb, INST_LIST_ITER iter) {
  G4_INST *inst = *iter;
  G4_Operand *dst = inst->getDst();
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");

  // Check whether test_inst is sharing the same dst.
  auto equal_mad_dst = [this](G4_INST *test_inst, G4_Operand *dst) {
    if (test_inst->opcode() == G4_pseudo_mad) {
      G4_Operand *test_dst = test_inst->getDst();
      if (test_dst->compareOperand(dst, builder) == Rel_eq)
        return true;
    }
    return false;
  };

  auto next_iter = std::next(iter);
  if (next_iter != bb->end()) {
    G4_INST *next_inst = *next_iter;
    if (equal_mad_dst(next_inst, dst))
      return false;
  }
  if (iter != bb->begin()) {
    auto prev_iter = std::prev(iter);
    G4_INST *prev_inst = *prev_iter;
    if (equal_mad_dst(prev_inst, dst))
      return false;
  }

  // FIXME: remove possile duplicate calls to isGoodAlign16Src, Cm only.
  // This will go away if we use an extra opcode to represent muladd.
  unsigned extraMov = 0;
  for (int k = 0; k < inst->getNumSrc(); k++) {
    if (!isGoodAlign16Src(inst, k)) {
      // If need to insert >1 number of moves, then do not use mad.
      if (++extraMov > 1)
        return false;
    }
  }

  return true;
}

// generate align1 mad, inserting moves if necessary
// returns true if conversion is successful
// for floating point mad this must succeed due to precision requirements
bool HWConformity::generateAlign1Mad(G4_BB *bb, INST_LIST_ITER iter) {

  G4_INST *inst = *iter;
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");
  bool mustDoMad = IS_TYPE_FLOAT_ALL(inst->getDst()->getType());

  // try swapping src0 (really src2) and src1 to see if we can save a move
  // some conditions where swap may help:
  // -- if src0 is D, as MAD only supports D + D * W
  // -- if src1 is imm, as MAD src2 supports 16-bit imm
  // -- if src0 is HF in a mix mode MAD, as MAD src1 supports HF
  // -- if src1 is scalar, as MAD src2 has more region restrictions
  // We perform the swapping before the dst checks as some platforms require dst
  // and src2 to have the same subreg
  {
    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);
    if (IS_DTYPE(src0->getType()) && src0->isSrcRegRegion() &&
        !IS_DTYPE(src1->getType())) {
      inst->swapSrc(0, 1);
    } else if (src1->isImm() && src1->getTypeSize() == 2) {
      // swap src0 and src1 as src0 supports imm
      inst->swapSrc(0, 1);
    } else if (src0->isSrcRegRegion() && !src0->asSrcRegRegion()->isScalar() &&
               src1->isSrcRegRegion() && src1->asSrcRegRegion()->isScalar()) {
      // Swap src0 and src1 if src1 is scalar but src0 is not, as src2 regioning
      // support is quite limited.
      // But don't swap if the swapping causes invalid datatype combination,
      // e.g. src0(:w) * src1(:d) ->  src0(:d) * src1(:w)
      if (IS_DTYPE(src0->getType()) || !IS_DTYPE(src1->getType()))
        inst->swapSrc(0, 1);
    } else if (isLowPrecisionFloatTy(src0->getType()) &&
               src1->getType() == Type_F) {
      inst->swapSrc(0, 1);
    }
  }

  if (!isGoodAlign1TernaryDst(inst)) {
    if (mustDoMad) {
      auto alignment =
          builder.noSrc2Regioning() ? builder.getGRFAlign() : Four_Word;
      inst->setDest(insertMovAfter(iter, inst->getDst(),
                                   inst->getDst()->getType(), bb, alignment));
    } else {
      return false;
    }
  }

  // check src
  bool canBeImm = true;
  bool anyByteSrc = false;
  bool allByteSrc = true;
  for (int k = inst->getNumSrc() - 1; k >= 0; k--) {
    G4_Operand *src = inst->getSrc(k);
    if (!isGoodAlign1TernarySrc(inst, k, canBeImm)) {
      if (mustDoMad) {
        bool isSrc2 = (k == 0);
        if (builder.noSrc2Regioning() && isSrc2) {
          fixSrc2(iter, bb, true);
        } else {
          inst->setSrc(insertMovBefore(iter, k, src->getType(), bb), k);
        }
      } else {
        // Promote src2 from :b to :w to allow mad, for example:
        //     pseudo_mad (16) V211(0,0)<1>:d V210(0,0)<1;0>:d V106(0,0)<0;0>:b
        //     V81(0,0)<1;0>:d
        //  =>
        //     mov (1) TV74(0,0)<1>:w V106(0,0)<0;1,0>:b {Q1, Align1, NoMask}
        //     mad (16) V211(0,0)<1>:d V81(0,0)<1;0>:d V210(0,0)<1;0>:d
        //     TV74(0,0)<0;0>:w {H1, Align1}
        // Do not allow mad if both src1 and src2 are :b as it will generate
        // mov+mov+mad. There is no benefit for instruction count as mov+mov+mas
        // equals to mov+mul+add. In some spilled cases the performace may be
        // even worse as more spill codes inserted.
        bool isSrc2 = (k == 0);
        if (builder.noSrc2Regioning() && isSrc2 && IS_BTYPE(src->getType()) &&
            !IS_BTYPE(inst->getSrc(1)->getType())) {
          bool hasModMinus = false;
          if (src->isSrcRegRegion()) {
            G4_SrcModifier mod = src->asSrcRegRegion()->getModifier();
            hasModMinus = (mod == Mod_Minus || mod == Mod_Minus_Abs);
          }

          // If minus modifier is present, need signed type.
          G4_Type type =
              (IS_SIGNED_INT(src->getType()) || hasModMinus) ? Type_W : Type_UW;
          auto dstStrideInBytes = inst->getDst()->getHorzStride() *
                                  TypeSize(inst->getDst()->getType());
          uint16_t stride = (uint16_t)(dstStrideInBytes / TypeSize(type));
          inst->setSrc(
              insertMovBefore(iter, k, type, bb, stride, builder.getGRFAlign()),
              k);
        } else {
          return false;
        }
      }
    } else {
      if (src->isImm()) {
        if (!(builder.getOption(vISA_EnableInt32DstMulMad) ||
             builder.hasSimplifiedRegions()))
        canBeImm = false;
      }
    }
    if (IS_BTYPE(inst->getSrc(k)->getType())) {
      anyByteSrc = true;
    }
    else {
      allByteSrc = false;
    }
  }

  if (anyByteSrc && !(allByteSrc && IS_WTYPE(inst->getDst()->getType()))) {
    for (int k = inst->getNumSrc() - 1; k >= 0; k--) {
      G4_Operand *src = inst->getSrc(k);
      G4_Type srcType = src->getType();
      if (IS_BTYPE(srcType)) {
        G4_Type newSrcType = IS_UNSIGNED_INT(srcType) ? Type_UW : Type_W;
        inst->setSrc(insertMovBefore(iter, k, newSrcType, bb), k);
      }
    }
  }

  inst->setOpcode(G4_mad);
  // swap src0 and src2 (vISA MAD is src0*src1+src2, while GEN MAD is
  // src1*src2+src0)
  inst->swapSrc(0, 2);

  return true;
}

// convert a FP (HF/F/DF) pseudo-mad into a GEN mad,
// inserting moves if necessary
// returns true if conversion is successful
// note that this must return true for IGC due to precision requirements
bool HWConformity::generateFPMad(G4_BB *bb, INST_LIST_ITER iter) {
  G4_INST *inst = *iter;
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");
  uint8_t execSize = inst->getExecSize();
  G4_DstRegRegion *dst = inst->getDst();

  // Align16 MAD requirements:
  // -- dst and all 3 srcs have the same F/HF/DF type (mixed F/HF is allowed on
  // CHV+)
  // -- dst and all 3 srcs have direct access
  // -- execution size is 16/8/4/1
  // -- dst and src must be packed
  // -- if src region is not scalar, its subregister must be 16 byte aligned

  // do not force fma for CM since it doesn't have precision requirements
  bool preferFpMad = builder.getOption(vISA_forceFPMAD) || builder.favorFpMad();
  if (!preferFpMad) {
    preferFpMad = isFpMadPreferred(bb, iter);
  }

  auto alignMent = execSize * dst->getTypeSize();
  alignMent = (alignMent > 32) ? 32 : alignMent;
  alignMent = (alignMent < 16) ? 16 : alignMent;

  if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 ||
      !builder.tryToAlignOperand(dst, alignMent)) {
    if (preferFpMad) {
      G4_DstRegRegion *tmpDst = insertMovAfter(iter, dst, dst->getType(), bb);
      inst->setDest(tmpDst);
    } else {
      return false;
    }
  }

  // check src
  for (int k = 0; k < inst->getNumSrc(); k++) {
    G4_Type type = inst->getSrc(k)->getType();
    bool goodSrc = isGoodAlign16Src(inst, k);
    if (!goodSrc && preferFpMad) {
      // insert moves if type is legal mad type
      if (isGoodMadType(type)) {
        G4_Operand *src = inst->getSrc(k);
        bool isReplicated =
            (type == Type_DF) && src->isSrcRegRegion() &&
            (src->asSrcRegRegion()->getRegion()->width == 2) &&
            (src->asSrcRegRegion()->getRegion()->horzStride == 0) &&
            (src->asSrcRegRegion()->getRegion()->vertStride == 2);
        if ((type == Type_DF ||
             (type == Type_HF && builder.getPlatform() == GENX_BDW)) &&
            execSize > 1 &&
            (src->isImm() || src->asSrcRegRegion()->isScalar())) {
          // MAD DF does not support .r, so we have to broadcast the value
          // '.r' on MAD HF on BDW is not a replication of that
          // scalar element but a pair of half.
          auto align = type == Type_HF ? builder.getGRFAlign() : Eight_Word;
          broadcast(bb, iter, k, align);
        }
        // No need to insert mov for replicated DF src with <2;2,0> region,
        // which can be encoded as "xyxy" or "zwzw" swizzle based on offfset
        else if (!isReplicated) {
          inst->setSrc(insertMovBefore(iter, k, type, bb), k);
        }
        goodSrc = true;
      }
    }
    if (!goodSrc) {
      return false;
    }
  }

  inst->setOpcode(G4_mad);

  // swap src0 and src2 (vISA MAD is src0*src1+src2, while GEN MAD is
  // src1*src2+src0)
  inst->swapSrc(0, 2);

  return true;
}

// If the LF MAD does not conform to Genx ISA semantics, then translate
// it into a valid GenX sequence - either an equivalent MUL/ADD sequence
// or an equivalent MAC.
// ASSUMPTION:
//    This phase must be called at the end of all other optimizations
//    phases and just prior to testing for ACC spilling.
void HWConformity::fixMADInst(G4_BB *bb) {
  bool doAlign1Mad = builder.hasAlign1Ternary();
  bb->resetLocalIds();
  INST_LIST_ITER i = bb->begin();

  for (auto iterEnd = bb->end(); i != iterEnd; ++i) {
    G4_INST *inst = *i;
    if (inst->opcode() != G4_pseudo_mad) {
      continue;
    }

    tryEliminateMadSrcModifier(builder, inst);

    G4_DstRegRegion *dst = inst->getDst();
    uint32_t exec_size = inst->getExecSize();

    bool conforming_genx_mad = true;

    if (exec_size > G4_ExecSize(builder.getNativeExecSize() * 2)) {
      conforming_genx_mad = false;
    } else {
      switch (dst->getType()) {
      case Type_F:
      case Type_HF:
      case Type_DF:
      case Type_BF:
        break;
      case Type_W:
      case Type_UW:
      case Type_D:
      case Type_UD:
        if (!doAlign1Mad) {
          conforming_genx_mad = false;
        }
        break;
      // XE3P accepts mad instruction with qword destination
      case Type_Q:
      case Type_UQ:
        if (builder.hasWideMulMadOpsEnabled()) {
          if (!doAlign1Mad) {
            conforming_genx_mad = false;
          }
          break;
        }
        // if not enabled, fall through
      default:
        conforming_genx_mad = false;
      }
    }

    if (conforming_genx_mad) {
      bool doMad =
          doAlign1Mad ? generateAlign1Mad(bb, i) : generateFPMad(bb, i);
      if (doMad) {
        // done with this pseudo-mad
        continue;
      }
    }

    // translate MAD into MUL/ADD
    convertMAD2MulAdd(i, bb);
    i++; // skip the add
  }
}

static bool isAccCandidate(G4_INST *inst, Gen4_Operand_Number opndNum,
                           G4_Kernel &kernel)

{
  if (!kernel.fg.builder->canMadHaveSrc0Acc()) {
    return false;
  }

  switch (opndNum) {
  case Opnd_src0:
  case Opnd_src1:
    break;
  default:
    return false;
  }

  if (!inst->canSrcBeAcc(opndNum)) {
    return false;
  }

  return true;
}

struct LiveNode {
  G4_INST *Inst;
  Gen4_Operand_Number OpNum;
  LiveNode(G4_INST *Inst, Gen4_Operand_Number OpNum)
      : Inst(Inst), OpNum(OpNum) {}
};

#define GLOBAL_USE_NUM 15

static bool isSameOperand(G4_Operand *srcOpnd, struct LiveNode *ln) {
  G4_Operand *opnd = ln->Inst->getOperand(ln->OpNum);

  if (opnd->compareOperand(srcOpnd, ln->Inst->getBuilder()) == Rel_eq) {
    return true;
  }

  return false;
}

bool HWConformity::checkDPASSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  G4_Operand *srcs[3] = {nullptr, nullptr, nullptr};
  bool hasOverlap = false;
  G4_DstRegRegion *dst = inst->getDst();

  for (int i = 0; i < inst->getNumSrc(); i++) {
    srcs[i] = inst->getSrc(i);
  }

  if (dst && !inst->hasNULLDst()) {
    for (int i = 0; i < inst->getNumSrc(); i++) {
      G4_CmpRelation rel = dst->compareOperand(srcs[i], builder);
      if (rel == Rel_eq && i == 0) { // src0 is allowed to be same as dst
        continue;
      }
      if (rel != Rel_disjoint) {
        unsigned int src_l = srcs[i]->getLinearizedStart();
        unsigned int src_r = srcs[i]->getLinearizedEnd();
        unsigned int dstGRFSize = src_r - src_l + 1;
        unsigned int elements = dstGRFSize / srcs[i]->getTypeSize();

        G4_Declare *dcl =
            builder.createTempVar(elements, srcs[i]->getType(), ThirtyTwo_Word);
        // Move 2 GRFs, per instruction
        unsigned movInstNum =
            (((dstGRFSize + builder.getGRFSize() - 1) / builder.getGRFSize()) +
             1) /
            2;

        for (unsigned k = 0; k < movInstNum; k++) {
          G4_DstRegRegion *newDst = builder.createDst(
              dcl->getRegVar(), 2 * k, 0, dst->getHorzStride(), Type_F);

          G4_Operand *newSrc =
              builder.createSrc(srcs[i]->getBase(),
                                srcs[i]->asSrcRegRegion()->getRegOff() + 2 * k,
                                srcs[i]->asSrcRegRegion()->getSubRegOff(),
                                builder.getRegionStride1(), Type_F);

          G4_ExecSize numOfF{(2 * builder.getGRFSize()) / TypeSize(Type_F)};
          if (k == movInstNum - 1) {
            numOfF = G4_ExecSize((dstGRFSize / TypeSize(Type_F)) - k * numOfF);
          }
          G4_INST *newInst = builder.createMov(numOfF, newDst, newSrc,
                                               InstOpt_WriteEnable, false);

          bb->insertBefore(iter, newInst);
        }

        // Replace the original source with the float type operand
        G4_Operand *newSrc0 =
            builder.createSrc(dcl->getRegVar(), 0, 0,
                              builder.getRegionStride1(), dcl->getElemType());
        inst->setSrc(newSrc0, 0);
        hasOverlap = true;
      }
    }
  }

  return hasOverlap;
}

G4_INST *HWConformity::evenlySplitDPAS8x8Inst(INST_LIST_ITER iter, G4_BB *bb) {
  auto *inst = *iter;

  // Insert mov if there is dst/src overlap
  checkDPASSrcDstOverlap(iter, bb);

  auto dst = inst->getDst();
  G4_Operand *src[3];
  for (int i = 0; i < 3; i++) {
    src[i] = inst->getSrc(i);
  }

  G4_DstRegRegion *newDst = nullptr;
  if (dst && !inst->hasNULLDst()) {
    unsigned int dst_l = dst->getLinearizedStart();
    unsigned int dst_r = dst->getLinearizedEnd();
    unsigned int GRFSize = (dst_r - dst_l + 1) / kernel.getGRFSize();
    newDst = builder.createDst(dst->getBase(), dst->getRegOff() + GRFSize / 2,
                               dst->getSubRegOff(), dst->getHorzStride(),
                               dst->getType());
    dst->unsetRightBound();
  } else if (inst->hasNULLDst()) // In case null dst
  {
    newDst = builder.duplicateOperand(dst);
  } else {
    newDst = nullptr;
  }

  G4_Operand *newSrc[3];
  for (int i = 0; i < 3; i++) {
    if (i == 1) // Src1 is not changed
    {
      if (src[i]) {
        newSrc[i] = builder.duplicateOperand(src[i]);
      } else {
        newSrc[i] = nullptr;
      }
      continue;
    }

    if (src[i] && !src[i]->isNullReg()) {
      unsigned int src_l = src[i]->getLinearizedStart();
      unsigned int src_r = src[i]->getLinearizedEnd();
      unsigned int GRFSize = (src_r - src_l + 1) / kernel.getGRFSize();
      vISA_ASSERT(((src_r - src_l + 1) % kernel.getGRFSize() == 0),
             "DPAS GRF size not aligned");

      if (GRFSize >= 2) {
        newSrc[i] = builder.createSrc(
            src[i]->getBase(),
            src[i]->asSrcRegRegion()->getRegOff() + GRFSize / 2,
            src[i]->asSrcRegRegion()->getSubRegOff(),
            builder.getRegionStride1(), src[i]->asSrcRegRegion()->getType());
        src[i]->unsetRightBound();
      } else {
        short subRegOff = src[i]->asSrcRegRegion()->getSubRegOff() +
                          ((src_r - src_l + 1) / src[i]->getTypeSize()) / 2;
        newSrc[i] = builder.createSrc(
            src[i]->getBase(), src[i]->asSrcRegRegion()->getRegOff(), subRegOff,
            builder.getRegionStride1(), src[i]->asSrcRegRegion()->getType());
        src[i]->unsetRightBound();
      }
    } else if (src[i]->isNullReg()) {
      newSrc[i] = builder.createNullSrc(src[i]->getType());
    } else {
      newSrc[i] = nullptr;
    }
  }

  G4_InstDpas *dpasInst = inst->asDpasInst();
  G4_INST *newInst = builder.createInternalDpasInst(
      inst->opcode(), inst->getExecSize(), newDst, newSrc[0], newSrc[1],
      newSrc[2], inst->getOption(), dpasInst->getSrc2Precision(),
      dpasInst->getSrc1Precision(), dpasInst->getSystolicDepth(),
      dpasInst->getRepeatCount() / 2);

  dpasInst->setRepeatCount(dpasInst->getRepeatCount() / 2);

  return newInst;
}

void HWConformity::DPASWA(G4_BB *bb, DPASSrc2RSCache *src2GRFCache) {
  INST_LIST_ITER i = bb->begin(), iEnd = bb->end();
  INST_LIST_ITER next_iter = i;
  for (; i != iEnd; i = next_iter) {
    ++next_iter;
    G4_INST *inst = *i;

    if (inst->isDpas()) {
      G4_INST *inst = *i;
      G4_InstDpas *dpasInst = inst->asDpasInst();
      uint8_t depth = dpasInst->getSystolicDepth();
      uint8_t repeatC = dpasInst->getRepeatCount();

      // Always split the first dpas8x8 of the BB, in case the HW issue happens
      // cross BB
      if (depth == 8 && repeatC == 8 && src2GRFCache->firstDpas) {
        G4_INST *newInst = evenlySplitDPAS8x8Inst(i, bb);
        INST_LIST_ITER nextIter = i;
        nextIter++;
        i = bb->insertBefore(nextIter, newInst);
        src2GRFCache->firstDpas = false;
      }

      // DPAS8x8, to disable the read suppression in src2
      if (hasDPASSourceTwoReuse(src2GRFCache, inst)) {
        G4_INST *newInst = evenlySplitDPAS8x8Inst(i, bb);
        INST_LIST_ITER nextIter = i;
        nextIter++;
        i = bb->insertBefore(nextIter, newInst);
      }
    }
  }
}

void HWConformity::localizeForAcc(G4_BB *bb) {
  std::map<const G4_Declare *, G4_Operand *> replacedOperand;
  std::unordered_map<const G4_Declare *, std::vector<struct LiveNode>> useNodes;
  std::vector<const G4_Declare *> erasedCandidates;

  curBB = bb;

  for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd;
       ++instIter) {
    G4_INST *inst = *instIter;

    // Not defined in current BB
    G4_Operand *dst = inst->getOperand(Opnd_dst);
    if (dst && dst->isGreg() && kernel.fg.globalOpndHT.isOpndGlobal(dst)) {
      const G4_Declare *dcl = dst->getTopDcl();
      if (useNodes.find(dcl) != useNodes.end()) {
        useNodes.erase(dcl);                // Maybe added again
        erasedCandidates.emplace_back(dcl); // erased declares
      }
    }

    // Source operand
    for (auto OpNum :
         {Gen4_Operand_Number::Opnd_src0, Gen4_Operand_Number::Opnd_src1,
          Gen4_Operand_Number::Opnd_src2}) {
      G4_Operand *src = inst->getOperand(OpNum);
      if (src && src->isGreg() && kernel.fg.globalOpndHT.isOpndGlobal(src)) {
        const G4_Declare *dcl = src->getTopDcl();
        if ((OpNum != Opnd_src0 && // Acc can be used only for src0 and src1
             OpNum != Opnd_src1) ||
            !isAccCandidate(inst, OpNum,
                            kernel)) // The operand is can be replaced with ACC
        {
          auto dclIter =
              std::find(erasedCandidates.begin(), erasedCandidates.end(), dcl);
          if (dclIter == erasedCandidates.end()) {
            erasedCandidates.emplace_back(dcl);
          }
        } else {
          if (useNodes[dcl].empty() ||
              isSameOperand(src, &(useNodes[dcl][0]))) {
            useNodes[dcl].emplace_back(inst, OpNum);
          }
        }
      }
    }
  }

  for (auto &Nodes : useNodes) {
    const G4_Declare *dcl = Nodes.first;
    auto dclIter =
        std::find(erasedCandidates.begin(), erasedCandidates.end(), dcl);
    if (dclIter != erasedCandidates.end()) {
      // removed already
      continue;
    }

    if (Nodes.second.size() >= GLOBAL_USE_NUM) {
      for (auto &LN : Nodes.second) {
        G4_INST *inst = LN.Inst;
        Gen4_Operand_Number opNum = LN.OpNum;
        int i = inst->getSrcNum(opNum);
        G4_Operand *src = inst->getSrc(i);
        G4_Operand *tmpOpnd = nullptr;

        auto itR = replacedOperand.find(dcl);
        if (itR != replacedOperand.end()) {
          tmpOpnd = builder.duplicateOperand(itR->second);
        } else {
          tmpOpnd = insertCopyAtBBEntry(bb, inst->getExecSize(), src);
          replacedOperand[dcl] = tmpOpnd;
        }
        inst->setSrc(tmpOpnd, i);
      }
    }
  }

  return;
}

// convert a psuedo mad inst into mul/add
// return the iterator pointing to add
void HWConformity::convertMAD2MulAdd(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect pseudo-mad");

  G4_DstRegRegion *addOpDst = inst->getDst();
  G4_Operand *addOpnd2 = inst->getSrc(2);
  G4_Type mulOpDstType = addOpDst->getType();
  G4_Type mulOpExecType = inst->getExecType();
  // pick the widest type of mad's src and dst as the intermediate type
  if (TypeSize(mulOpDstType) > TypeSize(mulOpExecType)) {
    mulOpExecType = mulOpDstType;
  }

  mulOpDstType = mulOpExecType;

  G4_SubReg_Align subAlign = Get_G4_SubRegAlign_From_Type(mulOpDstType);

  // Reuse the MAD op for MUL.
  inst->setOpcode(G4_mul);
  inst->setSrc(nullptr, 2);

  G4_Declare *mulDefDcl =
      builder.createTempVar(inst->getExecSize(), mulOpDstType, subAlign);

  G4_DstRegRegion *mulOpDst = builder.createDstRegRegion(mulDefDcl, 1);
  inst->setDest(mulOpDst);

  // Follow with an ADD.
  INST_LIST_ITER tIter = iter;
  tIter++;

  auto addOpnd1 =
      builder.createSrcRegRegion(mulDefDcl, builder.getRegionStride1());
  G4_INST *addOp = builder.createInternalInst(
      inst->getPredicate(), G4_add, inst->getCondMod(), inst->getSaturate(),
      inst->getExecSize(), addOpDst, addOpnd1, addOpnd2, nullptr,
      inst->getOption());

  bb->insertBefore(tIter, addOp);

  // predicate/condmod/saturate, if they exist, are propagated to the add
  // instruction
  inst->setSaturate(g4::NOSAT);
  inst->setPredicate(NULL);
  inst->setCondMod(nullptr);

  {
    inst->transferDef(addOp, Opnd_src2, Opnd_src1);
    if (addOp->getPredicate()) {
      inst->transferDef(addOp, Opnd_pred, Opnd_pred);
    }
    inst->transferUse(addOp);
    inst->addDefUse(addOp, Opnd_src0);
  }
}

// See if we can convert the pseudo_sada2 instruction into an actual Gen sada2
// This can be done if the following conditions are met:
// -- We can find the definition of the pseudo sada2 instruction's source 2 in
//    the same basic block, and that
// -- it may be replaced by an acc (i.e., the src2 is its only use, the dst and
//    the src have identical regions, and there are no intervening instructions
//    that update acc)
//
// We additionally attempt to schedule up the sada2 instruction to be as close
// as possible to the src2 defining instruction (subject to the constraints of
// def-use chains for def, src0 and src1), so that more opportunites may be
// exposed for later sada2 instructions

void HWConformity::fixSADA2Inst(G4_BB *bb) {

  INST_LIST_ITER i = bb->begin();
  while (i != bb->end()) {

    G4_INST *inst = *i;
    if (inst->opcode() != G4_pseudo_sada2) {
      ++i;
      continue;
    }

    G4_Operand *src2 = inst->getSrc(2);

    bool canDoSada2 = true;
    G4_INST *src2Dst = NULL;

    int emask = inst->getMaskOption();
    if (!bb->isAllLaneActive() && emask != InstOpt_WriteEnable &&
        inst->getMaskOffset() != 0) {
      canDoSada2 = false;
    }

    G4_DstRegRegion *dst = inst->getDst();
    if (canDoSada2) {
      if (src2->isSrcRegRegion() &&
          src2->asSrcRegRegion()->getRegAccess() == Direct) {
        // check Src2
        if (kernel.fg.globalOpndHT.isOpndGlobal(src2)) {
          // no sada2 if operand is global
          canDoSada2 = false;
        } else if (src2->asSrcRegRegion()->getModifier() != Mod_src_undef) {
          // no sada2 if src2 has a modifier
          canDoSada2 = false;
        } else {
          for (auto defIter = inst->def_begin(), end = inst->def_end();
               defIter != end; ++defIter) {
            if ((*defIter).second == Opnd_src2) {
              if (src2Dst != NULL) {
                // no sada2 if src2 has >1 definition
                canDoSada2 = false;
                break;
              }
              src2Dst = (*defIter).first;
            }
          }

          if (!src2Dst) {
            canDoSada2 = false;
          } else {
            if (!src2Dst->hasOneUse()) {
              // no sad2 if def has more than one use
              canDoSada2 = false;
            } else {
              G4_DstRegRegion *src2DstOpnd = src2Dst->getDst();
              G4_Type src2DstType = src2DstOpnd->getType();
              if (src2DstOpnd->getRegAccess() != Direct ||
                  (src2DstType != Type_W && src2DstType != Type_UW)) {
                // no sada2 if def's dst is indirect, or it type is not W or UW
                canDoSada2 = false;
              } else if (src2DstOpnd->compareOperand(src2, builder) != Rel_eq) {
                // no sada2 if src2Dst and src2 are not equal
                canDoSada2 = false;
              }
            }
          }
        }
      } else {
        canDoSada2 = false;
      }
    }

    // The new location of the sada2 after the conversion
    INST_LIST_ITER newSada2Iter = i;
    --newSada2Iter;
    if (canDoSada2) {
      // try to schedule up the sada2 to be as close to the src2-defining
      // instruction as possible to expose more optmizaition opportunities
      for (; *newSada2Iter != src2Dst; --newSada2Iter) {
        if (inst->isRAWdep(*newSada2Iter) || inst->isWAWdep(*newSada2Iter) ||
            inst->isWARdep(*newSada2Iter)) {
          break;
        }
      }

      // make sure there are no instructions between the sada2's new location
      // and the src2-defining instruction that updates acc
      for (auto iter = newSada2Iter; *iter != src2Dst; --iter) {
        G4_INST *aInst = *iter;
        if (aInst->hasACCOpnd()) {
          canDoSada2 = false;
          break;
        }
      }
    }

    if (canDoSada2) {
      // We have verified all conditions and can convert this instruction to
      // sada2. replace the destination for src2Dst to be acc0. The actual acc0
      // offset will be fixed in a later pass
      G4_DstRegRegion *accDstOpnd = builder.createDst(
          builder.phyregpool.getAcc0Reg(), 0, 0, 1, src2->getType());
      src2Dst->setDest(accDstOpnd);
      if (src2Dst->getExecSize() == g4::SIMD1) {
        // This can happen for the first sada2 instruction if src2 is scalar
        // expand its execution size so that acc is fully defined
        src2Dst->setExecSize(inst->getExecSize());
      }

      // create an implicit acc parameter for sada2
      inst->setOpcode(G4_sada2);
      inst->setSrc(nullptr, 2);
      G4_SrcRegRegion *accSrcOpnd =
          builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                            builder.getRegionStride1(), src2->getType());

      inst->setImplAccSrc(accSrcOpnd);

      ++newSada2Iter;
      bb->insertBefore(newSada2Iter, inst);
      i = bb->erase(i);

      // maintain def-use

      for (auto tmpIter = src2Dst->use_begin(), end = src2Dst->use_end();
           tmpIter != end; ++tmpIter) {
        if ((*tmpIter).first == inst && (*tmpIter).second == Opnd_src2) {
          (*tmpIter).second = Opnd_implAccSrc;
          break;
        }
      }

      for (auto tmpIter = inst->def_begin(), end = inst->def_end();
           tmpIter != end; ++tmpIter) {
        if ((*tmpIter).first == src2Dst && (*tmpIter).second == Opnd_src2) {
          (*tmpIter).second = Opnd_implAccSrc;
          break;
        }
      }
    } else {
      // pseudo_sada2 (N) dst src0 src1 src2
      // becomes
      // sad2 (n) tmp<1>:w src0 src1
      // add (n) dst tmp<n;n,1>:w src2

      inst->setOpcode(G4_sad2);
      inst->setSrc(nullptr, 2);

      G4_SubReg_Align sad2TmpSubAlign =
          Get_G4_SubRegAlign_From_Type(dst->getType());

      if ((unsigned)inst->getExecSize() * dst->getTypeSize() >
          kernel.numEltPerGRF<Type_UB>()) {
        // align to GRF
        sad2TmpSubAlign = builder.getGRFAlign();
      }
      // create a new temp variable as sad2's destination
      G4_Declare *sad2Tmp = builder.createTempVar(
          inst->getExecSize(), dst->getType(), sad2TmpSubAlign);
      G4_DstRegRegion *sad2Dst = builder.createDstRegRegion(sad2Tmp, 1);
      inst->setDest(sad2Dst);

      uint16_t srcVertStride, srcWidth, srcHorzStride;
      srcWidth =
          inst->getExecSize() > g4::SIMD8 ? g4::SIMD8 : inst->getExecSize();
      srcHorzStride = 1;
      srcVertStride = srcWidth;

      // opnd 0 for add is the new temp we've just created
      const RegionDesc *rd =
          builder.createRegionDesc(srcVertStride, srcWidth, srcHorzStride);
      G4_Operand *addSrc0Opnd =
          builder.createSrc(sad2Dst->getBase(), 0, 0, rd, sad2Dst->getType());

      // opnd 1 is src2 of the pseudo_sada2
      // dst is the same as the pseudo_sada2
      G4_INST *addInst = builder.createInternalInst(
          inst->getPredicate(), G4_add, inst->getCondMod(), inst->getSaturate(),
          inst->getExecSize(), dst, addSrc0Opnd, src2, NULL, inst->getOption());

      INST_LIST_ITER addLoc = i;
      ++addLoc;
      bb->insertBefore(addLoc, addInst);

      // FIXME: redundant?
      inst->addDefUse(addInst, Opnd_src0);

      // The sad2 op should not have the SAT attribute set,
      // as this is intended only for the final result of the
      // SADA2 (and thus the add op will keep the SAT attribute).
      inst->setSaturate(g4::NOSAT);
      inst->setPredicate(NULL);

      {
        inst->transferDef(addInst, Opnd_src2, Opnd_src1);
        if (addInst->getPredicate()) {
          inst->transferDef(addInst, Opnd_pred, Opnd_pred);
        }
        inst->transferUse(addInst);
        inst->addDefUse(addInst, Opnd_src0);
      }
      ++i;
    }
  }
}

void HWConformity::fixSendInst(G4_BB *bb) {

  for (INST_LIST_ITER i = bb->begin(), end = bb->end(); i != end; i++) {

    G4_INST *inst = *i;
    if (!inst->isSend()) {
      continue;
    }

    if (inst->getExecSize() < builder.getNativeExecSize()) {
      // A64 messages require a minimum msg len of two for address (src0), which
      // is inconsistent with our input IR as it allows <2 GRF address variables
      // (e.g., simd1 A64 scatter r/w). To avoid this causing overlap between
      // send dst/src0/src1 (it is known to cause HW hang), we have to ensure
      // they are all 2GRF-aligned
      G4_Declare *src0Dcl = inst->getSrc(0)->getTopDcl();
      // ToDo: check if dst/src1 may also exhibit such size mismatch
      bool sizeMismatch = inst->getMsgDesc()->getSrc0LenRegs() == 2 &&
                          (src0Dcl && src0Dcl->getRootDeclare()->getByteSize() <
                                          2u * kernel.numEltPerGRF<Type_UB>());
      auto doEvenAlign = [this](G4_Declare *dcl) {
        if (dcl) {
          dcl = dcl->getRootDeclare();
          // variables >= 2 GRF don't need even alignment since they can't
          // possibly overlap
          if (dcl->getByteSize() < 2u * kernel.numEltPerGRF<Type_UB>()) {
            dcl->setEvenAlign();
          }
        }
      };
      if (sizeMismatch) {
        doEvenAlign(inst->getSrc(0)->getTopDcl());
        if (inst->isSplitSend()) {
          doEvenAlign(inst->getSrc(1)->getTopDcl());
        }
        if (builder.WaDisableSendSrcDstOverlap()) {
          doEvenAlign(inst->getDst()->getTopDcl());
        }
      }
    }

    uint16_t offset = 0;
    if (!builder.tryToAlignOperand(inst->getDst(), offset,
                               kernel.numEltPerGRF<Type_UB>())) {
      replaceDst(i, inst->getDst()->getType(), builder.getGRFAlign());
    }

    G4_Operand *src0 = inst->getSrc(0);
    G4_Declare *src0TopDcl = src0->getTopDcl();

    // if src0 and src1 are hard-wired GRF, check that
    // they satisfy EOT and preemption restrictions
    auto needsTempSrc = [this](G4_INST *inst, G4_Declare *dcl) {
      if (dcl == nullptr) // %null is okay
        return false;
      return dcl->getRegVar() && dcl->getRegVar()->getPhyReg() &&
             ((inst->isEOT() && builder.hasEOTGRFBinding() &&
               dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() < 112) ||
              (builder.mustReserveR1() &&
               dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() < 2));
    };

    auto fixSrc = [&](G4_INST *inst, bool isSrc0) {
      auto sendSrc = isSrc0 ? inst->getSrc(0)->asSrcRegRegion()
                            : inst->getSrc(1)->asSrcRegRegion();
      uint16_t rows = isSrc0 ? inst->getMsgDesc()->getSrc0LenRegs()
                             : inst->getMsgDesc()->getSrc1LenRegs();

      G4_Declare *sendSrcDcl = sendSrc->getBase()->asRegVar()->getDeclare();
      G4_Type type = sendSrc->getType();
      G4_VarBase *base = sendSrc->getBase();
      short baseOff = sendSrc->getRegOff();
      short baseSubOff = sendSrc->getSubRegOff();

      if (TypeSize(type) != 4) {
        unsigned int byteSize = sendSrcDcl->getByteSize();
        vISA_ASSERT(byteSize % 4 == 0, "Unexpected src opnd type for send.");
        G4_Declare *tmpDcl = builder.createTempVar(
            byteSize / 4, Type_UD, sendSrcDcl->getSubRegAlign());
        tmpDcl->setAliasDeclare(sendSrcDcl, 0);
        base = tmpDcl->getRegVar();
        baseSubOff *= TypeSize(type) / 4;
        type = Type_UD;
      }

      G4_Declare *dcl = builder.createTempVar(
          rows * builder.getNativeExecSize(), type, builder.getGRFAlign());
      const RegionDesc *region = builder.getRegionStride1();
      for (uint16_t idx = 0; idx != rows; ++idx) {
        G4_SrcRegRegion *src = builder.createSrc(base, baseOff + idx,
                                                 baseSubOff + 0, region, type);
        G4_DstRegRegion *dst =
            builder.createDst(dcl->getRegVar(), idx, 0, 1, type);
        G4_INST *newInst = builder.createMov(builder.getNativeExecSize(), dst,
                                             src, InstOpt_WriteEnable, false);
        bb->insertBefore(i, newInst);
      }

      G4_Operand *newSrc =
          builder.createSrcRegRegion(dcl, builder.getRegionStride1());
      inst->setSrc(newSrc, isSrc0 ? 0 : 1);
    };

    if (needsTempSrc(inst, src0TopDcl)) {
      fixSrc(inst, true);
    }

    if (inst->isSplitSend() && !inst->getSrc(1)->isNullReg()) {
      // src1 may be null because some messages (e.g., CPS) require split send
      if (!builder.tryToAlignOperand(inst->getSrc(1),
                                 kernel.numEltPerGRF<Type_UB>())) {
        inst->setSrc(insertMovBefore(i, 1, inst->getSrc(1)->getType(), bb,
                                     builder.getGRFAlign()),
                     1);
      }
      G4_Operand *src1 = inst->getSrc(1);
      G4_Declare *src1TopDcl = src1->getTopDcl();

      if (needsTempSrc(inst, src1TopDcl)) {
        fixSrc(inst, false);
      }
    }

    if (builder.mustReserveR1()) {
      G4_DstRegRegion *dst = inst->getDst();
      if (!dst->isNullReg()) {
        G4_Declare *dstTopDcl = dst->getTopDcl();
        if (dstTopDcl != NULL && dstTopDcl->getRegVar() &&
            dstTopDcl->getRegVar()->getPhyReg()) {
          vISA_ASSERT(
              (dstTopDcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() > 2),
              "Unexpected preg used for send destination.");
        }
      }
    }

    if (builder.WaDisableSendSrcDstOverlap()) {
      // create copy if dst and src0/src1 overlap due to being the same variable
      bool src0Overlap = inst->getDst()->compareOperand(
                             inst->getSrc(0), builder) != Rel_disjoint;
      bool src1Overlap =
          inst->isSplitSend() && inst->getDst()->compareOperand(
                                     inst->getSrc(1), builder) != Rel_disjoint;
      if (src0Overlap || src1Overlap) {
        int dstSize = inst->getMsgDesc()->getDstLenRegs();
        int src0Size = src0Overlap ? inst->getMsgDesc()->getSrc0LenRegs() : 0;
        int src1Size = src1Overlap ? inst->getMsgDesc()->getSrc1LenRegs() : 0;
        if (inst->getPredicate() ||
            (bb->isDivergent() && !inst->isWriteEnableInst()) ||
            dstSize > src0Size + src1Size) {
          // copy src0/src1 if inst does not update all channels
          // ToDo: the copies may be OOB if src0/src1 are scalar. It should be
          // ok since we don't care about the values, but IR verifier might
          // complain about OOB.
          if (src0Overlap) {
            G4_Declare *copyDst = builder.createTempVar(
                src0Size * kernel.numEltPerGRF<Type_UD>(), Type_UD, Any);
            copyRegs(copyDst, 0,
                     inst->getSrc(0)->getBase()->asRegVar()->getDeclare(),
                     inst->getSrc(0)->asSrcRegRegion()->getRegOff() *
                         builder.getGRFSize(),
                     src0Size, bb, i);
            inst->setSrc(
                builder.createSrcRegRegion(copyDst, builder.getRegionStride1()),
                0);
          }
          if (src1Overlap) {
            G4_Declare *copyDst = builder.createTempVar(
                src1Size * kernel.numEltPerGRF<Type_UD>(), Type_UD, Any);
            copyRegs(copyDst, 0,
                     inst->getSrc(1)->getBase()->asRegVar()->getDeclare(),
                     inst->getSrc(1)->asSrcRegRegion()->getRegOff() *
                         builder.getGRFSize(),
                     src1Size, bb, i);
            inst->setSrc(
                builder.createSrcRegRegion(copyDst, builder.getRegionStride1()),
                1);
          }
        } else {
          // copy dst
          auto dst = inst->getDst();
          auto dstDcl = dst->getBase()->asRegVar()->getDeclare();
          auto copyIter = std::next(i);
          G4_Declare *copySrc = builder.createTempVar(
              dstSize * kernel.numEltPerGRF<Type_UD>(), Type_UD, Any);
          // speical case when send dst declare is <1 GRF (it must still be
          // GRF-aligned)
          if (dstDcl->getByteSize() < builder.getGRFSize()) {
            auto numDWords = dstDcl->getByteSize() / TypeSize(Type_UD);
            vASSERT(numDWords > 0);
            copyDwords(dstDcl, 0, copySrc, 0, numDWords, bb, copyIter);
          } else {
            copyRegs(dstDcl, dst->getRegOff() * builder.getGRFSize(), copySrc,
                     0, dstSize, bb, copyIter);
          }
          inst->setDest(builder.createDstRegRegion(copySrc, 1));
        }
      }
    }

    // Avoid src0 and src1 overlap for split send on pre-Xe platforms.
    // simd1 split send is allowed to have srcs overlap. When it's simd1,
    // overlap for the rest of the payload shouldn't matter
    if (inst->isSplitSend() && inst->getExecSize() != g4::SIMD1 &&
        builder.noSrc0Src1OverlapSend()) {
      bool src0Src1Overlap = inst->getSrc(0)->compareOperand(
                                 inst->getSrc(1), builder) != Rel_disjoint;

      // Fix overlap
      if (src0Src1Overlap)
        fixSrc(inst, true);
    }
  }
}

void HWConformity::fixsrc1src2Overlap(G4_BB *bb) {
  for (INST_LIST_ITER i = bb->begin(), end = bb->end(); i != end; i++) {
    G4_INST *inst = *i;

    if (inst->opcode() != G4_mad) {
      continue;
    }

    G4_Operand *src1 = inst->getSrc(1);
    G4_Operand *src2 = inst->getSrc(2);

    if (src1 && src2 && !src1->isNullReg() && !src2->isNullReg() &&
        src1->getType() == src2->getType()) {
      G4_CmpRelation cmpResult = src1->compareOperand(src2, builder);
      if (cmpResult != Rel_disjoint && cmpResult != Rel_undef) {
        G4_Type movType = src2->getType();
        bool changeType = true;
        switch (src2->getType()) {
        case Type_DF:
          movType = Type_UQ;
          break;
        case Type_F:
          movType = Type_UD;
          break;
        case Type_HF:
          movType = Type_UW;
          break;
        default:
          changeType = false;
          break;
        }
        if (changeType) {
          G4_Operand *opnd = insertMovBefore(i, 2, movType, bb);
          INST_LIST_ITER prev_it = i;
          prev_it--;
          G4_INST *movInst = (*prev_it);
          movInst->getSrc(0)->asSrcRegRegion()->setType(builder, movType);
          opnd->asSrcRegRegion()->setType(builder, src2->getType());
          inst->setSrc(opnd, 2);
        }
      }
    }
  }
}

//
// Fix sel and csel instructions:
//  -- set their cond mod to null as they don't modify it.  They will be
//  hard-coded to f0.0 in Gen asm

void HWConformity::fixSelCsel(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  if (inst->opcode() == G4_sel || inst->opcode() == G4_csel) {
    G4_CondMod *condMod = inst->getCondMod();
    if (condMod) {
      condMod->setBase(nullptr);
    }
  }

  // When opcode is "sel" and the destination is "word" datatype with execution
  // datatype as "dword" (i.e. at least one of the source operand is a dword),
  // compiler must not produce instructions with destination in the odd dword
  // location in the GRF e.g. word locations 2, 3, 6, 7, 10, 11, 14, 15, .. etc.
  // A move instruction with a temporary register may be used as shown below:
  // (~f0.1.any) sel (8|M8) (sat)r12.2<2>:uw -r8.23<1;2,0>:w (abs)r6.8<0;1,0>:d
  // =>
  // (~f0.1.any) sel (8|M8) (sat)r10.0<2>:uw -r8.23<1;2,0>:w (abs)r6.8<0;1,0>:d
  // (~f0.1.any) mov (8|M8) r12.2<2>:uw r10.0<2>:uw
  if (builder.hasSelWDstDwExecTypeAlignIssue() && inst->opcode() == G4_sel) {
    bool isWDstAndDwExecType =
        IS_WTYPE(inst->getDst()->getType()) && IS_DTYPE(inst->getExecType());
    // The WA requires dst to be 8 bytes aligned
    if (isWDstAndDwExecType && !builder.tryToAlignOperand(inst->getDst(), 8))
      replaceDst(it, inst->getDst()->getType(), builder.getGRFAlign());
  }
}

void HWConformity::avoidDstSrcOverlap(PointsToAnalysis &p) {
  for (auto &bb : kernel.fg) {
    INST_LIST_ITER i = bb->begin(), iEnd = bb->end();
    INST_LIST_ITER next_iter = i;
    curBB = bb;
    for (; i != iEnd; i = next_iter) {
      ++next_iter;
      avoidInstDstSrcOverlap(i, bb, p);
    }
  }
}

// Second half of a source operand must not point to the same register as the
// first half of destination operand in a compressed instruction.
// Avoid the dst and src overlap when they are using the same variable by
// inserting a mov instruction add(8)  var1<2>, var2, var1<0, 1, 0>
void HWConformity::avoidInstDstSrcOverlap(INST_LIST_ITER it, G4_BB *bb,
                                          PointsToAnalysis &p) {
  G4_INST *inst = *it;

  if (inst->isSend() || inst->isDpas() || inst->opcode() == G4_nop ||
      inst->opcode() == G4_madm || inst->isLabel()) {
    return;
  }

  auto dst = inst->getDst();
  if (!dst || dst->isNullReg() || !dst->getBase()->isRegVar()) {
    return;
  }

  unsigned grfSize = builder.getGRFSize();
  G4_Declare *dstDcl = dst->getTopDcl();
  if (dstDcl) {
    G4_DstRegRegion *dstRgn = dst;
    bool dstCrossGRF =
        ((dstRgn->getSubRegOff() * dstRgn->getTypeSize()) % grfSize +
         (dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart()) + 1) >
        grfSize;
    int dstFirstHalf = dst->getLinearizedStart() / grfSize;

    bool srcOverlap = false;
    for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++) {
      G4_Operand *src = inst->getSrc(i);

      if (!src || src->isNullReg() || !src->getTopDcl()) {
        continue;
      }
      G4_Declare *srcDcl = src->getTopDcl();

      if (src->isSrcRegRegion()) {
        G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
        if (srcDcl == dstDcl && srcRgn->getRegAccess() == Direct &&
            srcRgn->getBase()->isRegVar()) {
          bool srcCrossGRF =
              ((srcRgn->getSubRegOff() * srcRgn->getTypeSize()) % grfSize +
               (srcRgn->getLinearizedEnd() - srcRgn->getLinearizedStart()) +
               1) > grfSize;
          // The half define in region rule "second half of a source operand
          // must not point to the same register as the first half of
          // destination operand in a compressed instruction" is exactly size
          // half, not GRF boundary based half.
          int srcSecondHalf = 0;
          if (srcRgn->getRegion()->isContiguous(
                  inst->getExecSize())) { // For contiguous region, linear
                                          // start/end can be used to calculate
                                          // the start GRF of half size of
                                          // region
            srcSecondHalf = (srcRgn->getLinearizedStart() +
                             ((srcRgn->getLinearizedEnd() -
                               srcRgn->getLinearizedStart() + 1) /
                              2)) /
                            grfSize;
          } else { // For non-congtiguous region, there are holes in the region,
                   // the start of second half elements need be calcauted in
                   // stride and elemement sizes at same time.
            // Such as in following cases, there is no first/second half overlap issues.
            // add(M1, 32) V146(0,1)<2> V146(0,1)<2;1,0> V146(0,0)<2;1,0>
            // add(M1, 16) V147(0,2)<4> V147(0,2)<4;1,0> V147(0,1)<4;1,0>
            // add(M1, 16) V148(0,3)<4> V148(0,3)<4;1,0> V148(0,1)<4;1,0>
            const RegionDesc *regionDesc = srcRgn->getRegion();
            uint16_t vertSize = regionDesc->vertStride * srcRgn->getElemSize();
            uint16_t execTypeSize =
                regionDesc->horzStride == 0
                    ? srcRgn->getElemSize()
                    : regionDesc->horzStride * srcRgn->getElemSize();
            uint16_t rowSize = regionDesc->horzStride == 0
                                   ? execTypeSize
                                   : regionDesc->width * execTypeSize,
                     numRows = regionDesc->vertStride == 0
                                   ? 1
                                   : inst->getExecSize() / regionDesc->width,
                     numElePerRow = rowSize / execTypeSize,
                     numExecEmePerRow =
                         regionDesc->horzStride == 0 ? 1 : regionDesc->width;
            uint16_t totalNumEle = (regionDesc->vertStride >= numElePerRow)
                                       ? (numRows * numExecEmePerRow)
                                       : (srcRgn->getLinearizedEnd() -
                                          srcRgn->getLinearizedStart() + 1) /
                                             execTypeSize;
            srcSecondHalf =
                (srcRgn->getLinearizedStart() + (totalNumEle / 2) * vertSize) /
                grfSize;
          }

          if (dstCrossGRF || srcCrossGRF) {
            if (dstFirstHalf == srcSecondHalf) {
              srcOverlap = true;
              break;
            }
          }
        } else if (srcRgn->isIndirect()) {
          G4_RegVar *ptvar = NULL;
          int vid = 0;
          while ((ptvar = p.getPointsTo(srcDcl->getRegVar(), vid++)) != NULL) {
            G4_Declare *dcl = ptvar->getDeclare();
            if (dstDcl == dcl) {
              srcOverlap = true;
              break;
            }
          }
        }
      }
    }

    if (srcOverlap) {
      G4_AccRegSel accSel = inst->getDst()->getAccRegSel();
      G4_DstRegRegion *newDst =
          insertMovAfter(it, inst->getDst(), inst->getDst()->getType(), bb);
      newDst->setAccRegSel(accSel);
      inst->setDest(newDst);

      // After inserting temp to avoid dst/src overlapping, need to check
      // additional region rules to ensure both the new instruction and the
      // inserted mov instruction are legitimate. For example:
      // mov (M1, 4) V67(0,0)<1>:df (-)V67(0,5)<1;1,0>:df
      // After fixing dst/src overlapping =>
      // mov (M1, 4) TV1(0,0)<1>:df (-)V67(0,5)<1;1,0>:df
      // (W) mov (M1, 4) V67(0,0)<1>:df TV1(0,0)<1;1,0>:df
      // Both instructions are illegal.
      if (!builder.supportFloatOr64bRegioning()) {
        auto nextIt = std::next(it);
        fixUnalignedRegions(it, bb);
        fixUnalignedRegions(nextIt, bb);
      }
    }
  }
}

void HWConformity::fixCalla(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *fcall = *it;
  G4_Operand *src0 = fcall->getSrc(0);

  // fcall could have imm/label src for direct call
  // No need to fix src reg at the case
  if (!src0->isSrcRegRegion())
    return;

  if (builder.tryToAlignOperand(src0, builder.getGRFSize()))
    return;

  // insert a mov before fcall(calla) to mov src to a grf aligned reg
  replaceSrc(it, 0, src0->getType(), bb, builder.getGRFAlign());
}

void HWConformity::replaceHFBFwithFloat(INST_LIST_ITER it, G4_BB *bb) {
  auto *inst = *it;
  auto *dst = inst->getDst();
  auto *src0 = inst->getSrc(0);
  vASSERT(src0->getType() == Type_BF || src0->getType() == Type_HF);

  G4_InstDpas *dpasInst = inst->asDpasInst();
  uint8_t C = dpasInst->getRepeatCount();

  unsigned int src_l = src0->getLinearizedStart();
  unsigned int src_r = src0->getLinearizedEnd();
  unsigned int dstGRFSize =
      (src_r - src_l + 1) * (TypeSize(Type_F) / src0->getTypeSize());
  unsigned movInstNum =
      (((dstGRFSize + builder.getGRFSize() - 1) / builder.getGRFSize()) + 1) /
      2; // 2 GRFs per instruction

  G4_Declare *dcl = builder.createTempVar(builder.getNativeExecSize() * C,
                                          Type_F, ThirtyTwo_Word);

  // Copy HF/BF data to float with mov instructions.
  // If the new destination is more than 2 GRFs, multiple moves required.
  for (unsigned i = 0; i < movInstNum; i++) {
    G4_DstRegRegion *newDst = builder.createDst(dcl->getRegVar(), 2 * i, 0,
                                                dst->getHorzStride(), Type_F);

    G4_Operand *newSrc = builder.createSrc(
        src0->getBase(), src0->asSrcRegRegion()->getRegOff() + i,
        src0->asSrcRegRegion()->getSubRegOff(), builder.getRegionStride1(),
        src0->asSrcRegRegion()->getType());

    G4_ExecSize numOfF{(2 * builder.getGRFSize()) / TypeSize(Type_F)};
    if (i == movInstNum - 1) {
      numOfF = G4_ExecSize((dstGRFSize / TypeSize(Type_F)) - i * numOfF);
    }
    G4_INST *newInst =
        builder.createMov(numOfF, newDst, newSrc, InstOpt_WriteEnable, false);

    bb->insertBefore(it, newInst);
  }

  // Replace the original source with the float type operand
  G4_Operand *newSrc0 = builder.createSrc(
      dcl->getRegVar(), 0, 0, builder.getRegionStride1(), dcl->getElemType());
  inst->setSrc(newSrc0, 0);

  return;
}

bool HWConformity::hasDPASSourceTwoReuse(DPASSrc2RSCache *src2GRFCache,
                                         G4_INST *inst) {
  G4_InstDpas *dpasInst = inst->asDpasInst();
  uint8_t depth = dpasInst->getSystolicDepth();
  uint8_t repeatC = dpasInst->getRepeatCount();

  // Any non8x8 dpas will flush the src2 read suppression cache
  if (!(depth == 8 && repeatC == 8)) {
    for (int i = 0; i < 16; i++) {
      src2GRFCache->GRFCache[i] = nullptr;
    }
    return false;
  }
  G4_Operand *opnd = inst->getOperand(Opnd_src2);

  if (opnd) {
    int LB = opnd->getLinearizedStart();
    int RB = opnd->getLinearizedEnd();

    int startReg = LB / kernel.numEltPerGRF<Type_UB>();
    int endReg = RB / kernel.numEltPerGRF<Type_UB>();
    G4_Declare *src2Dcl = opnd->getTopDcl();
    // Cached?
    for (int i = 0; i < 16; i++) {
      if (src2GRFCache->GRFCache[i] == src2Dcl) {
        for (int i = 0; i < 16; i++) {
          src2GRFCache->GRFCache[i] = nullptr;
        }

        return true;
      }
    }

    // Add to cache
    for (int i = startReg; i <= endReg; i++) {
      src2GRFCache->latestID = src2GRFCache->latestID % 16;
      src2GRFCache->GRFCache[src2GRFCache->latestID] = src2Dcl;
      if (src2GRFCache->latestID % 4 ==
          0) // 4GRF per block, one is polluted, all others cannot be reuse
      {
        src2GRFCache->GRFCache[src2GRFCache->latestID + 1] = nullptr;
        src2GRFCache->GRFCache[src2GRFCache->latestID + 2] = nullptr;
        src2GRFCache->GRFCache[src2GRFCache->latestID + 3] = nullptr;
      }
      src2GRFCache->latestID++;
    }
  }
  return false;
}

void HWConformity::fixDPAS(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22010725011) &&
      !builder.getOption(vISA_EnableDPASBFHFH)) {
    G4_Type src0Type = inst->getSrc(0)->getType();

    if (src0Type == Type_BF || src0Type == Type_HF) {
      replaceHFBFwithFloat(it, bb);
    }
  }
  G4_InstDpas *dpasInst = inst->asDpasInst();
  uint8_t depth = dpasInst->getSystolicDepth();
  uint8_t repeatC = dpasInst->getRepeatCount();
  if (builder.hasGRFAlignedSrc2DPAS() && depth == 8 && repeatC == 8 &&
      !builder.tryToAlignOperand(inst->getSrc(2), builder.getGRFSize())) {
    G4_INST *newInst = evenlySplitDPAS8x8Inst(it, bb);
    INST_LIST_ITER nextIter = it;
    nextIter++;
    it = bb->insertBefore(nextIter, newInst);
  }
}

void HWConformity::conformBB(G4_BB *bb) {
  INST_LIST_ITER i = bb->begin(), iEnd = bb->end();
  INST_LIST_ITER next_iter = i;
  for (; i != iEnd; i = next_iter) {
    // by default we skip the newly inserted instructions as we assume they are
    // already HW conformed if a check may produce new instructions that violate
    // HW rules, it must adjust the next_iter to point to them
    ++next_iter;
    G4_INST *inst = *i;
    G4_opcode opcode = inst->opcode();

    if (inst->isDpas()) {
      fixDPAS(i, bb);
      continue;
    }

    if (inst->isFCall() && builder.supportCallaRegSrc() &&
        VISA_WA_CHECK(builder.getPWaTable(), Wa_1608127078))
      fixCalla(i, bb);

    if (opcode == G4_nop || opcode == G4_label) {
      continue;
    }

    if (builder.getOption(vISA_InsertDummyMovForHWRSWA) &&
        (VISA_WA_CHECK(builder.getPWaTable(), Wa_16012061344) ||
         VISA_WA_CHECK(builder.getPWaTable(), Wa_16012292205))) {
      fixPredicateIndirectInst(i, bb);
    }
    // do this early since otherwise the moves inserted by other passes may
    // still inherit bad regions from the original inst
    fixSrcRegion(inst);

    bool changed = fixMov(i, bb);
    if (changed) {
      next_iter = i;
      next_iter++;
    }

    fixOpndType(i, bb);

    fixSelCsel(i, bb);

    fixPredCtrl(i, bb);

    if (inst->getExecSize() > builder.getNativeExecSize()) {
      if (inst->opcode() == G4_math && inst->getDst()->getType() == Type_HF &&
          inst->getSrc(0)->getType() == Type_HF &&
          (!inst->getSrc(1) || inst->getSrc(1)->getType() == Type_HF)) {
        // split pure HF math to simd8
        evenlySplitInst(i, bb);
      }
    }

    if (inst->opcode() == G4_madw) {
      next_iter = fixMadwInst(i, bb);
      continue;
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fix3SrcInst(i, bb);

    G4_Operand *dst = inst->getDst();

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    if (inst->isMath()) {
      if (fixMathInst(i, bb)) {
        // check the newly added insts later
        next_iter = i;
        next_iter++;
      }
    }

    inst = *i;

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    if (inst->opcode() == G4_mul) {
      if (fixMULInst(i, bb)) {
        // inserted mach and mov
        // check the newly added insts later (MUL, MACH, MOV)
        next_iter = i;
        next_iter++;
      }
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    if (inst->opcode() == G4_mulh) {
      fixMULHInst(i, bb);
      next_iter = i;
      continue;
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    // HW check #6: indirect operand spilling
    fixIndirectOpnd(i, bb);

    fixIndirectSrcForCompressedInst(i, bb);

    fixIndirectMoviSimd16ToSimd8(i, bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    // HW check #8: unsigned dst with execution type F
    /* If the execution type is F and the destination type is either UD, UW
     * or UB and the destination is not saturated, then we need to add an
     * intermediate type conversion to D.
     */
    inst = *i;
    opcode = inst->opcode();

    if (opcode == G4_cmp || opcode == G4_cmpn) {
      dst = inst->getDst();
      int dst_elsize = 0;
      bool null_dst = !dst || inst->hasNULLDst();
      if (!null_dst) {
        dst_elsize =
            dst->isPredicate() ? TypeSize(Type_UW) : dst->getTypeSize();
      }
      int extypesize;
      G4_Type extype = inst->getOpExecType(extypesize);
      fixCompareInst(i, bb, extype, dst_elsize);
    }
    dst = inst->getDst();

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    if (fixAcc(i, bb)) {
      next_iter = i;
      next_iter++;
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    {
      dst = inst->getDst();
      G4_Type extype = inst->getExecType2();
      int extypesize = TypeSize(extype);
      int dst_elsize = 0;
      if (dst) {
        dst_elsize = dst->getTypeSize();
      }

      if (dst && inst->getExecSize() == g4::SIMD1 && dst_elsize < extypesize &&
          !IS_VTYPE(extype) && !inst->isMixedMode() &&
          !hasDedicateAlignRegionConformity(inst) && !inst->isSend()) {
        fixDstHstride(i, extypesize);
      }
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    bool planeDeleted = fixPlaneInst(i, bb);
    if (planeDeleted) {
      continue;
    }

    fixLine(i, bb);
    fixRotate(i, bb);

    if (!builder.hasVxHFloat64b()) {
      fixVxHFloat64b(i, bb);
    }

    if (builder.supportFloatOr64bRegioning()) {
      // This function is for pre-Xe_XeHPSDV platforms. Xe_XeHPSDV+ platforms
      // should be fixed in fixUnalignedRegions() later.
      if (fix64bInst(i, bb)) {
        continue;
      }
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    fixImm64(i, bb); // fixed immediates for DF4 in fixImm64()

    if ((*i)->opcode() == G4_mov) {
      if (fixBFMove(i, bb)) {
        continue;
      }
    }
    if ((*i)->opcode() == G4_fcvt) {
      (void)fixFcvt(i, bb);
      continue;
    }
    if ((*i)->opcode() == G4_srnd) {
      (void)fixSrnd(i, bb);
      continue;
    }

    if ((*i)->opcode() == G4_shl || (*i)->opcode() == G4_shr ||
        (*i)->opcode() == G4_asr) {
      fixShiftInsts(i, bb);
      continue;
    }

    if (builder.getPlatform() == GENX_BDW) {
      fixPackedHFConversions(i, bb);
    }

    fixFloatARFDst(i, bb);
  }

  if (!builder.supportFloatOr64bRegioning()) {
    for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd;
         /* empty */) {
      // pre-compute nextIter as the call may destroy iter
      auto nextIter = std::next(iter);
      // since insertMovBefore/After and similar helper instructions do not
      // understand XeHP_SDV regioning restrictions, they may produce illegal
      // moves We do a catch call pass here to catch them
      fixUnalignedRegions(iter, bb);
      iter = nextIter;
    }
  }

  // previous legalization passes may introduce int64 moves on platforms that
  // don't support int64 we do another catch-all pass here to legalize any such
  // moves ToDo: see if we can remove other calls to emulate64Mov()
  if (builder.noInt64()) {
    for (auto I = bb->begin(), E = bb->end(); I != E;) {
      auto inst = *I;
      auto next = std::next(I);
      if (inst->opcode() == G4_mov && (IS_QTYPE(inst->getDst()->getType()) ||
                                       IS_QTYPE(inst->getSrc(0)->getType()))) {
        emulate64bMov(I, bb);
      }
      I = next;
    }
  }

  // Do immdiate Address offset OOB check as previous fixes may generate
  // invalid ImmAddrOffset.
  for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd; ++iter) {
    fixImmAddrOffsetOOB(iter, bb);
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
  }

  if (builder.getNativeExecSize() <= g4::SIMD8) {
    return;
  }
  i = bb->begin(), iEnd = bb->end();
  next_iter = i;
  for (; i != iEnd; i = next_iter) {
    // by default we skip the newly inserted instructions as we assume they are
    // already HW conformed if a check may produce new instructions that violate
    // HW rules, it must adjust the next_iter to point to them
    ++next_iter;
    fixByteXBarRestriction(i, bb);
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
  }

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14017715663) ||
      builder.hasBDstWSrc1EvenAlignIssue()) {
    i = bb->begin(), iEnd = bb->end();
    next_iter = i;
    for (; i != iEnd; i = next_iter) {
      ++next_iter;
      if (fixOddAlignSrc1Region(i, bb)) {
        INST_LIST_ITER pre_iter = i;
        pre_iter--;
        fixByteXBarRestriction(pre_iter, bb);
      }
#ifdef _DEBUG
      verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    }
  }

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_16012725276)) {
    for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
      auto inst = *it;
      if (!inst->getDst() || inst->isSend() || inst->isDpas()) {
        continue;
      }

      bool hasNonQTypeScalarSrc = false;
      bool hasQTypeDst = false;
      bool hasQTypeSrc = false;

      if (IS_QTYPE(inst->getDst()->getType())) {
        hasQTypeDst = true;
      }

      for (int i = 0; i < inst->getNumSrc(); i++) {
        auto src = inst->getSrc(i);
        if (IS_QTYPE(src->getType())) {
          hasQTypeSrc = true;
        } else if (src->isSrcRegRegion() &&
                   src->asSrcRegRegion()->getRegion()->isScalar()) {
          hasNonQTypeScalarSrc = true;
        }
      }

      // WA-1
      if ((hasQTypeDst || hasQTypeSrc) && hasNonQTypeScalarSrc &&
          inst->getExecSize() != g4::SIMD1) {
        for (int i = 0; i < inst->getNumSrc(); i++) {
          auto src = inst->getSrc(i);
          if (!IS_QTYPE(src->getType()) && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->getRegion()->isScalar()) {
            inst->setSrc(insertMovBefore(
                             it, i,
                             IS_SIGNED_INT(src->getType()) ? Type_Q : Type_UQ,
                             bb, builder.getGRFAlign()),
                         i);
          }
        }
      }

      // WA-2
      if (hasQTypeDst && inst->getDst()->isCrossGRFDst(builder)) {
        hasQTypeSrc = false;
        for (int i = 0; i < inst->getNumSrc(); i++) {
          if (IS_QTYPE(inst->getSrc(i)->getType())) {
            hasQTypeSrc = true;
            break;
          }
        }
        if (!hasQTypeSrc) {
          evenlySplitInst(it, bb);
        }
      }

#ifdef _DEBUG
      verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    }
  }
}

//
// SIMD16 addc/subb are illegal on GEN, since they write to acc and there are
// only 8 acc channels for D/UD type.  In vISA IR we should get something like
//   addc (16|M0) V0  V2       V3
//   use  (16|M0) V1  ... acc0:ud // or :d
// which needs to be translated to
//   addc (8|M0)  V0(0)  V2(0)  V3(0)
//   use  (8|M0)  V1(0) ... acc0:ud
//   addc (8|M8)  V0(1)  V2(1)  V3(1)
//   use  (8|M8)  V1(1) ... acc0:ud
// NOTE: we also support other consumers such as add.
//
//
// We do this first thing in HW conformity to avoid REXES from splitting
// addc/subb incorrectly We also count on previous opt to preserve the inst pair
// by not inserting any acc using inst in between; it should hopefully be the
// case since we generally don't optimize instructions with acc src/dst
//
// If exec size of addc is < 8, we also have to make sure both the addc's dst
// and the carry move's dst are GRF-aligned, since acc's channel is dependent on
// the dst's subreg offset.  In other words, we fix
//   addc (1) r1.0 ...
//   mov (1) r1.1 acc0.0<0;1,0>
// into
//   addc (1) r1.0 ...
//   mov (1) r2.0 acc0.0<0;1,0>
//   mov (1) r1.1 r2.0
//
bool HWConformity::fixAddcSubb(G4_BB *bb) {
  bool changed = false;
  for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd; ++iter) {
    G4_INST *inst = *iter;

    if (inst->opcode() != G4_addc && inst->opcode() != G4_subb) {
      continue;
    }
    if ((builder.hasSimplifiedRegions() ||
         builder.getOption(vISA_GAReArchBugFix) ||
         builder.removedAccRestrictionsAsGRF()) &&
        inst->getExecSize() == g4::SIMD32) {
      continue;
    }

    // Fix the src1 if it's a immediate operand whose type can only be :ud
    for (int i = 0; i < 2; i++) {
      G4_Operand *src = inst->getSrc(i);
      if (src && src->isImm() && src->getType() == Type_UW) {
        // just change the immediate's type to :ud
        uint32_t immVal = (uint32_t)src->asImm()->getImm();
        inst->setSrc(builder.createImm(immVal, Type_UD), i);
      }
    }

    if (inst->getExecSize() != builder.getNativeExecSize()) {
      // find the matching carry move
      G4_INST *carryUse = nullptr;
      auto srchIter = iter;
      for (++srchIter; srchIter != iterEnd; ++srchIter) {
        G4_INST *inst2 = *srchIter;
        auto op = inst2->opcode();

        bool opPossibleConsumer = op == G4_mov || op == G4_add ||
                                  op == G4_addc || op == G4_mad ||
                                  op == G4_pseudo_mad || op == G4_add3;

        // only check for a handful of user instructions
        // this list could be extended
        if (opPossibleConsumer && inst2->getExecSize() == inst->getExecSize() &&
            inst2->useAcc()) {
          carryUse = inst2;
          break;
        } else if (inst2->useAcc()) {
          // someone redefines acc0; we can stop looking
          break;
        }
      }

      if (carryUse == NULL) {
        // can't find the move using acc, skip this addc/subb
        VISA_DEBUG(std::cout << "unable to find addc/subc consumer\n");
        continue;
      }

      if (inst->getExecSize() > builder.getNativeExecSize()) {
        // we're breaking a bigger instruction into a smaller one
        evenlySplitInst(iter, bb);
        evenlySplitInst(srchIter, bb);

        // srchIter now points to the second half of move, and we want to move
        // the first move to be before the second half of the addc/subb, which
        // is pointed by iter
        --srchIter;
        G4_INST *mov1 = *srchIter;
        bb->erase(srchIter);
        bb->insertBefore(iter, mov1);

        changed = true;
      } else {
        // we will need to GRF-align addc's dst as well as the move dst,
        // so that the acc will have the correct offset
        // note that insertMovAfter will align the tmp since addc/subb has
        // implicit acc use
        if (!builder.tryToAlignOperand(inst->getDst(), 32)) {
          inst->setDest(insertMovAfter(iter, inst->getDst(),
                                       inst->getDst()->getType(), bb));
          changed = true;
        }
        if (!builder.tryToAlignOperand(carryUse->getDst(), 32)) {
          carryUse->setDest(insertMovAfter(srchIter, carryUse->getDst(),
                                           carryUse->getDst()->getType(), bb));
          changed = true;
        }
      }
    }
  }
  return changed;
}

//
// Mixed mode instruction allows bfloat16 operands in the following cases:
//   1. dst, src0, and src1 for 2 source instructions format not involving
//   multiplier(mov, add, cmp, sel).
//   2. dst and src0 for 2 source instructions format involving multiplier(mul,
//   mac etc).
//   3. dst, src0, and src1 for 3 source instructions format(mad).
//   4. Broadcast of bfloat16 scalar is not supported.
//   5. Unpacked bfloat16 destination with stride 2 when register offset is 0
//   or 1.
//   6. Packed bfloat16 source and destination when register offset is 0 or 8
//   (16 for PVC+).
//   7. Execution size must not be greater than 8 (16 for PVC+)
//   8. Instructions with pure bfloat16 operands are not supported.
//
// **More examples**
//   1. BF imm is not allowed
//      mov  (1|M0)  r12.0<1>:f  0xffff:bf - ILLEGAL "Imm operand with BF type
//      is not allowed"
//   2. BF scalar operand can be used in SIMD1
//      mul  (1|M0)  r14.0<1>:f  r11.0<0;1,0>:bf  r12.3<0;1,0>:f - OK
//   3. For SIMD1, scalar operands (both dst/src) of F or BF can have any
//   subreg!
//      add  (1|M0)  r16.3<1>:bf  r11.0<0;1,0>:f  r12.3<0;1,0>:f - OK
//   4. F Operand should have subreg = 0 if execSize > SIMD1
//      add  (2|M0)  r10.4<1>:f  r11.0<1;1,0>:bf   0x12345:f
//       ILLEGAL "Src0 regioning must be aligned to destination or scalar for
//       Float/64bit pipes"
//   5. Others
//     add  (8|M0)  r16.0<2>:bf  r11.0<1;1,0>:f  r12.0<1;1,0>:f- OK
//     add  (8|M0)  r16.1<2>:bf  r11.0<1;1,0>:f  r12.8<1;1,0>:f- OK
//     add  (8|M0)  r16.0<1>:bf  r11.0<1;1,0>:f  r12.8<1;1,0>:f- OK
//     add  (8|M0)  r16.8<1>:bf  r11.0<1;1,0>:f  r12.0<1;1,0>:f- OK
//         Note that float source operands  can be scalar region <0;1,0>
//
void HWConformity::fixBFMixedMode() {
  auto useGivenType = [](G4_INST *I, G4_Type GivenTy) {
    G4_Operand *dst = I->getDst();
    // ignore cmp's dst (?)
    if (dst && !dst->isNullReg() && !I->isCompare()) {
      if (dst->getType() == GivenTy)
        return true;
    }
    for (int i = 0; i < I->getNumSrc(); ++i) {
      G4_Operand *src = I->getSrc(i);
      if (src && !src->isNullReg()) {
        if (src->getType() == GivenTy)
          return true;
      }
    }
    return false;
  };

  auto allowBFForInst = [](G4_INST *I,
                           Gen4_Operand_Number OpndNum = Opnd_total_num) {
    // Only mul/mac/mad/add/cmp/mov/sel support BF mixed mode.
    switch (I->opcode()) {
    case G4_mul:
    case G4_mac: {
      if (OpndNum == Opnd_src1)
        return false;
      return true;
    }
    case G4_mad:
      return OpndNum != Opnd_src2; // gen mad: s0+s1*s2
    case G4_pseudo_mad:
      return OpndNum != Opnd_src0; // s0*s1+s2
    case G4_add:
    case G4_cmp:
    case G4_mov:
    case G4_sel:
      return true;
    default:
      break;
    }
    return false;
  };

  auto skipBFCheck = [&useGivenType](G4_INST *I) {
    // Skip dpas/send
    if (I->isDpas() || I->isSend())
      return true;

    // Do not use BF, skip
    if (!useGivenType(I, Type_BF))
      return true;

    // Special case:
    //   1.  mov d:bf  s:bf   --> mov d:uw  s:uw
    //   2.  mov d:f   s:bf   --> shl d:ud  s:uw  16:ud
    if (I->opcode() == G4_mov && I->getSrc(0)->getType() == Type_BF) {
      // this will be handled by fixBFMov.
      return true;
    }
    return false;
  };

  if (!kernel.fg.builder->hasBFMixMode()) {
    return;
  }

  const G4_ExecSize nativeES = kernel.fg.builder->getNativeExecSize();
  for (auto &bb : kernel.fg) {
    // First iteration:
    //    1. Legalize scalar BF operand for insts that need splitting
    //       (If this is done in 3, we will have more than 1 scalar mov.)
    //         mul (16|M0)  d<1>:f  s0<1;1,0>:bf  s1<0;1,0>:bf
    //       ==>
    //         (W) mov (1|M0) t<1>:f   s1<0;1,0>:bf
    //         mul (16|M0)  d<1>:f  s0<1;1,0>:bf  t<0;1,0>:f
    //    2. split instructions  (case 7)
    //         add (16|M0)   d:bf   s0:bf    s1:bf
    //       ==>
    //         add (8|M0)   d:bf    s0:bf    s1:bf
    //         add (8|M8)   d.8:bf  s0.8:bf  s1.8:bf
    //    3. legalize operands by using cvt mov to BF or from BF. (case 1&2&3)
    //         mul (8|M0) d:bf   s0:bf  s1:bf
    //       ==>
    //         mov (8|M0) s:f   s1:bf
    //         mul (8|M0) t:bf   s0:bf  s:f
    //    Note pure BF insts will be handled in the second iteration.
    INST_LIST_ITER nextII = bb->begin();
    for (auto II = nextII, IE = bb->end(); II != IE; II = nextII) {
      ++nextII;
      G4_INST *Inst = *II;
      if (skipBFCheck(Inst))
        continue;

      const bool isBFAllowedInst = allowBFForInst(Inst);
      const G4_ExecSize currES = Inst->getExecSize();
      std::list<INST_LIST_ITER> instsToSplit;

      // 1. Handle illegal BF scalar by generating mov
      //    First generate mov for scalars instead of splitting first and
      //    than generating mov. Doing so reduces the number of mov insts.
      bool changed = false;
      if (currES > nativeES) {
        // No splitting needed for an inst whose execsize <= nativeES, as
        // its operand takes one GRF at most.
        for (int i = 0, nsrc = (int)Inst->getNumSrc(); i < nsrc; ++i) {
          G4_Operand *S = Inst->getSrc(i);
          Gen4_Operand_Number opndNum = Inst->getSrcOperandNum(i);
          if (S->getType() == Type_BF && S->isSrcRegRegion()) {
            if (S->asSrcRegRegion()->getRegion()->isScalar() &&
                (!isBFAllowedInst || !allowBFForInst(Inst, opndNum))) {
              G4_Operand *newSrc = insertMovBefore(II, i, Type_F, bb);
              Inst->setSrc(newSrc, i);
              changed = true;
            }
          } else if (S->getType() == Type_BF && S->isImm()) {
            vISA_ASSERT(false, "BF immediate not supported!");
          }
        }
      }

      // If changed, check if it still uses BF. Skip if not.
      if (changed && !useGivenType(Inst, Type_BF)) {
        continue;
      }

      // 2. Split instruction (case 7) if needed
      //    Now, BF operands are all non-scalar for insts that need splitting.
      //    We split inst under the following:
      //      1. If an inst, which don't support BF, has BF operands. Those BF
      //      operands
      //         must be replaced with F operands (by inserting mov to convert
      //         BF to F). If replacing a BF operand with a F operand makes it
      //         cross 2 GRF, it must be splitted (currES * F" > 2 GRF); or
      //      2. Split if currES > nativeES for insts that support BF. (case 7)
      std::list<INST_LIST_ITER> instsToCheck;
      if ((!isBFAllowedInst &&
           (TypeSize(Type_F) * currES) > (builder.getGRFSize() * 2)) ||
          (isBFAllowedInst && currES > nativeES)) {
        if (currES == g4::SIMD32) {
          splitSIMD32Inst(II, bb);
          if (isBFAllowedInst && nativeES == g4::SIMD8) {
            // need to split again.
            INST_LIST_ITER prev_it = std::prev(II);
            evenlySplitInst(prev_it, bb);
            instsToCheck.push_back(std::prev(prev_it));
            instsToCheck.push_back(prev_it);
            evenlySplitInst(II, bb);
          }
        } else {
          evenlySplitInst(II, bb);
        }
        instsToCheck.push_back(std::prev(II));
        instsToCheck.push_back(II);
      } else {
        instsToCheck.push_back(II);
      }

      // 3. Change BF operands, which are not legal, to F by generating mov.
      //    (isBFAllowedInst should be still valid to check if any new
      //    instruction
      //     from splitting is BF allowed or not.)
      for (const auto &LI : instsToCheck) {
        INST_LIST_ITER thisII = LI;
        G4_INST *tI = *thisII;
        for (int i = 0, nsrc = (int)tI->getNumSrc(); i < nsrc; ++i) {
          G4_Operand *S = tI->getSrc(i);
          Gen4_Operand_Number opndNum = tI->getSrcOperandNum(i);
          if (S->getType() == Type_BF &&
              (!isBFAllowedInst || !allowBFForInst(tI, opndNum))) {
            G4_Operand *newSrc = insertMovBefore(thisII, i, Type_F, bb);
            tI->setSrc(newSrc, i);
          }
        }

        G4_DstRegRegion *Dst = tI->getDst();
        if (!isBFAllowedInst && Dst && !Dst->isNullReg() &&
            Dst->getType() == Type_BF) {
          G4_DstRegRegion *newDst = insertMovAfter(thisII, Dst, Type_F, bb);
          tI->setDest(newDst);

          auto movII = std::next(II);
          instsToSplit.push_back(movII);
          G4_INST *movI = *movII;

          Inst->transferUse(movI);
          Inst->addDefUse(movI, Opnd_src0);
        }
      }
      instsToCheck.clear();
    }

    // Second iteration:
    //     Legalize regions by using mov.
    nextII = bb->begin();
    for (auto II = nextII, IE = bb->end(); II != IE; II = nextII) {
      ++nextII;
      G4_INST *Inst = *II;
      if (skipBFCheck(Inst))
        continue;

      // Because of the first iteration above, this inst must support bf mixed
      // mode.
      vASSERT(allowBFForInst(Inst));

      const G4_ExecSize currES = Inst->getExecSize();
      bool changed = false;
      // case 4: broadcast of bf is not supported!
      //    As this bf operand is changed to F. At the end of loop, need to
      //    check if this inst still has both BF and F, and "changed" is for
      //    this purpose.
      // case 8: pure BF is not allowed.
      for (int i = 0, nsrc = (int)Inst->getNumSrc(); i < nsrc; ++i) {
        G4_Operand *S = Inst->getSrc(i);
        if (S->getType() == Type_BF) {
          vASSERT(S->isSrcRegRegion());
          G4_SrcRegRegion *srcReg = S->asSrcRegRegion();
          if ((srcReg->getRegion()->isScalar() &&
               currES > g4::SIMD1) // broadcast BF scalar
              || (i == (nsrc - 1) && !useGivenType(Inst, Type_F))) // pure BF.
          {
            // Insert bf->f, which is just a left-shift.
            uint32_t nelts =
                (uint32_t)(srcReg->getRegion()->isScalar() ? g4::SIMD1
                                                           : currES);
            G4_Declare *newDcl = builder.createTempVar(
                nelts, Type_UD,
                (nelts == 1) ? Even_Word : builder.getGRFAlign(), "cvtF",
                false);
            G4_DstRegRegion *newDst =
                builder.createDst(newDcl->getRegVar(), Type_UD);
            srcReg->setType(builder, Type_UW);
            G4_INST *shlInst = builder.createBinOp(
                G4_shl, (nelts == 1) ? g4::SIMD1 : currES, newDst, S,
                builder.createImm(16, Type_UD), InstOpt_WriteEnable, false);
            bb->insertBefore(II, shlInst);

            // srcMod, if present, must be on the promoted F operand!
            G4_SrcModifier sMod = srcReg->getModifier();
            srcReg->setModifier(Mod_src_undef);
            G4_SrcRegRegion *newSrc =
                builder.createSrc(newDcl->getRegVar(), 0, 0,
                                  (nelts == 1) ? builder.getRegionScalar()
                                               : builder.getRegionStride1(),
                                  Type_F);
            newSrc->setModifier(sMod);
            Inst->setSrc(newSrc, i);

            Gen4_Operand_Number opndNum = Inst->getSrcOperandNum(i);
            Inst->transferDef(shlInst, opndNum, Opnd_src0);
            shlInst->addDefUse(Inst, opndNum);

            changed = true;
          }
        }
      }

      if (changed) {
        // Check again if there is still BF type, if not, we are done.
        if (!useGivenType(Inst, Type_BF)) {
          continue;
        }
      }

      if (currES == g4::SIMD1) {
        // Done
        continue;
      }

      for (int i = 0, nsrc = (int)Inst->getNumSrc(); i < nsrc; ++i) {
        G4_Operand *S = Inst->getSrc(i);
        if (S->getType() == Type_F &&
            (S->isImm() || (S->isSrcRegRegion() &&
                            S->asSrcRegRegion()->getRegion()->isScalar()))) {
          continue;
        }

        vASSERT(S->isSrcRegRegion());
        G4_SrcRegRegion *sReg = S->asSrcRegRegion();

        // case 6: Packed bfloat16 source and destination when register offset
        // is 0 or 8.
        //         (also for Float dst/src alignment)
        //         Note that for F, enforce it to have subRegOff = 0 (too
        //         restrictive?)
        bool isPackedSrc =
            (sReg->getRegion()->isContiguous(Inst->getExecSize()) &&
             (sReg->getSubRegOff() == 0 || (sReg->getType() == Type_BF &&
                                            sReg->getSubRegOff() == nativeES)));
        if (isPackedSrc) {
          continue;
        }

        G4_Operand *newSrc =
            insertMovBefore(II, i, sReg->getType(), bb, builder.getGRFAlign());
        Inst->setSrc(newSrc, i);
      }

      if (Inst->isCompare()) {
        // Ignore compare's dst.
        continue;
      }

      G4_DstRegRegion *dst = Inst->getDst();
      uint32_t subOff = dst->getSubRegOff();
      // case 5
      bool isUnpackedDst =
          (dst->getType() == Type_BF && dst->getHorzStride() == 2 &&
           (subOff == 0 || subOff == 1));
      // case 6, note for F, force it to have subOff = 0
      bool isPackedDst =
          (dst->getHorzStride() == 1 &&
           (subOff == 0 || (subOff == nativeES && dst->getType() == Type_BF)));
      if (!(isPackedDst || isUnpackedDst)) {
        // case 5 Unpacked bfloat16 destination with stride 2 when register
        // offset is 0 or 1.
        G4_DstRegRegion *newDst =
            insertMovAfter(II, dst, dst->getType(), bb, builder.getGRFAlign());
        Inst->setDest(newDst);

        auto movII = std::next(II);
        G4_INST *movI = *movII;

        Inst->transferUse(movI, false);
        Inst->addDefUse(movI, Opnd_src0);
      }
    }
  }
}

void HWConformity::fixEvEURegionRestrictions(G4_BB *bb) {
  for (auto II = bb->begin(), IE = bb->end(); II != IE; ++II) {
    G4_INST *inst = *II;
    bool needDstMove = false;
    unsigned short dstStride = 1;
    short dstOff = -1;

    if (inst->getExecSize() == g4::SIMD1) {
      continue;
    }

    auto isFixCandidate = [&](G4_INST *I) {
      if (I->isFlowControl() || I->isSend() || I->isDpas())
        return false;

      if (I->isMov()) {
        auto *dst = I->getDst();
        auto execTy = I->getExecType();
        // Packed destination for float instruction is not supported
        if (dst != nullptr && !dst->isNullReg() && dst->getTopDcl() &&
            dst->getTopDcl()->getRegFile() == G4_GRF &&
            TypeSize(dst->getType()) * dst->getHorzStride() <
                TypeSize(execTy) &&
            (IS_TYPE_FLOAT_ALL(dst->getType()) ||
             IS_TYPE_FLOAT_ALL(I->getSrc(0)->getType()))) {
          needDstMove = true;
          dstStride = TypeSize(execTy) / TypeSize(dst->getType());
          dstOff = 0;
        } else {
          return false;
        }
      }
      if (builder.enableMovCvt()) {
        auto *dst = I->getDst();
        auto *src = I->getSrc(0);
        auto execTy = I->getExecType();
        if (inst->opcode() == G4_mov) {
          if ((TypeSize(dst->getType()) *
                   dst->getHorzStride() < // Fix me, double check packed
                                          // destination condition
               TypeSize(execTy)) &&
              (((dst->getType() == Type_HF || dst->getType() == Type_BF) &&
                src->getType() == Type_F) ||
               (dst->getType() == Type_F &&
                (src->getType() == Type_HF || src->getType() == Type_BF)))) {
            return false;
          }
        }

        if (inst->opcode() == G4_fcvt) {
          if ((dst->getType() == Type_UB || dst->getType() == Type_B) &&
              src->getType() == Type_HF) {
            return false;
          }
        }

        if (inst->opcode() == G4_srnd) {
          return false;
        }
      }

      return true;
    };

    if (!isFixCandidate(inst))
      continue;

    auto allowTrueRegionOnSrc0 = [this](G4_INST *inst) {
      auto op = inst->opcode();
      // FIXME: mac/mach will be removed
      if (op == G4_mul || op == G4_mac || op == G4_mach || op == G4_mullh) {
        return false;
      }

      if (!builder.hasSimplifiedRegions() &&
          !builder.getOption(vISA_AllowSrcCRegion)) {
        return false;
      }

      auto canSwapSrc1Src2{[&inst]() {
        if (inst->opcode() == G4_add3 || inst->opcode() == G4_mad)
          return true;

        if (inst->opcode() == G4_bfn) {
          uint8_t funcCtrl = inst->asBfnInst()->getBooleanFuncCtrl();
          for (int k = 0; k < 8; k++) {
            auto calculateIdx{[](const int &k) {
              int b1 = (k >> 1) & 0x1;
              int b2 = (k >> 2) & 0x1;
              int x = b1 ^ b2;
              x = (x << 1) | (x << 2);
              return (k ^ x);
            }};
            if (((funcCtrl >> k) ^ (funcCtrl >> calculateIdx(k))) & 0x1) {
              return false;
            }
          }
          return true;
        }

        return false;
      }};

      G4_Operand *src2 = inst->getSrc(2);
      if (src2 == nullptr)
        return true;

      if (src2->isImm())
        return false;

      auto src2Rgn = src2->asSrcRegRegion();
      if (src2->isNullReg() || !src2->getTopDcl() ||
          (src2->getTopDcl()->getRegFile() != G4_GRF &&
           src2->getTopDcl()->getRegFile() != G4_INPUT))
        return true;

      if (src2Rgn->isScalar()) {
        G4_Operand *src1 = inst->getSrc(1);
        if (src1 == nullptr || !src1->isSrcRegRegion())
          return false;

        auto src1Rgn = src1->asSrcRegRegion();
        if (src1->isNullReg() || !src1->getTopDcl() ||
            (src1->getTopDcl()->getRegFile() != G4_GRF &&
             src1->getTopDcl()->getRegFile() != G4_INPUT))
          return false;

        if (src1Rgn->isScalar())
          return false;

        bool canBeImm = !src1->isImm() || !src2->isImm();
        if (canSwapSrc1Src2()) {
          inst->swapSrc(1, 2);
          inst->swapDefUse(Opnd_src1, Opnd_src2);
          if (!isGoodAlign1TernarySrc(inst, 1, canBeImm) ||
              !isGoodAlign1TernarySrc(inst, 2, canBeImm)) {
            inst->swapSrc(1, 2);
            inst->swapDefUse(Opnd_src1, Opnd_src2);
            return false;
          }
          return true;
        }

        return false;
      }

      return true;
    };

    auto execTy = inst->getExecType();
    auto dst = inst->getDst();
    G4_Type dstTy = (dst == nullptr) ? Type_UD : dst->getType();

    // Check if there is common base for dst/src or src/src operands
    bool hasCommonBase = true;
    G4_VarBase *commonBase = nullptr;
    if (dst != nullptr && dst->getTopDcl() &&
        dst->getTopDcl()->getRegFile() == G4_GRF) {
      commonBase = dst->getBase();

      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        if (i == 0 && allowTrueRegionOnSrc0(inst))
          continue;

        G4_Operand *src = inst->getSrc(i);

        if (src != nullptr && src->isSrcRegRegion()) {
          auto srcRgn = src->asSrcRegRegion();
          if (!srcRgn->isIndirect() && !src->isNullReg() && src->getTopDcl() &&
              (src->getTopDcl()->getRegFile() == G4_GRF ||
               src->getTopDcl()->getRegFile() == G4_INPUT)) {
            if (srcRgn->isScalar())
              continue;

            G4_VarBase *srcBase = src->getBase();
            if (commonBase == nullptr) {
              commonBase = srcBase;
            } else if (commonBase != srcBase) {
              hasCommonBase = false;
              break;
            }
          }
        }
      }
    } else {
      hasCommonBase = false;
    }

    // Check packed destionation restriction
    if (dst != nullptr) {
      dstTy = dst->getType();
      dstStride = dst->getHorzStride();
      if ((dst->getTopDcl() && dst->getTopDcl()->getRegFile() == G4_GRF) ||
          dst->isAccReg()) {
        dstOff = dst->getSubRegOff();
        if (!needDstMove && TypeSize(dstTy) * dstStride < TypeSize(execTy)) {
          needDstMove = true;
          dstStride = TypeSize(execTy) / TypeSize(dst->getType());
          dstOff = 0;
        }
        // Packed float cannot have indirect dst region
        else if (dst->isIndirect() || (!hasCommonBase && dst->getTopDcl() &&
                                       dst->getTopDcl()->getSubRegAlign() !=
                                           builder.getGRFAlign())) {
          needDstMove = true;
          dstStride = 1;
          dstOff = 0;
        }
      }
    }

    // Check if there is common offset for src operands
    bool hasCommonOff = true;
    int commonOff = -1;
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      if (i == 0 && allowTrueRegionOnSrc0(inst))
        continue;

      G4_Operand *src = inst->getSrc(i);

      if (src != nullptr && src->isSrcRegRegion()) {
        auto srcTy = src->getType();
        auto srcRgn = src->asSrcRegRegion();

        if (!srcRgn->isIndirect() && !src->isNullReg() && src->getTopDcl() &&
            (src->getTopDcl()->getRegFile() == G4_GRF ||
             src->getTopDcl()->getRegFile() == G4_INPUT)) {
          if (srcRgn->isScalar())
            continue;

          if (!hasCommonBase &&
              src->getTopDcl()->getSubRegAlign() != builder.getGRFAlign()) {
            hasCommonOff = false;
            break;
          }

          int newOff = srcRgn->getSubRegOff() * TypeSize(srcTy);
          if (commonOff == -1) {
            commonOff = newOff;
          } else if (commonOff != newOff) {
            hasCommonOff = false;
            break;
          }
        }
      }
    }

    // Adjust null dst stride to save potential mov
    if (inst->hasNULLDst()) {
      dstOff = 0;
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        if (i == 0 && allowTrueRegionOnSrc0(inst))
          continue;

        G4_Operand *src = inst->getSrc(i);
        if (src != nullptr && src->isSrcRegRegion()) {
          auto srcTy = src->getType();
          auto srcRgn = src->asSrcRegRegion();
          if (!srcRgn->isIndirect() && !src->isNullReg() && src->getTopDcl() &&
              (src->getTopDcl()->getRegFile() == G4_GRF ||
               src->getTopDcl()->getRegFile() == G4_INPUT) &&
              !srcRgn->isScalar()) {
            const RegionDesc *srcRgnDesc = srcRgn->getRegion();
            auto srcWidth = srcRgnDesc->width;
            auto srcHS = srcRgnDesc->horzStride;
            auto srcVS = srcRgnDesc->vertStride;
            uint16_t srcStride = (srcWidth == 1) ? srcVS : srcHS;
            if ((hasCommonBase ||
                 src->getTopDcl()->getSubRegAlign() == builder.getGRFAlign()) &&
                ((srcWidth == 1) || (srcVS == srcHS * srcWidth))) {
              uint32_t srcDSize = srcStride * TypeSize(srcTy);
              uint32_t dstDsize = dstStride * TypeSize(dstTy);
              if (!needDstMove && srcDSize > dstDsize) {
                dstStride = srcDSize / TypeSize(dstTy);
                dst->setHorzStride(dstStride);
                break;
              }
            }
          }
        }
      }
    }

    // FIXME: remove this WA if pure BF mov will be supported
    auto fixPureBFMov = [this](G4_INST *I) {
      auto movDst = I->getDst();
      auto movSrc = I->getSrc(0);
      if ((movDst->getType() == Type_BF && movSrc->getType() == Type_BF) ||
          (movDst->getType() == Type_HF && movSrc->getType() == Type_HF)) {
        movDst->setType(builder, Type_UW);
        movSrc->asSrcRegRegion()->setType(builder, Type_UW);
      }
    };

    // Check flat region restrictions
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      if (i == 0 && allowTrueRegionOnSrc0(inst))
        continue;

      G4_Operand *src = inst->getSrc(i);

      if (src != nullptr && src->isSrcRegRegion()) {
        auto srcTy = src->getType();
        auto srcRgn = src->asSrcRegRegion();
        uint16_t tmpStride = dstStride * TypeSize(dstTy) / TypeSize(srcTy);
        uint16_t srcStride = 1;

        bool needSrcMove = false;
        if (srcRgn->isIndirect()) {
          needSrcMove = true;
        } else if (!src->isNullReg() && src->getTopDcl() &&
                   (src->getTopDcl()->getRegFile() == G4_GRF ||
                    src->getTopDcl()->getRegFile() == G4_INPUT)) {
          const RegionDesc *srcRgnDesc = srcRgn->getRegion();
          auto srcWidth = srcRgnDesc->width;
          auto srcHS = srcRgnDesc->horzStride;
          auto srcVS = srcRgnDesc->vertStride;
          auto srcOff = srcRgn->getSubRegOff();
          srcStride = (srcWidth == 1) ? srcVS : srcHS;

          if (srcRgn->isScalar()) {
            needSrcMove = false;
          } else if ((hasCommonBase || src->getTopDcl()->getSubRegAlign() ==
                                           builder.getGRFAlign()) &&
                     ((srcWidth == 1) || (srcVS == srcHS * srcWidth)) &&
                     (srcStride * TypeSize(srcTy) ==
                      dstStride * TypeSize(dstTy))) {
            if (hasCommonOff &&
                srcOff * TypeSize(srcTy) ==
                    (dstOff * TypeSize(dstTy) *
                     (TypeSize(execTy) / TypeSize(dstTy)) % 64)) {
              needSrcMove = false;
            } else if ((builder.hasSimplifiedRegions() ||
                        builder.getOption(vISA_RelaxQWRegion)) &&
                       dstStride * TypeSize(dstTy) == 8 &&
                       !IS_TYPE_FLOAT_ALL(dstTy) &&
                       ((inst->opcode() == G4_mul && (i == 0 || i == 1)) ||
                        (inst->opcode() != G4_mul && (i == 1 || i == 2)))) {
              // Regioning for instructions with qword aligned destination
              // Mul instructions apply the following on src0 and src1
              // Non-mul instructions apply the following on src1 and src2
              // - dwords on source must be strided to all align to either
              //   dword0 or dword1 of the qword aligned destination.
              // - words on source must be strided to all align to either
              //   word0 or word2 of the qword aligned destination.
              if (dstOff == 0) {
                if ((TypeSize(srcTy) == 4 && (srcOff == 0 || srcOff == 1)) ||
                    (TypeSize(srcTy) == 2 && (srcOff == 0 || srcOff == 2))) {
                  needSrcMove = false;
                }
              } else {
                needSrcMove = true;
              }
            } else {
              needSrcMove = true;
            }
          } else {
            needSrcMove = true;
          }

          if (!needDstMove && needSrcMove && dstOff != 0) {
            needDstMove = true;
          }

          if (needDstMove || dstOff == 0) {
            if (((srcWidth == 1) || (srcVS == srcHS * srcWidth)) &&
                (srcStride * TypeSize(srcTy) == dstStride * TypeSize(dstTy)) &&
                (srcOff == 0 &&
                 src->getTopDcl()->getSubRegAlign() == builder.getGRFAlign())) {
              needSrcMove = false;
            }
          }
        }

        // FIXME: to remove WA below for emulating mixed mode bf regioning
        // Case-1:
        //   mov (16|M0) r102.0<2>:uw  r21.0<1;1,0>:uw
        //   mul (16|M0) r103.0<1>:f   r102.0<2;1,0>:bf  r232.0<0;1,0>:f
        //   =>
        //   mov (16|M0) r102.0<1>:uw  r21.0<1;1,0>:uw
        //   mul (16|M0) r103.0<1>:f   r102.0<1;1,0>:bf  r232.0<0;1,0>:f
        // Case-2:
        //   mov(16|M0)  r171.0<2>:uw  r3.16<1;1,0>:uw
        //   mad(16|M0)  r107.0<1>:f   r38.0<1;0>:f r171.0<4;2>:bf r36.0<1>:f
        //   =>
        //   mov(16|M0)  r171.0<1>:uw  r3.16<1;1,0>:uw
        //   mad(16|M0)  r107.0<1>:f   r38.0<1;0>:f r171.0<1;0>:bf r36.0<1>:f
        if (srcTy == Type_BF) {
          tmpStride = srcStride;
        }

        if (needSrcMove && !(inst->isMov() && needDstMove)) {
          // New horizontal stride must be <= 4
          if (tmpStride > 4) {
            needDstMove = true;
            tmpStride = TypeSize(dstTy) / TypeSize(srcTy);
          }

          G4_SrcModifier srcMod = srcRgn->getModifier();
          inst->setSrc(insertMovBefore(II, i, srcTy, bb, tmpStride,
                                       builder.getGRFAlign()),
                       i);

          auto movInst = *(std::prev(II));
          fixPureBFMov(movInst);

          if (srcMod != Mod_src_undef) {
            inst->getSrc(i)->asSrcRegRegion()->setModifier(srcMod);
            movInst->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
          }
        }
      }
    }

    if (needDstMove) {
      inst->setDest(insertMovAfter(II, dst, dstTy, bb, builder.getGRFAlign()));
      auto movInst = *(std::next(II));
      fixPureBFMov(movInst);
      if (IS_BTYPE(movInst->getDst()->getType()) ||
          IS_BTYPE(movInst->getSrc(0)->getType())) {
        fixByteXBarRestriction(std::next(II), bb);
      }
    }
  }
}

void HWConformity::chkHWConformity() {
  fixDataLayout();

  fixBFMixedMode();

  for (auto bb : kernel.fg) {
    curBB = bb;
    fixIntToHFMove(bb);
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    fixAddcSubb(bb);
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fixMADInst(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fixBfnInst(bb);
#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    // fix source operand first to avoid redundant MOVs if this fix is done
    // after reducing execution size. used by 3d. Mainly to fix sel with two imm
    // sources
    fixOpndTypeAlign(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fixInstExecSize(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fixMixedHFInst(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    fixSADA2Inst(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    fixSendInst(bb);

    if (builder.avoidSrc1Src2Overlap()) {
      fixsrc1src2Overlap(bb);
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    conformBB(bb);

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    if (builder.hasDPASSrc2ReadSuppressionIssue()) {
      DPASSrc2RSCache src2GRFCache;
      DPASWA(bb, &src2GRFCache);
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

    if (builder.hasSimplifiedRegions() ||
      builder.getOption(vISA_EURegionRemoval)) {
      fixEvEURegionRestrictions(bb);
    }

#ifdef _DEBUG
    verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
  }

  if (builder.avoidDstSrcOverlap()) {
    PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
    p.doPointsToAnalysis(kernel.fg);

    avoidDstSrcOverlap(p);
  }
}

bool HWConformity::hasBadRegion(G4_INST *inst) {
  if (inst->getImplAccDst() || inst->getImplAccSrc())
    return false;
  bool badRegion = false;

#define G4_MAX_ADDR_IMM 511
#define GENX_MAX_H_STRIDE 4
  for (unsigned int srcNum = 0, n_srcs = inst->getNumSrc(); srcNum < n_srcs;
       srcNum++) {
    if (!(inst->getSrc(srcNum)->isSrcRegRegion())) {
      continue;
    }
    const RegionDesc *rd = inst->getSrc(srcNum)->asSrcRegRegion()->getRegion();
    if (rd->isRegionWH()) {
      badRegion = true;
      break;
    }
    if (rd->horzStride == GENX_MAX_H_STRIDE && rd->width > 1) {
      badRegion = true;
      break;
    }
    G4_SrcRegRegion *expandSrcRegion = inst->getSrc(srcNum)->asSrcRegRegion();
    if (expandSrcRegion->getRegAccess() != Direct) {
      const RegionDesc *origRegion = expandSrcRegion->getRegion();
      short secondSubRegOffDiff = 0, secondAddrImmedDiff = 0;

      if (origRegion->width == 1) {
        secondSubRegOffDiff = origRegion->vertStride;
      } else {
        secondSubRegOffDiff = origRegion->horzStride;
      }
      secondAddrImmedDiff =
          (short)(secondSubRegOffDiff * expandSrcRegion->getTypeSize());
      if ((expandSrcRegion->getAddrImm() + secondAddrImmedDiff) >
          G4_MAX_ADDR_IMM) {
        badRegion = true;
        break;
      }
    }
  }
  return badRegion;
}

bool HWConformity::canSplitInst(G4_INST *inst, G4_INST *use_op) {
  if ((inst->getPredicate() && inst->getExecSize() < g4::SIMD16) ||
      hasBadRegion(inst))
    return false;

  G4_CondMod *condMod = inst->getCondMod();
  if (condMod) {
    return false;
  }

  for (int i = 0; i < inst->getNumSrc(); i++) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isAccReg()) {
      // don't split inst with explicit acc
      return false;
    }
    if (src->isSrcRegRegion() &&
        src->asSrcRegRegion()->getRegion()->vertStride == 32 &&
        src->asSrcRegRegion()->getRegion()->width == 1) {
      // don't split the source into even/odd since verstride can't exceed 32
      // ToDo: check for horizontal stride as well?
      return false;
    }
  }

  return true;
}

bool HWConformity::canSplitByteDst(G4_opcode op) {
  switch (op) {
  case G4_mac:
  case G4_mach:
  case G4_cmp:
  case G4_mad:
  case G4_sad2:
  case G4_sada2:
  case G4_line:
  case G4_send:
  case G4_sendc:
    return false;
  default:
    return true;
  }
}
// split one instruction into 2 if its dstination is packed byte and execution
// type is W. for example: add <16> V1(0,0)<1>:b V1(0,0)<16;16,1>:w
// V2(0,0)<16;16,1>:w
// ==>
// add <8> V1(0,0)<2>:b V1(0,0)<16;8,2>:w V2(0,0)<16;8,2>:w
// add <8> V1(0,1)<2>:b V1(0,1)<16;8,2>:w V2(0,1)<16;8,2>:w

// if predicate is used for instruction, the definition of this predicate is
// tracked and the corresponding instruction is checked to see if it can do the
// same split.
bool HWConformity::splitInstListForByteDst(INST_LIST_ITER it, G4_BB *bb,
                                           uint16_t extypesize) {
  G4_INST *inst = *it;
  G4_opcode inst_op = inst->opcode();
  G4_DstRegRegion *dst = inst->getDst();

  bool hasDstSrcOverlap = false;
  if (dst && !inst->hasNULLDst()) {
    auto srcNum = inst->getNumSrc();
    for (int i = 0; i < srcNum; i++) {
      G4_CmpRelation rel = dst->compareOperand(inst->getSrc(i), builder);
      if (rel != Rel_disjoint) {
        hasDstSrcOverlap = true;
        break;
      }
    }
  }

  // check if we can split the inst
  if (!canSplitByteDst(inst_op) ||
      inst->getExecSize() == g4::SIMD1 ||
      (!bb->isAllLaneActive() && !inst->isWriteEnableInst()) ||
      dst->getByteOffset() % extypesize != 0 ||
      dst->getHorzStride() != 1 ||
      extypesize != TypeSize(Type_W) ||
      inst->getPredicate() ||
      inst->getCondMod() ||
      // Do not split the instruction if dst has overlap with sources
      hasDstSrcOverlap) {
    return false;
  }

  // recursively the inst that defines its predicate can be split
  INST_LIST expandOpList;
  bool canSplit = canSplitInst(inst, NULL);
  if (canSplit) {
    expandOpList.push_back(inst);
  }

  G4_INST *currInst = inst;
  while (canSplit && currInst->getPredicate()) {
    // look for predicate def inst
    uint16_t defNum = 0;
    G4_INST *defInst = NULL;

    // FIXME: should be currInst->defInstList.begin()?
    for (auto def_iter = inst->def_begin(), end = inst->def_end();
         def_iter != end; def_iter++) {
      if ((*def_iter).second == Opnd_pred) {
        defNum++;
        defInst = (*def_iter).first;
      }
    }
    if (defNum != 1 || !defInst->getCondMod()) {
      canSplit = false;
      break;
    }
    if (canSplit) {
      if (!bb->isAllLaneActive() && !defInst->isWriteEnableInst()) {
        canSplit = false;
      } else {
        canSplit = canSplitInst(defInst, currInst);
      }
    }
    // check if def inst can be split
    if (!canSplit) {
      break;
    } else {
      expandOpList.push_back(defInst);
      currInst = defInst;
    }
  }

  // split inst into two
  INST_LIST_ITER new_iter = it;
  new_iter++;
  if (canSplit) {
    while (!expandOpList.empty()) {
      G4_INST *expand_op = expandOpList.front();
      expandOpList.pop_front();
      // find location of expand_op in instruction list
      do {
        new_iter--;
        if ((*new_iter) == expand_op) {
          break;
        }
      } while (new_iter != bb->begin());

      vISA_ASSERT(new_iter != bb->end(),
                   "Cannot find predicate definition function in BB.");
      new_iter++;
      G4_INST *secondHalfOp = splitInstWithByteDst(expand_op);
      vISA_ASSERT(secondHalfOp, "Error in splitting instruction.");
      bb->insertBefore(new_iter, secondHalfOp);
    }
  }

  return canSplit;
}

G4_INST *HWConformity::splitInstWithByteDst(G4_INST *expand_op) {
  G4_ExecSize newExecSize{expand_op->getExecSize() / 2};

  if (expand_op->getPredicate()) {
    expand_op->getPredicate()->splitPred();
  }
  if (expand_op->getCondMod()) {
    expand_op->getCondMod()->splitCondMod();
  }
  G4_INST *expand_sec_half_op = builder.createInternalInst(
      builder.duplicateOperand(expand_op->getPredicate()), expand_op->opcode(),
      builder.duplicateOperand(expand_op->getCondMod()),
      expand_op->getSaturate(), newExecSize, NULL, NULL, NULL, NULL,
      expand_op->getOption());
  vISA_ASSERT(expand_sec_half_op != NULL, ERROR_MEM_ALLOC);

  expand_op->setExecSize(newExecSize);

  if (expand_op->getDst() && !expand_op->hasNULLDst()) {
    G4_DstRegRegion *old_dst = expand_op->getDst();
    short secondSubRegOff = old_dst->getSubRegOff() + 1;

    G4_DstRegRegion *newDstOpnd = nullptr;

    if (!old_dst->isIndirect()) {
      newDstOpnd = builder.createDst(
          old_dst->getBase(), old_dst->getRegOff(), old_dst->getSubRegOff(),
          old_dst->getHorzStride() * 2, old_dst->getType());
    } else {
      newDstOpnd =
          builder.createIndirectDst(old_dst->getBase(), old_dst->getSubRegOff(),
                                    old_dst->getHorzStride() * 2,
                                    old_dst->getType(), old_dst->getAddrImm());
      secondSubRegOff -= 1;
    }

    expand_op->setDest(newDstOpnd);

    G4_DstRegRegion *secondDstOpnd = nullptr;

    if (!old_dst->isIndirect()) {
      secondDstOpnd = builder.createDst(
          old_dst->getBase(), old_dst->getRegOff(), secondSubRegOff,
          old_dst->getHorzStride() * 2, old_dst->getType());
    } else {
      secondDstOpnd = builder.createIndirectDst(
          old_dst->getBase(), secondSubRegOff, old_dst->getHorzStride() * 2,
          old_dst->getType(), old_dst->getAddrImm() + 1);
    }

    expand_sec_half_op->setDest(secondDstOpnd);
  } else {
    expand_sec_half_op->setDest(expand_op->getDst());
  }

  for (int k = 0, n_srcs = expand_op->getNumSrc(); k < n_srcs; k++) {
    G4_Operand *expand_src = expand_op->getSrc(k);

    if (!expand_src)
      continue;

    if ((expand_op->isMath() && k == 1 && expand_src->isNullReg()) ||
        expand_src->isImm()) {
      expand_sec_half_op->setSrc(expand_src, k);
    } else if (expand_src->isSrcRegRegion()) {
      G4_SrcRegRegion *expandSrcRegion = expand_src->asSrcRegRegion();

      if (expandSrcRegion->isScalar()) {
        expand_sec_half_op->setSrc(builder.duplicateOperand(expand_src), k);
      } else {
        short secondSubRegOffDiff = 0, secondAddrImmedDiff = 0;

        const RegionDesc *origRegion = expandSrcRegion->getRegion();
        const RegionDesc *newRegion = NULL;

        if (origRegion->width == 1) {
          newRegion = builder.createRegionDesc(origRegion->vertStride * 2,
                                               origRegion->width,
                                               origRegion->horzStride);
          secondSubRegOffDiff = origRegion->vertStride;
        } else {
          unsigned short newWD = origRegion->width / 2;
          secondSubRegOffDiff = origRegion->horzStride;
          newRegion = builder.createRegionDesc(
              (newWD == 1 && newExecSize == 1) ? 0 : origRegion->vertStride,
              newWD, (newWD == 1) ? 0 : origRegion->horzStride * 2);
        }
        secondAddrImmedDiff =
            (short)(secondSubRegOffDiff * expand_src->getTypeSize());
        expandSrcRegion->setRegion(builder, newRegion);

        bool directSrc = (expandSrcRegion->getRegAccess() == Direct);
        if (secondAddrImmedDiff >= (int)kernel.numEltPerGRF<Type_UB>()) {
          secondSubRegOffDiff =
              (short)((secondAddrImmedDiff - kernel.numEltPerGRF<Type_UB>()) /
                      expand_src->getTypeSize());
        }
        G4_SrcRegRegion *secondSrcOpnd = builder.createSrcRegRegion(
            expandSrcRegion->getModifier(), expandSrcRegion->getRegAccess(),
            expandSrcRegion->getBase(),
            expandSrcRegion->getRegOff() +
                ((directSrc &&
                  secondAddrImmedDiff >= (int)kernel.numEltPerGRF<Type_UB>())
                     ? 1
                     : 0),
            expandSrcRegion->getSubRegOff() +
                (directSrc ? secondSubRegOffDiff : 0),
            newRegion, expandSrcRegion->getType());
        if (expandSrcRegion->getRegAccess() != Direct) {
          secondSrcOpnd->setImmAddrOff(expandSrcRegion->getAddrImm() +
                                       secondAddrImmedDiff);
        }
        expand_sec_half_op->setSrc(secondSrcOpnd, k);
      }
    }
  }
  expand_sec_half_op->inheritDIFrom(expand_op);

  if (expand_op->getPredicate() || expand_op->getCondMod()) {
    if (expand_op->getMaskOffset() == 0) {
      expand_sec_half_op->setMaskOption(InstOpt_M8);
    } else if (expand_op->getMaskOffset() == 16) {
      expand_sec_half_op->setMaskOption(InstOpt_M24);
    } else if (!(expand_op->opcode() == G4_sel &&
                 !(expand_op->getPredicate()) && expand_op->getCondMod())) {
      expand_sec_half_op->setMaskOption(newExecSize > 8 ? InstOpt_M16
                                                        : InstOpt_M8);
    }
  }
  return expand_sec_half_op;
}

//  in addition, fix the source region to follow the region restriction:
//  1. ExecSize must be greater than or equal to Width.  -- no check for this
//  one
//  2. If ExecSize = Width and HorzStride ? 0, VertStride must be set to Width *
//  HorzStride.
//  3. If ExecSize = Width and HorzStride = 0, there is no restriction on
//  VertStride.
//  4. If Width = 1, HorzStride must be 0 regardless of the values of ExecSize
//  and VertStride.
//  5. If ExecSize = Width = 1, both VertStride and HorzStride must be 0. This
//  defines a scalar.
//  6. If VertStride = HorzStride = 0, Width must be 1 regardless of the value
//  of ExecSize.
//  7. Dst.HorzStride must not be 0.        -- this needs not to be checked.
//  8. VertStride must be used to cross GRF register boundaries. This rule
//  implies that
//      elements within a 'Width' cannot cross GRF boundaries.
void HWConformity::fixSrcRegion(G4_INST *inst) {
  bool comprInst = isCompressedInst(inst);
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion() &&
        !inst->getSrc(i)->isNullReg()) {
      G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
      const RegionDesc *srcRegion = src->getRegion();
      if (srcRegion->isRegionWH() || srcRegion->isRegionV() ||
          srcRegion->isRegionSW()) {
        // normalize VxH regions if possible
        if (srcRegion->isRegionWH() &&
            srcRegion->width == inst->getExecSize()) {
          // r[a0.0]<E, S> -> r[a0.0]<S;1,0>
          src->setRegion(builder,
                         builder.createRegionDesc(srcRegion->horzStride, 1, 0));
        }
        // ToDo: add other legalization
        continue;
      }

      // ToDo: most of these checks should be obsolete at this point
      uint16_t vs = srcRegion->vertStride, wd = srcRegion->width,
               hs = srcRegion->horzStride;
      uint8_t exSize = inst->getExecSize();
      vISA_ASSERT(inst->isSend() || exSize >= wd,
                   " Bad source region: Width is greater than execution size.");
      if (comprInst) {
        if (inst->getSrc(i)->getTypeSize() > G4_WSIZE && wd == exSize &&
            vs == wd && hs == 1) {
          vs = wd = exSize / 2;
        }
      }
      if (wd == exSize && hs != 0 && vs != wd * hs) {
        // <V;E,H> --> <V*H;E,H>
        vs = wd * hs;
      }
      if (wd == 1) {
        // <V;1,H> -> <V;1,0> or <0;1,0>
        hs = 0;
        if (1 == exSize)
          vs = 0;
      }
      if (vs == 0 && hs == 0) {
        // <0;N,0> -> <0;1,0>
        wd = 1;
      }
      if (hs == 0 && ((inst->getSrc(i)->getTypeSize() == G4_WSIZE &&
                       exSize == 32 && vs == 32 && wd == 32) ||
                      (inst->getSrc(i)->getTypeSize() == G4_DSIZE &&
                       exSize == 16 && vs == 16 && wd == 16))) {
        vs = 0;
        wd = 1;
      }

      // check cross GRF (rule 2H)
      // TODO! for the following two cases, split the instruction:
      // source region is like <8;4,1>
      // source region is like <2;4,1>
      if (src->getRegAccess() == Direct && src->crossGRF(builder) && hs != 0) {
        // TODO: this is a temp fix
        if ((builder.getPlatform() == GENX_BDW ||
             builder.getPlatform() == GENX_CHV) &&
            vs < wd * hs)
          continue;
        // check number of elements in first GRF.
        uint16_t execTypeSize = hs * src->getElemSize();
        uint16_t sizeInFirstGRF =
            kernel.numEltPerGRF<Type_UB>() -
            src->getLeftBound() % kernel.numEltPerGRF<Type_UB>();
        uint16_t vertSize = vs * src->getTypeSize();
        uint16_t numEle = (sizeInFirstGRF + execTypeSize - 1) / execTypeSize;
        uint16_t rowSize = wd * execTypeSize;

        if (sizeInFirstGRF <= vertSize) {
          if (numEle >= wd) {
            numEle = wd;
          }
        } else if (vs > wd) {
          numEle = sizeInFirstGRF / vertSize * wd +
                   ((sizeInFirstGRF % vertSize > rowSize)
                        ? wd
                        : (sizeInFirstGRF % vertSize + execTypeSize - 1) /
                              execTypeSize);
        }
        // wd is used to cross GRF, change to <vs;1,0>
        if (numEle < wd || (wd >= vs && numEle % wd != 0)) {

          wd = 1;
          vs = hs;
          hs = 0;
        }
      }

      if (vs != srcRegion->vertStride || wd != srcRegion->width ||
          hs != srcRegion->horzStride) {
        G4_SrcRegRegion *origSrc = inst->getSrc(i)->asSrcRegRegion();
        origSrc->setRegion(builder, builder.createRegionDesc(vs, wd, hs));
      }
    }
  }
  if (inst->getDst() && !inst->hasNULLDst()) {
    vISA_ASSERT(inst->getDst()->getHorzStride() != 0,
                 "Bad source region: Width is greater than execution size.");
  }
}

bool HWConformity::fixOddAlignSrc1Region(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;

  if (inst->getDst() && !inst->hasNULLDst()) {
    G4_Operand *src1 = inst->getSrc(1);
    if (src1 && src1->isSrcRegRegion()) {
      G4_Operand *dst = inst->getDst();
      if (dst && dst->isDstRegRegion() &&
          dst->asDstRegRegion()->getHorzStride() > 2 &&
          (dst->getType() == Type_B || dst->getType() == Type_UB) &&
          (src1->getType() == Type_W || src1->getType() == Type_UW) &&
          (src1->asSrcRegRegion()->getSubRegOff() % 2)) {
        G4_Operand *new_src1 =
            insertMovBefore(i, 1, src1->getType(), bb, Even_Word);
        inst->setSrc(new_src1, 1);
        return true;
      }
    }
  }
  return false;
}

//
// single entry point for HW conformity checks
//
void HWConformityChk(IR_Builder &builder, G4_Kernel &kernel) {
  if (builder.getPlatform() > Xe3) {
    PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
    p.doPointsToAnalysis(kernel.fg);
    HWConformityPro conformityPro(builder, p);
    conformityPro.checkHWConformity();
    return;
  }
  HWConformity conformity(builder, kernel);
  conformity.chkHWConformity();
}

bool HWConformity::markPackedByteReference(G4_Kernel &kernel, G4_Operand *opnd,
                                           G4_INST *inst) {
  G4_Declare *dcl = NULL, *topdcl = NULL;
  bool foundOptCandidate = false;

  if ((opnd->isSrcRegRegion() || opnd->isDstRegRegion())) {
    if (opnd->getBase() && opnd->getBase()->isRegVar()) {
      dcl = opnd->getBase()->asRegVar()->getDeclare();
      topdcl = dcl->getRootDeclare();
    }
  }

  if (topdcl != NULL && topdcl->getRegFile() == G4_GRF &&
      !(topdcl->getAddressed())) {
    if (topdcl->doNotWiden() || inst->isSend() || inst->isDpas()) {
      // send has no regioning so it is certainly illegal to change data layout
      setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
      return false;
    }

    if (opnd->isDstRegRegion() &&
        // check if the opnd has pre-assigned physical register
        !(topdcl->getRegVar()->isPhyRegAssigned()) &&
        // check if the opnd is global
        !(kernel.fg.globalOpndHT.isOpndGlobal(opnd)) &&
        // check if the opnd is used as packed byte
        opnd->getTypeSize() == 1 && !hasDedicateAlignRegionConformity(inst) &&
        dcl->getElemSize() == 1 &&
        opnd->asDstRegRegion()->getHorzStride() == 1 &&
        // check if the instruction is a raw mov
        !inst->isRawMov() &&
        // check if the instruction execution type is word
        // (This should be the most common case that can benefit
        //  from this optimization. It could be extended to other
        //  cases like D execution type).
        TypeSize(inst->getExecType()) == 2) {
      unsigned int leftBound = opnd->asDstRegRegion()->getLeftBound();
      unsigned int rightBound = opnd->asDstRegRegion()->getRightBound();

      if (((rightBound * 2 / kernel.numEltPerGRF<Type_UB>() -
            leftBound * 2 / kernel.numEltPerGRF<Type_UB>()) > 1) ||
          (builder.getPlatform() == GENX_BDW &&
           (rightBound * 2 / kernel.numEltPerGRF<Type_UB>() !=
            leftBound * 2 / kernel.numEltPerGRF<Type_UB>()))) {
        setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
      } else if (getAccessPattern(topdcl) == ACCESS_PATTERN_UNDEF) {
        setAccessPattern(topdcl, ACCESS_PATTERN_PACKED_BYTE);
        foundOptCandidate = true;
      }
    } else if (opnd->isSrcRegRegion() &&
               // check if the opnd has pre-assigned physical register
               !(opnd->asSrcRegRegion()
                     ->getBase()
                     ->asRegVar()
                     ->isPhyRegAssigned()) &&
               // check if the opnd is global
               !(kernel.fg.globalOpndHT.isOpndGlobal(opnd)) &&
               // check if the opnd is used as packed byte
               opnd->getTypeSize() == 1 && dcl->getElemSize() == 1 &&
               opnd->asSrcRegRegion()->getRegion()->isContiguous(
                   inst->getExecSize())) {
      unsigned int leftBound = opnd->asSrcRegRegion()->getLeftBound();
      unsigned int rightBound = opnd->asSrcRegRegion()->getRightBound();

      if (((rightBound * 2 / kernel.numEltPerGRF<Type_UB>() -
            leftBound * 2 / kernel.numEltPerGRF<Type_UB>()) > 1) ||
          (builder.getPlatform() == GENX_BDW &&
           (rightBound * 2 / kernel.numEltPerGRF<Type_UB>() !=
            leftBound * 2 / kernel.numEltPerGRF<Type_UB>()))) {
        setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
      }
    } else {
      setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
    }
  }

  return foundOptCandidate;
}

G4_Operand *HWConformity::fixPackedByteReference(IR_Builder &builder,
                                                 G4_Operand *opnd) {
  G4_Operand *newOpnd = NULL;
  G4_Declare *topdcl = NULL;

  if (opnd->isDstRegRegion() || opnd->isSrcRegRegion()) {
    topdcl = GetTopDclFromRegRegion(opnd);
  }

  if (topdcl != NULL &&
      getAccessPattern(topdcl) == ACCESS_PATTERN_PACKED_BYTE) {
    if (opnd->isDstRegRegion()) {
      short dst_regoff = opnd->asDstRegRegion()->getRegOff();
      short dst_subregoff = opnd->asDstRegRegion()->getSubRegOff();
      short off =
          (dst_regoff * kernel.numEltPerGRF<Type_UB>() + dst_subregoff) * 2;

      dst_regoff = off / kernel.numEltPerGRF<Type_UB>();
      dst_subregoff = off % kernel.numEltPerGRF<Type_UB>();

      G4_DstRegRegion *newDstOpnd =
          builder.createDst(opnd->getBase()->asRegVar(), dst_regoff,
                            dst_subregoff, 2, opnd->getType());
      newOpnd = newDstOpnd;
    } else if (opnd->isSrcRegRegion()) {
      short src_regoff = opnd->asSrcRegRegion()->getRegOff();
      short src_subregoff = opnd->asSrcRegRegion()->getSubRegOff();
      short off =
          (src_regoff * kernel.numEltPerGRF<Type_UB>() + src_subregoff) * 2;

      src_regoff = off / kernel.numEltPerGRF<Type_UB>();
      src_subregoff = off % kernel.numEltPerGRF<Type_UB>();

      const RegionDesc *rd = builder.getRegionStride2();
      G4_SrcRegRegion *newSrcOpnd = builder.createSrcRegRegion(
          opnd->asSrcRegRegion()->getModifier(), Direct,
          opnd->getBase()->asRegVar(), src_regoff, src_subregoff, rd,
          opnd->getType());
      newOpnd = newSrcOpnd;
    }
  }

  return newOpnd;
}

void HWConformity::fixDataLayout() {
  bool changeDataLayout = false;

  for (auto &bb : kernel.fg) {
    for (auto &inst : *bb) {
      if (G4_Inst_Table[inst->opcode()].n_dst == 1) {
        G4_Operand *dst = inst->getDst();

        if (dst) {
          bool foundOptCandidate = markPackedByteReference(kernel, dst, inst);
          if (changeDataLayout == false && foundOptCandidate) {
            changeDataLayout = true;
          }
        }
      }

      for (int i = 0; i < inst->getNumSrc(); i++) {
        G4_Operand *src = inst->getSrc(i);

        if (src) {
          markPackedByteReference(kernel, src, inst);
        }
      }
    }
  }

  if (changeDataLayout) {
    for (auto &dcl : kernel.Declares) {
      G4_Declare *topdcl = dcl->getRootDeclare();

      if (getAccessPattern(topdcl) == ACCESS_PATTERN_PACKED_BYTE) {
        dcl->setTotalElems(dcl->getTotalElems() * 2);

        if (dcl != topdcl) {
          G4_Declare *aliasDcl = dcl->getAliasDeclare();
          unsigned int aliasOffset = dcl->getAliasOffset();
          dcl->setAliasDeclare(aliasDcl, aliasOffset * 2);
        }
      }
    }

    for (auto &bb : kernel.fg) {
      for (auto &inst : *bb) {
        if (G4_Inst_Table[inst->opcode()].n_dst == 1) {
          G4_Operand *dst = inst->getDst();
          G4_Operand *newDst = NULL;

          if (dst) {
            newDst = fixPackedByteReference(builder, dst);
            if (newDst) {
              inst->setDest(newDst->asDstRegRegion());
            }
          }
        }

        for (int i = 0; i < inst->getNumSrc(); i++) {
          G4_Operand *src = inst->getSrc(i);
          G4_Operand *newSrc = NULL;

          if (src) {
            newSrc = fixPackedByteReference(builder, src);
            if (newSrc) {
              inst->setSrc(newSrc, i);
            }
          }
        }
      }
    }
  }
}

// maintain def-use chain for current inst and the MOV inst generated for its
// dst
void HWConformity::maintainDU4TempMov(G4_INST *inst, G4_INST *newInst) {
  if (newInst->getPredicate()) {
    inst->transferDef(newInst, Opnd_pred, Opnd_pred);
  }

  inst->transferUse(newInst);

  inst->addDefUse(newInst, Opnd_src0);
}

static void expandPlaneMacro(IR_Builder &builder, INST_LIST_ITER it, G4_BB *bb,
                             bool secondHalf) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
  G4_SrcRegRegion *src1 = inst->getSrc(1)->asSrcRegRegion();

  G4_SrcRegRegion *srcP = builder.createSrcRegRegion(
      src0->getModifier(), Direct, src0->getBase(), src0->getRegOff(),
      src0->getSubRegOff(), builder.getRegionScalar(), src0->getType());
  G4_SrcRegRegion *srcQ = builder.createSrcRegRegion(
      src0->getModifier(), Direct, src0->getBase(), src0->getRegOff(),
      src0->getSubRegOff() + 1, builder.getRegionScalar(), src0->getType());
  G4_SrcRegRegion *srcR = builder.createSrcRegRegion(
      src0->getModifier(), Direct, src0->getBase(), src0->getRegOff(),
      src0->getSubRegOff() + 3, builder.getRegionScalar(), src0->getType());

  auto u = builder.createSrcWithNewRegOff(src1, src1->getRegOff() +
                                                    (secondHalf ? 2 : 0));
  auto v = builder.createSrcWithNewRegOff(src1, src1->getRegOff() +
                                                    (secondHalf ? 3 : 1));
  if (builder.getGRFSize() == 64) {
    u = builder.createSrcRegRegion(src1->getModifier(), Direct, src1->getBase(),
                                   src1->getRegOff() + (secondHalf ? 1 : 0),
                                   src1->getSubRegOff(), src1->getRegion(),
                                   src1->getType(), src1->getAccRegSel());
    v = builder.createSrcRegRegion(src1->getModifier(), Direct, src1->getBase(),
                                   src1->getRegOff() + (secondHalf ? 1 : 0),
                                   src1->getSubRegOff() + 8, src1->getRegion(),
                                   src1->getType(), src1->getAccRegSel());
  }

  uint32_t options = inst->getOption();
  if (inst->getExecSize() == g4::SIMD16) {
    options &= ~InstOpt_QuarterMasks;
    int maskOffset = inst->getMaskOffset() + (secondHalf ? 8 : 0);
    switch (maskOffset) {
    case 0:
      options |= InstOpt_M0;
      break;
    case 8:
      options |= InstOpt_M8;
      break;
    case 16:
      options |= InstOpt_M16;
      break;
    case 24:
      options |= InstOpt_M24;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected offset value");
    }
  }

  G4_Declare *tmpVal =
      builder.hasNFType() ? nullptr : builder.createTempVar(8, Type_F, Any);
  G4_DstRegRegion *accDst =
      builder.hasNFType()
          ? builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, Type_NF)
          : builder.createDstRegRegion(tmpVal, 1);
  G4_INST *madInst = builder.createInternalInst(
      nullptr, G4_mad, nullptr, g4::NOSAT, g4::SIMD8, accDst, srcR, u, srcP,
      options | InstOpt_WriteEnable);
  bb->insertBefore(it, madInst);

  G4_Predicate *pred = inst->getPredicate()
                           ? builder.duplicateOperand(inst->getPredicate())
                           : nullptr;
  G4_CondMod *condMod = inst->getCondMod()
                            ? builder.duplicateOperand(inst->getCondMod())
                            : nullptr;
  G4_SrcRegRegion *accSrc =
      builder.hasNFType()
          ? builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                              builder.getRegionStride1(), Type_NF)
          : builder.createSrcRegRegion(tmpVal, builder.getRegionStride1());
  G4_DstRegRegion *newDst = builder.createDst(
      dst->getBase(), dst->getRegOff() + (secondHalf ? 1 : 0),
      dst->getSubRegOff(), dst->getHorzStride(), dst->getType());
  if (builder.getGRFSize() == 64) {
    newDst = builder.createDst(dst->getBase(), dst->getRegOff(),
                               dst->getSubRegOff() + (secondHalf ? 8 : 0),
                               dst->getHorzStride(), dst->getType());
  }
  G4_INST *secondMadInst =
      builder.createInternalInst(pred, G4_mad, condMod, inst->getSaturate(),
                                 g4::SIMD8, newDst, accSrc, v, srcQ, options);
  bb->insertBefore(it, secondMadInst);
}

// Replace plane with a macro sequence:
// pln dest:f src0:f src1:f
// -->
// mad acc0:nf src0.3:f src1:f src0.0:f
// mad dest:f acc0:nf src1+1:f src0.1:f
// simd16 pln also needs to be split as the macro is simd8 only

void HWConformity::expandPlaneInst(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_pln, "expect a plane inst");
  vISA_ASSERT(inst->getSrc(0)->isSrcRegRegion(),
               "src0 must be source reg region");

  G4_DstRegRegion *dst = inst->getDst();
  if (dst->getRegAccess() == IndirGRF || dst->getHorzStride() > 1) {
    inst->setDest(insertMovAfter(it, dst, dst->getType(), bb));
  }
  G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
  if (src0->getRegAccess() == IndirGRF) {
    // insert move to make src0 direct
    inst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
  }
  G4_SrcRegRegion *src1 = inst->getSrc(1)->asSrcRegRegion();
  if (src1->getRegAccess() == IndirGRF) {
    // insert move to make src1 direct
    inst->setSrc(insertMovBefore(it, 1, src1->getType(), bb), 1);
  }

  expandPlaneMacro(builder, it, bb, false);
  if (inst->getExecSize() == g4::SIMD16) {
    expandPlaneMacro(builder, it, bb, true);
  }

  it = bb->erase(it);
}

// plane does not support pln with non-packed dst.
// also fix up plane sources, which don't support modifiers
// returns true if the original plane is deleted
bool HWConformity::fixPlaneInst(INST_LIST_ITER it, G4_BB *bb) {

  G4_INST *inst = *it;
  if (inst->opcode() == G4_pln) {
    if (!builder.doPlane()) {
      expandPlaneInst(it, bb);
      return true;
    }
    G4_DstRegRegion *dst = inst->getDst();
    if (dst->getHorzStride() != 1) {
      G4_DstRegRegion *newDst = insertMovAfter(it, dst, dst->getType(), bb);
      inst->setDest(newDst);
    }

    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);

    // Source modifiers are not supported for pln instruction
    if (src0 && ((src0->isSrcRegRegion() &&
                  src0->asSrcRegRegion()->getModifier() != Mod_src_undef) ||
                 !builder.tryToAlignOperand(src0, 16))) {
      // src0 needs a temp
      G4_Declare *tmpDcl =
          builder.createTempVar(4, Type_F, builder.getGRFAlign());

      // Before:
      // pln (16) dst, (mod)src0, src1
      //
      // After:
      // mov (4) tmp(0,0):f (mod)src0(r)<4;4,1>:f
      // pln (16) dst, tmp(0,0)<0;1,0>, src1
      G4_DstRegRegion *dstRgn =
          builder.createDst(tmpDcl->getRegVar(), 0, 0, 1, Type_F);

      const RegionDesc *rd = builder.createRegionDesc(4, 4, 1);
      G4_SrcRegRegion *srcRgn = builder.createSrcRegRegion(
          src0->asSrcRegRegion()->getModifier(), Direct,
          src0->asSrcRegRegion()->getBase(),
          src0->asSrcRegRegion()->getRegOff(),
          src0->asSrcRegRegion()->getSubRegOff(), rd, Type_F);

      G4_INST *newInst =
          builder.createMov(g4::SIMD4, dstRgn, srcRgn, InstOpt_NoOpt, false);

      bb->insertBefore(it, newInst);

      rd = builder.createRegionDesc(0, 4, 1);
      G4_SrcRegRegion *newSrcRgn =
          builder.createSrc(tmpDcl->getRegVar(), 0, 0, rd, Type_F);

      inst->setSrc(newSrcRgn, 0);
      inst->transferDef(newInst, Opnd_src0, Opnd_src0);
      newInst->addDefUse(inst, Opnd_src0);
    }

    if (src1 && src1->isSrcRegRegion() &&
        src1->asSrcRegRegion()->getModifier() != Mod_src_undef) {
      // src1 needs a temp
      // For pln instruction src2 is implied from src1 and exec_size
      // When exec_size = 8, src2 is 1 GRF after src1 with size = 1 GRF
      // When exec_size = 16, src2 is 2 GRFs after src1 with size = 2 GRFs
      unsigned short numGRFsToCopy = inst->getExecSize() == g4::SIMD8 ? 2 : 4;

      G4_Declare *tmpDcl = builder.createTempVar(
          (unsigned short)(kernel.numEltPerGRF<Type_UB>() / TypeSize(Type_F) *
                           numGRFsToCopy),
          Type_F, Any);

      // Before:
      // pln (16) dst, src0, (mod)src1
      //
      // After:
      // mov (16) tmp(0,0):f (mod)src1(r)<8;8,1>:f
      // mov (16) tmp(2,0):f (mod)src1(r+2)<8;8,1>:f <-- only if exec_size = 16
      // pln (16) dst, src0, tmp(0,0)
      for (int i = 0; i < numGRFsToCopy; i += 2) {
        G4_DstRegRegion *dstRgn =
            builder.createDst(tmpDcl->getRegVar(), (short)i, 0, 1, Type_F);

        const RegionDesc *rd = builder.createRegionDesc(8, 8, 1);
        G4_SrcRegRegion *srcRgn = builder.createSrcRegRegion(
            src1->asSrcRegRegion()->getModifier(), Direct,
            src1->asSrcRegRegion()->getBase(),
            src1->asSrcRegRegion()->getRegOff() + i, 0, rd, Type_F);

        G4_INST *newInst =
            builder.createMov(g4::SIMD16, dstRgn, srcRgn, InstOpt_NoOpt, false);

        bb->insertBefore(it, newInst);

        if (i == 0) {
          G4_SrcRegRegion *newSrcRgn =
              builder.createSrc(tmpDcl->getRegVar(), 0, 0, rd, Type_F);

          inst->setSrc(newSrcRgn, 1);
          inst->transferDef(newInst, Opnd_src1, Opnd_src0);
        }
        newInst->addDefUse(inst, Opnd_src1);
      }
    }
  }
  return false;
}

void HWConformity::fixImm64(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->isSplitSend()) {
    return;
  }
  for (int j = 0, n_srcs = inst->getNumSrc(); j < n_srcs; j++) {
    G4_Operand *src = inst->getSrc(j);
    if (!src || !(src->isImm()) || src->getTypeSize() != 8) {
      continue;
    }
    // a 64bit immediate is supported ONLY for a MOV operation
    bool needsSplit = false;

    if (VISA_WA_CHECK(builder.getPWaTable(), WaDisallow64BitImmMov)) {
      needsSplit = true;
    }
    if (needsSplit) {
      char *immPtr = NULL;
      double dfValue = 0.0f;
      int64_t qValue = 0;

      if (IS_DFTYPE(src->getType())) {
        dfValue = src->asImm()->getDouble();
        immPtr = (char *)&dfValue;
      } else {
        qValue = src->asImm()->getInt();
        immPtr = (char *)&qValue;
      }
      unsigned int lowValue = *((unsigned int *)(immPtr));
      unsigned int highValue = *((unsigned int *)(immPtr + 4));
      G4_Imm *lowImm = builder.createImm((int64_t)lowValue, Type_UD);
      G4_Imm *highImm = builder.createImm((int64_t)highValue, Type_UD);

      G4_Declare *defDcl = NULL;

      defDcl = builder.createTempVar(1, src->getType(), Eight_Word);
      G4_Declare *dcl = builder.createTempVar(2, Type_UD, Eight_Word);
      dcl->setAliasDeclare(defDcl, 0);

      G4_DstRegRegion *dstRegion = builder.createDstRegRegion(dcl, 1);
      G4_INST *lowMovInst = builder.createMov(g4::SIMD1, dstRegion, lowImm,
                                              InstOpt_WriteEnable, false);

      bb->insertBefore(i, lowMovInst);

      auto newDst =
          builder.createDst(dcl->getRegVar(), 0, 1, 1, dcl->getElemType());
      G4_INST *highMovInst = builder.createMov(g4::SIMD1, newDst, highImm,
                                               InstOpt_WriteEnable, false);
      bb->insertBefore(i, highMovInst);

      inst->transferDef(lowMovInst, Gen4_Operand_Number(j + 1), Opnd_src0);
      lowMovInst->addDefUse(inst, Gen4_Operand_Number(j + 1));
      inst->transferDef(highMovInst, Gen4_Operand_Number(j + 1), Opnd_src0);
      highMovInst->addDefUse(inst, Gen4_Operand_Number(j + 1));

      unsigned short vs = 0, hs = 0, wd = 1; // gen7_5: always 0;1,0
      G4_SrcRegRegion *new_src = builder.createSrcRegRegion(
          defDcl, builder.createRegionDesc(vs, wd, hs));
      inst->setSrc(new_src, j);
    } else {
      if (inst->opcode() != G4_mov) {
        inst->setSrc(insertMovBefore(i, j, src->getType(), bb), j);
      }
    }
  }
}

// Check if the source of def_inst is redefined before inst
G4_INST *HWConformity::checkSrcDefInst(G4_INST *inst, G4_INST *def_inst,
                                       uint32_t srcNum) {
  G4_INST *valid_inst = def_inst;

  if (def_inst != NULL) {
    vISA_ASSERT(def_inst->opcode() == G4_mov,
                 "def inst must be a mov instruction");

    G4_INST *def_inst1 = NULL;
    for (auto def_it1 = inst->def_begin(), end = inst->def_end();
         def_it1 != end; def_it1++) {
      if ((*def_it1).second == srcNum + 1) {
        def_inst1 = (*def_it1).first;
      }
    }

    if (def_inst1 != NULL) {
      G4_INST *def_inst2 = NULL;
      for (auto def_it2 = def_inst->def_begin(), end2 = def_inst->def_end();
           def_it2 != end2; def_it2++) {
        if ((*def_it2).second == Opnd_src0) {
          def_inst2 = (*def_it2).first;
        }
      }

      if (def_inst1 != def_inst2) {
        valid_inst = NULL;
      }
    }
  }

  return valid_inst;
}

/*
    Helper function for fixMixedHFInst
    It assumes dst is not null and is of type DstRegRegion.
    This check must be done before this method is called.
*/
void HWConformity::helperGenerateTempDst(G4_BB *bb, INST_LIST_ITER instIter,
                                         G4_INST *inst, uint8_t hStride,
                                         G4_Type tempDstType,
                                         G4_SubReg_Align subAlign) {
  G4_DstRegRegion *dst = inst->getDst();
  G4_ExecSize execSize = inst->getExecSize();
  uint8_t dstSize = execSize * TypeSize(tempDstType);
  // create a new temp with horizontal stride of 1 (packed)
  // create a move to dst.

  uint32_t numElt = execSize == 1 ? 1 : execSize * hStride;
  subAlign = getDclAlignment(dstSize, inst, execSize == 1);

  G4_Declare *dcl = builder.createTempVar(numElt, tempDstType, subAlign);

  G4_DstRegRegion *dstRegion = builder.createDstRegRegion(dcl, hStride);
  inst->setDest(dstRegion);

  const RegionDesc *region =
      execSize == 1
          ? builder.getRegionScalar()
          : builder.createRegionDesc(execSize * hStride, execSize, hStride);
  G4_SrcRegRegion *srcRegion = builder.createSrcRegRegion(dcl, region);

  // creating a mov from temp dst to final destination using original options of
  // fixed instruction
  G4_INST *movInst =
      builder.createMov(execSize, dst, srcRegion, inst->getMaskOption(), false);

  ++instIter;
  // inserting mov after fixed instruction
  bb->insertBefore(instIter, movInst);

  /*
  Need to remove dst from uses list of mulh, and add them to movInst useList
  add movInst to uselist of mulh.
  Add mulh to def instruction list of movInst
  */
  inst->transferUse(movInst);
  inst->addDefUse(movInst, Opnd_src0);
}

/*
    Not Implemented rules:

    3:  (Does this mean align1 doesn't support replication?)
        In Align16 mode, replicate is supported and is coissueable.

    4: (handled in reduce execution size)
        No simd16 in mixed mode when destination is packed f16 for both Align1
   and Align16.

            mad(8) r3.xyzw:hf r4.xyzw:f r6.xyzw:hf r7.xyzw:hf

            add(8) r20.0<1>:hf r3<8;8,1>:f r6.0<8;8,1>:hf {Q1}

    5: (we are not producing this type of code)
        No accumulator read access for align16 mixed float

    6: (we do not generate code like this)
        [DevCHV, DevSKL+]: When source is float from accumulator register and
   destination is half float with a stride of 1, the source must register
   aligned. i.e., source must have offset zero.

    7: (doesn't seem like it is applicable to our code)
        In Align16, vertical stride can never be zero for f16

    8.a: (handled by another check)
        Math operations for mixed mode,
            - In Align16, only packed format is supported

    11. (handled in reduce execution size)
        [DevCHV, DevSKL, DevBXT]: No simd16 in mixed mode when destination is
   f32. Instruction Execution size must be no more than 8.

*/
void HWConformity::fixMixedHFInst(G4_BB *bb) {
  for (auto instIter = bb->begin(); instIter != bb->end(); ++instIter) {
    G4_INST *inst = *instIter;

    if (inst->isSend() || inst->isDpas() || !inst->getDst()) {
      continue;
    }

    if (VISA_WA_CHECK(builder.getPWaTable(), WaSrc1ImmHfNotAllowed)) {
      G4_Operand *tSrc1 = inst->getSrc(1);
      if (tSrc1 && tSrc1->isImm() && tSrc1->getType() == Type_HF) {
        inst->setSrc(insertMovBefore(instIter, 1, Type_HF, bb), 1);
      }
    }

    if (builder.hasPartialMixMode() && inst->getNumSrc() > 1) {
      // Skip G4_srnd that allows F->HF and HF->BF8 conversions.
      if (inst->opcode() != G4_srnd && inst->isIllegalMixedMode()) {
        // Convert the unsupported low precision float types to Type_F.
        if (isLowPrecisionFloatTy(inst->getDst()->getType()) &&
            inst->getDst()->getType() != builder.getMixModeType()) {
          inst->setDest(insertMovAfter(instIter, inst->getDst(), Type_F, bb));
        }
        for (int i = 0; i < inst->getNumSrc(); ++i) {
          if (isLowPrecisionFloatTy(inst->getSrc(i)->getType()) &&
              inst->getSrc(i)->getType() != builder.getMixModeType()) {
            inst->setSrc(insertMovBefore(instIter, i, Type_F, bb), i);
          }
        }
      }

      // no HF on mad src2 or mul src1
      if (inst->isMixedMode()) {
        auto canSwapSource = [](G4_INST *inst) {
          int srcPos = inst->opcode() == G4_mad ? 2 : 1;
          G4_Operand *src = inst->getSrc(srcPos);
          G4_Operand *otherSrc = inst->getSrc(srcPos - 1);
          if (src->isImm() || otherSrc->getType() != Type_F) {
            // swapping won't work
            return false;
          }
          if (inst->opcode() == G4_mad) {
            // src2 has more restrictive regioning, so we can swap only when
            // src1 is scalar or has contiguous region
            if (otherSrc->isSrcRegRegion()) {
              G4_SrcRegRegion *other = otherSrc->asSrcRegRegion();
              if (other->getRegion()->isScalar() ||
                  other->getRegion()->isContiguous(inst->getExecSize())) {
                return true;
              }
            }
            return false;
          } else {
            // swapping is always legal for mul
            return true;
          }
        };
        if (inst->opcode() == G4_mad) {
          if (isLowPrecisionFloatTy(inst->getSrc(2)->getType())) {
            if (canSwapSource(inst)) {
              inst->swapSrc(1, 2);
            } else {
              inst->setSrc(insertMovBefore(instIter, 2, Type_F, bb), 2);
            }
          }
          // at this point src2 must be F. Dst must be aligned to
          // same subreg as src2 if src2 is non-scalar
          bool nonScalarSrc2 =
              inst->getSrc(2)->isSrcRegRegion() &&
              !inst->getSrc(2)->asSrcRegRegion()->getRegion()->isScalar();
          if (nonScalarSrc2) {
            if (!builder.tryToAlignOperand(inst->getDst(),
                                       kernel.numEltPerGRF<Type_UB>())) {
              replaceDst(instIter, Type_F, builder.getGRFAlign());
            }
            if (!builder.tryToAlignOperand(inst->getSrc(2),
                                       kernel.numEltPerGRF<Type_UB>())) {
              inst->setSrc(insertMovBefore(instIter, 2,
                                           inst->getSrc(2)->getType(), bb,
                                           builder.getGRFAlign()),
                           2);
            }
          }
        } else if (inst->opcode() == G4_mul &&
                   isLowPrecisionFloatTy(inst->getSrc(1)->getType())) {
          if (canSwapSource(inst)) {
            inst->swapSrc(0, 1);
          } else {
            inst->setSrc(insertMovBefore(instIter, 1, Type_F, bb), 1);
          }
        }
      }
    }

    // The execution size must be no more than 8 when half-floats are used in
    // source or destination operand. ToDO: move this to fixmathinst
    if (inst->getExecSize() > builder.getNativeExecSize()) {
      if (inst->opcode() == G4_math && inst->getDst()->getType() == Type_HF &&
          inst->getSrc(0)->getType() == Type_HF &&
          (!inst->getSrc(1) || inst->getSrc(1)->getType() == Type_HF)) {
        evenlySplitInst(instIter, bb);
      }
    }

    G4_DstRegRegion *dst = inst->getDst();
    if (INST_FLOAT_SRC_ONLY(inst->opcode()) && dst && !dst->isNullReg() &&
        isLowPrecisionFloatTy(dst->getType())) {
      helperGenerateTempDst(bb, instIter, inst, 1, Type_F);
    }

    if (!inst->isMixedMode())
      continue;

    if (inst->getDst() && !inst->getDst()->isNullReg())
      dst = inst->getDst();

    if ((VISA_WA_CHECK(builder.getPWaTable(), WaMixModeSelInstDstNotPacked) ||
         VISA_WA_CHECK(builder.getPWaTable(),
                       WaFloatMixedModeSelNotAllowedWithPackedDestination)) &&
        inst->opcode() == G4_sel && dst &&
        (VISA_WA_CHECK(builder.getPWaTable(), WaMixModeSelInstDstNotPacked) ||
         dst->getHorzStride() == 1) &&
        dst->getType() == Type_HF) {
      helperGenerateTempDst(bb, instIter, inst, 1, Type_F);
    }

    if (!inst->isMixedMode())
      continue;

    if (builder.getPlatform() >= GENX_CHV) {
      // no SIMD16 mix mode instruction
      if (inst->getExecSize() > builder.getNativeExecSize() &&
          inst->isMixedMode() && !builder.enableMovCvt()) {
        evenlySplitInst(instIter, bb, false);
        // instruction was split, and new instruction inserted before
        // going back to previous instruction to double check it still confirms.
        --instIter;
        inst = *instIter;
      }
    }

    /*
        12: [DevCHV, DevSKL]: Indirect Addressing on source is not supported
       when source and destination data types are mixed float.
    */
    if (builder.getPlatform() == GENX_CHV ||
        builder.getPlatform() == GENX_SKL) {
      for (uint8_t i = 0; i < inst->getNumSrc(); ++i) {
        G4_Operand *src = inst->getSrc(i);
        if (src == nullptr || !src->isSrcRegRegion() ||
            !src->asSrcRegRegion()->isIndirect()) {
          continue;
        }
        inst->setSrc(insertMovBefore(instIter, i, src->getType(), bb), i);
      }
    }

    if (inst->getDst()->getBase()->isRegVar() &&
        inst->getDst()->getType() == Type_HF &&
        inst->getDst()->getHorzStride() == 1) {
      inst->getDst()->getBase()->asRegVar()->getDeclare()->setSubRegAlign(
          Eight_Word);
    }
  }
}

// Fix for packed half types on BDW.
// Conversions from F to packed HF are not supported on this platform,
// only unpacked HF is supported on destination.
// When we encounter an instruction with HF type on destination with <1> stride
// and float on source, add an additional mov that handles unpacking.
void HWConformity::fixPackedHFConversions(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  if (dst && dst->getType() == Type_HF && dst->getHorzStride() == 1 &&
      TypeSize(inst->getExecType()) > 2) {
    helperGenerateTempDst(bb, it, inst, 2, Type_HF);
  }
}

void HWConformity::fixSrc2(INST_LIST_ITER it, G4_BB *bb, bool swapSrc0and2) {
  G4_INST *inst = *it;
  int srcPos = swapSrc0and2 ? 0 : 2; // unfortunate side effect of vISA mad and
                                     // Gen mad having difference src order
  vISA_ASSERT(inst->getNumSrc() == 3, "expect 3-src inst");
  if (builder.noSrc2Regioning()) {
    auto src = inst->getSrc(srcPos);
    // we have to make sure src2 and dst are aligned
    // Promote src2's type to f if mix mode is supported.
    // e.g.,
    // mad (4) r10.0<1>:f src0 src1 r12.0<1>:hf  --> f
    // mad (4) r10.0<2>:hf src0 src1 r12.0<1>:hf --> f
    // mad (4) r10.0<1>:hf src0 src1 r12.0<2>:hf --> hf
    // mad (4) r10.0<2>:hf src0 src1 r12.1<2>:hf --> f
    // ditto for 3-src inst with int types
    G4_Type srcTy = src->getType();
    unsigned short dstEltSz = inst->getDst()->getExecTypeSize();
    if (dstEltSz >= 4) {
      if (IS_SIGNED_INT(srcTy)) {
        srcTy = Type_D;
      } else if (IS_UNSIGNED_INT(srcTy)) {
        srcTy = Type_UD;
      } else if (builder.hasMixMode() && builder.getMixModeType() == srcTy) {
        // we can change operand type to F to save one move
        srcTy = Type_F;
      }
    }
    inst->setSrc(insertMovBefore(it, srcPos, srcTy, bb, builder.getGRFAlign()),
                 srcPos);

    // Check if dst stride aligns with src2.
    if (dstEltSz != TypeSize(srcTy)) {
      replaceDst(it, inst->getDst()->getType(), builder.getGRFAlign());
    }
  }
}

void HWConformity::fixVxHFloat64b(INST_LIST_ITER it, G4_BB *bb) {
  // at this point VxH region should only be on src0
  G4_INST *inst = *it;
  G4_SrcRegRegion *src0 = inst->getSrc(0) && inst->getSrc(0)->isSrcRegRegion()
                              ? inst->getSrc(0)->asSrcRegRegion()
                              : nullptr;

  if (src0 && src0->getRegAccess() == IndirGRF &&
      src0->getRegion()->isRegionWH()) {
    auto type = src0->getType();
    // additionally check for int->float type conversion
    // FIXME: replace with SWSB's pipe check functions
    bool isFloatPipe = type == Type_HF || type == Type_F;
    if (inst->opcode() == G4_mov) {
      isFloatPipe |=
          TypeSize(type) < 8 && (inst->getDst()->getType() == Type_HF ||
                                 inst->getDst()->getType() == Type_F);
    }
    if (isFloatPipe) {
      auto intType = TypeSize(type) == 4 ? Type_UD : Type_UW;
      if (inst->isRawMov()) {
        // directly change the dst/src type to int
        inst->getDst()->setType(builder, intType);
        src0->setType(builder, intType);
      } else {
        // generate a copy move using int type
        // FIXME: code is a bit hacky, may want to change insertMovBefore
        // so that we could specify the move type
        auto origType = src0->getType();
        auto origMod = src0->getModifier();
        src0->setType(builder, intType);
        src0->setModifier(Mod_src_undef);
        auto newSrc = insertMovBefore(it, 0, intType, bb);
        newSrc->asSrcRegRegion()->setType(builder, origType);
        newSrc->asSrcRegRegion()->setModifier(origMod);
        inst->setSrc(newSrc, 0);
      }
    } else if (TypeSize(type) == 8) {
      // VxH:
      // mov (16|M0)   r7.0<1>:df   r[a0.0]<1,0>:df
      // =>
      // mov (16|M0)   r9.0<2>:ud   r[a0.0]<1,0>:ud
      // mov (16|M0)   r9.1<2>:ud   r[a0.0, 4]<1,0>:ud
      // mov (16|M0)   r7.0<1>:df   r9.0<1;1,0>:df

      // Vx1:
      // mov (16|M0)   r7.0<1>:df   r[a0.0]<2,1>:df
      // =>
      // mov (16|M0)   r9.0<2>:ud   r[a0.0]<2,2>:ud
      // mov (16|M0)   r9.1<2>:ud   r[a0.0, 4]<2,2>:ud
      // mov (16|M0)   r7.0<1>:df   r9.0<1;1,0>:df

      int numElement = inst->getExecSize();
      auto tmpSrcDcl = builder.createTempVar(numElement, type, Any);
      auto originWidth = src0->getRegion()->width;
      auto originHS = src0->getRegion()->horzStride;

      // mov instruction for lower half
      auto tmpDcl = builder.createTempVar(numElement * 2, Type_UD, Any);
      tmpDcl->setAliasDeclare(tmpSrcDcl, 0);
      auto lowerDst = builder.createDst(tmpDcl->getRegVar(), 0, 0, 2, Type_UD);
      auto newSrcRegion = (originWidth == 1) ?
                           src0->getRegion() : // VxH
                           builder.createRegionDesc(UNDEFINED_SHORT,
                                                    originWidth,
                                                    originHS * 2); //Vx1
      auto lowerSrc = builder.createIndirectSrc(
          Mod_src_undef, src0->getBase(), src0->getRegOff(),
          src0->getSubRegOff(), newSrcRegion, Type_UD, src0->getAddrImm());
      auto lowerMovInst = builder.createMov(inst->getExecSize(), lowerDst,
                                            lowerSrc, inst->getOption(), false);
      lowerMovInst->setPredicate(inst->getPredicate());
      bb->insertBefore(it, lowerMovInst);

      // mov instruction for higher half
      auto higherDst = builder.createDst(tmpDcl->getRegVar(), 0, 1, 2, Type_UD);
      auto higherSrc = builder.createIndirectSrc(
          Mod_src_undef, src0->getBase(), src0->getRegOff(),
          src0->getSubRegOff(), newSrcRegion, Type_UD, src0->getAddrImm() + 4);
      auto higherMovInst = builder.createMov(
          inst->getExecSize(), higherDst, higherSrc, inst->getOption(), false);
      higherMovInst->setPredicate(inst->getPredicate());
      bb->insertBefore(it, higherMovInst);

      // change the src0 of the instruction
      const RegionDesc *newRegion = builder.getRegionStride1();
      G4_SrcRegRegion *tmpSrcOpnd = builder.createSrcRegRegion(
          src0->getModifier(), Direct, tmpSrcDcl->getRegVar(), 0, 0, newRegion,
          tmpSrcDcl->getElemType());
      inst->setSrc(tmpSrcOpnd, 0);
    }
  }
}

bool HWConformity::fixIntToHFMove(G4_BB *bb) {
  // int to HF move requires dst to have stride 2, which would result in
  // an illegal SIMD32 inst. So we split in this case
  // we put it in a separate pass so that the split instructions may be
  // legalized later
  bool changed = false;
  for (auto I = bb->begin(), E = bb->end(); I != E; ++I) {
    auto inst = *I;
    if (inst->opcode() == G4_mov && inst->getDst()->getType() == Type_HF &&
        IS_INT(inst->getSrc(0)->getType())) {
      if (inst->getExecSize() * 2 * 2 > builder.getGRFSize() * 2) {
        evenlySplitInst(I, bb);
        changed = true;
      }
    }
  }
  return changed;
}

void HWConformity::fixPredCtrl(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_Predicate *pred = inst->getPredicate();
  if (pred && (pred->getControl() == PRED_ANY_WHOLE ||
               pred->getControl() == PRED_ALL_WHOLE)) {
    // we need WA if pred's size is greater than inst's exec size
    // and the platform does not support predctrl group size (indicated by the
    // fact we have PRED_ANY_WHOLE and PRED_ALL_WHOLE) The case where pred size
    // is less than inst's exec size is already undefined even with predCtrl
    // group size..
    G4_Declare *flagDcl = pred->getTopDcl();
    if (flagDcl->getNumberFlagElements() > inst->getExecSize()) {
      // convert
      // (f0.any32h) sel (1) ...
      // into
      // cmp (1) [ne] f1 f0 0
      // (f1) sel (1) ...
      // and
      // (f0.all32h) sel (1) ...
      // into
      // cmp (1) [e] f1 f0 0xFFFFFFFF
      //
      // if f0 happens to be < 16 elements we have to clear upper bits as well
      // in case it has garbage values
      vISA_ASSERT(!inst->getCondMod(),
             "currently don't handle an instruction with conditional modifier");
      vISA_ASSERT((inst->isWriteEnableInst() || bb->isAllLaneActive()),
             "don't handle instruction in SIMD CF for now");
      G4_Declare *tmpFlag = builder.createTempFlag(1);
      G4_Type flagType =
          flagDcl->getNumberFlagElements() == 32 ? Type_UD : Type_UW;
      uint32_t allOneMask =
          (uint32_t)((1ULL << flagDcl->getNumberFlagElements()) - 1);
      G4_Declare *cmpSrc0Flag = flagDcl;
      if (flagDcl->getNumberFlagElements() < 16) {
        // clear the upper bit of the flag
        auto andInst = builder.createBinOp(
            G4_and, g4::SIMD1, builder.createDstRegRegion(tmpFlag, 1),
            builder.createSrcRegRegion(flagDcl, builder.getRegionScalar()),
            builder.createImm(allOneMask, Type_UW), InstOpt_WriteEnable, false);
        bb->insertBefore(it, andInst);
        cmpSrc0Flag = tmpFlag;
      }
      G4_CondMod *condMod = builder.createCondMod(
          pred->getControl() == PRED_ANY_WHOLE ? Mod_ne : Mod_e,
          tmpFlag->getRegVar(), 0);

      G4_Imm *immVal = builder.createImm(
          pred->getControl() == PRED_ANY_WHOLE ? 0 : allOneMask, flagType);
      // cmp needs to be as wide as the original inst but is uniform and NoMask
      // otherwise
      auto cmpInst = builder.createInternalInst(
          nullptr, G4_cmp, condMod, g4::NOSAT, inst->getExecSize(),
          builder.createNullDst(flagType),
          builder.createSrc(cmpSrc0Flag->getRegVar(), 0, 0,
                            builder.getRegionScalar(), flagType),
          immVal, InstOpt_WriteEnable);
      bb->insertBefore(it, cmpInst);
      inst->setPredicate(
          builder.createPredicate(pred->getState(), tmpFlag->getRegVar(), 0));
    }
  }
}

// emulate mov F BF
// with
// shl UD UW 16
bool HWConformity::fixBFMove(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->opcode() != G4_mov) {
    return false;
  }
  G4_Operand *src0 = inst->getSrc(0);

  if (inst->getDst()->getType() == Type_BF) {
    // allow BF->BF moves as they may be introduced during HW conformity
    // we will change their type to HF later
    vISA_ASSERT((src0->getType() == Type_F || src0->getType() == Type_BF),
           "Only F->BF conversion is supported");
    vISA_ASSERT(!inst->getPredicate() && !inst->getCondMod() &&
           !inst->getSaturate(),
           "F->BF move does not support pred/cond mod/sat");
    if (src0->isSrcRegRegion()) {
      vISA_ASSERT(src0->asSrcRegRegion()->getModifier() == Mod_src_undef,
             "F->BF move does not support source modifier");
    }
    if (src0->getType() == Type_BF) {
      // change type of copy move to uw
      inst->getDst()->setType(builder, Type_UW);
      src0->asSrcRegRegion()->setType(builder, Type_UW);
    }
    return false;
  }

  if (src0->getType() == Type_BF && !builder.enableMovCvt()) {
    vISA_ASSERT(inst->getDst()->getType() == Type_F,
           "Only BF->F conversion is supported");

    vISA_ASSERT(!inst->getPredicate() && !inst->getCondMod(),
           "BF->F move does not support pred/cond mod");
    // don't support BF imm for now
    vISA_ASSERT(src0->isSrcRegRegion() &&
           src0->asSrcRegRegion()->getModifier() == Mod_src_undef,
           "F->BF move does not support source modifier");

    if (inst->getSaturate()) {
      replaceDst(i, inst->getDst()->getType());
      G4_INST *newMov = *(std::next(i));
      newMov->setSaturate(inst->getSaturate());
      inst->setSaturate(g4::NOSAT);
    }

    auto src0RR = src0->asSrcRegRegion();

    src0RR->setType(builder, Type_UW);
    G4_SrcRegRegion *newSrc0 = src0RR;

    inst->getDst()->setType(builder, Type_UD);
    auto newDst = inst->getDst();

    auto shlInst = builder.createBinOp(G4_shl, inst->getExecSize(), newDst,
                                       newSrc0, builder.createImm(16, Type_UW),
                                       inst->getOption(), false);
    bb->insertBefore(i, shlInst);
    bb->erase(i);

    return true;
  }

  return false;
}

bool HWConformity::isFloatOr64b(G4_INST *inst) {
  auto dst = inst->getDst();
  auto dstTy = dst->getType();

  bool goFloatPipe = IS_TYPE_FLOAT_ALL(dstTy) || TypeSize(dstTy) >= 8;

  if (!goFloatPipe) {
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      auto src = inst->getSrc(i);
      if (src) {
        bool nonScalarFloat = IS_TYPE_FLOAT_ALL(src->getType()) &&
                              src->isSrcRegRegion() &&
                              !src->asSrcRegRegion()->isScalar();
        // Q type may be mixed with other int (e.g., D = Q + D), so always needs
        // checking as we may need to fix the other operands. float type only
        // needs checking if it's non-scalar ToDo: consider skipping all mixed
        // mode as they should already confirm to region rules
        if (IS_QTYPE(src->getType()) || nonScalarFloat) {
          goFloatPipe = true;
          break;
        }
      }
    }
  }
  return goFloatPipe;
}

uint16_t HWConformity::getSrcStride(G4_SrcRegRegion *src) {
  uint16_t srcStride = 0;
  src->getRegion()->isSingleStride(src->getInst()->getExecSize(), srcStride);
  srcStride *= src->getTypeSize();
  return srcStride;
};

// split 64b copy move to multiple simd1 instructions
void HWConformity::split64bCopyToSIMD1Insts(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *movInst = *it;
  vISA_ASSERT(
      !(movInst->getSrc(0)->isIndirect() &&
        movInst->getSrc(0)->asSrcRegRegion()->getRegion()->isRegionWH()),
      "unexpected vx1/vxh indirect for 64b types");

  auto origIter = it;
  auto stride = movInst->getDst()->getHorzStride();
  for (int i = 0; i < static_cast<int>(movInst->getExecSize()); i++) {
    auto oldDst = movInst->getDst();
    G4_DstRegRegion *newDst = nullptr;
    if (oldDst->isIndirect()) {
      newDst = builder.createIndirectDst(
          oldDst->getBase(), oldDst->getSubRegOff(), 1, oldDst->getType(),
          oldDst->getAddrImm() + stride * i * 8);
    } else {
      newDst = builder.createDst(oldDst->getBase(), oldDst->getRegOff(),
                                 oldDst->getSubRegOff() + stride * i, 1,
                                 oldDst->getType(), oldDst->getAccRegSel());
    }

    auto oldSrc = movInst->getSrc(0)->asSrcRegRegion();
    G4_SrcRegRegion *newSrc = nullptr;
    if (oldSrc->isIndirect()) {
      newSrc = builder.createIndirectSrc(
          oldSrc->getModifier(), oldSrc->getBase(), oldSrc->getRegOff(),
          oldSrc->getSubRegOff(), builder.getRegionScalar(), oldSrc->getType(),
          oldSrc->getAddrImm() + stride * i * 8);
    } else {
      newSrc = builder.createSrcRegRegion(
          oldSrc->getModifier(), oldSrc->getRegAccess(), oldSrc->getBase(),
          oldSrc->getRegOff(), oldSrc->getSubRegOff() + stride * i,
          builder.getRegionScalar(), oldSrc->getType(), oldSrc->getAccRegSel());
    }

    auto newMov = builder.createMov(g4::SIMD1, newDst, newSrc,
                                    InstOpt_WriteEnable, false);
    it = bb->insertBefore(origIter, newMov);
  }
  bb->erase(origIter);
}

// on XeHP_SDV we have to make sure each source element is alignd to each dst
// element for all float/64b inst (packed HF is ok in mixed mode inst) For all
// violating instructions, we align each operand to the execution type for float
// copy moves we could directly convert their type to int
void HWConformity::fixUnalignedRegions(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  if (!inst->getDst() || inst->isSend() || inst->isDpas() ||
      hasDedicateAlignRegionConformity(it) ||
      inst->getExecSize() == g4::SIMD1) {
    // only check non-scalar ALU instructions
    return;
  }

  if (!isFloatOr64b(inst)) {
    return;
  }
  auto dst = inst->getDst();
  auto dstTy = dst->getType();
  G4_Type execTy = inst->getExecType();
  if (TypeSize(dstTy) > TypeSize(execTy)) {
    // getExecType() does not take dst ty into account, while we have to
    // consider the widest type in all operands here
    execTy = dstTy;
  }
  auto execTyWidth = TypeSize(execTy);

  // input must be a 64b copy move with packed dst and singly-strided src
  // this works for both direct and indirect dst and src
  auto change64bCopyToUD = [this](G4_INST *movInst, uint16_t srcStride) {
    auto oldSrc = movInst->getSrc(0)->asSrcRegRegion();
    G4_SrcRegRegion *movSrc = nullptr;
    if (oldSrc->getRegAccess() == Direct) {
      // change region, type, and subreg offset
      movSrc = builder.createSrcRegRegion(
          oldSrc->getModifier(), Direct, oldSrc->getBase(), oldSrc->getRegOff(),
          oldSrc->getSubRegOff() * 2,
          builder.createRegionDesc(srcStride * 2, 2, 1), Type_UD);
    } else {
      // change region and type
      movSrc = builder.createIndirectSrc(
          oldSrc->getModifier(), oldSrc->getBase(), oldSrc->getRegOff(),
          oldSrc->getSubRegOff(), builder.createRegionDesc(srcStride * 2, 2, 1),
          Type_UD, oldSrc->getAddrImm());
    }
    movInst->setSrc(movSrc, 0);

    auto oldDst = movInst->getDst();
    G4_DstRegRegion *movDst = nullptr;
    if (oldDst->getRegAccess() == Direct) {
      movDst = builder.createDst(
          oldDst->getBase(), oldDst->getRegOff(), oldDst->getSubRegOff() * 2,
          oldDst->getHorzStride(), Type_UD, oldDst->getAccRegSel());
    } else {
      movDst = builder.createIndirectDst(
          oldDst->getBase(), oldDst->getSubRegOff(), oldDst->getHorzStride(),
          Type_UD, oldDst->getAddrImm());
    }
    movInst->setDest(movDst);
    movInst->setExecSize(G4_ExecSize(movInst->getExecSize() * 2u));
    movInst->setOptionOn(InstOpt_WriteEnable);
    // caller guarantees movInst is not predicated, so we can reset its mask
    // offset to 0 this is to avoid a bug where changing mov (8|M24) r2.0<1>:q
    // -->
    // mov (16|M24) r2.0<1>:ud
    // would result in illegal mask offset for SIMD16
    movInst->setMaskOption(InstOpt_M0);
  };

  if (inst->isRawMov()) {
    // we can do better for float/64b copy moves by directly changing their type
    bool done = true;
    if (inst->getSrc(0)->isSrcRegRegion() &&
        !inst->getSrc(0)->asSrcRegRegion()->isScalar()) {
      auto src0RR = inst->getSrc(0)->asSrcRegRegion();
      int dstStride = TypeSize(dstTy) * inst->getDst()->getHorzStride();
      int srcStride = getSrcStride(src0RR);
      if (dstStride != srcStride ||
          !builder.tryToAlignOperand(inst->getSrc(0), builder.getGRFSize()) ||
          !builder.tryToAlignOperand(inst->getDst(), builder.getGRFSize())) {
        bool isNoMaskInst =
            !inst->getPredicate() &&
            (inst->isWriteEnableInst() || bb->isAllLaneActive());
        if (execTyWidth < 8) {
          auto intType = TypeSize(dstTy) == 4 ? Type_UD : Type_UW;
          inst->getDst()->setType(builder, intType);
          src0RR->setType(builder, intType);
        } else if (isNoMaskInst && inst->getDst()->getHorzStride() == 1 &&
                   srcStride != 0) {
          // For packed 64b copy moves that are not under divergent CF, we can
          // change its type to UD
          change64bCopyToUD(inst, srcStride / inst->getSrc(0)->getTypeSize());
        } else if (isNoMaskInst && inst->getDst()->getHorzStride() == 4 &&
                   srcStride != 0) {
          // If the dst stride of the 64b copy moves is 4, we can't split it
          // into 2 UD moves as the dst stride can't exceed 4. If it's not
          // under divergent CF, we can change it to multiple SIMD1 insts.
          // TODO: how to handle the case under divergent CF?
          split64bCopyToSIMD1Insts(it, bb);
        } else if (inst->getDst()->getHorzStride() < 4 && srcStride != 0 &&
                   !(src0RR->isIndirect() && dst->isIndirect())) {
          // If both dst and src0 are indirect, do not split 64b moves into 2 UD
          // moves as it may cause "infinite spill" assertion in RA for the case
          // that indirect pointing variable's size is too large:
          // .decl V0699 v_type=G type=q num_elts=512 align=wordx32
          // .decl V0700 v_type=G type=q num_elts=512 align=wordx32
          // addr_add (M1_NM, 1) A2(0)<1> &V0699 V0705(0,0)<0;1,0>
          // addr_add (M1_NM, 1) A3(0)<1> &V0700 V0706(0,0)<0;1,0>
          // mov (M1, 16) r[A3(0),0]<1>:q r[A2(0),0]<1;1,0>:q
          // =>
          // mov (M1, 16) r[A3(0),0]<2>:ud r[A2(0),0]<2;1,0>:ud
          // mov (M1, 16) r[A3(0),4]<2>:ud r[A2(0),4]<2;1,0>:ud
          // If the kernel is spilled, above code will have live range overlap
          // between r[A3(0,0), 0] and r[A2(0,0), 0]. V0699 and V0700 are both
          // 64 GRFs size, the live overlap will make them both spilled. Since
          // the size of them are so large, it will cause infinite spill.

          // we can split 64b moves with single source stride into 2 UD moves
          emulate64bMov(it, bb);
        } else {
          // a move we don't know how to handle without inserting more moves
          done = false;
        }
      }
    }
    if (done) {
      // the move is ok at this point
      return;
    }
  }

  // some operands may have fixed offset (e.g., input), and we can directly
  // check if all operands have the same sub-reg for simplicity we require all
  // operands to have same type and are packed.
  {
    bool goodOperand = true;
    if (inst->getDst()->getHorzStride() != 1) {
      goodOperand = false;
    }
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      if (inst->getSrc(i)->isSrcRegRegion()) {
        auto srcRR = inst->getSrc(i)->asSrcRegRegion();
        if (srcRR->getType() != inst->getDst()->getType() ||
            (!srcRR->isScalar() &&
             !srcRR->getRegion()->isContiguous(inst->getExecSize()))) {
          goodOperand = false;
          break;
        }
      }
    }
    uint32_t commonOffset = 0;
    if (goodOperand && hasSameSubregOffset(inst, commonOffset) &&
        commonOffset != 0) {
      // for some strange reason HW requires null operands to have the same
      // subreg offset as other operands as well
      if (inst->getDst()->isNullReg()) {
        inst->setDest(builder.createDst(builder.phyregpool.getNullReg(), 0,
                                        commonOffset / dst->getTypeSize(), 1,
                                        dst->getType()));
      }
      return;
    }
  }

  if (inst->getExecSize() == g4::SIMD2 && inst->getNumSrc() != 3) {
    if (inst->getDst()->getAccRegSel() != ACC_UNDEFINED) {
      // this instruction is internally generated, no need to check
      return;
    }

    // split currently can't handle packed imm
    // Also don't split src byte type since scalar byte to float conversion is
    // not allowed
    auto canSplit = [](G4_INST *inst, G4_BB *bb) {
      // Can not split any instruction except for NoMask ones because there is
      // no legal emask for the split instructions.
      bool isNoMaskInst = !inst->getPredicate() &&
                          (inst->isWriteEnableInst() || bb->isAllLaneActive());
      if (inst->getPredicate() || inst->getCondMod() || !isNoMaskInst) {
        return false;
      }
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        auto ty = inst->getSrc(i)->getType();
        if (IS_VINTTYPE(ty) || ty == Type_VF || IS_BTYPE(ty)) {
          return false;
        }
      }
      return true;
    };
    if (canSplit(inst, bb)) {
      auto prevIt = it == bb->begin() ? it : std::prev(it);
      if (evenlySplitInst(it, bb)) {
        // split introduces new moves which may need fixing
        // after splitting it points to the second instruction
        INST_LIST_ITER splitIt = std::prev(it);
        INST_LIST_ITER insertIt =
            prevIt == bb->begin() ? prevIt : std::next(prevIt);
        while (insertIt != splitIt) {
          fixUnalignedRegions(insertIt, bb);
          insertIt++;
        }
      }
      return;
    }
  }

  if (builder.hasFtoPackedHFMove() && inst->opcode() == G4_mov) {
    G4_Operand *src0 = inst->getSrc(0);
    const G4_Type src0Ty = src0->getType();
    G4_SrcRegRegion *reg0 =
        src0->isSrcRegRegion() ? src0->asSrcRegRegion() : nullptr;

    const bool isPackedMixedMode =
        ((dstTy == Type_HF && dst->getHorzStride() == 1 && src0Ty == Type_F) ||
         (dstTy == Type_F && src0Ty == Type_HF &&
          (!reg0 || reg0->getRegion()->isContiguous(inst->getExecSize()))));
    if (isPackedMixedMode) {
      // Indirect access (need to do it here ?) : assume they are misaligned.
      if (dst->isIndirect()) {
        dst = insertMovAfter(it, dst, dst->getType(), bb,
                             builder.getGRFAlign()); // update dst
        inst->setDest(dst);

        // The new mov may need more fixing.
        fixUnalignedRegions(std::next(it), bb);
      }
      if (reg0 && reg0->isIndirect()) {
        src0 = insertMovBefore(it, 0, src0Ty, bb, builder.getGRFAlign());
        vASSERT(src0->isSrcRegRegion());
        reg0 = src0->asSrcRegRegion(); // update reg0

        inst->setSrc(src0, 0);

        // The new mov may need more fixing.
        fixUnalignedRegions(std::prev(it), bb);
      }

      bool src0IsScalar = (!reg0 || reg0->getRegion()->isScalar());

      // If inst needs splitting, make sure to check if src0 after splitting is
      // still scalar. For example on platform with native execsize = 8:
      //    mov (16|M0)     r124.0<1>:hf  r2.3<8;8,0>:f
      // It must be splitted into the following:
      //    mov (8|M0)     r124.0<1>:hf  r2.3<0;1,0>:f
      //    mov (8|M8)     r124.8<1>:hf  r3.3<0;1,0>:f
      // After splitting, src0 becomes a scalar. Therefore, no need to do mov
      // before splitting.
      if (!src0IsScalar && inst->getExecSize() > builder.getNativeExecSize()) {
        const RegionDesc *regDesc = reg0->getRegion();
        if (regDesc->horzStride == 0 &&
            regDesc->width == builder.getNativeExecSize()) {
          src0IsScalar = true;
        }
      }

      bool d_isvalid = true, s_isvalid = true;
      uint32_t dstOffBytes = dst->ExSubRegNum(d_isvalid) * dst->getTypeSize();
      uint32_t src0OffBytes = 0;
      if (!src0IsScalar) {
        src0OffBytes = reg0->ExSubRegNum(s_isvalid) * reg0->getTypeSize();
      }

      // sanity check
      vISA_ASSERT(d_isvalid && s_isvalid, "ICE: Register region not valid!");
      vISA_ASSERT(dst->getBase()->isRegVar() &&
             (!reg0 || reg0->getBase()->isRegVar()),
             "ICE: incorrect mov operands for packed HF mov");

      const uint32_t halfGRFBytes = kernel.numEltPerGRF<Type_UB>() / 2;
      // For F operand, use the half of its offset in bytes!
      //
      // The rule is that HF operand's subreg within the first half or within
      // the second half must be the same as float operand's subreg. The idea
      // is to map HF operands to either the first half GRF or second half GRF.
      // For example,
      //   (1) The first half for hf operand
      //          mov (4|M0) r12.3<1>:hf  r10.3<1;1,0>:f
      //       r12.3 has subreg=3, it is No.4 in the first half GRF, which
      //       is equal to float src0's subreg 3 (No.4).
      //   (2) The second half for hf operand
      //          mov (4|M0) r12.11<1>:hf r10.3<1;1,0>:f
      //       r12.11 has subreg=11, it is still No.4 in the second half GRF,
      //       which is equal to float src0's subreg 3 (No.4).
      //
      // Note that all this checking assumes vars are properly aligned
      // that is, HF is at least aligned at halfGRF and F at GRF. May need to
      // adjust the alignment(cannot adjust the alignment if it is assigned reg
      // already).
      dstOffBytes = (dstTy == Type_F ? dstOffBytes / 2 : dstOffBytes);
      src0OffBytes = (src0Ty == Type_F ? src0OffBytes / 2 : src0OffBytes);
      bool isAligned;
      if (src0IsScalar) {
        // always aligned
        isAligned = true;
      } else {
        // make sure the 1st and the rest of subregs are aligned!
        isAligned =
            (dstOffBytes % halfGRFBytes) ==
                (src0OffBytes % halfGRFBytes) &&               // 1st subreg
            ((dstTy == Type_F && dst->getHorzStride() == 1) || // 2nd ... subreg
             (src0Ty == Type_F &&
              reg0->getRegion()->isContiguous(inst->getExecSize())));
      }

      G4_SubReg_Align dstMinAlign =
          (dstTy == Type_HF ? builder.getHalfGRFAlign()
                            : builder.getGRFAlign());
      G4_SubReg_Align src0MinAlign =
          (src0Ty == Type_HF ? builder.getHalfGRFAlign()
                             : builder.getGRFAlign());

      // Checking if HF opnd crosses the half GRF (as HF and F must be aligned,
      // it implies that F will not cross the GRF). As both offsets are already
      // in terms of hf, just need to check crossing half GRF! Note no need to
      // consider the case when dst subreg == 0 as this case will be
      //      handled when doing evenly split.  Also, no need to consider none-1
      //      stride as none-1 stride has been checked in isAligned!
      bool dstCrossHalfGRF = (dstOffBytes != 0 &&
                              (((dstOffBytes + 2 * inst->getExecSize() - 1) /
                                halfGRFBytes) != (dstOffBytes / halfGRFBytes)));

      if ((!isAligned && (dstOffBytes != 0 || dst->getHorzStride() != 1)) ||
          (dstCrossHalfGRF && (!src0IsScalar || dstTy == Type_HF))) {
        inst->setDest(insertMovAfter(it, dst, dst->getType(), bb, dstMinAlign));

        // The new mov may need more fixing.
        fixUnalignedRegions(std::next(it), bb);
      } else {
        G4_RegVar *baseVar = static_cast<G4_RegVar *>(dst->getBase());
        if (!baseVar->isPhyRegAssigned()) {
          G4_Declare *dstDcl = dst->getBaseRegVarRootDeclare();
          dstDcl->setSubRegAlign(dstMinAlign);
        }
      }
      if (!src0IsScalar) {
        // Similar to dstCrossHalfGRF, check src0 for crossing half grf.
        const bool src0CrossHalfGRF =
            (src0OffBytes != 0 &&
             (((src0OffBytes + 2 * inst->getExecSize() - 1) / halfGRFBytes) !=
              (src0OffBytes / halfGRFBytes)));

        if ((!isAligned &&
             (src0OffBytes != 0 ||
              !reg0->getRegion()->isContiguous(inst->getExecSize()))) ||
            src0CrossHalfGRF) {
          inst->setSrc(insertMovBefore(it, 0, src0Ty, bb, src0MinAlign), 0);

          // The new mov may need more fixing.
          fixUnalignedRegions(std::prev(it), bb);
        } else {
          G4_RegVar *baseVar = static_cast<G4_RegVar *>(reg0->getBase());
          if (!baseVar->isPhyRegAssigned()) {
            G4_Declare *src0Dcl = reg0->getBaseRegVarRootDeclare();
            src0Dcl->setSubRegAlign(src0MinAlign);
          }
        }
      }

      // Check if the execsize is legal
      if (inst->getExecSize() > builder.getNativeExecSize() &&
          !builder.enableMovCvt()) {
        evenlySplitInst(it, bb);
      }
      return;
    }
  }

  // fix Dst if necessary
  // some special mix mode dst are allowed provided the instruction has F type:
  // r1.0<2>:bf
  // r1.1<2>:bf
  // r1.0<1>:bf
  // r1.8<1>:bf
  bool isSpecialMixModeDst = false;
  if (builder.getMixModeType() == dstTy && IS_FTYPE(execTy)) {
    uint16_t offset = 0;
    bool isAligned =
        builder.tryToAlignOperand(dst, offset, builder.getGRFSize() / 2);
    if (dst->getHorzStride() == 1) {
      isSpecialMixModeDst = isAligned;
    } else if (dst->getHorzStride() == 2) {
      isSpecialMixModeDst = isAligned || (offset % 32) == 2;
    }
  }

  auto dstStride = TypeSize(dstTy) * dst->getHorzStride();
  uint16_t dstAlign =
      inst->getSrc(0)->getType() == Type_VF ? 16 : builder.getGRFSize();
  if (dst->getRegAccess() == Direct && !isSpecialMixModeDst &&
      !builder.enableMovCvt() &&
      (!builder.tryToAlignOperand(dst, dstAlign) || dstStride != execTyWidth)) {
    inst->setDest(
        insertMovAfter(it, dst, dst->getType(), bb, builder.getGRFAlign()));
    if (IS_TYPE_FLOAT_ALL(dst->getType()) || dst->getTypeSize() == 8) {
      // the move may need more fixing
      fixUnalignedRegions(std::next(it), bb);
    }
  } else if (dst->getRegAccess() == IndirGRF) {
    // Since we can't know if an indirect dst is aligned or not,
    // The proper fix is to insert a move then change its type to int.
    inst->setDest(
        insertMovAfter(it, dst, dst->getType(), bb, builder.getGRFAlign()));
    // the move may need more fixing
    fixUnalignedRegions(std::next(it), bb);
  }

  // generate a move where each element is aligned to execTyWidth
  // e.g.,
  // mov (8) V1<1>:q V2<1;1,0>:ud
  // becomes
  // mov (8) tmp<2>:ud V2<1;1,0>:ud
  // mov (8) V1<1>:q tmp<2;1,0>:ud
  // or
  // add (8) V1<1>:f V2<2;1,0>:f V3<1;1,0>:f
  // becomes
  // mov (8) tmp<1>:ud V2<2;1,0>:ud
  // add (8) V1<1>:f tmp<1;1,0>:f V3<1;1,0>:f
  // note that for float types we have to do the move in int since the move may
  // be illegal otherwise
  auto doAlignMove = [&](G4_INST *inst, int srcPos, int stride) {
    // caller must ensure src is a srcregregion
    bool movNeedsFix = false;
    auto src = inst->getSrc(srcPos)->asSrcRegRegion();
    auto srcTy = src->getType();
    auto tmpTy = getUnsignedType(TypeSize(srcTy));
    auto movSrcTy = tmpTy;
    auto newSrcTy = srcTy;
    if (stride == 8 ||
        (tmpTy == Type_UB && builder.getNativeExecSize() > g4::SIMD8 &&
         (stride == 2 || stride == 4))) {
      // use UW as the tmp, and divide the stride by 2
      // there are two reasons for this transform,
      // 1) stride 8 is not supported
      // 2) avoid read-modify-write on bytes
      // mov (4) V1<4>:uw V2:ub
      // then use <4;1,0>:uw in the original inst
      tmpTy = (srcTy == Type_UB) ? Type_UW : Type_W;
      movSrcTy = srcTy;
      stride = stride / 2;
      newSrcTy = tmpTy;
    }
    auto tmp = builder.createTempVar(inst->getExecSize() * stride, tmpTy,
                                     builder.getGRFAlign());
    auto movSrc = builder.createSrcRegRegion(*src);
    movSrc->setModifier(Mod_src_undef);
    movSrc->setType(builder, movSrcTy);
    auto movInst = builder.createMov(inst->getExecSize(),
                                     builder.createDstRegRegion(tmp, stride),
                                     movSrc, inst->getOption(), false);
    if (movSrc->getTypeSize() == 8) {
      vISA_ASSERT(stride == 1, "expect dst stride to be 1 here");
      // the move instruction is itself illegal due to the source region being
      // non-contiguous/not GRF-aligned if the region is singly-strided, we can
      // change it into a UD move, e.g., mov (8) V1<1>:q V2<2;1,0>:q becomes (W)
      // mov (16) V1<1>:ud V2<4;2,1>:ud
      uint16_t srcStride = 0;
      if (movSrc->getRegion()->isSingleStride(inst->getExecSize(), srcStride)) {
        change64bCopyToUD(movInst, srcStride);
      } else {
        movNeedsFix = true;
      }
    }
    bb->insertBefore(it, movInst);
    if (movNeedsFix) {
      // try splitting the move as last resort
      // it may be successful if we are not in SIMD CF
      evenlySplitInst(std::prev(it), bb);
    }
    auto newSrc = builder.createSrcRegRegion(
        src->getModifier(), Direct, tmp->getRegVar(), 0, 0,
        builder.createRegionDesc(stride, 1, 0), newSrcTy);
    inst->setSrc(newSrc, srcPos);
  };

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_SrcRegRegion *src = inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion()
                               ? inst->getSrc(i)->asSrcRegRegion()
                               : nullptr;
    if (src) {
      if (IS_BTYPE(src->getType()) &&
          (src->getRegion()->isRegionWH() || src->getRegion()->isScalar())) {
        // no scalar byte when dst is float
        // byte src with DF dst is handled by fixMov
        inst->setSrc(insertMovBefore(
                         it, 0,
                         inst->getDst()->getTypeSize() == 4 ? Type_D : Type_W,
                         bb),
                     0);
      } else if (!src->getRegion()->isRegionWH() &&
                 !src->getRegion()->isScalar()) {
        // indirect VxH operands are handled elsewhere
        auto srcStride = getSrcStride(src);
        bool isMixModeSrc =
            isLowPrecisionFloatTy(src->getType()) && IS_FTYPE(execTy);
        bool isMixModePackedSrc = isMixModeSrc && srcStride == 2;
        uint16_t alignment = isMixModePackedSrc ? (builder.getGRFSize() / 2)
                                                : builder.getGRFSize();
        // for mix mode the source must be packed, otherwise srcStride shoudl be
        // == sizeof(exec type)
        if (!builder.enableMovCvt() &&
            (!builder.tryToAlignOperand(src, alignment) ||
             (isMixModeSrc ? !isMixModePackedSrc : srcStride != execTyWidth))) {
          // for mad, mul and add with qword destination and dword operands,
          // src0, src1 have full regioning muxes, and so dwords can be
          // chosen anywhere. As a result, we do not need moves for
          // their alignment. src2 in mad has some regioning restriction,
          // which is already accomodated in emit visa pass. As
          // a result, src2 also does not need a move for alignment.

          if (builder.hasWideMulMadOpsEnabled() &&
              (inst->opcode() == G4_mad || inst->opcode() == G4_mul ||
               inst->opcode() == G4_add)) {
            // check if the execTyWidth is qword size
            // and src is dword or qword size (4bytes),
            // ignore the mov
            if (execTyWidth == 8 && srcStride >= 4 &&
                !IS_DFTYPE(src->getType())) {
              continue;
            } else {
              // if mad/mul operation does not have qword dst and
              // dword operads, then default to original behavior
              // which is to insert aligned movs for operands
              int stride =
                  (int)(isMixModeSrc ? 1 : execTyWidth / src->getTypeSize());
              doAlignMove(inst, i, stride);
            }
          } else
          {
            int stride =
                (int)(isMixModeSrc ? 1 : execTyWidth / src->getTypeSize());
            doAlignMove(inst, i, stride);
          }
        }
      }
    }
  }
}

bool HWConformity::fixFcvt(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->opcode() != G4_fcvt) {
    return false;
  }

  // Format conversion allowed between fp16 and fp8 operands in the following
  // cases:
  //  1, Execution size must not be 1.
  //  2, fp8 operand is packed.
  //  3, Src and dst register offset is restricted to 0 (GRF aligned).
  //  4. no scalar fp8 broadcast (as there is no simd1, fp8 operand should
  //     not be a scalar).
  if (IS_BTYPE(inst->getDst()->getType()) ||
      IS_BTYPE(inst->getSrc(0)->getType())) {
    vISA_ASSERT(((IS_BTYPE(inst->getDst()->getType()) &&
             inst->getSrc(0)->getType() == Type_HF) ||
            (IS_BTYPE(inst->getSrc(0)->getType()) &&
             inst->getDst()->getType() == Type_HF)),
           "Only FP8<->HF conversion is supported");
    vISA_ASSERT(!inst->getPredicate() && !inst->getCondMod(),
           "FP8<->HF move does not support pred/cond mod");
    vISA_ASSERT(inst->getSrc(0)->isSrcRegRegion(),
           "HF<->FP8 currently supports non-imm source only");
    vISA_ASSERT(inst->getSrc(0)->isSrcRegRegion() &&
           inst->getSrc(0)->asSrcRegRegion()->getRegAccess() == Direct &&
           inst->getSrc(0)->asSrcRegRegion()->getModifier() == Mod_src_undef,
           "FP8<->HF move does not support source modifier");

    if (!builder.enableMovCvt() &&
        ((!builder.tryToAlignOperand(
             inst->getSrc(0),
             builder.numEltPerGRF<Type_UB>())) || // case 3 for src
         (IS_BTYPE(inst->getSrc(0)->getType()) &&
          !inst->getSrc(0)->asSrcRegRegion()->getRegion()->isContiguous(
              inst->getExecSize())))) // case 2 for src
    {
      inst->setSrc(insertMovBefore(i, 0, inst->getSrc(0)->getType(), bb,
                                   builder.getGRFAlign()),
                   0);
      G4_INST *newMovInst = *(std::prev(i));
      if (newMovInst->getSrc(0)->getType() == Type_HF) {
        newMovInst->getSrc(0)->asSrcRegRegion()->setType(builder, Type_UW);
        newMovInst->getDst()->asDstRegRegion()->setType(builder, Type_UW);
      }
      newMovInst->getDst()->setHorzStride(1);
      if (inst->getExecSize() != g4::SIMD1) {
        inst->getSrc(0)->asSrcRegRegion()->setRegion(
            builder, builder.getRegionStride1());
      }
      inst->setOptionOn(InstOpt_WriteEnable);
    }

    if (!builder.enableMovCvt() &&
        ((IS_BTYPE(inst->getDst()->getType()) &&
          inst->getDst()->getHorzStride() != 1) || // case 2 for dst
         (!builder.tryToAlignOperand(
             inst->getDst(),
             builder.numEltPerGRF<Type_UB>())))) // case 3 for dst
    {
      replaceDst(i, inst->getDst()->getType(), builder.getGRFAlign());
      G4_INST *newMovInst = *(std::next(i));
      if (newMovInst->getDst()->getType() == Type_HF) {
        newMovInst->getSrc(0)->asSrcRegRegion()->setType(builder, Type_UW);
        newMovInst->getDst()->asDstRegRegion()->setType(builder, Type_UW);
      }
      if (inst->getExecSize() != g4::SIMD1) {
        newMovInst->getSrc(0)->asSrcRegRegion()->setRegion(
            builder, builder.getRegionStride1());
      }
      inst->getDst()->setHorzStride(1);
      inst->setOptionOn(InstOpt_WriteEnable);
    }

    // case 1: SIMD1 hf<->fp8, in general we do below transform:
    //     (W)  mov (1|M0)   r10.0<1>:bf8   r12.0<0;1,0>:hf
    //     =>
    //     (W)  mov (2|M0)   r20.0<1>:bf8   r12.0<0;1,0>:hf
    //     (W)  mov (1|M0)   r10.0<1>:ub    r20.0<0;1,0>:ub
    // If the root declare is fully used by dst, we can avoid generating the
    // extra mov by enlarging the declares' size:
    //      //.declare V0039 (41)  rf=r size=1 type=ub align=32 words (r10.0)
    //      (W)  mov (1|M0)   r10.0<1>:bf8   r12.0<0;1,0>:hf
    //      =>
    //      //.declare V0039 (41)  rf=r size=2 type=ub align=32 words (r10.0)
    //      (W)  mov (2|M0)   r10.0<1>:bf8   r12.0<0;1,0>:hf
    // case 4: scalar fp8 src0
    //      mov (2|M0)   r10.0<1>:hf  r12.0<0;1,0>:bf8
    //   ==>
    //      mov (2|M0)   r20.0<1>:ub  r12.0<0;1,0>:ub
    //      mov (2|M0)   r10.0<1>:hf  r20.0<1;1,0>:bf8
    //  Note if src0 dcl's size can be increased safely, it will be changed
    //  directly to
    //      mov (2|M0)   r10.0<1>:hf  r12.0<1;1,0>:bf8
    //    where r12.1:bf8 is not used and isn't initialized.
    if (inst->getExecSize() == g4::SIMD1) // case 1
    {
      G4_DstRegRegion *dst = inst->getDst();
      G4_Declare *rootDcl = nullptr;
      if (dst->getBase() && dst->getBase()->isRegVar()) {
        rootDcl = dst->getBaseRegVarRootDeclare();
      }
      if (rootDcl && rootDcl->getByteSize() == dst->getTypeSize()) {
        G4_Declare *dcl = dst->getBase()->asRegVar()->getDeclare();
        while (dcl) {
          dcl->setTotalElems(dcl->getTotalElems() * 2);
          dcl = dcl->getAliasDeclare();
        }
        inst->setExecSize(g4::SIMD2);
      } else {
        G4_Declare *dcl = builder.createTempVar(2, inst->getDst()->getType(),
                                                builder.getGRFAlign());
        G4_SrcRegRegion *srcRegion =
            builder.createSrcRegRegion(dcl, builder.getRegionScalar());
        uint32_t newOption = InstOpt_WriteEnable | inst->getMaskOption();
        G4_INST *newMovInst = builder.createMov(g4::SIMD1, inst->getDst(),
                                                srcRegion, newOption, false);
        bb->insertAfter(i, newMovInst);

        G4_DstRegRegion *newDst = builder.createDstRegRegion(dcl, 1);
        inst->setDest(newDst);
        inst->setExecSize(g4::SIMD2);
      }

      // case 4: if src is fp8, may insert mov as scalar broadcast is not
      // allowed.
      G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
      vASSERT(src0->getRegion()->isScalar());
      if (IS_BTYPE(src0->getType())) {
        G4_Declare *src0RootDcl = src0->getBaseRegVarRootDeclare();
        if (src0RootDcl->getByteSize() == src0->getTypeSize()) {
          G4_Declare *dcl = src0->getBase()->asRegVar()->getDeclare();
          while (dcl) {
            dcl->setTotalElems(dcl->getTotalElems() * 2);
            dcl = dcl->getAliasDeclare();
          }
          src0->setRegion(builder, builder.getRegionStride1());
        } else {
          broadcast(bb, i, 0, builder.getGRFAlign());
        }
      }
    }

    return true;
  }

  if (inst->getDst()->getType() == Type_UD) {
    // fcvt a:tf32   b:f
    // Make sure dst/src0 have the same subreg offset and stride, except for
    // scalar broadcast.
    G4_Operand *src0 = inst->getSrc(0);
    if (src0->isSrcRegRegion() &&
        !src0->asSrcRegRegion()->getRegion()->isScalar()) {
      G4_SrcRegRegion *regSrc0 = inst->getSrc(0)->asSrcRegRegion();
      G4_DstRegRegion *regDst = inst->getDst();
      uint16_t srcSingleStride;
      // Note that regSrc0 must not be scalar here!
      if (!regSrc0->getRegion()->isSingleStride(inst->getExecSize(),
                                                srcSingleStride)) {
        // set it to an invalid value as it has no single (uniform) stride
        srcSingleStride = 0xFFFF;
      }
      if (srcSingleStride != regDst->getHorzStride() ||
          !hasSameSubregOffset(inst)) {
        // Need to force GRF-alignment and stride = 1
        if (srcSingleStride != 1 || !regSrc0->checkGRFAlign(builder)) {
          // Make sure to do UD copy for src
          regSrc0->setType(builder, Type_UD);
          // Insert mov before i
          replaceSrc(i, 0, Type_UD, bb, ThirtyTwo_Word);
          // must have the original type (float) for i
          inst->getSrc(0)->asSrcRegRegion()->setType(builder, Type_F);
        }
        if (regDst->getHorzStride() != 1 || !regDst->checkGRFAlign(builder)) {
          replaceDst(i, regDst->getType(), ThirtyTwo_Word);
        }
        return true;
      }
    }
  }

  return false;
}

// on PVC there are new restrictions on using byte/word region due to XBar
// reduction
void HWConformity::fixByteXBarRestriction(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  // G4_fcvt/G4_srnd should be fixed in fixFcvt()/fixSrnd().
  if (inst->opcode() == G4_fcvt || inst->opcode() == G4_srnd) {
    return;
  }

  if (!inst->getDst() || inst->isSend() || inst->isDpas() ||
      inst->getExecSize() == g4::SIMD1) {
    // only check non-scalar ALU instructions
    return;
  }

  // due to much stronger restriction on float-pipe operation,
  // assume float-op has been fixed in fixUnalignedRegions
  if (isFloatOr64b(inst)) {
    return;
  }

  // hardware checks restriction even on null
  if (inst->getDst()->isNullReg()) {
    auto dst = inst->getDst();
    auto dstTy = dst->getType();
    auto stride = dst->getHorzStride();

    if ((dstTy == Type_W || dstTy == Type_UW) && stride < 2)
      dst->setHorzStride(2);
    else if (dstTy == Type_B || dstTy == Type_UB) {
       // create a new dst with W/UW type
      G4_DstRegRegion *new_null =
          builder.createNullDst(dstTy == Type_B ? Type_W : Type_UW);
      new_null->setHorzStride(2);
      inst->setDest(new_null);
    }
    return;
  }

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22010493955) &&
      inst->opcode() == G4_mov && inst->getSaturate()) {
    auto dst = inst->getDst();
    auto dstTy = dst->getType();
    if ((dstTy == Type_B || dstTy == Type_UB) && inst->getSrc(0) &&
        inst->getSrc(0)->isSrcRegRegion()) {
      auto src = inst->getSrc(0)->asSrcRegRegion();
      auto srcTy = src->getType();
      if (srcTy == Type_B) {
        insertMovBefore(it, 0, Type_D, bb);
        return;
      }
      if (srcTy == Type_UB) {
        insertMovBefore(it, 0, Type_UD, bb);
        return;
      }
    }
  }

  auto isDclGRFAligned = [this](G4_Declare *dcl) {
    if (!dcl) {
      return false;
    }
    uint32_t offset = 0;
    auto rootDcl = dcl->getRootDeclare(offset);
    return rootDcl->getSubRegAlign() >= builder.getGRFAlign() &&
           (offset % builder.getGRFSize() == 0);
  };

  bool needFix = false;
  auto dst = inst->getDst();
  auto dstTy = dst->getType();
  bool dstAligned =
      (dst->getRegAccess() == Direct) && isDclGRFAligned(dst->getTopDcl());
  auto dstSubRegOff = dst->getSubRegOff();
  bool allDirect = (dst->getRegAccess() == Direct);

  // Fix for the odd destination subregister for G4_and, G4_or, G4_xor, G4_add,
  // G4_asr, G4_sel, G4_cmp Adding mov instruction to change inst dst
  // subregister to even when conditions are met:
  // - instruction is at least two sources and dst isn't null
  // - dst sub-register is odd and dst stride is at least 1
  // - src0 reg region exist and isn't contiguous
  // - dst is B/UB, src0 is B/UB or W/UW, src1 is B/UB or W/UW
  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22010487853) &&
      inst->getNumSrc() > 1 && inst->getDst() != NULL && dstSubRegOff % 2 &&
      inst->getDst()->getHorzStride() > 1 &&
      inst->getSrc(0)->isSrcRegRegion() &&
      inst->getSrc(0)->asSrcRegRegion()->getRegion()->isContiguous(
          inst->getExecSize()) == false &&
      IS_BTYPE(inst->getDst()->getType()) &&
      (IS_BTYPE(inst->getSrc(0)->getType()) ||
       IS_WTYPE(inst->getSrc(0)->getType())) &&
      (IS_BTYPE(inst->getSrc(1)->getType()) ||
       IS_WTYPE(inst->getSrc(1)->getType()))) {
    auto newDstTy = inst->getDst()->getType();
    // if dst stride is equal 4 and b2b-DS2 rule isn't covered, changing dst
    // type to dword
    if (inst->getDst()->getHorzStride() == 4) {
      newDstTy = Type_D;
      replaceDst(it, newDstTy);
      return;
    }
    // force a fix when we applied b2b or w2b rule
    needFix = true;
  }

  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22010487853) &&
      (dstTy == Type_B || dstTy == Type_UB) && (dstSubRegOff % 2) &&
      dst->getHorzStride() >= 4 && inst->getExecSize() == g4::SIMD32) {
    vASSERT(canSplitInst(inst, NULL));
    evenlySplitInst(it, bb);
    return;
  }
  // check src0-to-dest and src1-to-dest restrictions
  for (int i = 0, numSrc = inst->getNumSrc(); !needFix && i < 2 && i < numSrc;
       ++i) {
    G4_SrcRegRegion *src = inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion()
                               ? inst->getSrc(i)->asSrcRegRegion()
                               : nullptr;
    if (!src) {
      continue;
    }
    // check then fix the restriction on Src
    auto srcTy = src->getType();
    auto region = src->getRegion();
    bool srcDirect = (src->getRegAccess() == Direct);
    allDirect &= srcDirect;
    // skip VxH indirect case because src operand will be read out one element a
    // time
    if (!srcDirect && region->isRegionWH()) {
      continue;
    }
    bool srcAligned = srcDirect && isDclGRFAligned(src->getTopDcl());
    auto srcSubRegOff = src->getSubRegOff();
    auto numRows = 1;
    unsigned ss =
        (region->width == 1) ? region->vertStride : region->horzStride;
    // we need to check the region rule one row at a time under the following
    // situation
    if (region->width > 1 &&
        region->width * region->horzStride != region->vertStride) {
      numRows = inst->getExecSize() / region->width;
      vASSERT((inst->getExecSize() % region->width) == 0);
    }
    for (int row = 0; row < numRows; ++row) {
      auto srcSubRegOffForRow = (srcSubRegOff + row * region->vertStride) %
                     (builder.getGRFSize() / TypeSize(srcTy));
      auto dstSubRegOffForRow =
          (dstSubRegOff + row * region->width * dst->getHorzStride()) %
          (builder.getGRFSize() / TypeSize(dstTy));
      bool dstSubRegOffDwordAlign = builder.tryToAlignOperand(dst, 4);
      if (TypeSize(srcTy) == 2) {
        // w2w and w2b rules
        // cannot have the case of w2b packing case, i.e. dest-stride == 1
        vASSERT(!(TypeSize(dstTy) == 1 && dst->getHorzStride() == 1));
        if ((TypeSize(dstTy) == 2 && dst->getHorzStride() == 1) ||
            (TypeSize(dstTy) == 1 && dst->getHorzStride() == 2)) {
          if (numRows > 1 && !dstSubRegOffDwordAlign) {
            needFix = true;
          } else if (ss == 2) {
            bool Aligned =
                srcAligned && dstAligned && (numRows == 1) &&
                !(i == 1 && TypeSize(dstTy) == 1 &&
                  (VISA_WA_CHECK(builder.getPWaTable(), Wa_16012383669) ||
                   builder.getOption(vISA_RestrictSrc1ByteSwizzle))) &&
                ((dstSubRegOffForRow % (32 / TypeSize(dstTy))) ==
                 (srcSubRegOffForRow / TypeSize(dstTy)));
            needFix |= !Aligned;
          } else if (ss > 2) {
            needFix = true;
          }
        }
      } else if (TypeSize(srcTy) == 1) {
        if (TypeSize(dstTy) == 2 && dst->getHorzStride() == 1) // b2w rule
        {
          if (numRows > 1 && !dstSubRegOffDwordAlign) {
            needFix = true;
          } else if (ss == 4) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((2 * (dstSubRegOffForRow % 16)) == (srcSubRegOffForRow / 2));
            needFix |= !Aligned;
          } else if (ss == 8) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((2 * (dstSubRegOffForRow % 8)) == (srcSubRegOffForRow / 4));
            needFix |= !Aligned;
          } else if (ss > 8) {
            needFix = true;
          }
        } else if (TypeSize(dstTy) == 1 &&
                   dst->getHorzStride() == 2) // b2b-DS2 rule
        {
          if (numRows > 1 && !dstSubRegOffDwordAlign) {
            needFix = true;
          } else if (ss == 4) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((dstSubRegOffForRow % 32) == (srcSubRegOffForRow / 2));
       // change dstAligned to false, so we need a pack-shift
       // in the end of the fix
            if (VISA_WA_CHECK(builder.getPWaTable(), Wa_1507979211)) {
              dstAligned &= (dstSubRegOffForRow < 32);
              Aligned &= (dstSubRegOffForRow < 32);
            }
            needFix |= !Aligned;
          } else if (ss == 8) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((dstSubRegOffForRow % 16) == (srcSubRegOffForRow / 4));
            needFix |= !Aligned;
          } else if (ss > 8) {
            needFix = true;
          }

        } else if (TypeSize(dstTy) == 1 && dst->getHorzStride() == 1 &&
                   region->width != 2) // b2b-DS1 rule
        {
          if (numRows > 1 && !dstSubRegOffDwordAlign) {
            needFix = true;
          } else if (ss == 2) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((dstSubRegOffForRow % 32) == (srcSubRegOffForRow / 2));
            needFix |= !Aligned;
          } else if (ss == 4) {
            bool Aligned = srcAligned && dstAligned && (numRows == 1) &&
                ((dstSubRegOffForRow % 16) == (srcSubRegOffForRow / 4));
       // change dstAligned to false, so we need a pack-shift
       // in the end of the fix
            if (VISA_WA_CHECK(builder.getPWaTable(), Wa_1507979211)) {
              dstAligned &= (dstSubRegOffForRow < 32);
              Aligned &= (dstSubRegOffForRow < 32);
            }
            needFix |= !Aligned;
          } else if (ss > 4) {
            needFix = true;
          }

        } else if (TypeSize(dstTy) == 1 && dst->getHorzStride() == 1 &&
                   region->width == 2) // b2b-DS1 rule
        {
          if (numRows > 1 && !dstSubRegOffDwordAlign) {
            needFix = true;
          } else if (region->horzStride + region->vertStride >= 4) {
            if (region->horzStride == 2 && region->vertStride == 4) {
              bool Aligned = srcAligned && dstAligned &&
                  ((dstSubRegOffForRow % 32) == (srcSubRegOffForRow / 2));
       // change dstAligned to false, so we need a pack-shift
       // in the end of the fix
              if (VISA_WA_CHECK(builder.getPWaTable(), Wa_1507979211)) {
                dstAligned &= (dstSubRegOffForRow < 32);
                Aligned &= (dstSubRegOffForRow < 32);
              }
              needFix |= !Aligned;
            } else if (region->horzStride == 4 && region->vertStride == 8) {
              bool Aligned = srcAligned && dstAligned &&
                  ((dstSubRegOffForRow % 16) == (srcSubRegOffForRow / 4));
       // change dstAligned to false, so we need a pack-shift
       // in the end of the fix
              if (VISA_WA_CHECK(builder.getPWaTable(), Wa_1507979211)) {
                dstAligned &= (dstSubRegOffForRow < 32);
                Aligned &= (dstSubRegOffForRow < 32);
              }
              needFix |= !Aligned;

            } else {
              needFix = true;
            }
          } else if (region->horzStride == 2) {
       // DS==1 && W==2 && HS==2 && VS == 0 or 1
            needFix = true;
          }
        }
      }
    }
  }

  if (needFix) {
    if (inst->opcode() == G4_mov && builder.enableMovCvt()) {
      G4_DstRegRegion *dst = inst->getDst();
      G4_SrcRegRegion *src = inst->getSrc(0)->asSrcRegRegion();
      auto dstTy = dst->getType();
      auto srcTy = src->getType();
      auto region = src->getRegion();
      if (dstTy == srcTy && dstTy == Type_UB && region->vertStride == 1 &&
          region->width == 16 && region->horzStride == 4) {
        return;
      }
    }

    // For addr add instruction, fix src0 instead of dst. This will
    // simplify pointer to analysis.
    // For example:
    //   add (16) A0(0,0)<1>:uw ByteOffset_0(0,0)<2;1,0>:uw &ShuffleTmp+0
    //   =>
    //   mov (16) TV1(0,0)<1>:uw ByteOffset_0(0,0)<2;1,0>:uw
    //   add (16) A0(0,0)<1>:uw TV1(0,0)<1;1,0>:uw &ShuffleTmp+0
    if (inst->isAddrAdd()) {
      replaceSrc(it, 0, inst->getSrc(0)->getType(), bb, builder.getGRFAlign());
      // Fix the new inserted mov inst
      fixByteXBarRestriction(std::prev(it), bb);
      return;
    }

    // split currently can't handle packed imm
    // Also don't split if src and dst have overlap as it will introduce extra
    // mov which could be illegal. If we further fix the extra illegal mov
    // instruction, we will get worse codes compared to not splitting.
    auto canDoSplit = [](G4_INST *inst, IR_Builder &builder) {
      if (inst->getPredicate() || inst->getCondMod()) {
        return false;
      }
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        auto ty = inst->getSrc(i)->getType();
        if (IS_VINTTYPE(ty) || IS_VFTYPE(ty)) {
          return false;
        } else {
          G4_CmpRelation rel =
              inst->getDst()->compareOperand(inst->getSrc(i), builder);
          if (rel != Rel_disjoint)
            return false;
        }
      }
      return true;
    };

    if (inst->getExecSize() == g4::SIMD2 && allDirect &&
        inst->getNumSrc() != 3 && canDoSplit(inst, builder)) {
      // just split the inst
      evenlySplitInst(it, bb);
      return;
    }

    auto scale = 4 / TypeSize(dstTy);
    const RegionDesc *unpackRegion = builder.createRegionDesc(scale, 1, 0);
    auto dstSubRegOffTmp =
        dstSubRegOff % (builder.getGRFSize() / TypeSize(dstTy));

    // compute the sub-reg-offset we need to use
    short tmpSSR = 0;
    if (TypeSize(dstTy) == 2) {
      tmpSSR = 2 * (dstSubRegOffTmp % 16);
    } else {
      vASSERT(TypeSize(dstTy) == 1);
      if (dst->getHorzStride() == 2) {
        tmpSSR = 2 * (dstSubRegOffTmp % 32);
      } else {
        vASSERT(dst->getHorzStride() == 1);
        tmpSSR = 4 * (dstSubRegOffTmp % 16);
      }
    }
    auto tempSize = std::max(inst->getExecSize() * scale + tmpSSR,
                             (int)builder.getGRFSize() / TypeSize(dstTy));

    // Replace the dest with a temp, same-type, offset == 0
    // stride == 2 for word-type; stride == 4 for byte-type
    // Add a B2B or W2W pack-move from temp.0(stride; 1, 0)  to the
    // original-dest.sub(ds) however, if the original-dest is NOT grf-aligned,
    // we need another B2B or W2W to shift the location of packed bytes or words
    // after packing.
    if (dstAligned && (tempSize * TypeSize(dstTy) <=
                       (unsigned short)(builder.getGRFSize() * 2))) {
      G4_Declare *unpackDcl =
          builder.createTempVar(tempSize, dstTy, builder.getGRFAlign());

      G4_SrcRegRegion *unpackSrc =
          builder.createSrc(unpackDcl->getRegVar(), 0, tmpSSR, unpackRegion,
                            unpackDcl->getElemType());

      G4_Predicate *pred = nullptr;
      if (inst->opcode() != G4_sel) {
        pred = inst->getPredicate();
        inst->setPredicate(nullptr);
      }
      unsigned int new_option = inst->getMaskOption();

      auto pos = it;
      pos++;

      // insert the packing move
      G4_INST *packInst = builder.createMov(inst->getExecSize(), dst, unpackSrc,
                                            new_option, false);
      packInst->setPredicate(pred);
      bb->insertBefore(pos, packInst);

      // change the destination of the original instruction
      if (dstTy == Type_UW || dstTy == Type_W || inst->getSaturate() ||
          (tmpSSR % scale)) {
        auto tmpDst = builder.createDst(unpackDcl->getRegVar(), 0, tmpSSR,
                                        scale, unpackDcl->getElemType());
        inst->setDest(tmpDst);
      } else {
        // use dword destination to avoid read-modify-write
        G4_Declare *tmpDstDcl = builder.createTempVar(
            tempSize / scale, (dstTy == Type_UB) ? Type_UD : Type_D,
            builder.getGRFAlign());
        tmpDstDcl->setAliasDeclare(unpackDcl, 0);
        auto tmpDst =
            builder.createDst(tmpDstDcl->getRegVar(), 0, tmpSSR / scale, 1,
                              tmpDstDcl->getElemType());
        inst->setDest(tmpDst);
      }
    } else {
      G4_Declare *unpackDcl = builder.createTempVar(
          inst->getExecSize() * scale, dstTy, builder.getGRFAlign());
      G4_SrcRegRegion *unpackSrc =
          builder.createSrcRegRegion(unpackDcl, unpackRegion);
      G4_Predicate *pred = nullptr;
      if (inst->opcode() != G4_sel) {
        pred = inst->getPredicate();
        inst->setPredicate(nullptr);
      }
      unsigned int new_option = inst->getMaskOption();
      auto pos = it;
      auto dstride = dst->getHorzStride();
      const RegionDesc *shiftRegion = builder.createRegionDesc(dstride, 1, 0);
      G4_Declare *shiftDcl = builder.createTempVar(
          inst->getExecSize() * dstride, dstTy, builder.getGRFAlign());
      G4_SrcRegRegion *shiftSrc =
          builder.createSrcRegRegion(shiftDcl, shiftRegion);
      auto packTmp = builder.createDstRegRegion(shiftDcl, dstride);

      // pack
      G4_INST *packInst = builder.createMov(inst->getExecSize(), packTmp,
                                            unpackSrc, new_option, false);
      pos = bb->insertAfter(pos, packInst);
      // then shift the bytes and words location
      G4_INST *shiftInst = builder.createMov(inst->getExecSize(), dst, shiftSrc,
                                             new_option, false);
      shiftInst->setPredicate(pred);
      pos = bb->insertAfter(pos, shiftInst);
      // change the destination of the original instruction
      if (dstTy == Type_UW || dstTy == Type_W || inst->getSaturate()) {
        inst->setDest(builder.createDstRegRegion(unpackDcl, scale));
      } else {
        // situations we use dword-tmp to reduce byte-read-mod-write
        G4_Declare *tmpDstDcl = builder.createTempVar(
            inst->getExecSize(), (dstTy == Type_UB) ? Type_UD : Type_D,
            builder.getGRFAlign());
        tmpDstDcl->setAliasDeclare(unpackDcl, 0);
        inst->setDest(builder.createDstRegRegion(tmpDstDcl, 1));
      }
    }

    // For src2,register regioning patterns where register data bit location of
    // the LSB of the channels are changed between source and destination are
    // not supported except for broadcast of a scalar. The src2 regioning should
    // have been conformed before this function for ternary instructions like
    // mad, add3, etc. But here to resolve the byte XBar restriction, the dst
    // region has been changed. So we need to fix src2 accordingly.
    //
    // For example, before fixByteXBarRestriction the src region is legal:
    //    add3 (8)   V39(0,0)<1>:w  V36(0,0)<0;0>:w  V37(0,0)<4;0>:w
    //    TV8(0,0)<1;0>:w
    // After fixing Byte XBar restriction, src2 region pattern is not the same
    // as dst any more:
    //    add3 (8)   TV10(0,0)<2>:w  V36(0,0)<0;0>:w  V37(0,0)<4;0>:w
    //    TV8(0,0)<1;0>:w mov (8)    V39(0,0)<1>:w  TV10(0,0)<2;1,0>:w
    // So need to fix the src2 region here to make it aligned with dst:
    //    mov (8)    TV11(0,0)<1>:d  TV8(0,0)<1;1,0>:w
    //    add3 (8)   TV10(0,0)<2>:w  V36(0,0)<0;0>:w  V37(0,0)<4;0>:w
    //    TV8(0,0)<1;0>:d mov (8)    V39(0,0)<1>:w  TV10(0,0)<2;1,0>:w
    if (inst->getNumSrc() == 3 && builder.noSrc2Regioning() &&
        inst->getSrc(2)->isSrcRegRegion() &&
        !isGoodAlign1TernarySrc(inst, 2, false)) {
      fixSrc2(it, bb, false);
    }
  }
}

bool HWConformity::fixSrnd(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  if (inst->opcode() != G4_srnd) {
    return false;
  }

  bool changed = false; // return value

  // case 1. src0 cannot be imm.
  // case 2. subreg must be zero  (must be grf-aligned)
  // case 3. For HF->BF8,  both dst and src must be packed
  // case 4. For HF->BF8, no simd1
  G4_DstRegRegion *dst = inst->getDst();
  uint32_t execsize = inst->getExecSize();
  const uint16_t GRFByteSize = builder.numEltPerGRF<Type_UB>();
  bool isHF2BF8 = (dst->getType() == Type_UB);
  if ((!builder.tryToAlignOperand(dst, GRFByteSize) &&
       !builder.enableMovCvt()) || // case 2
      (isHF2BF8 && dst->getHorzStride() != 1 &&
       !builder.enableMovCvt())) // case 3
  {
    G4_Declare *dcl =
        builder.createTempVar(execsize, dst->getType(), builder.getGRFAlign());
    G4_SrcRegRegion *srcRegion = builder.createSrcRegRegion(
        dcl,
        execsize == 1 ? builder.getRegionScalar() : builder.getRegionStride1());
    uint32_t newOption = InstOpt_WriteEnable | inst->getMaskOption();
    G4_INST *newInst = builder.createMov(G4_ExecSize(execsize), dst, srcRegion,
                                         newOption, false);
    bb->insertAfter(it, newInst);

    G4_DstRegRegion *newDst = builder.createDstRegRegion(dcl, 1);
    inst->setDest(newDst);
    changed = true;
  }

  G4_Operand *opnd0 = inst->getSrc(0);
  G4_SrcRegRegion* src0R =
    opnd0->isSrcRegRegion() ? opnd0->asSrcRegRegion() : nullptr;
  if (!src0R ||                                                 // case 1
      !builder.tryToAlignOperand(opnd0, GRFByteSize) ||         // case 2
      (isHF2BF8 && src0R->getRegion()->isContiguous(execsize) &&
       !builder.enableMovCvt()))               // case 3
  {
    G4_Operand *newSrc0 =
        insertMovBefore(it, 0, opnd0->getType(), bb, builder.getGRFAlign());
    inst->setSrc(newSrc0, 0);
    G4_INST *newMovInst = *(std::prev(it));
    newMovInst->setNoMask(true);
    changed = true;
  }

  G4_Operand *opnd1 = inst->getSrc(1);
  G4_SrcRegRegion* src1R =
    opnd1->isSrcRegRegion() ? opnd1->asSrcRegRegion() : nullptr;
  if (src1R && ((!builder.tryToAlignOperand(opnd1, GRFByteSize) &&
                 !builder.enableMovCvt()) || // case 2
                (isHF2BF8 && !src1R->getRegion()->isContiguous(execsize) &&
                 !builder.enableMovCvt()))) // case 3
  {
    G4_Operand *newSrc1 =
        insertMovBefore(it, 1, opnd1->getType(), bb, builder.getGRFAlign());
    inst->setSrc(newSrc1, 1);
    G4_INST *newMovInst = *(std::prev(it));
    newMovInst->setNoMask(true);
    changed = true;
  }

  // SRND src1 operand would need to be changed to
  // 1. f(unsigned 16bits Integer) for FP32 to FP16 conversions
  // 2. hf(unsigned 8bits Integer) for FP16 to BF8 conversions
  //
  //
  //(W) srnd(16|M0) r5.0<1>:hf  r3.0<1;1,0>:f  r12.0<1;1,0>:uw
  //-->
  //(W) mov (16|M0) r13.0<2>:uw  r12.0<1;1,0>:uw
  //(W) srnd(16|M0) r5.0<1>:hf  r3.0<1;1,0>:f  r13.0<1;1,0>:f
  bool typeTransform = !builder.enableMovCvt();

  if (typeTransform &&
      (opnd1->getType() == Type_UB || opnd1->getType() == Type_UW)) {
    G4_Type src0Type = opnd0->getType();
    //We are using src0 to check because for immediate ub, vISA always set to uw.
    G4_Type newType = opnd0->getType() == Type_HF ? Type_UW : Type_UD;
    vISA_ASSERT(TypeSize(src0Type) == TypeSize(newType),
                "src0 and src1 have same size operand type in old platforms");
    G4_Operand *newSrc1 =
        insertMovBefore(it, 1, newType, bb, builder.getGRFAlign());
    newSrc1->asSrcRegRegion()->setType(builder, src0Type);
    inst->setSrc(newSrc1, 1);
    opnd1 = newSrc1;
    changed = true;
  }

  // 3. Scalar broadcast of Src1 would not be supported.
  //(W) srnd(16|M0) r5.0<1>:hf  r3.0<1;1,0>:f  r12.0<0;1,0>:uw
  //-->
  //(W) mov (16|M0) r13.0<1>:uw  r12.0<0;1,0>:uw
  //(W) srnd(16|M0) r5.0<1>:hf  r3.0<1;1,0>:f  r13.0<1;1,0>:uw
  if (!typeTransform && opnd1->asSrcRegRegion()
          ->isScalar()) {
    G4_Type type = opnd1->getType();
    G4_ExecSize execSize = inst->getExecSize();
    uint32_t instMask = inst->getMaskOption();

    // mov(16) r10.0<1>:ub, r20.0<0,1,0>:ub broadcast
    G4_Declare *dcl =
        builder.createTempVar(execSize, type, builder.getGRFAlign());
    G4_DstRegRegion *dst = builder.createDst(dcl->getRegVar(), 0, 0, 1, type);
    G4_INST *newInst = builder.createMov(execSize, dst, opnd1, instMask, false);

    bb->insertBefore(it, newInst);
    const RegionDesc *srcRegion = builder.getRegionStride1();
    G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(dcl, srcRegion);
    inst->setSrc(newSrc, 1);
    src1R = newSrc;
    newInst->addDefUse(inst, inst->getSrcOperandNum(1));
  }

  // Align the dst and src1
  vASSERT(src1R || typeTransform);
  if (!typeTransform && !hasSameSubregOffsetSrnd(dst, src1R)) {
    if (!builder.tryToAlignOperand(src1R, kernel.numEltPerGRF<Type_UB>())) {
      G4_Operand *newSrc1 =
          insertMovBefore(it, 1, src1R->getType(), bb, builder.getGRFAlign());
      inst->setSrc(newSrc1, 1);
      if (dst->getHorzStride() !=
          newSrc1->asSrcRegRegion()->getRegion()->horzStride) {
        newSrc1->asSrcRegRegion()->setRegion(builder,
                                             builder.getRegionStride1());
        G4_INST *newMovInst = *(std::prev(it));
        newMovInst->getDst()->asDstRegRegion()->setHorzStride(1);
      }
      changed = true;
    }

    if (!builder.tryToAlignOperand(dst, kernel.numEltPerGRF<Type_UB>())) {
      G4_DstRegRegion *newDst =
          insertMovAfter(it, dst, dst->getType(), bb, builder.getGRFAlign());
      inst->setDest(newDst);
      if (newDst->getHorzStride() !=
          inst->getSrc(1)->asSrcRegRegion()->getRegion()->horzStride) {
        G4_INST *newMovInst = *(std::next(it));
        newMovInst->getSrc(0)->asSrcRegRegion()->setRegion(
            builder, builder.getRegionStride1());
        newDst->setHorzStride(1);
      }
      changed = true;
    }
  }

  dst = inst->getDst();
  if (isHF2BF8 && inst->getExecSize() == g4::SIMD1) { // case 4
    G4_Declare *rootDcl = dst->getBaseRegVarRootDeclare();
    if (rootDcl->getElemSize() == TypeSize(Type_UB) &&
        rootDcl->getNumElems() == 1) {
      // Extend dcl by one more element so we can do simd2.
      rootDcl->setTotalElems(2);

      G4_Declare *dcl = dst->getBase()->asRegVar()->getDeclare();
      if (dcl != rootDcl) {
        G4_DstRegRegion *nDst =
            builder.createDst(rootDcl->getRegVar(), dst->getType());
        inst->setDest(nDst);
      }
      inst->setExecSize(g4::SIMD2);
    } else {
      //    (W) srnd (1)  r10:ub  r11:hf r12:hf
      // to
      //    (W) srnd (2)  tmp  r11:hf r12:hf
      //    (W) mov  (1)  r10:ub  tmp:ub
      const RegionDesc *ScalarReg = builder.getRegionScalar();
      G4_Declare *nDcl =
          builder.createTempVar(2, dst->getType(), builder.getGRFAlign());
      G4_DstRegRegion *nDst =
          builder.createDst(nDcl->getRegVar(), dst->getType());
      inst->setDest(nDst);
      inst->setExecSize(g4::SIMD2);

      G4_SrcRegRegion *nSrc =
          builder.createSrc(nDcl->getRegVar(), 0, 0, ScalarReg, dst->getType());
      uint32_t newOpt = (InstOpt_WriteEnable | inst->getMaskOption());
      G4_INST *nI = builder.createMov(g4::SIMD1, dst, nSrc, newOpt, false);
      bb->insertAfter(it, nI);
    }
    changed = true;
  }
  return changed;
}

void HWConformity::fixShiftInsts(INST_LIST_ITER i, G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->opcode() != G4_shl && inst->opcode() != G4_shr &&
      inst->opcode() != G4_asr) {
    return;
  }

  auto dst = inst->getDst();
  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);

  bool needWA = false;

  if (builder.getPlatform() == Xe_PVCXT && !IS_QTYPE(dst->getType()) &&
      !IS_QTYPE(src0->getType()) && IS_QTYPE(src1->getType())) {
    needWA = true;
  }

  if (builder.getOption(vISA_forceSrc0ToQwForQwShlWA) &&
      inst->opcode() == G4_shl && IS_QTYPE(dst->getType()) &&
      !IS_QTYPE(src0->getType())) {
    needWA = true;
  }

  if (needWA) {
    G4_Operand *newSrc0 = insertMovBefore(
        i, 0, IS_SIGNED_INT(src0->getType()) ? Type_Q : Type_UQ, bb);
    inst->setSrc(newSrc0, 0);
  }
}

bool HWConformity::hasDedicateAlignRegionConformity(const G4_INST *I) const {
  switch (I->opcode()) {
  case G4_fcvt:
    return true;
  case G4_srnd:
    return true;
  default:
    break;
  }
  return false;
}

// get rid of source modifiers on this inst[srcPos]
void HWConformity::fixSrc1Region(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_Operand *src1 = inst->getSrc(1);

  // need extra move if horzStride >= 4
  if (src1->isSrcRegRegion() &&
      src1->asSrcRegRegion()->getRegion()->horzStride >= 4) {
    G4_Operand *new_src1 = insertMovBefore(it, 1, src1->getType(), bb);
    inst->setSrc(new_src1, 1);
  }
}

INST_LIST_ITER HWConformity::fixMadwInst(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *madwInst = *it;
  auto execSize = madwInst->getExecSize();
  vISA_ASSERT(madwInst->opcode() == G4_madw, "expect madw instruction");

  vISA_ASSERT(execSize != g4::SIMD32,
              "SIMD32 is not supported on this platform for madw");

  auto dst = madwInst->getDst();
  vISA_ASSERT(IS_DTYPE(dst->getType()), "dst only supports DW type");

  auto src0 = madwInst->getSrc(0);
  auto src1 = madwInst->getSrc(1);
  auto src2 = madwInst->getSrc(2);
  vISA_ASSERT(IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()) &&
                   IS_DTYPE(src2->getType()),
               "only DW-type sources are supported");

  bool hasMullh = false;
  if (builder.getOption(vISA_EnableInt32MULLH) ||
      builder.hasSimplifiedRegions()) {
    hasMullh = true;
  }

  if (!hasMullh) {
    // src1 does not support modifier
    checkSrcMod(it, bb, 1);

    // fix src1 region: stride can't exceed 4, otherwise the stride of src1 in
    // the expanded mul will be invalid
    fixSrc1Region(it, bb);
    src1 = madwInst->getSrc(1);

    // fix modifier for src0
    if (!builder.supportSrcModforMul()) {
      checkSrcMod(it, bb, 0);
      src0 = madwInst->getSrc(0);
    }
  }

  // sat cannot be used at all in the macro sequence
  // make the dst GRF-aligned before expanding to macro
  if (madwInst->getSaturate() || (!hasMullh && dst->getHorzStride() != 1) ||
      isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst) ||
      !builder.tryToAlignOperand(dst, builder.getGRFSize())) {
    // add tmp mov instructions
    int dstLowGRFNum = (int)std::ceil(
        (float)(execSize * dst->getExecTypeSize()) / builder.getGRFSize());
    int dstTotalGRFNum = dstLowGRFNum * 2;

    G4_Declare *newDstDcl = builder.createTempVar(
        kernel.numEltPerGRF(dst->getType()) * dstTotalGRFNum, dst->getType(),
        builder.getGRFAlign());

    // add a tmp mov for low results in dst
    G4_Declare *lowMovSrcDcl = builder.createTempVar(
        kernel.numEltPerGRF(dst->getType()) * dstLowGRFNum, dst->getType(),
        builder.getGRFAlign());
    lowMovSrcDcl->setAliasDeclare(newDstDcl, 0);
    G4_SrcRegRegion *lowMovSrc =
        builder.createSrcRegRegion(lowMovSrcDcl, builder.getRegionStride1());
    G4_DstRegRegion *dstLow = nullptr;
    if (dst->isIndirect()) {
      dstLow = builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(),
                                         dst->getHorzStride(), dst->getType(),
                                         dst->getAddrImm());
    } else {
      dstLow = builder.createDst(dst->getBase(), dst->getRegOff(),
                                 dst->getSubRegOff(), dst->getHorzStride(),
                                 dst->getType());
    }
    G4_INST *lowMovInst = builder.createMov(execSize, dstLow, lowMovSrc,
                                            madwInst->getMaskOption(), false);
    lowMovInst->setPredicate(madwInst->getPredicate());
    lowMovInst->setSaturate(madwInst->getSaturate());
    auto insertIter = bb->insertAfter(it, lowMovInst);
    maintainDU4TempMov(madwInst, lowMovInst);

    // add a tmp mov for high results in dst
    G4_Declare *hiMovSrcDcl = builder.createTempVar(
        kernel.numEltPerGRF(dst->getType()) * dstLowGRFNum, dst->getType(),
        builder.getGRFAlign());
    hiMovSrcDcl->setAliasDeclare(newDstDcl,
                                 dstLowGRFNum * builder.getGRFSize());
    G4_SrcRegRegion *hiMovSrc =
        builder.createSrcRegRegion(hiMovSrcDcl, builder.getRegionStride1());
    G4_DstRegRegion *dstHi = nullptr;
    if (dst->isIndirect()) {
      dstHi = builder.createIndirectDst(
          dst->getBase(), dst->getSubRegOff(), dst->getHorzStride(),
          dst->getType(), dst->getAddrImm() + builder.numEltPerGRF<Type_UB>());
    } else {
      dstHi = builder.createDst(dst->getBase(), dst->getRegOff() + dstLowGRFNum,
                                dst->getSubRegOff(), dst->getHorzStride(),
                                dst->getType());
    }
    G4_INST *hiMovInst = builder.createMov(execSize, dstHi, hiMovSrc,
                                           madwInst->getMaskOption(), false);
    hiMovInst->setPredicate(builder.duplicateOperand(madwInst->getPredicate()));
    hiMovInst->setSaturate(madwInst->getSaturate());
    bb->insertAfter(insertIter, hiMovInst);
    maintainDU4TempMov(madwInst, hiMovInst);

    G4_DstRegRegion *newDst = builder.createDstRegRegion(newDstDcl, 1);
    madwInst->setDest(newDst);
    madwInst->setPredicate(nullptr);
    madwInst->setSaturate(g4::NOSAT);
    dst = newDst;
  }

  INST_LIST_ITER retIter = it;
  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_14013677893) &&
      builder.getOption(vISA_expandMadwPostSchedule)) {
       // Here just create tmp variables to fix srcMod, cond modifier, saturate,
       // etc. And Madw->Mul+Mach+Addc+Add expanding will be done in
       // expandMadwPostSchedule pass.

    // need extra mov if dst is acc and src0 is indirect
    if (!builder.accDstforIndirectSrc()) {
      if (src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->getRegAccess() == IndirGRF) {
        madwInst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
        src0 = madwInst->getSrc(0);
      }
    }
    retIter = std::next(it);
  } else if (!hasMullh) {
    // clang-format off
    // SOA layout of dst:(dst_hi32:d, dst_lo32:d)
    // if src2 is not zero, then expand MADW(dst_hi32, dst_lo32) = src0 * src1 + src2 to:
    //     mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
    //     mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d
    //     addc (16) dst_lo32<1>:d  acc0.0<1;1,0>:d  src2<1;1,0>:d     // Low 32 bits
    //     add  (16) dst_hi32<1>:d  acc0.0<1;1,0>:d  dst_hi32<1;1,0>:d // High 32 bits
    // otherwise, expand to:
    //     mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
    //     mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d // High 32 bits
    //     mov  (16) dst_lo32<1>:d  acc0.0<1;1,0>:d                // Low 32 bits
    // clang-format on

    // unset AccWrCtrl
    madwInst->setOptionOff(InstOpt_AccWrCtrl);

    uint32_t origOptions = madwInst->getOption();
    G4_Predicate *origPredicate = madwInst->getPredicate();
    G4_Type tmpType =
        (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()) &&
         IS_UNSIGNED_INT(src2->getType()))
            ? Type_UD
            : Type_D;

    // 1, create a new mul inst
    G4_DstRegRegion *accDstOpnd =
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
    auto newMul =
        builder.createBinOp(origPredicate, G4_mul, execSize, accDstOpnd,
                            builder.duplicateOperand(src0),
                            builder.duplicateOperand(src1), origOptions, false);
    auto startIter = bb->insertBefore(it, newMul);
    madwInst->copyDefsTo(newMul, false);
    // change src1 type to uw type
    fixMulSrc1(std::prev(it), bb);

    // 2, create a mach inst
    int DstHiRegOffset = (int)std::ceil((float)(execSize * TypeSize(tmpType)) /
                                        builder.getGRFSize());
    G4_DstRegRegion *dstHi32 =
        builder.createDst(dst->getBase(), dst->getRegOff() + DstHiRegOffset,
                          dst->getSubRegOff(), 1, tmpType);
    G4_INST *machInst = builder.createMach(
        execSize, dstHi32, builder.duplicateOperand(src0),
        builder.duplicateOperand(src1), origOptions, tmpType);
    machInst->setPredicate(builder.duplicateOperand(origPredicate));
    *it = machInst;
    madwInst->transferUse(machInst);
    madwInst->removeAllDefs();
    newMul->addDefUse(machInst, Opnd_implAccSrc);

    auto endIter = it;
    // optimize: only do multiply if src2 is imme 0
    if (src2->isImm() && src2->asImm()->getImm() == 0) {
      // 3, create a mov inst
      auto dstLo32 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff(), 1, tmpType);
      auto accSrcOpndMov =
          builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                                  : builder.getRegionStride1(),
                            tmpType);
      auto movInst = builder.createMov(execSize, dstLo32, accSrcOpndMov,
                                       origOptions, false);
      movInst->setPredicate(builder.duplicateOperand(origPredicate));
      endIter = bb->insertAfter(endIter, movInst);
    } else {
      // 3, create a addc inst
      //    addc instruction can be :ud data type
      auto dstLo32 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff(), 1,
                                       getUnsignedType(TypeSize(tmpType)));
      auto accSrcOpnd =
          builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                                  : builder.getRegionStride1(),
                            getUnsignedType(TypeSize(tmpType)));
      auto addcSrc1 = builder.duplicateOperand(src2);
      if (addcSrc1->isImm())
        addcSrc1 = builder.createImm(addcSrc1->asImm()->getImm(), Type_UD);
      else
        addcSrc1->asSrcRegRegion()->setType(builder, Type_UD);
      auto addcInst = builder.createBinOp(
          G4_addc, execSize, dstLo32, accSrcOpnd, addcSrc1, origOptions, false);
      addcInst->setPredicate(builder.duplicateOperand(origPredicate));
      addcInst->setImplAccDst(builder.duplicateOperand(accDstOpnd));
      addcInst->setOptionOn(InstOpt_AccWrCtrl);
      endIter = bb->insertAfter(endIter, addcInst);

      // 4, create a add inst
      auto src1Add = builder.createSrc(
          dstHi32->getBase(), dstHi32->getRegOff(), dstHi32->getSubRegOff(),
          execSize == g4::SIMD1 ? builder.getRegionScalar()
                                : builder.getRegionStride1(),
          tmpType);
      auto addInst = builder.createBinOp(
          G4_add, execSize, builder.duplicateOperand(dstHi32),
          builder.duplicateOperand(accSrcOpnd), src1Add, origOptions, false);
      addInst->setPredicate(builder.duplicateOperand(origPredicate));
      endIter = bb->insertAfter(endIter, addInst);
    }

    // split inst if execSize is larger than native execSize
    if (execSize > builder.getNativeExecSize()) {
      splitDWMULInst(startIter, endIter, bb);
      retIter = startIter;
    } else {
      retIter = std::prev(it);
    }
  } else {
    // Create mullh instruction
    // mullh  (16) dst<1>:d    src0<1;1,0>:d    src1<1;1,0>:d
    auto endIter = it;
    uint32_t origOptions = madwInst->getOption();
    G4_Predicate *origPredicate = madwInst->getPredicate();

    auto mullhInst =
        builder.createBinOp(G4_mullh, execSize, builder.duplicateOperand(dst),
                            builder.duplicateOperand(src0),
                            builder.duplicateOperand(src1), origOptions, false);
    mullhInst->setPredicate(origPredicate);
    mullhInst->setOptionOff(InstOpt_AccWrCtrl);
    mullhInst->setVISAId(madwInst->getVISAId());
    madwInst->copyDefsTo(mullhInst, false);
    madwInst->transferUse(mullhInst);
    madwInst->removeAllDefs();
    (*it) = mullhInst;

    if (!(src2->isImm() && src2->asImm()->getImm() == 0)) {
      // When src2 is not immediate 0, generate addc/add to emulate madw
      // addc (16) dst_lo32<1>:d  dst_lo32<1;1,0>:d  src2<1;1,0>:d
      G4_Type tmpType =
          (IS_UNSIGNED_INT(src0->getType()) &&
           IS_UNSIGNED_INT(src1->getType()) && IS_UNSIGNED_INT(src2->getType()))
              ? Type_UD
              : Type_D;

      auto dstLo32 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                       dst->getSubRegOff(), 1, tmpType);
      auto src0Add = builder.createSrc(
          dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
          execSize == g4::SIMD1 ? builder.getRegionScalar()
                                : builder.getRegionStride1(),
          tmpType);
      auto addcInst = builder.createBinOp(G4_addc, execSize, dstLo32, src0Add,
                                          builder.duplicateOperand(src2),
                                          origOptions, false);
      auto *accDstOpnd =
          builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
      addcInst->setPredicate(builder.duplicateOperand(origPredicate));
      addcInst->setImplAccDst(accDstOpnd);
      addcInst->setOptionOn(InstOpt_AccWrCtrl);
      endIter = bb->insertAfter(endIter, addcInst);

      // add  (16) dst_hi32<1>:d  acc0.0<1;1,0>:d    dst_hi32<1;1,0>:d
      int DstHiRegOffset = (int)std::ceil(
          (float)(execSize * TypeSize(tmpType)) / builder.getGRFSize());
      auto *dstHi32 =
          builder.createDst(dst->getBase(), dst->getRegOff() + DstHiRegOffset,
                            dst->getSubRegOff(), 1, tmpType);
      auto src1Add = builder.createSrc(
          dstHi32->getBase(), dstHi32->getRegOff(), dstHi32->getSubRegOff(),
          execSize == g4::SIMD1 ? builder.getRegionScalar()
                                : builder.getRegionStride1(),
          tmpType);
      auto accSrcOpnd =
          builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                                  : builder.getRegionStride1(),
                            tmpType);
      auto addInst = builder.createBinOp(
          G4_add, execSize, builder.duplicateOperand(dstHi32), accSrcOpnd,
          src1Add, origOptions, false);
      addInst->setPredicate(builder.duplicateOperand(origPredicate));
      addInst->setOptionOff(InstOpt_AccWrCtrl);
      endIter = bb->insertAfter(endIter, addInst);
    }
  }

  return retIter;
}

// Currently the local copy propagation phase (newLocalDefHoisting) might be
// too aggressive and could fold a0 register into a select in the float pipe
// which is illegal. We try to fix the instruction in HWConformity because we
// may fix it easily by just flipping the types when it is a raw MOV or a raw
// SEL. This would keep the fp semantics and still save one MOV. Here's an
// example pattern being dealt with.
//
// BEFORE:
// (W&f0.0) sel (1|M0) a0.0<1>:f  r5.2<0;1,0>:f  r3.3<0;1,0>:f
// =>
// AFTER:
// (W&f0.0) sel (1|M0) a0.0<1>:ud r5.2<0;1,0>:ud r3.3<0;1,0>:ud
//
// For others cases, to keep the fp semantics first we create a temp GRF and
// set it as the new dst of the inst. Then we insert a MOV to the old dst (ARF)
// using the int pipe.
//
// BEFORE:
// (W&f0.0) sel (1|M0) (lt)f0.0 a0.0<1>:f  r5.2<0;1,0>:f  r3.3<0;1,0>:f
// =>
// AFTER:
// (W&f0.0) sel (1|M0) (lt)f0.0 r2.0<0;1,0>:f r5.2<0;1,0>:f r3.3<0;1,0>:f
// (W&f0.0) mov (1|M0) a0.0<1>:ud  r2.0<0;1,0>:ud
void HWConformity::fixFloatARFDst(INST_LIST_ITER it, G4_BB *bb) {
  auto isDstTargetedARFInFloat = [](G4_DstRegRegion *dst) -> bool {
    if (!dst || !dst->getTopDcl())
      return false;

    // Currently when ARF is used as an index register in dst, vISA treats
    // the dst as an ARF dst. Skip the IndirGRF case and return true if the
    // dst is ARF/FLAG with a fp type and Direct access. Here's an example
    // pattern.
    // mov (2)              r[A0(0,0), 0]<4>:f  V44(0,0)<1;1,0>:f
    auto regFile = dst->getTopDcl()->getRegFile();
    return (regFile == G4_ADDRESS || regFile == G4_FLAG) &&
           IS_TYPE_FLOAT_ALL(dst->getType()) && (dst->getRegAccess() == Direct);
  };

  auto isRawSel = [](G4_INST *inst) -> bool {
    return inst->opcode() == G4_sel &&
           inst->getDst()->getType() == inst->getSrc(0)->getType() &&
           inst->getDst()->getType() == inst->getSrc(1)->getType() &&
           inst->getCondMod() == nullptr &&
           (inst->getSrc(0)->isSrcRegRegion() &&
            inst->getSrc(0)->asSrcRegRegion()->getModifier() ==
                Mod_src_undef) &&
           (inst->getSrc(1)->isImm() ||
            (inst->getSrc(1)->isSrcRegRegion() &&
             inst->getSrc(1)->asSrcRegRegion()->getModifier() ==
                 Mod_src_undef));
  };

  auto getFlippedIntType = [](G4_Type floatTy) -> G4_Type {
    vASSERT(IS_TYPE_FLOAT_ALL(floatTy));
    switch (TypeSize(floatTy)) {
    case 2:
      return Type_UW;
    case 4:
      return Type_UD;
    case 8:
      return Type_UQ;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected float type size.");
      return Type_UNDEF;
    }
  };

  G4_INST *inst = *it;
  G4_DstRegRegion *dst = inst->getDst();
  if (!isDstTargetedARFInFloat(dst))
    return;

  G4_Type floatTy = dst->getType();
  G4_Type intTy = getFlippedIntType(floatTy);
  if (inst->isRawMov() || isRawSel(inst)) {
    // For raw MOV and raw predicate-based SEL (w/o conditional modifier),
    // we can just flip the types.
    dst->setType(builder, intTy);
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      auto src = inst->getSrc(i);
      if (src->isSrcRegRegion()) {
        src->asSrcRegRegion()->setType(builder, intTy);
      } else if (src->isImm()) {
        inst->setSrc(builder.createImm(src->asImm()->getImm(), intTy), i);
      }
    }
  } else {
    // For others, 2 steps are required.
    // 1. Replace the dst with a temp var in float type.
    G4_Declare *newDefDcl =
        builder.createTempVar(1, floatTy, dst->getTopDcl()->getSubRegAlign());
    inst->setDest(builder.createDstRegRegion(newDefDcl, 1));

    // 2. Create a MOV that moves the temp var to the old dst (ARF).
    G4_Declare *newUseDcl =
        builder.createTempVar(1, intTy, dst->getTopDcl()->getSubRegAlign());
    newUseDcl->setAliasDeclare(newDefDcl, 0);
    const RegionDesc *rd = inst->getExecSize() == 1
                               ? builder.getRegionScalar()
                               : builder.getRegionStride1();
    G4_SrcRegRegion *newSrcRegion = builder.createSrcRegRegion(newUseDcl, rd);
    dst->setType(builder, intTy);
    G4_INST *movInst = builder.createMov(inst->getExecSize(), dst, newSrcRegion,
                                         inst->getMaskOption(), false);
    bb->insertAfter(it, movInst);
  }
}

// There is a 10-bits encoding limit on immediate address offset. So the value
// must be [-512,511].
void HWConformity::fixImmAddrOffsetOOB(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (inst->nonALUInstructions())
    return;

  auto generateAddrAddInst = [&](G4_Operand *var, unsigned int sregOff,
                                 short imm, int execSize) {
    // Generate address add instruction:
    //  add(execSize) A(0,0)<1>:uw  A(0,0)<1;1,0>:uw  imm:w
    auto addrDst = builder.createDst(var->getBase(), 0, sregOff, 1, Type_UW);
    auto addrSrc = builder.createSrc(var->getBase(), 0, sregOff,
                                     execSize == 1 ? builder.getRegionScalar()
                                                   : builder.getRegionStride1(),
                                     Type_UW);
    auto immSrc = builder.createImm(imm, Type_W);
    auto addrAddInst = builder.createInternalInst(
        nullptr, G4_add, nullptr, g4::NOSAT, G4_ExecSize(execSize), addrDst,
        addrSrc, immSrc, InstOpt_WriteEnable);
    return addrAddInst;
  };

  // Indirect operand can be on dst, src0 or src1.
  // Check if dst and src operands have common base.
  G4_VarBase *commonBase = nullptr;
  bool hasCommonBase = false;
  bool isSrcImmAddrOffsetOOB = false;
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc && i < 2; ++i) {
    auto src = inst->getSrc(i);
    if (!src->isIndirect())
      continue;
    auto srcRR = src->asSrcRegRegion();
    isSrcImmAddrOffsetOOB |=
        (srcRR->getAddrImm() > 511 || srcRR->getAddrImm() < -512);
    if (commonBase == nullptr) {
      commonBase = src->getBase();
    } else if (commonBase == src->getBase()) {
      hasCommonBase = true;
    }
  }

  auto dst = inst->getDst();
  bool isDstImmAddrOffsetOOB = false;
  if (dst->isIndirect()) {
    isDstImmAddrOffsetOOB = dst->getAddrImm() > 511 || dst->getAddrImm() < -512;
    hasCommonBase |= (commonBase == dst->getBase());
  }

  if (!isDstImmAddrOffsetOOB && !isSrcImmAddrOffsetOOB)
    return;

  // TODO: dst/src operands are indirect and have common base, and any
  //       operand has invalid immAddrOffset.
  vISA_ASSERT(!hasCommonBase,
              "Unhandled case that dst and src operands are indirect and have "
              "common base, and any ooperand has invalid immAddrOffset");

  // Fix dst
  if (isDstImmAddrOffsetOOB) {
    auto immAddrOff = dst->getAddrImm();
    // Increase dst address register by immAddrOff
    bb->insertBefore(
        it, generateAddrAddInst(dst, dst->getSubRegOff(), immAddrOff, 1));
    // Set immAddrOff of dst as 0
    dst->setImmAddrOff(0);
    // Decrease dst address register by immAddrOff
    bb->insertAfter(
        it, generateAddrAddInst(dst, dst->getSubRegOff(), -immAddrOff, 1));
  }

  if (!isSrcImmAddrOffsetOOB)
    return;

  // Fix src0 and src1
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc && i < 2; ++i) {
    auto src = inst->getSrc(i);
    if (!src->isIndirect())
      continue;

    auto srcRR = src->asSrcRegRegion();
    bool isImmAddrOffsetOOB =
        srcRR->getAddrImm() > 511 || srcRR->getAddrImm() < -512;
    if (!isImmAddrOffsetOOB)
      continue;

    auto immAddrOff = srcRR->getAddrImm();
    auto execSize = srcRR->getRegion()->isRegionWH()
                        ? inst->getExecSize() / srcRR->getRegion()->width
                        : 1;
    // Increase src address register by immAddrOff
    bb->insertBefore(it,
                     generateAddrAddInst(srcRR, srcRR->getSubRegOff(),
                                         immAddrOff, G4_ExecSize(execSize)));
    // // Set immAddrOff of src as 0
    srcRR->setImmAddrOff(0);
    // Decrease src address register by immAddrOff
    bb->insertAfter(it,
                    generateAddrAddInst(srcRR, srcRR->getSubRegOff(),
                                        -immAddrOff, G4_ExecSize(execSize)));
  }
}

void HWConformity::fixBfnInst(G4_BB* bb) {
  INST_LIST_ITER it = bb->begin();

  for (auto iterEnd = bb->end(); it != iterEnd; ++it) {
    G4_INST *inst = *it;
    if (inst->opcode() != G4_bfn) {
      continue;
    }

    // BFN requires its operands to be UD/UW
    // ToDo: anyway to generalize this to all instructions requiring
    // signed/unsigned int type? IGA doesn't seem to have API to query supported
    // types
    auto dst = inst->getDst();
    if (dst->getType() == Type_D || dst->getType() == Type_W) {
      dst->setType(builder, dst->getType() == Type_D ? Type_UD : Type_UW);
    }

    // When create visa immediate operand, we will lower immediate type.
    // For example:
    // bfn.xd8 (M1_NM, 1) V0042(0,0)<1> 0xffff8089:d 0xffffb4d8:d 0xffff895b:d
    // lower to:
    // (W) bfn.0xD8 (1) V0042(0,0)<1>:d  0x8089:w  0xb4d8:w  0x895b:w
    // In this case, the dst is dword type, the immediate source operand should
    // be dword as well. Since HW can only support 16b immediate value, we need
    // to insert mov instruction to resolve it.
    for (int i = 0; i < inst->getNumSrc(); i++)
      if (inst->getSrc(i)->isImm() && IS_DTYPE(inst->getDst()->getType()) &&
          inst->getSrc(i)->getType() == Type_W)
        inst->setSrc(insertMovBefore(it, i, Type_D, bb), i);

    auto changeSrcToUnsigned = [this](G4_Operand *opnd) {
      if (opnd->isSrcRegRegion() &&
          (opnd->getType() == Type_D || opnd->getType() == Type_W)) {
        opnd->asSrcRegRegion()->setType(
            builder, opnd->getType() == Type_D ? Type_UD : Type_UW);
      }
    };
    changeSrcToUnsigned(inst->getSrc(0));
    changeSrcToUnsigned(inst->getSrc(1));
    changeSrcToUnsigned(inst->getSrc(2));
  }
}
