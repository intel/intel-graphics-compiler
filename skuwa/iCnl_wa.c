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

#define SIWA_ONLY_CNL_A0    SIWA_ONLY_A0
#define SIWA_UNTIL_CNL_A0   SIWA_UNTIL_A0
#define SIWA_FROM_CNL_A0    SIWA_FROM_A0
#define SIWA_AFTER_CNL_A0   SIWA_AFTER_A0

#define SIWA_ONLY_CNL_B0    SIWA_ONLY_A1
#define SIWA_UNTIL_CNL_B0   SIWA_UNTIL_A1
#define SIWA_FROM_CNL_B0    SIWA_FROM_A1
#define SIWA_AFTER_CNL_B0   SIWA_AFTER_A1


#define SIWA_ONLY_CNL_P0    SIWA_ONLY_A2
#define SIWA_UNTIL_CNL_P0   SIWA_UNTIL_A2
#define SIWA_FROM_CNL_P0    SIWA_FROM_A2
#define SIWA_AFTER_CNL_P0   SIWA_AFTER_A2

#define SIWA_ONLY_CNL_C0    SIWA_ONLY_A3
#define SIWA_UNTIL_CNL_C0   SIWA_UNTIL_A3
#define SIWA_FROM_CNL_C0    SIWA_FROM_A3
#define SIWA_AFTER_CNL_C0   SIWA_AFTER_A3

#define SIWA_ONLY_PCH_CNL_A0      SIWA_ONLY_A0
#define SIWA_FROM_PCH_CNL_A0      SIWA_FROM_A0
#define SIWA_UNTIL_PCH_CNL_A0     SIWA_UNTIL_A0
#define SIWA_AFTER_PCH_CNL_A0     SIWA_AFTER_A0


void InitCnlWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_CNL, ulStepId_PCH;
    unsigned int usHwRevId_CNL = pWaParam->usRevId;

    ulStepId_CNL = (1 << usHwRevId_CNL);
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH); 

    WA_ENABLE(
        ulStepId_CNL,
        WaForceEnableNonCoherent,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    if ((pWaParam->ePCHProductFamily >= PCH_LPT) &&
        (pWaParam->ePCHProductFamily <= PCH_CNP_H))
    {
        WA_ENABLE(
            ulStepId_PCH,
            WaUnitLevelClockGatingDisableGMBUS_PCH,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    
    WA_ENABLE(
        ulStepId_CNL,
        WaDisableSendsPreemption,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaStructuredBufferAsRawBufferOverride,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaConservativeRasterization,
        "No Link Provided" ,
        "No Link Provided" , 
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0 );

    WA_ENABLE(
        ulStepId_CNL,
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaClearTDRRegBeforeEOTForNonPS,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_C0);

    if (pWaParam->ePCHProductFamily >= PCH_SPT)
    {
        WA_ENABLE(
            ulStepId_CNL,
            WaReducedGMBusReadRetryCount,
            "No HWBugLink provided",
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    WA_ENABLE(
        ulStepId_CNL,
        Wa_220856683,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CNL,
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

    
    WA_ENABLE(
        ulStepId_CNL,
        WaNearestFilterLODClamp,
        "No Link Provided" , 
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaSkipInvalidSubmitsFromOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisable1DDepthStencil,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableLosslessCompressionForSampleL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableTiledResourceTranslationTables,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableRCWithAsyncFlip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_C0);

    WA_ENABLE(
        ulStepId_CNL,
        WaInterlacedmodeReqPlaneHeightMinTwoScanlines,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaMPOReqMinPlaneLeftFourBelowHActive,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaPSRandomCSNotDone,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
         SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSampleOffsetIZ,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaFlushHangWhenNonPipelineStateAndMarkerStalled,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableYV12BugFixInHalfSliceChicken7,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableSFCSrcCrop,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSFC270DegreeRotation,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        Wa8BitFrameIn10BitHevc,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        Wa32bitGeneralStateOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        Wa4kAlignUVOffsetNV12LinearSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcNukeOn3DBlt,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcPsrUpdateOnCpuHostModifyWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableDPFCGatingForFrontBufModifySignal,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);
    
    
    /*WA_ENABLE(
        ulStepId_CNL,
        WaLimit64BppScenarios,
        "No Link Provided" ,
        "No HWSightingLink Provide",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);*/

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcCdClkFreqTooLow,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcHighMemBwCorruptionAvoidance,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0
        );
        
    WA_ENABLE(
        ulStepId_CNL,
        WaFbcWakeMemOn,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcSkipSegments,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcTurnOffFbcWatermark,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcTurnOffFbcWhenHyperVisorIsUsed,
        "No Link Provided" ,
        "No HWSightingLink provided", 
        PLATFORM_ALL,
        SIWA_UNTIL_A0);
        
    WA_ENABLE(
        ulStepId_CNL,
        WaFbcNukeOnHostModify,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaFbcLinearSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaRsUseTimeoutMode,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_C0);
        
    WA_ENABLE(
        ulStepId_CNL,
        WaDisableNullPageAsDummy,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

#ifdef WIN32
    WA_ENABLE(
        ulStepId_CNL,
        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
#endif

    WA_ENABLE(
        ulStepId_CNL,
        WaVC1DecodingMaxResolution,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaAddVC1StuffingBytesForSPMP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaHuCNoStreamObject,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaReadVDEncOverflowStatus,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaHucBitstreamSizeLimitationEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableRFOSelfSnoop,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSetMipTailStartLODLargertoSurfaceLOD,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_CNL,
        WaFixR32G32FloatBorderTextureAddressingMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaSendMIFLUSHBeforeVFE,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_CNL,
        WaEnableGuCBootHashCheckNotSet,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_A0);

     WA_ENABLE(
         ulStepId_CNL,
         WaDisableGuCClockGating,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

     WA_ENABLE(
         ulStepId_CNL,
         WaGuCForceFenceByTlbInvalidateReg,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_ONLY_CNL_A0);

     WA_ENABLE(
         ulStepId_CNL,
         WaGuCCopyHuCKernelHashToSramVar,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_ONLY_CNL_A0);

     WA_ENABLE(
         ulStepId_CNL,
         WaGuCDummyWriteBeforeFenceCycle,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_ONLY_CNL_A0);

     WA_ENABLE(
         ulStepId_CNL,
         WaGuCDisableSRAMRestoreDisable,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_ONLY_CNL_A0);

    
    WA_ENABLE(
         ulStepId_CNL,
         WaReadVcrDebugRegister,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);
    
    WA_ENABLE(
        ulStepId_CNL,
        WaEnablePreemptionGranularityControlByUMD,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CNL,
        WaDisableRsInPostRestoreWaBb,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    
    WA_ENABLE(
        ulStepId_CNL,
        WaUsePseudoL3AddressingScheme,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableGamClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaGAMWrrbClkGateDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaIommuPendingInvalidationHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);
    
    WA_ENABLE(
        ulStepId_CNL,
        WaInvalidateTextureCache,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableSamplerL2BypassForTextureCompressedFormats,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_CNL,
        WaDCFlushOnCacheInvalidate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSendPushConstantsFromBTP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0 );

    WA_ENABLE(
        ulStepId_CNL,
        WaSendPushConstantsFromMMIO,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableChromaTrellisQuantization,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaLaceRAMGatedClockForLPDSTAutoIndexing,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaLaceIEWriteDuringPSR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSetDCFlushOnReadOnlyInvalidate,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaPCFlushBeforeRTCacheFlush,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaAllowUmdToModifySamplerMode,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaAvoidURBAllocationSizeMultipleOf3,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER );
    
    WA_ENABLE(
        ulStepId_CNL,
        WaVfPostSyncWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaUAVCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    
    
    WA_ENABLE(
        ulStepId_CNL,
        WaAllowUMDToModifyHDCChicken1,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    
    
    WA_ENABLE(
        ulStepId_CNL,
        WaAllowUMDToModify3DPrimitiveExtParam,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    
    
    WA_ENABLE(
        ulStepId_CNL,
        WaBindlessSamplerStateBoundsCheckingDefeature,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        FtrEnableFastAnisoL1BankingFix,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDefaultCrossAndSubSliceHashingForSimplePS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaAllowUmdWriteTRTTRootTable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaGucSizeUsedWhenValidatingHucCopy,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableIPC,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaIncreaseLatencyIPCEnabled,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaHwManagedClearConvertDepthFormat,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0 );

    WA_ENABLE(
        ulStepId_CNL,
        WaHwManagedClearResolveDepth,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_A0 );

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableDSCacheWorkAround,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaForceWakeRenderDuringMmioTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaResetPSDoesNotWriteToRT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaNeedHeightAlignmentForTiledYCaptureSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaSuperSliceHeaderPacking,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableNonStallingScoreboardBasedOnNumSlices,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaGuCInitSramToZeroes,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaSendDummyConstantsForPS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaInsertDummyPushConstPs,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);
   
    WA_ENABLE(
        ulStepId_CNL,
        WaPlanePosPlusWidthLessThanPipeHorSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableDMCForNV12MPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDups1GatingDisableClockGatingForMPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableChickenDCPR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableScalarClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
    );



    WA_ENABLE(
        ulStepId_CNL,
        WaPSR2MultipleRegionUpdateCorruption,
        "Wa to set 0x42080[3] = 1 before PSR2 enable",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaIgnoreDDIAStrap,
        "Workaround",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaWmMemoryReadLatency,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER 
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaEnablePSRExitOn3DLutUpdate,
        "Workaround",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaVRRDisableBackToBackMasterFlipHWSupport,
        "WA: Set bit 15 of MMIO register 0x42084 to 1 when using VRR with hardware port sync mode ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaGTCLockAcquisitionDelay,
        "Set Min Lock Duration = 1 (bits 11:8 of GTC_PORT_MISC_x) when enabling maintenance phase, and reset to default when maintenance phase is disabled.",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaUseYCordforPSR2,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaCDClkPLLLockCorrection,
        "CDCLK PLL may not lock reliably",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaWatermarkLinesBlocks,
        "If latency level 1 through 7 and Y tile: Result Blocks = Result Blocks + Y tile minimum; Result Lines = Result Lines + Minimum Scanlines for Y tile \
        If latency level 1 through 7 and not Y tile : Result Blocks = Result Blocks + 1",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaRCWaterMarkCalculation,
        "If Render Decompression enabled and latency level 0 : Result Blocks = Result Blocks + Y tile minimum, \
        Then ensure that the result blocks for higher latency levels are all at least as high as the new level 0. ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );    

    SI_WA_ENABLE(
        WaDisableTWM,
        "WA 1140: Enable Transition WM as default for all scenarios on all CNL/GLK steppings",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_NEVER
        );

   
    SI_WA_ENABLE(
            WaSklLpt,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    

    SI_WA_ENABLE(
        WaReducedGMBusReadRetryCount,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaMaskRegWriteinPSR2AndPSR2Playback,
        "No HWBug is filed yet ",
        "No Link",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaKeepPG1ActiveDueToDMCIssue,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableCursorWith1LineInInterlacedMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaEnableAccessToDisplayIO,
        "WA: Set 0x162088 bit 0 and 0x162090 bit 0 to 1b to enable access to display IO registers, Before the display initialize sequence.", 
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaHDMIRestrict12BpcRgbYuv444Modes,
        "WA 1139: Restrict HDMI to 8 bpc when the Htotal is >= 5461 pixels and the format is RGB or YUV444, \
         This means standard 4k CEA 24 - 30Hz resolutions cannot be supported with 12bpc and RGB or YUV444 ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0
        );

    WA_ENABLE(
        ulStepId_CNL,
        WaDisablePrimaryFlipsForMBO,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);
    
        WA_ENABLE(
        ulStepId_CNL,
        WaRsGatherPoolEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaRsDisableDecoupledMMIO,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    
    WA_ENABLE(
        ulStepId_CNL,
        WaPipeControlBefore3DStateSamplePattern,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaForceShaderChannelSelects,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableRc6Wabb,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaRCCCacheMissFix,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_CNL_P0);

   WA_ENABLE(
        ulStepId_CNL,
        WaAudioSetEPSS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableBlitterFbcTracking,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaAvoidBackToBackIdAndCurbeCommandsViaPipeControl,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableI2mCycleOnWRPort,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableMidThreadPreempt,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisablePreemptForMediaWalkerWithGroups,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaThrottleEUPerfToAvoidTDBackPressure,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CNL_B0);
        
    WA_ENABLE(
        ulStepId_CNL,
        WaDisableEnhancedSBEVertexCaching,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    
    
    
    
    /*
    WA_ENABLE(
        ulStepId_CNL,
        WaToggleSubsliceEnableBitsToClearCam,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);
    */

    WA_ENABLE(
        ulStepId_CNL,
        WaDisablePreemptForMediaWalkerWithGroups,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

     WA_ENABLE (
        ulStepId_CNL,
        WaWGBoxAndWDtranscoderEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

     
     WA_ENABLE(
         ulStepId_CNL,
         WaDisableReplayBufferBankArbitrationOptimization,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FROM_CNL_B0);

     
     
     WA_ENABLE (
        ulStepId_CNL,
        WaPipelineFlushCoherentLines,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaProgramMgsrForCorrectSliceSpecificMmioReads,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaInPlaceDecompressionHang,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    if (!(ulStepId_CNL & SIWA_FROM_CNL_B0))
    {
        pSkuTable->FtrGtPsmi = 0;
    }

    WA_ENABLE(
        ulStepId_CNL,
        WaSarbUnitClockGatingDisable,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaNV12YfTileHWCursorUnderrun,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    
    WA_ENABLE(
        ulStepId_CNL,
        WaClearRenderResponseMasks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaNV12YfTileHWCursorUnderrun,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaForceContextSaveRestoreNonCoherent,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaAlwaysEnableAlphaMode,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaNoSimd16TernarySrc0Imm,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaArbitraryNumMbsInSlice,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaPushConstantDereferenceHoldDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaAllowUMDToControlCoherency,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        Wa3DStateMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    if ((pWaParam->ePCHProductFamily == PCH_CNP_LP) || (pWaParam->ePCHProductFamily == PCH_CNP_H))
    {
        SI_WA_ENABLE(
            WaHardHangonHotPlug,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }

    

    SI_WA_ENABLE(
        WaRsForcewakeAddDelayForAck,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaForceRCPFEHangWorkaround,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_C0 );
        
    SI_WA_ENABLE(
        WaPruneModesHavingHfrontPorchBetween122To130,
        "No Link Provided" ,
        "Link",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableDcStatesWhenPSR_3DLUTEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaForIcompVarations,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_B0);

    WA_ENABLE(
        ulStepId_CNL,
        WaCSStallBefore3DSamplePattern,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaSamplerCacheFlushBetweenRedescribedSurfaceReads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CNL,
        WaDisableEarlyEOT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
}

#ifdef __KCH
void InitCnlHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    
}
#endif 
