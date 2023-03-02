/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "iicl_lp_rev_id.h"


void InitIclLpSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


    int iStepId_ICL_LP = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_GT_REV_ID_A0, ICL_LP_GT_REV_ID_B0 ));


    if (pWaParam->ePCHProductFamily >= PCH_SPT)
    {

    }


}

#ifdef __KCH
void InitIclLpHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif
