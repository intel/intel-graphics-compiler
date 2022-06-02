/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
// HW Capabilities
// these functions are part of IR_Builder class and included in Build_IR.h

    bool hasMixMode() const
    {
        if (getOption(vISA_ForceMixMode))
        {
            return true;
        }
        if (getOption(vISA_DisableMixMode))
        {
            return false;
        }

        return getPlatform() > GENX_BDW &&
            getPlatformGeneration() != PlatformGen::GEN11;
    }

    // True if it has bfloat16 mixed mode support.
    bool hasBFMixMode() const { return hasDPAS();}

    bool forceSamplerHeader() const
    {
        return m_options->getOption(vISA_forceSamplerHeader) ||
            (getPlatform() < GENX_ICLLP && m_options->getOption(vISA_enablePreemption));
    }

    bool samplerHeaderPreemptionWA() const
    {
        return m_options->getOption(vISA_enablePreemption) && m_options->getOption(vISA_samplerHeaderWA);
    }

    bool needsNoPreemptR2ForSend() const
    {
        return getPlatformGeneration() == PlatformGen::GEN11;
    }

    bool noDDAllowedPlatform() const
    {
        return getPlatform() >= GENX_SKL;
    }

    bool noSrcDepSetAllowedPlatform() const
    {
        return getPlatform() >= GENX_SKL;
    }

    bool doPlane() const
    {
        return getPlatform() < GENX_ICLLP && !getOption(vISA_expandPlane);
    }

    bool favorFloatMov() const
    {
        return getPlatformGeneration() == PlatformGen::GEN11 || isXeLP();
    }

    bool noScalarJmp() const
    {
        return !getOption(vISA_EnableScalarJmp);
    }

    bool hasAlign1Ternary() const
    {
        return getPlatform() >= GENX_CNL && getOption(vISA_doAlign1Ternary);
    }

    bool encodeUnitStrideTernary() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    bool hasMacl() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool hasCPS() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool hasBindlessSampler() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool noSrc2Regioning() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    bool noSrc1Byte() const
    {
        return getOption(vISA_noSrc1Byte) || getPlatform() >= GENX_ICLLP;
    }

    bool noDFTypeMac() const
    {
        if (getPlatform() == Xe_PVC)
            return true;
        return false;
    }

    bool needsFenceCommitEnable() const
    {
        if (m_options->getTarget() == VISA_CM)
        {
            return getPlatform() >= GENX_SKL;
        }
        else
        {
            return getPlatform() >= GENX_CNL;
        }
    }

    bool hasIEEEDivSqrt() const
    {
        return getPlatform() < GENX_ICLLP;
    }

    bool gotoJumpOnTrue() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool needsToReserveR127() const
    {
        return getPlatform() < GENX_CNL;
    }

    bool hasSLMFence() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    bool GRFAlign() const
    {
        return getPlatform() < GENX_SKL;
    }

    bool twoSourcesCollision() const
    {
        return getPlatform() < GENX_CNL;
    }

    bool lowHighBundle() const
    {
        return getPlatformGeneration() <= PlatformGen::GEN11;
    }

    bool hasSendShootdown() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool useNewR0Format() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    int getPredMaskEnableBit() const
    {
        return getPlatform() < GENX_ICLLP ? 30 : 23;
    }

    int getBarrierIDMask() const
    {
        return getPlatform() < GENX_ICLLP ? 0x8F000000 : 0x7F000000;
    }

    uint32_t getMaxSendMessageLength() const
    {
        return getPlatform() < GENX_CNL ? 16 : 32;
    }

    bool hasPixelNullMask() const
    {
        return getPlatform() >= GENX_SKL;
    }

    bool noSLMMsgHeader() const
    {
        return getPlatform() >= GENX_SKL;
    }

    bool needsA32MsgHeader() const
    {
        return getPlatform() < GENX_SKL;
    }

    int getThreadIDMask() const
    {
        return getPlatform() >= GENX_SKL ? 0x7FF : 0x1FF;
    }

    bool hasFloatAtomics() const
    {
        return getPlatform() >= GENX_SKL;
    }

    bool hasBlockedSLMMessage() const
    {
        return false;
    }

    bool hasHeaderlessMRTWrite() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    bool hasDotProductInst() const
    {
        return getPlatform() < GENX_CNL;
    }

    bool hasLRP() const
    {
        return getPlatform() < GENX_ICLLP;
    }

    int getBarrierMask(bool enableBarrierInstCounterBits) const
    {
        if (getPlatform() < GENX_SKL)
        {
            //pre-SKL: and (8) H0.0:ud r0.2:ud 0x0F000000 (r0.2, bit 24-27)
            return enableBarrierInstCounterBits ? 0x0F00FE00 : 0x0F000000;
        }
        else if (getPlatform() < GENX_ICLLP)
        {
            //SKL+: and (8) H0.0:ud r0.2:ud 0x8F000000 (r0.2, bit24-27, bit31)
            return enableBarrierInstCounterBits ? 0x8F00FE00 : 0x8F000000;
        }
        else
        {
            //else: and (8) H0.0:ud r0.2:ud 0x7F000000 (r0.2, bit24-30)
            return enableBarrierInstCounterBits ? 0x7F00FF00 : 0x7F000000;
        }
    }

    bool canMadHaveAcc() const
    {
        return getPlatform() >= GENX_CNL;
    }

    bool hasFdivPow() const
    {
        return getPlatformGeneration() < PlatformGen::XE;
    }

    bool hasFdivPowWA() const
    {
        return getPlatform() < GENX_ICLLP;
    }

    bool hasCondModForTernary() const
    {
        return getPlatformGeneration() < PlatformGen::XE;
    }

    bool hasSWSB() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    bool hasPartialMixMode() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    // whether EOT sources need to be assigned r112-r127
    bool hasEOTGRFBinding() const
    {
        return true;
    }

    bool hasSmov() const
    {
        return getPlatformGeneration() < PlatformGen::XE;
    }

    bool doAccSub() const
    {
        return getPlatformGeneration() >= PlatformGen::GEN11;
    }

    bool hasNFType() const
    {
        return getPlatform() >= GENX_ICLLP &&
               getPlatformGeneration() < PlatformGen::XE;
    }

    void getSSIDBits(uint32_t& width, uint32_t& start) const
    {
        if (getPlatform() < GENX_CNL)
        {
            width = 2;
            start = 12; //[12:13]
        }
        else if (getPlatform() == GENX_CNL)
        {
            width = 2;
            start = 8; //[8:9]
        }
        else
        {
            width = 3;
            start = 8;//[8:10]
        }
    }

    bool encodeAccRegSelAsAlign1() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    bool fuseTypedWrites() const
    {
        return getOption(vISA_FuseTypedWrites);
    }

    bool avoidWAWSubRegHazard() const
    {
        return getPlatformGeneration() < PlatformGen::XE &&
               getOption(vISA_WAWSubregHazardAvoidance);
    }

    bool newTernaryStride() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    // acc restrictions that are relaxed for Xe
    // -- dp4a can take acc src/dst
    // -- the same acc may be used in both src0 and src1 of a three-source inst
    bool relaxedACCRestrictions2() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    bool fuseURBMessage() const
    {
        return isXeLP();
    }

    bool supportSrcModforMul() const
    {
        return getPlatformGeneration() < PlatformGen::XE;
    }

    bool doMultiAccSub() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    bool canMadHaveSrc0Acc() const
    {
        return getPlatformGeneration() >= PlatformGen::XE;
    }

    bool accDstforIndirectSrc() const
    {
        return getPlatformGeneration() < PlatformGen::XE;
    }

    bool restrictedACCRestrictions() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool favorFpMad() const
    {
        return getPlatform() < GENX_CNL;
    }

    bool avoidAccDstWithIndirectSource() const
    {
        return getPlatform() >= GENX_TGLLP;
    }

    bool hasRSForSpecificPlatform() const
    {
        if (getPlatform() == Xe_DG2)
        {
            return m_options->getOption(vISA_ManualEnableRSWA);
        }
        return true;
    }

    bool hasSrc1ReadSuppressionIssue() const
    {
        return getPlatform() == Xe_PVC && hasRSForSpecificPlatform();
    }

    bool hasReadSuppression() const
    {
        return getPlatform() >= GENX_TGLLP && hasRSForSpecificPlatform();
    }

    bool hasEOTReadSuppressionIssue() const
    {
        if(getPlatform() == Xe_DG2 && hasRSForSpecificPlatform())
        {
            return true;
        }
        return false;
    }

    bool hasScalarReadSuppression() const
    {
        return getPlatform() > GENX_TGLLP && hasRSForSpecificPlatform();
    }

    bool hasFPU0ReadSuppressionIssue() const
    {
        return getPlatform() == Xe_PVCXT && hasRSForSpecificPlatform();
    }


    bool alignBindlessSampler() const
    {
        return getPlatformGeneration() == PlatformGen::GEN9;
    }

    bool noL3Flush() const
    {
        return getPlatform() == GENX_TGLLP;
    }

    bool needResetA0forVxHA0() const
    {
        return getPlatform() >= GENX_ICLLP;
    }

    unsigned getCoIssueUints() const
    {
#if 0
        auto GEN = getPlatformGeneration();
        return (GEN >= PlatformGen::XE) ? 1 : 2;
#else
        return 2;
#endif
    }

    bool useMultiThreadLatency() const
    {
#if 0
        auto GEN = getPlatformGeneration();
        if (GEN >= PlatformGen::XE)
            return false;
#endif

        return getOptions()->getOption(vISA_useMultiThreadedLatencies);
    }

    bool useIGAEncoder() const
    {
        return getOption(vISA_IGAEncoder) || getPlatformGeneration() >= PlatformGen::XE;
    }

    bool needReplaceIndirectCallWithJmpi() const
    {
        return getPlatform() <= GENX_ICLLP ||
               getOption(vISA_replaceIndirectCallWithJmpi);
    }

    bool needIPWA() const
    {
        return getPlatform() == Xe_XeHPSDV;
    }

    bool canEncodeFullExtDesc() const
    {
        return getPlatform() >= GENX_TGLLP;
    }

    bool needSwap64ImmLoHi() const
    {
        return getPlatform() >= GENX_TGLLP;
    }

    bool has8ByteA64Gather() const
    {
        return getPlatform() != GENX_TGLLP;
    }

    bool WaDisableSendSrcDstOverlap() const
    {
        return getOption(vISA_noSendSrcDstOverlap) ||
        (m_options->getTarget() == VISA_CM && getPlatform() >= GENX_SKL && getPlatform() < GENX_TGLLP) ||
        getPlatform() == GENX_ICLLP;
    }

    bool isXeLP() const
    {
        return getPlatform() == GENX_TGLLP;
    }

    bool hasEarlyGRFRead() const
    {
        return (getPlatform() == GENX_TGLLP) && getOption(vISA_HasEarlyGRFRead);
    }


    bool noInt64() const
    {
        return getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2;
    }

    bool noFP64() const
    {
        return getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2;
    }

    bool no64bitRegioning() const
    {
        return getPlatform() == GENX_CHV || getPlatform() == GENX_BXT ||
               getPlatform() == GENX_ICLLP || isXeLP() || getPlatform() == Xe_DG2;
    }

    bool hasBankCollision() const
    {
        if(getPlatform() == GENX_TGLLP)
        {
            return true;
        }
        return (getPlatformGeneration() <= PlatformGen::GEN11) || getOption(vISA_forceBCR) ||
        (getOption(vISA_enableBCR) && !hasEarlyGRFRead());
    }

    bool oneGRFBankDivision() const
    {
        return getPlatform() != Xe_XeHPSDV && getPlatform() != Xe_DG2 && !(getPlatform() == Xe_PVC && !getOption(vISA_HasPartialInt64));
    }

    bool hasMadm() const
    {
        return getPlatform() != GENX_ICLLP && !isXeLP() && getPlatform() != Xe_DG2;
    }

    bool hasSIMD16TypedRW() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool hasRegDistDepIssue() const
    {
        return getPlatform() == Xe_PVCXT;
    }

    bool doNotRewriteContiguousRegion() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    // acc restrictions that are relaxed for Xe HP
    // -- mov can have both acc src and dst simultaneously
    // -- acc0 and acc1 may both be used simultaneously in one inst
    // -- acc can be packed HF dst of a mix mode inst
    bool relaxedACCRestrictions() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool relaxedACCRestrictions_1() const
    {
        return getPlatform() >= Xe_DG2;
    }

    bool loadThreadPayload() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool needFenceBeforeEOT() const
    {
        return getPlatform() == Xe_XeHPSDV;
    }

    bool hasSrc0ReadSuppression() const
    {
        return getPlatform() >= Xe_XeHPSDV && hasRSForSpecificPlatform();
    }

    bool hasRMWReadSuppressionIssue() const
    {
        return (getPlatform() == Xe_DG2 || getPlatform() == Xe_XeHPSDV) && hasRSForSpecificPlatform();
    }

    bool needToClearScratchWrites() const
    {
        return getPlatform() < Xe_XeHPSDV;
    }

    bool needsToLoadLocalID() const
    {
        return loadThreadPayload() && getPerThreadInputSize() > 0 && !getOption(vISA_autoLoadLocalID);
    }

    bool noDwDstForDwordMul() const
    {
        return noInt64() || getPlatform() >= Xe_XeHPSDV;
    }

    bool useNewExtDescFormat() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool has16OWordSLMBlockRW() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool hasVxHFloat64b() const
    {
        return getPlatform() < Xe_XeHPSDV;
    }

    bool supportFloatOr64bRegioning() const
    {
        return getPlatform() < Xe_XeHPSDV && !getOption(vISA_forceNoFP64bRegioning);
    }

    int getFP64MadmExecSize() const
    {
        auto plat = getPlatform();
        if (plat >= Xe_PVC)
        {
            return 16;
        }
        else if (plat == Xe_XeHPSDV || plat == Xe_DG2)
        {
            return 8;
        }
        else
        {
            return 4;
        }
    }

    bool balanceIntFloatMoves() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    G4_Type getMixModeType() const
    {
        return getPlatform() >= Xe_XeHPSDV ? Type_BF : Type_HF;
    }

    // each flag register is 16-bit
    uint32_t getNumFlagRegisters(void)
    {
        return getPlatform() >= Xe_PVC ? 8 : 4;
    }

    uint32_t getNumScalarRegisters(void)
    {
        if (hasScalarRegister())
        {
            return 1;
        }
        return 0;
    }

    bool encodeAccWrEn() const
    {
        return getPlatform() < Xe_PVC;
    }

    SFID getEOTSFID() const
    {
        return getPlatform() >= Xe_DG2 ? SFID::GATEWAY : SFID::SPAWNER;
    }

    // EU native execution size for 32-bit types
    G4_ExecSize getNativeExecSize() const
    {
        return getPlatform() >= Xe_PVC ? g4::SIMD16 : g4::SIMD8;
    }

    bool noScalarByteToFloat() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool noHFToInteger() const
    {
        return getPlatform() == Xe_PVCXT;
    }

    bool useAccForDF() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool useAccForMadm() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool hasUnifiedBarrier() const
    {
        return getPlatform() >= Xe_DG2;
    }

    bool predCtrlHasWidth() const
    {
        return getPlatform() < Xe_PVC;
    }

    bool hasNibCtrl() const
    {
        return getPlatform() < Xe_PVC;
    }

    bool hasMaskForScratchMsg() const
    {
        return getPlatform() < Xe_PVC;
    }

    bool avoidDstSrcOverlap()
    {
        return getPlatform() >= GENX_ICLLP && getOption(vISA_DstSrcOverlapWA);
    }

    bool avoidSrc1Src2Overlap()
    {
        return getOption(vISA_Src1Src2OverlapWA);
    }

    bool hasTwoGRFMathMacro() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasFP64Acc() const
    {
        return !noFP64();
    }

    bool hasMathAcc() const
    {
        return getOption(vISA_mathAccSub);
    }

    //to relax ACC usage restrictions for FP MUL and MAD instructions.
    //1. Support both sources as ACC for FP MUL
    //2. Support Src2 as ACC for FP MAD
    bool relaxedACCRestrictions3() const
    {
        return false;
    }

    bool enableACCBeforRA() const
    {
        return false;
    }

    bool enablePreSchedACC() const
    {
        return false;
    }

    bool hasDoubleAcc() const
    {
        return getOption(vISA_hasDoubleAcc);
    }

    bool hasHFMathGRFAlign() const
    {
        return getPlatform() == Xe_PVCXT;
    }

    bool hasFixedCycleMathPipeline() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasByteALU() const
    {
        // Avoid byte operands in ALU operations if hasByteALU() returns false.
        return getPlatform() < Xe_PVC;
    }

    bool hasSingleALUPipe() const
    {
        return getPlatform() == GENX_TGLLP;
    }

    bool hasMathRSIsuue() const
    {
        return getPlatform() == GENX_TGLLP;
    }

    bool supportsSampler() const
    {
        return getPlatform() != Xe_PVC && getPlatform() != Xe_PVCXT;
    }

    bool has2xDP() const
    {
        return getPlatform() >= Xe_PVCXT;
    }

    int get3SrcDstAlign() const
    {
        return getPlatform() >= Xe_XeHPSDV ? 2 : 8;
    }

    bool supportCallaRegSrc() const
    {
        if (getPlatform() >= Xe_DG2)
            return true;
        return false;
    }


    bool hasTwoGRFBank16Bundles() const
    {
        const TARGET_PLATFORM P = getPlatform();
        return P == Xe_XeHPSDV || P == Xe_DG2 || (P == Xe_PVC && !getOption(vISA_HasPartialInt64));
    }

    bool hasOneGRFBank16Bundles() const
    {
    const TARGET_PLATFORM P = getPlatform();
        return P != Xe_XeHPSDV && P != Xe_DG2 && !(P == Xe_PVC && !getOption(vISA_HasPartialInt64));
    }

    bool hasDPASSrc0Src1BankConflict() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool needsToLoadCrossThreadConstantData() const
    {
        return loadThreadPayload() && getOption(vISA_loadCrossThreadConstantData);
    }

    bool hasDPAS() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool hasAMFSFastClear() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    // only valid type combinations during format mov conversion if acc is used:
    // Src Type UD/D - Dst Type W/UW/UD/D
    // Src Type UW/W - Dst Type W/UW/UD/D
    // Src Type F/HF - Dst Type F/HF
    // Src Type DF   - Dst Type DF
    bool hasFormatConversionACCRestrictions() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    bool hasScratchSurface() const
    {
        return getPlatform() >= Xe_XeHPSDV;
    }

    // Note that this function is intentionally omitted from HWCapsOpen.inc to avoid IP leak
    bool hasThreeALUPipes() const
    {
        const TARGET_PLATFORM P = getPlatform();
        return (P == Xe_XeHPSDV || P == Xe_DG2);
    }

    bool hasFusedEUWA() const
    {
        return ((getuint32Option(vISA_noMaskWA) & 0x3) > 0 ||
                getOption(vISA_forceNoMaskWA)) && !getOption(vISA_LinearScan) && hasFusedEU();
    }

    bool hasFusedEU() const
    {
        return (getPlatform() == GENX_TGLLP || getPlatform() == Xe_XeHPSDV || getPlatform() == Xe_DG2);
    }

    bool hasFusedEUNoMaskWA() const
    {
        return (hasFusedEU() &&
                ((getuint32Option(vISA_noMaskWA) & 0x3) > 0 || getOption(vISA_forceNoMaskWA)));
    }

    // temp
    bool useNewNoMaskWA() const
    {
        return (getOption(vISA_newNoMaskWA) && m_options->getTarget() == VISA_CM)  // VC
             || getuint32Option(vISA_newTmpNoMaskWA) > 0;                          // scalar igc
    }

    bool hasSLMWARIssue() const
    {
        return getPlatform() == Xe_DG2 || getPlatform() == Xe_PVC || getPlatform() == Xe_PVCXT;
    }

    bool hasLongOperandTypeDepIssue() const
    {
        return getPlatform() == Xe_PVCXT;
    }

    bool hasFourALUPipes() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasFiveALUPipes() const
    {
       return false;
    }

    bool hasDpasSrc2ReadSupression() const
    {
        return getPlatform() >= Xe_PVCXT;
    }


    bool hasDpasSrc2ReadSupressionSameRegSameType() const
    {
        return getPlatform() == Xe_PVC && !getOption(vISA_HasPartialInt64);
    }

    bool hasFixedCycleMathPipe() const
    {
        return getPlatform() >= Xe_PVC;
    }

     bool hasBFDstforDPAS() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasHWordBlockLoad() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasFenceControl() const
    {
        return false;
    }

    bool useLSCForPayloadLoad() const
    {
        return getPlatform() >= Xe_PVC;
    }

    bool hasPartialInt64Support() const
    {
        return getPlatform() >= Xe_PVCXT || (getPlatform() >= Xe_PVC && getOption(vISA_HasPartialInt64));
    }

    bool hasInt64Add() const
    {
        return getOption(vISA_HasInt64Add);
    }

    bool supportsLSC() const
    {
        return getPlatform() >= Xe_DG2;
    }

    bool encodeSendSrc1Length() const
    {
        return getPlatform() >= Xe_DG2;
    }

    bool hasLSCEnableHalfSIMD() const
    {
        return getPlatform() >= Xe_PVCXT || getOption(vISA_LSCEnableHalfSIMD);
    }

    bool isLSCCacheOpt17_19() const
    {
        return true;
    }


   bool has64bundleSize() const
   {
        return false;
   }


   bool hasDPASSrc2ReadSuppressionIssue() const
   {
        return getPlatform() == Xe_PVC;
   }
   bool hasDPASSrc2ReadSuppressionDepIssue() const
   {
        return getPlatform() == Xe_PVC;
   }

    bool hasMathDpasConflict() const
    {
        return (getPlatform() == Xe_PVC) && getOption(vISA_EnableMathDPASWA);
    }

   bool waDisableIntMac() const
   {
        return getPlatform() == Xe_PVC;
   }

   bool hasWriteCombine() const
   {
        return false;
   }

   bool hasA0WARHWissue()
   {
       return (getPlatform() == Xe_XeHPSDV || getPlatform() == Xe_DG2 || getPlatform() == Xe_PVC);
   }

   bool hasFtoPackedHFMove() const
   {
      return getPlatform() >= Xe_DG2;
   }

   bool hasGather4PO() const
   {
       return getPlatform() <= GENX_TGLLP;
   }

   bool hasQ2FInIntegerPipe() const
   {
       return false;
   }

   bool hasScalarRegister() const
   {
       return false;
   }

   unsigned int getMaxPTSS() const
   {
      if (getPlatform() >= GENX_TGLLP)
      {
         // Max PTSS supported is 256kb
         return 256 * 1024;
      }

      // Max PTSS supported is 2MB
      return 2 * 1024 * 1024;
   }


   uint32_t getInlineDataSize() const
   {
        if (getPlatform() >= Xe_XeHPSDV)
        {
            return 32;
        }
        return 0;
   }

   // end HW capabilities
