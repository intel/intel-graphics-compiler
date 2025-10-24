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
        float abs_x = SPIRV_OCL_BUILTIN(fabs, _f32, )(x);
        // Implemented as log(x + sqrt(x*x + 1)).
        // Conformance test checks for this "overflow" case, but
        // I don't think we should have to handle it.
        if( abs_x > 1500.0f )
        {
            result = SPIRV_OCL_BUILTIN(log, _f32, )(abs_x) + M_LN2_F;
        }
        else if (abs_x < 0x1.0p-12) {
            result = abs_x;
        }
        else
        {
            result = abs_x * abs_x + 1.0f;
            result = SPIRV_OCL_BUILTIN(sqrt, _f32, )( result );
            result = abs_x + result;
            result = SPIRV_OCL_BUILTIN(log, _f32, )( result );
        }
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(result, x);
    }
    else
    {
        result = __ocl_svml_asinhf(x);
    }

    return result;
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f32, )( float x )
{
    return __intel_asinh_f32( x, true );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( asinh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f64, )( double x )
{
    return __ocl_svml_asinh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( asinh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(asinh, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( asinh, half, half, f16 )

#endif // defined(cl_khr_fp16)
