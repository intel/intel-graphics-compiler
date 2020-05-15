#pragma once
    // HW Capabilities
    // this are part of IR_Builder class and include in Build_IR.h

    bool hasMixMode() const
    {
        return getOption(vISA_ForceMixMode) ||
            (getGenxPlatform() > GENX_BDW &&
            getPlatformGeneration(getGenxPlatform()) != PlatformGen::GEN11 && !getOption(vISA_DisableMixMode));
    }

    bool forceSamplerHeader() const
    {
        return m_options->getOption(vISA_forceSamplerHeader) ||
            (getGenxPlatform() < GENX_ICL && m_options->getOption(vISA_enablePreemption));
    }

    bool needsNoPreemptR2ForSend() const
    {
        return getPlatformGeneration(getGenxPlatform()) == PlatformGen::GEN11;
    }

    bool noDDAllowedPlatform() const
    {
        return getGenxPlatform() >= GENX_SKL;
    }

    bool noSrcDepSetAllowedPlatform() const
    {
        return getGenxPlatform() >= GENX_SKL;
    }

    bool no64bitType() const
    {
        return getGenxPlatform() == GENX_ICLLP || isGen12LP();
    }

    bool doPlane() const
    {
        return getGenxPlatform() < GENX_ICL && !getOption(vISA_expandPlane);
    }

    bool favorFloatMov() const
    {
        return getPlatformGeneration(getGenxPlatform()) == PlatformGen::GEN11 || isGen12LP();
    }

    bool noScalarJmp() const
    {
        return !getOption(vISA_EnableScalarJmp);
    }

    bool hasAlign1Ternary() const
    {
        return getGenxPlatform() >= GENX_CNL && getOption(vISA_doAlign1Ternary);
    }

    bool encodeUnitStrideTernary() const
    {
        return getPlatformGeneration(getGenxPlatform()) >= PlatformGen::GEN12;
    }

    bool hasMacl() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool hasCPS() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool hasBindlessSampler() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool noSrc2Regioning() const
    {
        return getGenxPlatform() >= GENX_ICL;
    }

    bool no64bitRegioning() const
    {
        return getGenxPlatform() == GENX_CHV || getGenxPlatform() == GENX_BXT ||
               getGenxPlatform() == GENX_ICLLP || isGen12LP();
    }

    bool noSrc1Byte() const
    {
        return getOption(vISA_noSrc1Byte) || getGenxPlatform() >= GENX_ICL;
    }

    bool needsFenceCommitEnable() const
    {
        if (m_options->getTarget() == VISA_CM)
        {
            return getGenxPlatform() >= GENX_SKL;
        }
        else
        {
            return getGenxPlatform() >= GENX_CNL;
        }
    }

    bool hasIEEEDivSqrt() const
    {
        return getGenxPlatform() < GENX_ICL;
    }

    bool gotoJumpOnTrue() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool needsToReserveR127() const
    {
        return getGenxPlatform() < GENX_CNL;
    }

    bool hasSLMFence() const
    {
        return getGenxPlatform() >= GENX_ICL;
    }

    bool GRFAlign() const
    {
        return getGenxPlatform() < GENX_SKL;
    }

    bool twoSourcesCollision() const
    {
        return getGenxPlatform() < GENX_CNL;
    }

    bool oneGRFBankDivision() const
    {
        return getGenxPlatform() != GENX_ICL;
    }

    bool lowHighBundle() const
    {
        return getPlatformGeneration(getGenxPlatform()) <= PlatformGen::GEN11;
    }

    bool hasBankCollision() const
    {
        return getPlatformGeneration(getGenxPlatform()) <= PlatformGen::GEN11 || (getOption(vISA_enableBCR) && !(getGenxPlatform() == GENX_TGLLP && GetStepping() == Step_A));
    }

    bool hasSendShootdown() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool useNewR0Format() const
    {
        return getGenxPlatform() >= GENX_ICL;
    }

    int getPredMaskEnableBit() const
    {
        return getGenxPlatform() < GENX_ICL ? 30 : 23;
    }

    int getBarrierIDMask() const
    {
        return getGenxPlatform() < GENX_ICL ? 0x8F000000 : 0x7F000000;
    }

    uint32_t getMaxSendMessageLength() const
    {
        return getGenxPlatform() < GENX_CNL ? 16 : 32;
    }

    bool hasPixelNullMask() const
    {
        return getGenxPlatform() >= GENX_SKL;
    }

    bool noSLMMsgHeader() const
    {
        return getGenxPlatform() >= GENX_SKL;
    }

    bool needsA32MsgHeader() const
    {
        return getGenxPlatform() < GENX_SKL;
    }

    int getThreadIDMask() const
    {
        return getGenxPlatform() >= GENX_SKL ? 0x7FF : 0x1FF;
    }

    bool hasFloatAtomics() const
    {
        return getGenxPlatform() >= GENX_SKL;
    }

    bool hasBlockedSLMMessage() const
    {
        return false;
    }

    bool hasHeaderlessMRTWrite() const
    {
        return getGenxPlatform() >= GENX_ICLLP;
    }

    bool hasDotProductInst() const
    {
        return getGenxPlatform() < GENX_CNL;
    }

    bool hasLRP() const
    {
        return getGenxPlatform() < GENX_ICL;
    }

    bool hasMadm() const
    {
        return getGenxPlatform() != GENX_ICLLP && !isGen12LP();
    }

    int getBarrierMask(bool enableBarrierInstCounterBits) const
    {
        if (getGenxPlatform() < GENX_SKL)
        {
            //pre-SKL: and (8) H0.0:ud r0.2:ud 0x0F000000 (r0.2, bit 24-27)
            return enableBarrierInstCounterBits ? 0x0F00FE00 : 0x0F000000;
        }
        else if (getGenxPlatform() < GENX_ICL)
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
        return getGenxPlatform() >= GENX_CNL;
    }

    bool hasFdivPow() const
    {
        return getPlatformGeneration(getGenxPlatform()) < PlatformGen::GEN12;
    }

    bool hasFdivPowWA() const
    {
        return getGenxPlatform() < GENX_ICL;
    }

    bool hasCondModForTernary() const
    {
        return getPlatformGeneration(getGenxPlatform()) < PlatformGen::GEN12;
    }

    bool hasSWSB() const
    {
        return getGenxPlatform() >= GENX_TGLLP;
    }

    bool hasPartialMixMode() const
    {
        return getGenxPlatform() >= GENX_TGLLP;
    }

    // whether EOT sources need to be assigned r112-r127
    bool hasEOTGRFBinding() const
    {
        return true;
    }

    bool hasSmov() const
    {
        return getGenxPlatform() < GENX_TGLLP;
    }

    bool doAccSub() const
    {
        return getPlatformGeneration(getGenxPlatform()) >= PlatformGen::GEN11;
    }

    bool hasNFType() const
    {
        return getGenxPlatform() == GENX_ICL || getGenxPlatform() == GENX_ICLLP;
    }

    void getSSIDBits(uint32_t& width, uint32_t& start) const
    {
        if (getGenxPlatform() < GENX_CNL)
        {
            width = 2;
            start = 12; //[12:13]
        }
        else if (getGenxPlatform() == GENX_CNL || getGenxPlatform() == GENX_ICL)
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
        return getGenxPlatform() >= GENX_ICL;
    }

    bool fuseTypedWrites() const
    {
        return getOption(vISA_FuseTypedWrites);
    }

    bool hasSIMD16TypedRW() const
    {
        return false;
    }

    bool avoidWAWSubRegHazard() const
    {
        return (getGenxPlatform() < GENX_TGLLP) && getOption(vISA_WAWSubregHazardAvoidance);
    }

    bool newTernaryStride() const
    {
        return getGenxPlatform() >= GENX_TGLLP;
    }

    bool doNotRewriteContiguousRegion() const
    {
        return false;
    }

    bool relaxedACCRestrictions() const
    {
        return false;
    }

    bool relaxedACCRestrictions2() const
    {
        return getGenxPlatform() >= GENX_TGLLP;
    }

    bool fuseURBMessage() const
    {
        return isGen12LP();
    }

    bool loadThreadPayload() const
    {
        return false;
    }

    bool supportSrcModforMul() const
    {
        return getPlatformGeneration(getGenxPlatform()) != PlatformGen::GEN12;
    }

    bool doMultiAccSub() const
    {
        return getPlatformGeneration(getGenxPlatform()) >= PlatformGen::GEN12;
    }

    bool canMadHaveSrc0Acc() const
    {
        return getPlatformGeneration(getGenxPlatform()) >= PlatformGen::GEN12;
    }

    bool accDstforIndirectSrc() const
    {
        return getPlatformGeneration(getGenxPlatform()) != PlatformGen::GEN12;
    }

    bool needFenceBeforeEOT() const
    {
        return false;
    }

    bool favorFpMad() const
    {
        return getGenxPlatform() < GENX_CNL;
    }

    bool hasReadSuppression() const
    {
        return getGenxPlatform() == GENX_TGLLP;
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
        return no64bitType();
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

    bool alignBindlessSampler() const
    {
        return getGenxPlatform() == GENX_SKL && getOption(vISA_alignBindlessSampler);
    }


    unsigned getHWThreadNumberPerEU() const
    {
        unsigned Val = getOptions()->getuInt32Option(vISA_HWThreadNumberPerEU);
        return (Val != 0) ? Val : 7;
    }

    unsigned getCoIssueUints() const
    {
        return 2;
    }

    bool useMultiThreadLatency() const
    {
        return getOptions()->getOption(vISA_useMultiThreadedLatencies);
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

    bool canEncodeFullExtDesc() const
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

    bool isGen12LP() const
    {
        return getGenxPlatform() == GENX_TGLLP;
    }

    bool noL3Flush() const
    {
        return getGenxPlatform() == GENX_TGLLP;
    }

    bool needResetA0forVxHA0() const
    {
        return getGenxPlatform() >= GENX_TGLLP;
    }

    bool predCtrlHasWidth() const
    {
        return true;
    }
    // end HW capabilities
