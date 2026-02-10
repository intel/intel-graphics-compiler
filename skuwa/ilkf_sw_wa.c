/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "ilkf_rev_id.h"

#define LKF_PCH_REV_ID_B0_B1 SI_REV_ID(16,16)


void InitLkfSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

    int iStepId_Ilkf = (int)pWaParam->usRevId;
    int iPchStepId_Ilkf = (int)pWaParam->usRevId_PCH;
    (void)iPchStepId_Ilkf;


    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(iStepId_Ilkf, LKF_GT_REV_ID_A0 ));


}

#ifdef __KCH
void InitLkfHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif
