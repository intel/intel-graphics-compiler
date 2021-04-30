/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_exp10( float x )
{
    return __builtin_spirv_OpenCL_half_exp10_f32( x );
}

INLINE float2 OVERLOADABLE half_exp10( float2 x )
{
    return __builtin_spirv_OpenCL_half_exp10_v2f32( x );
}

INLINE float3 OVERLOADABLE half_exp10( float3 x )
{
    return __builtin_spirv_OpenCL_half_exp10_v3f32( x );
}

INLINE float4 OVERLOADABLE half_exp10( float4 x )
{
    return __builtin_spirv_OpenCL_half_exp10_v4f32( x );
}

INLINE float8 OVERLOADABLE half_exp10( float8 x )
{
    return __builtin_spirv_OpenCL_half_exp10_v8f32( x );
}

INLINE float16 OVERLOADABLE half_exp10( float16 x )
{
    return __builtin_spirv_OpenCL_half_exp10_v16f32( x );
}
