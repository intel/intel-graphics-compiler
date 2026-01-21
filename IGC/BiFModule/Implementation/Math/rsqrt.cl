/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_rsqrt( float x )
{
    return __spirv_ocl_native_rsqrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_rsqrt( double x )
{
    return __spirv_ocl_native_rsqrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_rsqrt( half x )
{
    return __spirv_ocl_native_rsqrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( rsqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

