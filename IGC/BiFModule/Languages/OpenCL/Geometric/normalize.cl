/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE normalize( float p )
{
    return __spirv_ocl_normalize( p );
}

float2 OVERLOADABLE normalize( float2 p )
{
    return __spirv_ocl_normalize( p );
}

float3 OVERLOADABLE normalize( float3 p )
{
    return __spirv_ocl_normalize( p );
}

float4 OVERLOADABLE normalize( float4 p )
{
    return __spirv_ocl_normalize( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE normalize( double p )
{
    return __spirv_ocl_normalize( p );
}

double2 OVERLOADABLE normalize( double2 p )
{
    return __spirv_ocl_normalize( p );
}

double3 OVERLOADABLE normalize( double3 p )
{
    return __spirv_ocl_normalize( p );
}

double4 OVERLOADABLE normalize( double4 p )
{
    return __spirv_ocl_normalize( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE normalize( half p )
{
    return __spirv_ocl_normalize( p );
}

half2 OVERLOADABLE normalize( half2 p )
{
    return __spirv_ocl_normalize( p );
}

half3 OVERLOADABLE normalize( half3 p )
{
    return __spirv_ocl_normalize( p );
}

half4 OVERLOADABLE normalize( half4 p )
{
    return __spirv_ocl_normalize( p );
}

#endif // defined(cl_khr_fp16)
