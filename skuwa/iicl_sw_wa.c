/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

#define ICL_REV_ID_A0   SI_REV_ID(0,0)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitIclSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    int iStepId_ICL = (int)pWaParam->usRevId;

    //=================================================================================================================
    //
    //              ICL SW WA for all platforms
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
        SI_WA_UNTIL(iStepId_ICL, ICL_REV_ID_A0));
}

#ifdef __KCH
void InitIclHASWaTable( PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam )
{
    //TODO Gen11: Add WA as needed
}
#endif // __KCH
