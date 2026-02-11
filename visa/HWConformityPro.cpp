/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HWConformityPro.h"

using namespace vISA;

void HWConformityPro::checkHWConformity() {
  for (auto bb : kernel().fg) {
    //Fix specific instruction restrictions
    fixSpecificInstRestricts(bb);

    // Fix general restrictions on regioning parameters
    fixGeneralRestrictsOnRegionParameters(bb);

    // Fix register region restrictions
    // Make sure in this function all the new added mov instructions except for
    // raw move have not register region issue
    fixRegRegionRestricts(bb);

    // Fix restrictions for indirect operands
    // Make sure this function is called after fixRegRegionRestricts
    fixIndiret(bb);

    // Fix register region restrictions for raw MOV instructions
    fixRawMovRegRegionRestrictions(bb);

    // With native SIMD32 enabled, only the operands with 64b
    // datatypes and contiguous or scalar regions in SIMD32 ALU instructions
    // can span more than 2 GRFs. This is the limitation of current footprint
    // implementation which assumes all bits are set if the operand spans
    // more than 2 GRFs. HWConformity may generate invalid instructions
    // whose operands span more than 2 GRFs. For example:
    // Before HWConformity:
    //    mul (32)  V0007(0,0)<1>:q  V0008(0,0)<1;1,0>:d  0x3:w
    // After HWConformity:
    //    mov (32)  TV1(0,0)<2>:d  V0008(0,0)<1;1,0>:d
    //    mul (32)  V0007(0,0)<1>:q  TV1(0,0)<2;1,0>:d  0x3:w
    // The TV1 operand in both instructions spans more than 2 GRFs, but the
    // region is not scalar or contiguous. Hence, need to fix by splitting it.
    InstSplitPass instSplitter(&builder);
    instSplitter.runOnBB(bb);
  }
}

void HWConformityPro::fixSpecificInstRestricts(G4_BB *bb) {
  for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
    G4_INST *inst = *it;
    auto opcode = inst->opcode();

    if (inst->isSendg())
      fixSendg(it, bb);

    if (inst->isFCall())
      fixCalla(it, bb);

    if (inst->isBfn())
      fixBfn(it, bb);

    if (inst->isDpas())
      fixDPAS(it, bb);

    if (opcode == G4_sel || opcode == G4_csel)
      fixSelCsel(it, bb);

    if (inst->isCompare())
      fixCmpInst(it, bb);

    if (opcode == G4_ror || opcode == G4_rol)
      fixRotate(it, bb);

    if (opcode == G4_mul)
      fixMul(it, bb);

    if (opcode == G4_mulh)
      fixMulh(it, bb);

    if (opcode == G4_pseudo_mad)
      fixMad(it, bb);

    if (opcode == G4_madw)
      fixMadw(it, bb);

    if(opcode == G4_fcvt)
      fixFcvt(it, bb);

    if (opcode == G4_srnd)
      fixSrnd(it, bb);

    if (opcode == G4_mov)
      fixMov(it, bb);

    if (opcode == G4_lzd)
      fixLzd(it, bb);

    if (opcode == G4_addc || opcode == G4_subb)
      fixAddcSubb(it, bb);

    if (opcode == G4_lfsr)
      fixLfsr(it, bb);

    if (opcode == G4_fbl)
      fixFbl(it, bb);

    if (opcode == G4_shr)
      fixShr(it, bb);

    if (opcode == G4_add)
      fixAdd(it, bb);
  }
}

void HWConformityPro::fixRegRegionRestricts(G4_BB *bb) {
  for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
    G4_INST *inst = *it;
    auto opcode = inst->opcode();

    fixPredCtrl(it, bb);

    // Fix ARF restriction
    fixARF(it, bb);

    // Fix ACC restrictions
    fixAccRestrictions(it, bb);

    // Fix dst and src overlap
    fixDstSrcOverlap(it, bb);

    // G4_srnd is handled in fixFcvt() separately
    // G4_srnd is handled in fixSrnd() separately
    // Raw MOV is handled in fixRawMovRegRegionRestrictions()
    if (inst->nonALUInstructions() || inst->isRawMov() ||
        opcode == G4_srnd ||opcode == G4_fcvt)
      continue;

    // Fix 64-bit immediate source operand
    fixImm64(it, bb);

    // Fix BF/F and HF/F mixed mode, and pure BF mode
    // Make sure this function is called before fixing reg region for
    // math/float/int pipelines
    fixFloatMixedModeAndPureBFMode(it, bb);

    // Fix encoding restrictions for ternary instructions
    if (inst->getNumSrc() == 3 && inst->opcode() != G4_madm)
      fix3SrcInstEncodeRestriction(it, bb);

    // Fix imm of src operand for 2 sources instructions
    fix2SrcInstImm(it, bb);

    // Fix vector immediate source operand
    fixVectImm(it, bb);

    // fix register region restrictions for math pipeline
    if (inst->isMath()) {
      fixRegRegionMathPipe(it, bb);
      continue;
    }

    // fix register region restrictions for float pipeline
    if (inst->isFloatPipeInstructionXe() || inst->isLongPipeInstructionXe()) {
      fixRegRegionFloatPipe(it, bb);
      continue;
    }

    // fix register region restrictions for integer pipeline
    if (inst->isIntegerPipeInstructionXe()) {
      fixRegRegionIntPipe(it, bb);
      continue;
    }
  }
}

// Register restrictions for math pipeline:
// 1, All srcs must be flat region except that src is broadcast of a single
//    channel from GRF register.
// 2, Packed dst where dst datatype size times dst stride is less than the
//    execution datatype size is not allowed.
void HWConformityPro::fixRegRegionMathPipe(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isMath(), "Expect math instruction");

  // math macros are constructed internally and should have been comformed
  if (inst->asMathInst()->getMathCtrl() == MATH_INVM ||
      inst->asMathInst()->getMathCtrl() == MATH_RSQRTM ||
      inst->getExecSize() == g4::SIMD1) {
    return;
  }

  // Check flat region:
  // 1, Starting bit position of a channel in a register is not shift between
  //    src and dst, or
  // 2, Src/dst are all aliged to either the lower or upper fp16 within the
  //    same fp32 execution channel if the src/dst is fp16 datatype.
  auto checkFlatRegRegionFunc = [&](uint8_t dstStrideInBytes,
                                     uint8_t dstSubRegOffInBytes,
                                     uint8_t srcStrideInBytes,
                                     uint8_t srcSubRegOffInBytes,
                                     uint8_t exChannelWidth) -> bool {
    bool noBitPosShiftDstSrc = false;
    if (builder.supportPureBF())
      noBitPosShiftDstSrc = (dstSubRegOffInBytes == srcSubRegOffInBytes) &&
                            (dstStrideInBytes == srcStrideInBytes);
    return (noBitPosShiftDstSrc || ((dstSubRegOffInBytes / exChannelWidth ==
                                     srcSubRegOffInBytes / exChannelWidth) &&
                                    (dstStrideInBytes == srcStrideInBytes) &&
                                    (dstStrideInBytes % exChannelWidth == 0)));
  };

  // The execution channel is always fp32 for math pipeline
  uint8_t exChannelWidth = inst->getExecTypeSizeXe3p();

  if (isAllSrcsAlignedToDst(inst, exChannelWidth, checkFlatRegRegionFunc))
    return;

  auto dst = inst->getDst();
  uint16_t dstStrideInBytes = dst->getExecTypeSize();
  if ((dst->isIndirect() ||
       (!builder.supportPureBF() && dstStrideInBytes % exChannelWidth != 0) ||
       !builder.tryToAlignOperand(dst, builder.numEltPerGRF<Type_UB>()))) {
    replaceDstWithRawMov(
        it, bb, /*tmpStride*/
        builder.supportPureBF() ? 1 : exChannelWidth / TypeSize(dst->getType()),
        builder.getGRFAlign());
    dst = inst->getDst();
  }

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (!src->isSrcRegRegion())
      continue;
    auto srcRR = src->asSrcRegRegion();
    dstStrideInBytes = dst->getExecTypeSize();
    uint16_t srcStrideInBytes = getSrcStrideInBytes(srcRR);
    if (srcRR->isIndirect() ||
        !(srcRR->isFlatRegRegion(exChannelWidth, checkFlatRegRegionFunc) ||
        srcRR->isScalar() ||
          (dstStrideInBytes == srcStrideInBytes &&
           builder.tryToAlignOperand(src, builder.numEltPerGRF<Type_UB>())))) {
      replaceSrcWithRawMov(it, bb, i,
                 dstStrideInBytes / TypeSize(src->getType()),
                 builder.getGRFAlign());
    }
  }
}

// Register restrictions for float pipeline:
// 1, All srcs must be flat region except that src is broadcast of a single
//    channel from GRF register.
// 2, Packed dst where dst datatype size times dst stride is less than the
//    execution datatype size is not allowed.
void HWConformityPro::fixRegRegionFloatPipe(INST_LIST_ITER it,
  G4_BB* bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isFloatPipeInstructionXe() ||
                  inst->isLongPipeInstructionXe(),
              "Expect float/long pipe instructions!");
  if (inst->isRawMov() || inst->getExecSize() == g4::SIMD1)
    return;

  // Check flat region:
  // Starting bit position of a channel in a register is not shift between src
  // and dst.
  auto checkFlatRegRegionFunc = [](uint8_t dstStrideInBytes,
                                     uint8_t dstSubRegOffInBytes,
                                     uint8_t srcStrideInBytes,
                                     uint8_t srcSubRegOffInBytes,
                                     uint8_t exChannelWidth) -> bool {
    return ((dstSubRegOffInBytes == srcSubRegOffInBytes) &&
            (dstStrideInBytes == srcStrideInBytes) &&
            (dstStrideInBytes % exChannelWidth == 0));
  };

  // Get execution datatype size
  uint8_t exChannelWidth = inst->getExecTypeSizeXe3p();

  if (isAllSrcsAlignedToDst(inst, exChannelWidth, checkFlatRegRegionFunc))
    return;

  auto dst = inst->getDst();
  uint16_t dstStrideInBytes = dst->getExecTypeSize();
  if (dst->isIndirect() ||
      dstStrideInBytes % exChannelWidth != 0 ||
      !builder.tryToAlignOperand(dst, builder.numEltPerGRF<Type_UB>())) {
    replaceDstWithRawMov(it, bb,
               exChannelWidth / TypeSize(dst->getType()),
               builder.getGRFAlign());
    dst = inst->getDst();
  }

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (src->isSrcRegRegion()) {
      auto srcRR = src->asSrcRegRegion();
      dstStrideInBytes = dst->getExecTypeSize();
      uint16_t srcStrideInBytes = getSrcStrideInBytes(srcRR);
      if (srcRR->isIndirect() ||
          !(srcRR->isFlatRegRegion(exChannelWidth, checkFlatRegRegionFunc) ||
            srcRR->isScalar() ||
            (dstStrideInBytes == srcStrideInBytes &&
             builder.tryToAlignOperand(src,
                                       builder.numEltPerGRF<Type_UB>())))) {
        replaceSrcWithRawMov(it, bb, i,
                   dstStrideInBytes / TypeSize(src->getType()),
                   builder.getGRFAlign());
      }
    }
  }
}

static bool allowTrueRegionOnSrc0(G4_INST *inst) {
  // True region is allowed between src0 and dst when the instruction is not
  // mul and src2 is not scalar or immediate.
  if (inst->opcode() == G4_mul || inst->opcode() == G4_mullh)
    return false;

  // src0 is not accumulator
  auto src0 = inst->getSrc(0);
  if (src0->isAccReg())
    return false;

  // TODO: optimize by swapping src1(not scalar) and src2(scalar) if src1 and
  // src2 are swappable.
  auto src2 = inst->getSrc(2);
  if (!src2 || src2->isNullReg())
    return true;

  return !(src2->isImm() || src2->asSrcRegRegion()->isScalar());
}

// Alignment rule for down conversion from fp32 to fp16(bf/hf):
//    1, If dst is packed(stride is 1) with subreg offset .0/.16, src must be
//    packed(stride is 1) as well with subreg offset .0 only for fp32 and must
//    not span more than 1 register.
//    2, Otherwise, follow the int pipeline rules to check register region
//    restrictions which treat fp16 to int16 and fp32 to int32.
// For up conversion from fp16(bf/hf) to fp32, follow the intpipeline rules
// which treat fp16 to int16 and fp32 to int32.
void HWConformityPro::fixMovCvtBetweenFp16AndFp32(INST_LIST_ITER it,
  G4_BB* bb) {
  auto inst = *it;
  auto dst = inst->getDst();
  auto src = inst->getSrc(0);
  auto srcType = src->getType();
  auto dstType = dst->getType();

  // Conversions between FP32 and BF16 must not use Predication, Conditional
  // Modifiers, Saturation and Source Modifiers.
  if (IS_BFTYPE(dstType) || IS_BFTYPE(srcType)) {
    // Legalize unsupported saturation and predicate for bf<->f conversion:
    // (P0) mov.sat dst:f src0:bf
    // =>
    // mov tmp.f src0:bf
    // (P0) mov.sat dst:f tmp:f
    if (inst->getSaturate() || inst->getPredicate()) {
      replaceDst(it, bb, dstType, /*tmpStride*/ 0, Any);
      dst = inst->getDst();
      G4_INST *newMov = *(std::next(it));
      if (inst->getSaturate()) {
        newMov->setSaturate(inst->getSaturate());
        inst->setSaturate(g4::NOSAT);
      }
      if (inst->getPredicate())
        newMov->setPredicate(inst->getPredicate());
      inst->setPredicate(nullptr);
    }

    // Legalize unsupported src modifier
    if (src->isSrcRegRegion() &&
        src->asSrcRegRegion()->getModifier() != Mod_src_undef) {
      bool wasMovInserted =
          replaceSrcWasMovInserted(it, bb, 0, srcType, /*tmpStride*/ 0, Any);
      // Further fix the new added mov as it is not a raw mov
      if (wasMovInserted)
        fixRegRegionFloatPipe(std::prev(it), bb);
      src = inst->getSrc(0);
    }

    vISA_ASSERT(!inst->getPredicate() && !inst->getCondMod(),
                "F<->BF move does not support pred/cond mod");
    if (src->isSrcRegRegion())
      vISA_ASSERT(src->asSrcRegRegion()->getModifier() == Mod_src_undef,
                  "F<->BF move does not support source modifier");
    vISA_ASSERT(!inst->getSaturate(),
                "F<->BF move does not support saturation");
  }

  // For up conversion, we can skip the checks as it's always allowed
  // regioning patterns due to true regioning support on src0
  if (IS_FTYPE(dstType))
    return;

  // Handle down conversion restrictions
  bool isPackedDst = dst->getHorzStride() == 1;
  // Dst subreg offset is .0 or .16
  bool isValidDstOffset = builder.tryToAlignOperand(dst, 32);
  bool isSrcScalar =
      src->isImm() || src->asSrcRegRegion()->getRegion()->isScalar();
  // For simd32 instruction, do not do packed layout support as src always spans
  // more than 1 register.
  // For packed down converts HW doesn't support scalar broadcast on src. So,
  // do not do packed regioning fix.
  if (isPackedDst && isValidDstOffset && inst->getExecSize() != g4::SIMD32 &&
      !isSrcScalar) {
    bool isPackedSrc =
        src->asSrcRegRegion()->getRegion()->isContiguous(inst->getExecSize());
    // Src must be packed with offset .0 only
    if (!isPackedSrc ||
        !builder.tryToAlignOperand(src, builder.numEltPerGRF<Type_UB>()))
      replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
    return;
  }

  // Must follow the restrictions of int pipeline, which treat hf/bf as int16
  // and fp32 as int32

  // Packed destination, where destination datatype times destination stride
  // is less than the execution datatype is not allowed if execution size is
  // more than one.
  auto dstStrideInBytes = dst->getTypeSize() * dst->getHorzStride();
  auto execChannelWidth = inst->getExecTypeSizeXe3p();
  if (inst->getExecSize() != g4::SIMD1 &&
      (dstStrideInBytes < execChannelWidth ||
       !isAllowedTrueRegionPatternOnSrc0(src)))
    // Fix by moving dst to a tmp with same datatype and dword-aligned
    replaceDstWithRawMov(it, bb, 4 / TypeSize(dstType), builder.getGRFAlign());

  return;
}

// Format Conversion Operations between word/byte and fp16:
// 1, fp32 execution channel is used when source operand is fp16 and
//    destination operand is word/byte.
// 2, dword execution channel is used when source operand is word/byte
//    and destination operand is fp16
// 3, Source and destination must be dword-aligned during format conversion
//    between word/byte and fp16
void HWConformityPro::fixMovCvtBetweenFp16AndWordByte(INST_LIST_ITER it,
  G4_BB* bb) {
  auto inst = *it;
  auto dst = inst->getDst();
  auto src = inst->getSrc(0);
  auto dstTy = dst->getType();
  auto srcTy = src->getType();

  uint8_t exChannelWidth = inst->getExecTypeSizeXe3p();
  vISA_ASSERT(exChannelWidth == 4,
              "execution channel width must be 4 for format conversion "
              "operations between fp16 and word/byte");

  // Fix dst
  auto dstStrideInBytes = TypeSize(dstTy) * dst->getHorzStride();
  if (dstStrideInBytes % exChannelWidth != 0 ||
      !builder.tryToAlignOperand(inst->getDst(), exChannelWidth))
    replaceDstWithRawMov(it, bb, exChannelWidth / TypeSize(dstTy) /*stride*/,
                         Even_Word);

  // Fix src
  if (src->isImm())
    return;

  auto srcRR = src->asSrcRegRegion();
  if (srcRR->isScalar()) {
    if (!builder.tryToAlignOperand(inst->getSrc(0), exChannelWidth))
      replaceSrcWithRawMov(it, bb, 0, 0 /*stride*/, Even_Word);
  } else {
    auto srcStrideInBytes = getSrcStrideInBytes(srcRR);
    if (srcStrideInBytes == 0 || srcStrideInBytes % exChannelWidth != 0 ||
        !builder.tryToAlignOperand(inst->getSrc(0), exChannelWidth))
      replaceSrcWithRawMov(
          it, bb, 0, exChannelWidth / TypeSize(srcTy) /*stride*/, Even_Word);
  }
  return;
}

