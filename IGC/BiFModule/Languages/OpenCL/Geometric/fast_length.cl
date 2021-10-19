/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fast_length( float p )
{
    return SPIRV_OCL_BUILTIN(fast_length, _f32, )( p );
}

INLINE float OVERLOADABLE fast_length( float2 p )
{
    return SPIRV_OCL_BUILTIN(fast_length, _v2f32, )( p );
}

INLINE float OVERLOADABLE fast_length( float3 p )
{
    return SPIRV_OCL_BUILTIN(fast_length, _v3f32, )( p );
}

INLINE float OVERLOADABLE fast_length( float4 p )
{
    return SPIRV_OCL_BUILTIN(fast_length, _v4f32, )( p );
}
