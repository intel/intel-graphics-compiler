/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fast_normalize( float p )
{
    return SPIRV_OCL_BUILTIN(fast_normalize, _f32, )( p );
}

INLINE float2 OVERLOADABLE fast_normalize( float2 p )
{
    return SPIRV_OCL_BUILTIN(fast_normalize, _v2f32, )( p );
}

INLINE float3 OVERLOADABLE fast_normalize( float3 p )
{
    return SPIRV_OCL_BUILTIN(fast_normalize, _v3f32, )( p );
}

INLINE float4 OVERLOADABLE fast_normalize( float4 p )
{
    return SPIRV_OCL_BUILTIN(fast_normalize, _v4f32, )( p );
}
