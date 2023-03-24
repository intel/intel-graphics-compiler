/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


#define LKF_REV_ID_A0   SI_REV_ID(0,0)
#define LKF_REV_ID_B0   SI_REV_ID(3,3)

void InitLkfHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_LKF = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1406306137,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_2201674230,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, LKF_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_1406950495,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1805992985,
        "No Link provided",
        "No Link provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_LKF, LKF_REV_ID_A0, FUTURE_PROJECT));
}
