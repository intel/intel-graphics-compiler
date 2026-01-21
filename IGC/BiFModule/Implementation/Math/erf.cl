/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/erf_s_la.cl"
#include "../IMF/FP32/erf_s_la_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/erf_d_la.cl"
    #include "../IMF/FP64/erf_d_la_noLUT.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_erf( float x )
{
    float result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_erff_noLUT(x);
    } else {
        result = __ocl_svml_erff(x);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erf, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_erf( double x )
{
    double result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_erf_noLUT(x);
    } else {
        result = __ocl_svml_erf(x);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erf, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_erf( half x )
{
    return __spirv_ocl_erf((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erf, half, half, f16 )

#endif // defined(cl_khr_fp16)

