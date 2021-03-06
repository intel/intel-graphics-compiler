/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_rsqrt_f32( float x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_rsqrt_f64( double x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_rsqrt_f16( half x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)