// Restrictions for int pipeline:
// 1, src0 restrictions:
//    a, If opcode is non-mul and src2 is not broadcast, true region is allowed.
//       Need to follow the bspec psedo code for allowed regioning patterns:
//       https://gfxspecs.intel.com/Predator/Home/Index/73578
//    b, Otherwise, must be flat region except that src is broadcast of a
//       single channel from GRF register.
// 2, src1/src2 restrictions:
//    a, Byte data type is not supported.
//    b, must be flat region except that src is broadcast of a single channel
//       from GRF register.
// 3, Dst restrictions:
//    Register regioning patterns where the starting bit position of a channel
//    is shifted between execution channel and destination register are not
//    supported except when
//    -destination is word (UW, W), the destination subregnum can be aligned
//     to word 0 or 1 of the dword execution channel e.g. a source is dword.
//    -destination is byte (UB, B), the destination subregnum can be aligned
//     to byte 0, 1, 2 or 3 of the dword execution channel e.g. a source is
//     dword.
//    -destination is byte (UB, B), the destination subregnum can be aligned
//     to byte 0 or 1 of the word execution channel e.g. a source is word.
void HWConformityPro::fixRegRegionIntPipe(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isIntegerPipeInstructionXe(),
              "Expect int pipe instructions!");

  // G4_fcvt and G4_srnd are fixed separately
  if (inst->opcode() == G4_fcvt || inst->opcode() == G4_srnd ||
      inst->isRawMov())
    return;

  // Starting bit position of a channel in a register is not shift between src
  // and dst
  auto checkFlatRegRegionFunc =
      [](uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
         uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
         uint8_t exChannelWidth) -> bool {
    return ((dstSubRegOffInBytes == srcSubRegOffInBytes) &&
            (dstStrideInBytes == srcStrideInBytes) &&
            (dstStrideInBytes % exChannelWidth == 0));
  };

  // Qword Aligned rules:
  // Dst is aligned to qwords, e.g. even dwords with stride 2, etc.
  // Src should meet either of below conditions:
  // 1, dwords on source must be strided to all align to either dword0 or dword1
  // 2, words on source must be strided to all align to either word0 or word2
  auto checkQwAlignedRegionFunc =
      [](uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
         uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
         uint8_t exChannelWidth) -> bool {
    return (dstStrideInBytes % 8 == 0 && dstStrideInBytes == srcStrideInBytes &&
            dstSubRegOffInBytes / 8 == srcSubRegOffInBytes / 8 &&
            srcSubRegOffInBytes % 4 == 0);
  };

  uint8_t exChannelWidth = inst->getExecTypeSizeXe3p();
  // Byte packed dst is not allowed except for raw mov
  bool isDstBytePacked = IS_BTYPE(inst->getDst()->getType()) &&
                         inst->getDst()->getHorzStride() == 1;
  if (isDstBytePacked) {
    // Gen doesn't support hstride 8, so we add a W move here
    if (exChannelWidth == 8)
      replaceDst(it, bb, Type_W, exChannelWidth / TypeSize(Type_W),
                 builder.getGRFAlign());
    else if (inst->getExecSize() == g4::SIMD1)
      // Enlarge dst stride to avoid extra mov for SIMD1 case:
      // (p0.0) sel (1|M8) v1(0,0)<1>:ub v2(0,0)<0;1,0>:uw v3(0,0)<0;1,0>:uw
      // =>
      // (p0.0) sel (1|M8) v1(0,0)<2>:ub v2(0,0)<0;1,0>:uw v3(0,0)<0;1,0>:uw
      inst->getDst()->setHorzStride(exChannelWidth);
    else
      replaceDstWithRawMov(it, bb, exChannelWidth, builder.getGRFAlign());
  }

  // Byte datatype is not allowed on src1 and src2
  // TODO: optimize by swapping src0(non-byte) and src1(byte) if src0
  //       and src1 are swappable
  for (int i = 1, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || !IS_BTYPE(src->getType()))
      continue;
    auto srcRR = src->asSrcRegRegion();
    auto srcMod = srcRR->getModifier();
    auto hasModMinus = (srcMod == Mod_Minus) || (srcMod == Mod_Minus_Abs);
    // If minus modifier is present, need signed type
    auto tmpSrcType =
        (IS_SIGNED_INT(src->getType()) || hasModMinus) ? Type_W : Type_UW;
    replaceSrc(it, bb, i, tmpSrcType, exChannelWidth / TypeSize(tmpSrcType),
               builder.getGRFAlign());
    // Need to check/fix the new mov inst as it is not a raw mov
    fixRegRegionIntPipe(std::prev(it), bb);
  }

  // HW checks restriction even on NULL reg
  if (inst->getDst()->isNullReg()) {
    // Write to null register with byte date type is not supported
    if (IS_BTYPE(inst->getDst()->getType())) {
      G4_DstRegRegion *new_null = builder.createNullDst(
          inst->getDst()->getType() == Type_B ? Type_W : Type_UW);
      inst->setDest(new_null);
    }
  }

  // Fix some special restrictions of format conversion operations
  if (inst->opcode() == G4_mov) {
    auto dstTy = inst->getDst()->getType();
    auto src0Ty = inst->getSrc(0)->getType();

    // Fix format conversion operations between fp16 and word/byte
    if ((IS_HFTYPE(dstTy) && (IS_BTYPE(src0Ty) || IS_WTYPE(src0Ty))) ||
        (IS_HFTYPE(src0Ty) && (IS_BTYPE(dstTy) || IS_WTYPE(dstTy)))) {
      fixMovCvtBetweenFp16AndWordByte(it, bb);
      return;
    }

    // Fix mov convert instructions between fp16 and fp32
    if (((IS_HFTYPE(dstTy) || IS_BFTYPE(dstTy)) && IS_FTYPE(src0Ty)) ||
        ((IS_HFTYPE(src0Ty) || IS_BFTYPE(src0Ty)) && IS_FTYPE(dstTy))) {
      fixMovCvtBetweenFp16AndFp32(it, bb);
      return;
    }
  }

  // Fix destination subregnum restriction for qword execution channel.
  bool dstIsFixed = false; // only need to fix dst once
  uint8_t dstTySize = (uint8_t)inst->getDst()->getTypeSize();
  if (exChannelWidth == 8 &&
      !builder.tryToAlignOperand(inst->getDst(), exChannelWidth)) {
    // Gen doesn't support hstride 8, so we add a W move here
    if (exChannelWidth / dstTySize == 8)
      replaceDst(it, bb, Type_W, exChannelWidth / TypeSize(Type_W),
                 builder.getGRFAlign());
    else
      replaceDstWithRawMov(it, bb, exChannelWidth / dstTySize,
                           builder.getGRFAlign());
    dstIsFixed = true;
  }

  // Skip following restrictions checking for SIMD1 instructions
  if (inst->getExecSize() == g4::SIMD1)
    return;

  // Packed destination, where destination datatype times destination stride
  // is less than the execution datatype is not allowed if execution size is
  // more than one.
  auto dstStrideInBytes = dstTySize * inst->getDst()->getHorzStride();
  if (!dstIsFixed && (dstStrideInBytes < exChannelWidth)) {
    // Gen doesn't support hstride 8, so we add a W move here
    if (exChannelWidth / dstTySize == 8)
      replaceDst(it, bb, Type_W, exChannelWidth / TypeSize(Type_W),
                 builder.getGRFAlign());
    else
      replaceDstWithRawMov(it, bb, /*tmpStride*/ exChannelWidth / dstTySize,
                           builder.getGRFAlign());
    dstIsFixed = true;
  }

  // If all srcs are aligned to dst, can exit early
  if (isAllSrcsAlignedToDst(inst, exChannelWidth, checkFlatRegRegionFunc))
    return;

  // Adjust dst stride
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (!src->isSrcRegRegion() || src->asSrcRegRegion()->isScalar())
      continue;

    if (i == 0 && allowTrueRegionOnSrc0(inst))
      continue;

    auto srcRR = src->asSrcRegRegion();
    uint16_t dstStrideInBytes = inst->getDst()->getExecTypeSize();

    if (inst->getDst()->isNullReg()) {
      // If dst is NULL reg, we can adjust dst stride to save potential mov
      uint16_t srcStrideInBytes = getSrcStrideInBytes(srcRR);
      if (srcStrideInBytes > dstStrideInBytes) {
        inst->getDst()->setHorzStride(srcStrideInBytes /
                                      inst->getDst()->getTypeSize());
        break;
      }
    } else {
      // Later when fixing src to be aligned with dst, it may cause invalid
      // hstride issue for the newly inserted mov instruction. For example:
      //   shl (2|M8) v7th(0,0)<4>:uq v28th(0,0)<0;1,0>:ud v29th(0,0)<1;1,0>:ud
      // To fix src1, we may insert a mov instruction which has invalid hstride:
      //   mov (2|M8) TV(0,0)<8>:ud  v29th(0,0)<1;1,0>:ud
      //   shl (2|M8) v7th(0,0)<4>:uq v28th(0,0)<0;1,0>:ud  TV(0,0)<8;1,0>:ud
      // To avoid such issue, we need to adjust dst stride to be a smaller one
      if (dstStrideInBytes / TypeSize(src->getType()) > 4) {
        replaceDstWithRawMov(it, bb, exChannelWidth / dstTySize,
                             builder.getGRFAlign());
        dstIsFixed = true;
        break;
      }
    }
  }

  auto dst = inst->getDst();
  // Check and fix flat reg region restrictions
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || src->asSrcRegRegion()->isScalar())
      continue;

    if (i == 0 && allowTrueRegionOnSrc0(inst))
      continue;

    auto srcRR = src->asSrcRegRegion();

    // Only need to fix dst once
    if (!dstIsFixed &&
        (dst->isIndirect() ||
         !builder.tryToAlignOperand(dst, builder.numEltPerGRF<Type_UB>()))) {
      replaceDstWithRawMov(it, bb, exChannelWidth / dstTySize,
                           builder.getGRFAlign());
      dst = inst->getDst();
      dstIsFixed = true;
    }

    // Fix src if it's not flat region
    // Src0 and src1 of mul, and src1 and src2 of non-mul have relaxed qw
    // alignemnt rule.
    bool isSrc0Src1Mul = inst->opcode() == G4_mul && (i == 0 || i == 1);
    bool isSrc1Src2NonMul = inst->opcode() != G4_mul && (i == 1 || i == 2);
    uint16_t dstStrideInBytes = dst->getExecTypeSize();
    uint16_t srcStrideInBytes = getSrcStrideInBytes(srcRR);
    if (srcRR->isIndirect() ||
        !(srcRR->isScalar() ||
          srcRR->isFlatRegRegion(exChannelWidth, checkFlatRegRegionFunc) ||
          ((isSrc0Src1Mul || isSrc1Src2NonMul) &&
           srcRR->isFlatRegRegion(exChannelWidth,
                                    checkQwAlignedRegionFunc)) ||
          (dstStrideInBytes == srcStrideInBytes &&
           builder.tryToAlignOperand(src, builder.numEltPerGRF<Type_UB>())))) {
      replaceSrcWithRawMov(it, bb, i,
                           dstStrideInBytes / TypeSize(src->getType()),
                 builder.getGRFAlign());
    }
  }

  // Check true region restrictions on src0: must follow byte/word alignment
  // patterns
  auto src0 = inst->getSrc(0);
  if (allowTrueRegionOnSrc0(inst) && !isAllowedTrueRegionPatternOnSrc0(src0)) {
    // For addr add instruction, fix src0 instead of dst. This will
    // simplify pointer to analysis.
    // For example:
    //   add (16) A0(0,0)<1>:uw ByteOffset_0(0,0)<2;1,0>:uw &ShuffleTmp+0
    //   =>
    //   mov (16) TV1(0,0)<1>:uw ByteOffset_0(0,0)<2;1,0>:uw
    //   add (16) A0(0,0)<1>:uw TV1(0,0)<1;1,0>:uw &ShuffleTmp+0
    if (inst->isAddrAdd()) {
      replaceSrcWithRawMov(it, bb, 0, 1, Any);
      vISA_ASSERT(isAllowedTrueRegionPatternOnSrc0(inst->getSrc(0)),
                  "src0 region is illegal");
      return;
    }

    // If dst is GRF-aligned, we can not fix by changing dst as it may cause
    // src1 and src2 unaligned to dst again. Instead, we can fix by replacing
    // src0 with a temp, same type, GRF-aligned, stride=2 for 16 bits datatypes,
    // stride =4 for 8 bits datatypes. For example:
    //   add(16) r10.0<1>:w  r20.2<2;1,0>:w  r30.0<1>:w  //src0 is not ok
    //   =>
    //   mov(16) r40.0<2>:w r20.2<2;1,0>:w       // ok
    //   add(16) r10.0<1>:w  r40.0<2;1,0>:w  r30.0<1>:w  //ok
    // If dst is not GRF-aligned, we can fix by changing dst with a temp, same
    // type, stride=2 for 16 bits datatypes, stride=4 for 8 bits datatypes.
    //   add(16) r10.1<1>:w  r20.1<2;1,0>:w  r30.0<0;1,0>:w  //src0 is not ok
    //   =>
    //   add(16) r40.0<2>:w  r20.1<2;1,0>:w  r30.0<0;1,0>:w  // ok
    //   mov(16) r10.1<1>:w  r40.0<2>:w // raw mov will be fixed seprately later
    if (builder.tryToAlignOperand(inst->getDst(),
                                  builder.numEltPerGRF<Type_UB>()))
      replaceSrcWithRawMov(it, bb, 0, 4 / TypeSize(src0->getType()),
                           builder.getGRFAlign());
    else
      replaceDstWithRawMov(it, bb, 4 / inst->getDst()->getTypeSize(),
                           builder.getGRFAlign());

    vISA_ASSERT(isAllowedTrueRegionPatternOnSrc0(inst->getSrc(0)),
                "src0 region is illegal");
  }
}

bool HWConformityPro::isAllowedTrueRegionPatternOnSrc0(G4_Operand *src) {
  if (!src || !src->isSrcRegRegion() || src->asSrcRegRegion()->isScalar())
    return true;

  auto inst = src->getInst();
  vISA_ASSERT(allowTrueRegionOnSrc0(inst), "not allow true region on src0");

  auto srcRR = src->asSrcRegRegion();
  auto srcRegionDesc = srcRR->getRegion();
  auto srcWidth = srcRegionDesc->width;
  auto srcVS = srcRegionDesc->vertStride;
  auto srcHS = srcRegionDesc->horzStride;
  uint16_t srcStride = srcWidth == 1 ? srcVS : srcHS;
  bool srcVxH =
      srcRR->isIndirect() && srcRR->getRegion()->isRegionWH() && srcWidth == 1;
  bool srcVx1 = srcRR->isIndirect() && srcRR->getRegion()->isRegionWH();

  auto dst = inst->getDst();
  auto dstTypeSize = dst->getTypeSize();
  auto dstStride = dst->getHorzStride();
  auto srcTypeSize = src->getTypeSize();
  if (dstStride * dstTypeSize >= 4 || srcTypeSize >= 4 || srcVxH) {
    // Allowed. One element per dowrd channel.
    return true;
  }

  bool isSrcUniformStride =
      (srcWidth == 1) || (srcVS == srcWidth * srcHS) || srcVx1;
  bool isDstDwAligned = builder.tryToAlignOperand(dst, 4);
  if (isSrcUniformStride || isDstDwAligned) {
    uint32_t dstOffsetInBytes = 0;
    bool dstHasFixedSubReg = false;
    if (dst->isNullReg())
      dstHasFixedSubReg = true;
    else
      dstHasFixedSubReg = dst->hasFixedSubregOffset(builder, dstOffsetInBytes);
    uint32_t srcOffsetInBytes = 0;
    bool srcHasFixedSubReg = false;
    if (srcRR->isNullReg())
      srcHasFixedSubReg = true;
    else
      srcHasFixedSubReg =
          srcRR->hasFixedSubregOffset(builder, srcOffsetInBytes);
    auto dstSubReg = dstOffsetInBytes / dstTypeSize;
    auto srcSubReg = srcOffsetInBytes / srcTypeSize;

    // :w/:fp16 -> :w/:fp16
    if (srcTypeSize == 2 && dstTypeSize == 2) {
      if (srcStride < 2)
        return true; // Allowed
      if (srcStride == 2 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && dstSubReg % 16 == srcSubReg / 2)
        return true; // Allowed
    }

    // :b/:fp8 -> <2>:w/:fp16
    if (srcTypeSize == 2 && dstTypeSize == 1 && dstStride == 2) {
      if (srcStride < 2)
        return true; // Allowed
      if (srcStride == 2 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && dstSubReg % 32 == srcSubReg)
        return true; // Allowed
    }

    // :b/:fp8 -> :w/:fp16
    if (srcTypeSize == 1 && dstTypeSize == 2) {
      if (srcStride < 4)
        return true; // Allowed
      if (srcStride == 4 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (2 * (dstSubReg % 16) == srcSubReg / 2))
        return true; // Allowed
      if (srcStride == 8 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (2 * (dstSubReg % 8) == srcSubReg / 4))
        return true; // Allowed
    }

    // :b/:fp8 -> <2>:b/:fp8
    if (srcTypeSize == 1 && dstTypeSize == 1 && dstStride == 2) {
      if (srcStride < 4)
        return true; // Allowed
      if (srcStride == 4 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (dstSubReg % 32 == srcSubReg / 2))
        return true; // Allowed
      if (srcStride == 8 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (dstSubReg % 16 == srcSubReg / 4))
        return true; // Allowed
    }

    // <v;w!=2,h>:b/:fp8 -> <1>:b/:fp8
    if (srcTypeSize == 1 && dstTypeSize == 1 && dstStride == 1 &&
        srcWidth != 2) {
      if (srcStride < 2)
        return true; // Allowed
      if (srcStride == 2 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (dstSubReg % 32 == srcSubReg / 2))
        return true; // Allowed
      if (srcStride == 4 && isSrcUniformStride && dstHasFixedSubReg &&
          srcHasFixedSubReg && (dstSubReg % 16 == srcSubReg / 4))
        return true; // Allowed
    }

    // <v;2,h>:b/:fp8 -> <1>:b/:fp8
    if (srcTypeSize == 1 && dstTypeSize == 1 && dstStride == 1 &&
        srcWidth == 2) {
      if ((srcHS == 0 && srcVS < 4) || (srcHS == 1 && srcVS < 4) ||
          (srcHS == 2 && srcVS < 2))
        return true; // Allowed
      if (srcHS == 1 && srcVS == 4 && dstHasFixedSubReg && srcHasFixedSubReg &&
          (dstSubReg % 32 == 2 * (srcSubReg / 4)) && (srcSubReg % 2 == 0))
        return true; // Allowed
      if (srcHS == 2 && srcVS == 4 && dstHasFixedSubReg && srcHasFixedSubReg &&
          (dstSubReg % 32 == srcSubReg / 2))
        return true; // Allowed
      if (srcHS == 4 && srcVS == 8 && dstHasFixedSubReg && srcHasFixedSubReg &&
          (dstSubReg % 16 == srcSubReg / 4))
        return true; // Allowed
    }
  }
  return false;
}

