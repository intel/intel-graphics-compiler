/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/sinpi_s_la.cl"
#include "../IMF/FP32/sinpi_s_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/sinpi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_sinpi( float x )
{
    bool useNative = BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS));

    float result;
    if(useNative)
    {
        result = __spirv_ocl_sin(x * M_PI_F);
    }
    else
    {
        if(BIF_FLAG_CTRL_GET(UseMathWithLUT))
        {
            result = __ocl_svml_sinpif(x);
        }
        else
        {
            result = __ocl_svml_sinpif_noLUT(x);
        }
    }

    // OpenCL C spec, 7.5.1. Additional Requirements Beyond C99 TC2, requires that:
    //  - "sinpi(+-0) returns +-0",
    //  - "sinpi(-n) returns -0 for negative integers n".
    //  When the result is zero, ensure the sign matches the input.
    result = (result == 0.0f) ? __spirv_ocl_copysign(0.0f, x) : result;
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinpi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_sinpi( double x )
{
    return __ocl_svml_sinpi(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinpi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sinpi( half x )
{
    return __spirv_ocl_sinpi((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinpi, half, half, f16 )

#endif // defined(cl_khr_fp16)

