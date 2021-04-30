/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE length( float p )
{
    return __builtin_spirv_OpenCL_length_f32( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
float OVERLOADABLE length( float2 p )
{
    return __builtin_spirv_OpenCL_length_v2f32( p );
}

float OVERLOADABLE length( float3 p )
{
    return __builtin_spirv_OpenCL_length_v3f32( p );
}

float OVERLOADABLE length( float4 p )
{
    return __builtin_spirv_OpenCL_length_v4f32( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE length( double p )
{
    return __builtin_spirv_OpenCL_length_f64( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
double OVERLOADABLE length( double2 p )
{
    return __builtin_spirv_OpenCL_length_v2f64( p );
}

double OVERLOADABLE length( double3 p )
{
    return __builtin_spirv_OpenCL_length_v3f64( p );
}

double OVERLOADABLE length( double4 p )
{
    return __builtin_spirv_OpenCL_length_v4f64( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE length( half p )
{
    return __builtin_spirv_OpenCL_length_f16( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
half OVERLOADABLE length( half2 p )
{
    return __builtin_spirv_OpenCL_length_v2f16( p );
}

half OVERLOADABLE length( half3 p )
{
    return __builtin_spirv_OpenCL_length_v3f16( p );
}

half OVERLOADABLE length( half4 p )
{
    return __builtin_spirv_OpenCL_length_v4f16( p );
}

#endif // defined(cl_khr_fp16)
