/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"

#define ICL_LP_REV_ID_A0   SI_REV_ID(0,0)
#define ICL_LP_REV_ID_A2   SI_REV_ID(1,1)
#define ICL_LP_REV_ID_B0   SI_REV_ID(3,3)
#define ICL_LP_REV_ID_B2   SI_REV_ID(4,4)
#define ICL_LP_REV_ID_C0   SI_REV_ID(5,5)


void InitIclLpSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


    int iStepId_ICL_LP = (int)pWaParam->usRevId;


    SI_WA_ENABLE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, ICL_LP_REV_ID_B0));

}

#ifdef __KCH
void InitIclLpHASWaTable( PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam )
{

}
#endif
