/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

#define XE_HP_SDV_GT_REV_ID_A0   SI_REV_ID(0,0)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitXeHPSDVSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_XeHP_SDV = (int)pWaParam->usRevId;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              XeHP_SDV SW WA for all platforms
    //
    //=================================================================================================================
}

#ifdef __KCH
void InitXeHPSDVHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
}
#endif // __KCH
