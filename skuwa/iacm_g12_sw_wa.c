/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "wa_def.h"
#include "iacm_g12_rev_id.h"


void InitAcm_G12SwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_ACM_G12 = (int)pWaParam->usRevId;

    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


}

#ifdef __KCH
void InitAcm_G12HASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


}
#endif // __KCH
