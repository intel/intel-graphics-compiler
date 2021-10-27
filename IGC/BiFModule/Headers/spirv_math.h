/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_MATH_H__
#define __SPIRV_MATH_H__

#include "spirv_macros.h"

//
//  Common
//        -degrees,fclamp,fmax_common,fmin_common,mix,radians,sign,smoothstep,step
//

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f32, )(float r );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f64, )(double r );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f16, )(half r );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(float x, float y, float z);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _f64_f64_f64, )(double x, double y, double z);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _f16_f16_f16, )(half x, half y, half z);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fclamp, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _f32_f32, )(float x, float y);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _f64_f64, )(double x, double y);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _f16_f16, )(half x, half y);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax_common, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _f32_f32, )(float x, float y);
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _f16_f16, )(half x, half y);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _f64_f64, )(double x, double y);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin_common, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f32_f32_f32, )(float x, float y, float a );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f64_f64_f64, )(double x, double y, double a );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _f16_f16_f16, )(half x, half y, half a );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mix, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _f32, )(float d );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _f64, )(double d );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _f16, )(half d );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(radians, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sign, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _f32_f32_f32, )(float edge0, float edge1, float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _f64_f64_f64, )(double edge0, double edge1, double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _f16_f16_f16, )(half edge0, half edge1, half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(smoothstep, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f32_f32, )(float edge, float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f64_f64, )(double edge, double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _f16_f16, )(half edge, half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(step, _v16f16_v16f16, )(half16 x, half16 y);


//
//  Geometric
//        -cross,distance,fast_distance,fast_length,fast_normalize,length,normalize
//

float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f32_v3f32, )(float3 p0, float3 p1 );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f32_v4f32, )(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f64_v3f64, )(double3 p0, double3 p1 );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f64_v4f64, )(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v3f16_v3f16, )(half3 p0, half3 p1 );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cross, _v4f16_v4f16, )(half4 p0, half4 p1 );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f32_f32, )(float p0, float p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f32_v2f32, )(float2 p0, float2 p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f32_v3f32, )(float3 p0, float3 p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f32_v4f32, )(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f64_f64, )(double p0, double p1 );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f64_v2f64, )(double2 p0, double2 p1 );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f64_v3f64, )(double3 p0, double3 p1 );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f64_v4f64, )(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _f16_f16, )(half p0, half p1 );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v2f16_v2f16, )(half2 p0, half2 p1 );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v3f16_v3f16, )(half3 p0, half3 p1 );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(distance, _v4f16_v4f16, )(half4 p0, half4 p1 );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_distance, _f32_f32, )(float p0, float p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_distance, _v2f32_v2f32, )(float2 p0, float2 p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_distance, _v3f32_v3f32, )(float3 p0, float3 p1 );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_distance, _v4f32_v4f32, )(float4 p0, float4 p1 );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_length, _f32, )(float p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_length, _v2f32, )(float2 p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_length, _v3f32, )(float3 p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_length, _v4f32, )(float4 p );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _f32, )(float p );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v2f32, )(float2 p );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v3f32, )(float3 p );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fast_normalize, _v4f32, )(float4 p );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _f32, )(float p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v2f32, )(float2 p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v3f32, )(float3 p );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v4f32, )(float4 p );
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _f64, )(double p );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v2f64, )(double2 p );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v3f64, )(double3 p );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v4f64, )(double4 p );
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _f16, )(half p );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v2f16, )(half2 p );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v3f16, )(half3 p );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(length, _v4f16, )(half4 p );
#endif // defined(cl_khr_fp16)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _f32, )(float p );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v2f32, )(float2 p );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v3f32, )(float3 p );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v4f32, )(float4 p );
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _f64, )(double p );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v2f64, )(double2 p );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v3f64, )(double3 p );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v4f64, )(double4 p );
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _f16, )(half p );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v2f16, )(half2 p );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v3f16, )(half3 p );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(normalize, _v4f16, )(half4 p );


//
//  Half
//        -half_cos,half_divide,half_exp,half_exp2,half_exp10,half_log,half_log2,half_log10
//         half_powr,half_recip,half_rsqrt,half_sin,half_sqrt,half_tan
//

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_cos, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _f32_f32, )(float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v2f32_v2f32, )(float2 x, float2 y );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v3f32_v3f32, )(float3 x, float3 y );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v4f32_v4f32, )(float4 x, float4 y );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v8f32_v8f32, )(float8 x, float8 y );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_divide, _v16f32_v16f32, )(float16 x, float16 y );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp2, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_exp10, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log2, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_log10, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _f32_f32, )(float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v2f32_v2f32, )(float2 x, float2 y );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v3f32_v3f32, )(float3 x, float3 y );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v4f32_v4f32, )(float4 x, float4 y );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v8f32_v8f32, )(float8 x, float8 y );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_powr, _v16f32_v16f32, )(float16 x, float16 y );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_recip, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_rsqrt, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sin, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_sqrt, _v16f32, )(float16 x );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _v2f32, )(float2 x );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _v3f32, )(float3 x );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _v4f32, )(float4 x );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _v8f32, )(float8 x );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(half_tan, _v16f32, )(float16 x );


//
//  Integer (signed and unsigned )
//        -abs,abs_diff,add_sat,clamp,clz,ctz,hadd,mad_hi,mad_sat,mad24,max,min,mul_hi
//         mul24,popcnt,rhadd,rotate,sub_sat,upsample
//

uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _i8, )( char x );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i8, )( char2 x );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i8, )( char3 x );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i8, )( char4 x );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i8, )( char8 x );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i8, )( char16 x );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i8, )( uchar x );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i8, )( uchar2 x );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i8, )( uchar3 x );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i8, )( uchar4 x );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i8, )( uchar8 x );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i8, )( uchar16 x );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _i16, )( short x );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i16, )( short2 x );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i16, )( short3 x );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i16, )( short4 x );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i16, )( short8 x );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i16, )( short16 x );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i16, )( ushort x );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i16, )( ushort2 x );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i16, )( ushort3 x );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i16, )( ushort4 x );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i16, )( ushort8 x );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i16, )( ushort16 x );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _i32, )( int x );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i32, )( int2 x );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i32, )( int3 x );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i32, )( int4 x );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i32, )( int8 x );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i32, )( int16 x );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i32, )( uint x );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i32, )( uint2 x );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i32, )( uint3 x );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i32, )( uint4 x );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i32, )( uint8 x );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i32, )( uint16 x );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _i64, )( long x );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i64, )( long2 x );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i64, )( long3 x );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i64, )( long4 x );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i64, )( long8 x );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i64, )( long16 x );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i64, )( ulong x );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i64, )( ulong2 x );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i64, )( ulong3 x );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i64, )( ulong4 x );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i64, )( ulong8 x );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i64, )( ulong16 x );

uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _i8_i8, )( char x, char y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v2i8_v2i8, )( char2 x, char2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v3i8_v3i8, )( char3 x, char3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v4i8_v4i8, )( char4 x, char4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v8i8_v8i8, )( char8 x, char8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v16i8_v16i8, )( uchar16 x, uchar16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _i16_i16, )( short x, short y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v2i16_v2i16, )( short2 x, short2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v3i16_v3i16, )( short3 x, short3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v4i16_v4i16, )( short4 x, short4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v8i16_v8i16, )( short8 x, short8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v16i16_v16i16, )( ushort16 x, ushort16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _i32_i32, )( int x, int y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v2i32_v2i32, )( int2 x, int2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v3i32_v3i32, )( int3 x, int3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v4i32_v4i32, )( int4 x, int4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v8i32_v8i32, )( int8 x, int8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v16i32_v16i32, )( uint16 x, uint16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _i64_i64, )( long x, long y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v2i64_v2i64, )( long2 x, long2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v3i64_v3i64, )( long3 x, long3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v4i64_v4i64, )( long4 x, long4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v8i64_v8i64, )( long8 x, long8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs_diff, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs_diff, _v16i64_v16i64, )( ulong16 x, ulong16 y );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _i8_i8, )( char x, char y );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v2i8_v2i8, )( char2 x, char2 y );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v3i8_v3i8, )( char3 x, char3 y );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v4i8_v4i8, )( char4 x, char4 y );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v8i8_v8i8, )( char8 x, char8 y );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v16i8_v16i8, )( uchar16 x, uchar16 y );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _i16_i16, )( short x, short y );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v2i16_v2i16, )( short2 x, short2 y );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v3i16_v3i16, )( short3 x, short3 y );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v4i16_v4i16, )( short4 x, short4 y );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v8i16_v8i16, )( short8 x, short8 y );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v16i16_v16i16, )( ushort16 x, ushort16 y );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v16i32_v16i32, )( uint16 x, uint16 y );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _i64_i64, )( long x, long y );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v2i64_v2i64, )( long2 x, long2 y );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v3i64_v3i64, )( long3 x, long3 y );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v4i64_v4i64, )( long4 x, long4 y );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v8i64_v8i64, )( long8 x, long8 y );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_add_sat, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_add_sat, _v16i64_v16i64, )( ulong16 x, ulong16 y );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i8_i8_i8, )(char x, char minval, char maxval );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i8_i8_i8, )(uchar x, uchar minval, uchar maxval );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i16_i16_i16, )(short x, short minval, short maxval );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i16_i16_i16, )(ushort x, ushort minval, ushort maxval );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i32_i32_i32, )(int x, int minval, int maxval );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i32_i32_i32, )(uint x, uint minval, uint maxval );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i64_i64_i64, )(long x, long minval, long maxval );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i64_i64_i64, )(ulong x, ulong minval, ulong maxval );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v2i8_v2i8_v2i8, )(char2 x, char2 y, char2 z);
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v3i8_v3i8_v3i8, )(char3 x, char3 y, char3 z);
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v4i8_v4i8_v4i8, )(char4 x, char4 y, char4 z);
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v8i8_v8i8_v8i8, )(char8 x, char8 y, char8 z);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v16i8_v16i8_v16i8, )(char16 x, char16 y, char16 z);
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v2i8_v2i8_v2i8, )(uchar2 x, uchar2 y, uchar2 z);
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v3i8_v3i8_v3i8, )(uchar3 x, uchar3 y, uchar3 z);
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v4i8_v4i8_v4i8, )(uchar4 x, uchar4 y, uchar4 z);
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v8i8_v8i8_v8i8, )(uchar8 x, uchar8 y, uchar8 z);
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v16i8_v16i8_v16i8, )(uchar16 x, uchar16 y, uchar16 z);
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v2i16_v2i16_v2i16, )(short2 x, short2 y, short2 z);
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v3i16_v3i16_v3i16, )(short3 x, short3 y, short3 z);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v4i16_v4i16_v4i16, )(short4 x, short4 y, short4 z);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v8i16_v8i16_v8i16, )(short8 x, short8 y, short8 z);
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v16i16_v16i16_v16i16, )(short16 x, short16 y, short16 z);
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v2i16_v2i16_v2i16, )(ushort2 x, ushort2 y, ushort2 z);
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v3i16_v3i16_v3i16, )(ushort3 x, ushort3 y, ushort3 z);
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v4i16_v4i16_v4i16, )(ushort4 x, ushort4 y, ushort4 z);
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v8i16_v8i16_v8i16, )(ushort8 x, ushort8 y, ushort8 z);
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v16i16_v16i16_v16i16, )(ushort16 x, ushort16 y, ushort16 z);
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v2i32_v2i32_v2i32, )(int2 x, int2 y, int2 z);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v3i32_v3i32_v3i32, )(int3 x, int3 y, int3 z);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v4i32_v4i32_v4i32, )(int4 x, int4 y, int4 z);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v8i32_v8i32_v8i32, )(int8 x, int8 y, int8 z);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v16i32_v16i32_v16i32, )(int16 x, int16 y, int16 z);
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v2i32_v2i32_v2i32, )(uint2 x, uint2 y, uint2 z);
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v3i32_v3i32_v3i32, )(uint3 x, uint3 y, uint3 z);
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v4i32_v4i32_v4i32, )(uint4 x, uint4 y, uint4 z);
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v8i32_v8i32_v8i32, )(uint8 x, uint8 y, uint8 z);
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v16i32_v16i32_v16i32, )(uint16 x, uint16 y, uint16 z);
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v2i64_v2i64_v2i64, )(long2 x, long2 y, long2 z);
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v3i64_v3i64_v3i64, )(long3 x, long3 y, long3 z);
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v4i64_v4i64_v4i64, )(long4 x, long4 y, long4 z);
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v8i64_v8i64_v8i64, )(long8 x, long8 y, long8 z);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _v16i64_v16i64_v16i64, )(long16 x, long16 y, long16 z);
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v2i64_v2i64_v2i64, )(ulong2 x, ulong2 y, ulong2 z);
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v3i64_v3i64_v3i64, )(ulong3 x, ulong3 y, ulong3 z);
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v4i64_v4i64_v4i64, )(ulong4 x, ulong4 y, ulong4 z);
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v8i64_v8i64_v8i64, )(ulong8 x, ulong8 y, ulong8 z);
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _v16i64_v16i64_v16i64, )(ulong16 x, ulong16 y, ulong16 z);

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i8, )( char x );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i16, )( short x );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i32, )( int x );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i8, )( char2 x );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i8, )( char3 x );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i8, )( char4 x );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i8, )( char8 x );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i8, )( char16 x );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i16, )( short2 x );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i16, )( short3 x );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i16, )( short4 x );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i16, )( short8 x );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i16, )( short16 x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i32, )( int2 x );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i32, )( int3 x );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i32, )( int4 x );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i32, )( int8 x );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i32, )( int16 x );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i64, )( long x );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i64, )( long2 x );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i64, )( long3 x );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i64, )( long4 x );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i64, )( long8 x );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i64, )( long16 x );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _i8, )( char x );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v2i8, )( char2 x );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v3i8, )( char3 x );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v4i8, )( char4 x );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v8i8, )( char8 x );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v16i8, )( char16 x );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _i16, )( short x );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v2i16, )( short2 x );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v3i16, )( short3 x );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v4i16, )( short4 x );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v8i16, )( short8 x );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v16i16, )( short16 x );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _i32, )( int x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v2i32, )( int2 x );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v3i32, )( int3 x );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v4i32, )( int4 x );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v8i32, )( int8 x );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v16i32, )( int16 x );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _i64, )( long x );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v2i64, )( long2 x );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v3i64, )( long3 x );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v4i64, )( long4 x );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v8i64, )( long8 x );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ctz, _v16i64, )( long16 x );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )( char x, char y );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i8_v2i8, )( char2 x, char2 y );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i8_v3i8, )( char3 x, char3 y );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i8_v4i8, )( char4 x, char4 y );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i8_v8i8, )( char8 x, char8 y );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i8_v16i8, )( uchar16 x, uchar16 y );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )( short x, short y );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i16_v2i16, )( short2 x, short2 y );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i16_v3i16, )( short3 x, short3 y );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i16_v4i16, )( short4 x, short4 y );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i16_v8i16, )( short8 x, short8 y );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i16_v16i16, )( ushort16 x, ushort16 y );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i32_v16i32, )( uint16 x, uint16 y );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )( long x, long y );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i64_v2i64, )( long2 x, long2 y );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i64_v3i64, )( long3 x, long3 y );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i64_v4i64, )( long4 x, long4 y );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i64_v8i64, )( long8 x, long8 y );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i64_v16i64, )( ulong16 x, ulong16 y );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _i8_i8_i8, )( char a, char b, char c );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v2i8_v2i8_v2i8, )( char2 a, char2 b, char2 c );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v3i8_v3i8_v3i8, )( char3 a, char3 b, char3 c );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v4i8_v4i8_v4i8, )( char4 a, char4 b, char4 c );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v8i8_v8i8_v8i8, )( char8 a, char8 b, char8 c );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v16i8_v16i8_v16i8, )( char16 a, char16 b, char16 c );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _i8_i8_i8, )( uchar a, uchar b, uchar c );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v2i8_v2i8_v2i8, )( uchar2 a, uchar2 b, uchar2 c );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v3i8_v3i8_v3i8, )( uchar3 a, uchar3 b, uchar3 c );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v4i8_v4i8_v4i8, )( uchar4 a, uchar4 b, uchar4 c );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v8i8_v8i8_v8i8, )( uchar8 a, uchar8 b, uchar8 c );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v16i8_v16i8_v16i8, )( uchar16 a, uchar16 b, uchar16 c );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _i16_i16_i16, )( short a, short b, short c );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v2i16_v2i16_v2i16, )( short2 a, short2 b, short2 c );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v3i16_v3i16_v3i16, )( short3 a, short3 b, short3 c );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v4i16_v4i16_v4i16, )( short4 a, short4 b, short4 c );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v8i16_v8i16_v8i16, )( short8 a, short8 b, short8 c );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v16i16_v16i16_v16i16, )( short16 a, short16 b, short16 c );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _i16_i16_i16, )( ushort a, ushort b, ushort c );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v2i16_v2i16_v2i16, )( ushort2 a, ushort2 b, ushort2 c );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v3i16_v3i16_v3i16, )( ushort3 a, ushort3 b, ushort3 c );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v4i16_v4i16_v4i16, )( ushort4 a, ushort4 b, ushort4 c );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v8i16_v8i16_v8i16, )( ushort8 a, ushort8 b, ushort8 c );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v16i16_v16i16_v16i16, )( ushort16 a, ushort16 b, ushort16 c );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _i32_i32_i32, )( int a, int b, int c );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v2i32_v2i32_v2i32, )( int2 a, int2 b, int2 c );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v3i32_v3i32_v3i32, )( int3 a, int3 b, int3 c );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v4i32_v4i32_v4i32, )( int4 a, int4 b, int4 c );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v8i32_v8i32_v8i32, )( int8 a, int8 b, int8 c );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v16i32_v16i32_v16i32, )( int16 a, int16 b, int16 c );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _i32_i32_i32, )( uint a, uint b, uint c );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v2i32_v2i32_v2i32, )( uint2 a, uint2 b, uint2 c );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v3i32_v3i32_v3i32, )( uint3 a, uint3 b, uint3 c );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v4i32_v4i32_v4i32, )( uint4 a, uint4 b, uint4 c );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v8i32_v8i32_v8i32, )( uint8 a, uint8 b, uint8 c );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v16i32_v16i32_v16i32, )( uint16 a, uint16 b, uint16 c );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _i64_i64_i64, )( long a, long b, long c );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v2i64_v2i64_v2i64, )( long2 a, long2 b, long2 c );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v3i64_v3i64_v3i64, )( long3 a, long3 b, long3 c );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v4i64_v4i64_v4i64, )( long4 a, long4 b, long4 c );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v8i64_v8i64_v8i64, )( long8 a, long8 b, long8 c );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_hi, _v16i64_v16i64_v16i64, )( long16 a, long16 b, long16 c );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _i64_i64_i64, )( ulong a, ulong b, ulong c );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v2i64_v2i64_v2i64, )( ulong2 a, ulong2 b, ulong2 c );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v3i64_v3i64_v3i64, )( ulong3 a, ulong3 b, ulong3 c );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v4i64_v4i64_v4i64, )( ulong4 a, ulong4 b, ulong4 c );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v8i64_v8i64_v8i64, )( ulong8 a, ulong8 b, ulong8 c );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_hi, _v16i64_v16i64_v16i64, )( ulong16 a, ulong16 b, ulong16 c );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _i8_i8_i8, )( char a, char b, char c );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v2i8_v2i8_v2i8, )( char2 a, char2 b, char2 c );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v3i8_v3i8_v3i8, )( char3 a, char3 b, char3 c );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v4i8_v4i8_v4i8, )( char4 a, char4 b, char4 c );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v8i8_v8i8_v8i8, )( char8 a, char8 b, char8 c );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v16i8_v16i8_v16i8, )( char16 a, char16 b, char16 c );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _i8_i8_i8, )( uchar a, uchar b, uchar c );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v2i8_v2i8_v2i8, )( uchar2 a, uchar2 b, uchar2 c );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v3i8_v3i8_v3i8, )( uchar3 a, uchar3 b, uchar3 c );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v4i8_v4i8_v4i8, )( uchar4 a, uchar4 b, uchar4 c );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v8i8_v8i8_v8i8, )( uchar8 a, uchar8 b, uchar8 c );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v16i8_v16i8_v16i8, )( uchar16 a, uchar16 b, uchar16 c );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _i16_i16_i16, )( short a, short b, short c );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v2i16_v2i16_v2i16, )( short2 a, short2 b, short2 c );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v3i16_v3i16_v3i16, )( short3 a, short3 b, short3 c );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v4i16_v4i16_v4i16, )( short4 a, short4 b, short4 c );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v8i16_v8i16_v8i16, )( short8 a, short8 b, short8 c );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v16i16_v16i16_v16i16, )( short16 a, short16 b, short16 c );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _i16_i16_i16, )( ushort a, ushort b, ushort c );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v2i16_v2i16_v2i16, )( ushort2 a, ushort2 b, ushort2 c );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v3i16_v3i16_v3i16, )( ushort3 a, ushort3 b, ushort3 c );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v4i16_v4i16_v4i16, )( ushort4 a, ushort4 b, ushort4 c );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v8i16_v8i16_v8i16, )( ushort8 a, ushort8 b, ushort8 c );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v16i16_v16i16_v16i16, )( ushort16 a, ushort16 b, ushort16 c );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _i32_i32_i32, )( int a, int b, int c );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v2i32_v2i32_v2i32, )( int2 a, int2 b, int2 c );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v3i32_v3i32_v3i32, )( int3 a, int3 b, int3 c );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v4i32_v4i32_v4i32, )( int4 a, int4 b, int4 c );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v8i32_v8i32_v8i32, )( int8 a, int8 b, int8 c );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v16i32_v16i32_v16i32, )( int16 a, int16 b, int16 c );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _i32_i32_i32, )( uint a, uint b, uint c );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v2i32_v2i32_v2i32, )( uint2 a, uint2 b, uint2 c );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v3i32_v3i32_v3i32, )( uint3 a, uint3 b, uint3 c );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v4i32_v4i32_v4i32, )( uint4 a, uint4 b, uint4 c );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v8i32_v8i32_v8i32, )( uint8 a, uint8 b, uint8 c );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v16i32_v16i32_v16i32, )( uint16 a, uint16 b, uint16 c );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _i64_i64_i64, )( long a, long b, long c );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v2i64_v2i64_v2i64, )( long2 a, long2 b, long2 c );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v3i64_v3i64_v3i64, )( long3 a, long3 b, long3 c );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v4i64_v4i64_v4i64, )( long4 a, long4 b, long4 c );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v8i64_v8i64_v8i64, )( long8 a, long8 b, long8 c );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad_sat, _v16i64_v16i64_v16i64, )( long16 a, long16 b, long16 c );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _i64_i64_i64, )( ulong a, ulong b, ulong c );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v2i64_v2i64_v2i64, )( ulong2 a, ulong2 b, ulong2 c );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v3i64_v3i64_v3i64, )( ulong3 a, ulong3 b, ulong3 c );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v4i64_v4i64_v4i64, )( ulong4 a, ulong4 b, ulong4 c );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v8i64_v8i64_v8i64, )( ulong8 a, ulong8 b, ulong8 c );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad_sat, _v16i64_v16i64_v16i64, )( ulong16 a, ulong16 b, ulong16 c );

