/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE isnormal( float x )
{
    return SPIRV_BUILTIN(IsNormal, _f32, )( x );
}

static INLINE int OVERLOADABLE __intel_vector_isnormal_helper( float x )
{
    return isfinite(x) & (fabs(x) >= FLT_MIN) ? -1 : 0;
}

int OVERLOADABLE __intel_relaxed_isnormal( float x )
{
    return __intel_relaxed_isfinite(x) & (fabs(x) >= FLT_MIN) ? 1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnormal, __intel_vector_isnormal_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isnormal(double x)
{
    return SPIRV_BUILTIN(IsNormal, _f64, )( x );
}

static INLINE long OVERLOADABLE __intel_vector_isnormal_helper( double x )
{
    return isfinite(x) & (fabs(x) >= DBL_MIN) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnormal, __intel_vector_isnormal_helper, long, double )

int OVERLOADABLE __intel_relaxed_isnormal( double x )
{
    return __intel_relaxed_isfinite(x) & (fabs(x) >= DBL_MIN) ? 1 : 0;
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isnormal( half x )
{
    return SPIRV_BUILTIN(IsNormal, _f16, )( x );
}

static INLINE short OVERLOADABLE __intel_vector_isnormal_helper( half x )
{
    return isfinite(x) & (fabs(x) >= HALF_MIN) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnormal, __intel_vector_isnormal_helper, short, half )

int OVERLOADABLE __intel_relaxed_isnormal( half x )
{
    return __intel_relaxed_isfinite(x) & (fabs(x) >= HALF_MIN) ? 1 : 0;
}

#endif // defined(cl_khr_fp16)
