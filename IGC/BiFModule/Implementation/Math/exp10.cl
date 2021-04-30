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

float __builtin_spirv_OpenCL_exp10_f32( float x )
{
    if(__FastRelaxedMath)
    {
        return __builtin_spirv_OpenCL_native_exp10_f32(x);
    }
    else
    {
        return __ocl_svml_exp10f(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_exp10_f64( double x )
{
    return __ocl_svml_exp10(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp10_f16( half x )
{
    return __builtin_spirv_OpenCL_exp10_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)
