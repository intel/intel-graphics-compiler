/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/cosh_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/cosh_d_la.cl"
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable)) __spirv_ocl_cosh( float x )
{
    float result;

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // Implemented as 0.5 * ( exp(x) + exp(-x) ).
        float pexp = __spirv_ocl_exp(  x );
        float nexp = __spirv_ocl_exp( -x );
        result = 0.5f * ( pexp + nexp );
    }
    else
    {
        result = __ocl_svml_coshf(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cosh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_cosh( double x )
{
    return __ocl_svml_cosh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cosh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_cosh( half x )
{
    return __spirv_ocl_cosh((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cosh, half, half, f16 )

#endif // defined(cl_khr_fp16)