void HWConformityPro::fixRawMovRegRegionRestrictions(G4_BB *bb) {
  for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
    G4_INST *inst = *it;

    if (!inst->isRawMov())
      continue;

    bool invalidPureBfInst = inst->isPureBFInst() && !builder.supportPureBF();

    if (!inst->getSrc(0)->isSrcRegRegion() ||
        (inst->getSrc(0)->asSrcRegRegion()->isScalar() && !invalidPureBfInst))
      continue;

    // For conversions in float pipeline, change datatype to corresponding
    // int datatype directly as int pipeline has relaxed region restrictions.
    // Specifically:
    // HF/BF->HF/BF: UW->UW. Need further check the int pipeline restrictions.
    // F->F: UD->UD. Skip further check as UD support full regions.
    // DF->DF: UQ->UQ. Skip further check as UQ support full regions.
    auto dstType = inst->getDst()->getType();
    if (IS_TYPE_FLOAT_ALL(dstType)) {
      auto intType = TypeSize(dstType) == 8
                         ? Type_UQ
                         : (TypeSize(dstType) == 4 ? Type_UD : Type_UW);
      inst->getDst()->setType(builder, intType);
      inst->getSrc(0)->asSrcRegRegion()->setType(builder, intType);

      // Skip further check as UQ/UD support full regions.
      if (intType == Type_UQ || intType == Type_UD)
        continue;
    }

    // Fix register region restrictions for int pipeline:
    // q->q, uq->uq, d->d, ud->ud, w->w, uw->uw, b->b, ub->ub
    if (inst->isIntegerPipeInstructionXe()) {
      if (allowTrueRegionOnSrc0(inst) &&
          !isAllowedTrueRegionPatternOnSrc0(inst->getSrc(0))) {
        // q/uq/d/ud should have full region support
        vISA_ASSERT(IS_BTYPE(inst->getSrc(0)->getType()) ||
                        IS_WTYPE(inst->getSrc(0)->getType()),
                    "expected uw/w/ub/b datatypes");

        bool isPackedByteDst = IS_BTYPE(inst->getDst()->getType()) &&
                               inst->getDst()->getHorzStride() == 1;
        uint32_t srcSubOffInBytes = 0;
        bool hasFixedSubRegOffsetSrc =
            inst->getSrc(0)->asSrcRegRegion()->hasFixedSubregOffset(
                builder, srcSubOffInBytes);
        auto srcRRDesc = inst->getSrc(0)->asSrcRegRegion()->getRegion();
        bool is1x16x4RegionSrc = srcRRDesc->width == 16 &&
                                 srcRRDesc->horzStride == 4 &&
                                 srcRRDesc->vertStride == 1;
        if (inst->getExecSize() == g4::SIMD32 &&
            isPackedByteDst && builder.tryToAlignOperand(inst->getDst(), 32) &&
            is1x16x4RegionSrc && hasFixedSubRegOffsetSrc &&
            (srcSubOffInBytes == 0 || srcSubOffInBytes == 2))
          continue;

        // Try to make dst and src GRF-aligned
        bool isDstGrfAligned = builder.tryToAlignOperand(
            inst->getDst(), builder.numEltPerGRF<Type_UB>());
        bool isSrcGrfAligned = builder.tryToAlignOperand(
            inst->getSrc(0), builder.numEltPerGRF<Type_UB>());
        if (isDstGrfAligned || isSrcGrfAligned)
          if (isAllowedTrueRegionPatternOnSrc0(inst->getSrc(0)))
            continue;

        // General fix is to replace dst with a temp, same type, stride 2 for
        // word datatype or stride 4 for byte datatype. For example:
        //   mov(16) r10.1<1>:w r20.1<2;1,0>:w
        //   =>
        //   mov(16) r30.0<2>:w r20.1<2;1,0>:w // ok
        //   mov(16) r10.1<1>:w r30.0<2;1,0>:w // not ok, will be further fixed
        // But if src is GRF-align and new dst stride equals to src vertical
        // stride, there will be infinite loop issue. To avoid this, should fix
        // by repalcing dst with a temp, same type, stride 1 instead of using
        // above general fix. For example:
        //   mov(16) r10.1<1>:w r30.0<2;1,0>:w
        //   =>
        //   mov(16) r40.0<1>:w r30.0<2;1,0>:w  // ok
        //   mov(16) r10.1<1>:w r40.0<1;1,0>:w  // ok
        auto newDstStride = 4 / inst->getDst()->getTypeSize();
        auto srcRgDesc = inst->getSrc(0)->asSrcRegRegion()->getRegion();
        if (isSrcGrfAligned && srcRgDesc->vertStride == newDstStride &&
            srcRgDesc->width == 1 && srcRgDesc->horzStride == 0)
          newDstStride = 1;
        replaceDstWithRawMov(it, bb, newDstStride, builder.getGRFAlign());

        vISA_ASSERT(isAllowedTrueRegionPatternOnSrc0(inst->getSrc(0)),
                    "src0 region is illegal");
      }
    }
  }
}

// For BF and F mixed mode:
// Since HW has no lower channel access for bf, just convert bf to f datatype
// to avoid bf mixed mode. For example:
// add(16|M0) r10.0<1>:f r20.0<2;1,0>:bf r30.0<1;1,0>:f
// =>
// mov(16|M0) r40.0<1>:f r20.0<2;1,0>:bf
// add(16|M0) r10.0<1>:f r40.0<1;1,0>:f r30.0<1;1,0>:f
//
// For HF and F mixed mode:
// HW does not support HF/F mixed mode, so convert hf to f to fix:
// add(16|M0) r10.0<1>:f r20.0<2;1,0>:hf r30.0<1;1,0>:f
// =>
// mov(16|M0) r40.0<1>:f r20.0<2;1,0>:hf
// add(16|M0) r10.0<1>:f r40.0<1;1,0>:f r30.0<1;1,0>:f
void HWConformityPro::fixFloatMixedModeAndPureBFMode(INST_LIST_ITER it,
                                                      G4_BB *bb) {
  G4_INST *inst = *it;

  auto isMixedMode = [](G4_INST *inst) {
    auto dstTy = inst->getDst()->getType();
    for (int i = 0, numSrcs = inst->getNumSrc(); i < numSrcs; i++) {
      auto src = inst->getSrc(i);
      if (!src)
        continue;
      auto srcTy = src->getType();
      if ((isLowPrecisionFloatTy(dstTy) && IS_FTYPE(srcTy)) ||
          (isLowPrecisionFloatTy(srcTy) && IS_FTYPE(dstTy)))
        return true;
    }
    return false;
  };

  // MOV is skipped
  if (inst->opcode() == G4_mov ||
      !(isMixedMode(inst) ||
        (inst->isPureBFInst() && !builder.supportPureBF())))
    return;

  auto dst = inst->getDst();
  if (dst && isLowPrecisionFloatTy(dst->getType())) {
    bool wasMovInserted = replaceDstWasMovInserted(it, bb, Type_F, /*tmpStride*/ 0, builder.getGRFAlign());
    // Further fix the new added mov as it is not a raw mov
    if (wasMovInserted) {
      fixRegRegionIntPipe(std::next(it), bb);
    }
  }

  for (int i = 0, srcNum = inst->getNumSrc(); i < srcNum; ++i) {
    auto src = inst->getSrc(i);
    if (src && src->getType() == Type_BF && src->isImm()) {
      vISA_ASSERT(false, "BF immediate is not supported!");
    }
    if (src && isLowPrecisionFloatTy(src->getType())) {
      bool wasMovInserted = replaceSrcWasMovInserted(it, bb, i, Type_F, /*tmpStride*/ 0, builder.getGRFAlign());
      // Further fix the new added mov as it is not a raw mov
      if (wasMovInserted) {
        fixRegRegionIntPipe(std::prev(it), bb);
      }
    }
  }
}

// General Restrictions on Regioning Parameters:
// a. ExecSize must be greater than or equal to Width.
// b. If ExecSize = Width and HorzStride != 0, VertStride must be set to
//    Width * HorzStride.
// c. If ExecSize = Width and HorzStride = 0, there is no restriction on
//    VertStride.
// d. If Width = 1, HorzStride must be 0 regardless of the values of ExecSize
//    and VertStride.
// e. If ExecSize = Width = 1, both VertStride and HorzStride must be 0.
// f. If VertStride = HorzStride = 0, Width must be 1 regardless of the value
//    of ExecSize.
// g. Dst.HorzStride must not be 0.
// h. VertStride must be used to cross register boundaries. This rule implies
//    that elements within a 'Width' cannot cross register boundaries.
void HWConformityPro::fixGeneralRestrictsOnRegionParameters(G4_BB *bb) {
  for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
    G4_INST *inst = *it;

    if (inst->getDst() && !inst->hasNULLDst()) {
      vISA_ASSERT(inst->getDst()->getHorzStride() != 0,
                  "Bad dst region: Dst.HorzStride must not be 0.");
    }

    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion() &&
          !inst->getSrc(i)->isNullReg()) {
        auto *src = inst->getSrc(i)->asSrcRegRegion();
        auto *srcRegion = src->getRegion();

        if (srcRegion->isRegionWH() || srcRegion->isRegionV() ||
            srcRegion->isRegionSW()) {
          // normalize VxH regions if possible
          if (srcRegion->isRegionWH() &&
              srcRegion->width == inst->getExecSize()) {
            // r[a0.0]<E, S> -> r[a0.0]<S;1,0>
            src->setRegion(
                builder, builder.createRegionDesc(srcRegion->horzStride, 1, 0));
          }
          // ToDo: add other legalization
          continue;
        }

        // ToDo: most of these checks should be obsolete at this point
        uint16_t vs = srcRegion->vertStride, wd = srcRegion->width,
                 hs = srcRegion->horzStride;
        uint8_t exSize = inst->getExecSize();
        vISA_ASSERT(
            exSize >= wd,
            "Bad src region: ExecSize must be greater than or equal to Width.");

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

        // check cross GRF(rule h)
        if (src->getRegAccess() == Direct && src->crossGRF(builder) &&
            hs != 0) {
          // check number of elements in first GRF.
          uint16_t execTypeSize = hs * src->getElemSize();
          uint16_t sizeInFirstGRF =
              kernel().numEltPerGRF<Type_UB>() -
              src->getLeftBound() % kernel().numEltPerGRF<Type_UB>();
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
  }
}

void HWConformityPro::fixSendg(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (!inst->isSendg()) {
    return;
  }

  if (inst->getExecSize() < builder.getNativeExecSize()) {
    // For sendg, the IR declare size may be less than the operand length.
    // Since the smallest SIMD size for sendg is 16 (no matter input IR's SIMD
    // size, which can be SIMD1), the data length maybe larger than the variable
    // size.
    // SIMD16 + Q/UQ type, the maximal operand size counted by vISA is 2 GRFs.
    G4_Declare *src0Dcl = inst->getSrc(0)->getTopDcl();
    bool src0SizeMismatch = inst->getMsgDesc()->getSrc0LenRegs() == 2 &&
                        (src0Dcl && src0Dcl->getRootDeclare()->getByteSize() <
                                        2u * kernel().numEltPerGRF<Type_UB>());
    G4_Declare *src1Dcl = inst->getSrc(1)->getTopDcl();
    bool src1SizeMismatch =
        inst->getMsgDesc()->getSrc1LenRegs() == 2 &&
        (src1Dcl && src1Dcl->getRootDeclare()->getByteSize() <
                        2u * kernel().numEltPerGRF<Type_UB>());
    auto doEvenAlign = [this](G4_Declare *dcl) {
      if (dcl) {
        dcl = dcl->getRootDeclare();
        if (dcl->getByteSize() < 2u * kernel().numEltPerGRF<Type_UB>()) {
          dcl->setEvenAlign();
        }
      }
    };
    if (src0SizeMismatch) {
      doEvenAlign(inst->getSrc(0)->getTopDcl());
    }
    if (src1SizeMismatch) {
      doEvenAlign(inst->getSrc(1)->getTopDcl());
    }
  }
}

void HWConformityPro::fixCalla(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isFCall(), "expect fcall instruction");

  // fcall could have imm/label src for direct call
  // No need to fix src reg at the case
  G4_Operand *src0 = inst->getSrc(0);
  if (!src0->isSrcRegRegion())
    return;

  if (builder.tryToAlignOperand(src0, kernel().numEltPerGRF<Type_UB>()))
    return;

  // insert a mov before fcall(calla) to mov src to a grf aligned reg
  replaceSrc(it, bb, 0, src0->getType(), /*tmpStride*/0, builder.getGRFAlign());
}

void HWConformityPro::fixBfn(INST_LIST_ITER it, G4_BB *bb) {
  // BFN requires its operands to be UD/UW
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isBfn(), "expect bfn instruction");

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
      replaceSrc(it, bb, i, Type_D, /*tmpStride*/ 0, Any);

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

