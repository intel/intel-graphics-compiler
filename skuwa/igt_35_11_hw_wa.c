/*========================== begin_copyright_notice ============================

Copyright (C) 2025-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "igt_35_11_rev_id.h"


void InitGt_35_11HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_GT_35_11 = (int)pWaParam->usRenderRevID;


    SI_WA_ENABLE(
        Wa_14027487226,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_GT_35_11, GT_35_11_REV_ID_A0, FUTURE_PROJECT));


}
