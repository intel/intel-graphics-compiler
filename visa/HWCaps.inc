/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
// HW Capabilities
// these functions are part of IR_Builder class and included in Build_IR.h

bool hasMixMode() const {
  if (getOption(vISA_ForceMixMode)) {
    return true;
  }
  if (getOption(vISA_DisableMixMode)) {
    return false;
  }

  return getPlatform() > GENX_BDW &&
         getPlatformGeneration() != PlatformGen::GEN11;
}

// True if it has bfloat16 mixed mode support.
bool hasBFMixMode() const { return hasDPAS(); }

bool forceSamplerHeader() const {
  return m_options->getOption(vISA_forceSamplerHeader) ||
         (getPlatform() < GENX_ICLLP &&
          m_options->getOption(vISA_enablePreemption));
}

bool samplerHeaderPreemptionWA() const {
  // vISA_enablePreemptionR0Only is used as preeemption for the special platform
  return (m_options->getOption(vISA_enablePreemption) ||
          m_options->getOption(vISA_enablePreemptionR0Only)) &&
         m_options->getOption(vISA_samplerHeaderWA);
}

bool needsNoPreemptR2ForSend() const {
  return getPlatformGeneration() == PlatformGen::GEN11;
}

bool noDDAllowedPlatform() const { return getPlatform() >= GENX_SKL; }

bool noSrcDepSetAllowedPlatform() const { return getPlatform() >= GENX_SKL; }

bool doPlane() const {
  return getPlatform() < GENX_ICLLP && !getOption(vISA_expandPlane);
}

bool favorFloatMov() const {
  return getPlatformGeneration() == PlatformGen::GEN11 || isXeLP();
}

bool noScalarJmp() const { return !getOption(vISA_EnableScalarJmp); }

bool hasAlign1Ternary() const {
  return getPlatform() >= GENX_CNL && getOption(vISA_doAlign1Ternary);
}

bool encodeUnitStrideTernary() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

bool hasMacMacl() const {
  return getPlatform() >= GENX_CNL && getPlatform() <= Xe3 &&
         !removedAccRestrictionsAsGRF();
}

bool hasCPS() const { return getPlatform() >= GENX_CNL; }

bool hasBindlessSampler() const { return getPlatform() >= GENX_CNL; }

bool noSrc2Regioning() const { return getPlatform() >= GENX_ICLLP; }

bool noSrc1Byte() const {
  return getOption(vISA_noSrc1Byte) || getPlatform() >= GENX_ICLLP;
}

bool noDFTypeMac() const {
  if (getPlatform() == Xe_PVC)
    return true;
  return false;
}

bool needsFenceCommitEnable() const {
  if (m_options->getTarget() == VISA_CM) {
    return getPlatform() >= GENX_SKL;
  } else {
    return getPlatform() >= GENX_CNL;
  }
}

bool hasIEEEDivSqrt() const { return getPlatform() < GENX_ICLLP; }

bool gotoJumpOnTrue() const { return getPlatform() >= GENX_CNL; }

bool needsToReserveR127() const { return getPlatform() < GENX_CNL; }

bool hasSLMFence() const { return getPlatform() >= GENX_ICLLP; }

bool GRFAlign() const { return getPlatform() < GENX_SKL; }

bool twoSourcesCollision() const { return getPlatform() < GENX_CNL; }

bool lowHighBundle() const {
  return getPlatformGeneration() <= PlatformGen::GEN11;
}

bool hasSendShootdown() const { return getPlatform() >= GENX_CNL; }

bool useNewR0Format() const { return getPlatform() >= GENX_ICLLP; }

int getPredMaskEnableBit() const {
  return getPlatform() < GENX_ICLLP ? 30 : 23;
}

int getBarrierIDMask() const {
  return getPlatform() < GENX_ICLLP ? 0x8F000000 : 0x7F000000;
}

uint32_t getMaxSendMessageLength() const {
  return getPlatform() < GENX_CNL ? 16 : 32;
}

bool hasPixelNullMask() const { return getPlatform() >= GENX_SKL; }

bool noSLMMsgHeader() const { return getPlatform() >= GENX_SKL; }

bool needsA32MsgHeader() const { return getPlatform() < GENX_SKL; }

int getThreadIDMask() const {
  return getPlatform() >= GENX_SKL ? 0x7FF : 0x1FF;
}

bool hasFloatAtomics() const { return getPlatform() >= GENX_SKL; }

bool hasBlockedSLMMessage() const { return false; }

bool hasHeaderlessMRTWrite() const { return getPlatform() >= GENX_ICLLP; }

bool hasDotProductInst() const { return getPlatform() < GENX_CNL; }

bool hasLRP() const { return getPlatform() < GENX_ICLLP; }

