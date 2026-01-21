/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE int OVERLOADABLE __intel_relaxed_isnan( bfloat x )
{
    int result = __spirv_IsNan(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 0 : result;
}