int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _i32_i32_i32, )( int x, int y, int z );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _v2i32_v2i32_v2i32, )( int2 x, int2 y, int2 z );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _v3i32_v3i32_v3i32, )( int3 x, int3 y, int3 z );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _v4i32_v4i32_v4i32, )( int4 x, int4 y, int4 z );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _v8i32_v8i32_v8i32, )( int8 x, int8 y, int8 z );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mad24, _v16i32_v16i32_v16i32, )( int16 x, int16 y, int16 z );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _i32_i32_i32, )( uint x, uint y, uint z );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _v2i32_v2i32_v2i32, )( uint2 x, uint2 y, uint2 z );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _v3i32_v3i32_v3i32, )( uint3 x, uint3 y, uint3 z );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _v4i32_v4i32_v4i32, )( uint4 x, uint4 y, uint4 z );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _v8i32_v8i32_v8i32, )( uint8 x, uint8 y, uint8 z );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mad24, _v16i32_v16i32_v16i32, )( uint16 x, uint16 y, uint16 z );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _i8_i8, )(char x, char y);
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v2i8_v2i8, )(char2 x, char2 y);
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v3i8_v3i8, )(char3 x, char3 y);
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v4i8_v4i8, )(char4 x, char4 y);
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v8i8_v8i8, )(char8 x, char8 y);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v16i8_v16i8, )(char16 x, char16 y);
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _i8_i8, )(uchar x, uchar y);
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v2i8_v2i8, )(uchar2 x, uchar2 y);
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v3i8_v3i8, )(uchar3 x, uchar3 y);
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v4i8_v4i8, )(uchar4 x, uchar4 y);
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v8i8_v8i8, )(uchar8 x, uchar8 y);
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v16i8_v16i8, )(uchar16 x, uchar16 y);
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _i16_i16, )(short x, short y);
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v2i16_v2i16, )(short2 x, short2 y);
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v3i16_v3i16, )(short3 x, short3 y);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v4i16_v4i16, )(short4 x, short4 y);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v8i16_v8i16, )(short8 x, short8 y);
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v16i16_v16i16, )(short16 x, short16 y);
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _i16_i16, )(ushort x, ushort y);
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v2i16_v2i16, )(ushort2 x, ushort2 y);
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v3i16_v3i16, )(ushort3 x, ushort3 y);
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v4i16_v4i16, )(ushort4 x, ushort4 y);
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v8i16_v8i16, )(ushort8 x, ushort8 y);
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v16i16_v16i16, )(ushort16 x, ushort16 y);
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _i32_i32, )(int x, int y);
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v2i32_v2i32, )(int2 x, int2 y);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v3i32_v3i32, )(int3 x, int3 y);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v4i32_v4i32, )(int4 x, int4 y);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v8i32_v8i32, )(int8 x, int8 y);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v16i32_v16i32, )(int16 x, int16 y);
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _i32_i32, )(uint x, uint y);
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v2i32_v2i32, )(uint2 x, uint2 y);
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v3i32_v3i32, )(uint3 x, uint3 y);
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v4i32_v4i32, )(uint4 x, uint4 y);
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v8i32_v8i32, )(uint8 x, uint8 y);
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v16i32_v16i32, )(uint16 x, uint16 y);
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _i64_i64, )(long x, long y);
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v2i64_v2i64, )(long2 x, long2 y);
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v3i64_v3i64, )(long3 x, long3 y);
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v4i64_v4i64, )(long4 x, long4 y);
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v8i64_v8i64, )(long8 x, long8 y);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_max, _v16i64_v16i64, )(long16 x, long16 y);
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _i64_i64, )(ulong x, ulong y);
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v2i64_v2i64, )(ulong2 x, ulong2 y);
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v3i64_v3i64, )(ulong3 x, ulong3 y);
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v4i64_v4i64, )(ulong4 x, ulong4 y);
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v8i64_v8i64, )(ulong8 x, ulong8 y);
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_max, _v16i64_v16i64, )(ulong16 x, ulong16 y);

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _i8_i8, )( char x, char y );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v2i8_v2i8, )( char2 x, char2 y );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v3i8_v3i8, )( char3 x, char3 y );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v4i8_v4i8, )( char4 x, char4 y );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v8i8_v8i8, )( char8 x, char8 y );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v16i8_v16i8, )( uchar16 x, uchar16 y );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _i16_i16, )( short x, short y );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v2i16_v2i16, )( short2 x, short2 y );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v3i16_v3i16, )( short3 x, short3 y );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v4i16_v4i16, )( short4 x, short4 y );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v8i16_v8i16, )( short8 x, short8 y );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v16i16_v16i16, )( ushort16 x, ushort16 y );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v16i32_v16i32, )( uint16 x, uint16 y );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _i64_i64, )( long x, long y );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v2i64_v2i64, )( long2 x, long2 y );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v3i64_v3i64, )( long3 x, long3 y );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v4i64_v4i64, )( long4 x, long4 y );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v8i64_v8i64, )( long8 x, long8 y );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul_hi, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul_hi, _v16i64_v16i64, )( ulong16 x, ulong16 y );