int getBarrierMask(bool enableBarrierInstCounterBits) const {
  if (getPlatform() < GENX_SKL) {
    // pre-SKL: and (8) H0.0:ud r0.2:ud 0x0F000000 (r0.2, bit 24-27)
    return enableBarrierInstCounterBits ? 0x0F00FE00 : 0x0F000000;
  } else if (getPlatform() < GENX_ICLLP) {
    // SKL+: and (8) H0.0:ud r0.2:ud 0x8F000000 (r0.2, bit24-27, bit31)
    return enableBarrierInstCounterBits ? 0x8F00FE00 : 0x8F000000;
  } else {
    // else: and (8) H0.0:ud r0.2:ud 0x7F000000 (r0.2, bit24-30)
    return enableBarrierInstCounterBits ? 0x7F00FF00 : 0x7F000000;
  }
}

bool canMadHaveAcc() const { return getPlatform() >= GENX_CNL; }

bool hasFdivPow() const { return getPlatformGeneration() < PlatformGen::XE; }

bool hasFdivPowWA() const { return getPlatform() < GENX_ICLLP; }

bool hasCondModForTernary() const {
  return getPlatformGeneration() < PlatformGen::XE;
}

bool hasSWSB() const { return getPlatformGeneration() >= PlatformGen::XE; }
bool hasSWSBCounter() const {
  return getPlatform() > Xe3;
}

bool hasPartialMixMode() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

// whether EOT sources need to be assigned r112-r127
bool hasEOTGRFBinding() const {
  return getPlatformGeneration() < PlatformGen::XE3;
}

bool hasSmov() const { return getPlatformGeneration() < PlatformGen::XE; }

bool doAccSub() const { return getPlatformGeneration() >= PlatformGen::GEN11; }

bool doFlagLVN() const {
  return getPlatform() >= Xe3P_Graphics;
}

bool shareMergeVarGlobal() const {
  return getPlatform() >= Xe3P_Graphics;
}

bool hasNFType() const {
  return getPlatform() >= GENX_ICLLP &&
         getPlatformGeneration() < PlatformGen::XE;
}

void getSSIDBits(uint32_t &width, uint32_t &start) const {
  if (getPlatform() < GENX_CNL) {
    width = 2;
    start = 12; //[12:13]
  } else if (getPlatform() == GENX_CNL) {
    width = 2;
    start = 8; //[8:9]
  } else {
    width = 3;
    start = 8; //[8:10]
  }
}

bool encodeAccRegSelAsAlign1() const { return getPlatform() >= GENX_ICLLP; }

bool fuseTypedWrites() const { return getOption(vISA_FuseTypedWrites); }

bool avoidWAWSubRegHazard() const {
  return getPlatformGeneration() < PlatformGen::XE &&
         getOption(vISA_WAWSubregHazardAvoidance);
}

bool newTernaryStride() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

// acc restrictions that are relaxed for Xe
// -- dp4a can take acc src/dst
// -- the same acc may be used in both src0 and src1 of a three-source inst
bool relaxedACCRestrictions2() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

bool fuseURBMessage() const { return isXeLP(); }

bool supportSrcModforMul() const {
  return getPlatformGeneration() < PlatformGen::XE;
}

bool doMultiAccSub() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

bool canMadHaveSrc0Acc() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

bool accDstforIndirectSrc() const {
  return getPlatformGeneration() < PlatformGen::XE;
}

bool restrictedACCRestrictions() const { return getPlatform() >= Xe_PVC; }

bool favorFpMad() const { return getPlatform() < GENX_CNL; }

bool avoidAccDstWithIndirectSource() const {
  return getPlatform() >= GENX_TGLLP;
}

bool hasRSForSpecificPlatform() const {
  if (getPlatform() == Xe_DG2) {
    return m_options->getOption(vISA_ManualEnableRSWA);
  }
  return true;
}

bool hasDPASFuseRSWA() const {
  return VISA_WA_CHECK(getPWaTable(), Wa_18023229625) ||
         getOption(vISA_DPASFuseRSWA);
}

bool hasSrc1ReadSuppressionIssue() const {
  return getPlatform() == Xe_PVC && hasRSForSpecificPlatform();
}

bool hasReadSuppression() const {
  return getPlatform() >= GENX_TGLLP && hasRSForSpecificPlatform();
}

bool hasEOTReadSuppressionIssue() const {
  if (getPlatform() == Xe_DG2 && hasRSForSpecificPlatform()) {
    return true;
  }
  return false;
}

bool hasScalarReadSuppression() const {
  return getPlatform() > GENX_TGLLP && hasRSForSpecificPlatform();
}

