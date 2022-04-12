/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"
#include "iacm_g11_rev_id.h"


void InitAcm_G11HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_ACM_G11 = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_16011859583,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012688715,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012760189,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16011983264,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16012061344,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14013297064,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14012688258,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18012660806,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22012766191,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22012856258,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14014414195,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14012437816,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16013338947,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14013672992,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22012532006,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16012292205,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18015444900,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22013689345,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18015444900,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22012532006,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, ACM_G11_GT_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_22012766191,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_22013880840,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_18012201914,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_22014559856,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G11, ACM_G11_GT_REV_ID_A0, FUTURE_PROJECT));
}
