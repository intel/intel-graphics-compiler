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

INLINE float __attribute__((overloadable)) __spirv_ocl_lgamma( float x )
{
#ifdef USE_IMF_LGAMMA_IMPL
    return __ocl_svml_lgammaf(x);
#else // USE_IMF_LGAMMA_IMPL
    float r;
    if( __intel_relaxed_isnan(x) )
    {
        r = __spirv_ocl_nan(0);
    }
    else
    {
        float g = __spirv_ocl_tgamma(x);
        r = __spirv_IsNan(g) ? INFINITY : __spirv_ocl_native_log(__spirv_ocl_fabs(g));
    }
    return r;
#endif // USE_IMF_LGAMMA_IMPL
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_lgamma( double x )
{
    return libclc_lgamma_f64(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_lgamma( half x )
{
    return __spirv_ocl_lgamma((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( lgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)

