/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _HWCONFORMITYXE3P_H_
#define _HWCONFORMITYXE3P_H_

#include "BuildIR.h"
#include "PointsToAnalysis.h"

namespace vISA {

class HWConformityPro {
  IR_Builder &builder;
  PointsToAnalysis &pointsToAnalysis;

public:
  HWConformityPro(IR_Builder &b, PointsToAnalysis &p2a)
      : builder(b), pointsToAnalysis (p2a){}
  void checkHWConformity();
  void avoidDstSrcOverlap(PointsToAnalysis &p);
  void avoidInstDstSrcOverlap(INST_LIST_ITER it, G4_BB *bb,
                              PointsToAnalysis &p);

private:

  bool checkDPASSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb);

  G4_INST *evenlySplitDPAS8x8Inst(INST_LIST_ITER iter, G4_BB *bb);

  // ******** HW conformity check functions *********
  void fixSpecificInstRestricts(G4_BB *bb);

  void fixGeneralRestrictsOnRegionParameters(G4_BB *bb);

  void fixRegRegionRestricts(G4_BB *bb);

  void fixRawMovRegRegionRestrictions(G4_BB *bb);

  void fixRegRegionMathPipe(INST_LIST_ITER it, G4_BB *bb);

  void fixRegRegionFloatPipe(INST_LIST_ITER it, G4_BB *bb);

  void fixRegRegionIntPipe(INST_LIST_ITER it, G4_BB *bb);

  bool isAllowedTrueRegionPatternOnSrc0(G4_Operand *src);

  void fixSendg(INST_LIST_ITER it, G4_BB *bb);

  void fixCalla(INST_LIST_ITER it, G4_BB *bb);

  void fixBfn(INST_LIST_ITER it, G4_BB *bb);

  void fixDPAS(INST_LIST_ITER it, G4_BB *bb);

  void fixSelCsel(INST_LIST_ITER it, G4_BB *bb);

  void fixCmpInst(INST_LIST_ITER it, G4_BB *bb);

  void fixRotate(INST_LIST_ITER it, G4_BB *bb);

  void fixMul(INST_LIST_ITER it, G4_BB *bb);
  void fixMulDataTypes(INST_LIST_ITER it, G4_BB *bb);

  void fixMulh(INST_LIST_ITER it, G4_BB *bb);

  void fixMad(INST_LIST_ITER it, G4_BB *bb);

  void fixMadw(INST_LIST_ITER it, G4_BB *bb);

  void fixFcvt(INST_LIST_ITER it, G4_BB *bb);

  void fixSrnd(INST_LIST_ITER it, G4_BB *bb);

  void fixMov(INST_LIST_ITER it, G4_BB *bb);

  void fixLzd(INST_LIST_ITER it, G4_BB *bb);

  void fixLfsr(INST_LIST_ITER it, G4_BB *bb);

  void fixAdd(INST_LIST_ITER it, G4_BB *bb);

  void fixFloatMixedModeAndPureBFMode(INST_LIST_ITER it, G4_BB *bb);

  void fix3SrcInstEncodeRestriction(INST_LIST_ITER it, G4_BB *bb);

  void fixDstSrcOverlap(INST_LIST_ITER it, G4_BB *bb);

  void fixIndiret(G4_BB *bb);

  void fixVxHVx1Indirect(INST_LIST_ITER it, G4_BB *bb);

  void fix1x1Indirect(INST_LIST_ITER it, G4_BB *bb);

  void fixIndirectMoviSimd16ToSimd8(INST_LIST_ITER it, G4_BB *bb);

  void fixImmAddrOffsetOOB(INST_LIST_ITER it, G4_BB *bb);

  void fixPredCtrl(INST_LIST_ITER it, G4_BB *bb);

  bool generateMad(INST_LIST_ITER it, G4_BB *bb);

  void convertMAD2MulAdd(INST_LIST_ITER it, G4_BB *bb);

  void doTranslateMulh(INST_LIST_ITER it, G4_BB *bb);

  void fix2SrcInstImm(INST_LIST_ITER it, G4_BB *bb);

