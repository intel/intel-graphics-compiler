/*========================== begin_copyright_notice ============================

Copyright (C) 2016-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "idg1_rev_id.h"


void InitDg1SwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_DG1 = (int)pWaParam->usRevId;
    (void)StepId_DG1;


    SI_WA_ENABLE(

        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


}
