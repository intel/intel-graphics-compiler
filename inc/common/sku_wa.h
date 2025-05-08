/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SKU_WA_H__
#define __SKU_WA_H__

#if (_DEBUG || _RELEASE_INTERNAL)
#define GLOBAL_WAFTR_ENABLED 1
#endif

#ifdef _USC_

#include "usc.h"
#define SKU_FEATURE_TABLE  SUscSkuFeatureTable
#define PSKU_FEATURE_TABLE SUscSkuFeatureTable*

#else
#include "Driver_Model.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#if __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

typedef struct _SKU_FEATURE_TABLE
{

    struct
    {
        unsigned int   FtrDesktop : 1;
        unsigned int   FtrMultiFunc : 1;
        unsigned int   FtrGttBar : 1;
        unsigned int   FtrGmadrBar : 1;
        unsigned int   FtrMmioBar : 1;
        unsigned int   FtrDioBar : 1;
        unsigned int   FtrPrimaryVga : 1;
        unsigned int   FtrBandwidthLimit : 1;
        unsigned int   FtrCoreClkLimit : 1;
        unsigned int   Ftr36BitPhysAddress : 1;
        unsigned int   FtrVTdEnabled : 1;
        unsigned int   FtrPgtblEnableSupported : 1;
        unsigned int   FtrVERing : 1;
        unsigned int   FtrBlitterRing : 1;
        unsigned int   FtrMFXRing : 1;
        unsigned int   FtrVcs2 : 1;
        unsigned int   Ftr3dRing : 1;
        unsigned int   FtrPics : 1;
        unsigned int   FtrGtBigDie : 1;
        unsigned int   FtrGtMediumDie : 1;
        unsigned int   FtrGtSmallDie : 1;
        unsigned int   FtrGT1 : 1;
        unsigned int   FtrNativeGT1 : 1;
        unsigned int   FtrGT1_5 : 1;
        unsigned int   FtrGT1_75 : 1;
        unsigned int   FtrGT1_6 : 1;
        unsigned int   FtrGT2 : 1;
        unsigned int   FtrNativeGT2 : 1;
        unsigned int   FtrGT2_5 : 1;
        unsigned int   FtrGT3 : 1;
        unsigned int   FtrNativeGT3 : 1;
        unsigned int   FtrGT4 : 1;
        unsigned int   FtrPureGT1 : 1;
        unsigned int   FtrGTEuRowSelectLower : 1;
        unsigned int   FtrGTEuRowSelectUpper : 1;
        unsigned int   FtrGT4plus2 : 1;
        unsigned int   FtrGT4plus1 : 1;
        unsigned int   FtrGT2plus2 : 1;
        unsigned int   FtrGT2plus1 : 1;
        unsigned int   FtrEmbeddedPlatformEnabled : 1;
        unsigned int   FtrULT : 1;
        unsigned int   FtrIVBK0Platform : 1;
        unsigned int   FtrIVBM0M1Platform : 1;
        unsigned int   FtrIVBL0Platform : 1;
        unsigned int   FtrIVBL1Platform : 1;
        unsigned int   FtrIVBN0Platform : 1;
        unsigned int   FtrIVBE0E1Platform : 1;
        unsigned int   FtrIVBE2Platform : 1;
        unsigned int   FtrInternalSSC : 1;
        unsigned int   FtrUseMuxedSSCAsRef : 1;
        unsigned int   FtrChannelSwizzlingXOREnabled : 1;
        unsigned int   FtrULX : 1;
        unsigned int   FtrPipeCDisabled : 1;
        unsigned int   FtrPipeDDisabled : 1;
        unsigned int   FtrNumOfPipes : 3;
        unsigned int   FtrGTA : 1;
        unsigned int   FtrGTC : 1;
        unsigned int   FtrGTX : 1;
        unsigned int   Ftr5Slice : 1;
        unsigned int   FtrLLCMLCNotSupported : 1;
        unsigned int   FtrGfxClientSubmission : 1;
        unsigned int   FtrLCIA : 1;
        unsigned int   FtrHalo : 1;
        unsigned int   FtrDt : 1;
        unsigned int   FtrResourceStreamer : 1;
        unsigned int   FtrShaderDoubleSupportDisabled : 1;
        unsigned int   FtrHwSemaphore : 1;
        unsigned int   FtrCCSRing : 1;
        unsigned int   FtrSuperSku : 1;
    };
        struct
    {
        unsigned int   FtrPooledEuEnabled : 1;
    };

    struct
    {
        unsigned int   FtrMultiPipe : 1;
        unsigned int   Ftr3Pipes : 1;
        unsigned int   FtrInternalLvds : 1;
        unsigned int   FtrCrtOnPipeB : 1;
        unsigned int   FtrSerialDvo : 1;
        unsigned int   FtrSDVOHDMI : 1;
        unsigned int   FtrDualView : 1;
        unsigned int   FtrWirelessDisplayNndi : 1;
        unsigned int   FtreDPonPortD : 1;
        unsigned int   FtreDPonPortC : 1;
        unsigned int   FtreDPonPortB : 1;
        unsigned int   FtrWirelessDisplay_2_1 : 1;
        unsigned int   FtrDisplayDisabled : 1;
        unsigned int   FtrSGTPVSKUStrapPresent : 1;
        unsigned int   FtrS0ixSKU : 1;
        unsigned int   FtrS0ixSRByGunit : 1;
        unsigned int   FtrGfxS0ixCapable : 1;
        unsigned int   FtrConnectedStandby : 1;
        unsigned int   FtrSKU4KDisplay : 1;
        unsigned int   FtrSupportModesGreaterThan3840Horizontal : 1;
        unsigned int   FtrCollageSupport : 1;
        unsigned int   FtrLPEAudio : 1;
        unsigned int   FtrHDMIPixelMax225Mhz : 1;
        unsigned int   FtrDPMaxRes25x16_60 : 1;
        unsigned int   FtrCRTNotSupported : 1;
        unsigned int   FtrMipi : 1;
        unsigned int   FtrCommandModeMIPI  :   1;
        unsigned int   FtrAllowDC9WithMIPIDSR : 1;
        unsigned int   FtrDualMipi : 1;
        unsigned int   FtrSF1_0 : 1;
        unsigned int   FtrMPOSupport : 1;
        unsigned int   FtrDisableMPOMultiDisplayConfig : 1;
        unsigned int   FtrFractional48Hz : 1;
        unsigned int   FtrCHVBxSku : 1;
        unsigned int   FtrSplitScreenMPO : 1;
        unsigned int   FtrInvertRotation : 1;
        unsigned int   FtrPortraitLFP : 1;
        unsigned int   FtrGunitOffset : 1;
        unsigned int   FtrHWScalingNotSupported : 1;
        unsigned int   FtrDynamicCDClkOnlyForSDInternal : 1;
        unsigned int   FtrDDI4 : 1;
        unsigned int   FtrEnableUnderRun : 1;
        unsigned int   FtrEnable10bitRGBMPO : 1;
        unsigned int   FtrSAGVNotifyPCode : 1;
        unsigned int   FtrCoG : 1;
        unsigned int   FtreDPVDSC : 1;
        unsigned int   FtrPeriodicFrameNotification : 1;
        unsigned int   FtrDPVDSC : 1;
    };

    struct
    {
        unsigned int   FtrCapGttMapAddr : 1;

        unsigned int   FtrOverlay : 1;
        unsigned int   FtrOverlayMmioFlip : 1;
        unsigned int   FtrFullOverlayDownscale : 1;
        unsigned int   FtrFixedGfxMem64MbMax : 1;
        unsigned int   FtrTotalGfxMem256MbMax : 1;
        unsigned int   FtrAsyncMMIOFlipSupport : 1;


        unsigned int   FtrMediaReset : 1;
        unsigned int   FtrParallelEngine : 1;

        unsigned int   FtrEnableCloneOverlay : 1;
        unsigned int   FtrEnableCollageOverlay : 1;

        unsigned int   FtrGpGpuMidBatchPreempt                     :   1;
        unsigned int   FtrGpGpuThreadGroupLevelPreempt             :   1;
        unsigned int   FtrGpGpuMidThreadLevelPreempt               :   1;
        unsigned int   Ftr3dMidBatchPreempt                        :   1;
        unsigned int   Ftr3dObjectLevelPreempt                     :   1;
        unsigned int   FtrMediaMidBatchPreempt                     :   1;
        unsigned int   FtrMediaThreadGroupLevelPreempt             :   1;
        unsigned int   FtrMediaMidThreadLevelPreempt               :   1;
        unsigned int   FtrPerCtxtPreemptionGranularityControl      :   1;
        unsigned int   FtrDisableWDDMPreempt                       :   1;

        unsigned int   FtrBigPage                       : 1;
        unsigned int   FtrPPGTT                         : 1;
        unsigned int   FtrIA32eGfxPTEs                  : 1;
        unsigned int   FtrPml4Support                   : 1;
        unsigned int   FtrPml3OnHwPml4Support           : 1;
        unsigned int   FtrSVM                           : 1;
        unsigned int   FtrTileMappedResource            : 1;
        unsigned int   FtrTranslationTable              : 1;
        unsigned int   FtrUserModeTranslationTable      : 1;
        unsigned int   FtrNullPages                     : 1;
        unsigned int   FtrL3IACoherency                 : 1;
        unsigned int   FtrMIUpdateGTTCanUpdatePPGTT     : 1;
        unsigned int   FtrReportCombinedDVMSSVM         : 1;
        unsigned int   FtrRemoteFx                      : 1;
        unsigned int   FtrDriverManagedL3ParityErrors   : 1;
        unsigned int   FtrL3HangOnParityError           : 1;
        unsigned int   FtrEDram                         : 1;
        unsigned int   FtrLLCBypass                     : 1;
        unsigned int   FtrCrystalwell                   : 1;
        unsigned int   FtrCentralCachePolicy            : 1;
        unsigned int   FtrIoMmu                         : 1;
        unsigned int   FtrDriverControlledIoMmu         : 1;
        unsigned int   FtrIoMmuPageFaulting             : 1;
        unsigned int   FtrDmaBufferMemSpaceSplitting    : 1;
        unsigned int   FtrSecurePPGTTUpdate             : 1;
        unsigned int   FtrPigms                         : 1;
        unsigned int   FtrWddm2GpuMmu                   : 1;
        unsigned int   FtrWddm2Svm                      : 1;
        unsigned int   FtrStandardMipTailFormat         : 1;
        unsigned int   FtrDisplayColorEnhancement       : 1;
        unsigned int   FtrWddm2_1_64kbPages             : 1;
        unsigned int   FtrGttCacheInvalidation          : 1;
        unsigned int   FtrMemorySeg                     : 1;
        unsigned int   FtrCacheCoherentMemSeg           : 1;
        unsigned int   FtrLinearCCS                     : 1;

        unsigned int   FtrKMDTestSupportFromRegKey : 1;
        unsigned int   FtrKmdDaf : 1;
        unsigned int   FtrDisableOverlayRotation : 1;
        unsigned int   FtrUnmapPagingReservedGTTSeg : 1;
        unsigned int   FtrKmdNotifyUmd : 1;
        unsigned int   FtrDeferredWaitForEventOnAsyncFlip : 1;
        unsigned int   FtrPerfModeSdiWrite : 1;
        unsigned int   FtrUse3DEngineforLateralBlts : 1;
        unsigned int   FtrPreemptTestMode : 1;
        unsigned int   FtrGuCWriteCombineEnable : 1;
        unsigned int   FtrGuCInternalMsgChannelEnable : 1;

        unsigned int   FtrSubSliceIzHashing : 1;
        unsigned int   FtrFrameBufferLLC : 1;
        unsigned int   FtrGpuMmuPageFault : 1;
        unsigned int   FtrOSManagedAllocations  : 1;
        unsigned int   FtrCsResponseEventOptimization : 1;
        unsigned int   FtrRuntimeLogBuffer            : 1;
        unsigned int   FtrLocalMemory                 : 1;

    };

    struct
    {
        unsigned int   FtrHwBin : 1;
        unsigned int   Ftr8BitPalette : 1;
        unsigned int   FtrPixelShader : 1;
        unsigned int   FtrPixelShader30 : 1;
        unsigned int   FtrBWGConsumerTextures : 1;
        unsigned int   FtrMultiRenderTarget : 1;
        unsigned int   FtrHWTnL : 1;
        unsigned int   FtrOcclusionQuery : 1;
        unsigned int   FtrOcclusionQueryOGL : 1;
        unsigned int   FtrAutoGenMipMap : 1;
        unsigned int   FtrWorkstation : 1;
        unsigned int   FtrEtcFormats : 1;
        unsigned int   FtrAstcLdr2D : 1;
        unsigned int   FtrAstcHdr2D : 1;
        unsigned int   FtrAstc3D : 1;
        unsigned int   FtrUmdThreadingShim : 1;
        unsigned int   FtrBoundingBoxOptOGL : 1;
        unsigned int   FtrResourceStreamerEnabled : 1;
        unsigned int   FtrHiZSamplerDisabled : 1;
    };



    struct
    {
        unsigned int FtrWin7 : 1;
        unsigned int FtrWin8 : 1;
        unsigned int FtrWddm1_3 : 1;
        unsigned int FtrWddm2_0 : 1;
        unsigned int FtrWddm2_1 : 1;
        unsigned int FtrWddm2_2 : 1;
        unsigned int FtrWddm2_3 : 1;
        unsigned int FtrWddm2_4 : 1;
        unsigned int FtrWddm2_5 : 1;
    };

    struct
    {
        unsigned int    FtrKmSecurityParser : 1;
    };

    struct
    {
        unsigned int    FtrOsManagedHwContext : 1;
    };

    struct
    {
        unsigned int    FtrVgt : 1;
    };

    struct
    {
        unsigned int    FtrOGLTexelOffsetPrecisionFix : 1;
    };

    struct
    {
        unsigned int    FtrEnableMissingAlpaFormatFilter             : 1;
        unsigned int    FtrEnablePlanarYUVFilteringFix               : 1;
    };


    struct
    {
        unsigned int FtrGtPsmi : 1;

    };

} SKU_FEATURE_TABLE, *PSKU_FEATURE_TABLE;

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif




