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

#define GLK_REV_ID_A0   SI_REV_ID(0,0)
#define GLK_REV_ID_A1   SI_REV_ID(1,1)
#define GLK_REV_ID_A2   SI_REV_ID(2,2)
#define GLK_REV_ID_B0   SI_REV_ID(3,3)




void InitGlkWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    
    int StepId_GLK = (int)pWaParam->usRevId;

    
    
    
    
    

    
    SI_WA_ENABLE(

        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(

        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
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
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(

        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    
    
    

    SI_WA_ENABLE(

        WaStallBeforePostSyncOpOnGPGPU,
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

        WaSendMIFLUSHBeforeVFE,
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

        WaBindlessSurfaceStateModifyEnable,
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

        WaZeroOneClearValuesAtSampler,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_GLK, GLK_REV_ID_A0));

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

        WaVSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDSRefCountFullforceMissDisable,
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

        WaIntegerDivisionSourceModifierNotSupported,
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

        WaNullVertexBufferWhenZeroSize,
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
        SI_WA_FROM(StepId_GLK, GLK_REV_ID_A0));

    SI_WA_ENABLE(

        WaEnableTiledResourceTranslationTables,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FROM(StepId_GLK, GLK_REV_ID_A0));

    SI_WA_ENABLE(

        WaGucSizeUsedWhenValidatingHucCopy,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableGuCClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaSendPushConstantsFromMMIO,
        "No HWBugLink provided", 
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

        WaForceCsStallOnTimestampQuery,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);     

    SI_WA_ENABLE(
        WaDCFlushOnL3CacheConfig,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

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

        WaPipeControlBeforeGpgpuImplicitFlushes,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(

        WaSendDummyConstantsForPS,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaEnableVoidExtentBlockPatchingforASTCLDRTextures,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_GLK, BXT_REV_ID_B0));

    SI_WA_ENABLE(
        WaDisableSamplerRoundingDisableFix,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableKillLogic,
        "No Link Provided" ,
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
        WADisableGTPAndSetISPDisable,
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
        WaSetMipTailStartLODLargertoSurfaceLOD,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    
    
    

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

        WaDisableSTUnitPowerOptimization,
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

        WaClearRenderResponseMasks,
        "No HWBugLink provided",
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

        WaSkipInvalidSubmitsFromOS,
        "No Link Provided",
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
        WaToEnableHwFixForPushConstHWBug,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaGAMWrrbClkGateDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    
    

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

        WaMsaa8xTileYDepthPitchAlignment,
        "No HWBugLink provided",
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
        WaMaskRegWriteinPSR2AndPSR2Playback,
        "No HWBug is filed yet ",
        "No Link",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
    
    SI_WA_ENABLE(
        WaHDMIRestrict12BpcRgbYuv444Modes,
        "WA 1139: Restrict HDMI to 8 bpc when the Htotal is >= 5461 pixels and the format is RGB or YUV444, \
        This means standard 4k CEA 24 - 30Hz resolutions cannot be supported with 12bpc and RGB or YUV444 ",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_GLK, GLK_REV_ID_A1));

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
        WaDisableRFOSelfSnoop,
        "No Link Provided" ,
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
        WaFbcLinearSurfaceStride,
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

        WaRsDoubleRc6WrlWithCoarsePowerGating,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcWakeMemOn,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaFbcSkipSegments,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);

    SI_WA_ENABLE(
        WaFidMismatch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_GLK, GLK_REV_ID_A0));

    SI_WA_ENABLE(

        WaPipelineFlushCoherentLines,
        "No HWBugLink provided",
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
        WaAddMediaStateFlushCmd,
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

        WaEnableChromaTrellisQuantization,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_ONLY(StepId_GLK, GLK_REV_ID_A0));

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

#ifdef WIN32
    SI_WA_ENABLE(

        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
#endif

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

        WaReadVcrDebugRegister,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(

        WaVeboxSliceEnable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
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

        WaSuperSliceHeaderPacking,
        "No HWBugLink provided",
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

        WaEnableDscale,
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
        WaDisableRCWithAsyncFlip,
        "Need Link",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );
    
    SI_WA_ENABLE(
        WaUnitLevelClockGatingDisableGMBUS_SOC,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_AFTER(StepId_GLK, GLK_REV_ID_A0) 
        );

    SI_WA_ENABLE(

        WaEnableBitBashingFor4BlockEDID,
        "WA: Do not use GMBUS with 4 block EDID reads or similar cases where there is a wait between block reads.",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_GLK, GLK_REV_ID_A2) 
        );

    SI_WA_ENABLE(

        WaEnableSoftwarePCDDelay,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(

        WaDisablePWMClockGating,
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

        WaFbcPsrUpdateOnCpuHostModifyWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );

    SI_WA_ENABLE(
        WaDisableDPFCGatingForFrontBufModifySignal,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaEnableChickenDCPR,
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
        WaDisableTWM,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_NEVER
        );

    SI_WA_ENABLE(
        WaProgramHalfLineTimeForIPC,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDisableScalarClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER
    );

    SI_WA_ENABLE(
        WaDDIIOTimeout,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_GLK, GLK_REV_ID_A1) 
        );

    SI_WA_ENABLE(
        WaNV12YfTileHWCursorUnderrun,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaDSIRcompFailure,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER
        );
        
    SI_WA_ENABLE(
        WaMPOReqMinPlaneLeftFourBelowHActive,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaAlwaysEnableAlphaMode,
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
        WaSkipPortRegisterAccess,
        "No Link Provided" ,
        "Link",
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

        WaThreadSwitchAfterCall,
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

        WaBreakF32MixedModeIntoSimd8,
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

        WaStructuredBufferAsRawBufferOverride,
        "No Link Provided" ,
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

        WaLodRequiredOnTypedMsaaUav,
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
        WaInvalidateTextureCache,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

    SI_WA_ENABLE(
        WaDisableGatherAtSetShaderCommonSlice,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaProgramL3SqcReg1DefaultForPerf,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    SI_WA_ENABLE(
        WaVFEStateAfterPipeControlwithMediaStateClear,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(
        WaProgramL3SqcReg1DefaultForPerf,
        "No Link Provided" ,
        "No Link Provided" ,
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
        WaDisableMultiChannelAudioForDP,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
   
    SI_WA_ENABLE(
        WaDisableDcStatesWhenPSR_3DLUTEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
   
}

#ifdef __KCH
void InitGlkHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
   
}
#endif 
