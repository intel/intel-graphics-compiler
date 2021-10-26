/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE native_log( float x )
{
    return SPIRV_OCL_BUILTIN(native_log, _f32, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_log, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_log( double x )
{
    return SPIRV_OCL_BUILTIN(native_log, _f64, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_log, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE native_log( half x )
{
    return SPIRV_OCL_BUILTIN(native_log, _f16, )( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( native_log, half, half )

#endif // defined(cl_khr_fp16)
