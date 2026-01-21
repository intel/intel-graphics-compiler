/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/exp10_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/exp10_d_la.cl"
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable)) __spirv_ocl_exp10( float x )
{
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath))
    {
        return __spirv_ocl_native_exp10(x);
    }
    else
    {
        return __ocl_svml_exp10f(x);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_exp10( double x )
{
    return __ocl_svml_exp10(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_exp10( half x )
{
    return __spirv_ocl_exp10((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)

