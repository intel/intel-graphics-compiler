/*========================== begin_copyright_notice ============================

Copyright (C) 2013-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


void InitBxtWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_BXT = (int)pWaParam->usRevId;


    SI_WA_ENABLE(

        WaOCLEnableFMaxFMinPlusZero,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(

        WaSamplerResponseLengthMustBeGreaterThan1,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(
        WaEnablePooledEuFor2x6,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));


    SI_WA_ENABLE(

        WaClearArfDependenciesBeforeEot,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDoNotPushConstantsForAllPulledGSTopologies,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(

        WaThreadSwitchAfterCall,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(

        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));

    SI_WA_ENABLE(

        WaDisallow64BitImmMov,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(

        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

    SI_WA_ENABLE(

        WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(

        WaDisableEuBypassOnSimd16Float32,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(

        WaConservativeRasterization,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_B0));


    SI_WA_ENABLE(
        WaDisableSIMD16On3SrcInstr,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_B0));


    SI_WA_ENABLE(

        WaResetN0BeforeGatewayMessage,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4) {

    }


    SI_WA_ENABLE(

        WaHeaderRequiredOnSimd16Sample16bit,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));


    SI_WA_ENABLE(

        WaSrc1ImmHfNotAllowed,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(StepId_BXT, BXT_REV_ID_A0));

    SI_WA_ENABLE(

        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(

        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BEFORE(StepId_BXT, BXT_REV_ID_C0));


    if (pSkuTable->FtrGpGpuMidThreadLevelPreempt)
    {

    }


    SI_WA_ENABLE(

        WaMixModeSelInstDstNotPacked,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    if (pWaParam->bWinDoD)
    {

    }


}

#ifdef __KCH
void InitBxtHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_BXT = (int)pWaParam->usRevId;
    (void)StepId_BXT;


}
#endif
