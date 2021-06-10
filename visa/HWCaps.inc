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
            getPlatformGeneration(getPlatform()) != PlatformGen::GEN11;
    }

    bool forceSamplerHeader() const
    {
        return m_options->getOption(vISA_forceSamplerHeader) ||
            (getPlatform() < GENX_ICLLP && m_options->getOption(vISA_enablePreemption));
    }

    bool needsNoPreemptR2ForSend() const
    {
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN11;
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
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN11 || isXeLP();
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
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
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
        return getPlatformGeneration(getPlatform()) <= PlatformGen::GEN11;
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
        return getPlatformGeneration(getPlatform()) < PlatformGen::XE;
    }

    bool hasFdivPowWA() const
    {
        return getPlatform() < GENX_ICLLP;
    }

    bool hasCondModForTernary() const
    {
        return getPlatformGeneration(getPlatform()) < PlatformGen::XE;
    }

    bool hasSWSB() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    bool hasPartialMixMode() const
    {
        return getPlatformGeneration(getPlatform()) == PlatformGen::XE;
    }

    // whether EOT sources need to be assigned r112-r127
    bool hasEOTGRFBinding() const
    {
        return true;
    }

    bool hasSmov() const
    {
        return getPlatformGeneration(getPlatform()) < PlatformGen::XE;
    }

    bool doAccSub() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN11;
    }

    bool hasNFType() const
    {
        return getPlatform() >= GENX_ICLLP &&
               getPlatformGeneration(getPlatform()) != PlatformGen::XE;
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
        return getPlatformGeneration(getPlatform()) < PlatformGen::XE &&
               getOption(vISA_WAWSubregHazardAvoidance);
    }

    bool newTernaryStride() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    // acc restrictions that are relaxed for Xe
    // -- dp4a can take acc src/dst
    // -- the same acc may be used in both src0 and src1 of a three-source inst
    bool relaxedACCRestrictions2() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    bool fuseURBMessage() const
    {
        return isXeLP();
    }

    bool supportSrcModforMul() const
    {
        return getPlatformGeneration(getPlatform()) != PlatformGen::XE;
    }

    bool doMultiAccSub() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    bool canMadHaveSrc0Acc() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    bool accDstforIndirectSrc() const
    {
        return getPlatformGeneration(getPlatform()) != PlatformGen::XE;
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
        return true;
    }

    bool hasReadSuppression() const
    {
        return getPlatform() >= GENX_TGLLP && hasRSForSpecificPlatform();
    }

    bool hasEOTReadSuppressionIssue() const
    {
        return false;
    }

    bool hasScalarReadSuppression() const
    {
        return getPlatform() > GENX_TGLLP && hasRSForSpecificPlatform();
    }

    bool alignBindlessSampler() const
    {
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN9;
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
        auto GEN = getPlatformGeneration(getPlatform());
        return (GEN >= PlatformGen::XE) ? 1 : 2;
#else
        return 2;
#endif
    }

    bool useMultiThreadLatency() const
    {
#if 0
        auto GEN = getPlatformGeneration(getPlatform());
        if (GEN >= PlatformGen::XE)
            return false;
#endif

        return getOptions()->getOption(vISA_useMultiThreadedLatencies);
    }

    bool useIGAEncoder() const
    {
        return getOption(vISA_IGAEncoder) || getPlatformGeneration(getPlatform()) >= PlatformGen::XE;
    }

    bool needReplaceIndirectCallWithJmpi() const
    {
        return getPlatform() <= GENX_ICLLP ||
               getOption(vISA_replaceIndirectCallWithJmpi);
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
        return getPlatform() == GENX_ICLLP || isXeLP();
    }

    bool noFP64() const
    {
        return getPlatform() == GENX_ICLLP || isXeLP();
    }

    bool no64bitRegioning() const
    {
        return getPlatform() == GENX_CHV || getPlatform() == GENX_BXT ||
               getPlatform() == GENX_ICLLP || isXeLP();
    }

    bool hasBankCollision() const
    {
        return (getPlatformGeneration(getPlatform()) <= PlatformGen::GEN11) || getOption(vISA_forceBCR) ||
        (getOption(vISA_enableBCR) && !hasEarlyGRFRead());
    }

    bool oneGRFBankDivision() const
    {
        return getPlatform() != XE_HP;
    }

    bool hasMadm() const
    {
        return getPlatform() != GENX_ICLLP && !isXeLP();
    }

    bool hasSIMD16TypedRW() const
    {
        return getPlatform() >= XE_HP;
    }

    bool hasRegDistDepIssue() const
    {
        return false;
    }

    bool doNotRewriteContiguousRegion() const
    {
        return getPlatform() >= XE_HP;
    }

    // acc restrictions that are relaxed for Xe HP
    // -- mov can have both acc src and dst simultaneously
    // -- acc0 and acc1 may both be used simultaneously in one inst
    // -- acc can be packed HF dst of a mix mode inst
    bool relaxedACCRestrictions() const
    {
        return getPlatform() >= XE_HP;
    }

    bool relaxedACCRestrictions_1() const
    {
        return false;
    }

    bool loadThreadPayload() const
    {
        return getPlatform() >= XE_HP;
    }

    bool needFenceBeforeEOT() const
    {
        return getPlatform() == XE_HP;
    }

    bool hasSrc0ReadSuppression() const
    {
        return getPlatform() >= XE_HP && hasRSForSpecificPlatform();
    }

    bool needToClearScratchWrites() const
    {
        return getPlatform() < XE_HP;
    }

    bool needsToLoadLocalID() const
    {
        return loadThreadPayload() && getPerThreadInputSize() > 0 && !getOption(vISA_autoLoadLocalID);
    }

    bool noDwDstForDwordMul() const
    {
        return noInt64() || getPlatform() >= XE_HP;
    }

    bool useNewExtDescFormat() const
    {
        return getPlatform() >= XE_HP;
    }

    bool has16OWordSLMBlockRW() const
    {
        return getPlatform() >= XE_HP;
    }

    bool hasVxHFloat64b() const
    {
        return getPlatform() < XE_HP;
    }

    bool supportFloatOr64bRegioning() const
    {
        return getPlatform() < XE_HP && !getOption(vISA_forceNoFP64bRegioning);
    }

    int getFP64MadmExecSize() const
    {
        auto plat = getPlatform();
        return (plat == XE_HP) ? 8 : 4;
    }

    bool balanceIntFloatMoves() const
    {
        return getPlatform() >= XE_HP;
    }

    G4_Type getMixModeType() const
    {
        return getPlatform() >= XE_HP ? Type_BF : Type_HF;
    }

    // each flag register is 16-bit
    uint32_t getNumFlagRegisters(void)
    {
        return 4;
    }

    bool encodeAccWrEn() const
    {
        return true;
    }

    SFID getEOTSFID() const
    {
        return SFID::SPAWNER;
    }

    // EU native execution size for 32-bit types
    G4_ExecSize getNativeExecSize() const
    {
        return g4::SIMD8;
    }

    bool noScalarByteToFloat() const
    {
        return getPlatform() >= XE_HP;
    }

    bool useAccForDF() const
    {
        return getPlatform() >= XE_HP;
    }

    bool useAccForMadm() const
    {
        return getPlatform() >= XE_HP;
    }

    bool hasUnifiedBarrier() const
    {
        return false;
    }

    bool predCtrlHasWidth() const
    {
        return true;
    }

    bool hasNibCtrl() const
    {
        return true;
    }

    bool hasMaskForScratchMsg() const
    {
        return true;
    }

    bool avoidDstSrcOverlap()
    {
        return getPlatform() >= GENX_ICLLP && getOption(vISA_DstSrcOverlapWA);
    }

    bool hasTwoGRFMathMacro() const
    {
        return false;
    }

    bool hasFP64Acc() const
    {
        return !noFP64();
    }

    bool hasMathAcc() const
    {
        return getOption(vISA_mathAccSub);
    }

    bool hasSrc2Acc() const
    {
        return getOption(vISA_src2AccSub);
    }

    bool hasHFMathGRFAlign() const
    {
        return false;
    }

    bool hasFixedCycleMathPipeline() const
    {
        return false;
    }

    bool hasByteALU() const
    {
        return true;
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
        return true;
    }


    int get3SrcDstAlign() const
    {
        return getPlatform() >= XE_HP ? 2 : 8;
    }

    bool supportCallaRegSrc() const
    {
        return false;
    }


    bool hasTwoGRFBank16Bundles() const
    {
        return getPlatform() == XE_HP;
    }

    bool hasOneGRFBank16Bundles() const
    {
        return getPlatform() != XE_HP;
    }

    bool hasDPASSrc0Src1BankConflict() const
    {
        return getPlatform() >= XE_HP;
    }

    bool needsToLoadCrossThreadConstantData() const
    {
        return loadThreadPayload() && getOption(vISA_loadCrossThreadConstantData);
    }

    bool hasDPAS() const
    {
        return getPlatform() >= XE_HP;
    }

    bool hasAMFSFastClear() const
    {
        return getPlatform() >= XE_HP;
    }

    // only valid type combinations during format mov conversion if acc is used:
    // Src Type UD/D - Dst Type W/UW/UD/D
    // Src Type UW/W - Dst Type W/UW/UD/D
    // Src Type F/HF - Dst Type F/HF
    // Src Type DF   - Dst Type DF
    bool hasFormatConversionACCRestrictions() const
    {
        return getPlatform() >= XE_HP;
    }

    bool hasScratchSurface() const
    {
        return getPlatform() >= XE_HP;
    }

    // Note that this function is intentionally omitted from HWCapsOpen.inc to avoid IP leak
    bool hasThreeALUPipes() const
    {
        return (getPlatform() == XE_HP);
    }

    bool hasFusedEU() const
    {
        return (getPlatform() == GENX_TGLLP || getPlatform() == XE_HP);
    }

    bool hasFourALUPipes() const
    {
        return false;
    }

    bool hasSrc2ReadSupression() const
    {
        return false;
    }

    bool hasSrc2ReadSupressionSameRegSameType() const
    {
        return false;
    }

    bool hasFixedCycleMathPipe() const
    {
        return false;
    }

     bool hasBFDstforDPAS() const
    {
        return false;
    }

    bool hasHWordBlockLoad() const
    {
        return false;
    }
    bool hasPartialInt64Support() const
    {
        return false;
    }


   bool hasEOTWait() const
   {
        return true;
   }
   bool noMulExpandingBeforeScheduler()
   {
        return false;
   }
   bool hasA0WARHWissue()
   {
       return (getPlatform() == XE_HP);
   }

   bool hasFtoPackedHFMove() const
   {
      return false;
   }
   // end HW capabilities
