/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE half_sin( float x )
{
    return __spirv_ocl_half_sin( x );
}

INLINE float2 OVERLOADABLE half_sin( float2 x )
{
    return __spirv_ocl_half_sin( x );
}

INLINE float3 OVERLOADABLE half_sin( float3 x )
{
    return __spirv_ocl_half_sin( x );
}

INLINE float4 OVERLOADABLE half_sin( float4 x )
{
    return __spirv_ocl_half_sin( x );
}

INLINE float8 OVERLOADABLE half_sin( float8 x )
{
    return __spirv_ocl_half_sin( x );
}

INLINE float16 OVERLOADABLE half_sin( float16 x )
{
    return __spirv_ocl_half_sin( x );
}
