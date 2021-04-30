/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_maxmag_f32_f32( float x, float y )
{
    float fx = __builtin_spirv_OpenCL_fabs_f32(x);
    float fy = __builtin_spirv_OpenCL_fabs_f32(y);
    float m = __builtin_spirv_OpenCL_fmax_f32_f32(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_maxmag_f64_f64( double x, double y )
{
    double fx = __builtin_spirv_OpenCL_fabs_f64(x);
    double fy = __builtin_spirv_OpenCL_fabs_f64(y);
    double m = __builtin_spirv_OpenCL_fmax_f64_f64(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_maxmag_f16_f16( half x, half y )
{
    half fx = __builtin_spirv_OpenCL_fabs_f16(x);
    half fy = __builtin_spirv_OpenCL_fabs_f16(y);
    half m = __builtin_spirv_OpenCL_fmax_f16_f16(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, half, half, f16 )

#endif // defined(cl_khr_fp16)
