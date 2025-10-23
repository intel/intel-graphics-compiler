/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/expm1_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/expm1_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_expm1( float x )
{
    return __ocl_svml_expm1f(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_expm1( double x )
{
    return __ocl_svml_expm1(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_expm1( half x )
{
    return __spirv_ocl_expm1((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, half, half, f16 )

#endif // defined(cl_khr_fp16)