bool hasFPU0ReadSuppressionIssue() const {
  return getPlatform() == Xe_PVCXT && hasRSForSpecificPlatform();
}

bool hasWideMulMadOpsEnabled() const {
  return getPlatform() > Xe3 &&
         getOptions()->getOption(vISA_WideMulMadOpsEnable);
}

bool alignBindlessSampler() const {
  return getPlatformGeneration() == PlatformGen::GEN9;
}

bool noL3Flush() const { return getPlatform() == GENX_TGLLP; }

bool needResetA0forVxHA0() const { return getPlatform() >= GENX_ICLLP; }

unsigned getCoIssueUints() const {
#if 0
        auto GEN = getPlatformGeneration();
        return (GEN >= PlatformGen::XE) ? 1 : 2;
#else
  if (getOptions()->getOption(vISA_SendQueueSched)) {
    auto GEN = getPlatformGeneration();
    return (GEN >= PlatformGen::XE) ? 1 : 2;
  }
  return 2;
#endif
}

bool useMultiThreadLatency() const {
#if 0
        auto GEN = getPlatformGeneration();
        if (GEN >= PlatformGen::XE)
            return false;
#endif

  return getOptions()->getOption(vISA_useMultiThreadedLatencies);
}

bool useIGAEncoder() const {
  return getOption(vISA_IGAEncoder) ||
         getPlatformGeneration() >= PlatformGen::XE;
}

bool needReplaceIndirectCallWithJmpi() const {
  return getPlatform() <= GENX_ICLLP ||
         getOption(vISA_replaceIndirectCallWithJmpi);
}

bool needIPWA() const { return getPlatform() == Xe_XeHPSDV; }

bool canEncodeFullExtDesc() const { return getPlatform() >= GENX_TGLLP; }

bool needSwap64ImmLoHi() const { return getPlatform() >= GENX_TGLLP; }

bool has8ByteA64Gather() const { return getPlatform() != GENX_TGLLP; }

bool WaDisableSendSrcDstOverlap() const {
  return getOption(vISA_noSendSrcDstOverlap) ||
         (m_options->getTarget() == VISA_CM && getPlatform() >= GENX_SKL &&
          getPlatform() < GENX_TGLLP) || getPlatform() == GENX_ICLLP ||
          (getPlatform() >= Xe_MTL && getPlatform() <= Xe_PVCXT);
}

bool isXeLP() const { return getPlatform() == GENX_TGLLP; }

bool hasEarlyGRFRead() const {
  return (getPlatform() == GENX_TGLLP) && getOption(vISA_HasEarlyGRFRead);
}

bool noInt64() const {
  return getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2 ||
         getPlatform() == Xe_MTL || getPlatform() == Xe_ARL;
}

bool noFP64() const {
  return getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2;
}

bool hasQDDMul() const {
  return getPlatform() == GENX_BDW || getPlatform() == GENX_SKL ||
         getPlatform() == GENX_CNL || getPlatform() == Xe_XeHPSDV ||
         getPlatform() == Xe_PVC || getPlatform() > Xe3;
}

bool no64bitRegioning() const {
  return getPlatform() == GENX_CHV || getPlatform() == GENX_BXT ||
         getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2 ||
         getPlatform() == Xe_MTL || getPlatform() == Xe_ARL;
}

bool hasBankCollision() const {
  if (getPlatform() == GENX_TGLLP) {
    return true;
  }
  return (getPlatformGeneration() <= PlatformGen::GEN11) ||
         (getPlatform() >= Xe_PVC) || getOption(vISA_forceBCR) ||
         (getOption(vISA_enableBCR) && !hasEarlyGRFRead());
}

bool oneGRFBankDivision() const {
  return getPlatform() >= Xe2 ||
         (getPlatform() != Xe_XeHPSDV && getPlatform() != Xe_DG2 &&
          getPlatform() != Xe_MTL && getPlatform() != Xe_ARL &&
          !(getPlatform() == Xe_PVC && !getOption(vISA_HasPartialInt64)));
}

bool hasMadm() const {
  return (getPlatform() != GENX_ICLLP && !isXeLP() && getPlatform() != Xe_DG2 &&
          getPlatform() != Xe_MTL && getPlatform() != Xe_ARL);
}

bool hasSIMD16TypedRW() const { return getPlatform() >= Xe_XeHPSDV; }

bool hasRegDistDepIssue() const {
  return getPlatform() == Xe_PVCXT;
}

bool doNotRewriteContiguousRegion() const {
  return getPlatform() >= Xe_XeHPSDV;
}

// acc restrictions that are relaxed for Xe HP
// -- mov can have both acc src and dst simultaneously
// -- acc0 and acc1 may both be used simultaneously in one inst
// -- acc can be packed HF dst of a mix mode inst
bool relaxedACCRestrictions() const { return getPlatform() >= Xe_XeHPSDV; }