bool HWConformityPro::checkDPASSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb) {
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

G4_INST *HWConformityPro::evenlySplitDPAS8x8Inst(INST_LIST_ITER iter,
                                                 G4_BB *bb) {
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
    unsigned int GRFSize = (dst_r - dst_l + 1) / builder.getGRFSize();
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
      unsigned int GRFSize = (src_r - src_l + 1) / builder.getGRFSize();
      vISA_ASSERT(((src_r - src_l + 1) % builder.getGRFSize() == 0),
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

void HWConformityPro::fixDPAS(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
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

void HWConformityPro::fixSelCsel(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT((inst->opcode() == G4_sel || inst->opcode() == G4_csel),
              "expect sel/csel instruction");

  G4_CondMod *condMod = inst->getCondMod();
  if (condMod) {
    condMod->setBase(nullptr);
  }
}

void HWConformityPro::fixCmpInst(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->isCompare(), "expect cmp/cmpn instruction");

  auto dst = inst->getDst();
  if (dst && dst->isNullReg()) {
    auto execTy = inst->getExecType2();
    if (TypeSize(execTy) != dst->getTypeSize()) {
      auto newNullDst = builder.createNullDst(execTy);
      inst->setDest(newNullDst);
    }
  }
}

void HWConformityPro::fixRotate(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT((inst->opcode() == G4_ror || inst->opcode() == G4_rol),
              "expect ror/rol instruction");

  auto dst = inst->getDst();
  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);
  auto dstTy = dst->getType();
  auto src0Ty = src0->getType();
  auto src1Ty = src1->getType();

  vISA_ASSERT(IS_WTYPE(dstTy) || IS_DTYPE(dstTy) || IS_QTYPE(dstTy),
              "dst type must be *W or *D or *Q");
  vISA_ASSERT(IS_WTYPE(src0Ty) || IS_DTYPE(src0Ty) || IS_QTYPE(src0Ty),
              "src0 type must be *W or *D or *Q");
  vISA_ASSERT(IS_WTYPE(src1Ty) || IS_DTYPE(src1Ty) || IS_QTYPE(src1Ty),
              "src1 type must be *W or *D or *Q");

  // rotate requires src0 and dst to have the same datatype
  if (dst->getTypeSize() > src0->getTypeSize()) {
    // use dst type as rotation type
    replaceSrc(it, bb, 0, dst->getType(), /*tmpStride*/0, Any);
    src0 = inst->getSrc(0);
  } else if (src0->getTypeSize() > dst->getTypeSize()) {
    // use src type as rotation type
    G4_Type NewDstTy = src0->getType();
    if (TypeSize(NewDstTy) == 8 && inst->getExecSize() == g4::SIMD32 &&
        !builder.supportNativeSIMD32()) {
      // Split instrution if SIMD32 with qword datatype is not supported
      evenlySplitInst(it, bb);
      auto preIt = std::prev(it);
      fixRotate(preIt, bb);
    }
    replaceDst(it, bb, NewDstTy, /*tmpStride*/ 0, Any);
    dst = inst->getDst();
    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
  }

  // dst must be UW/UD/UQ
  dstTy = dst->getType();
  if (IS_SIGNED_INT(dstTy))
    dst->setType(builder, getUnsignedType(TypeSize(dstTy)));

  // src0 must be UW/UD/UQ
  src0Ty = src0->getType();
  if (IS_SIGNED_INT(src0Ty)) {
    auto newSrc0Ty = getUnsignedType(TypeSize(src0Ty));
    if (src0->isImm()) {
      inst->setSrc(builder.createImm(src0->asImm()->getImm(), newSrc0Ty), 0);
    } else {
      src0->asSrcRegRegion()->setType(builder, newSrc0Ty);
    }
  }

  // src1 can only be UW/UD/UQ
  if (IS_SIGNED_INT(src1Ty)) {
    auto newSrc1Ty = getUnsignedType(TypeSize(src1Ty));
    if (src1->isImm()) {
      uint32_t immVal = (uint32_t)src1->asImm()->getImm();
      // Can not encode imm64, so truncate to UD as rotate will takes the lower
      // rotation count (5bits for UD, 6bits for UQ).
      inst->setSrc(
          builder.createImm(immVal, newSrc1Ty == Type_UQ ? Type_UD : newSrc1Ty),
          1);
    } else {
      src1->asSrcRegRegion()->setType(builder, newSrc1Ty);
    }
  }
}

void HWConformityPro::fixMulDataTypes(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_mul, "expect mul instruction");

  auto dst = inst->getDst();
  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);

  // No need to handle float datatypes
  if (IS_TYPE_FLOAT_ALL(dst->getType()))
    return;

  // The following integer datatype combinations are supported
  //   Dst    Src0    Src1
  //  *W/*D    *W      *W
  //  *W/*D    *D      *W
  //  *D/*Q    *D      *D
  auto fixSrc = [&](int srcNum, G4_Operand *src, G4_Type newTy) {
    if (src->isImm() && !IS_VINTTYPE(src->getType()) &&
        src->getType() != newTy) {
      G4_Operand *newOpnd = builder.createImm(src->asImm()->getImm(), newTy);
      inst->setSrc(newOpnd, srcNum);
    } else {
      // // If minus modifier is present, need signed type.
      bool hasModMinus = false;
      if (src->isSrcRegRegion()) {
        G4_SrcModifier mod = src->asSrcRegRegion()->getModifier();
        hasModMinus = (mod == Mod_Minus || mod == Mod_Minus_Abs);
      }
      replaceSrc(it, bb, srcNum,
                 hasModMinus ? getSignedType(TypeSize(newTy)) : newTy,
                 inst->getExecTypeSizeXe3p() / TypeSize(newTy), Any);
    }
  };

  // Dst is Q type, promote source to D type if necessary
  if (IS_QTYPE(dst->getType())) {
    G4_Type newTy;

    if (!IS_DTYPE(src0->getType())) {
      newTy = IS_SIGNED_INT(src0->getType()) ? Type_D : Type_UD;
      fixSrc(0, src0, newTy);
    }

    if (!IS_DTYPE(src1->getType())) {
      newTy = IS_SIGNED_INT(src1->getType()) ? Type_D : Type_UD;
      fixSrc(1, src1, newTy);
    }
    return;
  }

  // Promote dst datatype if needed
  uint8_t execTypeSize = inst->getExecTypeSizeXe3p();
  if (dst->getTypeSize() < execTypeSize) {
    G4_Type newDstTy = dst->getType();
    switch (execTypeSize) {
    case 2:
      newDstTy = IS_UNSIGNED_INT(newDstTy) ? Type_UW : Type_W;
      break;
    case 4:
      // No need to promote dst datatype if dst is W type as HW supports W=DxW.
      // But must promote dst to D type if both sources are D type.
      if ((!IS_WTYPE(dst->getType()) ||
          (IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()))))
        newDstTy = IS_UNSIGNED_INT(newDstTy) ? Type_UD : Type_D;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected execution type size");
      break;
    }
    if (newDstTy != dst->getType()) {
      replaceDst(it, bb, newDstTy, execTypeSize / TypeSize(newDstTy), Any);
      dst = inst->getDst();
    }
  }

  // Dst is D type, promote source to W or D types if necessary. For example:
  // D=W*D                   =>  D=D*D(imm) or D=D(non-imm)*W
  // D=B*D or D=D*B or D=B*B =>  D=D*D
  // D=W*B or D=B*W          =>  D=W*W
  auto src0Ty = src0->getType();
  auto src1Ty = src1->getType();
  if (IS_DTYPE(dst->getType())) {
    bool isDxD = IS_DTYPE(src0Ty) && IS_DTYPE(src1Ty);
    bool isWxW = IS_WTYPE(src0Ty) && IS_WTYPE(src1Ty);
    bool isDxW = IS_DTYPE(src0Ty) && IS_WTYPE(src1Ty);
    // valid cases: DxD/DxW/WxW
    if (isDxD || isWxW || isDxW)
      return;

    // WxB/BxW => WxW
    if ((IS_BTYPE(src0Ty) && IS_WTYPE(src1Ty)) ||
        (IS_WTYPE(src0Ty) && IS_BTYPE(src1Ty))) {
      if (IS_BTYPE(src0Ty)) {
        auto newTy = IS_SIGNED_INT(src0Ty) ? Type_W : Type_UW;
        fixSrc(0, src0, newTy);
      }
      if (IS_BTYPE(src1Ty)) {
        auto newTy = IS_SIGNED_INT(src1Ty) ? Type_W : Type_UW;
        fixSrc(1, src1, newTy);
      }
      return;
    }

    // WxD(non-imm) => DxW
    if (IS_WTYPE(src0Ty) && IS_DTYPE(src1Ty) && !src1->isImm()) {
      inst->swapSrc(0, 1);
      return;
    }

    // WxD(imm)/DxB/BxD/BxB => DxD
    if (!IS_DTYPE(src0Ty)) {
      auto newTy =
          (IS_SIGNED_INT(src0Ty) || src0Ty == Type_V) ? Type_D : Type_UD;
      fixSrc(0, src0, newTy);
    }
    if (!IS_DTYPE(src1Ty)) {
      auto newTy =
          (IS_SIGNED_INT(src1Ty) || src1Ty == Type_V) ? Type_D : Type_UD;
      fixSrc(1, src1, newTy);
    }
    return;
  }

  // Dst is W type, promote source to W type if necessary. For example:
  // W=W*B or W=B*W or W=B*B => W=W*W
  // W=D*B or W=B*D or W=W*D => W=D*W
  if (IS_WTYPE(dst->getType())) {
    if (IS_BTYPE(src0Ty)) {
      auto newTy = IS_SIGNED_INT(src0Ty) ? Type_W : Type_UW;
      fixSrc(0, src0, newTy);
      src0 = inst->getSrc(0);
      src0Ty = src0->getType();
    }
    if (IS_BTYPE(src1Ty)) {
      auto newTy = IS_SIGNED_INT(src1Ty) ? Type_W : Type_UW;
      fixSrc(1, src1, newTy);
      src1 = inst->getSrc(1);
      src1Ty = src1->getType();
    }

    // WxD => DxW
    if (IS_WTYPE(src0Ty) && IS_DTYPE(src1Ty)) {
      inst->swapSrc(0, 1);
      if (inst->getSrc(0)->isImm())
        fixSrc(0, inst->getSrc(0), inst->getSrc(0)->getType());
    }
  }
}

void HWConformityPro::fixMul(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_mul, "expect mul instruction");

  // Fix mul data types combination
  fixMulDataTypes(it, bb);

  // vISA input doesn't allow mul with saturation for integer data types. But
  // some passes like localDefHoisting may introduce saturation to mul
  // instruction. Here to fix the unsupported saturation in below case:
  // When the destination datatype size is lesser than the sum of the datatype
  // sizes of src0 and src1 for an integer operation, the low bits of the result
  // are written to the destination register and the remaining high bits are
  // discarded. This results in undefined Overflow and Sign flags. Therefore,
  // conditional modifiers and saturation (.sat) cannot be used in this case.
  auto dstSize = inst->getDst()->getTypeSize();
  auto src0Size = inst->getSrc(0)->getTypeSize();
  auto src1Size = inst->getSrc(1)->getTypeSize();
  if (inst->getSaturate() && inst->isIntegerPipeInstructionXe() &&
      (dstSize < src0Size + src1Size)) {
    auto newDstType = (src0Size + src1Size <= 4) ? Type_D : Type_Q;
    replaceDst(it, bb, newDstType,/*tmpStride*/ 0, Any);
    inst->setSaturate(g4::NOSAT);
  }
}

// Translate mulh into:
//   mullh tmp_hi_low:d src0:d src1:d
//   mov dst.d tmp.hi:d
void HWConformityPro::fixMulh(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_mulh, "expect mulh instruction");

  if (inst->getSrc(0)->isImm() && !inst->getSrc(1)->isImm())
    inst->swapSrc(0, 1);

  // Fix immediate type as mulh can only support D/UD types
  auto src1 = inst->getSrc(1);
  if (src1->isImm()) {
    uint32_t immVal = (uint32_t)inst->getSrc(1)->asImm()->getImm();
    inst->setSrc(builder.createImm(
                     immVal, IS_SIGNED_INT(src1->getType()) ? Type_D : Type_UD),
                 1);

    auto src0 = inst->getSrc(0);
    if (src0->isImm()) {
        // TODO: we can calculate mulh of two immediates at compile time
        // and change to single value mov.

        uint32_t immVal0 = (uint32_t)inst->getSrc(0)->asImm()->getImm();
        inst->setSrc(builder.createImm(
            immVal0, IS_SIGNED_INT(src0->getType()) ? Type_D : Type_UD),
            0);
    }
  }

  vISA_ASSERT(IS_DTYPE(inst->getDst()->getType()), "dst only supports DW type");
  vISA_ASSERT(IS_DTYPE(inst->getSrc(0)->getType()) &&
                  IS_DTYPE(inst->getSrc(1)->getType()),
              "only DW-type sources are supported");

  if (inst->getExecSize() > builder.getNativeExecSize()) {
    auto startIter = it;
    bool isFirstInst = startIter == bb->begin();
    if (!isFirstInst) {
      --startIter;
    }
    evenlySplitInst(it, bb);
    if (!isFirstInst) {
      ++startIter;
    }
    // startIter now points to first mulh created by split
    auto endIter = it;
    ++endIter;
    // endIter points to the first inst after the original mulh
    for (auto iter = startIter; iter != endIter;) {
      auto nextIter = iter;
      ++nextIter;
      G4_INST *currInst = *iter;
      if (currInst->opcode() == G4_mulh) {
        doTranslateMulh(iter, bb);
      }
      iter = nextIter;
    }
  } else {
    doTranslateMulh(it, bb);
  }
}

void HWConformityPro::doTranslateMulh(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  auto execSize = inst->getExecSize();
  vISA_ASSERT(inst->opcode() == G4_mulh, "expect mulh instruction");
  vISA_ASSERT(execSize <= builder.getNativeExecSize(),
              "expect single register inst");

  // Create mullh instruction:
  // mullh (16)  tmp<1>:d  src0<1;1,0>:d    src1<1;1,0>:d
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
      G4_mullh, execSize, tmpDst,
                          builder.duplicateOperand(src0),
                          builder.duplicateOperand(src1), origOptions, false);
  mullhInst->setVISAId(inst->getVISAId());
  (*it) = mullhInst;

  // Create mov instruction:
  //   mov (16)  dst<1>:d  tmp.hi
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
}

// Return true if generating mad is successful.
// For fp mad, this must succeed due to precision requirements.
bool HWConformityPro::generateMad(INST_LIST_ITER it, G4_BB *bb) {
  auto inst = *it;
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");

  // Try to swap src0(actually src2) and src1 to see if we can save a mov.
  // Some conditions where swap may help:
  // -- if src1 is 16-bits imm, as MAD src2 supports 16-bits imm
  // -- if src1 is W and src0(actually src2) is D, as MAD support DxW cases
  if ((inst->getSrc(1)->isImm() && inst->getSrc(1)->getTypeSize() == 2) ||
      (IS_WTYPE(inst->getSrc(1)->getType()) &&
       IS_DTYPE(inst->getSrc(0)->getType()))) {
    inst->swapSrc(0, 1);
  }

  bool mustDoMad = IS_TYPE_FLOAT_ALL(inst->getDst()->getType());
  if (!mustDoMad) {
    // The following datatype combinations are supported:
    //  dst    src0   src1  src2
    // *W/*D   *W/*D   *W    *W
    // *D/*Q   *D/*Q   *D    *D
    // *D     16b_imm  *D    *D
    // *W/*D   *W/*D   *D    *W
    auto dstTy = inst->getDst()->getType();
    auto src0 = inst->getSrc(2);
    auto src1 = inst->getSrc(1);
    auto src2 = inst->getSrc(0);
    auto src0Ty = src0->getType();
    auto src1Ty = src1->getType();
    auto src2Ty = src2->getType();
    bool is_WD_WD_W_W = (IS_WTYPE(dstTy) || IS_DTYPE(dstTy)) &&
                        (IS_WTYPE(src0Ty) || IS_DTYPE(src0Ty)) &&
                        IS_WTYPE(src1Ty) && IS_WTYPE(src2Ty);
    bool is_WD_WD_D_W = (IS_WTYPE(dstTy) || IS_DTYPE(dstTy)) &&
                        (IS_WTYPE(src0Ty) || IS_DTYPE(src0Ty)) &&
                        IS_DTYPE(src1Ty) && IS_WTYPE(src2Ty);
    bool is_DQ_DQ_D_D = (IS_DTYPE(dstTy) || IS_QTYPE(dstTy)) &&
                         (IS_DTYPE(src0Ty) || IS_QTYPE(src0Ty)) &&
                         IS_DTYPE(src1Ty) && IS_DTYPE(src2Ty);
    bool is_D_imm16_D_D = IS_DTYPE(dstTy) &&
                          (src0->isImm() && IS_WTYPE(src0Ty)) &&
                          IS_DTYPE(src1Ty) && IS_DTYPE(src2Ty);

    bool supportedTypeComb =
        is_WD_WD_W_W || is_WD_WD_D_W || is_DQ_DQ_D_D || is_D_imm16_D_D;

    if (!supportedTypeComb)
      return false;
  }

  inst->setOpcode(G4_mad);
  inst->swapSrc(0, 2);
  return true;
}

// Convert a psuedo mad inst into mul/add
void HWConformityPro::convertMAD2MulAdd(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
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
  auto addOpnd1 =
      builder.createSrcRegRegion(mulDefDcl, builder.getRegionStride1());
  G4_INST *addOp = builder.createInternalInst(
      inst->getPredicate(), G4_add, inst->getCondMod(), inst->getSaturate(),
      inst->getExecSize(), addOpDst, addOpnd1, addOpnd2, nullptr,
      inst->getOption());
  bb->insertAfter(it, addOp);

  // predicate/condmod/saturate, if they exist, are propagated to the add
  // instruction
  inst->setSaturate(g4::NOSAT);
  inst->setPredicate(nullptr);
  inst->setCondMod(nullptr);

  // Fix the new mul instruction as some datatype combinations are not supported
  fixMul(it, bb);
}

void HWConformityPro::fixMad(INST_LIST_ITER it, G4_BB *bb) {
  [[maybe_unused]] G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_pseudo_mad, "expect mad instruction");

  if (generateMad(it, bb))
    return;

  // translate MAD into MUL/ADD
  convertMAD2MulAdd(it, bb);
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

