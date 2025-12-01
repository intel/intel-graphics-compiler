/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "igt_30_04_rev_id.h"


void InitGt_30_04HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_GT_30_04 = (int)pWaParam->usRenderRevID;


    SI_WA_ENABLE(
        Wa_18035690555,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_30_04, GT_30_04_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22019804511,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_30_04, GT_30_04_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22016140776,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_30_04, GT_30_04_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_18042479026,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_30_04, GT_30_04_REV_ID_A0, FUTURE_PROJECT));


}
