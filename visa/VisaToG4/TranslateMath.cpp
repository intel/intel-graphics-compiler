/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"
#include "Common_ISA_util.h"

using namespace vISA;

// Helper functions
static bool useSCF(IR_Builder &aBuilder) {
  // If it is scalar IGC, use SCF if structurizer is enabled; otherwise, don't
  // use.
  if (aBuilder.kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_CM) {
    return aBuilder.getOption(vISA_EnableStructurizer);
  }
  // VC, etc. Use SCF if platform supports SCF.
  return aBuilder.hasSCF();
}

static uint32_t getSIMDSize(IR_Builder &aBuilder) {
  int32_t simdsz =
      aBuilder.kernel.getInt32KernelAttr(Attributes::ATTR_SimdSize);
  return (uint32_t)(simdsz <= 0 ? 32 : simdsz);
}

//
// convert src into a direct packed region
//
static G4_SrcRegRegion *operandToDirectSrcRegRegion(IR_Builder &builder,
                                                    G4_Operand *src,
                                                    G4_ExecSize newSize,
                                                    G4_ExecSize oldSize) {
  if (src->isSrcRegRegion()) {
    G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
    if (srcRegion->getRegAccess() == IndirGRF) {
      G4_Declare *dcl =
          builder.createTempVarWithNoSpill(newSize, src->getType(), Any);
      if (srcRegion->getRegion()->isRegionWH() && newSize > oldSize) {
        // for VxH regions we can't directly broadcast if new exec size is wider
        if (oldSize == g4::SIMD1) {
          srcRegion->setRegion(builder, builder.getRegionScalar());
          builder.createMov(newSize, builder.createDstRegRegion(dcl, 1),
                            srcRegion, InstOpt_WriteEnable, true);
        } else {
          // ToDo: i think this is needed for all regions
          auto tmpDst = builder.createDstRegRegion(dcl, 1);
          builder.createMov(oldSize, tmpDst, src, InstOpt_WriteEnable, true);
          auto tmpSrc = builder.createSrc(
              dcl->getRegVar(), 0, 0, builder.createRegionDesc(0, oldSize, 1),
              src->getType());
          builder.createMov(newSize, builder.createDstRegRegion(dcl, 1), tmpSrc,
                            InstOpt_WriteEnable, true);
        }
      } else {
        builder.createMovInst(dcl, 0, 0, newSize, nullptr, nullptr, src, true);
      }
      return builder.createSrcRegRegion(dcl, builder.getRegionStride1());
    }
    return src->asSrcRegRegion();
  } else {
    // src is an immediate
    vISA_ASSERT_INPUT(src->isImm(), "expected an immediate operand");
    G4_Declare *tmpSrc = builder.createTempVarWithNoSpill(
        newSize, G4_Operand::GetNonVectorImmType(src->getType()), Any);
    builder.createMovInst(tmpSrc, 0, 0, newSize, nullptr, nullptr, src, true);
    return builder.createSrcRegRegion(tmpSrc, builder.getRegionStride1());
  }
}

void IR_Builder::expandFdiv(G4_ExecSize exsize, G4_Predicate *predOpnd,
                            G4_Sat saturate, G4_DstRegRegion *dstOpnd,
                            G4_Operand *src0Opnd, G4_Operand *src1Opnd,
                            uint32_t instOpt) {
  // math.fdiv dst src0 src1
  // -->
  // math.inv tmp src1
  // mul dst src0 tmp
  G4_MathOp mathOp = MATH_INV;
  G4_Type invType = src1Opnd->getType();
  if (IS_VFTYPE(invType)) {
    invType = Type_F;
  }
  G4_Declare *invResult = createTempVar(exsize, invType, Any);
  G4_DstRegRegion *invDst = createDstRegRegion(invResult, 1);
  createMathInst(predOpnd, g4::NOSAT, exsize, invDst, src1Opnd,
                 createNullSrc(invType), mathOp, instOpt, true);
  G4_SrcRegRegion *invSrc = createSrcRegRegion(invResult, getRegionStride1());
  createInst(duplicateOperand(predOpnd), G4_mul, nullptr, saturate, exsize,
             dstOpnd, src0Opnd, invSrc, instOpt, true);
}

void IR_Builder::expandPow(G4_ExecSize exsize, G4_Predicate *predOpnd,
                           G4_Sat saturate, G4_DstRegRegion *dstOpnd,
                           G4_Operand *src0Opnd, G4_Operand *src1Opnd,
                           uint32_t instOpt) {
  // math.pow dst src0 src1
  // -->
  // math.log tmp abs(src0)
  // mul dst tmp tmp src1
  // math.exp dst tmp
  G4_Type mathType = G4_Operand::GetNonVectorImmType(src0Opnd->getType());
  G4_Declare *tmpVar = createTempVar(exsize, mathType, Any);
  G4_DstRegRegion *logDst = createDstRegRegion(tmpVar, 1);
  G4_Operand *logSrc = src0Opnd;
  // make sure log source is positive
  if (src0Opnd->isSrcRegRegion()) {
    G4_SrcRegRegion *srcRegion = src0Opnd->asSrcRegRegion();
    switch (srcRegion->getModifier()) {
    case Mod_src_undef:
      srcRegion->setModifier(Mod_Abs);
      break;
    case Mod_Abs:
      // do nothing
      break;
    default: {
      G4_Declare *tmpLogSrc = createTempVar(exsize, src0Opnd->getType(), Any);
      createMovInst(tmpLogSrc, 0, 0, exsize, nullptr, nullptr, src0Opnd, false,
                    instOpt);
      logSrc = createSrcRegRegion(tmpLogSrc, getRegionStride1());
      logSrc->asSrcRegRegion()->setModifier(Mod_Abs);
    }
    }
  } else {
    switch (src0Opnd->getType()) {
    case Type_F: {
      float val = src0Opnd->asImm()->getFloat();
      if (val < 0) {
        logSrc = createImm(std::abs(val));
      }
      break;
    }
    case Type_HF: {
      uint16_t val = (uint16_t)src0Opnd->asImm()->getImm();
      if (val & 0x8000) {
        logSrc = createImm(val & 0x7FFFF, Type_HF);
      }
      break;
    }
    case Type_VF:
      // ToDo: what if VF contains negative values?
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected src0 type for pow");
    }
  }
  createMathInst(predOpnd, g4::NOSAT, exsize, logDst, logSrc,
                 createNullSrc(mathType), MATH_LOG, instOpt, true);
  G4_SrcRegRegion *mulSrc = createSrcRegRegion(tmpVar, getRegionStride1());
  G4_DstRegRegion *mulDst = createDstRegRegion(tmpVar, 1);
  createInst(duplicateOperand(predOpnd), G4_mul, nullptr, g4::NOSAT, exsize,
             mulDst, mulSrc, src1Opnd, instOpt, true);
  G4_SrcRegRegion *expSrc = createSrcRegRegion(tmpVar, getRegionStride1());
  createMathInst(duplicateOperand(predOpnd), saturate, exsize, dstOpnd, expSrc,
                 createNullSrc(mathType), MATH_EXP, instOpt, true);
}

// If default rounding and denorm is not set, then
// 1, Set SP/DP retain denorm for Xe+ platforms for both fast and slow(IEEE)
//    sequences.
// 2, Set RNE rounding for all platforms for slow(IEEE) sequence only.
static void setDefaultRoundDenorm(IR_Builder &builder,
                                  bool hasDefaultRoundDenorm,
                                  bool doFast,
                                  G4_Declare *tmpCR0ForRoundDenormRestore,
                                  G4_Declare *tmpCR0ForRoundRestore) {
  if (!hasDefaultRoundDenorm) {
    if (builder.getPlatform() >= GENX_TGLLP) {
      // Save cr0.0 for rounding and denorm restore:
      //   mov (1) TEMP_R_D_RESTORE<1>:ud cr0.0<0;1,0>:ud
      G4_DstRegRegion *dstRegRegionForSaveInst =
          builder.createDstRegRegion(tmpCR0ForRoundDenormRestore, 1);
      G4_SrcRegRegion *cr0SrcRegOpndForSaveInst =
          builder.createSrc(builder.phyregpool.getCr0Reg(), 0, 0,
                            builder.getRegionScalar(), Type_UD);
      builder.createMov(g4::SIMD1, dstRegRegionForSaveInst,
                        cr0SrcRegOpndForSaveInst, InstOpt_WriteEnable, true);

      if (doFast) { // for fast sequence
        // Set retain SP/DP denorm in CR0.0[7:6]:
        //   or (1) cr0.0<1>:ud TEMP_R_D_RESTORE<0;1,0>:ud 0xc0:ud
        G4_DstRegRegion *dstRegRegionForOrInst =
            builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
        G4_SrcRegRegion *srcRegRegionForOrInst = builder.createSrcRegRegion(
            tmpCR0ForRoundDenormRestore, builder.getRegionScalar());
        builder.createBinOp(nullptr, G4_or, g4::SIMD1, dstRegRegionForOrInst,
                            srcRegRegionForOrInst,
                            builder.createImm(0xc0, Type_UD),
                            InstOpt_WriteEnable, true);

      } else { // for slow(IEEE) sequence
        // Set retain SP/DP denorm in CR0[7:6]:
        //   or (1) TMP_R_RESTORE<1>:ud TEMP_R_D_RESTORE<0;1,0>:ud 0xc0:ud
        G4_DstRegRegion *dstRegRegionForOrInst =
            builder.createDstRegRegion(tmpCR0ForRoundRestore, 1);
        G4_SrcRegRegion *srcRegRegionForOrInst = builder.createSrcRegRegion(
            tmpCR0ForRoundDenormRestore, builder.getRegionScalar());
        builder.createBinOp(nullptr, G4_or, g4::SIMD1, dstRegRegionForOrInst,
                            srcRegRegionForOrInst,
                            builder.createImm(0xc0, Type_UD),
                            InstOpt_WriteEnable, true);

        // Set rounding mode in CR0.0[5:4] to RNE:
        //   and (1) cr0.0<1>:ud TMP_R_RESTORE<0;1,0>:ud 0xffffffcf:ud
        G4_DstRegRegion *cr0DstRegOpndForAndInst =
            builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
        G4_SrcRegRegion *srcRegRegionForAndInst = builder.createSrcRegRegion(
            tmpCR0ForRoundRestore, builder.getRegionScalar());
        builder.createBinOp(nullptr, G4_and, g4::SIMD1, cr0DstRegOpndForAndInst,
                            srcRegRegionForAndInst,
                            builder.createImm(0xffffffcf, Type_UD),
                            InstOpt_WriteEnable, true);
      }
    } else if (!doFast) { // slow(IEEE) sequence for pre-Xe platforms
      // Save cr0.0 for rounding restore:
      //   mov (1) TEMP_R_RESTORE<1>:ud cr0.0<0;1,0>:ud
      G4_DstRegRegion *dstRegRegionForSaveInst =
          builder.createDstRegRegion(tmpCR0ForRoundRestore, 1);
      G4_SrcRegRegion *cr0SrcRegOpndForSaveInst =
          builder.createSrc(builder.phyregpool.getCr0Reg(), 0, 0,
                            builder.getRegionScalar(), Type_UD);
      builder.createMov(g4::SIMD1, dstRegRegionForSaveInst,
                        cr0SrcRegOpndForSaveInst, InstOpt_WriteEnable, true);

      // Set rounding mode in CR0.0[5:4] to RNE:
      //   and (1) cr0.0<1>:ud TMP_R_RESTORE<0;1,0>:ud 0xffffffcf:ud
      G4_DstRegRegion *cr0DstRegOpndForAndInst =
          builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
      G4_SrcRegRegion *srcRegRegionForAndInst = builder.createSrcRegRegion(
          tmpCR0ForRoundRestore, builder.getRegionScalar());
      builder.createBinOp(nullptr, G4_and, g4::SIMD1, cr0DstRegOpndForAndInst,
                          srcRegRegionForAndInst,
                          builder.createImm(0xffffffcf, Type_UD),
                          InstOpt_WriteEnable, true);
    }
  }
}

static void initEOFlagIfNeeded(IR_Builder &builder, G4_Declare *EOFlag,
                               bool isSetFlag) {
  if (isSetFlag) {
    G4_Type flagTy = (EOFlag->getNumElems() > 1 ? Type_UD : Type_UW);
    G4_DstRegRegion *flagDst =
        builder.createDst(EOFlag->getRegVar(), 0, 0, 1, flagTy);
    G4_Imm *imm =
        builder.createImm((flagTy == Type_UD ? 0xFFFFFFFF : 0xFFFF), flagTy);
    builder.createMov(g4::SIMD1, flagDst, imm, InstOpt_WriteEnable, true);
  }
}

// Used for restoring rounding mode only in CR0.0 before the last madm inst
// of the low(IEEE) sequence.
static G4_INST *restoreCR0_0(IR_Builder &builder, bool needRestoreCR0,
                                  G4_Declare *tmpCR0ForRestore) {
  G4_INST *restoreInst = nullptr;
  if (needRestoreCR0) {
    G4_DstRegRegion *dstRRForRoundRestoreInst =
      builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
    auto srcRRForRoundRestoreInst = builder.createSrcRegRegion(tmpCR0ForRestore,
      builder.getRegionScalar());

    // restore cr0.0
    restoreInst =
        builder.createMov(g4::SIMD1, dstRRForRoundRestoreInst,
                          srcRRForRoundRestoreInst, InstOpt_WriteEnable, true);
  }
  return restoreInst;
}

