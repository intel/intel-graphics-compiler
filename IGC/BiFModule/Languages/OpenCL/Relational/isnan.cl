/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE isnan( float x )
{
    return SPIRV_BUILTIN(IsNan, _f32, )( x );
}

static INLINE int OVERLOADABLE __intel_vector_isnan_helper( float x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isnan( double x )
{
    return SPIRV_BUILTIN(IsNan, _f64, )( x );
}

static INLINE long OVERLOADABLE __intel_vector_isnan_helper( double x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isnan( half x )
{
    return SPIRV_BUILTIN(IsNan, _f16, )( x );
}

static INLINE short OVERLOADABLE __intel_vector_isnan_helper( half x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, short, half )

#endif // defined(cl_khr_fp16)
