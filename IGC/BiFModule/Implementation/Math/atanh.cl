/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/atanh_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/atanh_d_la.cl"
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_atanh_f32( float x )
{
    return __ocl_svml_atanhf(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atanh_f64( double x )
{
    return __ocl_svml_atanh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atanh_f16( half x )
{
    return __builtin_spirv_OpenCL_atanh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, half, half, f16 )

#endif // defined(cl_khr_fp16)
