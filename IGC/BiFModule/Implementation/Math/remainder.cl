/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f32_f32, )( float x, float y )
{
    int temp;
    return SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(x, y, &temp);
}

INLINE float __builtin_spirv_OpFRem_f32_f32( float x, float y )
{
    return SPIRV_OCL_BUILTIN(fmod, _f32_f32, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f64_f64, )( double x, double y )
{
    int temp;
    return SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(x, y, &temp);
}

INLINE double __builtin_spirv_OpFRem_f64_f64( double x, double y )
{
    return SPIRV_OCL_BUILTIN(fmod, _f64_f64, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f16_f16, )( half y, half x )
{
    return SPIRV_OCL_BUILTIN(remainder, _f32_f32, )((float)y, (float)x );
}

INLINE half __builtin_spirv_OpFRem_f16_f16( half x, half y )
{
    return SPIRV_OCL_BUILTIN(fmod, _f16_f16, )(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( remainder, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)
