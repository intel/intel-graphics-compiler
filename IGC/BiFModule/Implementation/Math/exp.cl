/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/exp_d_la.cl"
    #include "../IMF/FP64/exp_d_la_noLUT.cl"
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable)) __spirv_ocl_exp(float x)
{
    if (BIF_FLAG_CTRL_GET(FastRelaxedMath))
    {
        return __spirv_ocl_native_exp(x);
    }
    else
    {
        // e^x = 2^(log2(e^x)) = 2^(x * log2(e))
        // We'll compute 2^(x * log2(e)) by splitting x * log2(e)
        //   into a whole part and fractional part.

        // Compute the whole part of x * log2(e)
        // This part is easy!
        float w = __spirv_ocl_trunc( x * M_LOG2E_F );

        // Compute the fractional part of x * log2(e)
        // We have to do this carefully, so we don't lose precision.
        // Compute as:
        //   fract( x * log2(e) ) = ( x - w * C1 - w * C2 ) * log2(e)
        // C1 is the "Cephes Constant", and is close to 1/log2(e)
        // C2 is the difference between the "Cephes Constant" and 1/log2(e)
        const float C1 = as_float( 0x3F317200 );    // 0.693145751953125
        const float C2 = as_float( 0x35BFBE8E );    // 0.000001428606765330187
        float f = x;
        f = __spirv_ocl_fma( w, -C1, f );
        f = __spirv_ocl_fma( w, -C2, f );
        f = f * M_LOG2E_F;

        w = __spirv_ocl_native_exp2( w );   // this should be exact
        f = __spirv_ocl_native_exp2( f );   // this should be close enough

        float res = w * f;
        res = ( x < as_float( 0xC2D20000 ) ) ? as_float( 0x00000000 ) : res;
        res = ( x > as_float( 0x42D20000 ) ) ? as_float( 0x7F800000 ) : res;

        return res;
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_exp( double x )
{
    double result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_exp_noLUT(x);
    } else {
        result = __ocl_svml_exp(x);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( exp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_exp( half x )
{
    return __spirv_ocl_exp((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp, half, half, f16 )

#endif // defined(cl_khr_fp16)

