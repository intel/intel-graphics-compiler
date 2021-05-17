/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

#define ICL_LP_REV_ID_A0   SI_REV_ID(0,0)
#define ICL_LP_REV_ID_A2   SI_REV_ID(1,1)
#define ICL_LP_REV_ID_B0   SI_REV_ID(3,3)
#define ICL_LP_REV_ID_B2   SI_REV_ID(4,4)
#define ICL_LP_REV_ID_C0   SI_REV_ID(5,5)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitIclLpSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    int iStepId_ICL_LP = (int)pWaParam->usRevId;

    //=================================================================================================================
    //
    //              ICL LP SW WA for all platforms
    //
    //=================================================================================================================

    //=========================
    // IGC WA
    //=========================
    SI_WA_ENABLE(
        WaStructuredBufferAsRawBufferOverride,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER );

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
    //TODO Gen11LP: Add WA as needed
}
#endif // __KCH
