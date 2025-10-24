/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f32_f32, )( float x, float y )
{
    float fx = SPIRV_OCL_BUILTIN(fabs, _f32, )(x);
    float fy = SPIRV_OCL_BUILTIN(fabs, _f32, )(y);
    float m = SPIRV_OCL_BUILTIN(fmin, _f32_f32, )(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( minmag, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f64_f64, )( double x, double y )
{
    double fx = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);
    double fy = SPIRV_OCL_BUILTIN(fabs, _f64, )(y);
    double m = SPIRV_OCL_BUILTIN(fmin, _f64_f64, )(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( minmag, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f16_f16, )( half x, half y )
{
    half fx = SPIRV_OCL_BUILTIN(fabs, _f16, )(x);
    half fy = SPIRV_OCL_BUILTIN(fabs, _f16, )(y);
    half m = SPIRV_OCL_BUILTIN(fmin, _f16_f16, )(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( minmag, half, half, f16 )

#endif // defined(cl_khr_fp16)