static void fixDstMadw(INST_LIST_ITER it, G4_BB *bb, IR_Builder &builder) {
  //  Need to move both low and high results for madw. For example:
  //   madw(8) r10.1<1>:d r20.0<1;1,0>:d r30.0<1;1,0>:d
  //   =>
  //   madw(8) r40.0<1>:d r20.0<1;1,0>:d r30.0<1;1,0>:d
  //   mov(8) r10.1<1>:d r40.0<1;1,0>:d
  //   mov(8) r11.1<1>:d r41.0<1;1,0>:d
  G4_INST *inst = *it;
  auto execSize = inst->getExecSize();
  auto dst = inst->getDst();

  int origDstLowGRFNum = (int)std::ceil(
      (float)(execSize * dst->getExecTypeSize()) / builder.getGRFSize());

  // New dst's horz stride is always 1
  int newDstLowGRFNum = (int)std::ceil((float)(execSize * dst->getTypeSize()) /
                                       builder.getGRFSize());
  int newDstTotalGRFNum = newDstLowGRFNum * 2;

  G4_Declare *newDstDcl = builder.createTempVar(
      builder.numEltPerGRF(dst->getType()) * newDstTotalGRFNum, dst->getType(),
      builder.getGRFAlign());

  // Add a tmp mov for low results in dst
  //   mov dst_Lo.n<1>:d tmp_Lo.0<1;1,0>:d
  G4_Declare *lowMovSrcDcl = builder.createTempVar(
      builder.numEltPerGRF(dst->getType()) * newDstLowGRFNum, dst->getType(),
      builder.getGRFAlign());
  lowMovSrcDcl->setAliasDeclare(newDstDcl, 0);
  G4_SrcRegRegion *lowMovSrc = builder.createSrcRegRegion(
      lowMovSrcDcl, execSize == g4::SIMD1 ? builder.getRegionScalar()
                                          : builder.getRegionStride1());
  G4_DstRegRegion *dstLow = nullptr;
  if (dst->isIndirect()) {
    dstLow = builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(),
                                       dst->getHorzStride(), dst->getType(),
                                       dst->getAddrImm());
  } else {
    dstLow =
        builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                          dst->getHorzStride(), dst->getType());
  }
  G4_INST *lowMovInst = builder.createMov(execSize, dstLow, lowMovSrc,
                                          inst->getMaskOption(), false);
  lowMovInst->setPredicate(inst->getPredicate());
  lowMovInst->setSaturate(inst->getSaturate());
  auto insertIter = bb->insertAfter(it, lowMovInst);

  // Add a tmp mov for high results in dst:
  //   mov dst_Hi.n<1>:d tmp_Hi.0<1;1,0>:d
  G4_Declare *hiMovSrcDcl = builder.createTempVar(
      builder.numEltPerGRF(dst->getType()) * newDstLowGRFNum, dst->getType(),
      builder.getGRFAlign());
  hiMovSrcDcl->setAliasDeclare(newDstDcl,
                               newDstLowGRFNum * builder.getGRFSize());
  G4_SrcRegRegion *hiMovSrc = builder.createSrcRegRegion(
      hiMovSrcDcl, execSize == g4::SIMD1 ? builder.getRegionScalar()
                                         : builder.getRegionStride1());
  G4_DstRegRegion *dstHi = nullptr;
  if (dst->isIndirect()) {
    dstHi = builder.createIndirectDst(
        dst->getBase(), dst->getSubRegOff(), dst->getHorzStride(),
        dst->getType(),
        dst->getAddrImm() + origDstLowGRFNum * builder.numEltPerGRF<Type_UB>());
  } else {
    dstHi = builder.createDst(
        dst->getBase(), dst->getRegOff() + origDstLowGRFNum,
        dst->getSubRegOff(), dst->getHorzStride(), dst->getType());
  }
  G4_INST *hiMovInst = builder.createMov(execSize, dstHi, hiMovSrc,
                                         inst->getMaskOption(), false);
  hiMovInst->setPredicate(builder.duplicateOperand(inst->getPredicate()));
  hiMovInst->setSaturate(inst->getSaturate());
  bb->insertAfter(insertIter, hiMovInst);

  // Change the dst of madw with the tmp operand:
  //  madw tmpHiLo.0<1>:d src0:d src1:d
  G4_DstRegRegion *newDst = builder.createDstRegRegion(newDstDcl, 1);
  inst->setDest(newDst);
  inst->setPredicate(nullptr);
  inst->setSaturate(g4::NOSAT);
}

void HWConformityPro::fixMadw(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_madw, "expect madw instruction");

  auto execSize = inst->getExecSize();
  if (!builder.supportNativeSIMD32())
    vISA_ASSERT(execSize != g4::SIMD32,
                "SIMD32 is not supported on this platform for madw");

  auto dst = inst->getDst();
  vISA_ASSERT(IS_DTYPE(dst->getType()), "dst only supports DW type");

  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);
  auto src2 = inst->getSrc(2);
  if (src0->isImm()) {
    if (!src1->isImm()) {
      inst->swapSrc(0, 1);
      src0 = inst->getSrc(0);
      src1 = inst->getSrc(1);
    } else {
      replaceSrc(it, bb, 0, IS_UNSIGNED_INT(src0->getType()) ? Type_UD : Type_D,
                 /*tmpStride*/ 0, Any);
      src0 = inst->getSrc(0);
    }
  }

  // Fix immediate type of G4_madw as it can only support D/UD types
  if (src1->isImm()) {
    uint32_t immVal = (uint32_t)src1->asImm()->getImm();
    inst->setSrc(builder.createImm(
                     immVal, IS_SIGNED_INT(src1->getType()) ? Type_D : Type_UD),
                 1);
    src1 = inst->getSrc(1);
  }

  if (src2->isImm()) {
    uint32_t immVal = (uint32_t)src2->asImm()->getImm();
    inst->setSrc(builder.createImm(
                     immVal, IS_SIGNED_INT(src2->getType()) ? Type_D : Type_UD),
                 2);
    src2 = inst->getSrc(2);
  }

  vISA_ASSERT(IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()) &&
                  IS_DTYPE(src2->getType()),
              "only DW-type sources are supported");

  // If src2 is immediate value 0, there is no need to generate addc and add
  // instructions.
  bool notDoAdd = src2->isImm() && src2->asImm()->getImm() == 0;

  // GRF number of tmp variable for mullh dst. Stride is always 1.
  G4_Type dstType = dst->getType();
  int mullhDstLowGRFNum = (int)std::ceil((float)(execSize * TypeSize(dstType)) /
                                         builder.getGRFSize());
  int mullhDstTotalGRFNum = mullhDstLowGRFNum * 2;
  // Tmp variable for mullh dst
  G4_Declare *mullhTmpDcl = nullptr;
  G4_DstRegRegion *mullhDst = nullptr;
  G4_Type tmpType;

  if (notDoAdd) {
    // If src2 is 0, then madw will convert to gen mullh only:
    //   mullh  (16) dst<1>:d    src0<1;1,0>:d    src1<1;1,0>:d

    // Check if mullh dst crosses GRF boudaries:
    // 1, For SIMD32, dst(both low and high) cannot span more than 4 adjacent
    //    GRFs. In other words, the dst stride must be 1.
    // 2, For < SIMD32, dst(both low and high) cannot span more than 2 adjacent
    //    GRFs.
    bool dstCrossGrfBound =
        (execSize == g4::SIMD32 && dst->getHorzStride() > 1) ||
        (execSize != g4::SIMD32 && (unsigned)execSize * dst->getHorzStride() >
                                       builder.numEltPerGRF<Type_D>());

    // mullh does not support saturation and must be GRF-aligned for dst
    if (isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst) ||
        !builder.tryToAlignOperand(dst, builder.getGRFSize()) ||
        dstCrossGrfBound || inst->getSaturate() != g4::NOSAT) {
      fixDstMadw(it, bb, builder);
      dst = inst->getDst();
    }

    mullhDst = builder.duplicateOperand(dst);
    tmpType = dstType;
  } else {
    // If src2 is not 0 and has unsigned datatype, then madw will convert to gen
    // mullh + addc + add:
    //   madw (16) dst<1>:ud src0<1;1,0>:ud src0<1;1,0>:ud src2<1;1,0>:ud
    //   =>
    //   mullh  (16) mullh_dst<1>:ud    src0<1;1,0>:ud    src0<1;1,0>:ud
    //   addc (16) dst_lo32<1>:ud  mullh_dst_lo32<1;1,0>:ud  src2<1;1,0>:ud
    //   add  (16) dst_hi32<1>:ud  acc0.0<1;1,0>:ud    mullh_dst_hi32<1;1,0>:ud
    // If src2 is not 0 and has signed datatype, then madw will convert to gen
    // mullh + addc + mov + add3:
    //   madw (16) dst<1>:d src0<1;1,0>:ud src0<1;1,0>:ud src2<1;1,0>:d
    //   =>
    //   mullh  (16) mullh_dst<1>:d    src0<1;1,0>:ud    src1<1;1,0>:ud
    //   addc (16) dst_lo32<1>:ud  mullh_dst_lo32<1;1,0>:ud  src2<1;1,0>:ud
    //   mov (16) signExt<1>:q  src2<1;1,0>:d
    //   add3  (16) dst_hi32<1>:d  signExt.1<2;1,0>:d   acc0.0<1;1,0>:d
    //              mullh_dst_hi32<1;1,0>:d
    tmpType =
        (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()) &&
         IS_UNSIGNED_INT(src2->getType()))
            ? Type_UD
            : Type_D;

    // Create tmp variable for mullh dst
    mullhTmpDcl = builder.createTempVar(builder.numEltPerGRF(tmpType) *
                                            mullhDstTotalGRFNum,
                                        tmpType, builder.getGRFAlign());
    mullhDst = builder.createDstRegRegion(mullhTmpDcl, 1);
  }

  // Create mullh instruction:
  //   mullh  (16) mullh_dst<1>:d    src0<1;1,0>:d    src1<1;1,0>:d
  uint32_t origOptions = inst->getOption();
  G4_Predicate *origPredicate = inst->getPredicate();
  auto mullhInst = builder.createBinOp(
      G4_mullh, execSize, mullhDst, builder.duplicateOperand(src0),
      builder.duplicateOperand(src1), origOptions, false);
  mullhInst->setPredicate(origPredicate);
  mullhInst->setOptionOff(InstOpt_AccWrCtrl);
  mullhInst->setVISAId(inst->getVISAId());
  (*it) = mullhInst;

  // no need to do addc + add instructions
  if (notDoAdd)
    return;

  // create addc instruction:
  //   addc (16) dst_lo32<1>:d  mullh_dst_lo32<1;1,0>:d  src2<1;1,0>:d
  unsigned dstStride = dst->getHorzStride();
  auto dstLo32 =
      builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                        dstStride, dst->getType());
  G4_Declare *mullhTmpDclLo =
      builder.createTempVar(builder.numEltPerGRF(tmpType) * mullhDstLowGRFNum,
                            tmpType, builder.getGRFAlign());
  mullhTmpDclLo->setAliasDeclare(mullhTmpDcl, 0);
  auto src0Add = builder.createSrcRegRegion(
      mullhTmpDclLo, execSize == g4::SIMD1 ? builder.getRegionScalar()
                                           : builder.getRegionStride1());
  auto addcInst =
      builder.createBinOp(G4_addc, execSize, dstLo32, src0Add,
                          builder.duplicateOperand(src2), origOptions, false);
  auto *accDstOpnd =
      builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
  addcInst->setPredicate(builder.duplicateOperand(origPredicate));
  addcInst->setImplAccDst(accDstOpnd);
  addcInst->setOptionOn(InstOpt_AccWrCtrl);
  auto insertIter = bb->insertAfter(it, addcInst);

  // If src2 is signed datatype, we need to extend the sign bit of src2 which
  // is the addend for higher 32-bits result calculation:
  //  mov (16) signExt<1>:q  src2<1;1,0>:d
  G4_Declare *signExtDclQword = nullptr;
  if (src2->getType() == Type_D) {
    signExtDclQword = builder.createTempVar(
        builder.numEltPerGRF(Type_Q) * execSize, Type_Q, builder.getGRFAlign());
    auto movDst = builder.createDstRegRegion(signExtDclQword, 1);
    auto movInst = builder.createMov(
        execSize, movDst, builder.duplicateOperand(src2), origOptions, false);
    movInst->setPredicate(builder.duplicateOperand(origPredicate));
    movInst->setOptionOff(InstOpt_AccWrCtrl);
    insertIter = bb->insertAfter(insertIter, movInst);
  }

  // Create add or add3 instruction:
  // If src2 is unsigned datatype:
  //   add  (16) dst_hi32<1>:d  acc0.0<1;1,0>:d  mullh_dst_hi32<1;1,0>:d
  // Otherwise:
  //   add3 (16) dst_hi32<1>:d  signExt.1<2;1,0>:d acc0.0<1;1,0>:d
  //             mullh_dst_hi32<1;1,0>:d
  int DstHiRegOffset = (int)std::ceil(
      (float)(execSize * dst->getExecTypeSize()) / builder.getGRFSize());
  auto *dstHi32 =
      builder.createDst(dst->getBase(), dst->getRegOff() + DstHiRegOffset,
                        dst->getSubRegOff(), dstStride, dstType);
  G4_Declare *mullhTmpDclHi =
      builder.createTempVar(builder.numEltPerGRF(tmpType) * mullhDstLowGRFNum,
                            tmpType, builder.getGRFAlign());
  mullhTmpDclHi->setAliasDeclare(mullhTmpDcl,
                                 mullhDstLowGRFNum * builder.getGRFSize());
  auto srcAdd = builder.createSrcRegRegion(
      mullhTmpDclHi, execSize == g4::SIMD1 ? builder.getRegionScalar()
                                           : builder.getRegionStride1());
  auto accSrcOpnd =
      builder.createSrc(builder.phyregpool.getAcc0Reg(), 0, 0,
                        execSize == g4::SIMD1 ? builder.getRegionScalar()
                                              : builder.getRegionStride1(),
                        tmpType);
  G4_INST *addOrAdd3Inst = nullptr;
  if (src2->getType() == Type_D) {
    G4_Declare *signExtDclDword =
        builder.createTempVar(builder.numEltPerGRF(tmpType) * execSize * 2,
                              tmpType, builder.getGRFAlign());
    signExtDclDword->setAliasDeclare(signExtDclQword, 0);
    auto src0Add3 =
        builder.createSrc(signExtDclDword->getRegVar(), 0, 1,
                          execSize == g4::SIMD1 ? builder.getRegionScalar()
                                                : builder.getRegionStride2(),
                          tmpType);
    addOrAdd3Inst = builder.createInternalInst(
        nullptr, G4_add3, nullptr, g4::NOSAT, execSize, dstHi32, src0Add3,
        accSrcOpnd, srcAdd, origOptions);
  } else {
    addOrAdd3Inst = builder.createBinOp(G4_add, execSize, dstHi32, accSrcOpnd,
                                        srcAdd, origOptions, false);
  }
  addOrAdd3Inst->setPredicate(builder.duplicateOperand(origPredicate));
  addOrAdd3Inst->setOptionOff(InstOpt_AccWrCtrl);
  bb->insertAfter(insertIter, addOrAdd3Inst);
}

// Restrictions for fcvt instruction:
// 1, For down conversion (hf -> bf8/fp8), if dst is packed(stride is 1) with
//    subreg offset .0/.16/.32/.48, src must be packed(stride is 1) as well with
//    subreg offset .0/.16 and must not span more than 1 registers. Additionally
//    modulo 32 of destination offset must be equal to source offset when
//    converting from 16bit to 8bit format. Otherwise, follow the int pipeline
//    rules to check register region restrictions which treat fp8/bf8 to int8
//    and hf to int16.
// 2, For up conversion (bf8/fp8 -> hf), follow the int pipeline rules to check
//    register region restrictions which treat fp8/bf8 to int8 and hf to int16.
//    Packed int8 should be already supported with the src0 testriction rules.
// 3, For float to tf32 coversion, follow the int pipeline rule which
//    should have no restriction on this case.
void HWConformityPro::fixFcvt(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_fcvt, "expect fcvt instruction");

  auto dst = inst->getDst();
  auto src = inst->getSrc(0);
  auto dstType = dst->getType();
  auto srcType = src->getType();
  if (IS_BTYPE(dstType) || IS_BTYPE(srcType)) {
    vISA_ASSERT(((IS_BTYPE(dstType) && srcType == Type_HF) ||
                 (IS_BTYPE(srcType) && dstType == Type_HF)),
                "Only FP8<->HF conversion is supported");
    vISA_ASSERT(!inst->getPredicate() && !inst->getCondMod(),
                "FP8<->HF move does not support pred/cond mod");
    vISA_ASSERT(src->isSrcRegRegion(),
                "HF<->FP8 currently supports non-imm source only");
    vISA_ASSERT(src->isSrcRegRegion() && !src->asSrcRegRegion()->isIndirect() &&
                    src->asSrcRegRegion()->getModifier() == Mod_src_undef,
                "FP8<->HF move does not support source modifier");

    // Packed layout support for hf to hf8/bf8 coversion:
    // Dst is packed with subreg offset .0/.16/.32/.48, then src must be packed
    // as well with subreg offset .0/.16 and must not span more than 1 register.
    bool isPackedDst = dst->getHorzStride() == 1;
    bool isScalarSrc = src->asSrcRegRegion()->getRegion()->isScalar();
    // Dst subreg offset is .0 or .16 or .32 or .48
    bool dstOffsetAlignedTo16Bytes = builder.tryToAlignOperand(dst, 16);
    // For packed down converts HW doesn't support scalar broadcast on src. So,
    // do not do packed regioning fix.
    if (IS_BTYPE(dstType) && isPackedDst && dstOffsetAlignedTo16Bytes &&
        !isScalarSrc) {
      // Src must be packed with offset .0 or .16 and must not span more than
      // 1 register.
      bool isPackedSrc =
          src->asSrcRegRegion()->getRegion()->isContiguous(inst->getExecSize());
      if (!isPackedSrc || !builder.tryToAlignOperand(src, 32) ||
          src->crossGRF(builder)) {
        replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
        src = inst->getSrc(0);
      }

      // check if dst subreg offset is .0 or .32
      bool dstOffsetAlignedTo32Bytes = builder.tryToAlignOperand(dst, 32);
      // check if src subreg offset is .0 only
      bool srcOffsetGrfAligned =
          builder.tryToAlignOperand(src, builder.getGRFSize());
      // Modulo 32 of destination offset must be equal to source offset when
      // converting from 16bit to 8bit format
      if (dstOffsetAlignedTo32Bytes && !srcOffsetGrfAligned) {
        // dst subreg offset is .0/.32, and src subreg offset is .16
        replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
      } else if (!dstOffsetAlignedTo32Bytes && srcOffsetGrfAligned) {
        // dst subreg offset is .16/.48, and src subreg offset is .0
        replaceDstWithRawMov(it, bb, /*tmpStride*/ 1, builder.getGRFAlign());
      }

      inst->setOptionOn(InstOpt_WriteEnable);
      return;
    }

    // Must follow the restrictions of int pipeline, which treat hf as int16
    // and bf8/hf8 as int8

    // Packed destination, where destination datatype times destination stride
    // is less than the execution datatype is not allowed if execution size is
    // more than one.
    auto dstStrideInBytes = dst->getTypeSize() * dst->getHorzStride();
    auto execChannelWidth = inst->getExecTypeSizeXe3p();
    if (inst->getExecSize() != g4::SIMD1 &&
        (dstStrideInBytes < execChannelWidth ||
         !isAllowedTrueRegionPatternOnSrc0(src)))
      // Fix by moving dst to a tmp with same datatype and dword-aligned
      replaceDstWithRawMov(it, bb, 4 / TypeSize(dstType),
                           builder.getGRFAlign());
    inst->setOptionOn(InstOpt_WriteEnable);
    return;
  }
}

