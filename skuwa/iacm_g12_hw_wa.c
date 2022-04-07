/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "wa_def.h"
#include "iacm_g12_rev_id.h"

//******************* Main Wa Initializer for ACM_G12      ********************

void InitAcm_G12HwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
        int iStepId_ACM_G12 = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_14014414195,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_16013338947,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_16012061344,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_16012292205,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_22012856258,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_22013689345,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_22013880840,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_18012201914,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));

    SI_WA_ENABLE(
        Wa_14013672992,
        "No link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ACM_G12, ACM_G12_GT_REV_ID_A0, FUTURE_PROJECT));
}
