/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/sinh_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/sinh_d_la.cl"
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable)) __spirv_ocl_sinh( float x )
{
    float result;

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // For most inputs, we'll use the expansion
        //  sinh(x) = 0.5f * ( e^x - e^-x ):
        float pexp = __spirv_ocl_exp(  x );
        float nexp = __spirv_ocl_exp( -x );
        result = 0.5f * ( pexp - nexp );

        // For x close to zero, we'll simply use x.
        // We use 2^-10 as our cutoff value for
        // "close to zero".
        float px = __spirv_ocl_fabs( x );
        result = ( px > as_float(0x3A800000) ) ? result : x;
    }
    else
    {
        result = __ocl_svml_sinhf(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_sinh( double x )
{
    return __ocl_svml_sinh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sinh( half x )
{
    return __spirv_ocl_sinh((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinh, half, half, f16 )

#endif // defined(cl_khr_fp16)

