/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

#define DG1_REV_ID_A0   SI_REV_ID(0,0)
#define DG1_REV_ID_B0   SI_REV_ID(1,1)

void InitDg1HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_DG1 = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_220856683,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1409392000,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1409460247,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, DG1_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14010017096,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1808850743,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14010595310,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));


    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1607871015,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_16012061344,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_14013672992,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_DG1, DG1_REV_ID_A0, FUTURE_PROJECT));
}