int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v16i32_v16i32, )( uint16 x, uint16 y );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _i8, )(char x );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v2i8, )(char2 x);
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v3i8, )(char3 x);
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v4i8, )(char4 x);
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v8i8, )(char8 x);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v16i8, )(char16 x);
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _i16, )(short x );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v2i16, )(short2 x);
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v3i16, )(short3 x);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v4i16, )(short4 x);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v8i16, )(short8 x);
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v16i16, )(short16 x);
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _i32, )(int x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v2i32, )(int2 x);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v3i32, )(int3 x);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v4i32, )(int4 x);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v8i32, )(int8 x);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v16i32, )(int16 x);
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _i64, )(long x);
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v2i64, )(long2 x);
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v3i64, )(long3 x);
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v4i64, )(long4 x);
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v8i64, )(long8 x);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _v16i64, )(long16 x);

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _i8_i8, )( char x, char y );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v2i8_v2i8, )( char2 x, char2 y );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v3i8_v3i8, )( char3 x, char3 y );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v4i8_v4i8, )( char4 x, char4 y );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v8i8_v8i8, )( char8 x, char8 y );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v16i8_v16i8, )( uchar16 x, uchar16 y );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _i16_i16, )( short x, short y );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v2i16_v2i16, )( short2 x, short2 y );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v3i16_v3i16, )( short3 x, short3 y );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v4i16_v4i16, )( short4 x, short4 y );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v8i16_v8i16, )( short8 x, short8 y );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v16i16_v16i16, )( ushort16 x, ushort16 y );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v16i32_v16i32, )( uint16 x, uint16 y );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _i64_i64, )( long x, long y );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v2i64_v2i64, )( long2 x, long2 y );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v3i64_v3i64, )( long3 x, long3 y );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v4i64_v4i64, )( long4 x, long4 y );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v8i64_v8i64, )( long8 x, long8 y );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_rhadd, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_rhadd, _v16i64_v16i64, )( ulong16 x, ulong16 y );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i8_i8, )( char v, char i );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i8_v2i8, )( char2 v, char2 i );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i8_v3i8, )( char3 v, char3 i );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i8_v4i8, )( char4 v, char4 i );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i8_v8i8, )( char8 v, char8 i );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i8_v16i8, )( char16 v, char16 i );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i16_i16, )( short v, short i );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i16_v2i16, )( short2 v, short2 i );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i16_v3i16, )( short3 v, short3 i );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i16_v4i16, )( short4 v, short4 i );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i16_v8i16, )( short8 v, short8 i );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i16_v16i16, )( short16 v, short16 i );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i32_i32, )( int v, int i );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i32_v2i32, )( int2 v, int2 i );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i32_v3i32, )( int3 v, int3 i );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i32_v4i32, )( int4 v, int4 i );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i32_v8i32, )( int8 v, int8 i );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i32_v16i32, )( int16 v, int16 i );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i64_i64, )( long v, long i );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i64_v2i64, )( long2 v, long2 i );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i64_v3i64, )( long3 v, long3 i );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i64_v4i64, )( long4 v, long4 i );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i64_v8i64, )( long8 v, long8 i );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i64_v16i64, )( long16 v, long16 i );

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _i8_i8, )( char x, char y );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v2i8_v2i8, )( char2 x, char2 y );
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v3i8_v3i8, )( char3 x, char3 y );
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v4i8_v4i8, )( char4 x, char4 y );
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v8i8_v8i8, )( char8 x, char8 y );
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v16i8_v16i8, )( char16 x, char16 y );
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _i8_i8, )( uchar x, uchar y );
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v2i8_v2i8, )( uchar2 x, uchar2 y );
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v3i8_v3i8, )( uchar3 x, uchar3 y );
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v4i8_v4i8, )( uchar4 x, uchar4 y );
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v8i8_v8i8, )( uchar8 x, uchar8 y );
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v16i8_v16i8, )( uchar16 x, uchar16 y );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _i16_i16, )( short x, short y );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v2i16_v2i16, )( short2 x, short2 y );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v3i16_v3i16, )( short3 x, short3 y );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v4i16_v4i16, )( short4 x, short4 y );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v8i16_v8i16, )( short8 x, short8 y );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v16i16_v16i16, )( short16 x, short16 y );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _i16_i16, )( ushort x, ushort y );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v2i16_v2i16, )( ushort2 x, ushort2 y );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v3i16_v3i16, )( ushort3 x, ushort3 y );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v4i16_v4i16, )( ushort4 x, ushort4 y );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v8i16_v8i16, )( ushort8 x, ushort8 y );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v16i16_v16i16, )( ushort16 x, ushort16 y );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _i32_i32, )( int x, int y );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v2i32_v2i32, )( int2 x, int2 y );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v3i32_v3i32, )( int3 x, int3 y );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v4i32_v4i32, )( int4 x, int4 y );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v8i32_v8i32, )( int8 x, int8 y );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v16i32_v16i32, )( int16 x, int16 y );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _i32_i32, )( uint x, uint y );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v2i32_v2i32, )( uint2 x, uint2 y );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v3i32_v3i32, )( uint3 x, uint3 y );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v4i32_v4i32, )( uint4 x, uint4 y );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v8i32_v8i32, )( uint8 x, uint8 y );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v16i32_v16i32, )( uint16 x, uint16 y );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _i64_i64, )( long x, long y );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v2i64_v2i64, )( long2 x, long2 y );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v3i64_v3i64, )( long3 x, long3 y );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v4i64_v4i64, )( long4 x, long4 y );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v8i64_v8i64, )( long8 x, long8 y );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_sub_sat, _v16i64_v16i64, )( long16 x, long16 y );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _i64_i64, )( ulong x, ulong y );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v2i64_v2i64, )( ulong2 x, ulong2 y );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v3i64_v3i64, )( ulong3 x, ulong3 y );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v4i64_v4i64, )( ulong4 x, ulong4 y );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v8i64_v8i64, )( ulong8 x, ulong8 y );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_sub_sat, _v16i64_v16i64, )( ulong16 x, ulong16 y );

short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )( char  hi, uchar lo );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i8_v2i8, )( char2  hi, uchar2 lo );
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i8_v3i8, )( char3  hi, uchar3 lo );
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i8_v4i8, )( char4  hi, uchar4 lo );
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i8_v8i8, )( char8  hi, uchar8 lo );
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i8_v16i8, )( char16  hi, uchar16 lo );
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )( uchar hi, uchar lo );
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i8_v2i8, )( uchar2 hi, uchar2 lo );
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i8_v3i8, )( uchar3 hi, uchar3 lo );
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i8_v4i8, )( uchar4 hi, uchar4 lo );
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i8_v8i8, )( uchar8 hi, uchar8 lo );
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i8_v16i8, )( uchar16 hi, uchar16 lo );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )( short  hi, ushort lo );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i16_v2i16, )( short2  hi, ushort2 lo );
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i16_v3i16, )( short3  hi, ushort3 lo );
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i16_v4i16, )( short4  hi, ushort4 lo );
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i16_v8i16, )( short8  hi, ushort8 lo );
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i16_v16i16, )( short16  hi, ushort16 lo );
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )( ushort hi, ushort lo );
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i16_v2i16, )( ushort2 hi, ushort2 lo );
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i16_v3i16, )( ushort3 hi, ushort3 lo );
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i16_v4i16, )( ushort4 hi, ushort4 lo );
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i16_v8i16, )( ushort8 hi, ushort8 lo );
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i16_v16i16, )( ushort16 hi, ushort16 lo );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )( int  hi, uint lo );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i32_v2i32, )( int2  hi, uint2 lo );
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i32_v3i32, )( int3  hi, uint3 lo );
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i32_v4i32, )( int4  hi, uint4 lo );
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i32_v8i32, )( int8  hi, uint8 lo );
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i32_v16i32, )( int16  hi, uint16 lo );
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )( uint hi, uint lo );
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i32_v2i32, )( uint2 hi, uint2 lo );
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i32_v3i32, )( uint3 hi, uint3 lo );
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i32_v4i32, )( uint4 hi, uint4 lo );
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i32_v8i32, )( uint8 hi, uint8 lo );
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i32_v16i32, )( uint16 hi, uint16 lo );


