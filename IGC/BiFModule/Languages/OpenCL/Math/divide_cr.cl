/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

float OVERLOADABLE divide_cr( float a,
                              float b )
{
    return __builtin_spirv_divide_cr_f32_f32( a, b );
}

INLINE
float2 OVERLOADABLE divide_cr( float2 a,
                               float2 b )
{
    return __builtin_spirv_divide_cr_v2f32_v2f32( a, b );
}

INLINE
float3 OVERLOADABLE divide_cr( float3 a,
                               float3 b )
{
    return __builtin_spirv_divide_cr_v3f32_v3f32( a, b );
}

INLINE
float4 OVERLOADABLE divide_cr( float4 a,
                               float4 b )
{
    return __builtin_spirv_divide_cr_v4f32_v4f32( a, b );
}

INLINE
float8 OVERLOADABLE divide_cr( float8 a,
                               float8 b )
{
    return __builtin_spirv_divide_cr_v8f32_v8f32( a, b );
}

INLINE
float16 OVERLOADABLE divide_cr( float16 a,
                                float16 b )
{
    return __builtin_spirv_divide_cr_v16f32_v16f32( a, b );
}
