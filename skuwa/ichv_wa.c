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

#define SIWA_ONLY_CHV_A0    SIWA_ONLY_A0
#define SIWA_UNTIL_CHV_A1   SIWA_UNTIL_A0
#define SIWA_FROM_CHV_A0    SIWA_FROM_A0
#define SIWA_AFTER_CHV_A1   SIWA_AFTER_A0

#define SIWA_ONLY_CHV_A1    SIWA_ONLY_A1
#define SIWA_UNTIL_CHV_A3   SIWA_UNTIL_A1
#define SIWA_FROM_CHV_A1    SIWA_FROM_A1
#define SIWA_AFTER_CHV_A3   SIWA_AFTER_A1

#define SIWA_ONLY_CHV_A3    SIWA_ONLY_A2
#define SIWA_UNTIL_CHV_A7   SIWA_UNTIL_A2
#define SIWA_FROM_CHV_A3    SIWA_FROM_A2
#define SIWA_AFTER_CHV_A7   SIWA_AFTER_A2

#define SIWA_ONLY_CHV_B0    SIWA_ONLY_A3
#define SIWA_UNTIL_CHV_B7   SIWA_UNTIL_A3
#define SIWA_FROM_CHV_B0    SIWA_FROM_A3
#define SIWA_AFTER_CHV_B7   SIWA_AFTER_A3

#define SIWA_ONLY_CHV_C0    SIWA_ONLY_A4
#define SIWA_UNTIL_CHV_C7   SIWA_UNTIL_A4
#define SIWA_FROM_CHV_C0    SIWA_FROM_A4
#define SIWA_AFTER_CHV_C7   SIWA_AFTER_A4

#define SIWA_ONLY_CHV_D0    SIWA_ONLY_A5
#define SIWA_UNTIL_CHV_D1   SIWA_UNTIL_A5
#define SIWA_FROM_CHV_D0    SIWA_FROM_A5
#define SIWA_AFTER_CHV_D1   SIWA_AFTER_A5

#define SIWA_ONLY_CHV_K0    SIWA_ONLY_A6
#define SIWA_UNTIL_CHV_K7   SIWA_UNTIL_A6
#define SIWA_FROM_CHV_K0    SIWA_FROM_A6
#define SIWA_AFTER_CHV_K7   SIWA_AFTER_A6


#define SIWA_ONLY_CHV_E0    SIWA_ONLY_CHV_K0
#define SIWA_UNTIL_CHV_E7   SIWA_UNTIL_CHV_K7
#define SIWA_FROM_CHV_E0    SIWA_FROM_CHV_K0
#define SIWA_AFTER_CHV_E7   SIWA_AFTER_CHV_K7





