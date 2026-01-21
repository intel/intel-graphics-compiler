/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#include "../IMF/FP32/asinh_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/asinh_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __intel_asinh_f32( float x, bool doFast )
{
    float result;

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)) && doFast)
    {
        float abs_x = __spirv_ocl_fabs(x);
        // Implemented as log(x + sqrt(x*x + 1)).
        // Conformance test checks for this "overflow" case, but
        // I don't think we should have to handle it.
        if( abs_x > 1500.0f )
        {
            result = __spirv_ocl_log(abs_x) + M_LN2_F;
        }
        else if (abs_x < 0x1.0p-12) {
            result = abs_x;
        }
        else
        {
            result = abs_x * abs_x + 1.0f;
            result = __spirv_ocl_sqrt( result );
            result = abs_x + result;
            result = __spirv_ocl_log( result );
        }
        result = __spirv_ocl_copysign(result, x);
    }
    else
    {
        result = __ocl_svml_asinhf(x);
    }

    return result;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_asinh( float x )
{
    return __intel_asinh_f32( x, true );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( asinh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_asinh( double x )
{
    return __ocl_svml_asinh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asinh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_asinh( half x )
{
    return __spirv_ocl_asinh((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( asinh, half, half, f16 )

#endif // defined(cl_khr_fp16)