// Restrictions for srnd instruction:
// Src0 restriction:
// 1, For hf->bf8: if dst is packed(stride is 1) with subreg offset
//    .0/.16/.32/.48, src0 must be packed(stride is 1) as well with subreg offset
//    .0/.16 and must not span more than 1
//    register. Additionally modulo 32 of destination offset must be equal to
//    source offset when converting from 16bit to 8bit format.
//    Otherwise, follow the int pipeline rules to check register
//    region restrictions which treat bf8 to int8 and hf to int16.
// 2, For f->hf: if dst is packed(stride is 1) with subreg offset .0/.16, src0
//    must be packed(stride is 1) as well with subreg offset .0 only and must
//    not span more than 1 register. Otherwise, follow the int pipeline rules
//    to check register region restrictions which treat hf to int16 and f to
//    int32.
// 3, Src0 can not be immediate
// Src1 restriction:
// Strictly aligned with dst to avoid any shuffle operation. And can not be
// broadcast. But for fp8 output, src1 is 8bits, only even offset supported,
// destination offset could be the same offset or plus 1.
void HWConformityPro::fixSrnd(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_srnd, "expect srnd instruction");

  auto dst = inst->getDst();
  auto dstType = dst->getType();
  auto src0 = inst->getSrc(0);

  // Check src0
  if (src0->isImm()) {
    // src0 can not be immediate
    replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 0,
                         builder.getGRFAlign());
    src0 = inst->getSrc(0);
  }

  G4_SrcRegRegion *src0RR = src0->asSrcRegRegion();
  bool packedDst = dst->getHorzStride() == 1;
  bool packedSrc0 = src0RR->getRegion()->isContiguous(inst->getExecSize()) &&
                    inst->getExecSize() != g4::SIMD1;
  bool Src0IsScalar = src0RR->getRegion()->isScalar();
  // Valid offset of packed dst for down-conversion:
  //   For 8bits datatype, dst subreg offset must be .0 or .16 or .32 or .48
  //   For 16bits datatype, dst subreg offset must be .0 or .16
  bool isValidOffsetForPakcedDst =
      (IS_BTYPE(dstType) && builder.tryToAlignOperand(dst, 16)) ||
      (dstType == Type_HF && builder.tryToAlignOperand(dst, 32));

  // For down conversion from float to hf, if inst is SIMD32, then do not do
  // packed layout fix as src0 always expands 1 register.
  // For packed down converts HW doesn't support scalar broadcast on src. So,
  // do not do packed regioning fix.
  if (IS_BTYPE(dstType) && packedDst && isValidOffsetForPakcedDst &&
      !Src0IsScalar) {
    // Down-conversion: packed hf -> packed bf8
    // Src0 must be packed with offset .0 or .16 and must not span more than
    // 1 register.
    if (!packedSrc0 || !builder.tryToAlignOperand(src0, 32) ||
        src0->crossGRF(builder)) {
      replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
      src0 = inst->getSrc(0);
    }

    // check if dst subreg offset is .0 or .32
    bool dstOffsetAlignedTo32Bytes = builder.tryToAlignOperand(dst, 32);
    // check if src subreg offset is .0 only
    bool srcOffsetGrfAligned =
        builder.tryToAlignOperand(src0, builder.getGRFSize());
    // Modulo 32 of destination offset must be equal to source offset when
    // converting from 16bit to 8bit format
    if (dstOffsetAlignedTo32Bytes && !srcOffsetGrfAligned) {
      // dst subreg offset is .0/.32, and src subreg offset is .16
      replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
    } else if (!dstOffsetAlignedTo32Bytes && srcOffsetGrfAligned) {
      // dst subreg offset is .16/.48, and src subreg offset is .0
      replaceDstWithRawMov(it, bb, /*tmpStride*/ 1, builder.getGRFAlign());
    }
  } else if (dstType == Type_HF && packedDst && isValidOffsetForPakcedDst &&
             inst->getExecSize() != g4::SIMD32 && !Src0IsScalar) {
    // Down-conversion: packed f -> packed hf
    // Src0 be packed with offset .0 and must not span more than 1 register
    if (!packedSrc0 ||
        !builder.tryToAlignOperand(src0, builder.numEltPerGRF<Type_UB>()))
      replaceSrcWithRawMov(it, bb, 0, /*tmpStride*/ 1, builder.getGRFAlign());
  } else {
    // Follow the restrictions of int pipeline, which treat hf as int16
    // and bf8 as int8

    // Packed destination, where destination datatype times destination stride
    // is less than the execution datatype is not allowed if execution size is
    // more than one.
    auto dstStrideInBytes = dst->getTypeSize() * dst->getHorzStride();
    auto execChannelTypeSize = inst->getExecTypeSizeXe3p();
    if (inst->getExecSize() != g4::SIMD1 &&
        (dstStrideInBytes < execChannelTypeSize ||
         !isAllowedTrueRegionPatternOnSrc0(src0)))
      // Fix by moving dst to a tmp with same datatype and dword-aligned
      replaceDstWithRawMov(it, bb, 4 / TypeSize(dstType),
                           builder.getGRFAlign());
  }

  // Check src1
  auto src1 = inst->getSrc(1);
  auto src1Type = src1->getType();

  // When src1 is register, its type must align with dst
  vISA_ASSERT(src1->isImm() ||
      (dstType == Type_HF && src1Type == Type_UW) ||
      (IS_BTYPE(dstType) && src1Type == Type_UB),
      "src1 must be uw or ub datatype");
  // When src1 is imm, the type must be uw
  vISA_ASSERT(!src1->isImm() || src1Type == Type_UW,
      "imm src1 must be uw type");
  // When src1 is imm and dst is byte-type, src1's value must fit into a byte
  vISA_ASSERT(!src1->isImm() || !IS_BTYPE(dstType) ||
      G4_Imm::isInTypeRange(src1->asImm()->getImm(), G4_Type::Type_B),
      "src1 must align with dst");

  if (!src1->isSrcRegRegion())
    return;
  auto src1RR = src1->asSrcRegRegion();

  // Since src0 supports true region, so dst may not have fixed subreg. We need
  // to fix dst firstly. And then aligne src1 to dst.
  // For example:
  // srnd(16) r10.?<2>:hf r20.1<1;1,0>:f r30.1<1>:uw
  // =>
  // mov(16) r50.0<2>:uw r30.1<1>:uw
  // srnd(16) r40.0<2>:hf r20.1<1;1,0>:f r50.0<2;1,0>:uw
  // mov(16) r10.?<2>:hf r40.0<2;1,0>:hf
  //
  // If dst has fixed subreg. Just need to align src1 to dst. For example:
  // srnd(16) r10.2<4>:bf8 r20.1<1;1,0>:hf r30.1<1>:ub
  // =>
  // mov(16) r40.2<4>: ub r30.1<1>:ub
  // srnd(16) r10.2<4>:bf8 r20.1<1;1,0>:hf r40.2<4;1,0>:ub
  dst = inst->getDst();
  uint32_t dstSubOffInBytes = 0;
  bool dstHasFixedSubReg = dst->hasFixedSubregOffset(builder, dstSubOffInBytes);
  // Dst does not have fixed subreg offset
  if (!dstHasFixedSubReg) {
    replaceDstWithRawMov(it, bb, 4 / TypeSize(dstType), builder.getGRFAlign());
    dstSubOffInBytes = 0;
    dst = inst->getDst();
  }
  auto dstStride = dst->getHorzStride();

  uint32_t src1SubOffInBytes = 0;
  bool src1HasFixedSubReg =
      src1RR->hasFixedSubregOffset(builder, src1SubOffInBytes);
  uint16_t src1Stride = 0;
  src1RR->getRegion()->isSingleStride(inst->getExecSize(), src1Stride);
  // Special handle for SIMD1
  if (inst->getExecSize() == g4::SIMD1)
    src1Stride = 1;

  // Src1 regioning pattern must be flat, offset and channel aligned with
  // destination. But if src1 is 8bits, only even offset supported due to
  // encoding constrain, sub-register need to be aligned to the destination
  // with 16bits channel boundary.
  //   srnd (32|M0) r40.0<2>:bf8 r16.0<1;1,0>:hf r18.0<2;1,0>:ub {Atomic}
  //   srnd (32|M0) r40.1<2>:bf8 r17.0<1;1,0>:hf r19.0<2;1,0>:ub
  bool isAlignedSrc1 =
      src1HasFixedSubReg && dstStride == src1Stride &&
      (src1Type == Type_UW ? dstSubOffInBytes == src1SubOffInBytes
                           : (src1SubOffInBytes % 2 == 0 &&
                              src1SubOffInBytes / 2 == dstSubOffInBytes / 2));

  // Src1 does not support scalar broadcast
  if ((src1RR->isScalar() && inst->getExecSize() != g4::SIMD1) ||
      !isAlignedSrc1) {
    // Compute the subreg offset to use
    uint16_t tmpSubRegOff = dstSubOffInBytes / TypeSize(src1Type);
    // if src1 is :ub and not even offset, its offset should minus 1.
    if (src1Type == Type_UB && tmpSubRegOff % 2 != 0)
      tmpSubRegOff -= 1;
    uint16_t tmpStride = dstStride;
    auto tmpSize = inst->getExecSize() * tmpStride + tmpSubRegOff;
    G4_Declare *tmpDcl =
        builder.createTempVar(tmpSize, src1Type, builder.getGRFAlign());

    G4_DstRegRegion *tmpDst = builder.createDst(
        tmpDcl->getRegVar(), 0, tmpSubRegOff, tmpStride, src1Type);
    auto newInst = builder.createMov(inst->getExecSize(), tmpDst, src1,
                                     inst->getMaskOption(), false);
    bb->insertBefore(it, newInst);

    const RegionDesc *tmpSrcRegion =
        builder.createRegionDesc(tmpStride, 1, 0);
    G4_SrcRegRegion *tmpSrc = builder.createSrc(
        tmpDcl->getRegVar(), 0, tmpSubRegOff, tmpSrcRegion, src1Type);
    inst->setSrc(tmpSrc, 1);
  }
}

G4_SubReg_Align HWConformityPro::getDclAlignment(int opndBytes,
                                                  G4_INST* inst,
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
    if (inst->isMath()) {
      subAlign = builder.getGRFAlign();
    }
  }

  return subAlign;
}

std::pair<G4_Operand *, bool> HWConformityPro::insertMovBeforeAndGetInserted(
    INST_LIST_ITER it,
                                                                              G4_BB *bb,
                                                                              uint32_t srcNum,
                                                                              G4_Type type,
                                                                              uint16_t tmpStride,
                                                                              G4_SubReg_Align tmpAlign) {
  G4_INST *inst = *it;
  G4_SubReg_Align subAlign;
  const RegionDesc *region = nullptr;
  G4_ExecSize execSize = inst->getExecSize();
  G4_Operand *src = inst->getSrc(srcNum);
  bool wasMovInserted = false;

  if (src->isNullReg())
    return std::make_pair(builder.createNullSrc(type), wasMovInserted);

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
          ? kernel().numEltPerGRF<Type_UB>() / 2 *
                (execSize > 8 ? execSize / 8 : 1)
          : (src->getType() == Type_VF ? kernel().numEltPerGRF<Type_UB>() / 2 *
                                             (execSize > 4 ? execSize / 4 : 1)
                                       : newExecSize * TypeSize(type) * scale);

  subAlign = getDclAlignment(opExecWidthBytes, inst, newExecSize == 1);

  if (subAlign < tmpAlign) {
    subAlign = tmpAlign;
  }

  uint32_t newInstEMask =
      newExecSize == 1 ? InstOpt_WriteEnable : inst->getMaskOption();

  G4_Declare *dcl = builder.createTempVar(
      newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);
  G4_DstRegRegion *dstRegion = builder.createDstRegRegion(dcl, scale);
  G4_INST *newInst =
      builder.createMov(newExecSize, dstRegion, builder.duplicateOperand(src),
                        newInstEMask, false);
  bb->insertBefore(it, newInst);
  wasMovInserted = true;

  G4_SrcModifier modifier = Mod_src_undef;
  if (src->isSrcRegRegion()) {
    G4_SrcModifier srcMod = src->asSrcRegRegion()->getModifier();
    if (srcMod == Mod_Not) {
      // mov doesn't support logic modifiers, so we keep it on the new source
      modifier = Mod_Not;
      newInst->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
    }
  }

  return std::make_pair(builder.createSrcRegRegion(modifier, Direct, dcl->getRegVar(), 0,
                                    0,
                                    region, dcl->getElemType()), wasMovInserted);
}

std::pair<G4_DstRegRegion *, bool>
HWConformityPro::insertMovAfterAndGetInserted(INST_LIST_ITER it,
                                                                                  G4_BB *bb,
                                                                                  G4_DstRegRegion* dst,
                                                                                  G4_Type type,
                                                                                  uint16_t tmpStride,
                                                                                  G4_SubReg_Align dstAlign) {
  G4_INST *inst = *it;
  bool wasMovInserted = false;

  if (!dst) {
    return std::make_pair(dst, wasMovInserted);
  }

  if (inst->hasNULLDst()) {
    return std::make_pair(builder.createDst(dst->getBase(), 0, 0, tmpStride ? tmpStride : 1,
                             type), wasMovInserted);
  }

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

  G4_ExecSize exec_size = inst->getExecSize();
  G4_ExecSize newExecSize = ((inst->opcode() == G4_sel || !scalarSrc)
                                 ? exec_size
                                 : g4::SIMD1);

  G4_Type execType = inst->isRawMov() ? dst->getType() : inst->getExecType();
  if (TypeSize(execType) == 8 && IS_BTYPE(type)) {
    type = (type == Type_UB ? Type_UW : Type_W);
  }
  uint16_t scale = TypeSize(execType) / TypeSize(type);
  if (newExecSize > 1 && tmpStride > 0) {
    scale = tmpStride;
  } else {
    if (scale == 0)
      scale = 1;
  }
  vISA_ASSERT(scale < 8, "invalid horizontal stride");

  uint32_t opExecWidthBytes = newExecSize * TypeSize(execType) * scale;
  uint16_t dstWidthBytes = newExecSize * TypeSize(type) * scale;
  G4_SubReg_Align subAlign = getDclAlignment(
      opExecWidthBytes > dstWidthBytes ? opExecWidthBytes : dstWidthBytes, inst,
      newExecSize == 1);
  if (subAlign < dstAlign)
    subAlign = dstAlign;

  const RegionDesc *region = newExecSize > 1
                                 ? builder.createRegionDesc(scale, 1, 0)
                                 : builder.getRegionScalar();

  G4_Declare *dcl = builder.createTempVar(
      newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);

  G4_SrcRegRegion *srcRegion = builder.createSrcRegRegion(dcl, region);
  G4_Predicate *pred = nullptr;

  if (inst->opcode() != G4_sel) {
    pred = inst->getPredicate();
    inst->setPredicate(nullptr);
  }

  unsigned int new_option = inst->getMaskOption();
  G4_INST *newInst =
      builder.createMov(exec_size, dst, srcRegion, new_option, false);
  newInst->setPredicate(pred);
  bb->insertAfter(it, newInst);
  wasMovInserted = true;

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

  return std::make_pair(builder.createDstRegRegion(dcl, scale), wasMovInserted);
}

bool HWConformityPro::isAllSrcsAlignedToDst(
    G4_INST* inst, uint8_t exChannelWidth,
    std::function<bool(uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
                       uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
                       uint8_t exChannelWidth)> checkFlatRegRegionFunc) {
  for (int i = 0, srcNum = inst->getNumSrc(); i < srcNum; ++i) {
    auto src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion())
      continue;

    auto srcRR = src->asSrcRegRegion();
    if (inst->isIntegerPipeInstructionXe() && i == 0 &&
        allowTrueRegionOnSrc0(inst)) {
      if (!isAllowedTrueRegionPatternOnSrc0(srcRR))
        return false;
    } else {
      if (!(srcRR->isScalar() ||
            srcRR->isFlatRegRegion(exChannelWidth, checkFlatRegRegionFunc)))
        return false;
    }
  }
  return true;
}

