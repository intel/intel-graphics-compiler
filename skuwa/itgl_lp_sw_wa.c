/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

#define TGL_REV_ID_A0   SI_REV_ID(0,0)
#define TGL_REV_ID_B0   SI_REV_ID(3,3)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitTglLpSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_TGL = (int)pWaParam->usRevId;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              TGL SW WA for all platforms
    //
    //=================================================================================================================

    //TODO: Add WA as needed
    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

}

#ifdef __KCH
void InitTgllpHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    //TODO: Add WA as needed
}
#endif // __KCH
