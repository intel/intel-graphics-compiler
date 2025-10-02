/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float3 OVERLOADABLE cross( float3 p0, float3 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

INLINE float4 OVERLOADABLE cross( float4 p0, float4 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

#if defined(cl_khr_fp64)

INLINE double3 OVERLOADABLE cross( double3 p0, double3 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

INLINE double4 OVERLOADABLE cross( double4 p0, double4 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half3 OVERLOADABLE cross( half3 p0, half3 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

INLINE half4 OVERLOADABLE cross( half4 p0, half4 p1 )
{
    return __spirv_ocl_cross( p0, p1 );
}

#endif // defined(cl_khr_fp16)