typedef enum CHV_GFX_REVISION_ID_REC {
#if(_DEBUG || _RELEASE_INTERNAL)
    CHV_HAS_A_Backward_Compatibile          = 0xFF,   
    CHV_HAS_A           = 0xFA,   
    CHV_HAS_B = 0xFB,   
    CHV_HAS_C = 0xFC,   
    CHV_HAS_D = 0xFD,   
    CHV_HAS_K = 0xFE,   
#endif
    CHV_A0_17x17_Type4  = 0x00,   
    CHV_A0_25x27        = 0x01,   
    CHV_A0_17x17_Type3  = 0x02,   
    CHV_A0_CoPOP        = 0x03,   
    CHV_A1_17x17_Type4  = 0x04,   
    CHV_A1_25x27        = 0x05,   
    CHV_A1_17x17_Type3  = 0x06,   
    CHV_A1_CoPOP        = 0x07,   
    CHV_A2_17x17_Type4  = 0x08,   
    CHV_A2_25x27        = 0x09,   
    CHV_A2_17x17_Type3  = 0x0A,   
    CHV_A2_CoPOP        = 0x0B,   
    CHV_A3_17x17_Type4  = 0x0C,   
    CHV_A3_25x27        = 0x0D,   
    CHV_A3_17x17_Type3  = 0x0E,   
    CHV_A3_CoPOP        = 0x0F,   
    CHV_A4_17x17_Type4  = 0x80,   
    CHV_A4_25x27        = 0x81,   
    CHV_A4_17x17_Type3  = 0x82,   
    CHV_A4_CoPOP        = 0x83,   
    CHV_A5_17x17_Type4  = 0x84,   
    CHV_A5_25x27        = 0x85,   
    CHV_A5_17x17_Type3  = 0x86,   
    CHV_A5_CoPOP        = 0x87,   
    CHV_A6_17x17_Type4  = 0x88,   
    CHV_A6_25x27        = 0x89,   
    CHV_A6_17x17_Type3  = 0x8A,   
    CHV_A6_CoPOP        = 0x8B,   
    CHV_A7_17x17_Type4  = 0x8C,   
    CHV_A7_25x27        = 0x8D,   
    CHV_A7_17x17_Type3  = 0x8E,   
    CHV_A7_CoPOP        = 0x8F,   
    CHV_B0_17x17_Type4  = 0x10,   
    CHV_B0_25x27        = 0x11,   
    CHV_B0_17x17_Type3  = 0x12,   
    CHV_B0_CoPOP        = 0x13,   
    CHV_B1_17x17_Type4  = 0x14,   
    CHV_B1_25x27        = 0x15,   
    CHV_B1_17x17_Type3  = 0x16,   
    CHV_B1_CoPOP        = 0x17,   
    CHV_B2_17x17_Type4  = 0x18,   
    CHV_B2_25x27        = 0x19,   
    CHV_B2_17x17_Type3  = 0x1A,   
    CHV_B2_CoPOP        = 0x1B,   
    CHV_B3_17x17_Type4  = 0x1C,   
    CHV_B3_25x27        = 0x1D,   
    CHV_B3_17x17_Type3  = 0x1E,   
    CHV_B3_CoPOP        = 0x1F,   
    CHV_B4_17x17_Type4  = 0x90,   
    CHV_B4_25x27        = 0x91,   
    CHV_B4_17x17_Type3  = 0x92,   
    CHV_B4_CoPOP        = 0x93,   
    CHV_B5_17x17_Type4  = 0x94,   
    CHV_B5_25x27        = 0x95,   
    CHV_B5_17x17_Type3  = 0x96,   
    CHV_B5_CoPOP        = 0x97,   
    CHV_B6_17x17_Type4  = 0x98,   
    CHV_B6_25x27        = 0x99,   
    CHV_B6_17x17_Type3  = 0x9A,   
    CHV_B6_CoPOP        = 0x9B,   
    CHV_B7_17x17_Type4  = 0x9C,   
    CHV_B7_25x27        = 0x9D,   
    CHV_B7_17x17_Type3  = 0x9E,   
    CHV_B7_CoPOP        = 0x9F,   
    CHV_C0_17x17_Type4  = 0x20,   
    CHV_C0_25x27        = 0x21,   
    CHV_C0_17x17_Type3  = 0x22,   
    CHV_C0_CoPOP        = 0x23,   
    CHV_C1_17x17_Type4  = 0x24,   
    CHV_C1_25x27        = 0x25,   
    CHV_C1_17x17_Type3  = 0x26,   
    CHV_C1_CoPOP        = 0x27,   
    CHV_C2_17x17_Type4  = 0x28,   
    CHV_C2_25x27        = 0x29,   
    CHV_C2_17x17_Type3  = 0x2A,   
    CHV_C2_CoPOP        = 0x2B,   
    CHV_C3_17x17_Type4  = 0x2C,   
    CHV_C3_25x27        = 0x2D,   
    CHV_C3_17x17_Type3  = 0x2E,   
    CHV_C3_CoPOP        = 0x2F,   
    CHV_C4_17x17_Type4  = 0xA0,   
    CHV_C4_25x27        = 0xA1,   
    CHV_C4_17x17_Type3  = 0xA2,   
    CHV_C4_CoPOP        = 0xA3,   
    CHV_C5_17x17_Type4  = 0xA4,   
    CHV_C5_25x27        = 0xA5,   
    CHV_C5_17x17_Type3  = 0xA6,   
    CHV_C5_CoPOP        = 0xA7,   
    CHV_C6_17x17_Type4  = 0xA8,   
    CHV_C6_25x27        = 0xA9,   
    CHV_C6_17x17_Type3  = 0xAA,   
    CHV_C6_CoPOP        = 0xAB,   
    CHV_C7_17x17_Type4  = 0xAC,   
    CHV_C7_25x27        = 0xAD,   
    CHV_C7_17x17_Type3  = 0xAE,   
    CHV_C7_CoPOP        = 0xAF,   
    CHV_D0_17x17_Type4  = 0x30,   
    CHV_D0_25x27        = 0x31,   
    CHV_D0_17x17_Type3  = 0x32,   
    CHV_D0_CoPOP        = 0x33,   
    CHV_D1_17x17_Type4  = 0x34,   
    CHV_D1_25x27        = 0x35,   
    CHV_D1_17x17_Type3  = 0x36,   
    CHV_D1_CoPOP        = 0x37,   
    CHV_K0_17x17_Type4  = 0x40,   
    CHV_K0_25x27        = 0x41,   
    CHV_K0_17x17_Type3  = 0x42,   
    CHV_K0_CoPOP        = 0x43,   
    CHV_K1_17x17_Type4  = 0x44,   
    CHV_K1_25x27        = 0x45,   
    CHV_K1_17x17_Type3  = 0x46,   
    CHV_K1_CoPOP        = 0x47,   
    CHV_K2_17x17_Type4  = 0x48,   
    CHV_K2_25x27        = 0x49,   
    CHV_K2_17x17_Type3  = 0x4A,   
    CHV_K2_CoPOP        = 0x4B,   
    CHV_K3_17x17_Type4  = 0x4C,   
    CHV_K3_25x27        = 0x4D,   
    CHV_K3_17x17_Type3  = 0x4E,   
    CHV_K3_CoPOP        = 0x4F,   
    CHV_K4_17x17_Type4  = 0xC0,   
    CHV_K4_25x27        = 0xC1,   
    CHV_K4_17x17_Type3  = 0xC2,   
    CHV_K4_CoPOP        = 0xC3,   
    CHV_K5_17x17_Type4  = 0xC4,   
    CHV_K5_25x27        = 0xC5,   
    CHV_K5_17x17_Type3  = 0xC6,   
    CHV_K5_CoPOP        = 0xC7,   
    CHV_K6_17x17_Type4  = 0xC8,   
    CHV_K6_25x27        = 0xC9,   
    CHV_K6_17x17_Type3  = 0xCA,   
    CHV_K6_CoPOP        = 0xCB,   
    CHV_K7_17x17_Type4  = 0xCC,   
    CHV_K7_25x27        = 0xCD,   
    CHV_K7_17x17_Type3  = 0xCE,   
    CHV_K7_CoPOP        = 0xCF   
} CHV_GFX_REVISION_ID;


