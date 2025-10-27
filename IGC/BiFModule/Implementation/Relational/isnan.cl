/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE __intel_relaxed_isnan( float x )
{
    int result = __spirv_IsNan(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 0 : result;
}

#if defined(cl_khr_fp64)
INLINE int OVERLOADABLE __intel_relaxed_isnan( double x )
{
    int result = __spirv_IsNan(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 0 : result;
}
#endif // defined(cl_khr_fp64)

INLINE int OVERLOADABLE __intel_relaxed_isnan( half x )
{
    int result = __spirv_IsNan(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 0 : result;
}

