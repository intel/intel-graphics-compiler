/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "ipvc_xt_rev_id.h"


void InitPvc_XtHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


    int iStepId_PVC_XT_ComputeTile = (int)pWaParam->usRevId & 0b111;
    int iStepId_PVC_XT_BaseDie = ((int)pWaParam->usRevId & 0b111000) >> 3;
    (void)iStepId_PVC_XT_BaseDie;


    SI_WA_ENABLE(
        Wa_1507979211,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_14010017096,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_14012420496,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_14012437816,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_22010487853,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_22010493955,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_22010725011,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_NEVER);


    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_XT_ComputeTile, PVC_XT_GT_REV_ID_COMPUTETILE_A0, PVC_XT_GT_REV_ID_COMPUTETILE_B0));


}
