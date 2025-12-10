/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_maxmag( float x, float y )
{
    float fx = __spirv_ocl_fabs(x);
    float fy = __spirv_ocl_fabs(y);
    float m = __spirv_ocl_fmax(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( maxmag, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_maxmag( double x, double y )
{
    double fx = __spirv_ocl_fabs(x);
    double fy = __spirv_ocl_fabs(y);
    double m = __spirv_ocl_fmax(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( maxmag, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_maxmag( half x, half y )
{
    half fx = __spirv_ocl_fabs(x);
    half fy = __spirv_ocl_fabs(y);
    half m = __spirv_ocl_fmax(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( maxmag, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_maxmag( bfloat x, bfloat y )
{
    return __spirv_ocl_maxmag((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( maxmag, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)
