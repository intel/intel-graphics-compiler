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





void InitBxtWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_BXT = (int)pWaParam->usRevId; 
#ifdef __KCH
    
#endif

    
    
    
    
    

    
    SI_WA_ENABLE(

        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
       
        WAInsertNOPBetweenMathPOWDIVAnd2RegInstr,
        "No Link Provided" ,
        "No HWSightingLink provided",
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

        WaPipeControlBeforeGpgpuImplicitFlushes,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );
        
    SI_WA_ENABLE(

        WaToEnableHwFixForPushConstHWBug,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_C0));

    
    
    

    SI_WA_ENABLE(

        Wa32bitGeneralStateOffset,
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

        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaTranslationTableUnavailable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaIommuCCInvalidationHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableSkipCaching,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        Wa4kAlignUVOffsetNV12LinearSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaLLCCachingUnsupported,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaFlushTlbAfterCpuGgttWrites,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaMsaa8xTileYDepthPitchAlignment,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaLinearCaptureSurface,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableNullPageAsDummy,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaAddDummyPageForDisplayPrefetch,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

    SI_WA_ENABLE(

        WaOCLEnableFMaxFMinPlusZero,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaPipelineFlushCoherentLines,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSamplerResponseLengthMustBeGreaterThan1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaOCLEnableSLMSizeGWLWA1,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaOCLEnableSLMSizeGWLWA2,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaOCLGAMRepeaterBug,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaGAMWrrbClkGateDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

     
     SI_WA_ENABLE(
        WaEnablePooledEuFor2x6,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0) );


    
    
    

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

        WaClearNotificationRegInGpgpuContextSave,
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

        WaRestoreFC4RegisterDW0fromDW1,
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

        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

    SI_WA_ENABLE(

        WaDisallow64BitImmMov,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableNoSrcDepSetBeforeEOTSend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(

        WaL3UseSamplerForVectorLoadScatter,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableLowPrecisionWriteRTRepData,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices ,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    
    
    


    SI_WA_ENABLE(

        WaStallBeforePostSyncOpOnGPGPU,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSendMIFLUSHBeforeVFE,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaBindlessSurfaceStateModifyEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    
    /*SI_WA_ENABLE(

        WaDisableIndirectDataAndFlushGPGPUWalker,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
    SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));*/

    SI_WA_ENABLE(

        WaCSRUncachable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_B0));

    
    SI_WA_ENABLE(

        WaNearestFilterLODClamp,
        "No Link Provided" ,
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
        WaDisable1DDepthStencil,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

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

        WaZeroOneClearValues,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaZeroOneClearValuesAtSampler,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));

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
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableStencilBufferTestOnStencilBufferDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    
    SI_WA_ENABLE(

        WaInjectFlushInB2BFastCopyBlts,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaMaskRegWriteinPSR2AndPSR2Playback,
        "No Link",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaMaskUnmaskRegisterWriteForMBOinFlip,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnsureLP7WMInPSR2,
        "No HWBug is filed yet ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
       WaUseYCordforPSR2,
       "No HWBugLink provided",
       "No HWSightingLink provided",
       PLATFORM_ALL,
       SI_WA_FOR_EVER);

   SI_WA_ENABLE(
       WaPSR2MultipleRegionUpdateCorruption,
       "Wa to set 0x42080[3] = 1 before PSR2 enable",
       "No HWSightingLink provided",
       PLATFORM_ALL,
       SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaVSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableRepcolMessages,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaCsStallBeforeNonZeroInstanceCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaRCFlushEvery16RTVOnBTPUpdate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaPipeControlBeforeVFCacheInvalidationEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaInvalidateTextureCache,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(

        WaCompressedResourceDisplayOldHashMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaInPlaceDecompressionHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_C0));

    SI_WA_ENABLE(

        WaIntegerDivisionSourceModifierNotSupported,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WAResetN0AfterRenderTargetRead,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaIndirectDispatchPredicate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaAvoidStcPMAStall,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_NEVER);  

    SI_WA_ENABLE(

        WaNullVertexBufferWhenZeroSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        AccWrEnNotAllowedToAcc1With16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaSetDepthToArraySizeForUAV,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableEuBypassOnSimd16Float32,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaStructuredBufferAsRawBufferOverride,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaConservativeRasterization,
        "No Link Provided" ,
        "No Link Provided" , 
        PLATFORM_ALL,
        SI_WA_UNTIL( StepId_BXT, BXT_REV_ID_B0 ) );

    SI_WA_ENABLE(

        WaMSFAfterWalkerWithoutSLMorBarriers,
        "No Link Provided" ,
        "No HWSightingLink Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(

        WaForceDX10BorderColorFor64BPTTextures,
        "No Link Provided" ,
        "No HWSightingLink Provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaForceNullSurfaceTileY,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaEnableTiledResourceTranslationTables,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaSendPushConstantsFromBTP,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaSendPushConstantsFromMMIO,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaIndependentAlphaBlend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaLowPrecWriteRTOnlyFloat,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableSamplerL2BypassForTextureCompressedFormats,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE( 

        WaDisableSamplerPowerBypassForSOPingPong, 
        "No Link Provided" , 
        "No HWSightingLink provided", 
        PLATFORM_ALL, 
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0)); 


    SI_WA_ENABLE(

        WaForceCsStallOnTimestampQuery,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);     

    SI_WA_ENABLE(
        WaAvoid16KWidthForTiledSurfaces,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaDCFlushOnL3CacheConfig,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaLimitSLMSizeTo16KBOnA0,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaLimitSLMSizeTo8KBOnB0,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSIMD16On3SrcInstr,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaSendDummyVFEafterPipelineSelect,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableSamplerRoundingDisableFix,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFixR32G32FloatBorderTextureAddressingMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
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
        WaSamplerCacheFlushBetweenRedescribedSurfaceReads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
    );



    SI_WA_ENABLE(

        WaDisableYTileForS3D,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    
    
    

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

        WaHuCNoStreamObject,
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

        WAMMCDDisableStallBitInPipeControl,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        Wa8BitFrameIn10BitHevc,
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

        WaDisableSFCSrcCrop,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSFC270DegreeRotation,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

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

        WaTlbAllocationForAvcVdenc,
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

        WaResetN0BeforeGatewayMessage,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaForcePcBbFullCfgRestore,
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

        WaClearRenderResponseMasks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableLockForTranscodePerf,
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

#ifdef WIN32
    SI_WA_ENABLE(

        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
#endif

    
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
        WaRsForcewakeAddDelayForAck,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRsDoubleRc6WrlWithCoarsePowerGating,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
    SI_WA_ENABLE(
        WaFbcLinearSurfaceStride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRsDisableCoarsePowerGating,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaRsUseTimeoutMode,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaRsClearFWBitsAtFLR,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaRsDisableDecoupledMMIO,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

    
    SI_WA_ENABLE(
        WaFbcAsynchFlipDisableFbcQueue,
        "No Link Provided" ,
        "No HWSightingLink provided",
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
        WaFbcDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(        
        WaFbcNukeOn3DBlt,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcPsrUpdateOnCpuHostModifyWrite,
        "No Link Provided" ,
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
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaGsvEnableSWTurbo,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaGsvDisableTurbo,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaFbcTurnOffFbcWatermark,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(

        WaDisableMidThreadPreempt,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaStateBindingTableOverfetch,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaFDIAutoLinkSetTimingOverrride,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaS3DSoftwareMode,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableThreadStallDopClockGating,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaHeaderRequiredOnSimd16Sample16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaLodRequiredOnTypedMsaaUav,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSrc1ImmHfNotAllowed,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

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
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

    SI_WA_ENABLE(
        
        WaForceShaderChannelSelects,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSendExtraRSGatherConstantAndRSStoreImmCmds,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableDgMirrorFixInHalfSliceChicken5,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaSetDisablePixMaskCammingAndRhwoInCommonSliceChicken,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    
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
        WaDisablePerCtxtPreemptionGranularityControl,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaEnablePreemptionGranularityControlByUMD,
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

    SI_WA_ENABLE(

        WaDisableObjectLevelPreemptionForTrifanOrPolygon,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableObjectLevelPreemptionForLineStripAdjLineStripContPolygon,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaOGLGSVertexReorderingTriStripAdjOnly,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    
    SI_WA_ENABLE(

        WaDisableObjectLevelPreemptionForInstancedDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableObjectLevelPreemtionForInstanceId,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableDither,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaEnableYV12BugFixInHalfSliceChicken7,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisablePartialInstShootdown,
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
        WaEnableVoidExtentBlockPatchingforASTCLDRTextures,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    
    SI_WA_ENABLE(
         WaEnableuKernelHeaderValidFix,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));
        
    SI_WA_ENABLE(
        WaGucSizeUsedWhenValidatingHucCopy,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGucDisable2ElementSubmission,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaDisableMinuteIaClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

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

        WaC6DisallowByGfxPause,
        "No Software Sighting provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaSetHdcUnitClockGatingDisableInUcgctl6,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableFtrSubSliceIzHashing,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaEnableForceRestoreInCtxtDescForVCS,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableCtxRestoreArbitration,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaIdleLiteRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_C0));


    SI_WA_ENABLE(

        WaDisablePreemptionForWatchdogTimer,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

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

         WaReadVcrDebugRegister,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));
     
    SI_WA_ENABLE(

         WaMixModeSelInstDstNotPacked,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

         WaCcsTlbPrefetchDisable,
         "No HWBugLink provided",
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

         WaModifyVFEStateAfterGPGPUPreemption,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

         WaDisableSTUnitPowerOptimization,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaMtpRenderPowerGatingBug,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


      
      
      
     
     SI_WA_ENABLE(

         WaEnableBandWidthLimitation,
         "TBD",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER
         );

     SI_WA_ENABLE(
         WaWatermarkLinesBlocks,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER
         );

    SI_WA_ENABLE(

         WaDisableRCWithAsyncFlip,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_FOR_EVER
         );

    SI_WA_ENABLE(

         WaDisableTrickleFeedForNV12,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_FOR_EVER
         );

    SI_WA_ENABLE(

         WaEnableSoftwarePCDDelay,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_ONLY(StepId_BXT, BXT_REV_ID_A0)
         );

    SI_WA_ENABLE(

        WaDisablePWMClockGating,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
       SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(

         WaForBlackFlashInMBOMode,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_FOR_EVER
         );

    SI_WA_ENABLE(

         WaDisablePooledEuLoadBalancingFix,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

         WaDisablePartialResolveInVc,
         "No HWBugLink provided",
         "No Link Provided" ,
         PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

         WaDisableSbeCacheDispatchPortSharing,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(

        WaSetClckGatingDisableMedia,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaSetSDEunitClckGatingDisable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaDisablePlaneGamma,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableDMCForNV12MPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDups1GatingDisableClockGatingForMPO,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSetHDCunitClckGatingDisable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaStoreMultiplePTEenable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    
    SI_WA_ENABLE(
        WaMIPIChangesFromBStep,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_AFTER(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaWatermarkLinesBlocks,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

     SI_WA_ENABLE(
        WaDisplayYtiling,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_BX_PRIME));

     SI_WA_ENABLE(
         WaMipiDispShiftIssue,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_BX_PRIME));

    
    SI_WA_ENABLE(
        WaDDdiHPDSwapUntilBStep,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaEnableRCNV12,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaWmMemoryReadLatency,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER 
        );

    
    SI_WA_ENABLE(
        WaEnableChickenDCPR,
        "TBD",
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
        WaDisableTWM,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaProgramHalfLineTimeForIPC,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
    
    SI_WA_ENABLE(

        WaDisablePreemptionWithCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaEnableDefaultEUCount,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaCheckEU10Disabled,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(
        WaDisableLSQCROPERFforOCL,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE (
        WaWGBoxAndWDtranscoderEnable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaEnableSamplerGPGPUPreemptionSupport,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
         WaDisableLSPCONAuxTransactionInLSMode,
         "No HWBugLink provided",
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

     SI_WA_ENABLE(
        WaForceWakeRenderDuringMmioTLBInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

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
         WaSendDummyGPGPUWalkerBeforeHSWithBarrier,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
         WaSendDummyConstantsForPS,
         "No Link Provided" ,
         "No HWSightingLink provided",
         PLATFORM_ALL,
         SI_WA_FOR_EVER);

    SI_WA_ENABLE(
         WaUnitLevelClockGatingDisableGMBUS_SOC,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);
         
    SI_WA_ENABLE(
         WaDisableKillLogic,
         "No HWBugLink",
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_FOR_EVER);
 
    SI_WA_ENABLE(
         WaClearTdlStateAckDirtyBits,
         "No Link Provided" ,
         "No Link Provided" ,
         PLATFORM_ALL,
         SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

    SI_WA_ENABLE(
        WaAudioSetEPSS,
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
        WaMediaPoolStateCmdInWABB,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);    

    
    SI_WA_ENABLE(
        WaConextSwitchWithConcurrentTLBInvalidate,
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
        WaDisableDOPRenderClkGatingAroundSubmit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaProgramL3SqcReg1DefaultForPerf,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_AFTER(StepId_BXT, BXT_REV_ID_A0));

    if (pWaParam->bWinDoD)
    {
        SI_WA_ENABLE(
            WaDisableOcaLogging,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SI_WA_FOR_EVER);
    }
    SI_WA_ENABLE(
        DisableSpritePassThroughMode,
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

}

#ifdef __KCH
void InitBxtHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_BXT = (int)pWaParam->usRevId; 
}
#endif 
