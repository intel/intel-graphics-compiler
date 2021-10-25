/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
float OVERLOADABLE fast_fmod( float xx, float yy )
{
    float result = xx - yy * trunc( xx / yy );
    return result;
}

INLINE
float2 OVERLOADABLE fast_fmod( float2 xx, float2 yy )
{
    float2 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
float3 OVERLOADABLE fast_fmod( float3 xx, float3 yy )
{
    float3 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
float4 OVERLOADABLE fast_fmod( float4 xx, float4 yy )
{
    float4 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    temp.s3 = fast_fmod(xx.s3, yy.s3);
    return temp;
}


#if defined(cl_khr_fp16)
INLINE
half OVERLOADABLE fast_fmod( half xx, half yy )
{
    return (half)fast_fmod((float)xx, (float)yy);
}

INLINE
half2 OVERLOADABLE fast_fmod( half2 xx, half2 yy )
{
    half2 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
half3 OVERLOADABLE fast_fmod( half3 xx, half3 yy )
{
    half3 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
half4 OVERLOADABLE fast_fmod( half4 xx, half4 yy )
{
    half4 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    temp.s3 = fast_fmod(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp16


#if defined(cl_fp64_basic_ops)
INLINE
double OVERLOADABLE fast_fmod( double xx, double yy )
{
    return fast_fmod(xx, yy);
}

INLINE
double2 OVERLOADABLE fast_fmod( double2 xx, double2 yy )
{
    double2 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    return temp;
}

INLINE
double3 OVERLOADABLE fast_fmod( double3 xx, double3 yy )
{
    double3 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    return temp;
}

INLINE
double4 OVERLOADABLE fast_fmod( double4 xx, double4 yy )
{
    double4 temp;
    temp.s0 = fast_fmod(xx.s0, yy.s0);
    temp.s1 = fast_fmod(xx.s1, yy.s1);
    temp.s2 = fast_fmod(xx.s2, yy.s2);
    temp.s3 = fast_fmod(xx.s3, yy.s3);
    return temp;
}
#endif // cl_fp64_basic_ops

float OVERLOADABLE fmod( float xx, float yy )
{
    return SPIRV_OCL_BUILTIN(fmod, _f32_f32, )( xx, yy );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, float, float, float )

#if defined(cl_khr_fp64)

double OVERLOADABLE fmod( double xx, double yy )
{
    return SPIRV_OCL_BUILTIN(fmod, _f64_f64, )( xx, yy );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fmod( half x, half y )
{
    return SPIRV_OCL_BUILTIN(fmod, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( fmod, half, half, half )

#endif // defined(cl_khr_fp16)
