/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "iicl_lp_rev_id.h"


void InitIclLpHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_ICL_LP = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_220856683,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1406306137,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1406950495,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_2201674230,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_A0, ICL_LP_GT_REV_ID_C0));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_B0, FUTURE_PROJECT));


}
