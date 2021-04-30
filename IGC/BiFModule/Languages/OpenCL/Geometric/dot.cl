/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE dot( float p0, float p1 )
{
    return p0 * p1;
}

INLINE float OVERLOADABLE dot( float2 p0, float2 p1 )
{
    return SPIRV_BUILTIN(Dot, _v2f32_v2f32, )( p0, p1 );
}

INLINE float OVERLOADABLE dot( float3 p0, float3 p1 )
{
    return SPIRV_BUILTIN(Dot, _v3f32_v3f32, )( p0, p1 );
}

INLINE float OVERLOADABLE dot( float4 p0, float4 p1 )
{
    return SPIRV_BUILTIN(Dot, _v4f32_v4f32, )( p0, p1 );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE dot( double p0, double p1 )
{
    return p0 * p1;
}

INLINE double OVERLOADABLE dot( double2 p0, double2 p1 )
{
    return SPIRV_BUILTIN(Dot, _v2f64_v2f64, )( p0, p1 );
}

INLINE double OVERLOADABLE dot( double3 p0, double3 p1 )
{
    return SPIRV_BUILTIN(Dot, _v3f64_v3f64, )( p0, p1 );
}

INLINE double OVERLOADABLE dot( double4 p0, double4 p1 )
{
    return SPIRV_BUILTIN(Dot, _v4f64_v4f64, )( p0, p1 );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE dot( half p0, half p1 )
{
    return p0 * p1;
}

INLINE half OVERLOADABLE dot( half2 p0, half2 p1 )
{
    return SPIRV_BUILTIN(Dot, _v2f16_v2f16, )( p0, p1 );
}

INLINE half OVERLOADABLE dot( half3 p0, half3 p1 )
{
    return SPIRV_BUILTIN(Dot, _v3f16_v3f16, )( p0, p1 );
}

INLINE half OVERLOADABLE dot( half4 p0, half4 p1 )
{
    return SPIRV_BUILTIN(Dot, _v4f16_v4f16, )( p0, p1 );
}

#endif // defined(cl_khr_fp16)
