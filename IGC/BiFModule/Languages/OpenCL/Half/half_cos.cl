/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_cos( float x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _f32, )( x );
}

INLINE float2 OVERLOADABLE half_cos( float2 x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _v2f32, )( x );
}

INLINE float3 OVERLOADABLE half_cos( float3 x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _v3f32, )( x );
}

INLINE float4 OVERLOADABLE half_cos( float4 x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _v4f32, )( x );
}

INLINE float8 OVERLOADABLE half_cos( float8 x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _v8f32, )( x );
}

INLINE float16 OVERLOADABLE half_cos( float16 x )
{
    return SPIRV_OCL_BUILTIN(half_cos, _v16f32, )( x );
}
