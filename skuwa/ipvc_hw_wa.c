/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "wa_def.h"
#include "ipvc_rev_id.h"


void InitPvcHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_PVC_ComputeTile = (int)pWaParam->usRevId & 0b111;
    int iStepId_PVC_BaseDie = ((int)pWaParam->usRevId & 0b111000) >> 3;

    if (iStepId_PVC_ComputeTile == 0x1)
    {


        iStepId_PVC_ComputeTile = 0x0;
    }


    SI_WA_ENABLE(
        Wa_1507979211,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_14010017096,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_14012420496,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_22010487853,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_22010493955,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_22010725011,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTA0));


    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0P, PVC_GT_REV_ID_COMPUTETILE_XTB0));

    SI_WA_ENABLE(
        Wa_16011698357,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_XTA0, PVC_GT_REV_ID_COMPUTETILE_XTB0));

    SI_WA_ENABLE(
        Wa_14013677893,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_ComputeTile, PVC_GT_REV_ID_COMPUTETILE_A0, PVC_GT_REV_ID_COMPUTETILE_XTB0));
}
