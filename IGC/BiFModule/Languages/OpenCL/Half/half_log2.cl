/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_log2( float x )
{
    return __spirv_ocl_half_log2( x );
}

INLINE float2 OVERLOADABLE half_log2( float2 x )
{
    return __spirv_ocl_half_log2( x );
}

INLINE float3 OVERLOADABLE half_log2( float3 x )
{
    return __spirv_ocl_half_log2( x );
}

INLINE float4 OVERLOADABLE half_log2( float4 x )
{
    return __spirv_ocl_half_log2( x );
}

INLINE float8 OVERLOADABLE half_log2( float8 x )
{
    return __spirv_ocl_half_log2( x );
}

INLINE float16 OVERLOADABLE half_log2( float16 x )
{
    return __spirv_ocl_half_log2( x );
}