// Used for restoring both rounding and denorm in CR0.0 after the whole math
// macro.
static G4_INST *restoreCR0_0(IR_Builder &builder, bool hasDefaultRoundDenorm,
                             bool doFast,
                             G4_Declare *tmpCR0ForRoundRestore,
                             G4_Declare *tmpCR0ForRoundDenormRestore) {
  if (!hasDefaultRoundDenorm) {
    if (builder.getPlatform() >= GENX_TGLLP) {
      return restoreCR0_0(builder, true,
                          tmpCR0ForRoundDenormRestore);
    } else {
      return restoreCR0_0(builder, !doFast, tmpCR0ForRoundRestore);
    }
  }
  return nullptr;
}

int IR_Builder::translateVISAArithmeticDoubleInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_DstRegRegion *dstOpnd,
    G4_Operand *src0Opnd, G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_INST *inst = nullptr;
  G4_ExecSize instExecSize = toExecSize(executionSize);
  G4_ExecSize exsize = (G4_ExecSize)(getNativeExecSize() / 2);
  const RegionDesc *srcRegionDesc = getRegionStride1();
  const RegionDesc *rdAlign16 = getRegionStride1();
  uint8_t element_size =
      exsize; // element_size is set according to instExecSize
  unsigned int loopCount = 1;

  G4_Imm *dbl_constant_0 = createDFImm(0.0);
  G4_Imm *dbl_constant_1 = createDFImm(1.0);

  // On Xe2+ platforms if enableInterleaveMacro option is on, then do not split
  // here. Later InstSplitPass will do the split to generate interleaved macro
  // for FP64 DIV like below instructions sequence:
  //     math.invm (16|M0) (eo)f0.0  r16.mme0:df  r41.nomme:df    r61.nomme:df
  //     math.invm (16|M16) (eo)f0.0  r18.mme0:df  r55.nomme:df   r63.nomme:df
  //     (~f0.0) madm (16|M0)   acc0.mme1:df  r86.nomme:df      r41.nomme:df
  //     r16.mme0:df
  //     (~f0.0) madm (16|M16)   acc2.mme1:df  r84.nomme:df      r55.nomme:df
  //     r18.mme0:df
  //     (~f0.0) madm (16|M0)    r20.mme2:df   r96.nomme:df      -r61.nomme:df
  //     r16.mme0:df
  //     (~f0.0) madm (16|M16)   r22.mme2:df   r94.nomme:df      -r63.nomme:df
  //     r18.mme0:df
  //     (~f0.0) madm (16|M0)    r28.mme3:df   r20.mme2:df       r20.mme2:df
  //     r20.mme2:df
  //     (~f0.0) madm (16|M16)   r30.mme3:df   r22.mme2:df       r22.mme2:df
  //     r22.mme2:df
  //     (~f0.0) madm (16|M0)    acc0.mme4:df  r86.nomme:df      acc0.mme1:df
  //     r28.mme3:df
  //     (~f0.0) madm (16|M16)   acc2.mme4:df  r84.nomme:df      acc2.mme1:df
  //     r30.mme3:df
  //     (~f0.0) madm (16|M0)    r16.nomme:df  acc0.mme4:df      r41.nomme:df
  //     r16.mme0:df
  //     (~f0.0) madm (16|M16)   r18.nomme:df  acc2.mme4:df      r55.nomme:df
  //     r18.mme0:df
  // Otherwise, it will generate instrctuctions sequence like below:
  //     math.invm (16|M0) (eo)f0.0  r15.mme0:df  r40.nomme:df    r56.nomme:df
  //     {A@1}
  //     (~f0.0) madm(16|M0)  acc0.mme1:df  r70.nomme:df  r40.nomme:df
  //     r15.mme0:df{ M@1 }
  //     (~f0.0) madm(16|M0)  r35.mme2:df   r82.nomme:df  -r56.nomme:df
  //     r15.mme0:df
  //     (~f0.0) madm(16|M0)  r19.mme3:df   r35.mme2:df   r35.mme2:df
  //     r35.mme2:df{ L@1 }
  //     (~f0.0) madm (16|M0)   acc0.mme4:df  r70.nomme:df  acc0.mme1:df
  //     r19.mme3:df
  //     (~f0.0) madm (16|M0)   r15.nomme:df  acc0.mme4:df  r40.nomme:df
  //     r15.mme0:df math.invm (16|M16) (eo)f0.0  r16.mme0:df  r41.nomme:df
  //     r57.nomme:df     {A@1}
  //     (~f0.0) madm (16|M16)  acc0.mme1:df  r70.nomme:df  r41.nomme:df
  //     r16.mme0:df      {M@1}
  //     (~f0.0) madm (16|M16)  r36.mme2:df   r82.nomme:df  -r57.nomme:df
  //     r16.mme0:df
  //     (~f0.0) madm (16|M16)  r20.mme3:df   r36.mme2:df   r36.mme2:df
  //     r36.mme2:df
  //     (~f0.0) madm (16|M16)  acc0.mme4:df  r70.nomme:df  acc0.mme1:df
  //     r20.mme3:df
  //     (~f0.0) madm (16|M16)  r16.nomme:df  acc0.mme4:df  r41.nomme:df
  //     r16.mme0:df
  if (this->getOption(vISA_enableInterleaveMacro) && getPlatform() >= Xe2 &&
      (opcode == ISA_DIV || opcode == ISA_DIVM)) {
    if (instExecSize > exsize) {
      exsize = instExecSize;
      element_size = instExecSize;
      loopCount = 1;
    }
  } else
  {
    if (instExecSize > exsize) {
      element_size = instExecSize;
      exsize = std::min(instExecSize, G4_ExecSize(getFP64MadmExecSize()));
      loopCount = instExecSize / exsize;
    }
  }

  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);

  // Using either if-endif or goto/join
  // Given the following on platform invm is simd8 only:
  //
  //     divm (M1, 8) dst.0<1>:df V0035(0,0)<1;1,0>:df V0037(0,0)<1;1,0>:df
  //
  // (1) if-endif
  //       math.invm (4|M0) (eo)f0.0  r2.mme0:df  r58.nomme:df  r34.nomme:df
  //       (~f0.0) if (4|M0) L0   L0
  //          madm (4|M0) ...
  //          ....
  //          madm (4|M0) ...
  //   L0: endif   L1
  //   L1:
  //       math.invm (4|M4) (eo)f0.0  r3.mme0:df  r59.nomme:df  r35.nomme:df
  //       (~f0.0) if (4|M4) L2   L2
  //          madm (4|M4) ...
  //          ....
  //          madm (4|M4) ...
  //   L2: endif   L3
  //   L3:
  //
  // (2) goto/join
  //     As we only generate goto here and join will be inserted in
  //     processGoto(), need to follow the assumption of processGoto() in which
  //     goto/join's execsize should be the maximum number of active lanes when
  //     the control-flow reaches them. For this, need to use goto (8|M0), not
  //     goto (4|M0) or goto (4|M4)!
  //
  //       (W)  mov (1|M0) f0.0:uw 0xff:uw   // set f0.0 to EO
  //       math.invm (4|M0) (eo)f0.0  r2.mme0:df  r58.nomme:df  r34.nomme:df
  //       (~f0.0) goto (8|M0) L0   L0
  //          madm (4|M0) ...
  //          ....
  //          madm (4|M0) ...
  //   L0:
  //       (W)  mov (1|M0) f0.0:uw 0xff:uw   // set f0.0 to EO
  //       math.invm (4|M4) (eo)f0.0  r3.mme0:df  r59.nomme:df  r35.nomme:df
  //       (~f0.0) goto (8|M0) L2   L2
  //          madm (4|M4) ...
  //          ....
  //          madm (4|M4) ...
  //   L2:
  //
  bool generateIf;
  bool doFastDiv = (opcode == ISA_DIV || opcode == ISA_INV);
  switch (this->getuint32Option(vISA_PredicatedFdivSqrt)) {
  case 0: // force if-endif
    generateIf = true;
    break;
  case 1: // force predicated
    generateIf = false;
    break;
  case 2:  // visa selects
  default: // other value as default (2)
    // Using predicated for fast div
    generateIf = !doFastDiv;
    break;
  }

  // For NoMask inst, force using predicated
  if (isNoMask(emask)) {
    generateIf = false;
  }
  const bool use_goto = (!useSCF(*this));

  // pred and conModifier
  uint32_t flagSize = instExecSize + getvISAMaskOffset(emask);
  if (generateIf && use_goto) {
    // need to use flag that matches the simdsize.
    flagSize = getSIMDSize(*this);
  }
  G4_Declare *tmpFlag = createTempFlag(flagSize > 16 ? 2 : 1);
  G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

  // temp registers
  G4_Declare *t6 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t7 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t8 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t9 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t12 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t13 = createTempVarWithNoSpill(element_size, Type_DF, Any);

  // r0 = 0.0:df, r1 = 1.0:df
  G4_Declare *t0 = getImmDcl(dbl_constant_0, exsize);
  G4_Declare *t1 = getImmDcl(dbl_constant_1, exsize);

  createPseudoKills({t6, t7, t8, t9, t10, t11, t12, t13, tmpFlag},
                    PseudoKillType::Src);

  G4_SrcRegRegion tsrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);
  G4_SrcRegRegion tsrc1(*this, Mod_src_undef, Direct, t1->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);

  if (!src0Opnd) {
    // those are for drcp
    G4_SrcRegRegion valueOneScalarReg(*this, Mod_src_undef, Direct,
                                      t1->getRegVar(), 0, 0, getRegionScalar(),
                                      Type_DF);
    G4_Operand *valueOneOpnd =
        createSrcRegRegion(valueOneScalarReg); // it is used in drcp
    src0Opnd = valueOneOpnd;
  }

  bool noDstMove = exsize == 8 && !saturate && !predOpnd &&
                   tryToAlignOperand(dstOpnd, getGRFSize()) &&
                   dstOpnd->getRegAccess() == Direct &&
                   dstOpnd->getHorzStride() == 1 && instExecSize == exsize;
  if (noDstMove && (dstOpnd->getTopDcl() == src0Opnd->getTopDcl() ||
                    dstOpnd->getTopDcl() == src1Opnd->getTopDcl())) {
    noDstMove = false;
  }

  G4_SrcRegRegion *src0RR = operandToDirectSrcRegRegion(
      *this, src0Opnd, G4_ExecSize(element_size), instExecSize);
  G4_SrcRegRegion *src1RR = operandToDirectSrcRegRegion(
      *this, src1Opnd, G4_ExecSize(element_size), instExecSize);

  // src operand registers
  G4_DstRegRegion tdst_src0(*this, Direct, t6->getRegVar(), 0, 0, 1, Type_DF);
  G4_DstRegRegion tdst_src1(*this, Direct, t7->getRegVar(), 0, 0, 1, Type_DF);

  bool needsSrc0Move = src0RR->isScalar() ||
                       src0RR->getModifier() != Mod_src_undef ||
                       !tryToAlignOperand(src0RR, getGRFSize());
  if (needsSrc0Move) {
    if (opcode == ISA_DIV || opcode == ISA_DIVM) {
      G4_DstRegRegion *t6_dst_src0_opnd = createDstRegRegion(tdst_src0);
      inst = createMov(
          G4_ExecSize(element_size), t6_dst_src0_opnd, src0RR, instOpt,
          true); // mov (element_size) t6_dst_src0_opnd, src0RR {Q1/N1}
    }
  }
  bool needsSrc1Move = src1RR->isScalar() ||
                       src1RR->getModifier() != Mod_src_undef ||
                       !tryToAlignOperand(src1RR, getGRFSize());
  if (needsSrc1Move) {
    G4_DstRegRegion *t7_dst_src1_opnd = createDstRegRegion(tdst_src1);
    inst =
        createMov(G4_ExecSize(element_size), t7_dst_src1_opnd, src1RR, instOpt,
                  true); // mov (element_size) t7_dst_src1_opnd, src1RR {Q1/N1}
  }

  bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
  // Temp variable of CR0 register for both rounding and SP/DP denorm restore
  G4_Declare *tmpCR0ForRoundDenormRestore =
      createTempVarWithNoSpill(1, Type_UD, Any);
  // Temp variable of CR0 register for rounding restore only
  G4_Declare *tmpCR0ForRoundRestore = createTempVarWithNoSpill(1, Type_UD, Any);

  // each madm only handles 4 channel double data
  VISA_EMask_Ctrl currEMask = emask;
  uint16_t splitInstGRFSize =
      (uint16_t)((TypeSize(Type_DF) * exsize + getGRFSize() - 1) /
                 getGRFSize());
  for (uint16_t loopIndex = 0;
       currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
       ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize)) {

    uint16_t regIndex = loopIndex * splitInstGRFSize;
    instOpt = Get_Gen4_Emask(currEMask, exsize);
    instOpt |= isNoMask(emask) ? InstOpt_WriteEnable
                               : 0;     // setting channels for non-mad insts
    unsigned int madmInstOpt = instOpt; // setting channels for mad insts

    G4_DstRegRegion tdst6(*this, Direct, t6->getRegVar(), regIndex, 0, 1,
                          Type_DF);
    G4_DstRegRegion tdst7(*this, Direct, t7->getRegVar(), regIndex, 0, 1,
                          Type_DF);
    G4_DstRegRegion tdst8(
        *this, Direct, noDstMove ? dstOpnd->getBase() : t8->getRegVar(),
        noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, 1, Type_DF);
    G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1,
                          Type_DF);
    G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1,
                           Type_DF);
    G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1,
                           Type_DF);
    G4_DstRegRegion tdst12(*this, Direct, t12->getRegVar(), regIndex, 0, 1,
                           Type_DF);
    G4_DstRegRegion tdst13(*this, Direct, t13->getRegVar(), regIndex, 0, 1,
                           Type_DF);

    /* below 2 are prepared for G4_math with Align16, so the region 2;2,1 is
     * used not 4;4,1.*/
    G4_SrcRegRegion tsrc6_0(*this, Mod_src_undef, Direct, t6->getRegVar(),
                            regIndex, 0, rdAlign16, Type_DF);
    G4_SrcRegRegion tsrc7_0(*this, Mod_src_undef, Direct, t7->getRegVar(),
                            regIndex, 0, rdAlign16, Type_DF);

    G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct, t7->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct,
                          noDstMove ? dstOpnd->getBase() : t8->getRegVar(),
                          noDstMove ? dstOpnd->getRegOff() + regIndex
                                    : regIndex,
                          0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc12(*this, Mod_src_undef, Direct, t12->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc13(*this, Mod_src_undef, Direct, t13->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);

    G4_DstRegRegion *t8DstOpnd0 = createDstRegRegion(tdst8);
    G4_DstRegRegion *t8DstOpnd1 = createDstRegRegion(tdst8);
    G4_DstRegRegion *t8DstOpnd2 = createDstRegRegion(tdst8);
    G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
    G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);
    G4_DstRegRegion *t11DstOpnd1 = createDstRegRegion(tdst11);
    G4_DstRegRegion *t12DstOpnd0 = createDstRegRegion(tdst12);
    G4_DstRegRegion *t12DstOpnd1 = createDstRegRegion(tdst12);
    G4_DstRegRegion *t13DstOpnd0 = createDstRegRegion(tdst13);

    // src oprands passed by function calls
    // for INV instruction, src0 should be 1, contant value.
    /* below 2 are prepared for G4_math with Align16, so the region 2;2,1 is
     * used not 4;4,1.*/
    G4_SrcRegRegion fsrc0_0(*this, Mod_src_undef, Direct, src0RR->getBase(),
                            src0RR->getRegOff() +
                                ((opcode == ISA_INV) ? 0 : regIndex),
                            0, rdAlign16, Type_DF);
    G4_SrcRegRegion fsrc1_0(*this, Mod_src_undef, Direct, src1RR->getBase(),
                            src1RR->getRegOff() + regIndex, 0, rdAlign16,
                            Type_DF);

    G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(),
                          src0RR->getRegOff() +
                              ((opcode == ISA_INV) ? 0 : regIndex),
                          0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion fsrc1(*this, Mod_src_undef, Direct, src1RR->getBase(),
                          src1RR->getRegOff() + regIndex, 0, srcRegionDesc,
                          Type_DF);

    G4_SrcRegRegion *t6SrcOpnd0 = NULL;
    G4_SrcRegRegion *t6SrcOpnd1 = NULL;
    G4_SrcRegRegion *t6SrcOpnd2 = NULL;
    G4_SrcRegRegion *t6SrcOpnd3 = NULL;
    G4_SrcRegRegion *t7SrcOpnd0 = NULL;
    G4_SrcRegRegion *t8SrcOpnd0x0 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x1 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x2 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x3 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x4 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd1 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t9SrcOpnd0x0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd0x1 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
    G4_SrcRegRegion *t10SrcOpnd1 = createSrcRegRegion(tsrc10);
    G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);
    G4_SrcRegRegion *t11SrcOpnd1 = createSrcRegRegion(tsrc11);
    G4_SrcRegRegion *t12SrcOpnd0x0 = createSrcRegRegion(tsrc12);
    G4_SrcRegRegion *t12SrcOpnd0x1 = createSrcRegRegion(tsrc12);
    G4_SrcRegRegion *t12SrcOpnd0x2 = createSrcRegRegion(tsrc12);
    G4_SrcRegRegion *t12SrcOpnd0x3 = createSrcRegRegion(tsrc12);
    G4_SrcRegRegion *t12SrcOpnd1 = createSrcRegRegion(tsrc12);
    G4_SrcRegRegion *t13SrcOpnd0 = createSrcRegRegion(tsrc13);

    setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, doFastDiv,
                          tmpCR0ForRoundDenormRestore, tmpCR0ForRoundRestore);

    if (needsSrc0Move) {
      if (opcode == ISA_INV) {
        t6SrcOpnd0 = createSrcRegRegion(fsrc0_0);
        t6SrcOpnd1 = createSrcRegRegion(fsrc0);
        t6SrcOpnd2 = createSrcRegRegion(fsrc0);
        t6SrcOpnd3 = createSrcRegRegion(fsrc0);
      } else {
        t6SrcOpnd0 = createSrcRegRegion(tsrc6_0);
        t6SrcOpnd1 = createSrcRegRegion(tsrc6);
        t6SrcOpnd2 = createSrcRegRegion(tsrc6);
        t6SrcOpnd3 = createSrcRegRegion(tsrc6);
      }
    } else {
      t6SrcOpnd0 = createSrcRegRegion(fsrc0_0);
      t6SrcOpnd1 = createSrcRegRegion(fsrc0);
      t6SrcOpnd2 = createSrcRegRegion(fsrc0);
      t6SrcOpnd3 = createSrcRegRegion(fsrc0);
    }

    if (needsSrc1Move) {
      t7SrcOpnd0 = createSrcRegRegion(tsrc7_0);
    } else {
      t7SrcOpnd0 = createSrcRegRegion(fsrc1_0);
    }

    // create -r7.noacc
    G4_SrcRegRegion tsrc7_neg(*this, Mod_Minus, t7SrcOpnd0->getRegAccess(),
                              t7SrcOpnd0->getBase(), t7SrcOpnd0->getRegOff(),
                              t7SrcOpnd0->getSubRegOff(),
                              t7SrcOpnd0->getRegion(), t7SrcOpnd0->getType());
    G4_SrcRegRegion *t7SrcOpndNeg0 = createSrcRegRegion(tsrc7_neg);
    G4_SrcRegRegion *t7SrcOpndNeg1 = createSrcRegRegion(tsrc7_neg);
    G4_SrcRegRegion *t7SrcOpndNeg2 = createSrcRegRegion(tsrc7_neg);
    G4_SrcRegRegion *t7SrcOpndNeg3 = createSrcRegRegion(tsrc7_neg);

    // see comments regarding goto/join in translateVISAArithmeticDoubleInst().
    initEOFlagIfNeeded(*this, tmpFlag, generateIf && use_goto);

    // math.e0.f0.0 (4) r8.acc2 r6.noacc r7.noacc 0xe {Align16, N1/N2}
    t8DstOpnd0->setAccRegSel(ACC2);
    t6SrcOpnd0->setAccRegSel(NOACC);
    t7SrcOpnd0->setAccRegSel(NOACC);
    inst = createMathInst(NULL, g4::NOSAT, exsize, t8DstOpnd0, t6SrcOpnd0,
                          t7SrcOpnd0, MATH_INVM, madmInstOpt, true);
    G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
    inst->setCondMod(condModOverflow);

    G4_Label *gotoUIP = nullptr;
    if (generateIf) {
      if (!use_goto) {
        // if
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        inst = createIf(predicateFlagReg, exsize, instOpt);
      } else {
        gotoUIP = createLocalBlockLabel("macro");
        // visa goto (not gen goto inst) will jump on true!
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Plus, tmpFlag->getRegVar(), 0, predCtrlValue);
        G4_ExecSize gotoExSize(getSIMDSize(*this));
        inst = createCFInst(predicateFlagReg, G4_goto, gotoExSize, nullptr,
                            gotoUIP, InstOpt_NoOpt, true);
      }
    }

    // fast version
    if (doFastDiv) {

      G4_Predicate *predicateFlagReg_m1 = NULL;
      G4_Predicate *predicateFlagReg_m2 = NULL;
      G4_Predicate *predicateFlagReg_m3 = NULL;
      G4_Predicate *predicateFlagReg_m4 = NULL;
      G4_Predicate *predicateFlagReg_m5 = NULL;

      if (!generateIf) {
        predicateFlagReg_m1 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m2 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m3 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m4 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m5 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      }
      // initial quotient approximation
      // q = a * y;
      // madm (4) r9.acc3 r0.noacc r6.noacc r8.acc2 {Align16, N1/N2}
      G4_SrcRegRegion *t0SrcOpnd0 = createSrcRegRegion(tsrc0);
      t9DstOpnd0->setAccRegSel(ACC3);
      t0SrcOpnd0->setAccRegSel(NOACC);
      t6SrcOpnd1->setAccRegSel(NOACC);
      t8SrcOpnd0x0->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m1, exsize, t9DstOpnd0, t0SrcOpnd0,
                        t6SrcOpnd1, t8SrcOpnd0x0, madmInstOpt);

      // relative error:  eps = 1 -b*y
      // DP_FMA is the double precision FMA instruction
      // madm (4) r10.acc4 r1.noacc -r7.noacc r8.acc2 {Align16, N1/N2}
      G4_SrcRegRegion *t1SrcOpnd0 = createSrcRegRegion(tsrc1);
      t10DstOpnd0->setAccRegSel(ACC4);
      t1SrcOpnd0->setAccRegSel(NOACC);
      t7SrcOpndNeg0->setAccRegSel(NOACC);
      t8SrcOpnd0x1->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m2, exsize, t10DstOpnd0, t1SrcOpnd0,
                        t7SrcOpndNeg0, t8SrcOpnd0x1, madmInstOpt);

      // refine approximation
      // eps = DP_FMA(eps, eps, eps);
      // madm (4) r11.acc5 r10.acc4 r10.acc4 r10.acc4 {Align16, N1/N2}
      t11DstOpnd0->setAccRegSel(ACC5);
      t10SrcOpnd0->setAccRegSel(ACC4);
      inst = createMadm(predicateFlagReg_m3, exsize, t11DstOpnd0, t10SrcOpnd0,
                        t10SrcOpnd0, t10SrcOpnd0, madmInstOpt);

      // q*eps (extended exponent is important for accuracy in all cases)
      // q_e = q * eps;
      // madm (4) r12.acc6 r0.noacc r9.acc3 r11.acc5 {Align16, N1/N2}
      G4_SrcRegRegion *t0SrcOpnd1 = createSrcRegRegion(tsrc0);
      t12DstOpnd0->setAccRegSel(ACC6);
      t0SrcOpnd1->setAccRegSel(NOACC);
      t9SrcOpnd0x0->setAccRegSel(ACC3);
      t11SrcOpnd0->setAccRegSel(ACC5);
      inst = createMadm(predicateFlagReg_m4, exsize, t12DstOpnd0, t0SrcOpnd1,
                        t9SrcOpnd0x0, t11SrcOpnd0, madmInstOpt);

      // Final step:  inputs (other than a) are in extended exponent format
      // Output is in regular format (nomme)
      // res = a*y + q_e
      // madm (4) r8.noacc r12.acc6 r6.noacc r8.acc2 {Align16, N1/N2}
      t8DstOpnd2->setAccRegSel(NOACC);
      t12SrcOpnd0x0->setAccRegSel(ACC6);
      t6SrcOpnd2->setAccRegSel(NOACC);
      t8SrcOpnd0x2->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m5, exsize, t8DstOpnd2, t12SrcOpnd0x0,
                        t6SrcOpnd2, t8SrcOpnd0x2, madmInstOpt);
    } else {
      G4_Predicate *predicateFlagReg_m1 = NULL;
      G4_Predicate *predicateFlagReg_m2 = NULL;
      G4_Predicate *predicateFlagReg_m3 = NULL;
      G4_Predicate *predicateFlagReg_m4 = NULL;
      G4_Predicate *predicateFlagReg_m5 = NULL;
      G4_Predicate *predicateFlagReg_m6 = NULL;
      G4_Predicate *predicateFlagReg_m7 = NULL;
      G4_Predicate *predicateFlagReg_m8 = NULL;
      G4_Predicate *predicateFlagReg_m9 = NULL;
      G4_Predicate *predicateFlagReg_m10 = NULL;
      if (!generateIf) {
        predicateFlagReg_m1 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m2 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m3 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m4 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m5 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m6 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m7 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m8 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m9 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m10 = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      }

      // madm (4) r9.acc3 r0.noacc r6.noacc r8.acc2 {Align16, N1/N2}
      G4_SrcRegRegion *t0SrcOpnd = createSrcRegRegion(tsrc0);
      t9DstOpnd0->setAccRegSel(ACC3);
      t0SrcOpnd->setAccRegSel(NOACC);
      t6SrcOpnd1->setAccRegSel(NOACC);
      t8SrcOpnd0x0->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m1, exsize, t9DstOpnd0, t0SrcOpnd,
                        t6SrcOpnd1, t8SrcOpnd0x0, madmInstOpt);

      // madm (4) r10.acc4 r1.noacc -r7.noacc r8.acc2 {Align16, N1/N2}
      G4_SrcRegRegion *t1SrcOpnd0 = createSrcRegRegion(tsrc1);
      t10DstOpnd0->setAccRegSel(ACC4);
      t1SrcOpnd0->setAccRegSel(NOACC);
      t7SrcOpndNeg0->setAccRegSel(NOACC);
      t8SrcOpnd0x1->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m2, exsize, t10DstOpnd0, t1SrcOpnd0,
                        t7SrcOpndNeg0, t8SrcOpnd0x1, madmInstOpt);

      // madm (4) r11.acc5 r6.noacc -r7.noacc r9.acc3 {Align16, N1/N2}
      t11DstOpnd0->setAccRegSel(ACC5);
      t6SrcOpnd2->setAccRegSel(NOACC);
      t7SrcOpndNeg1->setAccRegSel(NOACC);
      t9SrcOpnd0x0->setAccRegSel(ACC3);
      inst = createMadm(predicateFlagReg_m3, exsize, t11DstOpnd0, t6SrcOpnd2,
                        t7SrcOpndNeg1, t9SrcOpnd0x0, madmInstOpt);

      // madm (4) r12.acc6 r8.acc2 r10.acc4 r8.acc2 {Align16, N1/N2}
      t12DstOpnd0->setAccRegSel(ACC6);
      t8SrcOpnd0x2->setAccRegSel(ACC2);
      t10SrcOpnd0->setAccRegSel(ACC4);
      t8SrcOpnd0x3->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m4, exsize, t12DstOpnd0, t8SrcOpnd0x2,
                        t10SrcOpnd0, t8SrcOpnd0x3, madmInstOpt);

      // madm (4) r13.acc7 r1.noacc -r7.noacc r12.acc6 {Align16, N1/N2}
      G4_SrcRegRegion *t1SrcOpnd1 = createSrcRegRegion(tsrc1);
      t13DstOpnd0->setAccRegSel(ACC7);
      t1SrcOpnd1->setAccRegSel(NOACC);
      t7SrcOpndNeg2->setAccRegSel(NOACC);
      t12SrcOpnd0x0->setAccRegSel(ACC6);
      inst = createMadm(predicateFlagReg_m5, exsize, t13DstOpnd0, t1SrcOpnd1,
                        t7SrcOpndNeg2, t12SrcOpnd0x0, madmInstOpt);

      // madm (4) r8.acc8 r8.acc2 r10.acc4 r12.acc6 {Align16, N1/N2}
      t8DstOpnd1->setAccRegSel(ACC8);
      t8SrcOpnd0x4->setAccRegSel(ACC2);
      t10SrcOpnd1->setAccRegSel(ACC4);
      t12SrcOpnd0x1->setAccRegSel(ACC6);
      inst = createMadm(predicateFlagReg_m6, exsize, t8DstOpnd1, t8SrcOpnd0x4,
                        t10SrcOpnd1, t12SrcOpnd0x1, madmInstOpt);

      // madm (4) r9.acc9 r9.acc3 r11.acc5 r12.acc6 {Align16, N1/N2}
      t9DstOpnd1->setAccRegSel(ACC9);
      t9SrcOpnd0x1->setAccRegSel(ACC3);
      t11SrcOpnd0->setAccRegSel(ACC5);
      t12SrcOpnd0x2->setAccRegSel(ACC6);
      inst = createMadm(predicateFlagReg_m7, exsize, t9DstOpnd1, t9SrcOpnd0x1,
                        t11SrcOpnd0, t12SrcOpnd0x2, madmInstOpt);

      // madm (4) r12.acc2 r12.acc6 r8.acc8 r13.acc7 {Align16, N1/N2}
      t12DstOpnd1->setAccRegSel(ACC2);
      t12SrcOpnd0x3->setAccRegSel(ACC6);
      t8SrcOpnd1->setAccRegSel(ACC8);
      t13SrcOpnd0->setAccRegSel(ACC7);
      inst = createMadm(predicateFlagReg_m8, exsize, t12DstOpnd1, t12SrcOpnd0x3,
                        t8SrcOpnd1, t13SrcOpnd0, madmInstOpt);

      // madm (4) r11.acc3 r6.noacc -r7.noacc r9.acc9 {Align16, N1/N2}
      t11DstOpnd1->setAccRegSel(ACC3);
      t6SrcOpnd3->setAccRegSel(NOACC);
      t7SrcOpndNeg3->setAccRegSel(NOACC);
      t9SrcOpnd1x0->setAccRegSel(ACC9);
      inst = createMadm(predicateFlagReg_m9, exsize, t11DstOpnd1, t6SrcOpnd3,
                        t7SrcOpndNeg3, t9SrcOpnd1x0, madmInstOpt);

      // Restore rounding mode in CR0.0:
      //   mov (1)  cr0.0<1>:ud  TEMP_R_RESTORE<1;1,0>:ud
      // Fast div would be 1ULP under default rounding mode and 2ULP otherwise.
      // Avoid setting rounding mode as 1-2ULP is acceptable for fast div.
      restoreCR0_0(*this, !hasDefaultRoundDenorm && !doFastDiv, tmpCR0ForRoundRestore);

      // madm (4) r8.noacc r9.acc9 r11.acc3 r12.acc2 {Align16, N1/N2}
      t8DstOpnd2->setAccRegSel(NOACC);
      t9SrcOpnd1x1->setAccRegSel(ACC9);
      t11SrcOpnd1->setAccRegSel(ACC3);
      t12SrcOpnd1->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m10, exsize, t8DstOpnd2, t9SrcOpnd1x1,
                        t11SrcOpnd1, t12SrcOpnd1, madmInstOpt);
    }

    if (generateIf) {
      if (!use_goto) {
        // endif (8) {Q1/Q2}
        inst = createEndif(exsize, instOpt);
      } else {
       vASSERT(gotoUIP);
        inst = createLabelInst(gotoUIP, true);
      }
    }

    // Restore denorm and rounding in CR0.0 after whole sequence:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_D_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, hasDefaultRoundDenorm, doFastDiv,
                 tmpCR0ForRoundRestore, tmpCR0ForRoundDenormRestore);
  }; // for loop

  // make final copy to dst
  if (!noDstMove || !hasDefaultRoundDenorm) {
    G4_SrcRegRegion tsrc8_final(
        *this, Mod_src_undef, Direct,
        noDstMove ? dstOpnd->getBase() : t8->getRegVar(),
        noDstMove ? dstOpnd->getRegOff() : 0, 0, getRegionStride1(), Type_DF);
    G4_SrcRegRegion *t8_src_opnd_final = createSrcRegRegion(tsrc8_final);
    t8_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
    if (hasDefaultRoundDenorm) {
      // mov(instExecSize) dstOpnd, t8_src_opnd_final
      inst = createInst(predOpnd, G4_mov, nullptr, saturate, instExecSize,
                        dstOpnd, t8_src_opnd_final, nullptr,
                        Get_Gen4_Emask(emask, instExecSize), true);
    } else {
      // If hasDefaultRoundDenorm is false, denorm mode may be flush to zero.
      // When denorm flush-to-zero is set, mov instructions with the same source
      // and destination data type may retain denorm as output. So, we need to
      // use mul instruction instead.
      // mul (instExecSize) dstOpnd, t8_src_opnd_final 1.0:df
      inst = createInst(predOpnd, G4_mul, nullptr, saturate, instExecSize,
                        dstOpnd, t8_src_opnd_final, createDFImm(1.0),
                        Get_Gen4_Emask(emask, instExecSize), true);
    }
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticSingleDivideIEEEInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_INST *inst;
  G4_ExecSize instExecSize = toExecSize(executionSize);
  uint8_t element_size =
      8; // element_size is changed according to insstExecSize
  G4_ExecSize exsize =
      getNativeExecSize(); // exsize is a constant and never changed
  unsigned int loopCount = 1;
  G4_InstOpts instOpt = Get_Gen4_Emask(
      emask, exsize); // for those execution size: element_size before the loop
  G4_InstOpts madmInstOpt =
      Get_Gen4_Emask(emask, exsize); // only used in the loop
  const RegionDesc *srcRegionDesc = getRegionStride1();

  G4_Imm *flt_constant_0 = createImm(float(0.0));
  G4_Imm *flt_constant_1 = createImm(float(1.0));
  if (instExecSize <= exsize) {
    element_size = exsize;
    loopCount = 1;
  } else {
    element_size = instExecSize;
    instOpt = Get_Gen4_Emask(emask, instExecSize);
    loopCount = instExecSize / exsize;
  }

  bool generateIf = (this->getuint32Option(vISA_PredicatedFdivSqrt) != 1);
  if (isNoMask(emask)) {
    generateIf = false;
  }
  const bool use_goto = (!useSCF(*this));

  // pred and conModifier
  uint32_t flagSize = instExecSize + getvISAMaskOffset(emask);
  if (generateIf && use_goto) {
    // goto requires flag to match simdsize.
    flagSize = getSIMDSize(*this);
  }
  G4_Declare *tmpFlag = createTempFlag(flagSize > 16 ? 2 : 1);
  G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

  // temp registers
  G4_Declare *t1 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t4 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t6 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t8 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t9 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_F, Any);

  // 0.0:f and 1.0:f constants
  G4_Declare *t2 = getImmDcl(flt_constant_0, exsize);
  G4_Declare *t5 = getImmDcl(flt_constant_1, exsize);

  createPseudoKills({t1, t4, t6, t8, t9, t10, t11, tmpFlag},
                    PseudoKillType::Src);

  // those are for drcp
  G4_SrcRegRegion valueOneScalarReg(*this, Mod_src_undef, Direct,
                                    t2->getRegVar(), 0, 0, getRegionScalar(),
                                    Type_F);
  G4_Operand *valueOneOpnd =
      createSrcRegRegion(valueOneScalarReg); // it is used in drcp

  if (src0Opnd == NULL) {
    src0Opnd = valueOneOpnd;
  }

  G4_SrcRegRegion *src0RR = operandToDirectSrcRegRegion(
      *this, src0Opnd, G4_ExecSize(element_size), instExecSize);
  G4_SrcRegRegion *src1RR = operandToDirectSrcRegRegion(
      *this, src1Opnd, G4_ExecSize(element_size), instExecSize);

  if (src0RR->isScalar() || src0RR->getModifier() != Mod_src_undef ||
      !tryToAlignOperand(src0RR, getGRFSize())) {
    G4_DstRegRegion *tmp = createDstRegRegion(t6, 1);
    inst = createMov(G4_ExecSize(element_size), tmp, src0RR, instOpt,
                     true); // mov (element_size) t6, src0RR {Q1/H1}
    src0RR = createSrcRegRegion(t6, getRegionStride1());
  }
  if (src1RR->isScalar() || src1RR->getModifier() != Mod_src_undef ||
      !tryToAlignOperand(src1RR, getGRFSize())) {
    G4_DstRegRegion *tmp = createDstRegRegion(t4, 1);
    inst = createMov(G4_ExecSize(element_size), tmp, src1RR, instOpt,
                     true); // mov (element_size) t4, src1RR {Q1/H1}
    src1RR = createSrcRegRegion(t4, getRegionStride1());
  }

  // t2 and t5 are constants
  G4_SrcRegRegion tsrc2(*this, Mod_src_undef, Direct, t2->getRegVar(), 0, 0,
                        srcRegionDesc, Type_F);
  G4_SrcRegRegion tsrc5(*this, Mod_src_undef, Direct, t5->getRegVar(), 0, 0,
                        srcRegionDesc, Type_F);

  G4_SrcRegRegion tsrc8_final(*this, Mod_src_undef, Direct, t8->getRegVar(), 0,
                              0, getRegionStride1(), t8->getElemType());
  G4_SrcRegRegion *t8_src_opnd_final = createSrcRegRegion(tsrc8_final);

  bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
  // Temp variable of CR0 register for both rounding and SP/DP denorm restore
  G4_Declare *tmpCR0ForRoundDenormRestore =
      createTempVarWithNoSpill(1, Type_UD, Any);
  // Temp variable of CR0 register for rounding restore only
  G4_Declare *tmpCR0ForRoundRestore = createTempVarWithNoSpill(1, Type_UD, Any);

  VISA_EMask_Ctrl currEMask = emask;
  uint16_t splitInstGRFSize =
      (uint16_t)((TypeSize(Type_F) * exsize + getGRFSize() - 1) / getGRFSize());
  for (uint16_t loopIndex = 0;
       currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
       ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize)) {
    G4_Predicate *predicateFlagReg_m1 = NULL;
    G4_Predicate *predicateFlagReg_m2 = NULL;
    G4_Predicate *predicateFlagReg_m3 = NULL;
    G4_Predicate *predicateFlagReg_m4 = NULL;
    G4_Predicate *predicateFlagReg_m5 = NULL;
    G4_Predicate *predicateFlagReg_m6 = NULL;
    G4_Predicate *predicateFlagReg_m7 = NULL;
    if (!generateIf) {
      predicateFlagReg_m1 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m2 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m3 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m4 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m5 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m6 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m7 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
    }

    uint16_t regIndex = loopIndex * splitInstGRFSize;
    // set Q1 for insts within the 1st loop and Q2 for the 2nd, if inside SIMD
    // CF
    instOpt = Get_Gen4_Emask(currEMask, exsize);
    instOpt |= isNoMask(emask) ? InstOpt_WriteEnable
                               : 0; // setting channels for non-mad insts
    madmInstOpt = instOpt;          // setting channels for mad insts

    // 1, 6, 8, 9, 10, 11
    G4_DstRegRegion tdst1(*this, Direct, t1->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst6(*this, Direct, t6->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst8(*this, Direct, t8->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1,
                           Type_F);
    G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1,
                           Type_F);

    G4_SrcRegRegion tsrc1(*this, Mod_src_undef, Direct, t1->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc4(*this, Mod_src_undef, Direct, t4->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_F);

    G4_DstRegRegion *t8DstOpnd0 = createDstRegRegion(tdst8);
    G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
    G4_DstRegRegion *t1DstOpnd0 = createDstRegRegion(tdst1);
    G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);
    G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t6DstOpnd0 = createDstRegRegion(tdst6);
    G4_DstRegRegion *t8DstOpnd1 = createDstRegRegion(tdst8);

    // src oprands passed by function calls
    G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(),
                          src0RR->getRegOff() + regIndex, 0, srcRegionDesc,
                          Type_F);
    G4_SrcRegRegion fsrc1(*this, Mod_src_undef, Direct, src1RR->getBase(),
                          src1RR->getRegOff() + regIndex, 0, srcRegionDesc,
                          Type_F);

    G4_SrcRegRegion *t4SrcOpnd0 = NULL;
    G4_SrcRegRegion *t6SrcOpnd0 = NULL;
    G4_SrcRegRegion *t6SrcOpnd1 = NULL;
    G4_SrcRegRegion *t6SrcOpnd2 = NULL;
    G4_SrcRegRegion *t6SrcOpnd3 = NULL;

    G4_SrcRegRegion *t1SrcOpnd0 = createSrcRegRegion(tsrc1);
    G4_SrcRegRegion *t1SrcOpnd1 = createSrcRegRegion(tsrc1);
    G4_SrcRegRegion *t6SrcOpnd4 = createSrcRegRegion(tsrc6);
    G4_SrcRegRegion *t8SrcOpnd0x0 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x1 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x2 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t8SrcOpnd0x3 = createSrcRegRegion(tsrc8);
    G4_SrcRegRegion *t9SrcOpnd0x0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd0x1 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
    G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);

    setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, false,
                          tmpCR0ForRoundDenormRestore, tmpCR0ForRoundRestore);

    t6SrcOpnd0 = createSrcRegRegion(fsrc0);
    t6SrcOpnd1 = createSrcRegRegion(fsrc0);
    t6SrcOpnd2 = createSrcRegRegion(fsrc0);
    t6SrcOpnd3 = createSrcRegRegion(fsrc0);

    t4SrcOpnd0 = createSrcRegRegion(fsrc1);

    // create -r4.noacc
    G4_SrcRegRegion tsrc4_neg(*this, Mod_Minus, t4SrcOpnd0->getRegAccess(),
                              t4SrcOpnd0->getBase(), t4SrcOpnd0->getRegOff(),
                              t4SrcOpnd0->getSubRegOff(),
                              t4SrcOpnd0->getRegion(), t4SrcOpnd0->getType());
    G4_SrcRegRegion *t4SrcOpndNeg0 = createSrcRegRegion(tsrc4_neg);
    G4_SrcRegRegion *t4SrcOpndNeg1 = createSrcRegRegion(tsrc4_neg);
    G4_SrcRegRegion *t4SrcOpndNeg2 = createSrcRegRegion(tsrc4_neg);

    // see comments regarding goto/join in translateVISAArithmeticDoubleInst().
    initEOFlagIfNeeded(*this, tmpFlag, generateIf && use_goto);

    // math.e0.f0.0 (8) r8.acc2 r6.noacc r4.noacc 0xe {Align16, Q1/Q2}
    t8DstOpnd0->setAccRegSel(ACC2);
    t6SrcOpnd0->setAccRegSel(NOACC);
    t4SrcOpnd0->setAccRegSel(NOACC);
    inst = createMathInst(nullptr, g4::NOSAT, exsize, t8DstOpnd0, t6SrcOpnd0,
                          t4SrcOpnd0, MATH_INVM, madmInstOpt, true);
    G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
    inst->setCondMod(condModOverflow);

    G4_Label *gotoUIP = nullptr;
    if (generateIf) {
      if (!use_goto) {
        // (-f0.1) if (8) k0__AUTO_GENERATED_IF_LABEL__0
        // k0__AUTO_GENERATED_ELSE_LABEL__1 {Q1/Q2}
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        inst = createIf(predicateFlagReg, exsize, instOpt);
      } else {
        gotoUIP = createLocalBlockLabel("macro");
        // visa goto (not gen goto inst) will jump on true!
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Plus, tmpFlag->getRegVar(), 0, predCtrlValue);
        G4_ExecSize gotoExSize(getSIMDSize(*this));
        inst = createCFInst(predicateFlagReg, G4_goto, gotoExSize, nullptr,
                            gotoUIP, InstOpt_NoOpt, true);
      }
    }

    // madm (8) r9.acc3 r2.noacc r6.noacc r8.acc2 {Align16, Q1/Q2}
    G4_SrcRegRegion *t2SrcOpnd = createSrcRegRegion(tsrc2);
    t9DstOpnd0->setAccRegSel(ACC3);
    t2SrcOpnd->setAccRegSel(NOACC);
    t6SrcOpnd1->setAccRegSel(NOACC);
    t8SrcOpnd0x0->setAccRegSel(ACC2);
    inst = createMadm(predicateFlagReg_m1, exsize, t9DstOpnd0, t2SrcOpnd,
                      t6SrcOpnd1, t8SrcOpnd0x0, madmInstOpt);

    // madm (8) r10.acc4 r5.noacc -r4.noacc r8.acc2 {Align16, Q1/Q2}
    G4_SrcRegRegion *t5SrcOpnd0 = createSrcRegRegion(tsrc5);
    t10DstOpnd0->setAccRegSel(ACC4);
    t5SrcOpnd0->setAccRegSel(NOACC);
    t4SrcOpndNeg0->setAccRegSel(NOACC);
    t8SrcOpnd0x1->setAccRegSel(ACC2);
    inst = createMadm(predicateFlagReg_m2, exsize, t10DstOpnd0, t5SrcOpnd0,
                      t4SrcOpndNeg0, t8SrcOpnd0x1, madmInstOpt);

    // madm (8) r1.acc5 r8.acc2 r10.acc4 r8.acc2 {Align16, Q1/Q2}
    t1DstOpnd0->setAccRegSel(ACC5);
    t8SrcOpnd0x2->setAccRegSel(ACC2);
    t10SrcOpnd0->setAccRegSel(ACC4);
    t8SrcOpnd0x3->setAccRegSel(ACC2);
    inst = createMadm(predicateFlagReg_m3, exsize, t1DstOpnd0, t8SrcOpnd0x2,
                      t10SrcOpnd0, t8SrcOpnd0x3, madmInstOpt);

    // madm (8) r11.acc6 r6.noacc -r4.noacc r9.acc3 {Align16, Q1/Q2}
    t11DstOpnd0->setAccRegSel(ACC6);
    t6SrcOpnd2->setAccRegSel(NOACC);
    t4SrcOpndNeg1->setAccRegSel(NOACC);
    t9SrcOpnd0x0->setAccRegSel(ACC3);
    inst = createMadm(predicateFlagReg_m4, exsize, t11DstOpnd0, t6SrcOpnd2,
                      t4SrcOpndNeg1, t9SrcOpnd0x0, madmInstOpt);

    // madm (8) r9.acc7 r9.acc3 r11.acc6 r1.acc5 {Align16, Q1/Q2}
    t9DstOpnd1->setAccRegSel(ACC7);
    t9SrcOpnd0x1->setAccRegSel(ACC3);
    t11SrcOpnd0->setAccRegSel(ACC6);
    t1SrcOpnd0->setAccRegSel(ACC5);
    inst = createMadm(predicateFlagReg_m5, exsize, t9DstOpnd1, t9SrcOpnd0x1,
                      t11SrcOpnd0, t1SrcOpnd0, madmInstOpt);

    // madm (8) r6.acc8 r6.noacc -r4.noacc r9.acc7 {Align16, Q1/Q2}
    t6DstOpnd0->setAccRegSel(ACC8);
    t6SrcOpnd3->setAccRegSel(NOACC);
    t4SrcOpndNeg2->setAccRegSel(NOACC);
    t9SrcOpnd1x0->setAccRegSel(ACC7);
    inst = createMadm(predicateFlagReg_m6, exsize, t6DstOpnd0, t6SrcOpnd3,
                      t4SrcOpndNeg2, t9SrcOpnd1x0, madmInstOpt);

    // Restore rounding mode in CR0.0:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, !hasDefaultRoundDenorm, tmpCR0ForRoundRestore);

    // madm (8) r8.noacc r9.acc7 r6.acc8 r1.acc5 {Align16, Q1/Q2}
    t8DstOpnd1->setAccRegSel(NOACC);
    t9SrcOpnd1x1->setAccRegSel(ACC7);
    t6SrcOpnd4->setAccRegSel(ACC8);
    t1SrcOpnd1->setAccRegSel(ACC5);
    inst = createMadm(predicateFlagReg_m7, exsize, t8DstOpnd1, t9SrcOpnd1x1,
                      t6SrcOpnd4, t1SrcOpnd1, madmInstOpt);

    if (generateIf) {
      if (!use_goto) {
        // endif (8) {Q1/Q2}
        inst = createEndif(exsize, instOpt);
      } else {
       vASSERT(gotoUIP);
        inst = createLabelInst(gotoUIP, true);
      }
    }

    // Restore denorm and rounding in CR0.0 after whole sequence:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_D_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, hasDefaultRoundDenorm, false,
                 tmpCR0ForRoundRestore, tmpCR0ForRoundDenormRestore);
  };

  // make final copy to dst
  t8_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
  if (hasDefaultRoundDenorm) {
    // mov (instExecSize) r86.0<1>:f r8.0<8;8,1>:f
    inst = createInst(predOpnd, G4_mov, condMod, saturate, instExecSize,
                      dstOpnd, t8_src_opnd_final, nullptr,
                      Get_Gen4_Emask(emask, instExecSize), true);
  } else {
    // If hasDefaultRoundDenorm is false, denorm mode may be flush to zero.
    // When denorm flush-to-zero is set, mov instructions with the same source
    // and destination data type may retain denorm as output. So, we need to
    // use mul instruction instead.
    // mul (instExecSize) r86.0<1>:f r8.0<8;8,1>:f 1.0:f
    inst = createInst(predOpnd, G4_mul, condMod, saturate, instExecSize,
                      dstOpnd, t8_src_opnd_final, createImm(float(1.0)),
                      Get_Gen4_Emask(emask, instExecSize), true);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticSingleSQRTIEEEInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_INST *inst;
  unsigned madmInstOpt = 0;
  G4_ExecSize instExecSize = toExecSize(executionSize);
  G4_ExecSize element_size =
      g4::SIMD8; // element_size is dynamic, changed according to instExecSize
  const G4_ExecSize exsize = getNativeExecSize();
  unsigned int loopCount = 1;
  G4_InstOpts instOpt = Get_Gen4_Emask(
      emask, exsize); // for those insts of execution size of element_size
  const RegionDesc *srcRegionDesc = getRegionStride1();

  G4_Imm *flt_constant_0 = createImm(float(0.0));
  G4_Imm *flt_constant_05 = createImm(float(0.5));
  if (instExecSize <= exsize) {
    element_size = exsize;
    loopCount = 1;
  } else {
    element_size = instExecSize;
    instOpt = Get_Gen4_Emask(emask, instExecSize);
    loopCount = instExecSize / exsize;
  }

  bool generateIf = (this->getuint32Option(vISA_PredicatedFdivSqrt) != 1);
  if (isNoMask(emask)) {
    generateIf = false;
  }
  const bool use_goto = (!useSCF(*this));

  // pred and conModifier
  uint32_t flagSize = instExecSize + getvISAMaskOffset(emask);
  if (generateIf && use_goto) {
    // goto requires flag to match simdsize.
    flagSize = getSIMDSize(*this);
  }
  G4_Declare *tmpFlag = createTempFlag(flagSize > 16 ? 2 : 1);
  G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

  // temp registers
  G4_Declare *t6 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t7 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t9 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_F, Any);
  G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_F, Any);

  // 0.0:f and 0.5:f constants
  G4_Declare *t0 = getImmDcl(flt_constant_0, exsize);
  G4_Declare *t8 = getImmDcl(flt_constant_05, exsize);

  createPseudoKills({t6, t7, t9, t10, t11, tmpFlag}, PseudoKillType::Src);

  G4_SrcRegRegion *src0RR =
      operandToDirectSrcRegRegion(*this, src0Opnd, element_size, instExecSize);

  if (src0RR->isScalar() || src0RR->getModifier() != Mod_src_undef ||
      !tryToAlignOperand(src0RR, getGRFSize())) {
    // expand src0 to vector src
    G4_DstRegRegion *t6_dst_src0_opnd = createDstRegRegion(t6, 1);
    inst = createMov(element_size, t6_dst_src0_opnd, src0RR, instOpt,
                     true); // mov (element_size) t6, src0RR {Q1/H1}

    src0RR = createSrcRegRegion(t6, getRegionStride1());
  }

  G4_SrcRegRegion tsrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0,
                        srcRegionDesc, Type_F);
  G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(), 0, 0,
                        srcRegionDesc, Type_F);

  G4_SrcRegRegion tsrc7_final(*this, Mod_src_undef, Direct, t7->getRegVar(), 0,
                              0, getRegionStride1(), t7->getElemType());
  G4_SrcRegRegion *t7_src_opnd_final = createSrcRegRegion(tsrc7_final);

  bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
  // Temp variable of CR0 register for both rounding and SP/DP denorm restore
  G4_Declare *tmpCR0ForRoundDenormRestore =
      createTempVarWithNoSpill(1, Type_UD, Any);
  // Temp variable of CR0 register for rounding restore only
  G4_Declare *tmpCR0ForRoundRestore = createTempVarWithNoSpill(1, Type_UD, Any);

  VISA_EMask_Ctrl currEMask = emask;
  uint16_t splitInstGRFSize =
      (uint16_t)((TypeSize(Type_F) * exsize + getGRFSize() - 1) / getGRFSize());
  for (uint16_t loopIndex = 0;
       currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
       ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize)) {
    G4_Predicate *predicateFlagReg_m1 = NULL;
    G4_Predicate *predicateFlagReg_m2 = NULL;
    G4_Predicate *predicateFlagReg_m3 = NULL;
    G4_Predicate *predicateFlagReg_m4 = NULL;
    G4_Predicate *predicateFlagReg_m5 = NULL;
    G4_Predicate *predicateFlagReg_m6 = NULL;
    G4_Predicate *predicateFlagReg_m7 = NULL;
    if (!generateIf) {
      predicateFlagReg_m1 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m2 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m3 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m4 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m5 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m6 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
      predicateFlagReg_m7 = createPredicate(
          PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
    }
    uint16_t regIndex = splitInstGRFSize * loopIndex;
    instOpt = Get_Gen4_Emask(currEMask, exsize);
    instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0;
    madmInstOpt = instOpt;

    // 7, 9, 10, 11
    G4_DstRegRegion tdst7(*this, Direct, t7->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1,
                          Type_F);
    G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1,
                           Type_F);
    G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1,
                           Type_F);

    G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct, t7->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_F);

    G4_DstRegRegion *t7DstOpnd0 = createDstRegRegion(tdst7);
    G4_DstRegRegion *t7DstOpnd1 = createDstRegRegion(tdst7);
    G4_DstRegRegion *t7DstOpnd2 = createDstRegRegion(tdst7);
    G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
    G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
    G4_DstRegRegion *t10DstOpnd1 = createDstRegRegion(tdst10);
    G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);

    // src oprands passed by function calls
    G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(),
                          src0RR->getRegOff() + regIndex, 0, srcRegionDesc,
                          Type_F);

    G4_SrcRegRegion *t6SrcOpnd0 = NULL;
    G4_SrcRegRegion *t6SrcOpnd1 = NULL;
    G4_SrcRegRegion *t6SrcOpnd2 = NULL;

    G4_SrcRegRegion *t7SrcOpnd0 = createSrcRegRegion(tsrc7);
    G4_SrcRegRegion *t7SrcOpnd1 = createSrcRegRegion(tsrc7);
    G4_SrcRegRegion *t7SrcOpnd2x0 = createSrcRegRegion(tsrc7);
    G4_SrcRegRegion *t7SrcOpnd2x1 = createSrcRegRegion(tsrc7);
    G4_SrcRegRegion *t7SrcOpnd3 = createSrcRegRegion(tsrc7);

    G4_SrcRegRegion *t9SrcOpnd0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
    G4_SrcRegRegion *t9SrcOpnd2 = createSrcRegRegion(tsrc9);

    G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
    G4_SrcRegRegion *t10SrcOpnd1 = createSrcRegRegion(tsrc10);
    G4_SrcRegRegion *t10SrcOpnd2 = createSrcRegRegion(tsrc10);

    G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);
    G4_SrcRegRegion *t11SrcOpnd1x0 = createSrcRegRegion(tsrc11);
    G4_SrcRegRegion *t11SrcOpnd1x1 = createSrcRegRegion(tsrc11);

    setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, false,
                          tmpCR0ForRoundDenormRestore, tmpCR0ForRoundRestore);

    t6SrcOpnd0 = createSrcRegRegion(fsrc0);
    t6SrcOpnd1 = createSrcRegRegion(fsrc0);
    t6SrcOpnd2 = createSrcRegRegion(fsrc0);

    // math.eo.f0.0 (8) r7.acc2 r6.noacc null 0xF {Aligned16, Q1/Q2}
    t7DstOpnd0->setAccRegSel(ACC2);
    t6SrcOpnd0->setAccRegSel(NOACC);

    // see comments regarding goto/join in translateVISAArithmeticDoubleInst().
    initEOFlagIfNeeded(*this, tmpFlag, generateIf && use_goto);

    G4_SrcRegRegion *null_src_opnd = createNullSrc(Type_F);
    inst = createMathInst(NULL, g4::NOSAT, exsize, t7DstOpnd0, t6SrcOpnd0,
                          null_src_opnd, MATH_RSQRTM, madmInstOpt, true);
    G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
    inst->setCondMod(condModOverflow);

    G4_Label *gotoUIP = nullptr;
    if (generateIf) {
      if (!use_goto) {
        // (-f0.1) if (8) k0__AUTO_GENERATED_IF_LABEL__0
        // k0__AUTO_GENERATED_ELSE_LABEL__1 {Q1/Q2}
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
        inst = createIf(predicateFlagReg, exsize, instOpt);
      } else {
        gotoUIP = createLocalBlockLabel("macro");
        // visa goto (not gen goto inst) will jump on true!
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Plus, tmpFlag->getRegVar(), 0, predCtrlValue);
        G4_ExecSize gotoExSize(getSIMDSize(*this));
        inst = createCFInst(predicateFlagReg, G4_goto, gotoExSize, nullptr,
                            gotoUIP, InstOpt_NoOpt, true);
      }
    }

    // madm (8) r9.acc3 r0.noacc r8.noacc r7.acc2 {Aligned16, Q1/Q2}
    G4_SrcRegRegion *t0SrcOpnd0 = createSrcRegRegion(tsrc0);
    G4_SrcRegRegion *t8SrcOpnd0 = createSrcRegRegion(tsrc8);
    t9DstOpnd0->setAccRegSel(ACC3);
    t0SrcOpnd0->setAccRegSel(NOACC);
    t8SrcOpnd0->setAccRegSel(NOACC);
    t7SrcOpnd0->setAccRegSel(ACC2);
    inst = createMadm(predicateFlagReg_m1, exsize, t9DstOpnd0, t0SrcOpnd0,
                      t8SrcOpnd0, t7SrcOpnd0, madmInstOpt);

    // madm (8) r11.acc4 r0.noacc r6.noacc r7.acc2 {Aligned16, Q1/Q2}
    G4_SrcRegRegion *t0SrcOpnd1 = createSrcRegRegion(tsrc0);
    t11DstOpnd0->setAccRegSel(ACC4);
    t0SrcOpnd1->setAccRegSel(NOACC);
    t6SrcOpnd1->setAccRegSel(NOACC);
    t7SrcOpnd1->setAccRegSel(ACC2);
    inst = createMadm(predicateFlagReg_m2, exsize, t11DstOpnd0, t0SrcOpnd1,
                      t6SrcOpnd1, t7SrcOpnd1, madmInstOpt);

    // create -r11.noacc
    G4_SrcRegRegion tsrc11_neg(
        *this, Mod_Minus, t11SrcOpnd0->getRegAccess(), t11SrcOpnd0->getBase(),
        t11SrcOpnd0->getRegOff(), t11SrcOpnd0->getSubRegOff(),
        t11SrcOpnd0->getRegion(), t11SrcOpnd0->getType());
    G4_SrcRegRegion *t11SrcOpndNeg0 = createSrcRegRegion(tsrc11_neg);
    // madm (8) r10.acc5 r8.noacc -r11.acc4 r9.acc3 {Aligned16, Q1/Q2}
    G4_SrcRegRegion *t8SrcOpnd1 = createSrcRegRegion(tsrc8);
    t10DstOpnd0->setAccRegSel(ACC5);
    t8SrcOpnd1->setAccRegSel(NOACC);
    t11SrcOpndNeg0->setAccRegSel(ACC4);
    t9SrcOpnd0->setAccRegSel(ACC3);
    inst = createMadm(predicateFlagReg_m3, exsize, t10DstOpnd0, t8SrcOpnd1,
                      t11SrcOpndNeg0, t9SrcOpnd0, madmInstOpt);

    // madm (8) r9.acc6 r9.acc3 r10.acc5 r9.acc3 {Aligned16, Q1/Q2}
    t9DstOpnd1->setAccRegSel(ACC6);
    t9SrcOpnd1x0->setAccRegSel(ACC3);
    t10SrcOpnd0->setAccRegSel(ACC5);
    t9SrcOpnd1x1->setAccRegSel(ACC3);
    inst = createMadm(predicateFlagReg_m4, exsize, t9DstOpnd1, t9SrcOpnd1x0,
                      t10SrcOpnd0, t9SrcOpnd1x1, madmInstOpt);

    // madm (8) r7.acc7 r11.acc4 r10.acc5 r11.acc4 {Aligned16, Q1/Q2}
    t7DstOpnd1->setAccRegSel(ACC7);
    t11SrcOpnd1x0->setAccRegSel(ACC4);
    t10SrcOpnd1->setAccRegSel(ACC5);
    t11SrcOpnd1x1->setAccRegSel(ACC4);
    inst = createMadm(predicateFlagReg_m5, exsize, t7DstOpnd1, t11SrcOpnd1x0,
                      t10SrcOpnd1, t11SrcOpnd1x1, madmInstOpt);

    // create -r7.noacc
    G4_SrcRegRegion tsrc7_neg(
        *this, Mod_Minus, t7SrcOpnd2x0->getRegAccess(), t7SrcOpnd2x0->getBase(),
        t7SrcOpnd2x0->getRegOff(), t7SrcOpnd2x0->getSubRegOff(),
        t7SrcOpnd2x0->getRegion(), t7SrcOpnd2x0->getType());
    G4_SrcRegRegion *t7SrcOpndNeg0 = createSrcRegRegion(tsrc7_neg);
    // madm (8) r10.acc8 r6.noacc -r7.acc7 r7.acc7 {Aligned16, Q1/Q2}
    t10DstOpnd1->setAccRegSel(ACC8);
    t6SrcOpnd2->setAccRegSel(NOACC);
    t7SrcOpndNeg0->setAccRegSel(ACC7);
    t7SrcOpnd2x1->setAccRegSel(ACC7);
    inst = createMadm(predicateFlagReg_m6, exsize, t10DstOpnd1, t6SrcOpnd2,
                      t7SrcOpndNeg0, t7SrcOpnd2x1, madmInstOpt);

    // Restore rounding mode in CR0.0:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, !hasDefaultRoundDenorm, tmpCR0ForRoundRestore);

    // madm (8) r7.noacc r7.acc7 r9.acc6 r10.acc8 {Aligned16, Q1/Q2}
    t7DstOpnd2->setAccRegSel(NOACC);
    t7SrcOpnd3->setAccRegSel(ACC7);
    t9SrcOpnd2->setAccRegSel(ACC6);
    t10SrcOpnd2->setAccRegSel(ACC8);
    inst = createMadm(predicateFlagReg_m7, exsize, t7DstOpnd2, t7SrcOpnd3,
                      t9SrcOpnd2, t10SrcOpnd2, madmInstOpt);

    if (generateIf) {
      if (!use_goto) {
        // endif (8) {Q1/Q2}
        inst = createEndif(exsize, instOpt);
      } else {
       vASSERT(gotoUIP);
        inst = createLabelInst(gotoUIP, true);
      }
    }

    // Restore denorm and rounding in CR0.0 after whole sequence:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_D_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, hasDefaultRoundDenorm, false,
                 tmpCR0ForRoundRestore, tmpCR0ForRoundDenormRestore);
  };

  // make final copy to dst
  t7_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
  if (hasDefaultRoundDenorm) {
    // mov (instExecSize) r86.0<1>:f r7.0<8;8,1>:f
    inst = createInst(predOpnd, G4_mov, condMod, saturate, instExecSize,
                      dstOpnd, t7_src_opnd_final, nullptr,
                      Get_Gen4_Emask(emask, instExecSize), true);
  } else {
    // If hasDefaultRoundDenorm is false, denorm mode may be flush to zero.
    // When denorm flush-to-zero is set, mov instructions with the same source
    // and destination data type may retain denorm as output. So, we need to
    // use mul instruction instead.
    // mul (instExecSize) r86.0<1>:f r7.0<8;8,1>:f 1.0:f
    inst = createInst(predOpnd, G4_mul, condMod, saturate, instExecSize,
                      dstOpnd, t7_src_opnd_final, createImm(float(1.0)),
                      Get_Gen4_Emask(emask, instExecSize), true);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticDoubleSQRTInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_INST *inst = nullptr;
  G4_ExecSize instExecSize = toExecSize(executionSize);
  G4_ExecSize exsize = (G4_ExecSize)(getNativeExecSize() / 2);

  const RegionDesc *srcRegionDesc = getRegionStride1();
  const RegionDesc *rdAlign16 = getRegionStride1();
  unsigned int loopCount = 1;
  G4_ExecSize element_size =
      exsize; // element_size is set according to instExecSize

  G4_DstRegRegion *dst0 = nullptr;
  G4_SrcRegRegion *src0 = nullptr;
  G4_SrcRegRegion *src1 = nullptr;
  G4_SrcRegRegion *src2 = nullptr;
  G4_SrcRegRegion *neg_src1 = nullptr;

  // If enableInterleaveMacro option is on, then do not split here. Later
  // InstSplitPass
  // On Xe2+ platforms if enableInterleaveMacro option is on, then do not split
  // here. Later InstSplitPass will do the splitting to generate interleaved
  // macro for FP64 SQRT.
  if (this->getOption(vISA_enableInterleaveMacro) && getPlatform() >= Xe2) {
    if (instExecSize > exsize) {
      exsize = instExecSize;
      element_size = instExecSize;
      loopCount = 1;
    }
  } else
  {
    if (instExecSize > exsize) {
      element_size = instExecSize;
      exsize = std::min(instExecSize, G4_ExecSize(getFP64MadmExecSize()));
      loopCount = instExecSize / exsize;
    }
  }

  bool noDstMove = exsize == 8 && !saturate && !predOpnd &&
                   tryToAlignOperand(dstOpnd, getGRFSize()) &&
                   dstOpnd->getRegAccess() == Direct &&
                   dstOpnd->getHorzStride() == 1 && instExecSize == exsize;
  if (noDstMove && dstOpnd->getTopDcl() == src0Opnd->getTopDcl()) {
    noDstMove = false;
  }

  unsigned int instOpt = Get_Gen4_Emask(emask, exsize);

  bool generateIf;
  bool doFastSqrt = (opcode == ISA_SQRT);
  switch (this->getuint32Option(vISA_PredicatedFdivSqrt)) {
  case 0: // force if-endif
    generateIf = true;
    break;
  case 1: // force predicated
    generateIf = false;
    break;
  case 2:  // visa selects
  default: // other value treated as 2 (default)
    // Using predicated for fast sqrt
    generateIf = !doFastSqrt;
    break;
  }

  if (isNoMask(emask)) {
    generateIf = false;
  }
  const bool use_goto = (!useSCF(*this));

  // pred and conModifier
  uint32_t flagSize = instExecSize + getvISAMaskOffset(emask);
  if (generateIf && use_goto) {
    // goto requires flag to match simdsize.
    flagSize = getSIMDSize(*this);
  }
  G4_Declare *flagReg = createTempFlag(flagSize > 16 ? 2 : 1);
  G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

  // temp registers
  G4_Declare *t0 = getImmDcl(createDFImm(0.0), exsize);
  G4_Declare *t1 = getImmDcl(createDFImm(1.0), exsize);
  G4_Declare *t2 = getImmDcl(createDFImm(0.5), exsize);
  G4_Declare *t3 = getImmDcl(createDFImm(1.5), exsize);
  G4_Declare *t6 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t7 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t8 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t9 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_DF, Any);
  G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_DF, Any);

  createPseudoKills({t6, t7, t8, t9, t10, t11, flagReg}, PseudoKillType::Src);

  G4_SrcRegRegion *src0RR =
      operandToDirectSrcRegRegion(*this, src0Opnd, element_size, instExecSize);

  bool IsSrc0Moved = src0RR->getRegion()->isScalar() ||
                     src0RR->getModifier() != Mod_src_undef ||
                     !tryToAlignOperand(src0RR, getGRFSize());
  if (IsSrc0Moved) {
    // expand scale src0 to vector src
    dst0 = createDstRegRegion(t6, 1);
    // mov (element_size) t6_dst_src0_opnd, src0RR {Q1/H1}
    inst = createMov(element_size, dst0, src0RR, instOpt, true);
  }

  // constants

  // r0 = 0.0:df, r1 = 1.0:df, r2(r8) = 0.5:df, r3 = 1.5:df
  // NOTE: 'NoMask' is required as constants are required for splitting
  // parts. Once they are in diverged branches, it won't be properly
  // initialized without 'NoMask'.

  // one GRF
  G4_SrcRegRegion csrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);
  G4_SrcRegRegion csrc1(*this, Mod_src_undef, Direct, t1->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);
  G4_SrcRegRegion csrc2(*this, Mod_src_undef, Direct, t2->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);
  G4_SrcRegRegion csrc3(*this, Mod_src_undef, Direct, t3->getRegVar(), 0, 0,
                        srcRegionDesc, Type_DF);

  bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);

  // Temp variable of CR0 register for both rounding and SP/DP denorm restore
  G4_Declare *tmpCR0ForRoundDenormRestore =
      createTempVarWithNoSpill(1, Type_UD, Any);
  // Temp variable of CR0 register for rounding restore only
  G4_Declare *tmpCR0ForRoundRestore = createTempVarWithNoSpill(1, Type_UD, Any);

  // each madm only handles 4 channel double data
  VISA_EMask_Ctrl currEMask = emask;
  uint16_t splitInstGRFSize =
      (uint16_t)((TypeSize(Type_DF) * exsize + getGRFSize() - 1) /
                 getGRFSize());
  for (uint16_t loopIndex = 0;
       currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
       ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize)) {
    uint16_t regIndex = splitInstGRFSize * loopIndex;
    instOpt = Get_Gen4_Emask(currEMask, exsize);
    instOpt |= isNoMask(emask) ? InstOpt_WriteEnable
                               : 0;     // setting channels for non-mad insts
    unsigned int madmInstOpt = instOpt; // setting channels for mad insts

    // dst : 7, 8, 9, 10 11
    G4_DstRegRegion tdst7(
        *this, Direct, noDstMove ? dstOpnd->getBase() : t7->getRegVar(),
        noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, 1, Type_DF);
    G4_DstRegRegion tdst8(*this, Direct, t8->getRegVar(), regIndex, 0, 1,
                          Type_DF);
    G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1,
                          Type_DF);
    G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1,
                           Type_DF);
    G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1,
                           Type_DF);

    // source of inst.
    G4_SrcRegRegion fsrc0_math(*this, Mod_src_undef, Direct, src0RR->getBase(),
                               src0RR->getRegOff() + regIndex, 0, rdAlign16,
                               Type_DF);

    // src : 6, 7, 8, 9, 10, 11
    G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(),
                          src0RR->getRegOff() + regIndex, 0, srcRegionDesc,
                          Type_DF);
    G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct,
                          noDstMove ? dstOpnd->getBase() : t7->getRegVar(),
                          noDstMove ? dstOpnd->getRegOff() + regIndex
                                    : regIndex,
                          0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(),
                          regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(),
                           regIndex, 0, srcRegionDesc, Type_DF);

    setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, doFastSqrt,
                          tmpCR0ForRoundDenormRestore, tmpCR0ForRoundRestore);

    // see comments regarding goto/join in translateVISAArithmeticDoubleInst().
    initEOFlagIfNeeded(*this, flagReg, generateIf && use_goto);

    // math.e0.f0.0 (4) r7.acc2 r6.noacc NULL 0xf {Align16, N1/N2}
    dst0 = createDstRegRegion(tdst7);
    dst0->setAccRegSel(ACC2);
    if (IsSrc0Moved) {
      // use t6, but need to adjust the offset since unlike the orig source t6
      // is zero-based.
      src0 = createSrc(t6->getRegVar(), regIndex, 0, rdAlign16, Type_DF);
    } else {
      src0 = createSrcRegRegion(fsrc0_math);
    }
    src0->setAccRegSel(NOACC);
    src1 = createNullSrc(Type_DF);
    G4_CondMod *condModOverflow = createCondMod(Mod_o, flagReg->getRegVar(), 0);
    inst = createMathInst(NULL, g4::NOSAT, exsize, dst0, src0, src1,
                          MATH_RSQRTM, madmInstOpt, true);
    inst->setCondMod(condModOverflow);

    G4_Label *gotoUIP = nullptr;
    if (generateIf) {
      if (!use_goto) {
        // if
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        inst = createIf(predicateFlagReg, exsize, instOpt);
      } else {
        gotoUIP = createLocalBlockLabel("macro");
        // visa goto (not gen goto inst) will jump on true!
        G4_Predicate *predicateFlagReg = createPredicate(
            PredState_Plus, flagReg->getRegVar(), 0, predCtrlValue);
        G4_ExecSize gotoExSize(getSIMDSize(*this));
        inst = createCFInst(predicateFlagReg, G4_goto, gotoExSize, nullptr,
                            gotoUIP, InstOpt_NoOpt, true);
      }
    }

    if (doFastSqrt) {

      G4_Predicate *predicateFlagReg_m1 = NULL;
      G4_Predicate *predicateFlagReg_m2 = NULL;
      G4_Predicate *predicateFlagReg_m3 = NULL;
      G4_Predicate *predicateFlagReg_m4 = NULL;
      G4_Predicate *predicateFlagReg_m5 = NULL;
      G4_Predicate *predicateFlagReg_m6 = NULL;

      if (!generateIf) {
        predicateFlagReg_m1 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m2 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m3 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m4 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m5 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m6 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
      }

      // S0 = a * y;
      //  madm (4) r11.acc4 r0.noacc r6.noacc r7.acc2 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst11);
      dst0->setAccRegSel(ACC4);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      if (IsSrc0Moved) {
        src1 = createSrcRegRegion(tsrc6);
      } else {
        src1 = createSrcRegRegion(fsrc0);
      }
      src1->setAccRegSel(NOACC);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m1, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // -0.5*y
      // H0 = -0.5*y;
      // madm (4) r9.acc3 r0.noacc r2(r8).noacc r7.acc2 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst9);
      dst0->setAccRegSel(ACC3);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(csrc2);
      src1->setAccRegSel(NOACC);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC2);
      G4_SrcRegRegion neg_srcRegion(*this, Mod_Minus, src1->getRegAccess(),
                                    src1->getBase(), src1->getRegOff(),
                                    src1->getSubRegOff(), src1->getRegion(),
                                    src1->getType());
      neg_src1 = createSrcRegRegion(neg_srcRegion);
      neg_src1->setAccRegSel(src1->getAccRegSel());
      inst = createMadm(predicateFlagReg_m2, exsize, dst0, src0, neg_src1, src2,
                        madmInstOpt);

      // relative error; use double precision FMA
      // eps = DP_FMA(H0, S0, 0.5);
      // eps = 0.5 + H0*S0
      // madm (4) r10.acc5 r2(r8).noacc -r11.acc4 r9.acc3 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst10);
      dst0->setAccRegSel(ACC5);
      src0 = createSrcRegRegion(csrc2);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc11);
      src1->setAccRegSel(ACC4);
      src2 = createSrcRegRegion(tsrc9);
      src2->setAccRegSel(ACC3);
      inst = createMadm(predicateFlagReg_m3, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // refine approximation to ~46 bits
      // S1 = DP_FMA(S0, eps, S0);
      // madm (4) r7.acc8 r11.acc4 r10.acc5 r11.acc4 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst7);
      dst0->setAccRegSel(ACC8);
      src0 = createSrcRegRegion(tsrc11);
      src0->setAccRegSel(ACC4);
      src1 = createSrcRegRegion(tsrc10);
      src1->setAccRegSel(ACC5);
      src2 = createSrcRegRegion(tsrc11);
      src2->setAccRegSel(ACC4);
      inst = createMadm(predicateFlagReg_m4, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // error term:  S1^2 - a
      // D = DP_FMA(S1, S1, -a);
      // madm (4) r8.acc7 -r6.noacc r7.acc8 r7.acc8 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst8);
      dst0->setAccRegSel(ACC7);
      if (IsSrc0Moved) {
        src0 = createSrcRegRegion(tsrc6);
      } else {
        src0 = createSrcRegRegion(fsrc0);
      }
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc7);
      src1->setAccRegSel(ACC8);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC8);
      G4_SrcRegRegion neg_srcRegion0(*this, Mod_Minus, src0->getRegAccess(),
                                     src0->getBase(), src0->getRegOff(),
                                     src0->getSubRegOff(), src0->getRegion(),
                                     src0->getType());

      G4_SrcRegRegion *neg_src0 = createSrcRegRegion(neg_srcRegion0);
      neg_src0->setAccRegSel(src0->getAccRegSel());
      inst = createMadm(predicateFlagReg_m5, exsize, dst0, neg_src0, src1, src2,
                        madmInstOpt);

      // final result:  S1 + D*H0
      // This result is computed and stored to regular double precision format
      // (nomme) y = DP_FMA(D, H0, S1); madm (4) r7.noacc r7.acc8 r9.acc3
      // r8.acc7 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst7);
      dst0->setAccRegSel(NOACC);
      src0 = createSrcRegRegion(tsrc7);
      src0->setAccRegSel(ACC8);
      src1 = createSrcRegRegion(tsrc9);
      src1->setAccRegSel(ACC3);
      src2 = createSrcRegRegion(tsrc8);
      src2->setAccRegSel(ACC7);
      inst = createMadm(predicateFlagReg_m6, exsize, dst0, src0, src1, src2,
                        madmInstOpt);
    } else {
      G4_Predicate *predicateFlagReg_m1 = NULL;
      G4_Predicate *predicateFlagReg_m2 = NULL;
      G4_Predicate *predicateFlagReg_m3 = NULL;
      G4_Predicate *predicateFlagReg_m4 = NULL;
      G4_Predicate *predicateFlagReg_m5 = NULL;
      G4_Predicate *predicateFlagReg_m6 = NULL;
      G4_Predicate *predicateFlagReg_m7 = NULL;
      G4_Predicate *predicateFlagReg_m8 = NULL;
      G4_Predicate *predicateFlagReg_m9 = NULL;
      G4_Predicate *predicateFlagReg_m10 = NULL;
      if (!generateIf) {
        predicateFlagReg_m1 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m2 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m3 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m4 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m5 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m6 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m7 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m8 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m9 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
        predicateFlagReg_m10 = createPredicate(
            PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
      }

      // madm (4) r9.acc3 r0.noacc r2(r8).noacc r7.acc2 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst9);
      dst0->setAccRegSel(ACC3);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(csrc2);
      src1->setAccRegSel(NOACC);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m1, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r11.acc4 r0.noacc r6.noacc r7.acc2 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst11);
      dst0->setAccRegSel(ACC4);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      if (IsSrc0Moved) {
        src1 = createSrcRegRegion(tsrc6);
      } else {
        src1 = createSrcRegRegion(fsrc0);
      }
      src1->setAccRegSel(NOACC);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC2);
      inst = createMadm(predicateFlagReg_m2, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r10.acc5 r2(r8).noacc -r11.acc4 r9.acc3 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst10);
      dst0->setAccRegSel(ACC5);
      src0 = createSrcRegRegion(csrc2);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc11);
      src1->setAccRegSel(ACC4);
      src2 = createSrcRegRegion(tsrc9);
      src2->setAccRegSel(ACC3);
      G4_SrcRegRegion neg_srcRegion(*this, Mod_Minus, src1->getRegAccess(),
                                    src1->getBase(), src1->getRegOff(),
                                    src1->getSubRegOff(), src1->getRegion(),
                                    src1->getType());
      neg_src1 = createSrcRegRegion(neg_srcRegion);
      neg_src1->setAccRegSel(src1->getAccRegSel());
      inst = createMadm(predicateFlagReg_m3, exsize, dst0, src0, neg_src1, src2,
                        madmInstOpt);

      // madm (4) r8.acc7 r1.noacc r3.noacc r10.acc5 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst8);
      dst0->setAccRegSel(ACC7);
      src0 = createSrcRegRegion(csrc1);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(csrc3);
      src1->setAccRegSel(NOACC);
      src2 = createSrcRegRegion(tsrc10);
      src2->setAccRegSel(ACC5);
      inst = createMadm(predicateFlagReg_m4, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r7.acc8 r0.noacc r10.acc5 r11.acc4 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst7);
      dst0->setAccRegSel(ACC8);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc10);
      src1->setAccRegSel(ACC5);
      src2 = createSrcRegRegion(tsrc11);
      src2->setAccRegSel(ACC4);
      inst = createMadm(predicateFlagReg_m5, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r10.acc9 r0.noacc r10.acc5 r9.acc3 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst10);
      dst0->setAccRegSel(ACC9);
      src0 = createSrcRegRegion(csrc0);
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc10);
      src1->setAccRegSel(ACC5);
      src2 = createSrcRegRegion(tsrc9);
      src2->setAccRegSel(ACC3);
      inst = createMadm(predicateFlagReg_m6, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r7.acc8 r11.acc4 r8.acc7 r7.acc8 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst7);
      dst0->setAccRegSel(ACC8);
      src0 = createSrcRegRegion(tsrc11);
      src0->setAccRegSel(ACC4);
      src1 = createSrcRegRegion(tsrc8);
      src1->setAccRegSel(ACC7);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC8);
      inst = createMadm(predicateFlagReg_m7, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r8.acc7 r9.acc3 r8.acc7 r10.acc9 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst8);
      dst0->setAccRegSel(ACC7);
      src0 = createSrcRegRegion(tsrc9);
      src0->setAccRegSel(ACC3);
      src1 = createSrcRegRegion(tsrc8);
      src1->setAccRegSel(ACC7);
      src2 = createSrcRegRegion(tsrc10);
      src2->setAccRegSel(ACC9);
      inst = createMadm(predicateFlagReg_m8, exsize, dst0, src0, src1, src2,
                        madmInstOpt);

      // madm (4) r9.acc3 r6.noacc -r7.acc8 r7.acc8 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst9);
      dst0->setAccRegSel(ACC3);
      if (IsSrc0Moved) {
        src0 = createSrcRegRegion(tsrc6);
      } else {
        src0 = createSrcRegRegion(fsrc0);
      }
      src0->setAccRegSel(NOACC);
      src1 = createSrcRegRegion(tsrc7);
      src1->setAccRegSel(ACC8);
      src2 = createSrcRegRegion(tsrc7);
      src2->setAccRegSel(ACC8);
      G4_SrcRegRegion neg_srcRegion1(*this, Mod_Minus, src1->getRegAccess(),
                                     src1->getBase(), src1->getRegOff(),
                                     src1->getSubRegOff(), src1->getRegion(),
                                     src1->getType());
      neg_src1 = createSrcRegRegion(neg_srcRegion1);
      neg_src1->setAccRegSel(src1->getAccRegSel());
      inst = createMadm(predicateFlagReg_m9, exsize, dst0, src0, neg_src1, src2,
                        madmInstOpt);

      // Restore rounding mode in CR0.0:
      //   mov (1)  cr0.0<1>:ud  TEMP_R_RESTORE<1;1,0>:ud
      // Fast sqrt would be 1ULP under default rounding mode and 2ULP otherwise.
      // Avoid setting rounding mode as 1-2ULP is acceptable for fast sqrt.
      restoreCR0_0(*this, !hasDefaultRoundDenorm && !doFastSqrt, tmpCR0ForRoundRestore);

      // madm (4) r7.noacc r7.acc8 r9.acc3 r8.acc7 {Align16, N1/N2}
      dst0 = createDstRegRegion(tdst7);
      dst0->setAccRegSel(NOACC);
      src0 = createSrcRegRegion(tsrc7);
      src0->setAccRegSel(ACC8);
      src1 = createSrcRegRegion(tsrc9);
      src1->setAccRegSel(ACC3);
      src2 = createSrcRegRegion(tsrc8);
      src2->setAccRegSel(ACC7);
      inst = createMadm(predicateFlagReg_m10, exsize, dst0, src0, src1, src2,
                        madmInstOpt);
    }

    if (generateIf) {
      if (!use_goto) {
        // endif (8) {Q1/Q2}
        inst = createEndif(exsize, instOpt);
      } else {
       vASSERT(gotoUIP);
        inst = createLabelInst(gotoUIP, true);
      }
    }

    // Restore denorm and rounding in CR0.0 after whole sequence:
    //   mov (1)  cr0.0<1>:ud  TEMP_R_D_RESTORE<1;1,0>:ud
    restoreCR0_0(*this, hasDefaultRoundDenorm, doFastSqrt,
                 tmpCR0ForRoundRestore, tmpCR0ForRoundDenormRestore);
  };

  // make final copy to dst
  if (!noDstMove || !hasDefaultRoundDenorm) {
    G4_SrcRegRegion tsrc7_final(*this, Mod_src_undef, Direct,
                                noDstMove ? dstOpnd->getBase()
                                          : t7->getRegVar(),
                                noDstMove ? dstOpnd->getRegOff() : 0, 0,
                                getRegionStride1(), t7->getElemType());
    G4_SrcRegRegion *t7_src_opnd_final = createSrcRegRegion(tsrc7_final);
    t7_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
    if (hasDefaultRoundDenorm) {
      // mov (instExecSize) r20.0<1>:df r7.0<8;8,1>:df
      inst = createInst(predOpnd, G4_mov, condMod, saturate, instExecSize,
                        dstOpnd, t7_src_opnd_final, nullptr,
                        Get_Gen4_Emask(emask, instExecSize), true);
    } else {
      // If hasDefaultRoundDenorm is false, denorm mode may be flush to zero.
      // When denorm flush-to-zero is set, mov instructions with the same source
      // and destination data type may retain denorm as output. So, we need to
      // use mul instruction instead.
      // mul (instExecSize) r20.0<1>:df r7.0<8;8,1>:df 1.0:df
      inst = createInst(predOpnd, G4_mul, condMod, saturate, instExecSize,
                        dstOpnd, t7_src_opnd_final, createDFImm(1.0),
                        Get_Gen4_Emask(emask, instExecSize), true);
    }
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAInvmRsqtmInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_DstRegRegion *dstOpnd,
    VISA_PredVar *dstPred, G4_Operand *src0Opnd, G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);
  vISA_ASSERT(opcode == ISA_INVM || opcode == ISA_RSQTM,
              "cocode should either invm or rsqtm");
  G4_Type Ty = dstOpnd->getType();
  vISA_ASSERT(Ty == Type_F || Ty == Type_DF,
              "invm/rsqtm only supports F or DF");
  vISA_ASSERT(saturate == g4::NOSAT, "sat not supported for invm/rsqtm!");

  // Given
  //   (p) math.invm   dstOpnd  predDst  src0Opnd src1Opnd
  //   (p) math.rsqtm  dstOpnd  predDst  src0Opnd null
  //
  // it is tranlated to
  //   tdst = null
  //   noDstMove = !(dstOpnd has sat && p);
  //   if (!noDstMov)
  //      tdst = temp
  //
  //   src0RR = null
  //   if (src0Opnd needs mov)
  //     mov t0  src0RR
  //     src0RR = t0
  //
  //   src1RR = null
  //   if (src1Opnd && src1Opnd needs mov)
  //     mov t1  src1RR
  //     src1RR = t1
  //
  //   // note: may split into multiple math
  //   dst = tdst ? tdst : dstOpnd
  //   math.invm predDst(eo) dst src0RR src1RR
  //
  //   if (!noDstMov)
  //     (p) mov dstOpnd dst
  //
  G4_INST *inst;
  G4_ExecSize instExecSize = toExecSize(executionSize);

  // It seems math.invm/rsqtm can have any execsize
  G4_ExecSize exsize = getNativeExecSize();
  if (instExecSize == 32 && supportNativeSIMD32())
    exsize = instExecSize;
  else if (instExecSize < exsize)
    exsize = instExecSize;

  G4_InstOpts instOpt = Get_Gen4_Emask(emask, instExecSize);
  instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0;
  uint32_t loopCount;
  uint8_t element_size = instExecSize;
  if (instExecSize <= exsize)
    loopCount = 1;
  else
    loopCount = instExecSize / exsize;

  bool noDstMove =
      !predOpnd && tryToAlignOperand(dstOpnd, getGRFSize()) &&
      dstOpnd->getRegAccess() == Direct && dstOpnd->getHorzStride() == 1;
  if (noDstMove &&
      (dstOpnd->getTopDcl() == src0Opnd->getTopDcl() ||
       (src1Opnd && dstOpnd->getTopDcl() == src1Opnd->getTopDcl()))) {
    noDstMove = false;
  }

  G4_Declare *tdst = nullptr;
  if (!noDstMove) {
    tdst = createTempVarWithNoSpill(element_size, Ty, getGRFAlign());
    createPseudoKills({tdst}, PseudoKillType::Src);
  }

  G4_SrcRegRegion *src0RR = operandToDirectSrcRegRegion(
      *this, src0Opnd, G4_ExecSize(element_size), instExecSize);
  G4_SrcRegRegion *src1RR = nullptr;
  if (src1Opnd)
    src1RR = operandToDirectSrcRegRegion(
        *this, src1Opnd, G4_ExecSize(element_size), instExecSize);

  bool needsSrc0Move =
       (src0RR->getModifier() != Mod_src_undef ||
        (!src0RR->isScalar() && !tryToAlignOperand(src0RR, getGRFSize())));
  if (needsSrc0Move) {
    G4_Declare *tsrc0 =
        createTempVarWithNoSpill(element_size, Ty, getGRFAlign());
    createPseudoKills({tsrc0}, PseudoKillType::Src);

    G4_DstRegRegion *tSrc0 = createDst(tsrc0->getRegVar(), 0, 0, 1, Ty);
    (void)createMov(instExecSize, tSrc0, src0RR, instOpt, true);
    src0RR = createSrcRegRegion(tsrc0, element_size == 1 ? getRegionScalar()
                                                         : getRegionStride1());
  }
  bool needsSrc1Move =
      (src1RR &&
       (src1RR->getModifier() != Mod_src_undef ||
        (!src1RR->isScalar() && !tryToAlignOperand(src1RR, getGRFSize()))));
  if (needsSrc1Move) {
    G4_Declare *tsrc1 =
        createTempVarWithNoSpill(element_size, Ty, getGRFAlign());
    createPseudoKills({tsrc1}, PseudoKillType::Src);

    G4_DstRegRegion *tSrc1 = createDst(tsrc1->getRegVar(), 0, 0, 1, Ty);
    (void)createMov(instExecSize, tSrc1, src1RR, instOpt, true);
    src1RR = createSrcRegRegion(tsrc1, element_size == 1 ? getRegionScalar()
                                                         : getRegionStride1());
  }

  uint16_t splitInstGRFSize =
      (uint16_t)((TypeSize(Ty) * exsize + getGRFSize() - 1) / getGRFSize());
  VISA_EMask_Ctrl currEMask = emask;
  for (uint16_t ix = 0; currEMask != vISA_NUM_EMASK && ix < loopCount;
       ++ix, currEMask = Get_Next_EMask(currEMask, exsize)) {
    uint16_t regIndex = ix * splitInstGRFSize;
    G4_InstOpts mathInstOpt = Get_Gen4_Emask(currEMask, exsize);
    mathInstOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0;

    G4_DstRegRegion *D =
        createDst(tdst ? tdst->getRegVar() : dstOpnd->getBase(),
                  tdst ? regIndex : dstOpnd->getRegOff() + regIndex, 0, 1, Ty);
    G4_SrcRegRegion *S0 = createSrc(
        src0RR->getBase(),
        src0RR->getRegOff() + (src0RR->isScalar() ? 0 : regIndex),
        src0RR->getSubRegOff(),
        src0RR->isScalar() ? getRegionScalar() : getRegionStride1(), Ty);
    G4_SrcRegRegion *S1;
    if (src1RR) {
      S1 = createSrc(
          src1RR->getBase(),
          src1RR->getRegOff() + (src1RR->isScalar() ? 0 : regIndex),
          src1RR->getSubRegOff(),
          src1RR->isScalar() ? getRegionScalar() : getRegionStride1(), Ty);
    } else {
      S1 = createNullSrc(Ty);
    }

    // Only INVM/RSQTM without using acc are exposed.
    D->setAccRegSel(NOACC);
    S0->setAccRegSel(NOACC);
    if (src1RR)
      S1->setAccRegSel(NOACC);
    inst = createMathInst(NULL, g4::NOSAT, exsize, D, S0, S1,
                          opcode == ISA_INVM ? MATH_INVM : MATH_RSQRTM,
                          mathInstOpt, true);
    G4_CondMod *condModOverflow =
        createCondMod(Mod_o, dstPred->predVar.dcl->getRegVar(), 0);
    inst->setCondMod(condModOverflow);
  }

  if (!noDstMove) {
    G4_SrcRegRegion *tS = createSrc(
        tdst->getRegVar(), 0, 0,
        instExecSize == 1 ? getRegionScalar() : getRegionStride1(), Ty);
    (void)createInst(predOpnd, G4_mov, nullptr, g4::NOSAT, instExecSize, dstOpnd,
                     tS, nullptr, instOpt, true);
  }

  return VISA_SUCCESS;
}
