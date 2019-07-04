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


/*********WA MAPPING - ISSUES EXISTING ON SKL GT H0, APPLICABLE ON KBL A0*********/
/**EACH COMPONENT MUST FOLLOW UP WITH HW DEs TO CONFIRM THE STEPPINGS FOR FIX**/


#define KBL_REV_ID_A0   SI_REV_ID(0,0)
#define KBL_REV_ID_B0   SI_REV_ID(1,1)
#define KBL_REV_ID_C0   SI_REV_ID(2,2)
#define KBL_REV_ID_D0   SI_REV_ID(3,3)
#define KBL_REV_ID_F0   SI_REV_ID(4,4)
#define KBL_REV_ID_C1   SI_REV_ID(5,5)
#define KBL_REV_ID_D1   SI_REV_ID(6,6)
#define KBL_REV_ID_H0   SI_REV_ID(7,7)
#define KBL_REV_ID_E0   SI_REV_ID(8,8)

#define KBL_PCH_SPT_A0_REV_ID     SI_REV_ID(0,0)
#define KBL_PCH_SPT_C0_REV_ID     SI_REV_ID(0x20, 0x20)
#define KBL_PCH_SPT_D0_REV_ID     SI_REV_ID(0x30, 0x30)



void InitKblNonDisplayWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_KBL = (int)pWaParam->usRevId;
    int iStepId_PCH = (int)pWaParam->usRevId_PCH;

    
    
    
    
    
