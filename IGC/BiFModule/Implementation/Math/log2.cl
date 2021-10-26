/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/log2_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/log2_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f32, )( float x )
{
    float result;

    if(__FastRelaxedMath)
    {
        result = SPIRV_OCL_BUILTIN(native_log2, _f32, )(x);
    }
    else
    {
        result = __ocl_svml_log2f(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log2, float, float, f32 )

#if defined(cl_khr_fp64)

#define _M_LOG2_E_DBL   (as_double(0x3ff71547652b82fe)) // 1.4426950408889634073599246

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f64, )( double x )
{
    return __ocl_svml_log2_v2(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log2, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(log2, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( log2, half, half, f16 )

#endif // defined(cl_khr_fp16)
