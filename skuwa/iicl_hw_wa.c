/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


#define ICL_REV_ID_A0   SI_REV_ID(0,0)
#define ICL_REV_ID_A1   SI_REV_ID(1,1)
#define ICL_REV_ID_A2   SI_REV_ID(2,2)
#define ICL_REV_ID_B0   SI_REV_ID(3,3)
#define ICL_REV_ID_C0   SI_REV_ID(5,5)
#define ICL_REV_ID_D0   SI_REV_ID(7,7)
#define ICL_REV_ID_E0   SI_REV_ID(9,9)

void InitIclHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_ICL = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_1406306137,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL, ICL_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_220856683,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL, ICL_REV_ID_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_2201674230,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL, ICL_REV_ID_A0, ICL_REV_ID_B0));


    SI_WA_ENABLE(
        Wa_1406950495,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL, ICL_REV_ID_A0, FUTURE_PROJECT));


}
