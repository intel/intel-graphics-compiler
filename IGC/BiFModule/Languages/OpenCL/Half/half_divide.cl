/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_divide( float x, float y )
{
    return __spirv_ocl_half_divide( x, y );
}

INLINE float2 OVERLOADABLE half_divide( float2 x, float2 y )
{
    return __spirv_ocl_half_divide( x, y );
}

INLINE float3 OVERLOADABLE half_divide( float3 x, float3 y )
{
    return __spirv_ocl_half_divide( x, y );
}

INLINE float4 OVERLOADABLE half_divide( float4 x, float4 y )
{
    return __spirv_ocl_half_divide( x, y );
}

INLINE float8 OVERLOADABLE half_divide( float8 x, float8 y )
{
    return __spirv_ocl_half_divide( x, y );
}

INLINE float16 OVERLOADABLE half_divide( float16 x, float16 y )
{
    return __spirv_ocl_half_divide( x, y );
}