//
//  Math_ext
//        -acos,acosh,acospi,asin,asinh,asinpi,atan,atan2,atan2pi,atanh,atanpi,cbrt,ceil,copysign,
//       cos,cosh,cospi,divide_cr,erf,erfc,exp,exp2,exp10,expm1,fabs,fdim,floor,fma,fmax,fmin,
//       fmod,fract,frexp,hypot,ilogb,ldexp,lgamma,lgamma_r,log,log1p,log2,log10,logb,mad,maxmag,minmag,
//         modf,nan,nextafter,pow,pown,powr,remainder,remquo,rint,rootn,round,rsqrt,sin,sincos,
//       sinh,sinpi,sqrt,sqrt_cr,tan,tanh,tanpi,tgamma,trunc
//

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acos, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acosh, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(acospi, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _f32, )(float value );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asin, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinh, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(asinpi, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _f32, )(float value );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _f32_f32, )( float y, float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _f64_f64, )( double y, double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _f16_f16, )( half y, half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2, _v16f16_v16f16, )(half16 x, half16 y);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atan2pi, _v16f16_v16f16, )(half16 x, half16 y);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanh, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(atanpi, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ceil, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(copysign, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cosh, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cospi, _v16f16, )(half16 x);

float __builtin_spirv_divide_cr_f32_f32( float a, float b );
float2 __builtin_spirv_divide_cr_v2f32_v2f32( float2 a, float2 b );
float3 __builtin_spirv_divide_cr_v3f32_v3f32( float3 a, float3 b );
float4 __builtin_spirv_divide_cr_v4f32_v4f32( float4 a, float4 b );
float8 __builtin_spirv_divide_cr_v8f32_v8f32( float8 a, float8 b );
float16 __builtin_spirv_divide_cr_v16f32_v16f32( float16 a, float16 b );

#if defined(cl_khr_fp64)
double __builtin_spirv_divide_cr_f64_f64(double a, double b);
#endif // defined(cl_khr_fp64)

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erf, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _f32, )(float x);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp10, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f32, )( float a );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(expm1, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fabs, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fdim, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(floor, _v16f16, )(half16 x);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( float a, float b, float c );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )( double a, double b, double c );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(half x, half y, half z);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fma, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f32_v16f32, )(float16 x, float16 y);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f32_f32, )(float2 x, float y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f32_f32, )(float3 x, float y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f32_f32, )(float4 x, float y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f32_f32, )(float8 x, float y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f32_f32, )(float16 x, float y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f64_v16f64, )(double16 x, double16 y);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f64_f64, )(double2 x, double y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f64_f64, )(double3 x, double y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f64_f64, )(double4 x, double y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f64_f64, )(double8 x, double y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f64_f64, )(double16 x, double y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f16_v16f16, )(half16 x, half16 y);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v2f16_f16, )(half2 x, half y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v3f16_f16, )(half3 x, half y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v4f16_f16, )(half4 x, half y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v8f16_f16, )(half8 x, half y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmax, _v16f16_f16, )(half16 x, half y);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmin, _v16f16_v16f16, )(half16 x, half16 y);

float  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f32_f32, )( float xx, float yy );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f64_f64, )( double xx, double yy );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fmod, _v16f16_v16f16, )(half16 x, half16 y);

