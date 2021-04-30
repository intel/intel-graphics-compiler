/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fast_length( float p )
{
    return __builtin_spirv_OpenCL_fast_length_f32( p );
}

INLINE float OVERLOADABLE fast_length( float2 p )
{
    return __builtin_spirv_OpenCL_fast_length_v2f32( p );
}

INLINE float OVERLOADABLE fast_length( float3 p )
{
    return __builtin_spirv_OpenCL_fast_length_v3f32( p );
}

INLINE float OVERLOADABLE fast_length( float4 p )
{
    return __builtin_spirv_OpenCL_fast_length_v4f32( p );
}