bool relaxedACCRestrictions_1() const { return getPlatform() >= Xe_DG2; }

bool loadThreadPayload() const { return getPlatform() >= Xe_XeHPSDV; }

bool needFenceBeforeEOT() const { return getPlatform() == Xe_XeHPSDV; }

bool hasSrc0ReadSuppression() const {
  return getPlatform() >= Xe_XeHPSDV && hasRSForSpecificPlatform();
}

bool hasRMWReadSuppressionIssue() const {
  return (getPlatform() == Xe_ARL || getPlatform() == Xe_DG2 ||
          getPlatform() == Xe_XeHPSDV) &&
         hasRSForSpecificPlatform();
}

bool needToClearScratchWrites() const { return getPlatform() < Xe_XeHPSDV; }

bool needsToLoadLocalID() const {
  return loadThreadPayload() && getPerThreadInputSize() > 0 &&
         !getOption(vISA_autoLoadLocalID);
}

bool noDwDstForDwordMul() const {
  return (noInt64() || getPlatform() >= Xe_XeHPSDV) && getPlatform() <= Xe3;
}

bool useNewExtDescFormat() const { return getPlatform() >= Xe_XeHPSDV; }

bool has16OWordSLMBlockRW() const { return getPlatform() >= Xe_XeHPSDV; }

bool hasVxHFloat64b() const { return getPlatform() < Xe_XeHPSDV; }

bool supportFloatOr64bRegioning() const {
  return getPlatform() < Xe_XeHPSDV && !getOption(vISA_forceNoFP64bRegioning);
}

int getFP64MadmExecSize() const {
  auto plat = getPlatform();
  if (plat >= Xe_PVC) {
    return 16;
  } else if (plat == Xe_XeHPSDV || plat == Xe_DG2 || plat == Xe_MTL ||
             plat == Xe_ARL) {
    return 8;
  } else {
    return 4;
  }
}

bool balanceIntFloatMoves() const { return getPlatform() >= Xe_XeHPSDV; }

G4_Type getMixModeType() const {
  return getPlatform() >= Xe_XeHPSDV ? Type_BF : Type_HF;
}

// each flag register is 16-bit
uint32_t getNumFlagRegisters(void) { return getPlatform() >= Xe_PVC ? 8 : 4; }

uint32_t getNumAddrRegisters() const {
    return supportNativeSIMD32() ? 32 : 16;
}

// each address register is 16 bits
uint32_t getNumAddrRegistersInGRFSizeSWSB() const {
  if (hasThreeALUPipes() || hasFourALUPipes()) {
    return ((supportNativeSIMD32() ? 32 : 16) * G4_WSIZE +
            numEltPerGRF<Type_UB>() - 1) /
           numEltPerGRF<Type_UB>();
  } else {
    return 0;
  }
}

uint32_t getNumScalarRegisters(void) {
  if (auto n = getuint32Option(vISA_ScalarPipe)) {
    return n;
  }
  if (enableSendIndirect() || isEfficient64bEnabled()) {
    return 1;
  }
  return 0;
}

uint32_t getScalarRegisterSizeInBytes() {
  if (getuint32Option(vISA_ScalarPipe)) {
    return 64; // 64 Bytes in s0
  }
  return getPlatform () > Xe3 ? 64 : 32; // 64B in s0
}

bool encodeAccWrEn() const { return getPlatform() < Xe_PVC; }

SFID getEOTSFID() const {
  return getPlatform() >= Xe_DG2 ? SFID::GATEWAY : SFID::SPAWNER;
}

// EU native execution size for 32-bit types
G4_ExecSize getNativeExecSize() const {
  return getPlatform() >= Xe_PVC ? g4::SIMD16 : g4::SIMD8;
}

bool noScalarByteToFloat() const { return getPlatform() >= Xe_XeHPSDV; }

bool noHFToInteger() const {
  return getPlatform() == Xe_PVCXT;
}

bool useAccForDF() const {
  return getPlatform() > Xe3 ||
         getPlatform() == Xe_PVCXT || getPlatform() == Xe_PVC ||
         getPlatform() == Xe_XeHPSDV;
}

bool hasAccExecSizeRestrictions() const {
  return !removedAccRestrictionsAsGRF();
}

bool useAccForMadm() const { return getPlatform() >= Xe_XeHPSDV; }

bool hasUnifiedBarrier() const { return getPlatform() >= Xe_DG2; }

bool predCtrlHasWidth() const { return getPlatform() < Xe_PVC; }

bool hasNibCtrl() const { return getPlatform() < Xe_PVC; }

bool hasMaskForScratchMsg() const { return getPlatform() < Xe_PVC; }

