/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/


#include "wa_def.h"

#define SIWA_ONLY_BDW_A0    SIWA_ONLY_A0
#define SIWA_UNTIL_BDW_A0   SIWA_UNTIL_A0
#define SIWA_FROM_BDW_A0    SIWA_FROM_A0
#define SIWA_AFTER_BDW_A0   SIWA_AFTER_A0

#define SIWA_ONLY_BDW_A1    SIWA_ONLY_A0
#define SIWA_UNTIL_BDW_A1   SIWA_UNTIL_A0
#define SIWA_FROM_BDW_A1    SIWA_FROM_A0
#define SIWA_AFTER_BDW_A1   SIWA_AFTER_A0


#define SIWA_ONLY_BDW_B0    SIWA_ONLY_A3
#define SIWA_UNTIL_BDW_B0   SIWA_UNTIL_A3
#define SIWA_FROM_BDW_B0    SIWA_FROM_A3
#define SIWA_AFTER_BDW_B0   SIWA_AFTER_A3



#define SIWA_ONLY_BDW_D0    SIWA_ONLY_A4
#define SIWA_UNTIL_BDW_D0   SIWA_UNTIL_A4
#define SIWA_FROM_BDW_D0    SIWA_FROM_A4
#define SIWA_AFTER_BDW_D0   SIWA_AFTER_A4

#define SIWA_ONLY_BDW_E0    SIWA_ONLY_A5
#define SIWA_UNTIL_BDW_E0   SIWA_UNTIL_A5
#define SIWA_FROM_BDW_E0    SIWA_FROM_A5
#define SIWA_AFTER_BDW_E0   SIWA_AFTER_A5

#define SIWA_ONLY_BDW_F0    SIWA_ONLY_A6
#define SIWA_UNTIL_BDW_F0   SIWA_UNTIL_A6
#define SIWA_FROM_BDW_F0    SIWA_FROM_A6
#define SIWA_AFTER_BDW_F0   SIWA_AFTER_A6

#define SIWA_ONLY_BDW_G0    SIWA_ONLY_A7
#define SIWA_UNTIL_BDW_G0   SIWA_UNTIL_A7
#define SIWA_FROM_BDW_G0    SIWA_FROM_A7
#define SIWA_AFTER_BDW_G0   SIWA_AFTER_A7

#define SIWA_ONLY_BDW_G1    SIWA_ONLY_A8
#define SIWA_UNTIL_BDW_G1   SIWA_UNTIL_A8
#define SIWA_FROM_BDW_G1    SIWA_FROM_A8
#define SIWA_AFTER_BDW_G1   SIWA_AFTER_A8

#define SIWA_ONLY_BDW_H0    SIWA_ONLY_A9
#define SIWA_UNTIL_BDW_H0   SIWA_UNTIL_A9
#define SIWA_FROM_BDW_H0    SIWA_FROM_A9
#define SIWA_AFTER_BDW_H0   SIWA_AFTER_A9

#define SIWA_ONLY_BDW_J0    SIWA_ONLY_AA
#define SIWA_UNTIL_BDW_J0   SIWA_UNTIL_AA
#define SIWA_FROM_BDW_J0    SIWA_FROM_AA
#define SIWA_AFTER_BDW_J0   SIWA_AFTER_AA







#define SIWA_ONLY_PCH_BDW_A0      SIWA_ONLY_A0
#define SIWA_FROM_PCH_BDW_A0      SIWA_FROM_A0
#define SIWA_UNTIL_PCH_BDW_A0     SIWA_UNTIL_A0
#define SIWA_AFTER_PCH_BDW_A0     SIWA_AFTER_A0




void InitBdwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_BDW, ulStepId_PCH;
    int platformForIndirectDispatch = SI_WA_NEVER;

#ifdef __KCH
    KCHASSERT(NULL != pWaParam);
