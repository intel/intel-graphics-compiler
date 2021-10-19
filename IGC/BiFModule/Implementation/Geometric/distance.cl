/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f32_f32, )(float p0, float p1 ){
    return SPIRV_OCL_BUILTIN(length, _f32, )( p0 - p1 );
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f32_v2f32, )(float2 p0, float2 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v2f32, )( p0 - p1 );
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f32_v3f32, )(float3 p0, float3 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v3f32, )( p0 - p1 );
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f32_v4f32, )(float4 p0, float4 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v4f32, )( p0 - p1 );
}

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f64_f64, )(double p0, double p1 ){
    return SPIRV_OCL_BUILTIN(length, _f64, )( p0 - p1 );
}

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f64_v2f64, )(double2 p0, double2 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v2f64, )( p0 - p1 );
}

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f64_v3f64, )(double3 p0, double3 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v3f64, )( p0 - p1 );
}

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f64_v4f64, )(double4 p0, double4 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v4f64, )( p0 - p1 );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f16_f16, )(half p0, half p1 ){
    return SPIRV_OCL_BUILTIN(length, _f16, )( p0 - p1 );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f16_v2f16, )(half2 p0, half2 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v2f16, )( p0 - p1 );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f16_v3f16, )(half3 p0, half3 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v3f16, )( p0 - p1 );
}

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f16_v4f16, )(half4 p0, half4 p1 ){
    return SPIRV_OCL_BUILTIN(length, _v4f16, )( p0 - p1 );
}

#endif // defined(cl_khr_fp16)

