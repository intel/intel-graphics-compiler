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
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN11 || isGen12LP();
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
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
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
        return getPlatformGeneration(getPlatform()) < PlatformGen::GEN12;
    }

    bool hasFdivPowWA() const
    {
        return getPlatform() < GENX_ICLLP;
    }

    bool hasCondModForTernary() const
    {
        return getPlatformGeneration(getPlatform()) < PlatformGen::GEN12;
    }

    bool hasSWSB() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
    }

    bool hasPartialMixMode() const
    {
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN12;
    }

    // whether EOT sources need to be assigned r112-r127
    bool hasEOTGRFBinding() const
    {
        return true;
    }

    bool hasSmov() const
    {
        return getPlatformGeneration(getPlatform()) < PlatformGen::GEN12;
    }

    bool doAccSub() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN11;
    }

    bool hasNFType() const
    {
        return getPlatform() >= GENX_ICLLP &&
               getPlatformGeneration(getPlatform()) != PlatformGen::GEN12;
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
        return getPlatformGeneration(getPlatform()) < PlatformGen::GEN12 &&
               getOption(vISA_WAWSubregHazardAvoidance);
    }

    bool newTernaryStride() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
    }

    // acc restrictions that are relaxed for gen12
    // -- dp4a can take acc src/dst
    // -- the same acc may be used in both src0 and src1 of a three-source inst
    bool relaxedACCRestrictions2() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
    }

    bool fuseURBMessage() const
    {
        return isGen12LP();
    }

    bool supportSrcModforMul() const
    {
        return getPlatformGeneration(getPlatform()) != PlatformGen::GEN12;
    }

    bool doMultiAccSub() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
    }

    bool canMadHaveSrc0Acc() const
    {
        return getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
    }

    bool accDstforIndirectSrc() const
    {
        return getPlatformGeneration(getPlatform()) != PlatformGen::GEN12;
    }

    bool favorFpMad() const
    {
        return getPlatform() < GENX_CNL;
    }

    bool avoidAccDstWithIndirectSource() const
    {
        return getPlatform() >= GENX_TGLLP;
    }

    bool hasReadSuppression() const
    {
        return getPlatform() >= GENX_TGLLP;
    }

    bool hasScalarReadSuppression() const
    {
        return getPlatform() > GENX_TGLLP;
    }

    bool alignBindlessSampler() const
    {
        return getPlatformGeneration(getPlatform()) == PlatformGen::GEN9 && getOption(vISA_alignBindlessSampler);
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
        return (GEN >= PlatformGen::GEN12) ? 1 : 2;
#else
        return 2;
#endif
    }

    bool useMultiThreadLatency() const
    {
#if 0
        auto GEN = getPlatformGeneration(getPlatform());
        if (GEN >= PlatformGen::GEN12)
            return false;
#endif

        return getOptions()->getOption(vISA_useMultiThreadedLatencies);
    }

    bool useIGAEncoder() const
    {
        return getOption(vISA_IGAEncoder) || getPlatformGeneration(getPlatform()) >= PlatformGen::GEN12;
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

    bool isGen12LP() const
    {
        return getPlatform() == GENX_TGLLP;
    }

    bool hasEarlyGRFRead() const
    {
        return (getPlatform() == GENX_TGLLP) && getOption(vISA_HasEarlyGRFRead);
    }


    bool noInt64() const
    {
        return getPlatform() == GENX_ICLLP || isGen12LP();
    }

    bool noFP64() const
    {
        return getPlatform() == GENX_ICLLP || isGen12LP();
    }

    bool no64bitRegioning() const
    {
        return getPlatform() == GENX_CHV || getPlatform() == GENX_BXT ||
               getPlatform() == GENX_ICLLP || isGen12LP();
    }

    bool hasBankCollision() const
    {
        return (getPlatformGeneration(getPlatform()) <= PlatformGen::GEN11) ||
        (getOption(vISA_enableBCR) && !hasEarlyGRFRead());
    }

    bool oneGRFBankDivision() const
    {
        return true;
    }

    bool hasMadm() const
    {
        return getPlatform() != GENX_ICLLP && !isGen12LP();
    }

    bool hasSIMD16TypedRW() const
    {
        return false;
    }

    bool doNotRewriteContiguousRegion() const
    {
        return false;
    }

    bool relaxedACCRestrictions() const
    {
        return false;
    }

    bool loadThreadPayload() const
    {
        return false;
    }

    bool needFenceBeforeEOT() const
    {
        return false;
    }

    bool hasSrc0ReadSuppression() const
    {
        return false;
    }

    bool needToClearScratchWrites() const
    {
        return true;
    }

    bool needsToLoadLocalID() const
    {
        return false;
    }

    bool noDwDstForDwordMul() const
    {
        return noInt64();
    }

    bool useNewExtDescFormat() const
    {
        return false;
    }

    bool has16OWordSLMBlockRW() const
    {
        return false;
    }

    bool hasVxHFloat64b() const
    {
        return true;
    }

    bool supportFloatOr64bRegioning() const
    {
        return true;
    }

    int getFP64MadmExecSize() const
    {
        return 4;
    }

    bool balanceIntFloatMoves() const
    {
        return false;
    }

    G4_Type getMixModeType() const
    {
        return Type_HF;
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

    uint32_t getNativeExecSize() const
    {
        return 8;
    }

    bool noScalarByteToFloat() const
    {
        return false;
    }

    bool useAccForDF() const
    {
        return false;
    }

    bool useAccForMadm() const
    {
        return false;
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
        return false;
    }

    bool hasTwoGRFMathMacro() const
    {
        return false;
    }

    bool hasFP64Acc() const
    {
        return !noFP64();
    }


    // end HW capabilities
