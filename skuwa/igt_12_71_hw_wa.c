/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "igt_12_71_rev_id.h"


void InitGt_12_71HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_GT_12_71 = (int)pWaParam->usRenderRevID;


    SI_WA_ENABLE(
        Wa_18012660806,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_18015444900,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14012437816,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_22013689345,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_16013338947,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_22013880840,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_1608127078,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14016243945,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_B0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14016880151,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14017131883,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_15010203763,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, GT_12_71_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_14019028097,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_12_71, GT_12_71_REV_ID_A0, FUTURE_PROJECT));

}
