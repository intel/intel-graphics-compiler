/*========================== begin_copyright_notice ============================

Copyright (C) 2016-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "iacm_g10_rev_id.h"


void InitAcm_G10SwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_ACM_G10 = (int)pWaParam->usRevId;
    (void)StepId_ACM_G10;


    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


}
