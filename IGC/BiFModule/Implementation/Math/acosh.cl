/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/acosh_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/acosh_d_la.cl"
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_acosh_f32( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as log(x + sqrt(x*x - 1)).

#if 1
        // Conformance test checks for NaN, but I don't think we should
        // have to handle this case.
        if( x < 1.0f )
        {
            result = __builtin_spirv_OpenCL_nan_i32((uint)0);
        }
        // Conformance test also checks for this "overflow" case, but
        // I don't think we should have to handle it.
        else if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
#endif
        {
            result = x * x - 1.0f;
            result = __builtin_spirv_OpenCL_sqrt_f32( result );
            result = x + result;
            result = __builtin_spirv_OpenCL_log_f32( result );
        }
    }
    else
    {
        result = __ocl_svml_acoshf(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_acosh_f64( double x )
{
    return __ocl_svml_acosh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_acosh_f16( half x )
{
    return __builtin_spirv_OpenCL_acosh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, half, half, f16 )

#endif // defined(cl_khr_fp16)
