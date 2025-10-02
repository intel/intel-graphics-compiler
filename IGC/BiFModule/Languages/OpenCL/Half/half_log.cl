/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_log( float x )
{
    return __spirv_ocl_half_log( x );
}

INLINE float2 OVERLOADABLE half_log( float2 x )
{
    return __spirv_ocl_half_log( x );
}

INLINE float3 OVERLOADABLE half_log( float3 x )
{
    return __spirv_ocl_half_log( x );
}

INLINE float4 OVERLOADABLE half_log( float4 x )
{
    return __spirv_ocl_half_log( x );
}

INLINE float8 OVERLOADABLE half_log( float8 x )
{
    return __spirv_ocl_half_log( x );
}

INLINE float16 OVERLOADABLE half_log( float16 x )
{
    return __spirv_ocl_half_log( x );
}
