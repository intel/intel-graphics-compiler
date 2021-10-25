/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f32_v3f32, )(float3 p0, float3 p1 ){
    float3 result;
    result.x = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(p0.y, p1.z, -p0.z * p1.y );
    result.y = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(p0.z, p1.x, -p0.x * p1.z );
    result.z = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f32_v4f32, )(float4 p0, float4 p1 ){
    float4 result;
    result.xyz = SPIRV_OCL_BUILTIN(cross, _v3f32_v3f32, )( p0.xyz, p1.xyz );
    result.w = 0.0f;

    return result;
}

#if defined(cl_khr_fp64)

INLINE double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f64_v3f64, )(double3 p0, double3 p1 ){
    double3 result;
    result.x = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p0.y, p1.z, -p0.z * p1.y );
    result.y = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p0.z, p1.x, -p0.x * p1.z );
    result.z = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f64_v4f64, )(double4 p0, double4 p1 ){
    double4 result;
    result.xyz = SPIRV_OCL_BUILTIN(cross, _v3f64_v3f64, )( p0.xyz, p1.xyz );

    result.w = (half) 0.0;

    return result;
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f16_v3f16, )(half3 p0, half3 p1 ){
    float3 ret = SPIRV_OCL_BUILTIN(cross, _v3f32_v3f32, )(SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(p0), SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(p1));
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)(ret);
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f16_v4f16, )(half4 p0, half4 p1 ){
    half4 result;
    result.xyz = SPIRV_OCL_BUILTIN(cross, _v3f16_v3f16, )( p0.xyz, p1.xyz );
    result.w = (half)0.0f;

    return result;
}

#endif // defined(cl_khr_fp16)
