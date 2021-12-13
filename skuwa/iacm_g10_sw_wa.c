/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "wa_def.h"
#include "idg2_rev_id.h"


void InitDg2SwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_DG2 = (int)pWaParam->usRevId;

    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No HWBugLink provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);


}

#ifdef __KCH
void InitDg2HASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{


}
#endif
