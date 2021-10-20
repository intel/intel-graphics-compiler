/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_powr( float x, float y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _f32_f32, )( x, y );
}

INLINE float2 OVERLOADABLE half_powr( float2 x, float2 y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _v2f32_v2f32, )( x, y );
}

INLINE float3 OVERLOADABLE half_powr( float3 x, float3 y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _v3f32_v3f32, )( x, y );
}

INLINE float4 OVERLOADABLE half_powr( float4 x, float4 y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _v4f32_v4f32, )( x, y );
}

INLINE float8 OVERLOADABLE half_powr( float8 x, float8 y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _v8f32_v8f32, )( x, y );
}

INLINE float16 OVERLOADABLE half_powr( float16 x, float16 y )
{
    return SPIRV_OCL_BUILTIN(half_powr, _v16f32_v16f32, )( x, y );
}
