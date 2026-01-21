/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_sqrt( float x )
{
    return __spirv_ocl_native_sqrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, float, float, f32 )

#ifdef cl_fp64_basic_ops

INLINE double __attribute__((overloadable)) __spirv_ocl_sqrt( double x )
{
    return __spirv_ocl_native_sqrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, double, double, f64 )

#endif // cl_fp64_basic_ops

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sqrt( half x )
{
    return __spirv_ocl_native_sqrt((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( sqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