bool avoidDstSrcOverlap() {
  return getPlatform() >= GENX_ICLLP && getOption(vISA_DstSrcOverlapWA);
}

bool avoidSrc1Src2Overlap() { return getOption(vISA_Src1Src2OverlapWA); }

bool hasTwoGRFMathMacro() const { return getPlatform() >= Xe_PVC; }

// to relax ACC usage restrictions for FP MUL and MAD instructions.
// 1. Support both sources as ACC for FP MUL
// 2. Support Src2 as ACC for FP MAD
bool relaxedACCRestrictions3() const {
  return ((getPlatform() == Xe_ARL || getPlatform() >= Xe2) &&
          !getOption(vISA_disableSrc2AccSub));
}

bool relaxedACCRestrictions4() const {
  return getPlatform() >= Xe2;
}

bool hasACCRestrictionsForCdyn() const {
  return getPlatform() >= Xe3P_Graphics;
}

bool enableACCBeforRA() const {
  return (getOption(vISA_accSubBeforeRA) &&
          (getPlatform() >= Xe2 && getPlatform() <= Xe3 ||
           getPlatform() >= Xe3P_Graphics)) ||
         getOption(vISA_enableAccSubBeforeRA);
}

bool enablePreSchedACC() const {
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D) {
    // Do not run pre-RA scheduler for CM unless user forces it.
    if (!getOption(vISA_preRA_ScheduleForce))
      return false;
  }

  return (getOption(vISA_PreSchedForAcc) && (getPlatform() >= Xe2));
}

bool hasDoubleAcc() const { return getOption(vISA_hasDoubleAcc); }

bool hasHFMathGRFAlign() const {
  return VISA_WA_CHECK(getPWaTable(), Wa_22011647401);
}

bool hasHFMathSrcBroadCast() const {
  return getPlatform() == Xe_PVCXT || getPlatform() == Xe_PVC;
}

bool hasFixedCycleMathPipeline() const { return getPlatform() >= Xe_PVC; }

bool hasByteALU() const {
  // Avoid byte operands in ALU operations if hasByteALU() returns false.
  return getPlatform() < Xe_PVC;
}

bool supportsByteDestinationSubreg2Or3() const {
  // If destination is UB or B, the destination subregnum can be aligned
  // to byte 0, 1, 2 or 3 of the dword execution channel.
  return getPlatform() >= Xe_PVC;
}

bool hasSingleALUPipe() const { return getPlatform() == GENX_TGLLP; }

bool hasMathRSIsuue() const { return getPlatform() == GENX_TGLLP && getOption(vISA_InsertDummyMovForHWRSWA); }

bool supportsSampler() const {
  return getPlatform() != Xe_PVC && getPlatform() != Xe_PVCXT;
}

bool has2xDP() const { return getPlatform() >= Xe_PVCXT; }

bool has3SrcDstAlignRequirement() const { return getPlatform() < Xe_XeHPSDV; }

int get3SrcDstAlign() const { return has3SrcDstAlignRequirement() ? 8 : 2; }

bool supportCallaRegSrc() const {
  if (getPlatform() >= Xe_DG2)
    return true;
  return false;
}

bool has2xSP() const { return getPlatform() >= Xe2; }

bool hasTwoGRFBank16Bundles() const {
  const TARGET_PLATFORM P = getPlatform();
  return P == Xe_XeHPSDV || P == Xe_DG2 || P == Xe_MTL || P == Xe_ARL ||
         (P == Xe_PVC && !getOption(vISA_HasPartialInt64));
}

bool hasOneGRFBank16Bundles() const {
  const TARGET_PLATFORM P = getPlatform();
  return P != Xe_XeHPSDV && P != Xe_DG2 && P != Xe_MTL && P != Xe_ARL &&
         !(P == Xe_PVC && !getOption(vISA_HasPartialInt64));
}

bool hasDPASSrc0Src1BankConflict() const { return getPlatform() >= Xe_XeHPSDV; }

bool needsToLoadCrossThreadConstantData() const {
  return loadThreadPayload() && getOption(vISA_loadCrossThreadConstantData);
}
bool has4DeepSystolic() const { return getOption(vISA_has4DeepSystolic); }

bool hasDPAS() const {
  return getPlatform() >= Xe_XeHPSDV && getPlatform() != Xe_MTL;
}

bool hasAMFSFastClear() const { return getPlatform() >= Xe_XeHPSDV; }

// only valid type combinations during format mov conversion if acc is used:
// Src Type UD/D - Dst Type W/UW/UD/D
// Src Type UW/W - Dst Type W/UW/UD/D
// Src Type F/HF - Dst Type F/HF
// Src Type DF   - Dst Type DF
// Fixme: the rule should be removed with removedAccRestrictionsAsGRF().
bool hasFormatConversionACCRestrictions() const {
  return getPlatform() >= Xe_XeHPSDV;
}

