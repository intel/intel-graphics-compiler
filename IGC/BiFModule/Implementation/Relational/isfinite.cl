/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

int OVERLOADABLE __intel_relaxed_isfinite( float x )
{
    int result = SPIRV_BUILTIN(IsFinite, _f32, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 1 : result;
}

#if defined(cl_khr_fp64)
int OVERLOADABLE __intel_relaxed_isfinite( double x )
{
    int result = SPIRV_BUILTIN(IsFinite, _f64, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 1 : result;
}
#endif // defined(cl_khr_fp64)

int OVERLOADABLE __intel_relaxed_isfinite( half x )
{
    int result = SPIRV_BUILTIN(IsFinite, _f16, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return BIF_FLAG_CTRL_GET(FastRelaxedMath) ? 1 : result;
}

