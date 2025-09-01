/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "iacm_g10_rev_id.h"


void InitAcm_G10HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_ACM_G10 = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_1609337769,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14010198302,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_1608127078,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14010875903,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_22010811838,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012420496,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012504847,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012760189,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16011983264,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14013297064,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_C0));


    SI_WA_ENABLE(
        Wa_14013672992,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18012660806,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14014414195,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14012437816,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14014595444,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16013338947,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22013880840,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_15010203763,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14016880151,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_C0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18023229625,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_C0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14017131883,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_C0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16011859583,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16012061344,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18012201914,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14012562260,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_18013852970,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, ACM_G10_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14012688258,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16012292205,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18015444900,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22012856258,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22013689345,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22014559856,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G10, ACM_G10_GT_REV_ID_A0, FUTURE_PROJECT));


}
