/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#define USE_IMF_LGAMMA_IMPL 1

#ifdef USE_IMF_LGAMMA_IMPL
#include "../IMF/FP32/lgamma_s_noFP64.cl"
#endif // USE_IMF_LGAMMA_IMPL

#if defined(cl_khr_fp64)
    //#include "../IMF/FP64/erf_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __builtin_spirv_OpenCL_lgamma_f32( float x )
{
#ifdef USE_IMF_LGAMMA_IMPL
    return __ocl_svml_lgammaf(x);
#else // USE_IMF_LGAMMA_IMPL
    float r;
    if( __intel_relaxed_isnan(x) )
    {
        r = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else
    {
        float g = __builtin_spirv_OpenCL_tgamma_f32(x);
        r = SPIRV_BUILTIN(IsNan, _f32, )(g) ? INFINITY : __builtin_spirv_OpenCL_native_log_f32(__builtin_spirv_OpenCL_fabs_f32(g));
    }
    return r;
#endif // USE_IMF_LGAMMA_IMPL
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_lgamma_f64( double x )
{
    return libclc_lgamma_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_lgamma_f16( half x )
{
    return __builtin_spirv_OpenCL_lgamma_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)
