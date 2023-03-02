/*========================== begin_copyright_notice ============================

Copyright (C) 2013-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


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


void InitSklWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_SKL, ulStepId_PCH;
    unsigned int usHwRevId_SKL = pWaParam->usRevId;

    ulStepId_SKL = (1 << usHwRevId_SKL);
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH);


    if ((pWaParam->ePCHProductFamily >= PCH_LPT) &&
        (pWaParam->ePCHProductFamily <= PCH_CNP_H))
    {

    }


    if (pWaParam->ePCHProductFamily == PCH_SPT)
    {


#ifndef  _USC_
        if (pWaParam->usRevId_PCH == SKL_PCH_SPT_A0_REV_ID && pSkuTable->FtrULT)
        {
            pWaTable->WaSPTMmioAccessSbi = 1;
        }
#endif
        WA_ENABLE(
            ulStepId_PCH,
            WaSPTMmioReadFailure,
            "No Link Provided",
            "No Link Provided",
            PLATFORM_ALL,
            SIWA_FOREVER);

        if ((pSkuTable->FtrDesktop && pWaParam->usRevId_PCH >= SKL_PCH_SPT_D0_REV_ID) || ((!pSkuTable->FtrDesktop) && pWaParam->usRevId_PCH >= SKL_PCH_SPT_C0_REV_ID))
        {
            pWaTable->WaSPTMmioReadFailure = 0;
        }


    }

    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {

    }


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
    {

    }


    WA_ENABLE(
        ulStepId_SKL,
        WaSamplerResponseLengthMustBeGreaterThan1,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_F0);


    WA_ENABLE(
        ulStepId_SKL,
        WaClearArfDependenciesBeforeEot,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDoNotPushConstantsForAllPulledGSTopologies,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4) {

    }


    WA_ENABLE(
        ulStepId_SKL,
        WaThreadSwitchAfterCall,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_SKL,
        WaDisableDSDualPatchMode,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisallow64BitImmMov,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);


    WA_ENABLE(
        ulStepId_SKL,
        WaNoA32ByteScatteredStatelessMessages,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);


    WA_ENABLE(
        ulStepId_SKL,
        WaDisableEuBypassOnSimd16Float32,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);


    WA_ENABLE(
        ulStepId_SKL,
        WaConservativeRasterization,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT4)
    {

    }


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
    {


    }


#if (_DEBUG || _RELEASE_INTERNAL)


#endif


    WA_ENABLE(
        ulStepId_SKL,
        WaOCLEnableFMaxFMinPlusZero,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaHeaderRequiredOnSimd16Sample16bit,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);


    WA_ENABLE(
        ulStepId_SKL,
        WaSrc1ImmHfNotAllowed,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_D0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSendsSrc0DstOverlap,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableSIMD16On3SrcInstr,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        (SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0));


    if (pSkuTable->FtrGT2 || pSkuTable->FtrGT1)
    {

    }
    if (pSkuTable->FtrGT3)
    {

    }


    WA_ENABLE(
        ulStepId_SKL,
        WaSendsSrc1SizeLimitWhenEOT,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_A0);


    WA_ENABLE(
        ulStepId_SKL,
        WaDstSubRegNumNotAllowedWithLowPrecPacked,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_A0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModeLog,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModePow,
        "No Link Provided",

        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaDisableMixedModeFdiv,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);

    WA_ENABLE(
        ulStepId_SKL,
        WaResetN0BeforeGatewayMessage,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    WA_ENABLE(
        ulStepId_SKL,
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT4)
    {


    }


    WA_ENABLE(
        ulStepId_SKL,
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_SKL,
        WaForceMinMaxGSThreadCount,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_SKL_B0);


    if (pSkuTable->FtrGpGpuMidThreadLevelPreempt)
    {

    }


    if (pSkuTable->FtrIoMmuPageFaulting)
    {
        WA_ENABLE(
            ulStepId_SKL,
            WADisableWriteCommitForPageFault,
            "No Link Provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_UNTIL_SKL_B0);
    }


    WA_ENABLE(
        ulStepId_SKL,
        WaSendSEnableIndirectMsgDesc,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_ONLY_SKL_C0 | SIWA_ONLY_SKL_D0);


    WA_ENABLE(
        ulStepId_SKL,
        WaMixModeSelInstDstNotPacked,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT1 || pSkuTable->FtrGT2)
    {

    }
    if (pSkuTable->FtrGT3)
    {

    }


}

#ifdef __KCH
void InitSklHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_SKL, ulStepId_PCH;
    unsigned int usHwRevId_SKL = pWaParam->usRevId;

    ulStepId_SKL = (1 << usHwRevId_SKL);
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH);


    WA_ENABLE(
        ulStepId_PCH,
        WaSPTMmioAccessSbi,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FALSE);

}
#endif