bool hasScratchSurface() const { return getPlatform() >= Xe_XeHPSDV; }

// Note that this function is intentionally omitted from HWCapsOpen.inc to avoid
// IP leak
bool hasThreeALUPipes() const {
  const TARGET_PLATFORM P = getPlatform();
  return (P == Xe_XeHPSDV || P == Xe_DG2 || P == Xe_MTL || P == Xe_ARL);
}

bool hasFusedEU() const {
  return (getPlatform() == GENX_TGLLP || getPlatform() == Xe_XeHPSDV ||
          getPlatform() == Xe_DG2 || getPlatform() == Xe_MTL ||
          getPlatform() == Xe_ARL);
}

bool hasFusedEUNoMaskWA() const {
  return (hasFusedEU() &&
          (getOption(vISA_noMaskWA) || getOption(vISA_forceNoMaskWA)));
}

bool hasSLMWARIssue() const {
  return getPlatform() == Xe_DG2 || getPlatform() == Xe_MTL || getPlatform() == Xe_ARL ||
         getPlatform() == Xe_PVC || getPlatform() == Xe_PVCXT;
}

bool hasLongOperandTypeDepIssue() const {
  return getPlatform() == Xe_PVCXT;
}

bool hasFourALUPipes() const { return getPlatform() >= Xe_PVC; }

bool hasFiveALUPipes() const {
  return getPlatform() >= Xe2;
}

bool hasDpasSrc2ReadSupression() const { return getPlatform() >= Xe_PVCXT; }

bool hasDpasFwdAndDoubleSrcReadSupression() const {
  if (getuint32Option(vISA_enableDpasFwd) != -1)
    return getuint32Option(vISA_enableDpasFwd);
  return getPlatform() == Xe3P_CRI;
}

bool allowsMixedDstAndSrc0TypesInMacro() const {
  return getPlatform() > Xe3;
}

bool hasDpasFP4() const {
  return getPlatform() > Xe3;
}

bool hasDpasSrc2ReadSupressionSameRegSameType() const {
  return getPlatform() == Xe_PVC && !getOption(vISA_HasPartialInt64);
}

bool hasFixedCycleMathPipe() const { return getPlatform() >= Xe_PVC; }

bool hasBFDstforDPAS() const { return getPlatform() >= Xe_PVC; }

bool hasHWordBlockLoad() const { return getPlatform() >= Xe_PVC; }

bool hasFenceControl() const { return false; }

bool useLSCForPayloadLoad() const { return getPlatform() >= Xe_PVC; }

bool byteGranularitySendDep() const { return getPlatform() >= Xe_PVC && getOption(vISA_byteGranulairySendDep); }

bool hasPartialInt64Support() const {
  return getPlatform() >= Xe_PVCXT ||
         (getPlatform() >= Xe_PVC && getOption(vISA_HasPartialInt64));
}

bool hasInt64Add() const { return !getOption(vISA_HasNoInt64Add); }

bool supportsLSC() const { return getPlatform() >= Xe_DG2; }

bool useLscForNonStackSpillFill() const {
  bool useLSC = getOption(vISA_lscNonStackSpill);
  useLSC |= getPlatform() >= Xe2;
  return supportsLSC() && useLSC;
}

bool encodeSendSrc1Length() const { return getPlatform() >= Xe_DG2; }

bool hasLSCEnableHalfSIMD() const {
  return getPlatform() >= Xe_PVCXT || getOption(vISA_LSCEnableHalfSIMD);
}

bool isLSCCacheOpt17_19() const {
  if (getPlatform() >= Xe2)
    return false;
  return true;
}

bool has64bundleSize2GRFPerBank() const { return getPlatform() == Xe_ARL; }

bool has64bundleSize() const {
  return getPlatform() >= Xe2;
}

// Sampler header has additional sampler feedback surface state data.
bool hasSamplerFeedbackSurface() const { return getPlatform() >= Xe2; }
bool hasSampleMlod() const { return getPlatform() >= Xe2; }

bool hasDPASSrc2ReadSuppressionIssue() const { return getPlatform() == Xe_PVC; }
bool hasDPASSrc2ReadSuppressionDepIssue() const {
  return getPlatform() == Xe_PVC;
}

bool hasIntraReadSuppressionIssue() const {
  return (VISA_WA_CHECK(getPWaTable(), Wa_14018126777));
}

bool hasMathDpasConflict() const {
  return (getPlatform() == Xe_PVC) && getOption(vISA_EnableMathDPASWA);
}