/* Helper function for fmod */
float  __builtin_spirv_fast_fmod_f32_f32( float xx, float yy );
float2  __builtin_spirv_fast_fmod_v2f32_v2f32( float2 xx, float2 yy );
float3  __builtin_spirv_fast_fmod_v3f32_v3f32( float3 xx, float3 yy );
float4  __builtin_spirv_fast_fmod_v4f32_v4f32( float4 xx, float4 yy );
half  __builtin_spirv_fast_fmod_f16_f16( half xx, half yy );
half2  __builtin_spirv_fast_fmod_v2f16_v2f16( half2 xx, half2 yy );
half3  __builtin_spirv_fast_fmod_v3f16_v3f16( half3 xx, half3 yy );
half4  __builtin_spirv_fast_fmod_v4f16_v4f16( half4 xx, half4 yy );
#if defined(cl_khr_fp64)
double  __builtin_spirv_fast_fmod_f64_f64( double xx, double yy );
double2  __builtin_spirv_fast_fmod_v2f64_v2f64( double2 xx, double2 yy );
double3  __builtin_spirv_fast_fmod_v3f64_v3f64( double3 xx, double3 yy );
double4  __builtin_spirv_fast_fmod_v4f64_v4f64( double4 xx, double4 yy );
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p1f32, )( float x, __global float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p1v2f32, )( float2 x, __global float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p1v3f32, )( float3 x, __global float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p1v4f32, )( float4 x, __global float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p1v8f32, )( float8 x, __global float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p1v16f32, )( float16 x, __global float16* iptr );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p0f32, )( float x, __private float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p0v2f32, )( float2 x, __private float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p0v3f32, )( float3 x, __private float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p0v4f32, )( float4 x, __private float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p0v8f32, )( float8 x, __private float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p0v16f32, )( float16 x, __private float16* iptr );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p3f32, )( float x, __local float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p3v2f32, )( float2 x, __local float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p3v3f32, )( float3 x, __local float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p3v4f32, )( float4 x, __local float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p3v8f32, )( float8 x, __local float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p3v16f32, )( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f32_p4f32, )( float x, __generic float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f32_p4v2f32, )( float2 x, __generic float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f32_p4v3f32, )( float3 x, __generic float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f32_p4v4f32, )( float4 x, __generic float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f32_p4v8f32, )( float8 x, __generic float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f32_p4v16f32, )( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p1f16, )( half x, __global half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p1v2f16, )( half2 x, __global half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p1v3f16, )( half3 x, __global half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p1v4f16, )( half4 x, __global half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p1v8f16, )( half8 x, __global half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p1v16f16, )( half16 x, __global half16* iptr );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p0f16, )( half x, __private half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p0v2f16, )( half2 x, __private half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p0v3f16, )( half3 x, __private half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p0v4f16, )( half4 x, __private half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p0v8f16, )( half8 x, __private half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p0v16f16, )( half16 x, __private half16* iptr );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p3f16, )( half x, __local half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p3v2f16, )( half2 x, __local half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p3v3f16, )( half3 x, __local half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p3v4f16, )( half4 x, __local half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p3v8f16, )( half8 x, __local half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p3v16f16, )( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f16_p4f16, )( half x, __generic half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f16_p4v2f16, )( half2 x, __generic half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f16_p4v3f16, )( half3 x, __generic half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f16_p4v4f16, )( half4 x, __generic half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f16_p4v8f16, )( half8 x, __generic half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f16_p4v16f16, )( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p1f64, )( double x, __global double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p1v2f64, )( double2 x, __global double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p1v3f64, )( double3 x, __global double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p1v4f64, )( double4 x, __global double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p1v8f64, )( double8 x, __global double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p1v16f64, )( double16 x, __global double16* iptr );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p0f64, )( double x, __private double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p0v2f64, )( double2 x, __private double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p0v3f64, )( double3 x, __private double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p0v4f64, )( double4 x, __private double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p0v8f64, )( double8 x, __private double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p0v16f64, )( double16 x, __private double16* iptr );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p3f64, )( double x, __local double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p3v2f64, )( double2 x, __local double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p3v3f64, )( double3 x, __local double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p3v4f64, )( double4 x, __local double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p3v8f64, )( double8 x, __local double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p3v16f64, )( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _f64_p4f64, )( double x, __generic double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v2f64_p4v2f64, )( double2 x, __generic double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v3f64_p4v3f64, )( double3 x, __generic double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v4f64_p4v4f64, )( double4 x, __generic double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v8f64_p4v8f64, )( double8 x, __generic double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(fract, _v16f64_p4v16f64, )( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p1i32, )( float x, __global int* exp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p1v2i32, )( float2 x, __global int2* exp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p1v3i32, )( float3 x, __global int3* exp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p1v4i32, )( float4 x, __global int4* exp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p1v8i32, )( float8 x, __global int8* exp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p1v16i32, )( float16 x, __global int16* exp );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( float x, __private int* exp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p0v2i32, )( float2 x, __private int2* exp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p0v3i32, )( float3 x, __private int3* exp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p0v4i32, )( float4 x, __private int4* exp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p0v8i32, )( float8 x, __private int8* exp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p0v16i32, )( float16 x, __private int16* exp );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p3i32, )( float x, __local int* exp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p3v2i32, )( float2 x, __local int2* exp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p3v3i32, )( float3 x, __local int3* exp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p3v4i32, )( float4 x, __local int4* exp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p3v8i32, )( float8 x, __local int8* exp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p3v16i32, )( float16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( float x, __generic int* exp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p4v2i32, )( float2 x, __generic int2* exp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p4v3i32, )( float3 x, __generic int3* exp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p4v4i32, )( float4 x, __generic int4* exp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p4v8i32, )( float8 x, __generic int8* exp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p4v16i32, )( float16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p1i32, )( half x, __global int* exp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p1v2i32, )( half2 x, __global int2* exp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p1v3i32, )( half3 x, __global int3* exp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p1v4i32, )( half4 x, __global int4* exp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p1v8i32, )( half8 x, __global int8* exp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p1v16i32, )( half16 x, __global int16* exp );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( half x, __private int* exp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p0v2i32, )( half2 x, __private int2* exp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p0v3i32, )( half3 x, __private int3* exp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p0v4i32, )( half4 x, __private int4* exp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p0v8i32, )( half8 x, __private int8* exp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p0v16i32, )( half16 x, __private int16* exp );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p3i32, )( half x, __local int* exp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p3v2i32, )( half2 x, __local int2* exp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p3v3i32, )( half3 x, __local int3* exp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p3v4i32, )( half4 x, __local int4* exp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p3v8i32, )( half8 x, __local int8* exp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p3v16i32, )( half16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( half x, __generic int* exp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p4v2i32, )( half2 x, __generic int2* exp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p4v3i32, )( half3 x, __generic int3* exp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p4v4i32, )( half4 x, __generic int4* exp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p4v8i32, )( half8 x, __generic int8* exp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p4v16i32, )( half16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p1i32, )( double x, __global int* exp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p1v2i32, )( double2 x, __global int2* exp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p1v3i32, )( double3 x, __global int3* exp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p1v4i32, )( double4 x, __global int4* exp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p1v8i32, )( double8 x, __global int8* exp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p1v16i32, )( double16 x, __global int16* exp );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( double x, __private int* exp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p0v2i32, )( double2 x, __private int2* exp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p0v3i32, )( double3 x, __private int3* exp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p0v4i32, )( double4 x, __private int4* exp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p0v8i32, )( double8 x, __private int8* exp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p0v16i32, )( double16 x, __private int16* exp );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p3i32, )( double x, __local int* exp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p3v2i32, )( double2 x, __local int2* exp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p3v3i32, )( double3 x, __local int3* exp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p3v4i32, )( double4 x, __local int4* exp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p3v8i32, )( double8 x, __local int8* exp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p3v16i32, )( double16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( double x, __generic int* exp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p4v2i32, )( double2 x, __generic int2* exp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p4v3i32, )( double3 x, __generic int3* exp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p4v4i32, )( double4 x, __generic int4* exp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p4v8i32, )( double8 x, __generic int8* exp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p4v16i32, )( double16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(hypot, _v16f16_v16f16, )(half16 x, half16 y);

int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f32, )( float x );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f32, )( float x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v2f32, )(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v3f32, )(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v4f32, )(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v8f32, )(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f64, )( double x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v2f64, )(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v3f64, )(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v4f64, )(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v8f64, )(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _f16, )( half x );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v2f16, )(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v3f16, )(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v4f16, )(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v8f16, )(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ilogb, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( float x, int n );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v2f32_v2i32, )(float2 x, int2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v3f32_v3i32, )(float3 x, int3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v4f32_v4i32, )(float4 x, int4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v8f32_v8i32, )(float8 x, int8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v16f32_v16i32, )(float16 x, int16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( double x, int n );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v2f64_v2i32, )(double2 x, int2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v3f64_v3i32, )(double3 x, int3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v4f64_v4i32, )(double4 x, int4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v8f64_v8i32, )(double8 x, int8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v16f64_v16i32, )(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _f16_i32, )( half x, int n );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v2f16_v2i32, )(half2 x, int2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v3f16_v3i32, )(half3 x, int3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v4f16_v4i32, )(half4 x, int4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v8f16_v8i32, )(half8 x, int8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(ldexp, _v16f16_v16i32, )(half16 x, int16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p1i32, )( float x, __global int* signp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p1v2i32, )( float2 x, __global int2* signp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p1v3i32, )( float3 x, __global int3* signp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p1v4i32, )( float4 x, __global int4* signp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p1v8i32, )( float8 x, __global int8* signp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p1v16i32, )( float16 x, __global int16* signp );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p3i32, )( float x, __local int* signp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p3v2i32, )( float2 x, __local int2* signp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p3v3i32, )( float3 x, __local int3* signp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p3v4i32, )( float4 x, __local int4* signp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p3v8i32, )( float8 x, __local int8* signp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p3v16i32, )( float16 x, __local int16* signp );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p0i32, )( float x, __private int* signp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p0v2i32, )( float2 x, __private int2* signp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p0v3i32, )( float3 x, __private int3* signp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p0v4i32, )( float4 x, __private int4* signp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p0v8i32, )( float8 x, __private int8* signp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p0v16i32, )( float16 x, __private int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f32_p4i32, )( float x, __generic int* signp );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f32_p4v2i32, )( float2 x, __generic int2* signp );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f32_p4v3i32, )( float3 x, __generic int3* signp );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f32_p4v4i32, )( float4 x, __generic int4* signp );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f32_p4v8i32, )( float8 x, __generic int8* signp );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f32_p4v16i32, )( float16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p1i32, )( half x, __global int* signp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p1v2i32, )( half2 x, __global int2* signp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p1v3i32, )( half3 x, __global int3* signp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p1v4i32, )( half4 x, __global int4* signp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p1v8i32, )( half8 x, __global int8* signp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p1v16i32, )( half16 x, __global int16* signp );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p0i32, )( half x, __private int* signp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p0v2i32, )( half2 x, __private int2* signp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p0v3i32, )( half3 x, __private int3* signp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p0v4i32, )( half4 x, __private int4* signp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p0v8i32, )( half8 x, __private int8* signp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p0v16i32, )( half16 x, __private int16* signp );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p3i32, )( half x, __local int* signp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p3v2i32, )( half2 x, __local int2* signp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p3v3i32, )( half3 x, __local int3* signp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p3v4i32, )( half4 x, __local int4* signp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p3v8i32, )( half8 x, __local int8* signp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p3v16i32, )( half16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f16_p4i32, )( half x, __generic int* signp );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f16_p4v2i32, )( half2 x, __generic int2* signp );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f16_p4v3i32, )( half3 x, __generic int3* signp );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f16_p4v4i32, )( half4 x, __generic int4* signp );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f16_p4v8i32, )( half8 x, __generic int8* signp );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f16_p4v16i32, )( half16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p1i32, )( double x, __global int* signp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p1v2i32, )( double2 x, __global int2* signp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p1v3i32, )( double3 x, __global int3* signp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p1v4i32, )( double4 x, __global int4* signp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p1v8i32, )( double8 x, __global int8* signp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p1v16i32, )( double16 x, __global int16* signp );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p0i32, )( double x, __private int* signp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p0v2i32, )( double2 x, __private int2* signp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p0v3i32, )( double3 x, __private int3* signp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p0v4i32, )( double4 x, __private int4* signp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p0v8i32, )( double8 x, __private int8* signp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p0v16i32, )( double16 x, __private int16* signp );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p3i32, )( double x, __local int* signp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p3v2i32, )( double2 x, __local int2* signp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p3v3i32, )( double3 x, __local int3* signp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p3v4i32, )( double4 x, __local int4* signp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p3v8i32, )( double8 x, __local int8* signp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p3v16i32, )( double16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _f64_p4i32, )( double x, __generic int* signp );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v2f64_p4v2i32, )( double2 x, __generic int2* signp );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v3f64_p4v3i32, )( double3 x, __generic int3* signp );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v4f64_p4v4i32, )( double4 x, __generic int4* signp );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v8f64_p4v8i32, )( double8 x, __generic int8* signp );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(lgamma_r, _v16f64_p4v16i32, )( double16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _f32, )( float a );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _f64, )( double a );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log1p, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log2, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(log10, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(logb, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )( float a, float b, float c );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _f64_f64_f64, )( double a, double b, double c );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _f16_f16_f16, )(half x, half y, half z);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(maxmag, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(minmag, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f32_p1f32, )( float x, __global float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f32_p1v2f32, )( float2 x, __global float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f32_p1v3f32, )( float3 x, __global float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f32_p1v4f32, )( float4 x, __global float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f32_p1v8f32, )( float8 x, __global float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f32_p1v16f32, )( float16 x, __global float16* iptr );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f32_p0f32, )( float x, __private float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f32_p0v2f32, )( float2 x, __private float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f32_p0v3f32, )( float3 x, __private float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f32_p0v4f32, )( float4 x, __private float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f32_p0v8f32, )( float8 x, __private float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f32_p0v16f32, )( float16 x, __private float16* iptr );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f32_p3f32, )( float x, __local float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f32_p3v2f32, )( float2 x, __local float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f32_p3v3f32, )( float3 x, __local float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f32_p3v4f32, )( float4 x, __local float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f32_p3v8f32, )( float8 x, __local float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f32_p3v16f32, )( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f32_p4f32, )( float x, __generic float* iptr );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f32_p4v2f32, )( float2 x, __generic float2* iptr );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f32_p4v3f32, )( float3 x, __generic float3* iptr );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f32_p4v4f32, )( float4 x, __generic float4* iptr );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f32_p4v8f32, )( float8 x, __generic float8* iptr );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f32_p4v16f32, )( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f16_p1f16, )( half x, __global half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f16_p1v2f16, )( half2 x, __global half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f16_p1v3f16, )( half3 x, __global half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f16_p1v4f16, )( half4 x, __global half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f16_p1v8f16, )( half8 x, __global half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f16_p1v16f16, )( half16 x, __global half16* iptr );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f16_p0f16, )( half x, __private half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f16_p0v2f16, )( half2 x, __private half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f16_p0v3f16, )( half3 x, __private half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f16_p0v4f16, )( half4 x, __private half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f16_p0v8f16, )( half8 x, __private half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f16_p0v16f16, )( half16 x, __private half16* iptr );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f16_p3f16, )( half x, __local half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f16_p3v2f16, )( half2 x, __local half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f16_p3v3f16, )( half3 x, __local half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f16_p3v4f16, )( half4 x, __local half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f16_p3v8f16, )( half8 x, __local half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f16_p3v16f16, )( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f16_p4f16, )( half x, __generic half* iptr );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f16_p4v2f16, )( half2 x, __generic half2* iptr );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f16_p4v3f16, )( half3 x, __generic half3* iptr );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f16_p4v4f16, )( half4 x, __generic half4* iptr );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f16_p4v8f16, )( half8 x, __generic half8* iptr );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f16_p4v16f16, )( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f64_p1f64, )( double x, __global double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f64_p1v2f64, )( double2 x, __global double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f64_p1v3f64, )( double3 x, __global double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f64_p1v4f64, )( double4 x, __global double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f64_p1v8f64, )( double8 x, __global double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f64_p1v16f64, )( double16  x, __global double16* iptr );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f64_p0f64, )( double x, __private double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f64_p0v2f64, )( double2 x, __private double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f64_p0v3f64, )( double3 x, __private double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f64_p0v4f64, )( double4 x, __private double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f64_p0v8f64, )( double8 x, __private double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f64_p0v16f64, )( double16   x, __private double16* iptr );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f64_p3f64, )( double x, __local double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f64_p3v2f64, )( double2 x, __local double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f64_p3v3f64, )( double3 x, __local double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f64_p3v4f64, )( double4 x, __local double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f64_p3v8f64, )( double8 x, __local double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f64_p3v16f64, )( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _f64_p4f64, )( double x, __generic double* iptr );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v2f64_p4v2f64, )( double2 x, __generic double2* iptr );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v3f64_p4v3f64, )( double3 x, __generic double3* iptr );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v4f64_p4v4f64, )( double4 x, __generic double4* iptr );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v8f64_p4v8f64, )( double8 x, __generic double8* iptr );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(modf, _v16f64_p4v16f64, )( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _i32, )( int nancode );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v2i32, )(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v3i32, )(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v4i32, )(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v8i32, )(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v16i32, )(int16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _i64, )( long nancode );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v2i64, )(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v3i64, )(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v4i64, )(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v8i64, )(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v16i64, )(long16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _i16, )( short nancode );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v2i16, )(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v3i16, )(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v4i16, )(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v8i16, )(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nan, _v16i16, )(short16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(nextafter, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pow, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _f32_i32, )( float x, int y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v2f32_v2i32, )(float2 x, int2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v3f32_v3i32, )(float3 x, int3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v4f32_v4i32, )(float4 x, int4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v8f32_v8i32, )(float8 x, int8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v16f32_v16i32, )(float16 x, int16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _f64_i32, )( double x, int y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v2f64_v2i32, )(double2 x, int2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v3f64_v3i32, )(double3 x, int3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v4f64_v4i32, )(double4 x, int4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v8f64_v8i32, )(double8 x, int8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v16f64_v16i32, )(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _f16_i32, )( half x, int y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v2f16_v2i32, )(half2 x, int2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v3f16_v3i32, )(half3 x, int3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v4f16_v4i32, )(half4 x, int4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v8f16_v8i32, )(half8 x, int8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(pown, _v16f16_v16i32, )(half16 x, int16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _f16_f16, )( half y, half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remainder, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p1i32, )( float xx, float yy, __global int* quo );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p1v2i32, )( float2 xx, float2 yy, __global int2* quo );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p1v3i32, )( float3 xx, float3 yy, __global int3* quo );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p1v4i32, )( float4 xx, float4 yy, __global int4* quo );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p1v8i32, )( float8 xx, float8 yy, __global int8* quo );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p1v16i32, )( float16 xx, float16 yy, __global int16* quo );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )( float xx, float yy, __private int* quo );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p0v2i32, )( float2 xx, float2 yy, __private int2* quo );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p0v3i32, )( float3 xx, float3 yy, __private int3* quo );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p0v4i32, )( float4 xx, float4 yy, __private int4* quo );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p0v8i32, )( float8 xx, float8 yy, __private int8* quo );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p0v16i32, )( float16 xx, float16 yy, __private int16* quo );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p3i32, )( float xx, float yy, __local int* quo );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p3v2i32, )( float2 xx, float2 yy, __local int2* quo );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p3v3i32, )( float3 xx, float3 yy, __local int3* quo );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p3v4i32, )( float4 xx, float4 yy, __local int4* quo );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p3v8i32, )( float8 xx, float8 yy, __local int8* quo );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p3v16i32, )( float16 xx, float16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )( float xx, float yy, __generic int* quo );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p4v2i32, )( float2 xx, float2 yy, __generic int2* quo );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p4v3i32, )( float3 xx, float3 yy, __generic int3* quo );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p4v4i32, )( float4 xx, float4 yy, __generic int4* quo );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p4v8i32, )( float8 xx, float8 yy, __generic int8* quo );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p4v16i32, )( float16 xx, float16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p1i32, )( half xx, half yy, __global int* quo );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p1v2i32, )( half2 xx, half2 yy, __global int2* quo );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p1v3i32, )( half3 xx, half3 yy, __global int3* quo );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p1v4i32, )( half4 xx, half4 yy, __global int4* quo );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p1v8i32, )( half8 xx, half8 yy, __global int8* quo );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p1v16i32, )( half16 xx, half16 yy, __global int16* quo );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )( half xx, half yy, __private int* quo );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p0v2i32, )( half2 xx, half2 yy, __private int2* quo );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p0v3i32, )( half3 xx, half3 yy, __private int3* quo );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p0v4i32, )( half4 xx, half4 yy, __private int4* quo );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p0v8i32, )( half8 xx, half8 yy, __private int8* quo );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p0v16i32, )( half16 xx, half16 yy, __private int16* quo );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p3i32, )( half xx, half yy, __local int* quo );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p3v2i32, )( half2 xx, half2 yy, __local int2* quo );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p3v3i32, )( half3 xx, half3 yy, __local int3* quo );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p3v4i32, )( half4 xx, half4 yy, __local int4* quo );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p3v8i32, )( half8 xx, half8 yy, __local int8* quo );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p3v16i32, )( half16 xx, half16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )( half xx, half yy, __generic int* quo );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p4v2i32, )( half2 xx, half2 yy, __generic int2* quo );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p4v3i32, )( half3 xx, half3 yy, __generic int3* quo );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p4v4i32, )( half4 xx, half4 yy, __generic int4* quo );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p4v8i32, )( half8 xx, half8 yy, __generic int8* quo );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p4v16i32, )( half16 xx, half16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p1i32, )( double xx, double yy, __global int* quo );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p1v2i32, )( double2 xx, double2 yy, __global int2* quo );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p1v3i32, )( double3 xx, double3 yy, __global int3* quo );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p1v4i32, )( double4 xx, double4 yy, __global int4* quo );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p1v8i32, )( double8 xx, double8 yy, __global int8* quo );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p1v16i32, )( double16 xx, double16 yy, __global int16* quo );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )( double xx, double yy, __private int* quo );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p0v2i32, )( double2 xx, double2 yy, __private int2* quo );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p0v3i32, )( double3 xx, double3 yy, __private int3* quo );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p0v4i32, )( double4 xx, double4 yy, __private int4* quo );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p0v8i32, )( double8 xx, double8 yy, __private int8* quo );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p0v16i32, )( double16 xx, double16 yy, __private int16* quo );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p3i32, )( double xx, double yy, __local int* quo );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p3v2i32, )( double2 xx, double2 yy, __local int2* quo );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p3v3i32, )( double3 xx, double3 yy, __local int3* quo );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p3v4i32, )( double4 xx, double4 yy, __local int4* quo );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p3v8i32, )( double8 xx, double8 yy, __local int8* quo );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p3v16i32, )( double16 xx, double16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )( double xx, double yy, __generic int* quo );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p4v2i32, )( double2 xx, double2 yy, __generic int2* quo );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p4v3i32, )( double3 xx, double3 yy, __generic int3* quo );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p4v4i32, )( double4 xx, double4 yy, __generic int4* quo );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p4v8i32, )( double8 xx, double8 yy, __generic int8* quo );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p4v16i32, )( double16 xx, double16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rint, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f32_i32, )( float x, int n );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v2f32_v2i32, )(float2 x, int2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v3f32_v3i32, )(float3 x, int3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v4f32_v4i32, )(float4 x, int4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v8f32_v8i32, )(float8 x, int8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v16f32_v16i32, )(float16 x, int16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f64_i32, )( double y, int x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v2f64_v2i32, )(double2 x, int2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v3f64_v3i32, )(double3 x, int3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v4f64_v4i32, )(double4 x, int4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v8f64_v8i32, )(double8 x, int8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v16f64_v16i32, )(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f16_i32, )( half y, int x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v2f16_v2i32, )(half2 x, int2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v3f16_v3i32, )(half3 x, int3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v4f16_v4i32, )(half4 x, int4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v8f16_v8i32, )(half8 x, int8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _v16f16_v16i32, )(half16 x, int16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(round, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rsqrt, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sin, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f32_p0f32, )( float x, __private float* cosval );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f32_p1f32, )( float x, __global float* cosval );
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f32_p3f32, )( float x, __local float* cosval );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f32_p0v2f32, )( float2 x, __private float2* cosval );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f32_p1v2f32, )( float2 x, __global float2* cosval );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f32_p3v2f32, )( float2 x, __local float2* cosval );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f32_p0v3f32, )( float3 x, __private float3* cosval );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f32_p1v3f32, )( float3 x, __global float3* cosval );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f32_p3v3f32, )( float3 x, __local float3* cosval );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f32_p0v4f32, )( float4 x, __private float4* cosval );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f32_p1v4f32, )( float4 x, __global float4* cosval );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f32_p3v4f32, )( float4 x, __local float4* cosval );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f32_p0v8f32, )( float8 x, __private float8* cosval );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f32_p1v8f32, )( float8 x, __global float8* cosval );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f32_p3v8f32, )( float8 x, __local float8* cosval );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f32_p0v16f32, )( float16 x, __private float16* cosval );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f32_p1v16f32, )( float16 x, __global float16* cosval );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f32_p3v16f32, )( float16 x, __local float16* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f32_p4f32, )( float x, __generic float* cosval );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f32_p4v2f32, )( float2 x, __generic float2* cosval );
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f32_p4v3f32, )( float3 x, __generic float3* cosval );
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f32_p4v4f32, )( float4 x, __generic float4* cosval );
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f32_p4v8f32, )( float8 x, __generic float8* cosval );
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f32_p4v16f32, )( float16 x, __generic float16* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f16_p0f16, )( half x, __private half* cosval );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f16_p1f16, )( half x, __global half* cosval );
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f16_p3f16, )( half x, __local half* cosval );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f16_p0v2f16, )( half2 x, __private half2* cosval );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f16_p1v2f16, )( half2 x, __global half2* cosval );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f16_p3v2f16, )( half2 x, __local half2* cosval );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f16_p0v3f16, )( half3 x, __private half3* cosval );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f16_p1v3f16, )( half3 x, __global half3* cosval );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f16_p3v3f16, )( half3 x, __local half3* cosval );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f16_p0v4f16, )( half4 x, __private half4* cosval );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f16_p1v4f16, )( half4 x, __global half4* cosval );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f16_p3v4f16, )( half4 x, __local half4* cosval );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f16_p0v8f16, )( half8 x, __private half8* cosval );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f16_p1v8f16, )( half8 x, __global half8* cosval );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f16_p3v8f16, )( half8 x, __local half8* cosval );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f16_p0v16f16, )( half16 x, __private half16* cosval );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f16_p1v16f16, )( half16 x, __global half16* cosval );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f16_p3v16f16, )( half16 x, __local half16* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f16_p4f16, )( half x, __generic half* cosval );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f16_p4v2f16, )( half2 x, __generic half2* cosval );
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f16_p4v3f16, )( half3 x, __generic half3* cosval );
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f16_p4v4f16, )( half4 x, __generic half4* cosval );
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f16_p4v8f16, )( half8 x, __generic half8* cosval );
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f16_p4v16f16, )( half16 x, __generic half16* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f64_p0f64, )( double x, __private double* cosval );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f64_p3f64, )( double x, __local double* cosval );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f64_p1f64, )( double x, __global double* cosval );
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f64_p3f64, )( double x, __local double* cosval );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f64_p0v2f64, )( double2 x, __private double2* cosval );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f64_p1v2f64, )( double2 x, __global double2* cosval );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f64_p3v2f64, )( double2 x, __local double2* cosval );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f64_p0v3f64, )( double3 x, __private double3* cosval );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f64_p1v3f64, )( double3 x, __global double3* cosval );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f64_p3v3f64, )( double3 x, __local double3* cosval );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f64_p0v4f64, )( double4 x, __private double4* cosval );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f64_p1v4f64, )( double4 x, __global double4* cosval );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f64_p3v4f64, )( double4 x, __local double4* cosval );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f64_p0v8f64, )( double8 x, __private double8* cosval );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f64_p1v8f64, )( double8 x, __global double8* cosval );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f64_p3v8f64, )( double8 x, __local double8* cosval );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f64_p0v16f64, )( double16 x, __private double16* cosval );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f64_p1v16f64, )( double16 x, __global double16* cosval );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f64_p3v16f64, )( double16 x, __local double16* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _f64_p4f64, )( double x, __generic double* cosval );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v2f64_p4v2f64, )( double2 x, __generic double2* cosval );
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v3f64_p4v3f64, )( double3 x, __generic double3* cosval );
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v4f64_p4v4f64, )( double4 x, __generic double4* cosval );
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v8f64_p4v8f64, )( double8 x, __generic double8* cosval );
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sincos, _v16f32_p4v16f64, )( double16 x, __generic double16* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinh, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sinpi, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f32, )( float a );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v16f32, )(float16 x);
#ifdef cl_fp64_basic_ops
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v16f64, )(double16 x);
#endif // cl_fp64_basic_ops
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _f16, )( half a );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(sqrt_cr, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tan, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanpi, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tgamma, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(trunc, _v16f16, )(half16 x);

//
//  Native
//        -native_cos,native_divide,native_exp,native_exp2,native_exp10,native_log,native_log2,
//         native_log10,native_powr,native_recip,native_rsqrt,native_sin,native_sqrt,native_tan
//

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_cos, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f32_f32, )( float x, float y );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _f16_f16, )( half x, half y );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_divide, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp2, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log2, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f32, )( float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f16, )( half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _f32_f32, )(float x, float y);
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v2f32_v2f32, )(float2 x, float2 y);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v3f32_v3f32, )(float3 x, float3 y);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v4f32_v4f32, )(float4 x, float4 y);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v8f32_v8f32, )(float8 x, float8 y);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _f64_f64, )( double x, double y );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v2f64_v2f64, )(double2 x, double2 y);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v3f64_v3f64, )(double3 x, double3 y);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v4f64_v4f64, )(double4 x, double4 y);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v8f64_v8f64, )(double8 x, double8 y);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _f16_f16, )(half x, half y);
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v2f16_v2f16, )(half2 x, half2 y);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v3f16_v3f16, )(half3 x, half3 y);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v4f16_v4f16, )(half4 x, half4 y);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v8f16_v8f16, )(half8 x, half8 y);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_powr, _v16f16_v16f16, )(half16 x, half16 y);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_recip, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_rsqrt, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v16f64, )(double16 x);
#endif // cl_khr_fp64
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sin, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v16f32, )(float16 x);
#ifdef cl_fp64_basic_ops
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _f64, )(double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v16f64, )(double16 x);
#endif // cl_fp64_basic_ops
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_sqrt, _v16f16, )(half16 x);

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _f32, )(float x );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _f64, )( double x );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v2f64, )(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v3f64, )(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v4f64, )(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v8f64, )(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _f16, )(half x );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v2f16, )(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v3f16, )(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v4f16, )(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v8f16, )(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_tan, _v16f16, )(half16 x);