#ifdef WIN32
    SI_WA_ENABLE(
        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
#endif

    if ((pWaParam->ePCHProductFamily >= PCH_LPT) &&
        (pWaParam->ePCHProductFamily <= PCH_CNP_H))
    {
        SI_WA_ENABLE(
            WaUnitLevelClockGatingDisableGMBUS_PCH,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }

    SI_WA_ENABLE(
        WaEnableChickenBits_Hypervisor,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(iStepId_KBL, KBL_REV_ID_F0));

    
    SI_WA_ENABLE(
        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaForceEnableNonCoherent,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaIs64BInstrEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisablePlaneGamma,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDups1GatingDisableClockGatingForMPO,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
		WaKeepPG1ActiveDueToDMCIssue,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    if (pWaParam->ePCHProductFamily == PCH_SPT)
    {

#ifndef  _USC_
        if (pSkuTable->FtrULT)
        {
            SI_WA_ENABLE(
                WaSPTMmioAccessSbi,
                "No Link Provided" ,
                "No HWSightingLink provided",
                PLATFORM_ALL,
                SI_WA_ONLY(iStepId_PCH, KBL_PCH_SPT_A0_REV_ID));
        }
#endif 

        if (pSkuTable->FtrDesktop)
        {
            SI_WA_ENABLE(
                WaSPTMmioReadFailure,
                "No Link Provided" ,
                "No Link Provided" ,
                PLATFORM_ALL,
                SI_WA_BEFORE(iStepId_PCH, KBL_PCH_SPT_D0_REV_ID));
        }
        if (!pSkuTable->FtrDesktop)
        {
            SI_WA_ENABLE(
                WaSPTMmioReadFailure,
                "No Link Provided" ,
                "No Link Provided" ,
                PLATFORM_ALL,
                SI_WA_BEFORE(iStepId_PCH, KBL_PCH_SPT_C0_REV_ID));
        }

        SI_WA_ENABLE(
            WaReducedGMBusReadRetryCount,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }

    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {
        SI_WA_ENABLE(
            WaSklLpt,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }

    SI_WA_ENABLE(
        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
        WaForcePcBbFullCfgRestore,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_E0));     

     SI_WA_ENABLE(
        WaMaskRegWriteinPSR2AndPSR2Playback,
        "No HWBug is filed yet ",
        "No Link",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaMaskUnmaskRegisterWriteForMBOinFlip,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C0));

     SI_WA_ENABLE(
        WaEnsureLP7WMInPSR2,
        "No HWBug is filed yet ",
        "No Link Provided" ,
        PLATFORM_ALL,
         SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C1)); 

     SI_WA_ENABLE(         
         WaPipeControlBeforeGpgpuImplicitFlushes,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER );


    SI_WA_ENABLE(
        WaDisableGamClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C0));

    SI_WA_ENABLE(
        WaIommuPendingInvalidationHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C0));

    SI_WA_ENABLE(
        WaHDCL3Deadlock,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C0));

    SI_WA_ENABLE(
        WaConextSwitchWithConcurrentTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaInvalidateTextureCache,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );


     if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
     {
         SI_WA_ENABLE(
             WaDisableGafsUnitClkGating,
             "No Link Provided" ,
             "No Link Provided" ,
             PLATFORM_ALL,
             SI_WA_FOR_EVER);
     }

     SI_WA_ENABLE(
         WaToEnableHwFixForPushConstHWBug,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FROM(iStepId_KBL, KBL_REV_ID_C0));

     SI_WA_ENABLE(
        WaSetMipTailStartLODLargertoSurfaceLOD,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    
    
    


    SI_WA_ENABLE(
        Wa4kAlignUVOffsetNV12LinearSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        Wa32bitGeneralStateOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSkipCaching,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaLosslessCompressionSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableNullPageAsDummy,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableRFOSelfSnoop,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableDynamicCreditSharing,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_B0));

    SI_WA_ENABLE(
        WaAddDummyPageForDisplayPrefetch,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);   

    
    
    

    SI_WA_ENABLE(
        WaDisableLSQCROPERFforOCL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(iStepId_KBL, KBL_REV_ID_D1));

    SI_WA_ENABLE(
        WaPipelineFlushCoherentLines,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaMsaa8xTileYDepthPitchAlignment,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

    SI_WA_ENABLE(
        WaScalarAtomic,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaAdditionalMovWhenSrc1ModOnMulMach,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaClearArfDependenciesBeforeEot,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDoNotPushConstantsForAllPulledGSTopologies,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaCallForcesThreadSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaThreadSwitchAfterCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGrfScoreboardClearInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaStoreAcc2to9InAlign16InGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRestoreFC4RegisterDW0fromDW1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaClearFlowControlGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaClearCr0SpfInGpgpuContextRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaBreakF32MixedModeIntoSimd8,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableNoSrcDepSetBeforeEOTSend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

    SI_WA_ENABLE(
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaForceShaderChannelSelects,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaStallBeforePostSyncOpOnGPGPU,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaMSFAfterWalkerWithoutSLMorBarriers,
        "No Link Provided" ,
        "No HWSightingLink Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaFlushBefore3DSTATEGS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaVfPostSyncWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaNearestFilterLODClamp,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisable1DDepthStencil,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaZeroOneClearValuesAtSampler,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaZeroOneClearValuesMSAA,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableLosslessCompressionForSampleL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaDisableSamplerL2BypassForTextureCompressedFormats,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaCompressedResourceSamplerPbeMediaNewHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaCompressedResourceDisplayNewHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaInPlaceDecompressionHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
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
        WaDisableSamplerPowerBypassForSOPingPong, 
        "No Link Provided" , 
        "No HWSightingLink provided", 
        PLATFORM_ALL, 
        SI_WA_FOR_EVER); 


    SI_WA_ENABLE(
        WaNullVertexBufferWhenZeroSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaPipeControlBeforeVFCacheInvalidationEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSetDepthToArraySizeForUAV,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaStructuredBufferAsRawBufferOverride,
        "No Link Provided" ,
        "No Link Provided" , 
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaConservativeRasterization,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL( iStepId_KBL, KBL_REV_ID_A0 ) );

    SI_WA_ENABLE(
        WaBindlessSurfaceStateModifyEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaEnableTiledResourceTranslationTables,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSendPushConstantsFromMMIO,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
        
    SI_WA_ENABLE(
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL( iStepId_KBL, KBL_REV_ID_A0 ) );

    SI_WA_ENABLE(
        WaEnableDSCacheWorkAround,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE( iStepId_KBL, KBL_REV_ID_C0 ) );

    SI_WA_ENABLE(
        WaSendDummyConstantsForPS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSbeCacheDispatchPortSharing,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaAtomicFlushOnInterfaceDescriptor,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaMSFWithNoWatermarkTSGHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    if (pSkuTable->FtrGT4)
    {
        SI_WA_ENABLE(
            WaForceCsStallOnTimestampQueryOrDepthCount,
            "No Link Provided" ,
            "No Link Provided" ,
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }


    SI_WA_ENABLE(
        WaAllocateExtraVBPageForGpuMmuPageFaults,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSamplerCacheFlushBetweenRedescribedSurfaceReads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableVoidExtentBlockPatchingforASTCLDRTextures,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSamplerRoundingDisableFix,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WADisableGTPAndSetISPDisable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaResetN0BeforeGatewayMessage,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

    SI_WA_ENABLE(
        WaRsClearFWBitsAtFLR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRsForcewakeAddDelayForAck,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER); 
    
    SI_WA_ENABLE(
        WaFbcProgramYTileCbStrideRegister,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
        WaFbcProgramLinTileCbStrideRegister,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaRsDoubleRc6WrlWithCoarsePowerGating,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcNukeOn3DBlt,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcHighMemBwCorruptionAvoidance,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );
        
    SI_WA_ENABLE(
        WaFbcWakeMemOn,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(		
        WaFbcTurnOffFbcWhenHyperVisorIsUsed,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_A0));

    SI_WA_ENABLE(
        WaFbcLinearSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcTurnOffFbcWatermark,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcNukeOnHostModify,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFixR32G32FloatBorderTextureAddressingMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    
    SI_WA_ENABLE(
        WaHucStreamoutEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaHucStreamoutOnlyDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaReadVDEncOverflowStatus,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaHucBitstreamSizeLimitationEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaHuCNoStreamObject,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaAddMediaStateFlushCmd,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        Wa8BitFrameIn10BitHevc,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSFCSrcCrop,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_A0));

    SI_WA_ENABLE(
        WaSFC270DegreeRotation,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableChromaTrellisQuantization,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_A0));

    SI_WA_ENABLE(
        WaChickenBitsMidBatchPreemption,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(
        WaNeedHeightAlignmentForTiledYCaptureSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableDscale,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSuperSliceHeaderPacking,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

   SI_WA_ENABLE(
        Wa4x4STCOptimizationDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaClearNotificationRegInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

#if (_DEBUG || _RELEASE_INTERNAL)
    
    SI_WA_ENABLE(
        WaEnableKernelDebugFeatureInHWUsingCsDebugMode1,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
#endif

    
    SI_WA_ENABLE(
        WaL3UseSamplerForVectorLoadScatter,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaLodRequiredOnTypedMsaaUav,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSendDummyVFEafterPipelineSelect,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaFlushCoherentL3CacheLinesAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaProgramMgsrForCorrectSliceSpecificMmioReads,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaModifyVFEStateAfterGPGPUPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(iStepId_KBL, KBL_REV_ID_D0));

    SI_WA_ENABLE(
        WaSendMIFLUSHBeforeVFE,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaSkipInvalidSubmitsFromOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    if (pSkuTable->FtrGT2 || pSkuTable->FtrGT1 || pSkuTable->FtrGT1_5)
    {
        SI_WA_ENABLE(
            WaGttCachingOffByDefault,
            "No Link Provided" ,
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_D0) || SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_C1));
    }

    
    if( pSkuTable->FtrGpGpuMidThreadLevelPreempt )
    {
        SI_WA_ENABLE(
            WAGPGPUMidThreadPreemption,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }

    SI_WA_ENABLE(
        WaIntegerDivisionSourceModifierNotSupported,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableYV12BugFixInHalfSliceChicken7,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnablePreemptionGranularityControlByUMD,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisablePartialInstShootdown,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaClearRenderResponseMasks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaVC1DecodingMaxResolution,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaAddVC1StuffingBytesForSPMP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaParseVC1PicHeaderInSlice,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaParseVC1FirstFieldPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaParseVC1BPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaEnableGapsTsvCreditFix,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

     SI_WA_ENABLE(
        WaForceContextSaveRestoreNonCoherent,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
        WaEnableGuCBootHashCheckNotSet,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaDisableGuCClockGating,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaEnableGoMsgAckDuringCPD,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_F0) || SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_C1));

     
     SI_WA_ENABLE(
         WaReadVcrDebugRegister,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

     SI_WA_ENABLE(
         WaMixModeSelInstDstNotPacked,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

      SI_WA_ENABLE(
         WaDisablePartialResolveInVc,
         "No HWBugLink provided",
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

      SI_WA_ENABLE(
         WaSplitPipeControlForTlbInvalidate,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER); 

      SI_WA_ENABLE(
         WaEnableLbsSlaRetryTimerDecrement,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
         WaCcsTlbPrefetchDisable,
        "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
         WaDisableSTUnitPowerOptimization,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableSamplerGPGPUPreemptionSupport,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaAllowUMDToModifyHDCChicken1,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaForceWakeRenderDuringMmioTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_A0));

    SI_WA_ENABLE(
        WaEnableuKernelHeaderValidFix,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_A0));

    SI_WA_ENABLE(
        WaAllowUmdWriteTRTTRootTable,
        "No HWBugLink provided",
        "No HwSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDualMapUntil3DOnlyTRTT,
        "No HWBugLink provided",
        "No HwSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGucSizeUsedWhenValidatingHucCopy,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        (SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_F0) || SI_WA_ONLY(iStepId_KBL, KBL_REV_ID_C1)));

    SI_WA_ENABLE(
        WaGuCForceFenceByTlbInvalidateReg,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGuCCopyHuCKernelHashToSramVar,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaResetPSDoesNotWriteToRT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaForGAMHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(iStepId_KBL, KBL_REV_ID_C0));

    SI_WA_ENABLE(
        WaGAMWrrbClkGateDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableGatherAtSetShaderCommonSlice,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaVFEStateAfterPipeControlwithMediaStateClear,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
}
void InitKblDisplayWaTable(
    PWA_TABLE                       pWaTable,
    PSKU_FEATURE_TABLE              pSkuTable,
    PWA_INIT_PARAM                  pWaParam)
{
    int iStepId_KBL = (int)pWaParam->usRevId;
    SI_WA_ENABLE(
        WaDisableRCWithAsyncFlip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaDisableIPC,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaIncreaseLatencyIPCEnabled,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaDisableTWM,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaIgnoreDDIAStrap,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER 
        );

    SI_WA_ENABLE(
        WaWmMemoryReadLatency,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER 
        );

    SI_WA_ENABLE(
        WaControlPrimaryTLBUtilization,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
         WaEnableChickenDCPR,
        "TBD",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
    
    SI_WA_ENABLE(
        WaPSR2MultipleRegionUpdateCorruption,
        "Wa to set 0x42080[3] = 1 before PSR2 enable",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaEnableBandWidthLimitation,
        "TBD",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaWatermarkLinesBlocks,
        "TBD",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGTCLockAcquisitionDelay,
        "Set Min Lock Duration = 1 (bits 11:8 of GTC_PORT_MISC_x) when enabling maintenance phase, and reset to default when maintenance phase is disabled.",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaUseYCordforPSR2,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
		
	SI_WA_ENABLE(
		WaPruneModesHavingHfrontPorchBetween122To130,
		"No Link Provided" ,
		"Link",
		PLATFORM_ALL,
		SI_WA_FOR_EVER);
		

    SI_WA_ENABLE(
        WaPlaneSizeAlignmentFor180Rotation,
        "No Link Provided" ,
		"No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
    );
	SI_WA_ENABLE(
		WaInitCDClkNewSeq,
		"No HWBugLink provided",
		"No HWSightingLink provided",
		PLATFORM_ALL,
		SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        DisableSpritePassThroughMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    
    
    
    SI_WA_ENABLE(
         WaDisableLSPCONAuxTransactionInLSMode,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);
    
    
    
    
    

    SI_WA_ENABLE(
        WaAudioSetEPSS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaKBLVECSSemaphoreWaitPoll,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_MOBILE,
        SI_WA_UNTIL(iStepId_KBL, KBL_REV_ID_F0));

    
    WA_ENABLE(
        iStepId_KBL,
        WaKBLDPAfeOverride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_MOBILE,
        SI_WA_FOR_EVER);

    
    WA_ENABLE(
        iStepId_KBL,
        WaHDMIVswingChickenBitOverride,
        "No SWSighting Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
}

#ifdef __KCH
void InitKblHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam )
{
    int iStepId_KBL = (int)pWaParam->usRevId;

    
    SI_WA_ENABLE(
        WaSPTMmioAccessSbi,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);
}
#endif 
