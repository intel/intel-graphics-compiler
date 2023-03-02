/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"


#define EHL_REV_ID_A0   SI_REV_ID(0,0)
#define EHL_REV_ID_A1   SI_REV_ID(0,0)
#define EHL_REV_ID_B0   SI_REV_ID(3,3)
#define EHL_REV_ID_C0   SI_REV_ID(6,6)

void InitEhlHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_EHL = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_EHL, EHL_REV_ID_A0, FUTURE_PROJECT));


}