#endif
    ulStepId_BDW = (1 << pWaParam->usRevId); 
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH); 

    
    
    
    
    
    WA_ENABLE(
        ulStepId_BDW,
        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaL3WriteIncomplete,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        Wa4x4STCOptimizationDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaMSFAfterWalkerWithoutSLMorBarriers,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_BDW,
        WaClearRenderResponseMasks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaNoMMIOWhenPGOff,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_BDW,
        WaNoMinimizedTrivialSurfacePadding,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableSIMD32PixelShaderDispatchFor2xMSAA,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WAInsertNOPBetweenMathPOWDIVAnd2RegInstr,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
        
    WA_ENABLE(
        ulStepId_BDW,
        WANOPBeetweenIndirectAdressingAndBranch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaFlushWriteCachesOnMultisampleChange,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaAvoidPMAStall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableRSBeforeBTPoolDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDividePSInvocationCountBy4,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaHizAmbiguateRequiredNonAlignedBeforeRender,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaA32StatelessMessagesRequireHeader,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaNoA32ByteScatteredStatelessMessages,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaGuardbandSize,
        "No Link Provided" ,
        "Unavailable",
        PLATFORM_ALL,
        SIWA_FOREVER);

    if( pSkuTable->FtrGT3 )
    {
        WA_ENABLE(
            ulStepId_BDW,
            WaSendLRITwice,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_BDW_A1);
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaRsNullConstantBuffer,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaL3UseSamplerForVectorLoadScatter,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableSamplerRoundingDisableFix,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableLockForTranscodePerf,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaAddMediaStateFlushCmd,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaAvcCabacDistortionBasedMinQp,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaVC1UnequalFieldHeights,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDummyDMVBufferForMVCInterviewPred,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaJPEGHeightAlignYUV422H2YToNV12,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaInsertAVCFrameForFormatSwitchToJPEG,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaUseVP8DecodePrivateInputBuffer,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaVC1DecodingMaxResolution,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaAddVC1StuffingBytesForSPMP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaParseVC1PicHeaderInSlice,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaParseVC1FirstFieldPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaParseVC1BPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

#ifdef WIN32
    WA_ENABLE(
        ulStepId_BDW,
        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
#endif

    
    WA_ENABLE(
        ulStepId_BDW,
        WaIs64BInstrEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableGroupIDLoopSelect,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_E0);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaUseVAlign16OnTileXYBpp816,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        Wa32bitGeneralStateOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_BDW,
        WaL3IACoherencyRequiresIoMmu,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaL3ParitySupportDisable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaL3ParityInterruptUnmask,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceTypeConvertF32To16ToAlign1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaAdditionalMovWhenSrc1ModOnMulMach,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaCallForcesThreadSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaThreadSwitchAfterCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaScalarAtomic,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceNonZeroSBEOutputAttributeCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    

    WA_ENABLE(
        ulStepId_BDW,
        WaNullVertexBufferWhenZeroSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaCRCDisabledForMBO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G0);

     WA_ENABLE(
        ulStepId_BDW,
        WaPsrSfuMaskSprite,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_BDW,
        WaIsAudioControllerinLPSPWell,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaPortWriteforAudioControllerinBIOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForBlackFlashInMBOMode,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaScoreboardStallBeforeStateCacheInvalidate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisablePrimaryFlipsForMBO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G0);

    WA_ENABLE(
        ulStepId_BDW,
        WaExtIotlbInvHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaIommuCCInvalidationHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableIommuAccessedDirtyBit,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaOCLDisableA64Messages,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaCsStallBeforeStateCacheInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableBwgTlbClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaVSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaHalfFloatSelNotAllowedWithSourceModifiers,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaSpriteYUVOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableRowChickenDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaLimitMaxHSUrbHandles,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaSbeConservativeCacheMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaNearestFilterLODClamp,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaPipelineFlushCoherentLines,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableAuxCCSForMRTBlend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaUrbAtomics,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaBasicCompilationForDPInstructions,
        "This is not a HW or SW bug.",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1 );

    WA_ENABLE(
        ulStepId_BDW,
        WaFlushOpOnCSStall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIndirectDispatchPredicate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisablePreemptionMMIOWhenBarrierEnabled,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaTSGStarvation,
        "No Link Provided" ,
        "HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_E0);

    WA_ENABLE(
        ulStepId_BDW,
        WaInhibitPreemptionForOCLProfiling,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaMediaStateFlushBeforePipeControl,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaOCLEnableFMaxFMinPlusZero,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableDSCaching,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaSubtract1FromMaxNoOfThreads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaCSScratchSpaceSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableTileWInRcc,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaEmitVtxWhenOutVtxCntIsZero,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceToNonPrivFifthRegisterNonFunctional,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaCsStallBeforeURBVS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisable4x2SubSpanOptimizationForDS,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableIndirectDataForGPGPUWalker,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableIndirectDataForIndirectDispatch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        platformForIndirectDispatch);

    WA_ENABLE(
        ulStepId_BDW,
        WaUseNonPrivRegisterForObjectLevelPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemptionSequentialMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemptionForDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_A0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemptionForGSLineStripAdj,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemptionForInstancedDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableNullDepthBuffer,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_E0);

    WA_ENABLE(
        ulStepId_BDW,
        WaSendMediaStateFlushAfterGPGPUWalker,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisablePushConstantHSGS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaEnableAllWriteChannelMask,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaGather4WithGreenChannelSelectOnR32G32Float,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaLimitSizeOfSDEPolyFifo,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    if(pWaTable->WaLimitSizeOfSDEPolyFifo)
    {
        WA_ENABLE(
            ulStepId_BDW,
            WaDisableEarlyEOT,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_BDW_A1);
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableSamplerPowerBypass,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaInsertNOPViaChickenBit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaBlockMsgChannelDuringGfxReset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        DisableClockGatingForGucClocks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisablePerfMonGathering,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaCsStallBeforeEnabling3DstateHS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaCsStallBeforeNonZeroInstanceCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaCsStallBefore3DStateVS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaSendMiRsStoreDataImmBeforeRsSyncCommand,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaGrfDepClearOnOutstandingSamplerInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaStoreAcc2to9InAlign16InGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER)    

    WA_ENABLE(
        ulStepId_BDW,
        WaGrfDependecyClearInGpgpuContextRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaRestoreFc0RegistersWithOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRestoreFC4RegisterDW0fromDW1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRestoreFCandMSGRegistersFromUpperOword,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaGrfScoreboardClearInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaClearFlowControlGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaClearCr0SpfInGpgpuContextRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableDbg0Register,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaFlushBefore3DStateConstant,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaSendExtraRSGatherConstantAndRSStoreImmCmds,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableNonSlm,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaClearNotificationRegInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemptionDuringUAVDrawCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaEnsureMemCoherencyBeforeLoadRegisterMem,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaClearCCStatePriorPipelineSelect,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaSendDummyVFEafterPipelineSelect,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER); 

    WA_ENABLE(
        ulStepId_BDW,
        WaClearSlmSpaceAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaFlushCoherentL3CacheLinesAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableCtxRestoreArbitration,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    if (pSkuTable->FtrGT3)
    {


        WA_ENABLE(
            ulStepId_BDW,
            WaPerCtxtBbInvalidateRoCaches,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_BDW_E0);
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaGttCachingOffByDefault,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    
    if( pSkuTable->FtrGpGpuMidThreadLevelPreempt )
    {
        WA_ENABLE(
            ulStepId_BDW,
            WAGPGPUMidThreadPreemption,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_BDW_G0);
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableWaBBUse,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableFenceDestinationToSLM,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_E0|SIWA_ONLY_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableLSQCROPERFforOCL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    WA_ENABLE(
        ulStepId_BDW,
        WaSetPipeControlCSStallforGPGPUAndMediaWorkloads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaAtomicsForceCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableSDEUnitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceMinMaxGSThreadCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        AccWrEnNotAllowedToAcc1With16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaSetDepthToArraySizeForUAV,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaSendPipeControlWithProtectedMemoryDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_BDW,
        WaAvoid16KWidthForTiledSurfaces,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
        
    

    
    WA_ENABLE(
        ulStepId_BDW,
        WaFbcAsynchFlipDisableFbcQueue,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaFbcExceedCdClockThreshold,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G0);

    WA_ENABLE(
        ulStepId_BDW,
        WaFdiRxMiscProgramming,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaFbcDisableRleCompressionForVTD,
        "NO HWBugLink Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaFbcInvalidateCompressedLines,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableIPS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_E0);

    WA_ENABLE(
        ulStepId_BDW,
        WaIpsWaitForPcodeOnDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIpsDisableOnPaletteAccess,
        "No HWBugLink provided",
        "No Link Provided" , 
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIpsDisableOnAsyncFlips,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIpsDisableOnCdClockThreshold,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaRsConsecutiveOptimizedWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaRsForceSingleThreadFW,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRsUseTimeoutMode,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRsClearFWBitsAtFLR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    
    WA_ENABLE(
        ulStepId_BDW,
        WaSyncSameMMIORegAccess,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaGsvHighRingFreqScalingForUlt,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_E0);

    WA_ENABLE(
        ulStepId_BDW,
        WaCSNoWayToLimitScratchSpace,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaStateBindingTableOverfetch,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaProgramMgsrForCorrectSliceSpecificMmioReads,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaUAVCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableBcCentroidPerformanceOptimization,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    WA_ENABLE(
        ulStepId_PCH,
        WaMPhyProgramming,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaB0ParityHangDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaFDIAutoLinkSetTimingOverrride,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaS3DSoftwareMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForcePreemptWaitForIdleOnNonRcsEngines,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaVeboxSliceEnable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaPreventHSTessLevelsInterference,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaAvoidDomainShaderCacheStall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIncreaseTagClockTimer,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableGpGpuPreemptOnGt1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_A1);

    WA_ENABLE(
        ulStepId_BDW,
        WaUseGfxModeFor3DPreemptionGranularity,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaClearArfDependenciesBeforeEot,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableThreadStallDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableInstructionShootdown,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisablePartialInstShootdown,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaEnableForceWakeForRc6WaBb,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableMADMMacros,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    
    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {
        WA_ENABLE(
            ulStepId_BDW,
            WADP0ClockGatingDisable,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaDuplicateMiDisplayFlip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableLiteRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaHdcDisableFetchWhenMasked,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaSetVfGuardbandPreemptionVertexCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableTdsClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaPsrDPRSUnmaskVBlankInSRD,
        "No HWBug is filed yet ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaPsrDPAMaskVBlankInSRD,
        "No HWBug is filed yet ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableCoherency,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_G1);

    WA_ENABLE(
        ulStepId_BDW,
        WaEnableCoherencyHWFixes,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_BDW_H0);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceEnableNonCoherent,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaCselUnsupported,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaPipeControlUpperDwordCorruption,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableNativeDWxDWMultiplication,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_D0);

    if( pSkuTable->FtrIoMmuPageFaulting )
    {
        WA_ENABLE( 
            ulStepId_BDW, 
            WADisableWriteCommitForPageFault, 
            "No Link Provided" ,
            "No HWSightingLink provided", 
            PLATFORM_ALL, 
            SIWA_FOREVER );
    }

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableFfDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaSkipInvalidSubmitsFromOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaLimitSqIdiCounterToEleven,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaC6DisallowByGfxPause,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_AFTER_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableMinuteIaClockGating,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceL3LlcCoherencyInDescriptor,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreempt,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaEngineResetAfterMidThreadPreemption,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaModifyVFEStateAfterGPGPUPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemtionForVertexCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableObjectLevelPreemtionForInstanceId,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_F0);

    WA_ENABLE(
        ulStepId_BDW,
        WaIntegerDivisionSourceModifierNotSupported,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaDisableMidThreadPreempt,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_B0);

    WA_ENABLE(
        ulStepId_BDW,
        WaAllocateSLML3CacheCtrlOverride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_BDW_D0);

    WA_ENABLE(
        ulStepId_BDW,
        WaProgramL3SqcReg1Default,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaOGLGSVertexReordering,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceDX10BorderColorSampleC,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceDX10BorderColorFor64BPTTextures,
        "No Link Provided" ,
        "No HWSightingLink Provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRsRestoreWithPerCtxtBb,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaVfPostSyncWrite,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceNullSurfaceTileY,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaPartitionLevelClockGatingDisable,
       "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaForceContextSaveRestoreNonCoherent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE_NO_PLATFORM_CHECK(
        ulStepId_BDW,
        WaPruneModeWithIncorrectHsyncOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaEnableHGAsyncFlipLinearToTileConvert,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaTempDisableDOPClkGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIOBAddressMustBeValidInHwContext,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaIdleLiteRestore,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaShaderCalculateResourceOffsets,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaDisableAsyncFlipPerfMode,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaAuxRetryOnUnknownError,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_BDW,
        WaAllowUMDToModifyHDCChicken1,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_BDW,
        WaResetPSDoesNotWriteToRT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    
    
    

    WA_ENABLE(
        ulStepId_BDW,
        WaNullStateDepthBufferAsD16And2d,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_DESKTOP,
        SIWA_UNTIL_BDW_F0);


    
    
    
    
    

    WA_ENABLE(
        ulStepId_BDW,
        WaEDPModeSetSequenceChange,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_MOBILE,
        SIWA_FOREVER);
    
    WA_ENABLE(
        ulStepId_BDW,
        WaS3DCurrentFieldLeft,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
}

#ifdef __KCH
void InitBdwHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_BDW/*, ulStepId_PCH*/;

    ulStepId_BDW = (1 << pWaParam->usRevId); 
    
}
#endif 
