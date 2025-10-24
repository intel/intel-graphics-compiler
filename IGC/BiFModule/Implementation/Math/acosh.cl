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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f32, )( float x )
{
    float result;

    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // Implemented as log(x + sqrt(x*x - 1)).

#if 1
        // Conformance test checks for NaN, but I don't think we should
        // have to handle this case.
        if( x < 1.0f )
        {
            result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
        }
        // Conformance test also checks for this "overflow" case, but
        // I don't think we should have to handle it.
        else if( x > 1500.0f )
        {
            result = SPIRV_OCL_BUILTIN(log, _f32, )(x) + M_LN2_F;
        }
        else
#endif
        {
            result = x * x - 1.0f;
            result = SPIRV_OCL_BUILTIN(sqrt, _f32, )( result );
            result = x + result;
            result = SPIRV_OCL_BUILTIN(log, _f32, )( result );
        }
    }
    else
    {
        result = __ocl_svml_acoshf(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f64, )( double x )
{
    return __ocl_svml_acosh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(acosh, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( acosh, half, half, f16 )

#endif // defined(cl_khr_fp16)
