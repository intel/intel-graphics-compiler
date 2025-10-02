/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_log10( float x )
{
    return __spirv_ocl_half_log10( x );
}

INLINE float2 OVERLOADABLE half_log10( float2 x )
{
    return __spirv_ocl_half_log10( x );
}

INLINE float3 OVERLOADABLE half_log10( float3 x )
{
    return __spirv_ocl_half_log10( x );
}

INLINE float4 OVERLOADABLE half_log10( float4 x )
{
    return __spirv_ocl_half_log10( x );
}

INLINE float8 OVERLOADABLE half_log10( float8 x )
{
    return __spirv_ocl_half_log10( x );
}

INLINE float16 OVERLOADABLE half_log10( float16 x )
{
    return __spirv_ocl_half_log10( x );
}
