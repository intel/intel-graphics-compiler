/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE length( float p )
{
    return __spirv_ocl_length( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
float OVERLOADABLE length( float2 p )
{
    return __spirv_ocl_length( p );
}

float OVERLOADABLE length( float3 p )
{
    return __spirv_ocl_length( p );
}

float OVERLOADABLE length( float4 p )
{
    return __spirv_ocl_length( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE length( double p )
{
    return __spirv_ocl_length( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
double OVERLOADABLE length( double2 p )
{
    return __spirv_ocl_length( p );
}

double OVERLOADABLE length( double3 p )
{
    return __spirv_ocl_length( p );
}

double OVERLOADABLE length( double4 p )
{
    return __spirv_ocl_length( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE length( half p )
{
    return __spirv_ocl_length( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
half OVERLOADABLE length( half2 p )
{
    return __spirv_ocl_length( p );
}

half OVERLOADABLE length( half3 p )
{
    return __spirv_ocl_length( p );
}

half OVERLOADABLE length( half4 p )
{
    return __spirv_ocl_length( p );
}

#endif // defined(cl_khr_fp16)
