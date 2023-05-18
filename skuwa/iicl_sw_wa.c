/*========================== begin_copyright_notice ============================

Copyright (C) 2015-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"

#define ICL_REV_ID_A0   SI_REV_ID(0,0)
#define ICL_REV_ID_B0   SI_REV_ID(3,3)


void InitIclSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


    int iStepId_ICL = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_UNTIL(iStepId_ICL, ICL_REV_ID_A0));


    if (pWaParam->ePCHProductFamily >= PCH_SPT)
    {

    }


}

#ifdef __KCH
void InitIclHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif
