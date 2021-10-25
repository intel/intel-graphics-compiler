/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/expm1_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/expm1_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f32, )( float x )
{
    if(__FastRelaxedMath && (!__APIRS))
    {
        return SPIRV_OCL_BUILTIN(exp, _f32, )( x ) - 1.0f;
    }
    else
    {
        return __ocl_svml_expm1f(x);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f64, )( double x )
{
    return __ocl_svml_expm1(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(expm1, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( expm1, half, half, f16 )

#endif // defined(cl_khr_fp16)
