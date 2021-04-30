/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_remainder_f32_f32( float x, float y )
{
    int temp;
    return __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(x, y, &temp);
}

INLINE float __builtin_spirv_OpFRem_f32_f32( float x, float y )
{
    return __builtin_spirv_OpenCL_fmod_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_remainder_f64_f64( double x, double y )
{
    int temp;
    return __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(x, y, &temp);
}

INLINE double __builtin_spirv_OpFRem_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_fmod_f64_f64(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_remainder_f16_f16( half y, half x )
{
    return __builtin_spirv_OpenCL_remainder_f32_f32((float)y, (float)x );
}

INLINE half __builtin_spirv_OpFRem_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_fmod_f16_f16(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)
