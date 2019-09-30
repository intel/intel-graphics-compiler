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

#define SIWA_ONLY_SKL_A0    SIWA_ONLY_A0
#define SIWA_UNTIL_SKL_A0   SIWA_UNTIL_A0
#define SIWA_FROM_SKL_A0    SIWA_FROM_A0
#define SIWA_AFTER_SKL_A0   SIWA_AFTER_A0

#define SIWA_ONLY_SKL_B0    SIWA_ONLY_A1
#define SIWA_UNTIL_SKL_B0   SIWA_UNTIL_A1
#define SIWA_FROM_SKL_B0    SIWA_FROM_A1
#define SIWA_AFTER_SKL_B0   SIWA_AFTER_A1

#define SIWA_ONLY_SKL_C0    SIWA_ONLY_A2
#define SIWA_UNTIL_SKL_C0   SIWA_UNTIL_A2
#define SIWA_FROM_SKL_C0    SIWA_FROM_A2
#define SIWA_AFTER_SKL_C0   SIWA_AFTER_A2

#define SIWA_ONLY_SKL_D0    SIWA_ONLY_A3
#define SIWA_UNTIL_SKL_D0   SIWA_UNTIL_A3
#define SIWA_FROM_SKL_D0    SIWA_FROM_A3
#define SIWA_AFTER_SKL_D0   SIWA_AFTER_A3

#define SIWA_ONLY_SKL_E0    SIWA_ONLY_A4
#define SIWA_UNTIL_SKL_E0   SIWA_UNTIL_A4
#define SIWA_FROM_SKL_E0    SIWA_FROM_A4
#define SIWA_AFTER_SKL_E0   SIWA_AFTER_A4

#define SIWA_ONLY_SKL_F0    SIWA_ONLY_A5
#define SIWA_UNTIL_SKL_F0   SIWA_UNTIL_A5
#define SIWA_FROM_SKL_F0    SIWA_FROM_A5
#define SIWA_AFTER_SKL_F0   SIWA_AFTER_A5

#define SIWA_ONLY_SKL_G0    SIWA_ONLY_A6
#define SIWA_UNTIL_SKL_G0   SIWA_UNTIL_A6
#define SIWA_FROM_SKL_G0    SIWA_FROM_A6
#define SIWA_AFTER_SKL_G0   SIWA_AFTER_A6

#define SIWA_ONLY_SKL_H0    SIWA_ONLY_A7
#define SIWA_UNTIL_SKL_H0   SIWA_UNTIL_A7
#define SIWA_FROM_SKL_H0    SIWA_FROM_A7
#define SIWA_AFTER_SKL_H0   SIWA_AFTER_A7

#define SIWA_ONLY_SKL_I0    SIWA_ONLY_A8
#define SIWA_UNTIL_SKL_I0   SIWA_UNTIL_A8
#define SIWA_FROM_SKL_I0    SIWA_FROM_A8
#define SIWA_AFTER_SKL_I0   SIWA_AFTER_A8

#define SIWA_ONLY_SKL_J0    SIWA_ONLY_A9
#define SIWA_UNTIL_SKL_J0   SIWA_UNTIL_A9
#define SIWA_FROM_SKL_J0    SIWA_FROM_A9
#define SIWA_AFTER_SKL_J0   SIWA_AFTER_A9


#define SIWA_ONLY_PCH_SKL_A0      SIWA_ONLY_A0
#define SIWA_FROM_PCH_SKL_A0      SIWA_FROM_A0
#define SIWA_UNTIL_PCH_SKL_A0     SIWA_UNTIL_A0
#define SIWA_AFTER_PCH_SKL_A0     SIWA_AFTER_A0

#define SKL_PCH_SPT_A0_REV_ID     0x0
#define SKL_PCH_SPT_C0_REV_ID     0x20
#define SKL_PCH_SPT_D0_REV_ID     0x30


void InitSklWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam )
{
    unsigned int ulStepId_SKL, ulStepId_PCH;
    unsigned int usHwRevId_SKL = pWaParam->usRevId;

    ulStepId_SKL = (1 << usHwRevId_SKL); 
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH); 

    
    
    
    
    
#ifdef WIN32
    WA_ENABLE(
        ulStepId_SKL,
        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
#endif

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
        ulStepId_SKL,
        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);
        
    WA_ENABLE(
        ulStepId_SKL,
        WAInsertNOPBetweenMathPOWDIVAnd2RegInstr,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
        
    WA_ENABLE(
        ulStepId_SKL,
        WaForceEnableNonCoherent,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaIs64BInstrEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePlaneGamma,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableDMCForNV12MPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDups1GatingDisableClockGatingForMPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaFirstSyncFlipAfterMPOExit,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    if (pWaParam->ePCHProductFamily == PCH_SPT)
    {
       
        WA_ENABLE(
            ulStepId_PCH,
            WaSPTMmioReadFailure,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER);

        if ((pSkuTable->FtrDesktop && pWaParam->usRevId_PCH >= SKL_PCH_SPT_D0_REV_ID) || ((!pSkuTable->FtrDesktop) && pWaParam->usRevId_PCH >= SKL_PCH_SPT_C0_REV_ID)) 
        {
            pWaTable->WaSPTMmioReadFailure = 0;
        }

        WA_ENABLE(
            ulStepId_PCH,
            WaReducedGMBusReadRetryCount,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER);

    }

    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {
        WA_ENABLE(
            ulStepId_PCH,
            WaSklLpt,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableDC5DC6,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0); 

    WA_ENABLE(
        ulStepId_SKL,
        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_SKL,
        WaForcePcBbFullCfgRestore,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableL3ErrorDetectionHangOnError,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePrimaryFlipsForMBO,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

     WA_ENABLE(
        ulStepId_SKL,
        WaMaskRegWriteinPSR2AndPSR2Playback,
        "No HWBug is filed yet ",
        "No Link",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_SKL,
        WaMaskUnmaskRegisterWriteForMBOinFlip,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);
        
     WA_ENABLE(
        ulStepId_SKL,
        WaEnsureLP7WMInPSR2,
        "No HWBug is filed yet ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
         ulStepId_SKL,
         WaPipeControlBeforeGpgpuImplicitFlushes,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER );

     if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
     {
         WA_ENABLE(
             ulStepId_SKL,
             WaDisableGafsUnitClkGating,
             "No Link Provided" ,
             "No Link Provided" ,
             PLATFORM_ALL,
             SIWA_FOREVER);
     }

     WA_ENABLE(
         ulStepId_SKL,
         WaGAMWrrbClkGateDisable,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSetMipTailStartLODLargertoSurfaceLOD,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    
    
    

    WA_ENABLE(
        ulStepId_SKL,
        Wa32bitGeneralStateOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAllPasidInvHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaTranslationTableUnavailable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaIommuCCInvalidationHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableIommuTEBit,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSkipCaching,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaLosslessCompressionSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcLinearSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        Wa4kAlignUVOffsetNV12LinearSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);    

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableHGAsyncFlipLinearToTileConvert,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableNullPageAsDummy,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableRFOSelfSnoop,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAddDummyPageForDisplayPrefetch,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableEdramForDisplayRT,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    

    WA_ENABLE(
        ulStepId_SKL,
        WaOCLUseLegacyTiming,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableLSQCROPERFforOCL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE (
        ulStepId_SKL,
        WaWGBoxAndWDtranscoderEnable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaPipelineFlushCoherentLines,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAtomicsForceCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSamplerResponseLengthMustBeGreaterThan1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
        ulStepId_SKL,
        WaMsaa8xTileYDepthPitchAlignment,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    

    WA_ENABLE(
        ulStepId_SKL,
        WaScalarAtomic,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAdditionalMovWhenSrc1ModOnMulMach,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaClearArfDependenciesBeforeEot,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDoNotPushConstantsForAllPulledGSTopologies,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaCallForcesThreadSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaThreadSwitchAfterCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaGrfScoreboardClearInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaStoreAcc2to9InAlign16InGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaRestoreFc0RegistersWithOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaRestoreFC4RegisterDW0fromDW1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaClearFlowControlGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaClearCr0SpfInGpgpuContextRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaBreakF32MixedModeIntoSimd8,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableDSDualPatchMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisallow64BitImmMov,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisallowDFImmMovWithSimd8,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableNoSrcDepSetBeforeEOTSend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableLowPrecisionWriteRTRepData,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    
    
    

    WA_ENABLE(
        ulStepId_SKL,
        WaStallBeforePostSyncOpOnGPGPU,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    
    WA_ENABLE(
        ulStepId_SKL,
        WaCSRUncachable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFlushBefore3DSTATEGS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaVfPostSyncWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaNearestFilterLODClamp,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisable1DDepthStencil,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaZeroOneClearValues,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaZeroOneClearValuesAtSampler,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaZeroOneClearValuesMSAA,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableCCSClearsIfRtCompressionEnabledInGT3,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableLosslessCompressionForSampleL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableStencilBufferTestOnStencilBufferDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaResendURBWhenGSorHSGetsEnabled,
        "No Link Provided" , 
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaHizAndClearedResourcesBoundToSamplerAtSameTime,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaHizAndCompressedAtSamplerAtSameTime,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0 | SIWA_ONLY_SKL_E0 | SIWA_ONLY_SKL_F0 );

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSamplerL2BypassForTextureCompressedFormats,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaCompressedResourceRequiresConstVA21,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_B0 | SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0 );

    WA_ENABLE(
        ulStepId_SKL,
        WaCompressedResourceSamplerPbeMediaNewHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_C0);

    WA_ENABLE(
        ulStepId_SKL,
        WaCompressedResourceDisplayOldHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaInPlaceDecompressionHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_H0);

    WA_ENABLE( 
        ulStepId_SKL, 
        WaDisableSamplerPowerBypassForSOPingPong, 
        "No Link Provided" , 
        "No HWSightingLink provided", 
        PLATFORM_ALL,
        SIWA_FOREVER); 


    WA_ENABLE(
        ulStepId_SKL,
        WaCompressedResourceDisplayNewHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_E0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableKillLogic,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaNoA32ByteScatteredStatelessMessages,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaInjectFlushInB2BFastCopyBlts,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaIndirectDispatchPredicate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaAvoidStcPMAStall,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_NEVER);  

    SI_WA_ENABLE(
        WaAvoidStcPMAStallShaderFiltering,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);

    SI_WA_ENABLE(
        WaKeepPG1ActiveDueToDMCIssue,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaNullVertexBufferWhenZeroSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaCsStallBeforeNonZeroInstanceCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaRCFlushEvery16RTVOnBTPUpdate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaPipeControlBeforeVFCacheInvalidationEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaInvalidateTextureCache,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemtionForVertexCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSetVfGuardbandPreemptionVertexCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSetDepthToArraySizeForUAV,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableEuBypassOnSimd16Float32,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaStructuredBufferAsRawBufferOverride,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FROM_SKL_C0);

    WA_ENABLE(
        ulStepId_SKL,
        WaConservativeRasterization,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaMSFAfterWalkerWithoutSLMorBarriers,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

     WA_ENABLE(
        ulStepId_SKL,
        WaLowPrecWriteRTOnlyFloat,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaMSFWithNoWatermarkTSGHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaAtomicFlushOnInterfaceDescriptor,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaForceDX10BorderColorFor64BPTTextures,
        "No Link Provided" ,
        "No HWSightingLink Provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaForceNullSurfaceTileY,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaBindlessSurfaceStateModifyEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableTiledResourceTranslationTables,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_E0);

    WA_ENABLE(
        ulStepId_SKL,
        WaForceShaderChannelSelects,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendPushConstantsFromBTP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendPushConstantsFromMMIO,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_E0);

     WA_ENABLE(
        ulStepId_SKL,
        WaIndependentAlphaBlend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_E0);
        
    WA_ENABLE(
        ulStepId_SKL,
        WaAvoid16KWidthForTiledSurfaces,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_I0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisable4KPushConstant,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableDSCacheWorkAround,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendDummyConstantsForPS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    if (pSkuTable->FtrGT4)
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaForceCsStallOnTimestampQueryOrDepthCount,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    WA_ENABLE(
        ulStepId_SKL,
        WaInsertDummyPushConstPs,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER );


    WA_ENABLE(
        ulStepId_SKL,
        WaAllocateExtraVBPageForGpuMmuPageFaults,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSamplerCacheFlushBetweenRedescribedSurfaceReads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableVoidExtentBlockPatchingforASTCLDRTextures,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaFixR32G32FloatBorderTextureAddressingMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WADisableGTPAndSetISPDisable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    
    
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaRsForcewakeDelayAckPoll,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaRsDisableDecoupledMMIO,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaRsUseTimeoutMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaRsClearFWBitsAtFLR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaRsDisableCoarsePowerGating,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_E0);

        WA_ENABLE(
            ulStepId_SKL,
            WaDisableSlicePowerGating,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_E0);
    }
    
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaEnableGoMsgToGAMDuringCPD,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaRsDoubleRc6WrlWithCoarsePowerGating,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaGsvEnableSWTurbo,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaFbcAsynchFlipDisableFbcQueue,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0); 

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcDisableOnCompressionRatio2Or4,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcRequireStrideBeMultipleOfCompressionRatio,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcDisableOnNonZeroPlanePosition,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcProgramYTileCbStrideRegister,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_C0);

     WA_ENABLE(
        ulStepId_SKL,
        WaFbcProgramLinTileCbStrideRegister,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_C0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcInvalidateCompressedLines,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcTurnOffFbcWatermark,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
    
    WA_ENABLE(
        ulStepId_SKL,
        WaFbcNukeOn3DBlt,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcHighMemBwCorruptionAvoidance,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER
        );
        
    WA_ENABLE(
        ulStepId_SKL,
        WaFbcWakeMemOn,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcTurnOffFbcWhenHyperVisorIsUsed,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER); 

    WA_ENABLE(
        ulStepId_SKL,
        WaFbcNukeOnHostModify,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);
    

    WA_ENABLE(
        ulStepId_SKL,
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableRCWithAsyncFlip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableRCWithS3D,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaIgnoreDDIAStrap,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER 
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaWmMemoryReadLatency,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER 
        );

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableHBR2,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0
        );

     WA_ENABLE(
        ulStepId_SKL,
        WaEnableRCNV12,
        "TBD",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_C0
        );

     
     WA_ENABLE(
         ulStepId_SKL,
         WaEnableChickenDCPR,
         "TBD",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER
         );

     
     WA_ENABLE(
         ulStepId_SKL,
         WaEnableBandWidthLimitation,
         "TBD",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER
         );

    
    WA_ENABLE(
        ulStepId_SKL,
        WaControlPrimaryTLBUtilization,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableIPC,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL, 
        WaIncreaseLatencyIPCEnabled,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableTWM,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaWatermarkLinesBlocks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaPSR2MultipleRegionUpdateCorruption,
        "Wa to set 0x42080[3] = 1 before PSR2 enable",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER
        );

    WA_ENABLE(
        ulStepId_SKL,
        WaGTCLockAcquisitionDelay,
        "Set Min Lock Duration = 1 (bits 11:8 of GTC_PORT_MISC_x) when enabling maintenance phase, and reset to default when maintenance phase is disabled.",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER
        );
    SI_WA_ENABLE(
        WaPlaneSizeAlignmentFor180Rotation,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
    );

    SI_WA_ENABLE(
        WaPruneModesHavingHfrontPorchBetween122To130,
        "No Link Provided" ,
        "Link",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaInitCDClkNewSeq,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableYTileForS3D,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    
    
    WA_ENABLE(
        ulStepId_SKL,
        WaHucStreamoutEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaHucStreamoutOnlyDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaZeroHuCImemDmemAttributes,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaAllocateSLML3CacheCtrlOverride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableFenceDestinationToSLM,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaAddMediaStateFlushCmd,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaModeSwitchDummyFrame,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaNeedHeightAlignmentForTiledYCaptureSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSFCSrcCrop,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSFC270DegreeRotation,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaChickenBitsMidBatchPreemption,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

#if (_DEBUG || _RELEASE_INTERNAL)
    
    WA_ENABLE(
        ulStepId_SKL,
        WaEnableKernelDebugFeatureInHWUsingCsDebugMode1,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
#endif

    
    
    

   WA_ENABLE(
        ulStepId_SKL,
        Wa4x4STCOptimizationDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaClearNotificationRegInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaL3UseSamplerForVectorLoadScatter,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaOCLEnableFMaxFMinPlusZero,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0 );

    WA_ENABLE(
        ulStepId_SKL,
        WaHeaderRequiredOnSimd16Sample16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaLodRequiredOnTypedMsaaUav,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSrc1ImmHfNotAllowed,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSendsSrc0DstOverlap,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSIMD16On3SrcInstr,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        (SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0));

    WA_ENABLE(
        ulStepId_SKL,
        WaSendDummyVFEafterPipelineSelect,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_SKL,
        WaClearSlmSpaceAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaFlushCoherentL3CacheLinesAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaProgramMgsrForCorrectSliceSpecificMmioReads,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

        
    if( pSkuTable->FtrGT2 || pSkuTable->FtrGT1 )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaRccHangDisableMCSUnrefined,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER );
    }        
    if( pSkuTable->FtrGT3 )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaRccHangDisableMCSUnrefined,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_J0 );
    }
    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableRepcolMessages,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WAResetN0AfterRenderTargetRead,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendsSrc1SizeLimitWhenEOT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaForceMulSrc1WordToAlign1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDstSubRegNumNotAllowedWithLowPrecPacked,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModeLog,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModePow,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModeFdiv,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaResetN0BeforeGatewayMessage,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableHSEightPatchIfInputControlGeq29,
        "",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendMIFLUSHBeforeVFE,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    /*WA_ENABLE(
        ulStepId_SKL,
        WaDisableIndirectDataAndFlushGPGPUWalker,
        "No Link Provided" ,
        "No HWSightingLink provided",
        SIWA_UNTIL_SKL_E0,
        SIWA_FOREVER);*/

    WA_ENABLE(
        ulStepId_SKL,
        WaSendExtraRSGatherConstantAndRSStoreImmCmds,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableDgMirrorFixInHalfSliceChicken5,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_B0);

    if (pSkuTable->FtrGT4)
    {
        
        
        WA_ENABLE(
            ulStepId_SKL,
            WaDisableDopClockGating,
            "No HWBugLink provided",
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSDEUnitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSetMDRBunitClckGatingDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSetGAPSunitClckGateDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableAutostripInFFMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableDscale,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaForceMinMaxGSThreadCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSkipInvalidSubmitsFromOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WAMMCDUseSlice0Subslice0,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WAMMCDDisableStallBitInPipeControl,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSamplerRoundingDisableFix,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    if( pSkuTable->FtrGpGpuMidThreadLevelPreempt )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WAGPGPUMidThreadPreemption,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER);
    }

    WA_ENABLE(
        ulStepId_SKL,
        WaIntegerDivisionSourceModifierNotSupported,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForQuadStrip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForLineStripAdjLineStripContPolygon,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionDuringUAVDrawCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableVFUnitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForTrifanOrPolygon,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisable3DPreemptionDuringUAVDrawCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForInstancedDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemtionForInstanceId,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaOGLGSVertexReorderingTriStripAdjOnly,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableObjectLevelPreemptionForLineLoop,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableYV12BugFixInHalfSliceChicken7,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_C0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePreemptionOnSimd32,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMidThreadPreempt,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePerCtxtPreemptionGranularityControl,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnablePreemptionGranularityControlByUMD,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        AccWrEnNotAllowedToAcc1With16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaSendsSrc1Length0NotAllowed,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableDither,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    if( pSkuTable->FtrIoMmuPageFaulting )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WADisableWriteCommitForPageFault,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_B0 );
    }

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableFenceDestinationToSLM,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePowerCompilerClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePartialInstShootdown,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableInstructionShootdown,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaInsertGSforConstInterpolatedTrailingVertex,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMinuteIaClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaGuCForceFenceByTlbInvalidateReg,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaGuCCopyHuCKernelHashToSramVar,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

        WA_ENABLE(
        ulStepId_SKL,
        WaC6DisallowByGfxPause,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_C0);

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableFtrSubSliceIzHashing,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaHuCNoStreamObject,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaReadVDEncOverflowStatus,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaClearRenderResponseMasks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_SKL,
        WaEnableForceRestoreInCtxtDescForVCS,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaVC1DecodingMaxResolution,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAddVC1StuffingBytesForSPMP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaParseVC1PicHeaderInSlice,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaParseVC1FirstFieldPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaParseVC1BPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableCtxRestoreArbitration,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaIdleLiteRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);


    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePreemptionForWatchdogTimer,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableEUChangeForSs0DisableDieRecovery,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

     WA_ENABLE(
        ulStepId_SKL,
        WaSetDisablePixMaskCammingAndRhwoInCommonSliceChicken,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableChickenBitTSGBarrierAckForFFSliceCS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableCLVertexCache,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableRendCompFeature,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

      WA_ENABLE(
        ulStepId_SKL,
        WaSendSEnableIndirectMsgDesc,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0);

     WA_ENABLE(
        ulStepId_SKL,
        WaForceContextSaveRestoreNonCoherent,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

      WA_ENABLE(
         ulStepId_SKL,
         WaBarrierPerformanceFixDisable,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0);

    WA_ENABLE(
         ulStepId_SKL,
         WaEnableGapsTsvCreditFix,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FROM_SKL_C0);

   WA_ENABLE(
        ulStepId_SKL,
        WaEnableuKernelHeaderValidFix,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableGuCBootHashCheckNotSet,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableGuCClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableGoMsgAckDuringCPD,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

     
     WA_ENABLE(
         ulStepId_SKL,
         WaReadVcrDebugRegister,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);

     WA_ENABLE(
         ulStepId_SKL,
         WaMixModeSelInstDstNotPacked,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SIWA_FOREVER);

      WA_ENABLE(
         ulStepId_SKL,
         WaModifyVFEStateAfterGPGPUPreemption,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_UNTIL_SKL_D0);

      WA_ENABLE(
         ulStepId_SKL,
         WaDisablePreemptionWithCoherency,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_UNTIL_SKL_F0);

      WA_ENABLE(
         ulStepId_SKL,
         WaDisablePartialResolveInVc,
         "No HWBugLink provided",
         "No Link Provided" ,
         PLATFORM_ALL,
         SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_SKL,
        WaDisableHDCInvalidation,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

      WA_ENABLE(
         ulStepId_SKL,
         WaSplitPipeControlForTlbInvalidate,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER); 
      
      WA_ENABLE(
         ulStepId_SKL,
         WaEnableLbsSlaRetryTimerDecrement,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);

      WA_ENABLE(
         ulStepId_SKL,
         WaDisableSbeCacheDispatchPortSharing,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_UNTIL_SKL_F0);

    WA_ENABLE(
         ulStepId_SKL,
        WaCcsTlbPrefetchDisable,
        "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);

    WA_ENABLE(
         ulStepId_SKL,
         WaDisableSTUnitPowerOptimization,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisablePixelMaskBasedCammingInRcpbe,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaMtpRenderPowerGatingBug,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_G0);

    WA_ENABLE(
        ulStepId_SKL,
        WaUseYCordforPSR2,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_SKL_E0);

    WA_ENABLE(
        ulStepId_SKL,
        WaEnableSamplerGPGPUPreemptionSupport,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaAllowUMDToModifyHDCChicken1,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER)

    WA_ENABLE(
        ulStepId_SKL,
        WaForceWakeRenderDuringMmioTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaAllowUmdWriteTRTTRootTable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    SI_WA_ENABLE(
        WaDualMapUntil3DOnlyTRTT,
        "No HWBugLink provided",
        "No HwSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaGucSizeUsedWhenValidatingHucCopy,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableGatherAtSetShaderCommonSlice,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaResetPSDoesNotWriteToRT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaConextSwitchWithConcurrentTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaVFEStateAfterPipeControlwithMediaStateClear,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    if( pSkuTable->FtrGT1 || pSkuTable->FtrGT2 )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaUseAuxSurfaceMode,
            "No HWBugLink provided",
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_FOREVER );
    }
    if( pSkuTable->FtrGT3 )
    {
        WA_ENABLE(
            ulStepId_SKL,
            WaUseAuxSurfaceMode,
            "No HWBugLink provided",
            "No Link Provided" ,
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_J0 );
    }

    SI_WA_ENABLE(
        DisableSpritePassThroughMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    
    
    
    WA_ENABLE(
         ulStepId_SKL,
         WaDisableLSPCONAuxTransactionInLSMode,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SIWA_FOREVER);
    
    
    
    
    

    
    WA_ENABLE(
        ulStepId_SKL,
        WaSKLDPAfeOverride,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_MOBILE,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaHDMIVswingChickenBitOverride,
        "No SWSighting Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_SKL,
        WaAudioSetEPSS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

}

#ifdef __KCH
void InitSklHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam )
{
    unsigned int ulStepId_SKL, ulStepId_PCH;
    unsigned int usHwRevId_SKL = pWaParam->usRevId;

    ulStepId_SKL = (1 << usHwRevId_SKL); 
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH); 

    
    WA_ENABLE(
        ulStepId_PCH,
        WaSPTMmioAccessSbi,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FALSE);
    
}
#endif 
