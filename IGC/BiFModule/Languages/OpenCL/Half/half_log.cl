/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_log( float x )
{
    return SPIRV_OCL_BUILTIN(half_log, _f32, )( x );
}

INLINE float2 OVERLOADABLE half_log( float2 x )
{
    return SPIRV_OCL_BUILTIN(half_log, _v2f32, )( x );
}

INLINE float3 OVERLOADABLE half_log( float3 x )
{
    return SPIRV_OCL_BUILTIN(half_log, _v3f32, )( x );
}

INLINE float4 OVERLOADABLE half_log( float4 x )
{
    return SPIRV_OCL_BUILTIN(half_log, _v4f32, )( x );
}

INLINE float8 OVERLOADABLE half_log( float8 x )
{
    return SPIRV_OCL_BUILTIN(half_log, _v8f32, )( x );
}

INLINE float16 OVERLOADABLE half_log( float16 x )
{
    return SPIRV_OCL_BUILTIN(half_log, _v16f32, )( x );
}
