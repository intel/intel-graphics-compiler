/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"

INLINE float OVERLOADABLE dot( float p0, float p1 )
{
    return p0 * p1;
}

INLINE float OVERLOADABLE dot( float2 p0, float2 p1 )
{
    return mad( p0.x,  p1.x,
               (p0.y * p1.y) );
}

INLINE float OVERLOADABLE dot( float3 p0, float3 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
               (p0.z * p1.z )));
}

INLINE float OVERLOADABLE dot( float4 p0, float4 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
           mad( p0.z,  p1.z,
               (p0.w * p1.w) )));
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE dot( double p0, double p1 )
{
    return p0 * p1;
}

INLINE double OVERLOADABLE dot( double2 p0, double2 p1 )
{
    return mad( p0.x,  p1.x,
               (p0.y * p1.y) );
}

INLINE double OVERLOADABLE dot( double3 p0, double3 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
               (p0.z * p1.z )));
}

INLINE double OVERLOADABLE dot( double4 p0, double4 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
           mad( p0.z,  p1.z,
               (p0.w * p1.w) )));
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE dot( half p0, half p1 )
{
    return p0 * p1;
}

INLINE half OVERLOADABLE dot( half2 p0, half2 p1 )
{
    return mad( p0.x,  p1.x,
               (p0.y * p1.y) );
}

INLINE half OVERLOADABLE dot( half3 p0, half3 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
               (p0.z * p1.z )));
}

INLINE half OVERLOADABLE dot( half4 p0, half4 p1 )
{
    return mad( p0.x,  p1.x,
           mad( p0.y,  p1.y,
           mad( p0.z,  p1.z,
               (p0.w * p1.w) )));
}

#endif // defined(cl_khr_fp16)
