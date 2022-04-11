/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "inc/common/igfxfmid.h"

namespace IGC {
    void getPrologueShader(
        PRODUCT_FAMILY eProductFamily,
        void*& pKernel,
        unsigned& size);
}
