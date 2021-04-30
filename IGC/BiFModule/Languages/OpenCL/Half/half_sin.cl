/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_sin( float x )
{
    return __builtin_spirv_OpenCL_half_sin_f32( x );
}

INLINE float2 OVERLOADABLE half_sin( float2 x )
{
    return __builtin_spirv_OpenCL_half_sin_v2f32( x );
}

INLINE float3 OVERLOADABLE half_sin( float3 x )
{
    return __builtin_spirv_OpenCL_half_sin_v3f32( x );
}

INLINE float4 OVERLOADABLE half_sin( float4 x )
{
    return __builtin_spirv_OpenCL_half_sin_v4f32( x );
}

INLINE float8 OVERLOADABLE half_sin( float8 x )
{
    return __builtin_spirv_OpenCL_half_sin_v8f32( x );
}

INLINE float16 OVERLOADABLE half_sin( float16 x )
{
    return __builtin_spirv_OpenCL_half_sin_v16f32( x );
}
