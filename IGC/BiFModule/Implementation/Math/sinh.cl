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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f32, )( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // For most inputs, we'll use the expansion
        //  sinh(x) = 0.5f * ( e^x - e^-x ):
        float pexp = SPIRV_OCL_BUILTIN(exp, _f32, )(  x );
        float nexp = SPIRV_OCL_BUILTIN(exp, _f32, )( -x );
        result = 0.5f * ( pexp - nexp );

        // For x close to zero, we'll simply use x.
        // We use 2^-10 as our cutoff value for
        // "close to zero".
        float px = SPIRV_OCL_BUILTIN(fabs, _f32, )( x );
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

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f64, )( double x )
{
    return __ocl_svml_sinh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(sinh, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sinh, half, half, f16 )

#endif // defined(cl_khr_fp16)
