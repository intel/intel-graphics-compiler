/*========================== begin_copyright_notice ============================

Copyright (C) 2013-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


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
    (void)ulStepId_PCH;

    ulStepId_CNL = (1 << usHwRevId_CNL);
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH);

    if ((pWaParam->ePCHProductFamily >= PCH_LPT) &&
        (pWaParam->ePCHProductFamily <= PCH_CNP_H))
    {

    }


    WA_ENABLE(
        ulStepId_CNL,
        WaConservativeRasterization,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);

    WA_ENABLE(
        ulStepId_CNL,
        WaClearTDRRegBeforeEOTForNonPS,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_C0);

    if (pWaParam->ePCHProductFamily >= PCH_SPT)
    {

    }

    WA_ENABLE(
        ulStepId_CNL,
        Wa_220856683,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_CNL,
        WaForceCB0ToBeZeroWhenSendingPC,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_CNL_A0);


    WA_ENABLE(
        ulStepId_CNL,
        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (!(ulStepId_CNL & SIWA_FROM_CNL_B0))
    {
        pSkuTable->FtrGtPsmi = 0;
    }


    WA_ENABLE(
        ulStepId_CNL,
        WaNoSimd16TernarySrc0Imm,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if ((pWaParam->ePCHProductFamily == PCH_CNP_LP) || (pWaParam->ePCHProductFamily == PCH_CNP_H))
    {

    }


    if (pSkuTable->FtrVcs2)
    {

    }


}

#ifdef __KCH
void InitCnlHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif
