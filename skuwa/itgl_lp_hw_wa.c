/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


#define TGL_LP_REV_ID_A0   SI_REV_ID(0,0)
#define TGL_LP_REV_ID_B0   SI_REV_ID(3,3)

void InitTglLpHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_TGL_LP = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_220856683,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1409460247,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1808850743,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1607871015,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16012061344,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_14013672992,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_TGL_LP, TGL_LP_REV_ID_A0, FUTURE_PROJECT));
}
