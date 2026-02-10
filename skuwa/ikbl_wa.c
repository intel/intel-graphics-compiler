/*========================== begin_copyright_notice ============================

Copyright (C) 2015-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


#define KBL_REV_ID_A0   SI_REV_ID(0,0)
#define KBL_REV_ID_B0   SI_REV_ID(1,1)
#define KBL_REV_ID_C0   SI_REV_ID(2,2)
#define KBL_REV_ID_D0   SI_REV_ID(3,3)
#define KBL_REV_ID_F0   SI_REV_ID(4,4)
#define KBL_REV_ID_C1   SI_REV_ID(5,5)
#define KBL_REV_ID_D1   SI_REV_ID(6,6)
#define KBL_REV_ID_G0   SI_REV_ID(7,7)

#define KBL_PCH_SPT_A0_REV_ID     SI_REV_ID(0,0)
#define KBL_PCH_SPT_C0_REV_ID     SI_REV_ID(0x20, 0x20)
#define KBL_PCH_SPT_D0_REV_ID     SI_REV_ID(0x30, 0x30)


void InitKblNonDisplayWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_KBL = (int)pWaParam->usRevId;
    int iStepId_PCH = (int)pWaParam->usRevId_PCH;
    (void)iStepId_KBL;

    if ((pWaParam->ePCHProductFamily >= PCH_LPT) &&
        (pWaParam->ePCHProductFamily <= PCH_CNP_H))
    {

    }


    if (pWaParam->ePCHProductFamily == PCH_SPT)
    {

#ifndef  _USC_
        if (pSkuTable->FtrULT)
        {
            SI_WA_ENABLE(
                WaSPTMmioAccessSbi,
                "No Link Provided",
                "No HWSightingLink provided",
                PLATFORM_ALL,
                SI_WA_ONLY(iStepId_PCH, KBL_PCH_SPT_A0_REV_ID));
        }
#endif

        if (pSkuTable->FtrDesktop)
        {
            SI_WA_ENABLE(
                WaSPTMmioReadFailure,
                "No Link Provided",
                "No Link Provided",
                PLATFORM_ALL,
                SI_WA_BEFORE(iStepId_PCH, KBL_PCH_SPT_D0_REV_ID));
        }
        if (!pSkuTable->FtrDesktop)
        {
            SI_WA_ENABLE(
                WaSPTMmioReadFailure,
                "No Link Provided",
                "No Link Provided",
                PLATFORM_ALL,
                SI_WA_BEFORE(iStepId_PCH, KBL_PCH_SPT_C0_REV_ID));
        }
    }

    if (pWaParam->ePCHProductFamily >= PCH_SPT)
    {

    }

    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {

    }


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4)
    {

    }


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


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4 || pSkuTable->Ftr5Slice) {

    }


    SI_WA_ENABLE(
        WaThreadSwitchAfterCall,
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


    SI_WA_ENABLE(
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(
        WaConservativeRasterization,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    SI_WA_ENABLE(
        WaDispatchGRFHWIssueInGSAndHSUnit,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(iStepId_KBL, KBL_REV_ID_A0));


    if (pSkuTable->FtrGT4)
    {

    }


    SI_WA_ENABLE(
        WaResetN0BeforeGatewayMessage,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


#if (_DEBUG || _RELEASE_INTERNAL)


#endif


    SI_WA_ENABLE(
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


    if (pSkuTable->FtrGT2 || pSkuTable->FtrGT1 || pSkuTable->FtrGT1_5)
    {

    }


    if (pSkuTable->FtrGpGpuMidThreadLevelPreempt)
    {

    }


    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


}

void InitKblDisplayWaTable(
    PWA_TABLE                       pWaTable,
    PSKU_FEATURE_TABLE              pSkuTable,
    PWA_INIT_PARAM                  pWaParam)
{
    int iStepId_KBL = (int)pWaParam->usRevId;
    (void)iStepId_KBL;

    if ((pWaParam->ePCHProductFamily == PCH_CNP_LP) || (pWaParam->ePCHProductFamily == PCH_CNP_H))
    {

    }

}


#ifdef __KCH
void InitKblHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_KBL = (int)pWaParam->usRevId;
    (void)iStepId_KBL;

    SI_WA_ENABLE(
        WaSPTMmioAccessSbi,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);
}
#endif
