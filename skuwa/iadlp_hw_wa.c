/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "wa_def.h"
#include "iadlp_rev_id.h"

//******************* Main Wa Initializer for ADLP         ********************

void InitAdlpHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_ADLP = (int)pWaParam->usRevId;

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1409392000,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1604402567,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1406609750,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1406664125,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1406337848,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1406872168,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1407528679,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1407901919,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1407917427,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1409600907,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1606932921,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1808850743,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1607871015,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_14010840176,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_14010013414,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1809626530,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_14010595310,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, ADLP_GT_REV_ID_B0));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_16011879768,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_22010594632,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, ADLP_GT_REV_ID_B0));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_16012061344,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ADLP, ADLP_GT_REV_ID_A0, FUTURE_PROJECT));

}