bool waDisableIntMac() const { return getPlatform() == Xe_PVC; }

bool hasGRFAlignedSrc2DPAS() const { return getPlatform() >= Xe_PVC; }

bool hasWriteCombine() const {
  return getPlatform() >= Xe2;
}

bool needWriteCombineWA() const {
  return (VISA_WA_CHECK(getPWaTable(), Wa_18042479026));
}

bool hasA0WARHWissue() {
  return getPlatform() >= Xe_XeHPSDV && getPlatform() < Xe2;
}

bool hasFtoPackedHFMove() const { return getPlatform() >= Xe_DG2; }

bool hasGather4PO() const { return getPlatform() <= GENX_TGLLP; }

bool hasQ2FInIntegerPipe() const {
  return getOption(vISA_Q2FInIntegerPipe); // getPlatform() >= Xe2;
}

bool enableSendIndirect() const {
  return getuint32Option(vISA_EnableGatherWithImmPreRA) &&
    (getuint32Option(vISA_EnableGatherWithImmPreRA) < 4) &&
    (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D) &&
    getPlatform() >= Xe3;
}

unsigned int getMaxPTSS() const {
  if (getuint32Option(vISA_maxPTSSOverride)) {
    return getuint32Option(vISA_maxPTSSOverride);
  }

  if (getPlatform() > Xe3) {
      return 16 * 1024 * 1024;
  }
  if (hasScratchSurface()) {
    // Max PTSS supported is 256kb
    return 256 * 1024;
  }

  // Max PTSS supported is 2MB
  return 2 * 1024 * 1024;
}

uint32_t getInlineDataSize() const {
  if (isEfficient64bEnabled()) {
    return 64;
  }
  if (getPlatform() >= Xe_XeHPSDV) {
    return 32;
  }
  return 0;
}

bool isEfficient64bEnabled() const {
  return getPlatform() > Xe3 && getOption(vISA_enableEfficient64b);
}
bool isKernelArgumentEnabled() const {
  return getPlatform() >= Xe3 && getOption(vISA_EnableKernelArgument);
}

// Return true if platform has structured control-flow instructions.
// Note: stop using SCF for DG2+ even though DG2/PVC still support it.
bool hasSCF() const { return (getPlatform() < Xe_DG2); }

bool hasBDstWSrc1EvenAlignIssue() const {
  return getPlatform() == Xe_PVCXT || getPlatform() == Xe2;
}

bool removedAccRestrictionsAsGRF() const {
  return getOption(vISA_GAReArchACC) && getPlatform() > Xe3;
}

bool hasSimplifiedRegions() const {
  return getPlatform() > Xe3;
}

bool needBothAcc(G4_Operand *opnd) const {
  if (hasSimplifiedRegions() || getOption(vISA_GAReArchBugFix)) {
    return (((unsigned)opnd->getInst()->getExecSize() * opnd->getTypeSize())
        > getACCSize());
  }
  switch (opnd->getType()) {
  case Type_UD:
  case Type_D:
  case Type_W:
  case Type_UW:
    return true;
  default:
    return (((unsigned)opnd->getInst()->getExecSize() * opnd->getTypeSize())
        > getACCSize());
  }
}

bool needBarrierWA() const {
  return (getOption(vISA_enableBarrierWA) &&
      (getPlatform() == Xe_XeHPSDV || getPlatform() == Xe_PVC ||
      getPlatform() == Xe_PVCXT || getPlatform() == Xe_DG2 ||
      getPlatform() == Xe_MTL));
}

bool hasSelWDstDwExecTypeAlignIssue() const {
  return getPlatform() == Xe_PVCXT || VISA_WA_CHECK(getPWaTable(), Wa_18027439769);
}

bool hasGatherReadSuppressionWARA() const {
  return (VISA_WA_CHECK(getPWaTable(), Wa_14019028097));
}

bool needEmaskSetupProlog() const {
  if (!getOption(vISA_addEmaskSetupProlog))
    return false;
  // Disable the WA for TGLLP for now because of a possible RS issue.
  return getPlatform() > GENX_TGLLP && getPlatform() <= Xe_PVCXT;
}

bool needLSCFenceDiscardWA() const {
  return (VISA_WA_CHECK(getPWaTable(), Wa_22017182272));
}

bool needIndirectSrcForCompressedInstWA() const {
  if (getOption(vISA_noIndirectSrcForCompressedInstWA))
    return false;

  return VISA_WA_CHECK(getPWaTable(), Wa_13010473643);
}

bool hasPackedRestrictedFloatVector() const {
  return getPlatform() < Xe2;
}

bool noSrc0Src1OverlapSend() const {
  return getPlatformGeneration() < PlatformGen::XE;
}

