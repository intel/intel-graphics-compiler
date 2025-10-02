/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE distance( float p0, float p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE float OVERLOADABLE distance( float2 p0, float2 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE float OVERLOADABLE distance( float3 p0, float3 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE float OVERLOADABLE distance( float4 p0, float4 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE distance( double p0, double p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE double OVERLOADABLE distance( double2 p0, double2 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE double OVERLOADABLE distance( double3 p0, double3 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE double OVERLOADABLE distance( double4 p0, double4 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE distance( half p0, half p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE half OVERLOADABLE distance( half2 p0, half2 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE half OVERLOADABLE distance( half3 p0, half3 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

INLINE half OVERLOADABLE distance( half4 p0, half4 p1 )
{
    return __spirv_ocl_distance( p0, p1 );
}

#endif // defined(cl_khr_fp16)