//
//  Relational
//        -bitselect,select
//

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i8_i8_i8, )( char a, char b, char c );
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2i8_v2i8_v2i8, )(char2 x, char2 y, char2 z);
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3i8_v3i8_v3i8, )(char3 x, char3 y, char3 z);
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4i8_v4i8_v4i8, )(char4 x, char4 y, char4 z);
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8i8_v8i8_v8i8, )(char8 x, char8 y, char8 z);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16i8_v16i8_v16i8, )(char16 x, char16 y, char16 z);
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i16_i16_i16, )( short a, short b, short c );
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2i16_v2i16_v2i16, )(short2 x, short2 y, short2 z);
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3i16_v3i16_v3i16, )(short3 x, short3 y, short3 z);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4i16_v4i16_v4i16, )(short4 x, short4 y, short4 z);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8i16_v8i16_v8i16, )(short8 x, short8 y, short8 z);
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16i16_v16i16_v16i16, )(short16 x, short16 y, short16 z);
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i32_i32_i32, )( int a, int b, int c );
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2i32_v2i32_v2i32, )(int2 x, int2 y, int2 z);
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3i32_v3i32_v3i32, )(int3 x, int3 y, int3 z);
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4i32_v4i32_v4i32, )(int4 x, int4 y, int4 z);
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8i32_v8i32_v8i32, )(int8 x, int8 y, int8 z);
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16i32_v16i32_v16i32, )(int16 x, int16 y, int16 z);
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i64_i64_i64, )( long a, long b, long c );
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2i64_v2i64_v2i64, )(long2 x, long2 y, long2 z);
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3i64_v3i64_v3i64, )(long3 x, long3 y, long3 z);
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4i64_v4i64_v4i64, )(long4 x, long4 y, long4 z);
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8i64_v8i64_v8i64, )(long8 x, long8 y, long8 z);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16i64_v16i64_v16i64, )(long16 x, long16 y, long16 z);
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f32_f32_f32, )( float a, float b, float c );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2f32_v2f32_v2f32, )(float2 x, float2 y, float2 z);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3f32_v3f32_v3f32, )(float3 x, float3 y, float3 z);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4f32_v4f32_v4f32, )(float4 x, float4 y, float4 z);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8f32_v8f32_v8f32, )(float8 x, float8 y, float8 z);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16f32_v16f32_v16f32, )(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f64_f64_f64, )( double a, double b, double c );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2f64_v2f64_v2f64, )(double2 x, double2 y, double2 z);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3f64_v3f64_v3f64, )(double3 x, double3 y, double3 z);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4f64_v4f64_v4f64, )(double4 x, double4 y, double4 z);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8f64_v8f64_v8f64, )(double8 x, double8 y, double8 z);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16f64_v16f64_v16f64, )(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f16_f16_f16, )( half a, half b, half c );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v2f16_v2f16_v2f16, )(half2 x, half2 y, half2 z);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v3f16_v3f16_v3f16, )(half3 x, half3 y, half3 z);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v4f16_v4f16_v4f16, )(half4 x, half4 y, half4 z);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v8f16_v8f16_v8f16, )(half8 x, half8 y, half8 z);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _v16f16_v16f16_v16f16, )(half16 x, half16 y, half16 z);