// Implement HW unsupported datatypes on mov instruction:
// -- There is no direct conversion from B/UB to DF or DF to B/UB.
//    Use two instructions and a word or DWord intermediate type.
// -- There is no direct conversion from B/UB to Q/UQ or Q/UQ to B/UB.
//    Use two instructions and a word or DWord intermediate integer type.
// -- There is no direct conversion from HF/BF to DF or DF to HF/BF.
//    Use two instructions and F (Float) as an intermediate type.
// -- There is no direct conversion from HF/BF to Q/UQ or Q/UQ to HF/BF.
//    Use two instructions and F (Float) or a word integer type or a DWord
//    integer type as an intermediate type.
void HWConformityPro::fixMov(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_mov, "expect mov instruction");

  auto dstType = inst->getDst()->getType();
  auto srcType = inst->getSrc(0)->getType();

  bool dstByteSrc64b =
      IS_BTYPE(dstType) && (IS_DFTYPE(srcType) || IS_QTYPE(srcType));
  bool srcByteDst64b =
      IS_BTYPE(srcType) && (IS_DFTYPE(dstType) || IS_QTYPE(dstType));
  if (dstByteSrc64b || srcByteDst64b) {
    replaceDst(it, bb, Type_D, /*tmpStride*/ 0, Any);
    return;
  }

  bool dstHFBFSrc64b = isLowPrecisionFloatTy(dstType) &&
                       (IS_DFTYPE(srcType) || IS_QTYPE(srcType));
  bool srcHFBFDst64b = isLowPrecisionFloatTy(srcType) &&
                       (IS_DFTYPE(dstType) || IS_QTYPE(dstType));
  if (dstHFBFSrc64b || srcHFBFDst64b) {
    replaceDst(it, bb, Type_F, /*tmpStride*/ 0, Any);
    return;
  }
}

void HWConformityPro::fixIndiret(G4_BB *bb) {
  for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
    G4_INST *inst = *it;

    if (inst->nonALUInstructions())
      continue;

    fix1x1Indirect(it, bb);

    fixVxHVx1Indirect(it, bb);

    fixIndirectMoviSimd16ToSimd8(it, bb);

    // This function should be called after fixVxHVx1Indirect() as
    // fixVxHVx1Indirect may cause the ImmAddrOffset OOB issue.
    fixImmAddrOffsetOOB(it, bb);
  }
}

// There is a 10-bits encoding limit on immediate address offset. So the value
// must be [-512,511].
void HWConformityPro::fixImmAddrOffsetOOB(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  auto generateAddrAddInst = [&](G4_Operand *var, unsigned int sregOff,
                                 short imm,int execSize) {
    // Generate address add instruction:
    //  add(execSize) A(0,0)<1>:uw  A(0,0)<1;1,0>:uw  imm:w
    auto addrDst =
        builder.createDst(var->getBase(), 0, sregOff, 1, Type_UW);
    auto addrSrc = builder.createSrc(var->getBase(), 0, sregOff,
                                     builder.getRegionStride1(), Type_UW);
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
/*
 * This function evenly splits movi from simd16 to simd8.
 */
void HWConformityPro::fixIndirectMoviSimd16ToSimd8(INST_LIST_ITER i,
                                                   G4_BB *bb) {
  G4_INST *inst = *i;
  if (inst->opcode() != G4_movi)
    return;
  if (inst->getExecSize() == g4::SIMD16) {
    // split the instruction
    evenlySplitInst(i, bb);
  }
}

// Restrictions of indirect Vx1/VxH addressing mode:
// 1, For int pipeline, Vx1/VxH is allowed only on src0 and can not support
//    qword and byte datatypes.
// 2, For float/long/math pipelines, Vx1/VxH indirect addressing modes are not
//    supported.
void HWConformityPro::fixVxHVx1Indirect(INST_LIST_ITER it, G4_BB *bb) {
  // At this point (after fixRegRegionRestricts()), Vx1/VxH region should only
  // be on src0 and limited to below cases:
  // 1, Raw mov with df datatype. Fix here.
  // 2, Raw mov with other fp datatypes except for df. Will be fixed in
  //    fixRawMovRegRegionRestrictions() later.
  // 3, Src0 is qw/byte datatype in the instructions of int pipeline. Fix here.
  //    FIXME: how to handle byte datatype???
  // 4, Src0 is other int datatypes except for qw/byte in the instructions of
  //    int pipeline. No need to fix as allowed.

  G4_INST *inst = *it;

  auto src0 = inst->getSrc(0);
  if (!src0 || !src0->isSrcRegRegion() || !src0->isIndirect() ||
      !src0->asSrcRegRegion()->getRegion()->isRegionWH())
    return;

  auto src0Type = src0->getType();
  auto src0RR = src0->asSrcRegRegion();
  if (TypeSize(src0Type) == 8) {
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
    auto tmpSrcDcl = builder.createTempVar(numElement, src0Type, Any);
    auto originWidth = src0RR->getRegion()->width;
    auto originHS = src0RR->getRegion()->horzStride;

    // mov instruction for lower half
    auto tmpDcl = builder.createTempVar(numElement * 2, Type_UD, Any);
    tmpDcl->setAliasDeclare(tmpSrcDcl, 0);
    auto lowerDst = builder.createDst(tmpDcl->getRegVar(), 0, 0, 2, Type_UD);
    auto newSrcRegion =
        (originWidth == 1) ? src0RR->getRegion() : // VxH
            builder.createRegionDesc(UNDEFINED_SHORT, originWidth,
                                     originHS * 2); // Vx1
    auto lowerSrc = builder.createIndirectSrc(
        Mod_src_undef, src0RR->getBase(), src0RR->getRegOff(),
        src0RR->getSubRegOff(), newSrcRegion, Type_UD, src0RR->getAddrImm());
    auto lowerMovInst = builder.createMov(inst->getExecSize(), lowerDst,
                                          lowerSrc, inst->getOption(), false);
    lowerMovInst->setPredicate(inst->getPredicate());
    bb->insertBefore(it, lowerMovInst);

    // mov instruction for higher half
    auto higherDst = builder.createDst(tmpDcl->getRegVar(), 0, 1, 2, Type_UD);
    auto higherSrc = builder.createIndirectSrc(
        Mod_src_undef, src0RR->getBase(), src0RR->getRegOff(),
        src0RR->getSubRegOff(), newSrcRegion, Type_UD,
        src0RR->getAddrImm() + 4);
    auto higherMovInst = builder.createMov(inst->getExecSize(), higherDst,
                                           higherSrc, inst->getOption(), false);
    higherMovInst->setPredicate(inst->getPredicate());
    bb->insertBefore(it, higherMovInst);

    // change the src0 of the instruction
    const RegionDesc *newRegion = builder.getRegionStride1();
    G4_SrcRegRegion *tmpSrcOpnd = builder.createSrcRegRegion(
        src0RR->getModifier(), Direct, tmpSrcDcl->getRegVar(), 0, 0, newRegion,
        tmpSrcDcl->getElemType());
    inst->setSrc(tmpSrcOpnd, 0);
  }
}

void HWConformityPro::fixLzd(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_lzd, "expect lzd instruction");

  if (inst->getSrc(0)->isImm() && inst->getSrc(0)->getType() != Type_UD) {
    uint32_t immVal = (uint32_t)inst->getSrc(0)->asImm()->getImm();
    inst->setSrc(builder.createImm(immVal, Type_UD), 0);
  }

  vISA_ASSERT(inst->getDst()->getType() == Type_UD, "dst must be UD type");
  vISA_ASSERT(inst->getSrc(0)->getType() == Type_UD, "src must be UD type");
}

void HWConformityPro::fixLfsr(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_lfsr, "expect lfsr instruction");

  // Fix immediate src operand whose type can only be :ud
  for (int i = 0; i < 2; i++) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isImm() && src->getType() == Type_UW) {
      // Just change the immediate's type to :ud
      uint32_t immVal = (uint32_t)src->asImm()->getImm();
      inst->setSrc(builder.createImm(immVal, Type_UD), i);
    }
  }

  vISA_ASSERT(inst->getDst()->getType() == Type_UD &&
                  inst->getSrc(0)->getType() == Type_UD &&
                  inst->getSrc(1)->getType() == Type_UD,
              "all operands of lfsr must be UD type");
}

uint16_t HWConformityPro::getSrcStrideInBytes(G4_SrcRegRegion *src) {
  uint16_t srcStride = 0;
  src->getRegion()->isSingleStride(src->getInst()->getExecSize(), srcStride);
  srcStride *= src->getTypeSize();
  return srcStride;
}

// For three-source instructions, there are some encoding restrictions:
// 1, the SubRegNum must be Word-aligned for all operands
// 2, dst stride must be 1 or 2
// 3, either src0 or src2 can be 16b imm
// 4, all operands can not be indirect
// 5, the following regions are supported:
//    <N;N,0>
//    <0;1,0>
//    <W*H;W,H>
void HWConformityPro::fix3SrcInstEncodeRestriction(INST_LIST_ITER it,
                                                    G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->getNumSrc() == 3, "Expect three-source inst");

  G4_DstRegRegion *dst = inst->getDst();
  auto dstTySize = dst->getTypeSize();
  int alignInBytes = std::max((int)dstTySize, builder.get3SrcDstAlign());
  if (!builder.tryToAlignOperand(dst, alignInBytes) ||
      dst->getHorzStride() > 2) {
    uint16_t tmpDstStride = inst->getExecTypeSizeXe3p() / dstTySize;
    vISA_ASSERT(tmpDstStride <= 2, "dst stride must be 1 or 2");
    replaceDstWithRawMov(it, bb, tmpDstStride, builder.getGRFAlign());
  }

  // Check if the src operand has valid single stride.
  auto checkSingleStrideRegion = [&](G4_SrcRegRegion *src) {
    uint8_t execSize = src->getInst()->getExecSize();
    const RegionDesc *srcRegion = src->asSrcRegRegion()->getRegion();
    uint16_t stride = 0;
    if (!srcRegion->isSingleStride(execSize, stride))
      return false;

    if (stride > 4) {
      return false;
    } else if (srcRegion->isContiguous(execSize)) {
      // Normalize the region if it is not.
      if (srcRegion->width != 1)
        src->setRegion(builder, builder.getRegionStride1(),
                       /*invariant*/ true);
    } else if (src->asSrcRegRegion()->getRegAccess() == Direct &&
               src->crossGRF(builder)) {
      // Make sure only VertStride is used to cross GRF register boundaries
      int width = 2;
      int vStride = stride * 2;
      int elementSize = src->getTypeSize();
      int startOffset =
          src->getLeftBound() % builder.kernel.numEltPerGRF<Type_UB>();
      for (int row = 0; row < execSize / width; row++) {
        int rowOffset = (startOffset + row * vStride * elementSize) %
                        builder.numEltPerGRF<Type_UB>();
        if (rowOffset + (width - 1) * stride * elementSize >=
            (int)builder.kernel.numEltPerGRF<Type_UB>())
          return false;
      }
    }
    return true;
  };

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    auto src = inst->getSrc(i);
    if (src->isImm()) {
      // Either src0 or src2 can be 16b imm
      if ((i == 0 || i == 2) && src->getTypeSize() <= 2)
        continue;
    } else if (!src->isIndirect()){
      // FIXME: Src1/src2 has no regioning and must be aligned to dst. It will
      // be done in fixRegRegionInt(/Math/Float)Pipe. So no need to check
      // here??
      if (i != 0)
        continue;

      auto srcRR = src->asSrcRegRegion();
      if (srcRR->isScalar())
        continue;

      if (checkSingleStrideRegion(srcRR))
        continue;

      // Src0 may use <N;N,0> region as they come with a vStride in encoding.
      const RegionDesc *srcRegion = srcRR->getRegion();
      if (srcRegion->vertStride == srcRegion->width &&
          srcRegion->horzStride == 0 &&
          (srcRegion->width == 4 || srcRegion->width == 8))
        continue;
    }

    // Fix src by inserting mov instruction
    replaceSrcWithRawMov(it, bb, i, /*tmpStride*/ 0, Any);
  }
  return;
}

// Second half of a source operand must not point to the same register as the
// first half of destination operand in a compressed instruction.
void HWConformityPro::fixDstSrcOverlap(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (inst->nonALUInstructions() || inst->opcode() == G4_madm)
    return;

  auto dst = inst->getDst();
  if (dst->isNullReg() || !dst->getBase()->isRegVar() || !dst->getTopDcl()) {
    return;
  }

  unsigned grfSize = builder.getGRFSize();
  G4_Declare *dstDcl = dst->getTopDcl();
  bool dstCrossGRF =
      ((dst->getSubRegOff() * dst->getTypeSize()) % grfSize +
       (dst->getLinearizedEnd() - dst->getLinearizedStart()) + 1) > grfSize;
  bool dstCross2GRF =
      ((dst->getSubRegOff() * dst->getTypeSize()) % grfSize +
       (dst->getLinearizedEnd() - dst->getLinearizedStart()) + 1) > grfSize * 2;
  int dstFirstHalf = dst->getLinearizedStart() / grfSize;

  bool srcOverlap = false;
  for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++) {
    G4_Operand *src = inst->getSrc(i);
    if (!src || src->isNullReg() || !src->getTopDcl() || !src->isSrcRegRegion())
      continue;

    G4_SrcRegRegion *srcRg = src->asSrcRegRegion();
    G4_Declare *srcDcl = src->getTopDcl();
    if (srcDcl == dstDcl && srcRg->getRegAccess() == Direct &&
        srcRg->getBase()->isRegVar()) {
      bool srcCrossGRF =
          ((srcRg->getSubRegOff() * srcRg->getTypeSize()) % grfSize +
           (srcRg->getLinearizedEnd() - srcRg->getLinearizedStart()) + 1) >
          grfSize;
      bool srcCross2GRF =
          ((srcRg->getSubRegOff() * srcRg->getTypeSize()) % grfSize +
           (srcRg->getLinearizedEnd() - srcRg->getLinearizedStart()) + 1) >
          grfSize * 2;

      // The half define in region rule "second half of a source operand
      // must not point to the same register as the first half of
      // destination operand in a compressed instruction" is exactly size
      // half, not GRF boundary based half.
      int srcSecondHalf = 0;
      if (srcRg->getRegion()->isContiguous(
              inst->getExecSize())) { // For contiguous region, linear
                                      // start/end can be used to calculate
                                      // the start GRF of half size of
                                      // region
        srcSecondHalf =
            (srcRg->getLinearizedStart() +
             ((srcRg->getLinearizedEnd() - srcRg->getLinearizedStart() + 1) /
              2)) /
            grfSize;
      } else {
        // For non-congtiguous region, there are holes in the region,
        // the start of second half elements need be calcauted in
        // stride and elemement sizes at same time.
        // Such as in following cases, there is no first/second half overlap
        // issues.
        // add(M1, 32) V146(0,1)<2> V146(0,1)<2;1,0> V146(0,0)<2;1,0>
        // add(M1, 16) V147(0,2)<4> V147(0,2)<4;1,0> V147(0,1)<4;1,0>
        // add(M1, 16) V148(0,3)<4> V148(0,3)<4;1,0> V148(0,1)<4;1,0>
        const RegionDesc *regionDesc = srcRg->getRegion();
        uint16_t vertSize = regionDesc->vertStride * srcRg->getElemSize();
        uint16_t execTypeSize =
            regionDesc->horzStride == 0
                ? srcRg->getElemSize()
                : regionDesc->horzStride * srcRg->getElemSize();
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
                                   : (srcRg->getLinearizedEnd() -
                                      srcRg->getLinearizedStart() + 1) /
                                         execTypeSize;
        srcSecondHalf =
            (srcRg->getLinearizedStart() + (totalNumEle / 2) * vertSize) /
            grfSize;
      }

      if (dstCross2GRF || srcCross2GRF) {
        if (inst->opcode() == G4_mullh || inst->opcode() == G4_madw) {
          // Special case for SIMD32 mullh/madw instruction:
          // The dst occupies 4 contiguous GRFs and src occupies 2 contiguous
          // GRFs. But the first phase will write to the 1st and 3rd GRF of
          // dst. For example:
          // mullh (32|M0)  r6.0<1>:ud  -(abs)r7.0<1;1,0>:ud  -r19.0<1;1,0>:d
          // The 1st phase will write r6 and r8, and the 2nd phase will read
          // r8 as source. So, dst and src are overlapped.
          if (dstFirstHalf == srcSecondHalf ||
              (dstFirstHalf + 2) == srcSecondHalf) {
            srcOverlap = true;
            break;
          }
        } else {
          // Other SIMD32 instructions with 64b datatypes:
          // Either dst or src may occupy 4 GRFs. Need to compare 2 GRFs of dst
          // or source. For examples:
          // add (32|M0) r6.0<1>:q  r3.0<1;1,0>:q  r10.0<0;1,0>:q
          // add (32|M0) r6.0<1>:q  r4.0<1;1,0>:q  r10.0<0;1,0>:q
          // add (32|M0) r6.0<1>:q  r5.0<1;1,0>:q  r10.0<0;1,0>:q
          // Above instructions all have dst and src0 overlapped
          int dstFisrtHalfLeftBound = dstFirstHalf;
          int dstFirstHalfRightBound =
              dstCross2GRF ? (dstFirstHalf + 1) : dstFirstHalf;
          int srcSecondHalfLeftBound = srcSecondHalf;
          int srcSecondHalfRightBound =
              srcCross2GRF ? (srcSecondHalf + 1) : srcSecondHalf;
          if (srcSecondHalfLeftBound <= dstFirstHalfRightBound &&
              srcSecondHalfRightBound >= dstFisrtHalfLeftBound) {
            srcOverlap = true;
            break;
          }
        }
      } else if (dstCrossGRF || srcCrossGRF) {
        if (dstFirstHalf == srcSecondHalf) {
          srcOverlap = true;
          break;
        }
      }
    } else if (srcRg->isIndirect()) {
      auto pointsToSet = pointsToAnalysis.getAllInPointsTo(srcDcl->getRegVar());
      for (auto &pt : *pointsToSet) {
        G4_Declare *dcl = pt.var->getDeclare();
        if (dstDcl == dcl) {
          srcOverlap = true;
          break;
        }
      }
    }
  }

  if (srcOverlap) {
    // madw and mullh need separate handling as they have both low and high
    // results
    if (inst->opcode() == G4_mullh || inst->opcode() == G4_madw) {
      fixDstMadw(it, bb, builder);
    } else {
      G4_AccRegSel accSel = inst->getDst()->getAccRegSel();
      replaceDstWithRawMov(it, bb, /*tmpStride*/ 0, Any);
      inst->getDst()->setAccRegSel(accSel);
    }
  }
}

