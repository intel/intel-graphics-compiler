/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fast_length( float p )
{
    return __spirv_ocl_fast_length( p );
}

INLINE float OVERLOADABLE fast_length( float2 p )
{
    return __spirv_ocl_fast_length( p );
}

INLINE float OVERLOADABLE fast_length( float3 p )
{
    return __spirv_ocl_fast_length( p );
}

INLINE float OVERLOADABLE fast_length( float4 p )
{
    return __spirv_ocl_fast_length( p );
}