enum WA_BUG_TYPE
{
    WA_BUG_TYPE_UNKNOWN = 0,
    WA_BUG_TYPE_CORRUPTION = 1,
    WA_BUG_TYPE_HANG = 2,
    WA_BUG_TYPE_PERF = 4,
    WA_BUG_TYPE_FUNCTIONAL = 8,
    WA_BUG_TYPE_SPEC = 16,
    WA_BUG_TYPE_FAIL = 32
};

#define WA_BUG_PERF_IMPACT(f) f
#define WA_BUG_PERF_IMPACT_UNKNOWN -1

enum WA_BIT_TYPE
{
    WA_BIT_LEGACY = 0,
    WA_BIT_GT = 1,
    WA_BIT_MEDIA = 2,
    WA_BIT_DISPLAY = 3,
    WA_BIT_MAX = WA_BIT_DISPLAY,
};

enum WA_COMPONENT
{
    WA_COMPONENT_UNKNOWN = 0,
    WA_COMPONENT_KMD = 0x1,
    WA_COMPONENT_MINIPORT = 0x2,
    WA_COMPONENT_GMM = 0x4,
    WA_COMPONENT_D3D = 0x8,
    WA_COMPONENT_OGL = 0x10,
    WA_COMPONENT_SOFTBIOS = 0x20,
    WA_COMPONENT_PWRCONS = 0x40,
    WA_COMPONENT_MEDIA = 0x80,
    WA_COMPONENT_OCL = 0x100,
};


