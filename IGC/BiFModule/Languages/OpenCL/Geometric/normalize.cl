/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE normalize( float p )
{
    return SPIRV_OCL_BUILTIN(normalize, _f32, )( p );
}

float2 OVERLOADABLE normalize( float2 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v2f32, )( p );
}

float3 OVERLOADABLE normalize( float3 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v3f32, )( p );
}

float4 OVERLOADABLE normalize( float4 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v4f32, )( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE normalize( double p )
{
    return SPIRV_OCL_BUILTIN(normalize, _f64, )( p );
}

double2 OVERLOADABLE normalize( double2 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v2f64, )( p );
}

double3 OVERLOADABLE normalize( double3 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v3f64, )( p );
}

double4 OVERLOADABLE normalize( double4 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v4f64, )( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE normalize( half p )
{
    return SPIRV_OCL_BUILTIN(normalize, _f16, )( p );
}

half2 OVERLOADABLE normalize( half2 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v2f16, )( p );
}

half3 OVERLOADABLE normalize( half3 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v3f16, )( p );
}

half4 OVERLOADABLE normalize( half4 p )
{
    return SPIRV_OCL_BUILTIN(normalize, _v4f16, )( p );
}

#endif // defined(cl_khr_fp16)