// We need WA if pred's size is greater than inst's exec size
// and the platform does not support predctrl group size (indicated by the
// fact we have PRED_ANY_WHOLE and PRED_ALL_WHOLE). The case where pred size
// is less than inst's exec size is already undefined even with predCtrl
// group size.
void HWConformityPro::fixPredCtrl(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  G4_Predicate *pred = inst->getPredicate();

  if (!pred ||
      !(pred->getControl() == PRED_ANY_WHOLE ||
        pred->getControl() == PRED_ALL_WHOLE) ||
      pred->getTopDcl()->getNumberFlagElements() <= inst->getExecSize())
    return;

  // (f0.any32h) sel (1) ...
  // =>
  // cmp (1) [ne] f1 f0 0
  // (f1) sel (1) ...
  //
  // (f0.all32h) sel (1) ...
  // =>
  // cmp (1) [e] f1 f0 0xFFFFFFFF
  // (f1) sel (1) ...
  //
  // If f0 happens to be < 16 elements we have to clear upper bits as well
  // in case it has garbage values.
  G4_Declare *flagDcl = pred->getTopDcl();
  vISA_ASSERT(
      !inst->getCondMod(),
      "currently don't handle an instruction with conditional modifier");
  vISA_ASSERT((inst->isWriteEnableInst() || bb->isAllLaneActive()),
              "don't handle instruction in SIMD CF for now");
  G4_Declare *tmpFlag = builder.createTempFlag(1);
  G4_Type flagType = flagDcl->getNumberFlagElements() == 32 ? Type_UD : Type_UW;
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

// Evenly split an inst into two instructions with half execution size.
// This will has two instructions: one is right before "iter", the other is to
// re-use "iter". The caller is safe to use "--iter" and "iter" to refer those
// two instructions.
void HWConformityPro::evenlySplitInst(INST_LIST_ITER iter, G4_BB *bb) {
  G4_INST *inst = *iter;
  G4_opcode op = inst->opcode();
  G4_Operand *srcs[3];
  int origMaskOffset = inst->getMaskOffset();
  const int numSrc = inst->getNumSrc();

  bool useARF = false;
  for (int i = 0; i < numSrc; i++) {
    srcs[i] = inst->getSrc(i);
  }

  G4_DstRegRegion *dst = inst->getDst();
  bool nullDst = dst && inst->hasNULLDst();

  // Check src/dst dependency
  if (!nullDst) {
    for (int i = 0; i < numSrc; i++) {
      bool useTmp = false;
      G4_CmpRelation rel = dst->compareOperand(srcs[i], builder);
      if (rel != Rel_disjoint) {
        useTmp = (rel != Rel_eq) ||
                 srcs[i]->asSrcRegRegion()->getRegion()->isRepeatRegion(
                     inst->getExecSize());
      }

      if (useTmp) {
        // insert mov
        replaceSrcWithRawMov(iter, bb, i, /*tmpStride*/ 1, Any);
        srcs[i] = inst->getSrc(i);
      }
    }
  }

  // compute max exeuction size.
  // boundary is GRF-boundary and HS change, but for Dst, elements should be
  // symetric if half-GRF boundary is crossed.

  G4_ExecSize instExSize = inst->getExecSize(),
              currExSize = G4_ExecSize(instExSize / 2);

  G4_Predicate *newPred = nullptr;
  if (inst->getPredicate()) {
    newPred = inst->getPredicate();
    newPred->splitPred();
  }

  G4_CondMod *newCond = nullptr;
  if (inst->getCondMod()) {
    newCond = inst->getCondMod();
    newCond->splitCondMod();
  }

  G4_SrcRegRegion *accSrcRegion = nullptr;
  if (inst->getImplAccSrc()) {
    accSrcRegion = inst->getImplAccSrc()->asSrcRegRegion();
  }

  G4_DstRegRegion *accDstRegion = nullptr;
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
        } else if (op == G4_movi && srcs[j]->asSrcRegRegion()->getRegion()->isRegion110()) {
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
  }
}

void HWConformityPro::fix2SrcInstImm(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  if (inst->getNumSrc() != 2)
    return;

  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);

  // Src0 can not be imm for binary instructions
  if (src0 && (src0->isImm() || src0->isAddrExp())) {
    if (INST_COMMUTATIVE(inst->opcode()) && !src1->isImm()) {
      // swap src0 and src1
      inst->swapSrc(0, 1);
    } else if (inst->opcode() == G4_sel && !src1->isImm()) {
      /*
       * A select operation isn't commutative, but we may commute the
       * operands provided we perform a predicate inversion as well.
       * (v0)  sel ... const V1
       *    =>
       * (-v0) sel ... V1 const
       */
      auto cond = inst->getCondMod();
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
        auto pred = inst->getPredicate();
        vISA_ASSERT(pred, "predicate must not be null");
        G4_PredState reverse = pred->getState() == PredState_Minus
                                   ? PredState_Plus
                                   : PredState_Minus;
        inst->setPredicate(builder.createPredicate(reverse, pred->getBase(),
                                                   pred->getSubRegOff(),
                                                   pred->getControl()));
      }
      inst->swapSrc(0, 1);
    } else {
      // If src0 is not 64-bit, src1 is 64-bit, swap them to save one move.
      if (INST_COMMUTATIVE(inst->opcode()) && src0->isImm() && src1->isImm() &&
          src0->getTypeSize() != 8 && src1->getTypeSize() == 8) {
        inst->swapSrc(0, 1);
        src0 = inst->getSrc(0);
        src1 = inst->getSrc(1);
        // this needs to fall through as we still need move for src0
      }

      if (INST_COMMUTATIVE(inst->opcode()) && src0->isAddrExp() &&
          src1->isImm()) {
        // The original IR has both addr expr and immediate
        //   add(8) A0(0,0)<1>:uw &V36 + 0 0xeca86420:uv
        // We insert a move for src1 which is an immediate
        //   mov(8) TV0(0,0)<1>:uw 0xeca86420:uv
        //   add(8) A0(0,0)<1>:uw &V36+0 TV0(0,0)<8;8,1>:uw
        G4_Type type = src1->getType();
        replaceSrc(it, bb, 1, G4_Operand::GetNonVectorImmType(type),
                   /*tmpStride*/ 0, Any);
        src1 = inst->getSrc(1);

        // And we swap addr expr and the new variable
        //   add(8) A0(0,0)<1>:uw TV0(0,0)<8;8,1>:uw &V36+0
        // The final code sequence is
        //   mov(8) r13.0<1>:uw 0xeca86420:uv
        //   add(8) a0.0<1>:uw r13.0<8;8,1>:uw 0x60:uw
        inst->swapSrc(0, 1);
      } else {
        replaceSrc(it, bb, 0, G4_Operand::GetNonVectorImmType(src0->getType()),
                   /*tmpStride*/ 0, Any);
      }
    }
  }
}

void HWConformityPro::fixAddcSubb(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_addc || inst->opcode() == G4_subb,
              "expect addc and subb instructions");

  // vISA has constraited that ADDC and SUBB must be :ud data type, but
  // in previous function "fixMadw()" it may generate addc instruction
  // with :d data type. We can either fix in fixMadw() or this function.
  auto dst = inst->getDst();
  if (dst->getType() == Type_D)
    dst->setType(builder, Type_UD);
  vISA_ASSERT(inst->getDst()->getType() == Type_UD,
              "dst of addc/subb must be :ud data type");

  // Fix immediate src operand whose type can only be :ud
  for (int i = 0; i < 2; i++) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isImm() && src->getType() == Type_UW) {
      // Just change the immediate's type to :ud
      uint32_t immVal = (uint32_t)src->asImm()->getImm();
      inst->setSrc(builder.createImm(immVal, Type_UD), i);
    } else if (src->isSrcRegRegion() && src->getType() == Type_D) {
      src->asSrcRegRegion()->setType(builder, Type_UD);
    }
    vISA_ASSERT(inst->getSrc(i)->getType() == Type_UD,
                "src of addc/subb must be :ud data type");
  }
}

// 1, :v and :uv must not be used when destination is any of the float datatypes.
// 2, When an immediate vector is used in an instruction, the destination must be
// 128-bit aligned with destination horizontal stride equivalent to a word for
// an immediate integer vector (v). For example:
//    mov (8) r5.0<2>:uw 0xfdb97531:uv
//    =>
//    mov (8) r6.0<1>:uw 0xfdb97531:uv
//    mov (8) r5.0<2>:uw r6.0<1;1,0>:uw
// 3, non-word type src is not allowed to co-exist with :v src.
void HWConformityPro::fixVectImm(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  bool hasVectImm = false;
  bool incompatibleSrcTypeFound = false;
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isVectImm()) {
      hasVectImm = true;
    } else {
      // Make sure other src operands are of word type only as this is a HW
      // requirement
      if (src->getType() != Type_W && src->getType() != Type_UW)
        incompatibleSrcTypeFound = true;
    }
  }

  if (!hasVectImm)
    return;

  auto dst = inst->getDst();
  uint8_t execSize = inst->getExecSize();
  bool dstAligned = builder.tryToAlignOperand(dst, 16);
  unsigned dstStrideInBytes = dst->getHorzStride() * dst->getTypeSize();

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (!src->isVectImm())
      continue;

    G4_Type ty = src->getType();
    vISA_ASSERT(ty != Type_VF,
                ":vf datatype is not supported on this platform");

    G4_Type moveTy = (ty == Type_V) ? Type_W : Type_UW;

    if (!dstAligned || IS_TYPE_FLOAT_ALL(dst->getType()) ||
        incompatibleSrcTypeFound) {
      replaceSrc(it, bb, i, moveTy, /*tmpStride*/ 0, /*tmpAlign*/ Any);
    } else if (dstStrideInBytes != TypeSize(moveTy)) {
      if (dstStrideInBytes == 4 && execSize < 8) {
        // For the case where dst is dword and execution size is < 8,
        // we can interleave the vector to avoid a move. For example:
        //   mov (2) r1.0<1>:d 0x21:uv
        //   -->
        //   mov (2) r1.0<1>:d 0x0201:uv
        uint32_t bitValue = 0;
        uint16_t immBits = (uint16_t)src->asImm()->getImm();
        for (int k = 0; k < execSize; ++k) {
          int val = (immBits >> (k * 4)) & 0xF;
          bitValue |= val << (k * 8);
        }
        inst->setSrc(builder.createImm(bitValue, ty), i);
      } else {
        replaceSrc(it, bb, i, moveTy, /*tmpStride*/ 0, /*tmpAlign*/ Any);
      }
    }
  }
}

void HWConformityPro::fixImm64(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    auto src = inst->getSrc(i);
    if (!src->isImm() || src->getTypeSize() != 8)
      continue;

    // A 64bit immediate is supported only for a MOV operation
    if (inst->opcode() != G4_mov)
      replaceSrcWithRawMov(it, bb, i, /*tmpStride*/ 0, Any);
  }
}

// In HWConformity, acc can be implicit dst or explict src. In such case, we
// must make acc operand is algined to dst. Since we always use sub-reg offset
// as 0 and stride as 1 for ACC, dst must be also GRF-aligned and have stride
// as 1. For example:
//   addc (16) r1.1<2> ...
//   mov (16) r2.1<2> acc0.0<1;1,0>
// into
//   addc (16) r2.0<1> ...
//   mov (16) r1.1<2>  r2.0<2;1,0>
//   mov (16) r3.0<1> acc0.0<1;1,0>
//   mov (16) r2.1<2> r3.0<1>
void HWConformityPro::fixAccRestrictions(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (!inst->hasACCOpnd())
    return;

  vISA_ASSERT(!inst->getDst()->isAccReg(), "unexpected explicit ACC dst");

  // Make dst as GRF-aligned, and dst stride as 1. Otherwise, the implicit
  // acc dst or explicit acc src won't be aligned with dst.
  if (!builder.tryToAlignOperand(inst->getDst(),
                                 builder.numEltPerGRF<Type_UB>()) ||
      inst->getDst()->getHorzStride() != 1)
    replaceDstWithRawMov(it, bb, /*tmpStride*/ 1, builder.getGRFAlign());
}

// Fix ARF regstricons:
// Explicit ARF(not including ACC, NULL) are only allowed on dst and src0.
void HWConformityPro::fixARF(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (inst->getNumSrc() < 2)
    return;

  // Local propagating may propagate flag register to src1
  auto isARF = [](G4_Operand *opnd) {
    return opnd->isAreg() || opnd->isFlag();
  };
  auto src0 = inst->getSrc(0);
  auto src1 = inst->getSrc(1);
  if (isARF(src1) && !src1->isNullReg() && !src1->isAccReg()) {
    // See if we can swap the src1
    if (INST_COMMUTATIVE(inst->opcode()) && !isARF(src0)) {
      inst->swapSrc(0, 1);
      inst->swapDefUse();
    } else {
      // Otherwise introduce a tmp
      replaceSrcWithRawMov(it, bb, 1, /*tmpStride*/ 0, Any);
    }
  }
}

void HWConformityPro::fixFbl(INST_LIST_ITER it, G4_BB *bb) {
  // FBL requires its operands to be UD
  G4_INST *inst = *it;
  vISA_ASSERT(inst->opcode() == G4_fbl, "expect fbl instruction");

  auto dst = inst->getDst();
  auto dstTy = dst->getType();
  if (dstTy != Type_UD) {
    if (dstTy == Type_D)
      dst->setType(builder, Type_UD);
    else
      replaceDst(it, bb, Type_UD, /*tmpStride*/ 0, Any);
  }

  auto src = inst->getSrc(0);
  auto srcTy = src->getType();
  if (srcTy != Type_UD) {
    if (src->isImm()) {
      G4_Operand *newSrc = builder.createImm(src->asImm()->getImm(), Type_UD);
      inst->setSrc(newSrc, 0);
    } else if (srcTy == Type_D){
      src->asSrcRegRegion()->setType(builder, Type_UD);
    } else {
      replaceSrc(it, bb, 0, Type_UD, /*tmpStride*/ 0, Any);
    }
  }
}

// Restrictions of indirect 1x1 addressing mode:
// For math pipeline, 1x1 indirect addressing is not allowed.
// For float/int pipelines, 1x1 indirect addressing is allowed on dst, src0 and
// src1.
void HWConformityPro::fix1x1Indirect(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  // For instructions in non-math pipelines, 1x1 indirect addressing is not
  // allowed on src2. But do nothing here as fix3SrcInstEncodeRestriction()
  // has fixed it.
  if (!inst->isMath())
    return;

  // For instructions in math pipeline
  if (inst->getDst()->isIndirect())
    replaceDstWithRawMov(it, bb, /*tmpStride*/ 0, /*dstAlign*/ Any);
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    auto src = inst->getSrc(i);
    if (!src->isSrcRegRegion())
      continue;
    if (src->isIndirect() && !src->asSrcRegRegion()->getRegion()->isRegionWH())
      replaceSrcWithRawMov(it, bb, i, /*stride*/ 0, /*tmpAlign*/ Any);
  }
  return;
}

void HWConformityPro::fixShr(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  assert(inst->opcode() == G4_shr && "expect shr instruction");

  // dst/src0 must be unsigned type
  auto dst = inst->getDst();
  auto dstTy = dst->getType();
  if (IS_SIGNED_INT(dstTy)) {
    auto newDstTy = getUnsignedType(TypeSize(dstTy));
    if (inst->getSaturate() == g4::NOSAT)
      // If no saturation, we can directly change the type
      dst->setType(builder, newDstTy);
    else
      replaceDst(it, bb, newDstTy, /*tmpStride*/ 0, Any);
  }

  auto src0 = inst->getSrc(0);
  auto src0Ty = src0->getType();
  if (IS_SIGNED_INT(src0Ty)) {
    auto newSrc0Ty = getUnsignedType(TypeSize(src0Ty));
    if (src0->isImm()) {
      inst->setSrc(builder.createImm(src0->asImm()->getImm(), newSrc0Ty), 0);
    } else {
      src0->asSrcRegRegion()->setType(builder, newSrc0Ty);
    }
  }

  return;
}

void HWConformityPro::fixAdd(INST_LIST_ITER it, G4_BB* bb) {
  G4_INST *inst = *it;
  assert(inst->opcode() == G4_add && "expect add instruction");

  // Saturation must not be used when source or destination is a qword.
  // For example:
  // (W) add (1) (sat)v32th(0,64)<1>:q v18th(0,64)<0;1,0>:q v4th(0,64)<0;1,0>:q
  // =>
  // (W) add (1)  TV2(0,0)<1>:q  v18th(0,64)<0;1,0>:q  v4th(0,64)<0;1,0>:q
  // (W) mov (1)  (sat)v32th(0,64)<1>:q  TV2(0,0)<0;1,0>:q
  if (!inst->getSaturate())
    return;

  bool notSupportSat = IS_QTYPE(inst->getDst()->getType());
  if (!notSupportSat) {
    for (int i = 0; i < inst->getNumSrc(); ++i) {
      notSupportSat = IS_QTYPE(inst->getSrc(i)->getType());
      if (notSupportSat)
        break;
    }
  }

  if (notSupportSat) {
    replaceDst(it, bb, inst->getDst()->getType(), /*tmpStride*/ 0, Any);
    G4_INST *newMov = *(std::next(it));
    newMov->setSaturate(inst->getSaturate());
    inst->setSaturate(g4::NOSAT);
  }
}