typedef struct _WA_TABLE
{

#define WA_DECLARE( wa, wa_comment, wa_bugType, wa_impact, wa_component) unsigned int wa : 1;
#include "sku_wa_defs.h"
#undef WA_DECLARE

} WA_TABLE, *PWA_TABLE;

#ifdef _USC_
/*****************************************************************************\

STRUCT:
    HW_STATUS

Description:
    holds WA info for compiler

\*****************************************************************************/
struct HW_STATUS
{
    SKU_FEATURE_TABLE   SkuTable;
    WA_TABLE            WaTable;

    SKU_FEATURE_TABLE*  pSkuTable;
    WA_TABLE*           pWaTable;
};
#endif



#if (defined(__MINIPORT) || defined(__KCH) || defined(__SOFTBIOS) || defined(__GRM) || defined(__PWRCONS))
#if LHDM || LINUX
#define GFX_IS_SKU(s, f) ((s)->SkuTable.f)
#define GFX_IS_WA(s, w)  ((s)->WaTable.w)
#define GFX_WRITE_WA(x, y, z) ((x)->WaTable.y = z)


#define GFX_WRITE_SKU(x, y, z) ((x)->SkuTable.y = z)
#else
#define GFX_IS_SKU(h, f) (((PHW_DEVICE_EXTENSION)(h))->pHWStatusPage->pSkuTable->f)
#define GFX_IS_WA(h, w) (((PHW_DEVICE_EXTENSION)(h))->pHWStatusPage->pWaTable->w)
#define GFX_WRITE_WA(x, y, z) (((HW_DEVICE_EXTENSION *)(x))->pHWStatusPage->pWaTable->y = z)


#define GFX_WRITE_SKU(x, y, z) (((HW_DEVICE_EXTENSION *)(x))->pHWStatusPage->pSkuTable->y = z)
#endif
#else
#if XPDM
#define GFX_IS_SKU(s, f) ((s)->pSkuTable->f)
#define GFX_IS_WA(s, w)  ((s)->pWaTable->w)
#else
#define GFX_IS_SKU(s, f) ((s)->SkuTable.f)
#define GFX_IS_WA(s, w)  ((s)->WaTable.w)
#endif
#endif
#define GRAPHICS_IS_SKU(s, f) ((s)->f)
#define GRAPHICS_IS_WA(s, w)  ((s)->w)

#endif

