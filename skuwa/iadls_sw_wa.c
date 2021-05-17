/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"
#include "iadls_rev_id.h"

#define ADLS_PCH_A0_REV_ID 0

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitAdlsSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_ADLS = (int)pWaParam->usRevId;
    int PchStepId_Adls = (int)pWaParam->usRevId_PCH;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              ADLS SW WA for all platforms
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
void InitAdlsHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{

}
#endif // __KCH
