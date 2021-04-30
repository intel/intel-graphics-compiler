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

    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        // Implemented as log(x + sqrt(x*x + 1)).
        // Conformance test checks for this "overflow" case, but
        // I don't think we should have to handle it.
        if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
        {
            result = x * x + 1.0f;
            result = __builtin_spirv_OpenCL_sqrt_f32( result );
            result = x + result;
            result = __builtin_spirv_OpenCL_log_f32( result );
        }
    }
    else
    {
        result = __ocl_svml_asinhf(x);
    }

    return result;
}

INLINE float __builtin_spirv_OpenCL_asinh_f32( float x )
{
    return __intel_asinh_f32( x, true );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_asinh_f64( double x )
{
    return __ocl_svml_asinh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_asinh_f16( half x )
{
    return __builtin_spirv_OpenCL_asinh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, half, half, f16 )

#endif // defined(cl_khr_fp16)
