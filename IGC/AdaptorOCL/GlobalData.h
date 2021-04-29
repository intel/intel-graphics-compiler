/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "sku_wa.h"
#include "igfxfmid.h"
#include "gtsysinfo.h"

/*****************************************************************************\
Global Data: Platform, Sku Features, and Workaround Table
\*****************************************************************************/
typedef struct _SGlobalData
{
    const PLATFORM*          pPlatform;   // Target platform
    const SKU_FEATURE_TABLE* pSkuTable;   // SKU table
    const WA_TABLE*          pWaTable;    // WA table
    const GT_SYSTEM_INFO*    pSysInfo;    // GtType

    // Profiling timer resolution depending on platform
    float                    ProfilingTimerResolution;
} SGlobalData;