  void fixAddcSubb(INST_LIST_ITER it, G4_BB *bb);

  void fixVectImm(INST_LIST_ITER it, G4_BB *bb);

  void fixImm64(INST_LIST_ITER it, G4_BB *bb);

  void fixAccRestrictions(INST_LIST_ITER it, G4_BB *bb);

  void fixMovCvtBetweenFp16AndFp32(INST_LIST_ITER it, G4_BB *bb);

  void fixMovCvtBetweenFp16AndWordByte(INST_LIST_ITER it, G4_BB *bb);

  void fixARF(INST_LIST_ITER it, G4_BB *bb);

  void fixFbl(INST_LIST_ITER it, G4_BB *bb);

  void fixShr(INST_LIST_ITER it, G4_BB *bb);

  // ********** HW conformity help functions ***********

  G4_Kernel &kernel() { return builder.kernel; }

  // This function is to get the alignment of the temp variable used in
  // insertMovBefore() and insertMovAfter().
  G4_SubReg_Align getDclAlignment(int opndBytes, G4_INST *inst, bool isScalar);

  // Before inst <*it>, insert a mov instruction from src<srcNum> to a tmp
  // variable with customized type, stride, alignment. If src is null reg
  // no mov will be inserted.
  // Pair's first is a new src operand and pair's second is boolean stating
  // whether mov was inserted or not.
  std::pair<G4_Operand *, bool> insertMovBeforeAndGetInserted(INST_LIST_ITER it,
                                                              G4_BB *bb,
                                                              uint32_t srcNum,
                                                              G4_Type type,
                                                              uint16_t tmpStride,
                                                              G4_SubReg_Align tmpAlign);
  G4_Operand *insertMovBefore(INST_LIST_ITER it,
                              G4_BB *bb,
                              uint32_t srcNum,
                              G4_Type type,
                              uint16_t tmpStride,
                              G4_SubReg_Align tmpAlign) {
    return insertMovBeforeAndGetInserted(it, bb, srcNum, type, tmpStride, tmpAlign).first;
  }

  // After inst <*it>, insert a mov instruction from a tmp variable to dst with
  // customized type, stride, alignment. If inst has null dst or dst is nullptr
  // no mov will be inserted.
  // Pair's first is a new DstRegRegion and pair's second is boolean stating
  // whether mov was inserted or not.
  std::pair<G4_DstRegRegion *, bool> insertMovAfterAndGetInserted(INST_LIST_ITER it,
                                                                  G4_BB *bb,
                                                                  G4_DstRegRegion *dst,
                                                                  G4_Type type,
                                                                  uint16_t tmpStride,
                                                                  G4_SubReg_Align dstAlign);
  G4_DstRegRegion *insertMovAfter(INST_LIST_ITER it,
                                  G4_BB *bb,
                                  G4_DstRegRegion *dst,
                                  G4_Type type,
                                  uint16_t tmpStride,
                                  G4_SubReg_Align dstAlign) {
    return insertMovAfterAndGetInserted(it, bb, dst, type, tmpStride, dstAlign).first;
  }

  // Replace src<srcNum> for inst <*it> with a temp variable of type <type>.
  // The original src is now the src of a new mov to the tmp variable unless
  // original src was null reg, in which case no new mov was created.
  // This is used to satisfy various HW restrictions on src
  // type/alignment/region/modifier.
  void replaceSrc(INST_LIST_ITER it,
                  G4_BB *bb,
                  uint32_t srcNum,
                  G4_Type type,
                  uint16_t stride,
                  G4_SubReg_Align tmpAlign) {
    replaceSrcWasMovInserted(it, bb, srcNum, type, stride, tmpAlign);
  }

  bool replaceSrcWasMovInserted(INST_LIST_ITER it,
                                G4_BB *bb,
                                uint32_t srcNum,
                                G4_Type type,
                                uint16_t stride,
                                G4_SubReg_Align tmpAlign) {
    G4_INST *inst = *it;
    auto [newSrc, wasMovInserted] = insertMovBeforeAndGetInserted(it, bb, srcNum, type, stride, tmpAlign);
    inst->setSrc(newSrc, srcNum);

    return wasMovInserted;
  }

