/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/exp10_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/exp10_d_la.cl"
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f32, )( float x )
{
    if(__FastRelaxedMath)
    {
        return SPIRV_OCL_BUILTIN(native_exp10, _f32, )(x);
    }
    else
    {
        return __ocl_svml_exp10f(x);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f64, )( double x )
{
    return __ocl_svml_exp10(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(exp10, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)
