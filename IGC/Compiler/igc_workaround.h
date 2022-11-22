/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "inc/common/sku_wa.h"
#include "inc/common/igfxfmid.h"
#include "skuwa/wa_def.h"
#include "Compiler/CISACodeGen/Platform.hpp"

namespace IGC
{
    void SetWorkaroundTable(SKU_FEATURE_TABLE* pSkuFeatureTable, CPlatform* platform);
    void SetWorkaroundTable(const SUscSkuFeatureTable*, CPlatform* platform);
    void SetGTSystemInfo(const SUscGTSystemInfo*, CPlatform* platform);
    void SetGTSystemInfo(const GT_SYSTEM_INFO*, CPlatform* platform);
}