unsigned short ConvertChvRevId(unsigned short usRevId)
{
    switch (usRevId)
    {
        
    case CHV_A0_17x17_Type4:
    case CHV_A0_25x27:
    case CHV_A0_17x17_Type3:
    case CHV_A0_CoPOP:
        return REVISION_A0;
    case CHV_A1_17x17_Type4:
    case CHV_A1_25x27:
    case CHV_A1_17x17_Type3:
    case CHV_A1_CoPOP:
        return REVISION_A1;
    case CHV_A2_17x17_Type4:
    case CHV_A2_25x27:
    case CHV_A2_17x17_Type3:
    case CHV_A2_CoPOP:
    case CHV_A3_17x17_Type4:
    case CHV_A3_25x27:
    case CHV_A3_17x17_Type3:
    case CHV_A3_CoPOP:
    case CHV_A4_17x17_Type4:
    case CHV_A4_25x27:
    case CHV_A4_17x17_Type3:
    case CHV_A4_CoPOP:
    case CHV_A5_17x17_Type4:
    case CHV_A5_25x27:
    case CHV_A5_17x17_Type3:
    case CHV_A5_CoPOP:
    case CHV_A6_17x17_Type4:
    case CHV_A6_25x27:
    case CHV_A6_17x17_Type3:
    case CHV_A6_CoPOP:
    case CHV_A7_17x17_Type4:
    case CHV_A7_25x27:
    case CHV_A7_17x17_Type3:
    case CHV_A7_CoPOP:
#if(_DEBUG || _RELEASE_INTERNAL)
    case CHV_HAS_A:
    case CHV_HAS_A_Backward_Compatibile:
#endif
        return REVISION_A3;
        break;
        
    case CHV_B0_17x17_Type4:
    case CHV_B0_25x27:
    case CHV_B0_17x17_Type3:
    case CHV_B0_CoPOP:
    case CHV_B1_17x17_Type4:
    case CHV_B1_25x27:
    case CHV_B1_17x17_Type3:
    case CHV_B1_CoPOP:
    case CHV_B2_17x17_Type4:
    case CHV_B2_25x27:
    case CHV_B2_17x17_Type3:
    case CHV_B2_CoPOP:
    case CHV_B3_17x17_Type4:
    case CHV_B3_25x27:
    case CHV_B3_17x17_Type3:
    case CHV_B3_CoPOP:
    case CHV_B4_17x17_Type4:
    case CHV_B4_25x27:
    case CHV_B4_17x17_Type3:
    case CHV_B4_CoPOP:
    case CHV_B5_17x17_Type4:
    case CHV_B5_25x27:
    case CHV_B5_17x17_Type3:
    case CHV_B5_CoPOP:
    case CHV_B6_17x17_Type4:
    case CHV_B6_25x27:
    case CHV_B6_17x17_Type3:
    case CHV_B6_CoPOP:
    case CHV_B7_17x17_Type4:
    case CHV_B7_25x27:
    case CHV_B7_17x17_Type3:
    case CHV_B7_CoPOP:
#if(_DEBUG || _RELEASE_INTERNAL)
    case CHV_HAS_B:
#endif
        return REVISION_B;
        break;
        
    case CHV_C0_17x17_Type4:
    case CHV_C0_25x27:
    case CHV_C0_17x17_Type3:
    case CHV_C0_CoPOP:
    case CHV_C1_17x17_Type4:
    case CHV_C1_25x27:
    case CHV_C1_17x17_Type3:
    case CHV_C1_CoPOP:
    case CHV_C2_17x17_Type4:
    case CHV_C2_25x27:
    case CHV_C2_17x17_Type3:
    case CHV_C2_CoPOP:
    case CHV_C3_17x17_Type4:
    case CHV_C3_25x27:
    case CHV_C3_17x17_Type3:
    case CHV_C3_CoPOP:
    case CHV_C4_17x17_Type4:
    case CHV_C4_25x27:
    case CHV_C4_17x17_Type3:
    case CHV_C4_CoPOP:
    case CHV_C5_17x17_Type4:
    case CHV_C5_25x27:
    case CHV_C5_17x17_Type3:
    case CHV_C5_CoPOP:
    case CHV_C6_17x17_Type4:
    case CHV_C6_25x27:
    case CHV_C6_17x17_Type3:
    case CHV_C6_CoPOP:
    case CHV_C7_17x17_Type4:
    case CHV_C7_25x27:
    case CHV_C7_17x17_Type3:
    case CHV_C7_CoPOP:
#if(_DEBUG || _RELEASE_INTERNAL)
    case CHV_HAS_C:
#endif
        return REVISION_C;
        break;
        
    case CHV_D0_17x17_Type4:
    case CHV_D0_25x27:
    case CHV_D0_17x17_Type3:
    case CHV_D0_CoPOP:
    case CHV_D1_17x17_Type4:
    case CHV_D1_25x27:
    case CHV_D1_17x17_Type3:
    case CHV_D1_CoPOP:
#if(_DEBUG || _RELEASE_INTERNAL)
    case CHV_HAS_D:
#endif
        return REVISION_D;
        break;
        
    case CHV_K0_17x17_Type4:
    case CHV_K0_25x27:
    case CHV_K0_17x17_Type3:
    case CHV_K0_CoPOP:
    case CHV_K1_17x17_Type4:
    case CHV_K1_25x27:
    case CHV_K1_17x17_Type3:
    case CHV_K1_CoPOP:
    case CHV_K2_17x17_Type4:
    case CHV_K2_25x27:
    case CHV_K2_17x17_Type3:
    case CHV_K2_CoPOP:
    case CHV_K3_17x17_Type4:
    case CHV_K3_25x27:
    case CHV_K3_17x17_Type3:
    case CHV_K3_CoPOP:
    case CHV_K4_17x17_Type4:
    case CHV_K4_25x27:
    case CHV_K4_17x17_Type3:
    case CHV_K4_CoPOP:
    case CHV_K5_17x17_Type4:
    case CHV_K5_25x27:
    case CHV_K5_17x17_Type3:
    case CHV_K5_CoPOP:
    case CHV_K6_17x17_Type4:
    case CHV_K6_25x27:
    case CHV_K6_17x17_Type3:
    case CHV_K6_CoPOP:
    case CHV_K7_17x17_Type4:
    case CHV_K7_25x27:
    case CHV_K7_17x17_Type3:
    case CHV_K7_CoPOP:
#if(_DEBUG || _RELEASE_INTERNAL)
    case CHV_HAS_K:
#endif
        return REVISION_K;
        break;
    default:
        
        break;
    }
    return usRevId;
}


void InitChvWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_CHV, ulStepId_PCH;
#ifdef __KCH
    KCHASSERT(NULL != pWaParam);
#endif
    ulStepId_CHV =(unsigned int) ( 1 << ConvertChvRevId( pWaParam->usRevId) ); 
    ulStepId_PCH =(unsigned int) ( 1 << ConvertChvRevId( pWaParam->usRevId_PCH ) ); 

    /******************************************************************************\
     * A-Step ( A-Step CHV bugs ported from BDW which started at B-step )
    \******************************************************************************/
    
    WA_ENABLE(
        ulStepId_CHV,
        WaForceTypeConvertF32To16ToAlign1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaIncreaseDefaultTLBEntries,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableAsyncFlipPerfMode,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableNativeDWxDWMultiplication,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableThreadDispatchToEU0,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CHV_A3);

    /*****************************************************************************\
     * SIWA_FOREVER ( BDW B0+ Was or Permanent WA's)
    \*****************************************************************************/
    

    WA_ENABLE(
        ulStepId_CHV,
        Wa4x4STCOptimizationDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMSFAfterWalkerWithoutSLMorBarriers,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableLockForTranscodePerf,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaJPEGHeightAlignYUV422H2YToNV12,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaVC1UnequalFieldHeights,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaAddMediaStateFlushCmd,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaReadVcrDebugRegister,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisablePreemptionMMIOWhenBarrierEnabled,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaInhibitPreemptionForOCLProfiling,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaAddVC1StuffingBytesForSPMP,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaParseVC1PicHeaderInSlice,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaParseVC1FirstFieldPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaParseVC1BPictureHeader,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaIs64BInstrEnabled,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableNonStallingScoreboard,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaAllocateSLML3CacheCtrlOverride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaDisableDSHEncryptionForWiDi,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaInsertAVCFrameForFormatSwitchToJPEG,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaVC1DecodingMaxResolution,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableGroupIDLoopSelect,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaL3WriteIncomplete,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

     WA_ENABLE(
        ulStepId_CHV,
        WaIndirectDispatchPredicate,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

     WA_ENABLE(
        ulStepId_CHV,
        WaDisableDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    
     WA_ENABLE(
        ulStepId_CHV,
        WaRsConsecutiveOptimizedWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

     
     WA_ENABLE(
         ulStepId_CHV,
         WaDisableDPSTWhenPipeBandCActive,
         "No HWBugLink provided",
         "No Link Provided" ,
         PLATFORM_ALL,
         SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaVeboxSliceEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaPMICRegisterReadWrite,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnablebyPassModePllForMIPI,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE_NO_PLATFORM_CHECK(
        ulStepId_CHV,
        WaMipiDPOUnitClkGateEnable,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDummyDMVBufferForMVCInterviewPred,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableHuCBasedDRM,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

#ifdef WIN32
    WA_ENABLE(
        ulStepId_CHV,
        WaAssumeSubblockPresent,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
#endif

    WA_ENABLE(
        ulStepId_CHV,
        WaOCLEnableFMaxFMinPlusZero,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaAdditionalMovWhenSrc1ModOnMulMach,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceMulSrc1WordToAlign1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaLowPrecWriteRTOnlyFloat,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaScalarAtomic,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaNearestFilterLODClamp,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaFlushOpOnCSStall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableRSBeforeBTPoolDisable,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaA32StatelessMessagesRequireHeader,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaGuardbandSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCallForcesThreadSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaThreadSwitchAfterCall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceNonZeroSBEOutputAttributeCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaNullVertexBufferWhenZeroSize,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCsStallBeforeStateCacheInvalidate,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnableEwaLodOnlyForAniso,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_A1);

#define  SI_WA_VALUE SI_WA_NEVER
    
    WA_ENABLE(
        ulStepId_CHV,
        WaDisableIndirectDataForIndirectDispatch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_VALUE
        );
    
#undef SI_WA_VALUE

    WA_ENABLE(
        ulStepId_CHV,
        WaVSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDSRefCountFullforceMissDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisable_RenderCache_OperationalFlush,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_CHV,
        WaFastClearRegistersInPrivilegedList,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSubtract1FromMaxNoOfThreads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaBlockMsgChannelDuringGfxReset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaFbcAsynchFlipDisableFbcQueue,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableAsyncFlipPerfMode,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceSyncFlipWithVisibilityOff,
        "Unknown",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    #if(LHDM)
    WA_ENABLE(
        ulStepId_CHV,
        WaDisplayPitchPowerOf2,
        "No HWBugLink provided",
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);
    #endif

    WA_ENABLE(
        ulStepId_CHV,
        WaClearCCStatePriorPipelineSelect,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSendDummyVFEafterPipelineSelect,
        "No Link Provided" ,
        "Unknown",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaPreventHSTessLevelsInterference,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaStateBindingTableOverfetch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaClearNotificationRegInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRestoreFc0RegistersWithOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaRestoreFC4RegisterDW0fromDW1,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaGrfScoreboardClearInGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaClearFlowControlGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaClearCr0SpfInGpgpuContextRestore,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaStoreAcc2to9InAlign16InGpgpuContextSave,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableDbg0Register,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnsureMemCoherencyBeforeLoadRegisterMem,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaClearArfDependenciesBeforeEot,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEmitVtxWhenOutVtxCntIsZero,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaSetVfGuardbandPreemptionVertexCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRsNullConstantBuffer,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaUseNonPrivRegisterForObjectLevelPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableObjectLevelPreemptionSequentialMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableObjectLevelPreemptionForDraw,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

     WA_ENABLE(
        ulStepId_CHV,
        WaFlushCoherentL3CacheLinesAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaClearSlmSpaceAtContextSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableRc6OnWaitForEvent,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableGunitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaSetMaskForGfxBusyness,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaRsConsecutiveOptimizedWrite,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaPerCtxtBbInvalidateRoCaches,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisablePwrmtrEvent,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaInvalidateROCacheOnCxtSwitch,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableSDEUnitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableSemaphoreAndSyncFlipWait,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableCSUnitClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSetPipeControlCSStallforGPGPUAndMediaWorkloads,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

       WA_ENABLE(
        ulStepId_CHV,
        WaForceMinMaxGSThreadCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCsStallBeforeNonZeroInstanceCount,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSendExtraRSGatherConstantAndRSStoreImmCmds,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSendPipeControlWithProtectedMemoryDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    /*WA_ENABLE(
        ulStepId_CHV,
        WaDisableIndirectDataAndFlushGPGPUWalker,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);*/

    WA_ENABLE(
        ulStepId_CHV,
        WaAvoid16KWidthForTiledSurfaces,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaAllowUMDToModifyHDCChicken1,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableSamplerRoundingDisableFix,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaUseVAlign16OnTileXYBpp816,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        Wa32bitGeneralStateOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        Wa32bitInstructionBaseOffset,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        AccWrEnNotAllowedToAcc1With16bit,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSetDepthToArraySizeForUAV,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaClearRenderResponseMasks,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDividePSInvocationCountBy4,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaAvoidPMAStall,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaDisableMidThreadPreempt,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    
    WA_ENABLE(
        ulStepId_CHV,
        WaDisableObjectLevelPreempt,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaUAVCoherency,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaUseGfxModeFor3DPreemptionGranularity,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaFDIAutoLinkSetTimingOverrride,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableThreadStallDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisablePartialInstShootdown,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaHdcDisableFetchWhenMasked,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisDdbClkWhileProgVisibilityOn,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaEDPModeSetSequenceChange,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMIPIPhaseDelay,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A3);

    WA_ENABLE(
        ulStepId_CHV,
        WaCSCDisable,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMIPIDSIRegRead,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMIPICControlRegAndTERegRead,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaHPDPeriodIncorrect,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRealignCursorC,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaPlaneCxTilingFailure,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaS3DSoftwareMode,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMPhyProgramming,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaBreakF32MixedModeIntoSimd8,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDstSubRegNumNotAllowedWithLowPrecPacked,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableFenceDestinationToSLM,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceEnableNonCoherent,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnableVMEReferenceWindowCheck,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaUpdateFBCasPlaneStatusChanges,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_DESKTOP,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnableGunitDisplayInterruptMask,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaL3UseSamplerForVectorLoadScatter,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaProgramMgsrForCorrectSliceSpecificMmioReads,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableEuBypassOnSimd16Float16,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableEuBypassOnSimd16Float32,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_B7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableMixedModeLog,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableMixedModeFdiv,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableMixedModePow,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaLLCCachingUnsupported,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableFfDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaSkipInvalidSubmitsFromOS,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableSamplerPowerBypass,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceL3LlcCoherencyInDescriptor,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    
    if( pSkuTable->FtrGpGpuMidThreadLevelPreempt )
    {
        WA_ENABLE(
            ulStepId_CHV,
            WAGPGPUMidThreadPreemption,
            "No HWBugLink provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_CHV_A7);
    }


    WA_ENABLE(
        ulStepId_CHV,
        WaEngineResetAfterMidThreadPreemption,
        "No Link Provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaModifyVFEStateAfterGPGPUPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMSFWithNoWatermarkTSGHang,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaIntegerDivisionSourceModifierNotSupported,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaHucStreamoutEnable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaHucStreamoutOnlyDisable,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSSCClockOverride,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
    WA_ENABLE(
        ulStepId_CHV,
        WaCCKClkSkuIncorrect,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);
    WA_ENABLE(
        ulStepId_CHV,
        WaOGLGSVertexReordering,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableCtxRestoreArbitration,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaVfPostSyncWrite,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaForceDX10BorderColorSampleC,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRsRestoreWithPerCtxtBb,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_B7);

    WA_ENABLE(
        ulStepId_CHV,
        WaPixelRepeatModeFixForC0,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_CHV_C0);

    WA_ENABLE(
        ulStepId_CHV,
        WaOGLGSVertexReordering,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableObjectLevelPreemptionForGSLineStripAdj,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaEnableOnlyGpgpuCommandLevelPreemption,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        Wa32bitGeneralStateOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        Wa32bitInstructionBaseOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableRowChickenDopClockGating,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableNullDepthBuffer,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

     WA_ENABLE(
        ulStepId_CHV,
        WaDisableInstructionShootdown,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableGPGPUSlmPerfFix,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        Wa16TileFencesOnly,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaSetDVMSizeToStolenMemorySize,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRsUseTimeoutMode,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaGsvAxSteppingRegisters,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaReportPerfCountUseGlobalContextID,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaNoMinimizedTrivialSurfacePadding,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCur180BufAddressCalcForPipeA,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCHVBxMPOHWSupport,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_A7);

    WA_ENABLE(
        ulStepId_CHV,
        WaCHVKxPlaneScalarSupport,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CHV_B7);

    WA_ENABLE(
        ulStepId_CHV,
        WaCHVMPO,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaCHVDisableScalar,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaMixModeSelInstDstNotPacked,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaIndependentAlphaBlend,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);



    WA_ENABLE(
        ulStepId_CHV,
        WaIdleLiteRestore,
        "No Link Provided" ,
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER );



    WA_ENABLE(
        ulStepId_CHV,
        WaDisableSamplerL2BypassForTextureCompressedFormats,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER );

    WA_ENABLE(
        ulStepId_CHV,
        WaReInitRingContextLriPostedWrite,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

#ifdef ANDROID
    WA_ENABLE(
        ulStepId_CHV,
        WaC0AstcCorruptionForOddCompressedBlockSizeX,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_CHV_C0);
#endif

    WA_ENABLE(
        ulStepId_CHV,
        WaAstcCorruptionForOddCompressedBlockSizeX,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FROM_CHV_D0);

    
    WA_ENABLE(
        ulStepId_CHV,
        WaEnableHGAsyncFlipLinearToTileConvert,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaShaderCalculateResourceOffsets,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaDisableDeepLoopsUnrolling,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaGlobalDepthConstantScaleUp,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaRasterisationOfDegenerateTriangles,
        "No HWBugLink provided",
        "No Link Provided" ,
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_CHV,
        WaResetPSDoesNotWriteToRT,
        "No Link Provided" ,
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
}

#ifdef __KCH
void InitChvHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_CHV, ulStepId_PCH;
    unsigned int ulRegdata = 0;

    ulStepId_CHV = ( 1 << ConvertChvRevId( pWaParam->usRevId) ); 
    ulStepId_PCH = ( 1 << ConvertChvRevId( pWaParam->usRevId_PCH ) ); 

    
    WA_ENABLE(
        ulStepId_CHV,
        WaHas2CSLStubDisplay,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);
}

void InitChvSLEWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

    

}


#endif 
