/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f64, )( double x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f64, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f16, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)
