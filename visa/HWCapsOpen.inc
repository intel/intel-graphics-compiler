#pragma once
    // HW Capabilities for open source branch (currently Gen9 and below)
    // this are part of IR_Builder class and include in Build_IR.h

    enum class PlatformGen
    {
        GEN_UNKNOWN = 0,
        GEN8 = 8,
        GEN9 = 9,
        GEN10 = 10
    };

    PlatformGen getPlatformGeneration(TARGET_PLATFORM platform) const 
    {
      switch (platform)
      {
          case GENX_BDW:
          case GENX_CHV:
            return PlatformGen::GEN8;
          case GENX_SKL:
          case GENX_BXT:
            return PlatformGen::GEN9;
          case GENX_CNL:
            return PlatformGen::GEN10;
          default:
            assert(false && "unsupported platform");
            return PlatformGen::GEN_UNKNOWN;
        }
    }

    bool hasMixMode() const
    {
        return (getGenxPlatform() > GENX_BDW && !getOption(vISA_DisableMixMode));
    }

    bool canDoSLMSpill() const
    {
        return getOption(vISA_SLMSpill) &&
            !(kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc());
    }

    bool forceSamplerHeader() const
    {
        return m_options->getOption(vISA_forceSamplerHeader) ||
            m_options->getOption(vISA_enablePreemption);
    }

    bool needsNoPreemptR2ForSend() const 
    {
        return false;
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
        return false;
    }

    bool doPlane() const
    {
        return !getOption(vISA_expandPlane);
    }

    bool favorFloatMov() const
    {
        return false;
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
        return false;
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
        return false;
    }

    bool no64bitRegioning() const
    {
        return getGenxPlatform() == GENX_CHV || getGenxPlatform() == GENX_BXT;
    }

    bool noSrc1Byte() const
    {
        return getOption(vISA_noSrc1Byte);
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
        return true;
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
        return false;
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
        return true;
    }

    bool lowHighBundle() const
    {
        return true;
    }

    bool hasBankCollision() const
    {
        return true;
    }

    bool hasSendShootdown() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool useNewR0Format() const
    {
        return false;
    }

    int getPredMaskEnableBit() const
    {
        return 30;
    }

    int getBarrierIDMask() const
    {
        return 0x8F000000;
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
        return false;
    }

    bool hasDotProductInst() const
    {
        return getGenxPlatform() < GENX_CNL;
    }

    bool hasLRP() const
    {
        return true;
    }

    bool hasMadm() const
    {
        return true;
    }

    int getBarrierMask(bool enableBarrierInstCounterBits) const
    {
        if (getGenxPlatform() < GENX_SKL)
        {
            //pre-SKL: and (8) H0.0:ud r0.2:ud 0x0F000000 (r0.2, bit 24-27)
            return enableBarrierInstCounterBits ? 0x0F00FE00 : 0x0F000000;
        }
        else
        {
            //SKL+: and (8) H0.0:ud r0.2:ud 0x8F000000 (r0.2, bit24-27, bit31)
            return enableBarrierInstCounterBits ? 0x8F00FE00 : 0x8F000000;
        }
    }

    bool canMadHaveAcc() const
    {
        return getGenxPlatform() >= GENX_CNL;
    }

    bool hasFdivPow() const
    {
        return true;
    }

    bool hasFdivPowWA() const
    {
        return true;
    }

    bool hasCondModForTernary() const
    {
        return true;
    }

    bool hasPartialMixMode() const
    {
        return false;
    }

    // whether EOT sources need to be assigned r112-r127
    bool hasEOTGRFBinding() const
    {
        return getOptions()->getuInt32Option(vISA_TotalGRFNum) == 128;
    }

    bool hasSmov() const
    {
        return true;
    }

    bool doAccSub() const
    {
        return false;
    }

    bool hasNFType() const
    {
        return false;
    }

    void getSSIDBits(uint32_t& width, uint32_t& start) const
    {
        if (getGenxPlatform() < GENX_CNL)
        {
            width = 2;
            start = 12; //[12:13]
        }
        else
        {
            width = 2;
            start = 8; //[8:9]    
        }
    }

    bool encodeAccRegSelAsAlign1() const
    {
        return false;
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
        return getOption(vISA_WAWSubregHazardAvoidance);
    }

    bool doNotRewriteRegion() const
    {
        return false;
    }

    bool relaxedACCRestrictions() const
    {
        return false;
    }

    // 1 means one SIMD16 float accumulator
    uint32_t getNumACC() const
    {
        return 1;
    }

    bool fuseURBMessage() const
    {
        return false;
    }

    bool loadThreadPayload() const
    {
        return false;
    }

    bool supportSrcModforMul() const
    {
        return true;
    }

    // end HW capabilities
