/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/log10_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/log10_d_la.cl"
#endif // defined(cl_khr_fp64)

#define _M_LOG10_E_DBL  (as_double(0x3fdbcb7b1526e50e)) // 0.4342944819032518276511289

INLINE float __builtin_spirv_OpenCL_log10_f32( float x )
{
    float result;

    if(__FastRelaxedMath)
    {
        result = __builtin_spirv_OpenCL_native_log10_f32(x);
    }
    else
    {
        result = __ocl_svml_log10f(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_log10_f64( double x )
{
    return __ocl_svml_log10_v2(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log10_f16( half x )
{
    return __builtin_spirv_OpenCL_log10_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, half, half, f16 )

#endif // defined(cl_khr_fp16)