  // Replace src<srcNum> for inst <*it> with a temp variable of the same type.
  // The original src is now the src of a new mov to the tmp variable. If the
  // original src has source modifier, the source modifier will be moved to
  // the new src of the inst. The new added mov instruction is a raw mov.
  // his is used to satisfy various HW restrictions on src alignment/region.
  void replaceSrcWithRawMov(INST_LIST_ITER it,
                            G4_BB* bb,
                            uint32_t srcNum,
                            uint16_t stride,
                            G4_SubReg_Align tmpAlign) {
    G4_INST *inst = *it;
    inst->setSrc(insertMovBefore(it, bb, srcNum,
                                 inst->getSrc(srcNum)->getType(), stride,
                                 tmpAlign),
                 srcNum);
    G4_INST *newMov = *(std::prev(it));
    if (newMov->getSrc(0)->isSrcRegRegion() &&
        newMov->getSrc(0)->asSrcRegRegion()->getModifier() != Mod_src_undef) {
      inst->getSrc(srcNum)->asSrcRegRegion()->setModifier(
          newMov->getSrc(0)->asSrcRegRegion()->getModifier());
      newMov->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
    }
  }

  // Replace dst for inst <*it> with a temp variable of type <type>. The
  // original dst is now the dst of a new mov instruction from the temp
  // variable, unless dst was null in which case no new mov was created.
  // If the inst has saturate, the new mov instruction may also have
  // saturate.
  // This is used to satisfy various HW restrictions on dst
  // type/region/alignment.
  void replaceDst(INST_LIST_ITER it,
                  G4_BB *bb,
                  G4_Type type,
                  uint16_t tmpStride,
                  G4_SubReg_Align dstAlign) {
    replaceDstWasMovInserted(it, bb, type, tmpStride, dstAlign);
  }

  bool replaceDstWasMovInserted(INST_LIST_ITER it,
                                G4_BB *bb,
                                G4_Type type,
                                uint16_t tmpStride,
                                G4_SubReg_Align dstAlign) {
    G4_INST *inst = *it;
    auto originalDstType = inst->getDst()->getType();
    auto [newDest, wasMovInserted] = insertMovAfterAndGetInserted(it, bb, inst->getDst(), type, tmpStride, dstAlign);
    inst->setDest(newDest);

    if (wasMovInserted) {
      G4_INST *newMov = *(std::next(it));
      newMov->setSaturate(inst->getSaturate());

      if (type == originalDstType) {
        newMov->setSaturate(g4::NOSAT);
      }
    }
    if (type != originalDstType && IS_TYPE_FLOAT_ALL(type)) {
      inst->setSaturate(g4::NOSAT);
    }

    return wasMovInserted;
  }

  // Replace dst for inst <*it> with a tmp variable of the same type. The
  // original dst is now the dst of a new move instruction from the temp
  // variable.The new added mov instruction is a raw mov.
  // This is used to satisfy various HW restrictions on dst region/alignment.
  void replaceDstWithRawMov(INST_LIST_ITER it, G4_BB* bb, uint16_t tmpStride,
    G4_SubReg_Align dstAlign) {
    G4_INST *inst = *it;
    inst->setDest(insertMovAfter(it, bb, inst->getDst(),
                                 inst->getDst()->getType(), tmpStride,
                                 dstAlign));
  }

  // check if all sources are aligned to dst
  bool isAllSrcsAlignedToDst(
      G4_INST *inst, uint8_t exChannelWidth,
      std::function<bool(uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
                         uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
                         uint8_t exChannelWidth)> checkAlignmentFunc);

  bool nonALUInstructions(G4_INST* inst) {
    return inst->isSend() || inst->isLabel() || inst->isCFInst() ||
           inst->isDpas() || inst->isIntrinsic() || inst->opcode() == G4_nop;
  }

  uint16_t getSrcStrideInBytes(G4_SrcRegRegion *src);

  void evenlySplitInst(INST_LIST_ITER iter, G4_BB *bb);
};
} // namespace vISA
#endif /* _HWCONFORMITYXE3P_H_ */