char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( char a, char b, char c );
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( short a, short b, short c );
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( int a, int b, int c );
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( long a, long b, long c );

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( float a, float b, int c );
float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v2f32_v2f32_v2i32, )(float2 a, float2 b, int2 c);
float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v3f32_v3f32_v3i32, )(float3 a, float3 b, int3 c);
float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v4f32_v4f32_v4i32, )(float4 a, float4 b, int4 c);
float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v8f32_v8f32_v8i32, )(float8 a, float8 b, int8 c);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v16f32_v16f32_v16i32, )(float16 a, float16 b, int16 c);

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( double a, double b, long c );
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v2f64_v2f64_v2i64, )(double2 a, double2 b, long2 c);
double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v3f64_v3f64_v3i64, )(double3 a, double3 b, long3 c);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v4f64_v4f64_v4i64, )(double4 a, double4 b, long4 c);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v8f64_v8f64_v8i64, )(double8 a, double8 b, long8 c);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v16f64_v16f64_v16i64, )(double16 a, double16 b, long16 c);

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( half a, half b, short c );
half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v2f16_v2f16_v2i16, )(half2 a, half2 b, short2 c);
half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v3f16_v3f16_v3i16, )(half3 a, half3 b, short3 c);
half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v4f16_v4f16_v4i16, )(half4 a, half4 b, short4 c);
half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v8f16_v8f16_v8i16, )(half8 a, half8 b, short8 c);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _v16f16_v16f16_v16i16, )(half16 a, half16 b, short16 c);

#endif // defined(cl_khr_fp16)

#endif // __SPIRV_MATH_H__