bool enableMovCvt() const { return getPlatform() > Xe3; }

bool hasAPQIDInMsg0() const { return getPlatform() > Xe3; }

bool WARLocalization() const {
  if (getOption(vISA_SWSBMakeLocalWAR)) {
    return true;
  }

  return false;
}

bool PVCSendWARWA() const {
  if (getOption(vISA_PVCSendWARWA) && (getPlatform() == Xe_PVC || getPlatform() == Xe_PVCXT)) {
    return true;
  }
  return false;
}

bool supportPureBF() const {
  return getPlatform() > Xe3;
}

// WA to support debugger stepping. Currently only enabled when compiling with
// -debug.
bool needNopAfterCFInstWA() const {
  const TARGET_PLATFORM p = getPlatform();
  if (p == Xe_DG2 || p == Xe_PVC || p == Xe_PVCXT)
    return getOption(vISA_Debug);
  return false;
}

bool modelSendSrcReadLatency () const {
  return getOption(vISA_schedWithSendSrcReadCycle);
}

bool hasReadSuppressionOrSharedLocalMemoryWAs() const {
  return true;
}

bool supportNativeSIMD32() const {
  return getPlatform() >= Xe3P_CRI && getOption(vISA_enableOptimizeSIMD32);
}

bool supports4GRFAlign() const {
  return supportNativeSIMD32();
}

bool onlyDoLocalVariableSplitWhenSpill() const {
  return getPlatform() >= Xe3P_Graphics && getOption(vISA_DelayLocalDeclareSplitInGlobalRA);
}

bool needA0WAR() const {
  return (getPlatform() >= Xe2);
}

bool needPredicateWAR() const {
  return (getPlatform() < Xe3);
}

bool alwaysAllowGlobalFlagOpt() const {
  // We shouldn't add this kind of ugly platform check. However, there's
  // a complicated bug found in TGLLP when disabling a flagopt case. There
  // might be some random behavior that creates non-deterministic visa inputs
  // making debugging extremely difficult. Given that TGLLP and other old
  // platforms are not the top priority so here we add this WA to keep the
  // behavior for now.
  return getPlatform() <= GENX_TGLLP;
}
bool hasMulMacRSIssue() const {
  if (getOption(vISA_hasMulMacRSIssue))
    return true;

  return VISA_WA_CHECK(getPWaTable(), Wa_18035690555);
}

uint32_t getSizeOfSendQueue() const {
  if (getuint32Option(vISA_SendQueueEntries)) {
    return getuint32Option(vISA_SendQueueEntries);
  }
  if (getPlatform() > Xe3) {
    return 8;
  }
  return 6;
}

bool hasImmOnSrc0Src2ForAlign1Ternary() const {
  return getPlatformGeneration() >= PlatformGen::XE;
}

bool needTGMDoubleFenceWA() const {
  return VISA_WA_CHECK(getPWaTable(), Wa_14021891663) &&
      getOption(vISA_TGMDoubleFenceWA);
}

bool useDynamicAddrForExDesc() const {
  return getPlatform() <= Xe3 && getOption(vISA_dynamicAddrForExDescInLscSend);
}

bool samplerCachingInHeader() const {
  if (m_options->getOption(vISA_enableEfficient64b))
    return false;
  return (getPlatform() >= TARGET_PLATFORM::Xe3P_Graphics &&
      getOption(vISA_enableSamplerLSCCaching));
}

bool src1Src2SwapForCompaction() {
  return getPlatform() >= Xe3P_Graphics && getOption(vISA_Compaction);
}

bool reserver510() {
  return false;
}

bool skipPredicateAndExecMaskForWARDepCheck() {
  return false;
}

// Check if the message supports address offset scaling according to the option
// setting
bool supportSendAddrScale(bool isSLM, bool isBTS) {
  // global address can support offset scaling
  if (!isSLM && !isBTS)
    return true;

  // vISA_supportLSCImmScale value:
  // 0: disable both SLM and UGM-scratch
  // 1: enable SLM
  // 2: enable UGM-scratch
  // 3: enable both SLM and UGM-scratch (default)
  auto immScaleOptVal = getuint32Option(vISA_supportLSCImmScale);

  // Otherwise, enable SLM/UGM offset scaling according to immScaleOptVal:
  // offset scaling is supported for SLM and UGM-scratch
  if (immScaleOptVal == 3)
    return true;

  // offset scaling is supported for SLM
  if (isSLM && immScaleOptVal == 1)
    return true;

  // offset scaling is supported for UGM-scratch
  if (isBTS && immScaleOptVal == 2)
    return true;

  return false;
}
bool needThreeSrcInstNullSrc2WA() {
  return getPlatform() == Xe3P_Graphics;
}
// end HW capabilities
