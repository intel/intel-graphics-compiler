/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE isinf( float x )
{
    return SPIRV_BUILTIN(IsInf, _f32, )( x );
}

static INLINE int OVERLOADABLE __intel_vector_isinf_helper( float x )
{
    return fabs(x) == (float)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isinf, __intel_vector_isinf_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isinf( double x )
{
    return SPIRV_BUILTIN(IsInf, _f64, )( x );
}

static INLINE long OVERLOADABLE __intel_vector_isinf_helper( double x )
{
    return fabs(x) == (double)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isinf, __intel_vector_isinf_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isinf( half x )
{
    return SPIRV_BUILTIN(IsInf, _f16, )( x );
}

static INLINE short OVERLOADABLE __intel_vector_isinf_helper( half x )
{
    return fabs(x) == (half)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isinf, __intel_vector_isinf_helper, short, half )

#endif // defined(cl_khr_fp16)
