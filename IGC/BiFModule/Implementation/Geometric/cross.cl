/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float3 __attribute__((overloadable)) __spirv_ocl_cross(float3 p0, float3 p1 ){
    float3 result;
    result.x = __spirv_ocl_fma(p0.y, p1.z, -p0.z * p1.y );
    result.y = __spirv_ocl_fma(p0.z, p1.x, -p0.x * p1.z );
    result.z = __spirv_ocl_fma(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_cross(float4 p0, float4 p1 ){
    float4 result;
    result.xyz = __spirv_ocl_cross( p0.xyz, p1.xyz );
    result.w = 0.0f;

    return result;
}

#if defined(cl_khr_fp64)

INLINE double3 __attribute__((overloadable)) __spirv_ocl_cross(double3 p0, double3 p1 ){
    double3 result;
    result.x = __spirv_ocl_fma(p0.y, p1.z, -p0.z * p1.y );
    result.y = __spirv_ocl_fma(p0.z, p1.x, -p0.x * p1.z );
    result.z = __spirv_ocl_fma(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE double4 __attribute__((overloadable)) __spirv_ocl_cross(double4 p0, double4 p1 ){
    double4 result;
    result.xyz = __spirv_ocl_cross( p0.xyz, p1.xyz );

    result.w = (half) 0.0;

    return result;
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half3 __attribute__((overloadable)) __spirv_ocl_cross(half3 p0, half3 p1 ){
    float3 ret = __spirv_ocl_cross(__spirv_FConvert_Rfloat3(p0), __spirv_FConvert_Rfloat3(p1));
    return __spirv_FConvert_Rhalf3(ret);
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_cross(half4 p0, half4 p1 ){
    half4 result;
    result.xyz = __spirv_ocl_cross( p0.xyz, p1.xyz );
    result.w = (half)0.0f;

    return result;
}

#endif // defined(cl_khr_fp16)

