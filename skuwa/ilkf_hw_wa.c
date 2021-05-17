/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

//******************* Main Wa Initializer for Device Id ********************

#define LKF_REV_ID_A0   SI_REV_ID(0,0)
#define LKF_REV_ID_B0   SI_REV_ID(3,3)          //placeholder until stepping value is decided

void InitLkfHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_LKF = (int)pWaParam->usRevId;


    // Components affected: igc
    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1406306137,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: visa
    SI_WA_ENABLE(
        Wa_2201674230,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, LKF_REV_ID_B0));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1406950495,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1805992985,
        "No Link provided",
        "No Link provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));
}
