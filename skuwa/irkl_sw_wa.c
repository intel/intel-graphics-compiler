/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"
#include "irkl_rev_id.h"

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitRklSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_RKL = (int)pWaParam->usRevId;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              RKL SW WA for all platforms
    //
    //=================================================================================================================

    SI_WA_ENABLE(

        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);
}

#ifdef __KCH
void InitRklHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif // __KCH
