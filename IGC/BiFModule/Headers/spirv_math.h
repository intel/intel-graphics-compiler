/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_MATH_H__
#define __SPIRV_MATH_H__

//
//  Common
//        -degrees,fclamp,fmax_common,fmin_common,mix,radians,sign,smoothstep,step
//

float __attribute__((overloadable))   __spirv_ocl_degrees(float r);
float2 __attribute__((overloadable))  __spirv_ocl_degrees(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_degrees(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_degrees(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_degrees(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_degrees(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_degrees(double r);
double2 __attribute__((overloadable))  __spirv_ocl_degrees(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_degrees(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_degrees(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_degrees(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_degrees(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_degrees(half r);
half2 __attribute__((overloadable))    __spirv_ocl_degrees(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_degrees(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_degrees(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_degrees(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_degrees(half16 x);

float __attribute__((overloadable))   __spirv_ocl_fclamp(float x, float y, float z);
float2 __attribute__((overloadable))  __spirv_ocl_fclamp(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable))  __spirv_ocl_fclamp(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable))  __spirv_ocl_fclamp(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable))  __spirv_ocl_fclamp(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable)) __spirv_ocl_fclamp(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_fclamp(double x, double y, double z);
double2 __attribute__((overloadable)) __spirv_ocl_fclamp(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable)) __spirv_ocl_fclamp(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable)) __spirv_ocl_fclamp(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable)) __spirv_ocl_fclamp(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_fclamp(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))    __spirv_ocl_fclamp(half x, half y, half z);
half2 __attribute__((overloadable))   __spirv_ocl_fclamp(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))   __spirv_ocl_fclamp(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))   __spirv_ocl_fclamp(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))   __spirv_ocl_fclamp(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable))  __spirv_ocl_fclamp(half16 x, half16 y, half16 z);

float __attribute__((overloadable))   __spirv_ocl_fmax_common(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_fmax_common(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fmax_common(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fmax_common(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fmax_common(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fmax_common(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fmax_common(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_fmax_common(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fmax_common(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fmax_common(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fmax_common(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fmax_common(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fmax_common(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_fmax_common(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fmax_common(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fmax_common(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fmax_common(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fmax_common(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_fmin_common(float x, float y);
half __attribute__((overloadable))    __spirv_ocl_fmin_common(half x, half y);
float2 __attribute__((overloadable))  __spirv_ocl_fmin_common(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fmin_common(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fmin_common(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fmin_common(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fmin_common(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fmin_common(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_fmin_common(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fmin_common(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fmin_common(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fmin_common(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fmin_common(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half2 __attribute__((overloadable))    __spirv_ocl_fmin_common(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fmin_common(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fmin_common(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fmin_common(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fmin_common(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_mix(float x, float y, float a);
float2 __attribute__((overloadable))  __spirv_ocl_mix(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable))  __spirv_ocl_mix(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable))  __spirv_ocl_mix(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable))  __spirv_ocl_mix(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable)) __spirv_ocl_mix(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_mix(double x, double y, double a);
double2 __attribute__((overloadable)) __spirv_ocl_mix(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable)) __spirv_ocl_mix(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable)) __spirv_ocl_mix(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable)) __spirv_ocl_mix(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_mix(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_mix(half x, half y, half a);
half2 __attribute__((overloadable))  __spirv_ocl_mix(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))  __spirv_ocl_mix(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))  __spirv_ocl_mix(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))  __spirv_ocl_mix(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable)) __spirv_ocl_mix(half16 x, half16 y, half16 z);

float __attribute__((overloadable))   __spirv_ocl_radians(float d);
float2 __attribute__((overloadable))  __spirv_ocl_radians(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_radians(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_radians(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_radians(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_radians(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_radians(double d);
double2 __attribute__((overloadable))  __spirv_ocl_radians(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_radians(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_radians(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_radians(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_radians(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_radians(half d);
half2 __attribute__((overloadable))    __spirv_ocl_radians(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_radians(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_radians(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_radians(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_radians(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sign(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sign(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sign(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sign(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sign(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sign(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_sign(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sign(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sign(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sign(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sign(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sign(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_sign(half x);
half2 __attribute__((overloadable))    __spirv_ocl_sign(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_sign(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_sign(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_sign(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_sign(half16 x);

float __attribute__((overloadable))
__spirv_ocl_smoothstep(float edge0, float edge1, float x);
float2 __attribute__((overloadable)) __spirv_ocl_smoothstep(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable)) __spirv_ocl_smoothstep(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable)) __spirv_ocl_smoothstep(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable)) __spirv_ocl_smoothstep(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable))
__spirv_ocl_smoothstep(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_ocl_smoothstep(double edge0, double edge1, double x);
double2 __attribute__((overloadable))
__spirv_ocl_smoothstep(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable))
__spirv_ocl_smoothstep(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable))
__spirv_ocl_smoothstep(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable))
__spirv_ocl_smoothstep(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_smoothstep(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable)) __spirv_ocl_smoothstep(half edge0, half edge1, half x);
half2 __attribute__((overloadable))  __spirv_ocl_smoothstep(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))  __spirv_ocl_smoothstep(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))  __spirv_ocl_smoothstep(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))  __spirv_ocl_smoothstep(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable)) __spirv_ocl_smoothstep(half16 x, half16 y, half16 z);

float __attribute__((overloadable))   __spirv_ocl_step(float edge, float x);
float2 __attribute__((overloadable))  __spirv_ocl_step(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_step(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_step(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_step(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_step(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_step(double edge, double x);
double2 __attribute__((overloadable))  __spirv_ocl_step(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_step(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_step(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_step(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_step(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_step(half edge, half x);
half2 __attribute__((overloadable))    __spirv_ocl_step(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_step(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_step(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_step(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_step(half16 x, half16 y);

//
//  Geometric
//        -cross,distance,fast_distance,fast_length,fast_normalize,length,normalize
//

float3 __attribute__((overloadable)) __spirv_ocl_cross(float3 p0, float3 p1);
float4 __attribute__((overloadable)) __spirv_ocl_cross(float4 p0, float4 p1);
#if defined(cl_khr_fp64)
double3 __attribute__((overloadable)) __spirv_ocl_cross(double3 p0, double3 p1);
double4 __attribute__((overloadable)) __spirv_ocl_cross(double4 p0, double4 p1);
#endif // defined(cl_khr_fp64)
half3 __attribute__((overloadable))   __spirv_ocl_cross(half3 p0, half3 p1);
half4 __attribute__((overloadable))   __spirv_ocl_cross(half4 p0, half4 p1);

float __attribute__((overloadable)) __spirv_ocl_distance(float p0, float p1);
float __attribute__((overloadable)) __spirv_ocl_distance(float2 p0, float2 p1);
float __attribute__((overloadable)) __spirv_ocl_distance(float3 p0, float3 p1);
float __attribute__((overloadable)) __spirv_ocl_distance(float4 p0, float4 p1);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ocl_distance(double p0, double p1);
double __attribute__((overloadable)) __spirv_ocl_distance(double2 p0, double2 p1);
double __attribute__((overloadable)) __spirv_ocl_distance(double3 p0, double3 p1);
double __attribute__((overloadable)) __spirv_ocl_distance(double4 p0, double4 p1);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_distance(half p0, half p1);
half __attribute__((overloadable))   __spirv_ocl_distance(half2 p0, half2 p1);
half __attribute__((overloadable))   __spirv_ocl_distance(half3 p0, half3 p1);
half __attribute__((overloadable))   __spirv_ocl_distance(half4 p0, half4 p1);

float __attribute__((overloadable)) __spirv_ocl_fast_distance(float p0, float p1);
float __attribute__((overloadable)) __spirv_ocl_fast_distance(float2 p0, float2 p1);
float __attribute__((overloadable)) __spirv_ocl_fast_distance(float3 p0, float3 p1);
float __attribute__((overloadable)) __spirv_ocl_fast_distance(float4 p0, float4 p1);

float __attribute__((overloadable)) __spirv_ocl_fast_length(float p);
float __attribute__((overloadable)) __spirv_ocl_fast_length(float2 p);
float __attribute__((overloadable)) __spirv_ocl_fast_length(float3 p);
float __attribute__((overloadable)) __spirv_ocl_fast_length(float4 p);

float __attribute__((overloadable))  __spirv_ocl_fast_normalize(float p);
float2 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float2 p);
float3 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float3 p);
float4 __attribute__((overloadable)) __spirv_ocl_fast_normalize(float4 p);

float __attribute__((overloadable)) __spirv_ocl_length(float p);
float __attribute__((overloadable)) __spirv_ocl_length(float2 p);
float __attribute__((overloadable)) __spirv_ocl_length(float3 p);
float __attribute__((overloadable)) __spirv_ocl_length(float4 p);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ocl_length(double p);
double __attribute__((overloadable)) __spirv_ocl_length(double2 p);
double __attribute__((overloadable)) __spirv_ocl_length(double3 p);
double __attribute__((overloadable)) __spirv_ocl_length(double4 p);
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_ocl_length(half p);
half __attribute__((overloadable)) __spirv_ocl_length(half2 p);
half __attribute__((overloadable)) __spirv_ocl_length(half3 p);
half __attribute__((overloadable)) __spirv_ocl_length(half4 p);
#endif // defined(cl_khr_fp16)

float __attribute__((overloadable))  __spirv_ocl_normalize(float p);
float2 __attribute__((overloadable)) __spirv_ocl_normalize(float2 p);
float3 __attribute__((overloadable)) __spirv_ocl_normalize(float3 p);
float4 __attribute__((overloadable)) __spirv_ocl_normalize(float4 p);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_normalize(double p);
double2 __attribute__((overloadable)) __spirv_ocl_normalize(double2 p);
double3 __attribute__((overloadable)) __spirv_ocl_normalize(double3 p);
double4 __attribute__((overloadable)) __spirv_ocl_normalize(double4 p);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))    __spirv_ocl_normalize(half p);
half2 __attribute__((overloadable))   __spirv_ocl_normalize(half2 p);
half3 __attribute__((overloadable))   __spirv_ocl_normalize(half3 p);
half4 __attribute__((overloadable))   __spirv_ocl_normalize(half4 p);

//
//  Half
//        -half_cos,half_divide,half_exp,half_exp2,half_exp10,half_log,half_log2,half_log10
//         half_powr,half_recip,half_rsqrt,half_sin,half_sqrt,half_tan
//

float __attribute__((overloadable))   __spirv_ocl_half_cos(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_cos(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_cos(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_cos(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_cos(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_cos(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_divide(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_half_divide(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_half_divide(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_half_divide(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_half_divide(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_half_divide(float16 x, float16 y);

float __attribute__((overloadable))   __spirv_ocl_half_exp(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_exp(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_exp(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_exp(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_exp(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_exp(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_exp2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_exp2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_exp2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_exp2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_exp2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_exp2(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_exp10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_exp10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_exp10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_exp10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_exp10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_exp10(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_log(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_log(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_log(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_log(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_log(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_log(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_log2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_log2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_log2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_log2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_log2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_log2(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_log10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_log10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_log10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_log10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_log10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_log10(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_powr(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_half_powr(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_half_powr(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_half_powr(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_half_powr(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_half_powr(float16 x, float16 y);

float __attribute__((overloadable))   __spirv_ocl_half_recip(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_recip(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_recip(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_recip(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_recip(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_recip(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_rsqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_rsqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_rsqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_rsqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_rsqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_rsqrt(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_sin(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_sin(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_sin(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_sin(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_sin(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_sin(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_sqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_sqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_sqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_sqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_sqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_sqrt(float16 x);

float __attribute__((overloadable))   __spirv_ocl_half_tan(float x);
float2 __attribute__((overloadable))  __spirv_ocl_half_tan(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_half_tan(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_half_tan(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_half_tan(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_half_tan(float16 x);

//
//  Integer (signed and unsigned )
//        -abs,abs_diff,add_sat,clamp,clz,ctz,hadd,mad_hi,mad_sat,mad24,max,min,mul_hi
//         mul24,popcnt,rhadd,rotate,sub_sat,upsample
//

uchar __attribute__((overloadable))    __spirv_ocl_s_abs(char x);
uchar2 __attribute__((overloadable))   __spirv_ocl_s_abs(char2 x);
uchar3 __attribute__((overloadable))   __spirv_ocl_s_abs(char3 x);
uchar4 __attribute__((overloadable))   __spirv_ocl_s_abs(char4 x);
uchar8 __attribute__((overloadable))   __spirv_ocl_s_abs(char8 x);
uchar16 __attribute__((overloadable))  __spirv_ocl_s_abs(char16 x);
uchar __attribute__((overloadable))    __spirv_ocl_u_abs(uchar x);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_abs(uchar2 x);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_abs(uchar3 x);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_abs(uchar4 x);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_abs(uchar8 x);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_abs(uchar16 x);
ushort __attribute__((overloadable))   __spirv_ocl_s_abs(short x);
ushort2 __attribute__((overloadable))  __spirv_ocl_s_abs(short2 x);
ushort3 __attribute__((overloadable))  __spirv_ocl_s_abs(short3 x);
ushort4 __attribute__((overloadable))  __spirv_ocl_s_abs(short4 x);
ushort8 __attribute__((overloadable))  __spirv_ocl_s_abs(short8 x);
ushort16 __attribute__((overloadable)) __spirv_ocl_s_abs(short16 x);
ushort __attribute__((overloadable))   __spirv_ocl_u_abs(ushort x);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_abs(ushort2 x);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_abs(ushort3 x);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_abs(ushort4 x);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_abs(ushort8 x);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_abs(ushort16 x);
uint __attribute__((overloadable))     __spirv_ocl_s_abs(int x);
uint2 __attribute__((overloadable))    __spirv_ocl_s_abs(int2 x);
uint3 __attribute__((overloadable))    __spirv_ocl_s_abs(int3 x);
uint4 __attribute__((overloadable))    __spirv_ocl_s_abs(int4 x);
uint8 __attribute__((overloadable))    __spirv_ocl_s_abs(int8 x);
uint16 __attribute__((overloadable))   __spirv_ocl_s_abs(int16 x);
uint __attribute__((overloadable))     __spirv_ocl_u_abs(uint x);
uint2 __attribute__((overloadable))    __spirv_ocl_u_abs(uint2 x);
uint3 __attribute__((overloadable))    __spirv_ocl_u_abs(uint3 x);
uint4 __attribute__((overloadable))    __spirv_ocl_u_abs(uint4 x);
uint8 __attribute__((overloadable))    __spirv_ocl_u_abs(uint8 x);
uint16 __attribute__((overloadable))   __spirv_ocl_u_abs(uint16 x);
ulong __attribute__((overloadable))    __spirv_ocl_s_abs(long x);
ulong2 __attribute__((overloadable))   __spirv_ocl_s_abs(long2 x);
ulong3 __attribute__((overloadable))   __spirv_ocl_s_abs(long3 x);
ulong4 __attribute__((overloadable))   __spirv_ocl_s_abs(long4 x);
ulong8 __attribute__((overloadable))   __spirv_ocl_s_abs(long8 x);
ulong16 __attribute__((overloadable))  __spirv_ocl_s_abs(long16 x);
ulong __attribute__((overloadable))    __spirv_ocl_u_abs(ulong x);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_abs(ulong2 x);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_abs(ulong3 x);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_abs(ulong4 x);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_abs(ulong8 x);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_abs(ulong16 x);

uchar __attribute__((overloadable))    __spirv_ocl_s_abs_diff(char x, char y);
uchar2 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(char2 x, char2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(char3 x, char3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(char4 x, char4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(char8 x, char8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_abs_diff(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(uchar16 x, uchar16 y);
ushort __attribute__((overloadable))   __spirv_ocl_s_abs_diff(short x, short y);
ushort2 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(short2 x, short2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(short3 x, short3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(short4 x, short4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(short8 x, short8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_s_abs_diff(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_abs_diff(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_abs_diff(ushort16 x, ushort16 y);
uint __attribute__((overloadable))     __spirv_ocl_s_abs_diff(int x, int y);
uint2 __attribute__((overloadable))    __spirv_ocl_s_abs_diff(int2 x, int2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_s_abs_diff(int3 x, int3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_s_abs_diff(int4 x, int4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_s_abs_diff(int8 x, int8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_abs_diff(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_abs_diff(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_abs_diff(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_abs_diff(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_abs_diff(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(uint16 x, uint16 y);
ulong __attribute__((overloadable))    __spirv_ocl_s_abs_diff(long x, long y);
ulong2 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(long2 x, long2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(long3 x, long3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(long4 x, long4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_s_abs_diff(long8 x, long8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_s_abs_diff(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_abs_diff(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_abs_diff(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_abs_diff(ulong16 x, ulong16 y);

char __attribute__((overloadable))     __spirv_ocl_s_add_sat(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_add_sat(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_add_sat(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_add_sat(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_add_sat(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_add_sat(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_add_sat(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_add_sat(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_add_sat(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_add_sat(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_add_sat(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_add_sat(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_add_sat(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_add_sat(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_add_sat(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_add_sat(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_add_sat(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_add_sat(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_add_sat(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_add_sat(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_add_sat(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_add_sat(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_add_sat(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_add_sat(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_add_sat(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_add_sat(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_add_sat(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_add_sat(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_add_sat(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_add_sat(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_add_sat(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_add_sat(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_add_sat(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_add_sat(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_add_sat(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_add_sat(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_add_sat(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_add_sat(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_add_sat(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_add_sat(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_add_sat(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_add_sat(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_add_sat(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_add_sat(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_add_sat(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_add_sat(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_add_sat(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_add_sat(ulong16 x, ulong16 y);

char __attribute__((overloadable)) __spirv_ocl_s_clamp(char x, char minval, char maxval);
uchar __attribute__((overloadable))
__spirv_ocl_u_clamp(uchar x, uchar minval, uchar maxval);
short __attribute__((overloadable))
__spirv_ocl_s_clamp(short x, short minval, short maxval);
ushort __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort x, ushort minval, ushort maxval);
int __attribute__((overloadable))  __spirv_ocl_s_clamp(int x, int minval, int maxval);
uint __attribute__((overloadable)) __spirv_ocl_u_clamp(uint x, uint minval, uint maxval);
long __attribute__((overloadable)) __spirv_ocl_s_clamp(long x, long minval, long maxval);
ulong __attribute__((overloadable))
__spirv_ocl_u_clamp(ulong x, ulong minval, ulong maxval);
char2 __attribute__((overloadable))  __spirv_ocl_s_clamp(char2 x, char2 y, char2 z);
char3 __attribute__((overloadable))  __spirv_ocl_s_clamp(char3 x, char3 y, char3 z);
char4 __attribute__((overloadable))  __spirv_ocl_s_clamp(char4 x, char4 y, char4 z);
char8 __attribute__((overloadable))  __spirv_ocl_s_clamp(char8 x, char8 y, char8 z);
char16 __attribute__((overloadable)) __spirv_ocl_s_clamp(char16 x, char16 y, char16 z);
uchar2 __attribute__((overloadable)) __spirv_ocl_u_clamp(uchar2 x, uchar2 y, uchar2 z);
uchar3 __attribute__((overloadable)) __spirv_ocl_u_clamp(uchar3 x, uchar3 y, uchar3 z);
uchar4 __attribute__((overloadable)) __spirv_ocl_u_clamp(uchar4 x, uchar4 y, uchar4 z);
uchar8 __attribute__((overloadable)) __spirv_ocl_u_clamp(uchar8 x, uchar8 y, uchar8 z);
uchar16 __attribute__((overloadable))
                                     __spirv_ocl_u_clamp(uchar16 x, uchar16 y, uchar16 z);
short2 __attribute__((overloadable)) __spirv_ocl_s_clamp(short2 x, short2 y, short2 z);
short3 __attribute__((overloadable)) __spirv_ocl_s_clamp(short3 x, short3 y, short3 z);
short4 __attribute__((overloadable)) __spirv_ocl_s_clamp(short4 x, short4 y, short4 z);
short8 __attribute__((overloadable)) __spirv_ocl_s_clamp(short8 x, short8 y, short8 z);
short16 __attribute__((overloadable))
__spirv_ocl_s_clamp(short16 x, short16 y, short16 z);
ushort2 __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort2 x, ushort2 y, ushort2 z);
ushort3 __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort3 x, ushort3 y, ushort3 z);
ushort4 __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort4 x, ushort4 y, ushort4 z);
ushort8 __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort8 x, ushort8 y, ushort8 z);
ushort16 __attribute__((overloadable))
__spirv_ocl_u_clamp(ushort16 x, ushort16 y, ushort16 z);
int2 __attribute__((overloadable))   __spirv_ocl_s_clamp(int2 x, int2 y, int2 z);
int3 __attribute__((overloadable))   __spirv_ocl_s_clamp(int3 x, int3 y, int3 z);
int4 __attribute__((overloadable))   __spirv_ocl_s_clamp(int4 x, int4 y, int4 z);
int8 __attribute__((overloadable))   __spirv_ocl_s_clamp(int8 x, int8 y, int8 z);
int16 __attribute__((overloadable))  __spirv_ocl_s_clamp(int16 x, int16 y, int16 z);
uint2 __attribute__((overloadable))  __spirv_ocl_u_clamp(uint2 x, uint2 y, uint2 z);
uint3 __attribute__((overloadable))  __spirv_ocl_u_clamp(uint3 x, uint3 y, uint3 z);
uint4 __attribute__((overloadable))  __spirv_ocl_u_clamp(uint4 x, uint4 y, uint4 z);
uint8 __attribute__((overloadable))  __spirv_ocl_u_clamp(uint8 x, uint8 y, uint8 z);
uint16 __attribute__((overloadable)) __spirv_ocl_u_clamp(uint16 x, uint16 y, uint16 z);
long2 __attribute__((overloadable))  __spirv_ocl_s_clamp(long2 x, long2 y, long2 z);
long3 __attribute__((overloadable))  __spirv_ocl_s_clamp(long3 x, long3 y, long3 z);
long4 __attribute__((overloadable))  __spirv_ocl_s_clamp(long4 x, long4 y, long4 z);
long8 __attribute__((overloadable))  __spirv_ocl_s_clamp(long8 x, long8 y, long8 z);
long16 __attribute__((overloadable)) __spirv_ocl_s_clamp(long16 x, long16 y, long16 z);
ulong2 __attribute__((overloadable)) __spirv_ocl_u_clamp(ulong2 x, ulong2 y, ulong2 z);
ulong3 __attribute__((overloadable)) __spirv_ocl_u_clamp(ulong3 x, ulong3 y, ulong3 z);
ulong4 __attribute__((overloadable)) __spirv_ocl_u_clamp(ulong4 x, ulong4 y, ulong4 z);
ulong8 __attribute__((overloadable)) __spirv_ocl_u_clamp(ulong8 x, ulong8 y, ulong8 z);
ulong16 __attribute__((overloadable))
__spirv_ocl_u_clamp(ulong16 x, ulong16 y, ulong16 z);

char __attribute__((overloadable))    __spirv_ocl_clz(char x);
short __attribute__((overloadable))   __spirv_ocl_clz(short x);
int __attribute__((overloadable))     __spirv_ocl_clz(int x);
char2 __attribute__((overloadable))   __spirv_ocl_clz(char2 x);
char3 __attribute__((overloadable))   __spirv_ocl_clz(char3 x);
char4 __attribute__((overloadable))   __spirv_ocl_clz(char4 x);
char8 __attribute__((overloadable))   __spirv_ocl_clz(char8 x);
char16 __attribute__((overloadable))  __spirv_ocl_clz(char16 x);
short2 __attribute__((overloadable))  __spirv_ocl_clz(short2 x);
short3 __attribute__((overloadable))  __spirv_ocl_clz(short3 x);
short4 __attribute__((overloadable))  __spirv_ocl_clz(short4 x);
short8 __attribute__((overloadable))  __spirv_ocl_clz(short8 x);
short16 __attribute__((overloadable)) __spirv_ocl_clz(short16 x);
int2 __attribute__((overloadable))    __spirv_ocl_clz(int2 x);
int3 __attribute__((overloadable))    __spirv_ocl_clz(int3 x);
int4 __attribute__((overloadable))    __spirv_ocl_clz(int4 x);
int8 __attribute__((overloadable))    __spirv_ocl_clz(int8 x);
int16 __attribute__((overloadable))   __spirv_ocl_clz(int16 x);
long __attribute__((overloadable))    __spirv_ocl_clz(long x);
long2 __attribute__((overloadable))   __spirv_ocl_clz(long2 x);
long3 __attribute__((overloadable))   __spirv_ocl_clz(long3 x);
long4 __attribute__((overloadable))   __spirv_ocl_clz(long4 x);
long8 __attribute__((overloadable))   __spirv_ocl_clz(long8 x);
long16 __attribute__((overloadable))  __spirv_ocl_clz(long16 x);

char __attribute__((overloadable))    __spirv_ocl_ctz(char x);
char2 __attribute__((overloadable))   __spirv_ocl_ctz(char2 x);
char3 __attribute__((overloadable))   __spirv_ocl_ctz(char3 x);
char4 __attribute__((overloadable))   __spirv_ocl_ctz(char4 x);
char8 __attribute__((overloadable))   __spirv_ocl_ctz(char8 x);
char16 __attribute__((overloadable))  __spirv_ocl_ctz(char16 x);
short __attribute__((overloadable))   __spirv_ocl_ctz(short x);
short2 __attribute__((overloadable))  __spirv_ocl_ctz(short2 x);
short3 __attribute__((overloadable))  __spirv_ocl_ctz(short3 x);
short4 __attribute__((overloadable))  __spirv_ocl_ctz(short4 x);
short8 __attribute__((overloadable))  __spirv_ocl_ctz(short8 x);
short16 __attribute__((overloadable)) __spirv_ocl_ctz(short16 x);
int __attribute__((overloadable))     __spirv_ocl_ctz(int x);
int2 __attribute__((overloadable))    __spirv_ocl_ctz(int2 x);
int3 __attribute__((overloadable))    __spirv_ocl_ctz(int3 x);
int4 __attribute__((overloadable))    __spirv_ocl_ctz(int4 x);
int8 __attribute__((overloadable))    __spirv_ocl_ctz(int8 x);
int16 __attribute__((overloadable))   __spirv_ocl_ctz(int16 x);
long __attribute__((overloadable))    __spirv_ocl_ctz(long x);
long2 __attribute__((overloadable))   __spirv_ocl_ctz(long2 x);
long3 __attribute__((overloadable))   __spirv_ocl_ctz(long3 x);
long4 __attribute__((overloadable))   __spirv_ocl_ctz(long4 x);
long8 __attribute__((overloadable))   __spirv_ocl_ctz(long8 x);
long16 __attribute__((overloadable))  __spirv_ocl_ctz(long16 x);

char __attribute__((overloadable))     __spirv_ocl_s_hadd(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_hadd(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_hadd(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_hadd(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_hadd(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_hadd(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_hadd(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_hadd(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_hadd(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_hadd(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_hadd(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_hadd(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_hadd(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_hadd(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_hadd(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_hadd(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_hadd(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_hadd(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_hadd(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_hadd(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_hadd(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_hadd(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_hadd(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_hadd(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_hadd(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_hadd(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_hadd(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_hadd(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_hadd(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_hadd(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_hadd(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_hadd(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_hadd(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_hadd(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_hadd(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_hadd(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_hadd(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_hadd(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_hadd(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_hadd(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_hadd(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_hadd(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_hadd(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_hadd(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_hadd(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_hadd(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_hadd(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_hadd(ulong16 x, ulong16 y);

char __attribute__((overloadable))   __spirv_ocl_s_mad_hi(char a, char b, char c);
char2 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(char2 a, char2 b, char2 c);
char3 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(char3 a, char3 b, char3 c);
char4 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(char4 a, char4 b, char4 c);
char8 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(char8 a, char8 b, char8 c);
char16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(char16 a, char16 b, char16 c);
uchar __attribute__((overloadable))  __spirv_ocl_u_mad_hi(uchar a, uchar b, uchar c);
uchar2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(uchar2 a, uchar2 b, uchar2 c);
uchar3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(uchar3 a, uchar3 b, uchar3 c);
uchar4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(uchar4 a, uchar4 b, uchar4 c);
uchar8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(uchar8 a, uchar8 b, uchar8 c);
uchar16 __attribute__((overloadable))
                                    __spirv_ocl_u_mad_hi(uchar16 a, uchar16 b, uchar16 c);
short __attribute__((overloadable)) __spirv_ocl_s_mad_hi(short a, short b, short c);
short2 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(short2 a, short2 b, short2 c);
short3 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(short3 a, short3 b, short3 c);
short4 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(short4 a, short4 b, short4 c);
short8 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(short8 a, short8 b, short8 c);
short16 __attribute__((overloadable))
__spirv_ocl_s_mad_hi(short16 a, short16 b, short16 c);
ushort __attribute__((overloadable)) __spirv_ocl_u_mad_hi(ushort a, ushort b, ushort c);
ushort2 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ushort2 a, ushort2 b, ushort2 c);
ushort3 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ushort3 a, ushort3 b, ushort3 c);
ushort4 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ushort4 a, ushort4 b, ushort4 c);
ushort8 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ushort8 a, ushort8 b, ushort8 c);
ushort16 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ushort16 a, ushort16 b, ushort16 c);
int __attribute__((overloadable))    __spirv_ocl_s_mad_hi(int a, int b, int c);
int2 __attribute__((overloadable))   __spirv_ocl_s_mad_hi(int2 a, int2 b, int2 c);
int3 __attribute__((overloadable))   __spirv_ocl_s_mad_hi(int3 a, int3 b, int3 c);
int4 __attribute__((overloadable))   __spirv_ocl_s_mad_hi(int4 a, int4 b, int4 c);
int8 __attribute__((overloadable))   __spirv_ocl_s_mad_hi(int8 a, int8 b, int8 c);
int16 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(int16 a, int16 b, int16 c);
uint __attribute__((overloadable))   __spirv_ocl_u_mad_hi(uint a, uint b, uint c);
uint2 __attribute__((overloadable))  __spirv_ocl_u_mad_hi(uint2 a, uint2 b, uint2 c);
uint3 __attribute__((overloadable))  __spirv_ocl_u_mad_hi(uint3 a, uint3 b, uint3 c);
uint4 __attribute__((overloadable))  __spirv_ocl_u_mad_hi(uint4 a, uint4 b, uint4 c);
uint8 __attribute__((overloadable))  __spirv_ocl_u_mad_hi(uint8 a, uint8 b, uint8 c);
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(uint16 a, uint16 b, uint16 c);
long __attribute__((overloadable))   __spirv_ocl_s_mad_hi(long a, long b, long c);
long2 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(long2 a, long2 b, long2 c);
long3 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(long3 a, long3 b, long3 c);
long4 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(long4 a, long4 b, long4 c);
long8 __attribute__((overloadable))  __spirv_ocl_s_mad_hi(long8 a, long8 b, long8 c);
long16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi(long16 a, long16 b, long16 c);
ulong __attribute__((overloadable))  __spirv_ocl_u_mad_hi(ulong a, ulong b, ulong c);
ulong2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(ulong2 a, ulong2 b, ulong2 c);
ulong3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(ulong3 a, ulong3 b, ulong3 c);
ulong4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(ulong4 a, ulong4 b, ulong4 c);
ulong8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi(ulong8 a, ulong8 b, ulong8 c);
ulong16 __attribute__((overloadable))
__spirv_ocl_u_mad_hi(ulong16 a, ulong16 b, ulong16 c);

char __attribute__((overloadable))   __spirv_ocl_s_mad_sat(char a, char b, char c);
char2 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(char2 a, char2 b, char2 c);
char3 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(char3 a, char3 b, char3 c);
char4 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(char4 a, char4 b, char4 c);
char8 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(char8 a, char8 b, char8 c);
char16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(char16 a, char16 b, char16 c);
uchar __attribute__((overloadable))  __spirv_ocl_u_mad_sat(uchar a, uchar b, uchar c);
uchar2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(uchar2 a, uchar2 b, uchar2 c);
uchar3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(uchar3 a, uchar3 b, uchar3 c);
uchar4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(uchar4 a, uchar4 b, uchar4 c);
uchar8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(uchar8 a, uchar8 b, uchar8 c);
uchar16 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(uchar16 a, uchar16 b, uchar16 c);
short __attribute__((overloadable))  __spirv_ocl_s_mad_sat(short a, short b, short c);
short2 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(short2 a, short2 b, short2 c);
short3 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(short3 a, short3 b, short3 c);
short4 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(short4 a, short4 b, short4 c);
short8 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(short8 a, short8 b, short8 c);
short16 __attribute__((overloadable))
__spirv_ocl_s_mad_sat(short16 a, short16 b, short16 c);
ushort __attribute__((overloadable)) __spirv_ocl_u_mad_sat(ushort a, ushort b, ushort c);
ushort2 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ushort2 a, ushort2 b, ushort2 c);
ushort3 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ushort3 a, ushort3 b, ushort3 c);
ushort4 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ushort4 a, ushort4 b, ushort4 c);
ushort8 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ushort8 a, ushort8 b, ushort8 c);
ushort16 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ushort16 a, ushort16 b, ushort16 c);
int __attribute__((overloadable))    __spirv_ocl_s_mad_sat(int a, int b, int c);
int2 __attribute__((overloadable))   __spirv_ocl_s_mad_sat(int2 a, int2 b, int2 c);
int3 __attribute__((overloadable))   __spirv_ocl_s_mad_sat(int3 a, int3 b, int3 c);
int4 __attribute__((overloadable))   __spirv_ocl_s_mad_sat(int4 a, int4 b, int4 c);
int8 __attribute__((overloadable))   __spirv_ocl_s_mad_sat(int8 a, int8 b, int8 c);
int16 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(int16 a, int16 b, int16 c);
uint __attribute__((overloadable))   __spirv_ocl_u_mad_sat(uint a, uint b, uint c);
uint2 __attribute__((overloadable))  __spirv_ocl_u_mad_sat(uint2 a, uint2 b, uint2 c);
uint3 __attribute__((overloadable))  __spirv_ocl_u_mad_sat(uint3 a, uint3 b, uint3 c);
uint4 __attribute__((overloadable))  __spirv_ocl_u_mad_sat(uint4 a, uint4 b, uint4 c);
uint8 __attribute__((overloadable))  __spirv_ocl_u_mad_sat(uint8 a, uint8 b, uint8 c);
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(uint16 a, uint16 b, uint16 c);
long __attribute__((overloadable))   __spirv_ocl_s_mad_sat(long a, long b, long c);
long2 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(long2 a, long2 b, long2 c);
long3 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(long3 a, long3 b, long3 c);
long4 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(long4 a, long4 b, long4 c);
long8 __attribute__((overloadable))  __spirv_ocl_s_mad_sat(long8 a, long8 b, long8 c);
long16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat(long16 a, long16 b, long16 c);
ulong __attribute__((overloadable))  __spirv_ocl_u_mad_sat(ulong a, ulong b, ulong c);
ulong2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(ulong2 a, ulong2 b, ulong2 c);
ulong3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(ulong3 a, ulong3 b, ulong3 c);
ulong4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(ulong4 a, ulong4 b, ulong4 c);
ulong8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat(ulong8 a, ulong8 b, ulong8 c);
ulong16 __attribute__((overloadable))
__spirv_ocl_u_mad_sat(ulong16 a, ulong16 b, ulong16 c);

int __attribute__((overloadable))    __spirv_ocl_s_mad24(int x, int y, int z);
int2 __attribute__((overloadable))   __spirv_ocl_s_mad24(int2 x, int2 y, int2 z);
int3 __attribute__((overloadable))   __spirv_ocl_s_mad24(int3 x, int3 y, int3 z);
int4 __attribute__((overloadable))   __spirv_ocl_s_mad24(int4 x, int4 y, int4 z);
int8 __attribute__((overloadable))   __spirv_ocl_s_mad24(int8 x, int8 y, int8 z);
int16 __attribute__((overloadable))  __spirv_ocl_s_mad24(int16 x, int16 y, int16 z);
uint __attribute__((overloadable))   __spirv_ocl_u_mad24(uint x, uint y, uint z);
uint2 __attribute__((overloadable))  __spirv_ocl_u_mad24(uint2 x, uint2 y, uint2 z);
uint3 __attribute__((overloadable))  __spirv_ocl_u_mad24(uint3 x, uint3 y, uint3 z);
uint4 __attribute__((overloadable))  __spirv_ocl_u_mad24(uint4 x, uint4 y, uint4 z);
uint8 __attribute__((overloadable))  __spirv_ocl_u_mad24(uint8 x, uint8 y, uint8 z);
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad24(uint16 x, uint16 y, uint16 z);

char __attribute__((overloadable))     __spirv_ocl_s_max(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_max(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_max(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_max(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_max(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_max(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_max(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_max(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_max(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_max(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_max(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_max(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_max(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_max(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_max(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_max(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_max(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_max(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_max(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_max(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_max(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_max(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_max(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_max(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_max(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_max(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_max(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_max(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_max(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_max(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_max(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_max(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_max(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_max(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_max(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_max(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_max(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_max(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_max(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_max(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_max(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_max(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_max(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_max(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_max(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_max(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_max(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_max(ulong16 x, ulong16 y);

char __attribute__((overloadable))     __spirv_ocl_s_mul_hi(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_mul_hi(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_mul_hi(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_mul_hi(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_mul_hi(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_mul_hi(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_mul_hi(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_mul_hi(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_mul_hi(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_mul_hi(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_mul_hi(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_mul_hi(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_mul_hi(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_mul_hi(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_mul_hi(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_mul_hi(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_mul_hi(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_mul_hi(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_mul_hi(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_mul_hi(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_mul_hi(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_mul_hi(ulong16 x, ulong16 y);

int __attribute__((overloadable))    __spirv_ocl_s_mul24(int x, int y);
int2 __attribute__((overloadable))   __spirv_ocl_s_mul24(int2 x, int2 y);
int3 __attribute__((overloadable))   __spirv_ocl_s_mul24(int3 x, int3 y);
int4 __attribute__((overloadable))   __spirv_ocl_s_mul24(int4 x, int4 y);
int8 __attribute__((overloadable))   __spirv_ocl_s_mul24(int8 x, int8 y);
int16 __attribute__((overloadable))  __spirv_ocl_s_mul24(int16 x, int16 y);
uint __attribute__((overloadable))   __spirv_ocl_u_mul24(uint x, uint y);
uint2 __attribute__((overloadable))  __spirv_ocl_u_mul24(uint2 x, uint2 y);
uint3 __attribute__((overloadable))  __spirv_ocl_u_mul24(uint3 x, uint3 y);
uint4 __attribute__((overloadable))  __spirv_ocl_u_mul24(uint4 x, uint4 y);
uint8 __attribute__((overloadable))  __spirv_ocl_u_mul24(uint8 x, uint8 y);
uint16 __attribute__((overloadable)) __spirv_ocl_u_mul24(uint16 x, uint16 y);

char __attribute__((overloadable))    __spirv_ocl_popcount(char x);
char2 __attribute__((overloadable))   __spirv_ocl_popcount(char2 x);
char3 __attribute__((overloadable))   __spirv_ocl_popcount(char3 x);
char4 __attribute__((overloadable))   __spirv_ocl_popcount(char4 x);
char8 __attribute__((overloadable))   __spirv_ocl_popcount(char8 x);
char16 __attribute__((overloadable))  __spirv_ocl_popcount(char16 x);
short __attribute__((overloadable))   __spirv_ocl_popcount(short x);
short2 __attribute__((overloadable))  __spirv_ocl_popcount(short2 x);
short3 __attribute__((overloadable))  __spirv_ocl_popcount(short3 x);
short4 __attribute__((overloadable))  __spirv_ocl_popcount(short4 x);
short8 __attribute__((overloadable))  __spirv_ocl_popcount(short8 x);
short16 __attribute__((overloadable)) __spirv_ocl_popcount(short16 x);
int __attribute__((overloadable))     __spirv_ocl_popcount(int x);
int2 __attribute__((overloadable))    __spirv_ocl_popcount(int2 x);
int3 __attribute__((overloadable))    __spirv_ocl_popcount(int3 x);
int4 __attribute__((overloadable))    __spirv_ocl_popcount(int4 x);
int8 __attribute__((overloadable))    __spirv_ocl_popcount(int8 x);
int16 __attribute__((overloadable))   __spirv_ocl_popcount(int16 x);
long __attribute__((overloadable))    __spirv_ocl_popcount(long x);
long2 __attribute__((overloadable))   __spirv_ocl_popcount(long2 x);
long3 __attribute__((overloadable))   __spirv_ocl_popcount(long3 x);
long4 __attribute__((overloadable))   __spirv_ocl_popcount(long4 x);
long8 __attribute__((overloadable))   __spirv_ocl_popcount(long8 x);
long16 __attribute__((overloadable))  __spirv_ocl_popcount(long16 x);

char __attribute__((overloadable))     __spirv_ocl_s_rhadd(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_rhadd(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_rhadd(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_rhadd(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_rhadd(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_rhadd(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_rhadd(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_rhadd(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_rhadd(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_rhadd(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_rhadd(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_rhadd(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_rhadd(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_rhadd(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_rhadd(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_rhadd(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_rhadd(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_rhadd(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_rhadd(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_rhadd(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_rhadd(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_rhadd(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_rhadd(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_rhadd(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_rhadd(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_rhadd(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_rhadd(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_rhadd(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_rhadd(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_rhadd(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_rhadd(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_rhadd(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_rhadd(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_rhadd(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_rhadd(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_rhadd(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_rhadd(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_rhadd(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_rhadd(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_rhadd(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_rhadd(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_rhadd(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_rhadd(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_rhadd(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_rhadd(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_rhadd(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_rhadd(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_rhadd(ulong16 x, ulong16 y);

char __attribute__((overloadable))    __spirv_ocl_rotate(char v, char i);
char2 __attribute__((overloadable))   __spirv_ocl_rotate(char2 v, char2 i);
char3 __attribute__((overloadable))   __spirv_ocl_rotate(char3 v, char3 i);
char4 __attribute__((overloadable))   __spirv_ocl_rotate(char4 v, char4 i);
char8 __attribute__((overloadable))   __spirv_ocl_rotate(char8 v, char8 i);
char16 __attribute__((overloadable))  __spirv_ocl_rotate(char16 v, char16 i);
short __attribute__((overloadable))   __spirv_ocl_rotate(short v, short i);
short2 __attribute__((overloadable))  __spirv_ocl_rotate(short2 v, short2 i);
short3 __attribute__((overloadable))  __spirv_ocl_rotate(short3 v, short3 i);
short4 __attribute__((overloadable))  __spirv_ocl_rotate(short4 v, short4 i);
short8 __attribute__((overloadable))  __spirv_ocl_rotate(short8 v, short8 i);
short16 __attribute__((overloadable)) __spirv_ocl_rotate(short16 v, short16 i);
int __attribute__((overloadable))     __spirv_ocl_rotate(int v, int i);
int2 __attribute__((overloadable))    __spirv_ocl_rotate(int2 v, int2 i);
int3 __attribute__((overloadable))    __spirv_ocl_rotate(int3 v, int3 i);
int4 __attribute__((overloadable))    __spirv_ocl_rotate(int4 v, int4 i);
int8 __attribute__((overloadable))    __spirv_ocl_rotate(int8 v, int8 i);
int16 __attribute__((overloadable))   __spirv_ocl_rotate(int16 v, int16 i);
long __attribute__((overloadable))    __spirv_ocl_rotate(long v, long i);
long2 __attribute__((overloadable))   __spirv_ocl_rotate(long2 v, long2 i);
long3 __attribute__((overloadable))   __spirv_ocl_rotate(long3 v, long3 i);
long4 __attribute__((overloadable))   __spirv_ocl_rotate(long4 v, long4 i);
long8 __attribute__((overloadable))   __spirv_ocl_rotate(long8 v, long8 i);
long16 __attribute__((overloadable))  __spirv_ocl_rotate(long16 v, long16 i);

char __attribute__((overloadable))     __spirv_ocl_s_sub_sat(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_sub_sat(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_sub_sat(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_sub_sat(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_sub_sat(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_sub_sat(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_sub_sat(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_sub_sat(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_sub_sat(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_sub_sat(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_sub_sat(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_sub_sat(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_sub_sat(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_sub_sat(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_sub_sat(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_sub_sat(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_sub_sat(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_sub_sat(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_sub_sat(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_sub_sat(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_sub_sat(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_sub_sat(ulong16 x, ulong16 y);

short __attribute__((overloadable))    __spirv_ocl_s_upsample(char hi, uchar lo);
short2 __attribute__((overloadable))   __spirv_ocl_s_upsample(char2 hi, uchar2 lo);
short3 __attribute__((overloadable))   __spirv_ocl_s_upsample(char3 hi, uchar3 lo);
short4 __attribute__((overloadable))   __spirv_ocl_s_upsample(char4 hi, uchar4 lo);
short8 __attribute__((overloadable))   __spirv_ocl_s_upsample(char8 hi, uchar8 lo);
short16 __attribute__((overloadable))  __spirv_ocl_s_upsample(char16 hi, uchar16 lo);
ushort __attribute__((overloadable))   __spirv_ocl_u_upsample(uchar hi, uchar lo);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_upsample(uchar2 hi, uchar2 lo);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_upsample(uchar3 hi, uchar3 lo);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_upsample(uchar4 hi, uchar4 lo);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_upsample(uchar8 hi, uchar8 lo);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_upsample(uchar16 hi, uchar16 lo);
int __attribute__((overloadable))      __spirv_ocl_s_upsample(short hi, ushort lo);
int2 __attribute__((overloadable))     __spirv_ocl_s_upsample(short2 hi, ushort2 lo);
int3 __attribute__((overloadable))     __spirv_ocl_s_upsample(short3 hi, ushort3 lo);
int4 __attribute__((overloadable))     __spirv_ocl_s_upsample(short4 hi, ushort4 lo);
int8 __attribute__((overloadable))     __spirv_ocl_s_upsample(short8 hi, ushort8 lo);
int16 __attribute__((overloadable))    __spirv_ocl_s_upsample(short16 hi, ushort16 lo);
uint __attribute__((overloadable))     __spirv_ocl_u_upsample(ushort hi, ushort lo);
uint2 __attribute__((overloadable))    __spirv_ocl_u_upsample(ushort2 hi, ushort2 lo);
uint3 __attribute__((overloadable))    __spirv_ocl_u_upsample(ushort3 hi, ushort3 lo);
uint4 __attribute__((overloadable))    __spirv_ocl_u_upsample(ushort4 hi, ushort4 lo);
uint8 __attribute__((overloadable))    __spirv_ocl_u_upsample(ushort8 hi, ushort8 lo);
uint16 __attribute__((overloadable))   __spirv_ocl_u_upsample(ushort16 hi, ushort16 lo);
long __attribute__((overloadable))     __spirv_ocl_s_upsample(int hi, uint lo);
long2 __attribute__((overloadable))    __spirv_ocl_s_upsample(int2 hi, uint2 lo);
long3 __attribute__((overloadable))    __spirv_ocl_s_upsample(int3 hi, uint3 lo);
long4 __attribute__((overloadable))    __spirv_ocl_s_upsample(int4 hi, uint4 lo);
long8 __attribute__((overloadable))    __spirv_ocl_s_upsample(int8 hi, uint8 lo);
long16 __attribute__((overloadable))   __spirv_ocl_s_upsample(int16 hi, uint16 lo);
ulong __attribute__((overloadable))    __spirv_ocl_u_upsample(uint hi, uint lo);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_upsample(uint2 hi, uint2 lo);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_upsample(uint3 hi, uint3 lo);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_upsample(uint4 hi, uint4 lo);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_upsample(uint8 hi, uint8 lo);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_upsample(uint16 hi, uint16 lo);

//
//  Math_ext
//        -acos,acosh,acospi,asin,asinh,asinpi,atan,atan2,atan2pi,atanh,atanpi,cbrt,ceil,copysign,
//       cos,cosh,cospi,divide_cr,erf,erfc,exp,exp2,exp10,expm1,fabs,fdim,floor,fma,fmax,fmin,
//       fmod,fract,frexp,hypot,ilogb,ldexp,lgamma,lgamma_r,log,log1p,log2,log10,logb,mad,maxmag,minmag,
//         modf,nan,nextafter,pow,pown,powr,remainder,remquo,rint,rootn,round,rsqrt,sin,sincos,
//       sinh,sinpi,sqrt,sqrt_cr,tan,tanh,tanpi,tgamma,trunc
//

float __attribute__((overloadable))   __spirv_ocl_acos(float x);
float2 __attribute__((overloadable))  __spirv_ocl_acos(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_acos(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_acos(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_acos(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_acos(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_acos(double x);
double2 __attribute__((overloadable))  __spirv_ocl_acos(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_acos(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_acos(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_acos(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_acos(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_acos(half x);
half2 __attribute__((overloadable))  __spirv_ocl_acos(half2 x);
half3 __attribute__((overloadable))  __spirv_ocl_acos(half3 x);
half4 __attribute__((overloadable))  __spirv_ocl_acos(half4 x);
half8 __attribute__((overloadable))  __spirv_ocl_acos(half8 x);
half16 __attribute__((overloadable)) __spirv_ocl_acos(half16 x);

float __attribute__((overloadable))   __spirv_ocl_acosh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_acosh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_acosh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_acosh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_acosh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_acosh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_acosh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_acosh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_acosh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_acosh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_acosh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_acosh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_acosh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_acosh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_acosh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_acosh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_acosh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_acosh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_acospi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_acospi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_acospi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_acospi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_acospi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_acospi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_acospi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_acospi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_acospi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_acospi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_acospi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_acospi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_acospi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_acospi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_acospi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_acospi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_acospi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_acospi(half16 x);

float __attribute__((overloadable))   __spirv_ocl_asin(float value);
float2 __attribute__((overloadable))  __spirv_ocl_asin(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_asin(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_asin(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_asin(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_asin(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_asin(double x);
double2 __attribute__((overloadable))  __spirv_ocl_asin(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_asin(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_asin(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_asin(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_asin(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_asin(half x);
half2 __attribute__((overloadable))    __spirv_ocl_asin(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_asin(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_asin(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_asin(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_asin(half16 x);

float __attribute__((overloadable))   __spirv_ocl_asinh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_asinh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_asinh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_asinh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_asinh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_asinh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_asinh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_asinh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_asinh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_asinh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_asinh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_asinh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_asinh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_asinh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_asinh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_asinh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_asinh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_asinh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_asinpi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_asinpi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_asinpi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_asinpi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_asinpi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_asinpi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_asinpi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_asinpi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_asinpi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_asinpi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_asinpi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_asinpi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_asinpi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_asinpi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_asinpi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_asinpi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_asinpi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_asinpi(half16 x);

float __attribute__((overloadable))   __spirv_ocl_atan(float value);
float2 __attribute__((overloadable))  __spirv_ocl_atan(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_atan(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_atan(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_atan(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_atan(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_atan(double x);
double2 __attribute__((overloadable))  __spirv_ocl_atan(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_atan(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_atan(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_atan(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_atan(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_atan(half x);
half2 __attribute__((overloadable))    __spirv_ocl_atan(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_atan(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_atan(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_atan(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_atan(half16 x);

float __attribute__((overloadable))   __spirv_ocl_atan2(float y, float x);
float2 __attribute__((overloadable))  __spirv_ocl_atan2(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_atan2(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_atan2(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_atan2(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_atan2(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_atan2(double y, double x);
double2 __attribute__((overloadable))  __spirv_ocl_atan2(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_atan2(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_atan2(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_atan2(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_atan2(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_atan2(half y, half x);
half2 __attribute__((overloadable))    __spirv_ocl_atan2(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_atan2(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_atan2(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_atan2(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_atan2(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_atan2pi(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_atan2pi(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_atan2pi(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_atan2pi(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_atan2pi(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_atan2pi(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_atan2pi(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_atan2pi(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_atan2pi(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_atan2pi(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_atan2pi(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_atan2pi(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_atan2pi(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_atan2pi(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_atan2pi(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_atan2pi(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_atan2pi(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_atan2pi(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_atanh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_atanh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_atanh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_atanh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_atanh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_atanh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_atanh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_atanh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_atanh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_atanh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_atanh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_atanh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_atanh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_atanh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_atanh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_atanh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_atanh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_atanh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_atanpi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_atanpi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_atanpi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_atanpi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_atanpi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_atanpi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_atanpi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_atanpi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_atanpi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_atanpi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_atanpi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_atanpi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_atanpi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_atanpi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_atanpi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_atanpi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_atanpi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_atanpi(half16 x);

float __attribute__((overloadable))   __spirv_ocl_cbrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_cbrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_cbrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_cbrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_cbrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_cbrt(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_cbrt(double x);
double2 __attribute__((overloadable))  __spirv_ocl_cbrt(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_cbrt(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_cbrt(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_cbrt(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_cbrt(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_cbrt(half x);
half2 __attribute__((overloadable))    __spirv_ocl_cbrt(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_cbrt(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_cbrt(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_cbrt(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_cbrt(half16 x);

float __attribute__((overloadable))   __spirv_ocl_ceil(float x);
float2 __attribute__((overloadable))  __spirv_ocl_ceil(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_ceil(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_ceil(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_ceil(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_ceil(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_ceil(double x);
double2 __attribute__((overloadable))  __spirv_ocl_ceil(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_ceil(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_ceil(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_ceil(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_ceil(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_ceil(half x);
half2 __attribute__((overloadable))    __spirv_ocl_ceil(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_ceil(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_ceil(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_ceil(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_ceil(half16 x);

float __attribute__((overloadable))   __spirv_ocl_copysign(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_copysign(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_copysign(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_copysign(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_copysign(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_copysign(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_copysign(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_copysign(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_copysign(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_copysign(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_copysign(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_copysign(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_copysign(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_copysign(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_copysign(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_copysign(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_copysign(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_copysign(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_cos(float x);
float2 __attribute__((overloadable))  __spirv_ocl_cos(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_cos(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_cos(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_cos(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_cos(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_cos(double x);
double2 __attribute__((overloadable))  __spirv_ocl_cos(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_cos(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_cos(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_cos(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_cos(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_cos(half x);
half2 __attribute__((overloadable))    __spirv_ocl_cos(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_cos(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_cos(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_cos(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_cos(half16 x);

float __attribute__((overloadable))   __spirv_ocl_cosh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_cosh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_cosh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_cosh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_cosh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_cosh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_cosh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_cosh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_cosh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_cosh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_cosh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_cosh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_cosh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_cosh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_cosh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_cosh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_cosh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_cosh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_cospi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_cospi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_cospi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_cospi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_cospi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_cospi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_cospi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_cospi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_cospi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_cospi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_cospi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_cospi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_cospi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_cospi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_cospi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_cospi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_cospi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_cospi(half16 x);

float   __builtin_spirv_divide_cr_f32_f32(float a, float b);
float2  __builtin_spirv_divide_cr_v2f32_v2f32(float2 a, float2 b);
float3  __builtin_spirv_divide_cr_v3f32_v3f32(float3 a, float3 b);
float4  __builtin_spirv_divide_cr_v4f32_v4f32(float4 a, float4 b);
float8  __builtin_spirv_divide_cr_v8f32_v8f32(float8 a, float8 b);
float16 __builtin_spirv_divide_cr_v16f32_v16f32(float16 a, float16 b);

#if defined(cl_khr_fp64)
double __builtin_spirv_divide_cr_f64_f64(double a, double b);
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_erf(float x);
float2 __attribute__((overloadable))  __spirv_ocl_erf(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_erf(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_erf(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_erf(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_erf(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_erf(double x);
double2 __attribute__((overloadable))  __spirv_ocl_erf(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_erf(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_erf(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_erf(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_erf(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_erf(half x);
half2 __attribute__((overloadable))    __spirv_ocl_erf(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_erf(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_erf(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_erf(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_erf(half16 x);

float __attribute__((overloadable))   __spirv_ocl_erfc(float x);
float2 __attribute__((overloadable))  __spirv_ocl_erfc(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_erfc(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_erfc(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_erfc(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_erfc(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_erfc(double x);
double2 __attribute__((overloadable))  __spirv_ocl_erfc(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_erfc(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_erfc(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_erfc(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_erfc(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_erfc(half x);
half2 __attribute__((overloadable))    __spirv_ocl_erfc(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_erfc(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_erfc(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_erfc(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_erfc(half16 x);

float __attribute__((overloadable))   __spirv_ocl_exp(float x);
float2 __attribute__((overloadable))  __spirv_ocl_exp(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_exp(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_exp(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_exp(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_exp(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_exp(double x);
double2 __attribute__((overloadable))  __spirv_ocl_exp(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_exp(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_exp(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_exp(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_exp(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_exp(half x);
half2 __attribute__((overloadable))    __spirv_ocl_exp(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_exp(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_exp(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_exp(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_exp(half16 x);

float __attribute__((overloadable))   __spirv_ocl_exp2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_exp2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_exp2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_exp2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_exp2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_exp2(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_exp2(double x);
double2 __attribute__((overloadable))  __spirv_ocl_exp2(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_exp2(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_exp2(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_exp2(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_exp2(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_exp2(half x);
half2 __attribute__((overloadable))    __spirv_ocl_exp2(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_exp2(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_exp2(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_exp2(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_exp2(half16 x);

float __attribute__((overloadable))   __spirv_ocl_exp10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_exp10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_exp10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_exp10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_exp10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_exp10(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_exp10(double x);
double2 __attribute__((overloadable))  __spirv_ocl_exp10(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_exp10(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_exp10(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_exp10(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_exp10(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_exp10(half x);
half2 __attribute__((overloadable))    __spirv_ocl_exp10(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_exp10(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_exp10(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_exp10(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_exp10(half16 x);

float __attribute__((overloadable))   __spirv_ocl_expm1(float a);
float2 __attribute__((overloadable))  __spirv_ocl_expm1(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_expm1(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_expm1(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_expm1(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_expm1(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_expm1(double x);
double2 __attribute__((overloadable))  __spirv_ocl_expm1(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_expm1(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_expm1(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_expm1(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_expm1(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_expm1(half x);
half2 __attribute__((overloadable))    __spirv_ocl_expm1(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_expm1(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_expm1(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_expm1(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_expm1(half16 x);

float __attribute__((overloadable))   __spirv_ocl_fabs(float x);
float2 __attribute__((overloadable))  __spirv_ocl_fabs(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_fabs(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_fabs(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_fabs(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_fabs(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fabs(double x);
double2 __attribute__((overloadable))  __spirv_ocl_fabs(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_fabs(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_fabs(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_fabs(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_fabs(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fabs(half x);
half2 __attribute__((overloadable))    __spirv_ocl_fabs(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_fabs(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_fabs(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_fabs(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_fabs(half16 x);

float __attribute__((overloadable))   __spirv_ocl_fdim(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_fdim(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fdim(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fdim(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fdim(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fdim(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fdim(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_fdim(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fdim(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fdim(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fdim(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fdim(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fdim(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_fdim(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fdim(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fdim(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fdim(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fdim(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_floor(float x);
float2 __attribute__((overloadable))  __spirv_ocl_floor(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_floor(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_floor(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_floor(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_floor(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_floor(double x);
double2 __attribute__((overloadable))  __spirv_ocl_floor(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_floor(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_floor(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_floor(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_floor(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_floor(half x);
half2 __attribute__((overloadable))    __spirv_ocl_floor(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_floor(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_floor(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_floor(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_floor(half16 x);

float __attribute__((overloadable))   __spirv_ocl_fma(float a, float b, float c);
float2 __attribute__((overloadable))  __spirv_ocl_fma(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable))  __spirv_ocl_fma(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable))  __spirv_ocl_fma(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable))  __spirv_ocl_fma(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable)) __spirv_ocl_fma(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_fma(double a, double b, double c);
double2 __attribute__((overloadable)) __spirv_ocl_fma(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable)) __spirv_ocl_fma(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable)) __spirv_ocl_fma(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable)) __spirv_ocl_fma(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_fma(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))    __spirv_ocl_fma(half x, half y, half z);
half2 __attribute__((overloadable))   __spirv_ocl_fma(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))   __spirv_ocl_fma(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))   __spirv_ocl_fma(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))   __spirv_ocl_fma(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable))  __spirv_ocl_fma(half16 x, half16 y, half16 z);

float __attribute__((overloadable))   __spirv_ocl_fmax(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_fmax(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fmax(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fmax(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fmax(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fmax(float16 x, float16 y);
float2 __attribute__((overloadable))  __spirv_ocl_fmax(float2 x, float y);
float3 __attribute__((overloadable))  __spirv_ocl_fmax(float3 x, float y);
float4 __attribute__((overloadable))  __spirv_ocl_fmax(float4 x, float y);
float8 __attribute__((overloadable))  __spirv_ocl_fmax(float8 x, float y);
float16 __attribute__((overloadable)) __spirv_ocl_fmax(float16 x, float y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fmax(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_fmax(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fmax(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fmax(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fmax(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fmax(double16 x, double16 y);
double2 __attribute__((overloadable))  __spirv_ocl_fmax(double2 x, double y);
double3 __attribute__((overloadable))  __spirv_ocl_fmax(double3 x, double y);
double4 __attribute__((overloadable))  __spirv_ocl_fmax(double4 x, double y);
double8 __attribute__((overloadable))  __spirv_ocl_fmax(double8 x, double y);
double16 __attribute__((overloadable)) __spirv_ocl_fmax(double16 x, double y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fmax(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_fmax(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fmax(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fmax(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fmax(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fmax(half16 x, half16 y);

half2 __attribute__((overloadable))  __spirv_ocl_fmax(half2 x, half y);
half3 __attribute__((overloadable))  __spirv_ocl_fmax(half3 x, half y);
half4 __attribute__((overloadable))  __spirv_ocl_fmax(half4 x, half y);
half8 __attribute__((overloadable))  __spirv_ocl_fmax(half8 x, half y);
half16 __attribute__((overloadable)) __spirv_ocl_fmax(half16 x, half y);

float __attribute__((overloadable))   __spirv_ocl_fmin(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_fmin(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fmin(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fmin(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fmin(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fmin(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fmin(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_fmin(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fmin(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fmin(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fmin(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fmin(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fmin(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_fmin(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fmin(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fmin(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fmin(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fmin(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_fmod(float xx, float yy);
float2 __attribute__((overloadable))  __spirv_ocl_fmod(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_fmod(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_fmod(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_fmod(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_fmod(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_fmod(double xx, double yy);
double2 __attribute__((overloadable))  __spirv_ocl_fmod(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_fmod(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_fmod(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_fmod(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_fmod(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_fmod(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_fmod(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_fmod(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_fmod(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_fmod(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_fmod(half16 x, half16 y);

/* Helper function for fmod */
float  __builtin_spirv_fast_fmod_f32_f32(float xx, float yy);
float2 __builtin_spirv_fast_fmod_v2f32_v2f32(float2 xx, float2 yy);
float3 __builtin_spirv_fast_fmod_v3f32_v3f32(float3 xx, float3 yy);
float4 __builtin_spirv_fast_fmod_v4f32_v4f32(float4 xx, float4 yy);
half   __builtin_spirv_fast_fmod_f16_f16(half xx, half yy);
half2  __builtin_spirv_fast_fmod_v2f16_v2f16(half2 xx, half2 yy);
half3  __builtin_spirv_fast_fmod_v3f16_v3f16(half3 xx, half3 yy);
half4  __builtin_spirv_fast_fmod_v4f16_v4f16(half4 xx, half4 yy);
#if defined(cl_khr_fp64)
double  __builtin_spirv_fast_fmod_f64_f64(double xx, double yy);
double2 __builtin_spirv_fast_fmod_v2f64_v2f64(double2 xx, double2 yy);
double3 __builtin_spirv_fast_fmod_v3f64_v3f64(double3 xx, double3 yy);
double4 __builtin_spirv_fast_fmod_v4f64_v4f64(double4 xx, double4 yy);
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))  __spirv_ocl_fract(float x, __global float* iptr);
float2 __attribute__((overloadable)) __spirv_ocl_fract(float2 x, __global float2* iptr);
float3 __attribute__((overloadable)) __spirv_ocl_fract(float3 x, __global float3* iptr);
float4 __attribute__((overloadable)) __spirv_ocl_fract(float4 x, __global float4* iptr);
float8 __attribute__((overloadable)) __spirv_ocl_fract(float8 x, __global float8* iptr);
float16 __attribute__((overloadable))
                                     __spirv_ocl_fract(float16 x, __global float16* iptr);
float __attribute__((overloadable))  __spirv_ocl_fract(float x, __private float* iptr);
float2 __attribute__((overloadable)) __spirv_ocl_fract(float2 x, __private float2* iptr);
float3 __attribute__((overloadable)) __spirv_ocl_fract(float3 x, __private float3* iptr);
float4 __attribute__((overloadable)) __spirv_ocl_fract(float4 x, __private float4* iptr);
float8 __attribute__((overloadable)) __spirv_ocl_fract(float8 x, __private float8* iptr);
float16 __attribute__((overloadable))
                                    __spirv_ocl_fract(float16 x, __private float16* iptr);
float __attribute__((overloadable)) __spirv_ocl_fract(float x, __local float* iptr);
float2 __attribute__((overloadable))  __spirv_ocl_fract(float2 x, __local float2* iptr);
float3 __attribute__((overloadable))  __spirv_ocl_fract(float3 x, __local float3* iptr);
float4 __attribute__((overloadable))  __spirv_ocl_fract(float4 x, __local float4* iptr);
float8 __attribute__((overloadable))  __spirv_ocl_fract(float8 x, __local float8* iptr);
float16 __attribute__((overloadable)) __spirv_ocl_fract(float16 x, __local float16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))  __spirv_ocl_fract(float x, __generic float* iptr);
float2 __attribute__((overloadable)) __spirv_ocl_fract(float2 x, __generic float2* iptr);
float3 __attribute__((overloadable)) __spirv_ocl_fract(float3 x, __generic float3* iptr);
float4 __attribute__((overloadable)) __spirv_ocl_fract(float4 x, __generic float4* iptr);
float8 __attribute__((overloadable)) __spirv_ocl_fract(float8 x, __generic float8* iptr);
float16 __attribute__((overloadable))
__spirv_ocl_fract(float16 x, __generic float16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __attribute__((overloadable))   __spirv_ocl_fract(half x, __global half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_fract(half2 x, __global half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_fract(half3 x, __global half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_fract(half4 x, __global half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_fract(half8 x, __global half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_fract(half16 x, __global half16* iptr);
half __attribute__((overloadable))   __spirv_ocl_fract(half x, __private half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_fract(half2 x, __private half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_fract(half3 x, __private half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_fract(half4 x, __private half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_fract(half8 x, __private half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_fract(half16 x, __private half16* iptr);
half __attribute__((overloadable))   __spirv_ocl_fract(half x, __local half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_fract(half2 x, __local half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_fract(half3 x, __local half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_fract(half4 x, __local half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_fract(half8 x, __local half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_fract(half16 x, __local half16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))   __spirv_ocl_fract(half x, __generic half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_fract(half2 x, __generic half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_fract(half3 x, __generic half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_fract(half4 x, __generic half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_fract(half8 x, __generic half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_fract(half16 x, __generic half16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ocl_fract(double x, __global double* iptr);
double2 __attribute__((overloadable))
__spirv_ocl_fract(double2 x, __global double2* iptr);
double3 __attribute__((overloadable))
__spirv_ocl_fract(double3 x, __global double3* iptr);
double4 __attribute__((overloadable))
__spirv_ocl_fract(double4 x, __global double4* iptr);
double8 __attribute__((overloadable))
__spirv_ocl_fract(double8 x, __global double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_fract(double16 x, __global double16* iptr);
double __attribute__((overloadable)) __spirv_ocl_fract(double x, __private double* iptr);
double2 __attribute__((overloadable))
__spirv_ocl_fract(double2 x, __private double2* iptr);
double3 __attribute__((overloadable))
__spirv_ocl_fract(double3 x, __private double3* iptr);
double4 __attribute__((overloadable))
__spirv_ocl_fract(double4 x, __private double4* iptr);
double8 __attribute__((overloadable))
__spirv_ocl_fract(double8 x, __private double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_fract(double16 x, __private double16* iptr);
double __attribute__((overloadable))  __spirv_ocl_fract(double x, __local double* iptr);
double2 __attribute__((overloadable)) __spirv_ocl_fract(double2 x, __local double2* iptr);
double3 __attribute__((overloadable)) __spirv_ocl_fract(double3 x, __local double3* iptr);
double4 __attribute__((overloadable)) __spirv_ocl_fract(double4 x, __local double4* iptr);
double8 __attribute__((overloadable)) __spirv_ocl_fract(double8 x, __local double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_fract(double16 x, __local double16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_ocl_fract(double x, __generic double* iptr);
double2 __attribute__((overloadable))
__spirv_ocl_fract(double2 x, __generic double2* iptr);
double3 __attribute__((overloadable))
__spirv_ocl_fract(double3 x, __generic double3* iptr);
double4 __attribute__((overloadable))
__spirv_ocl_fract(double4 x, __generic double4* iptr);
double8 __attribute__((overloadable))
__spirv_ocl_fract(double8 x, __generic double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_fract(double16 x, __generic double16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_frexp(float x, __global int* exp);
float2 __attribute__((overloadable))  __spirv_ocl_frexp(float2 x, __global int2* exp);
float3 __attribute__((overloadable))  __spirv_ocl_frexp(float3 x, __global int3* exp);
float4 __attribute__((overloadable))  __spirv_ocl_frexp(float4 x, __global int4* exp);
float8 __attribute__((overloadable))  __spirv_ocl_frexp(float8 x, __global int8* exp);
float16 __attribute__((overloadable)) __spirv_ocl_frexp(float16 x, __global int16* exp);
float __attribute__((overloadable))   __spirv_ocl_frexp(float x, __private int* exp);
float2 __attribute__((overloadable))  __spirv_ocl_frexp(float2 x, __private int2* exp);
float3 __attribute__((overloadable))  __spirv_ocl_frexp(float3 x, __private int3* exp);
float4 __attribute__((overloadable))  __spirv_ocl_frexp(float4 x, __private int4* exp);
float8 __attribute__((overloadable))  __spirv_ocl_frexp(float8 x, __private int8* exp);
float16 __attribute__((overloadable)) __spirv_ocl_frexp(float16 x, __private int16* exp);
float __attribute__((overloadable))   __spirv_ocl_frexp(float x, __local int* exp);
float2 __attribute__((overloadable))  __spirv_ocl_frexp(float2 x, __local int2* exp);
float3 __attribute__((overloadable))  __spirv_ocl_frexp(float3 x, __local int3* exp);
float4 __attribute__((overloadable))  __spirv_ocl_frexp(float4 x, __local int4* exp);
float8 __attribute__((overloadable))  __spirv_ocl_frexp(float8 x, __local int8* exp);
float16 __attribute__((overloadable)) __spirv_ocl_frexp(float16 x, __local int16* exp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))   __spirv_ocl_frexp(float x, __generic int* exp);
float2 __attribute__((overloadable))  __spirv_ocl_frexp(float2 x, __generic int2* exp);
float3 __attribute__((overloadable))  __spirv_ocl_frexp(float3 x, __generic int3* exp);
float4 __attribute__((overloadable))  __spirv_ocl_frexp(float4 x, __generic int4* exp);
float8 __attribute__((overloadable))  __spirv_ocl_frexp(float8 x, __generic int8* exp);
float16 __attribute__((overloadable)) __spirv_ocl_frexp(float16 x, __generic int16* exp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __attribute__((overloadable))   __spirv_ocl_frexp(half x, __global int* exp);
half2 __attribute__((overloadable))  __spirv_ocl_frexp(half2 x, __global int2* exp);
half3 __attribute__((overloadable))  __spirv_ocl_frexp(half3 x, __global int3* exp);
half4 __attribute__((overloadable))  __spirv_ocl_frexp(half4 x, __global int4* exp);
half8 __attribute__((overloadable))  __spirv_ocl_frexp(half8 x, __global int8* exp);
half16 __attribute__((overloadable)) __spirv_ocl_frexp(half16 x, __global int16* exp);
half __attribute__((overloadable))   __spirv_ocl_frexp(half x, __private int* exp);
half2 __attribute__((overloadable))  __spirv_ocl_frexp(half2 x, __private int2* exp);
half3 __attribute__((overloadable))  __spirv_ocl_frexp(half3 x, __private int3* exp);
half4 __attribute__((overloadable))  __spirv_ocl_frexp(half4 x, __private int4* exp);
half8 __attribute__((overloadable))  __spirv_ocl_frexp(half8 x, __private int8* exp);
half16 __attribute__((overloadable)) __spirv_ocl_frexp(half16 x, __private int16* exp);
half __attribute__((overloadable))   __spirv_ocl_frexp(half x, __local int* exp);
half2 __attribute__((overloadable))  __spirv_ocl_frexp(half2 x, __local int2* exp);
half3 __attribute__((overloadable))  __spirv_ocl_frexp(half3 x, __local int3* exp);
half4 __attribute__((overloadable))  __spirv_ocl_frexp(half4 x, __local int4* exp);
half8 __attribute__((overloadable))  __spirv_ocl_frexp(half8 x, __local int8* exp);
half16 __attribute__((overloadable)) __spirv_ocl_frexp(half16 x, __local int16* exp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))   __spirv_ocl_frexp(half x, __generic int* exp);
half2 __attribute__((overloadable))  __spirv_ocl_frexp(half2 x, __generic int2* exp);
half3 __attribute__((overloadable))  __spirv_ocl_frexp(half3 x, __generic int3* exp);
half4 __attribute__((overloadable))  __spirv_ocl_frexp(half4 x, __generic int4* exp);
half8 __attribute__((overloadable))  __spirv_ocl_frexp(half8 x, __generic int8* exp);
half16 __attribute__((overloadable)) __spirv_ocl_frexp(half16 x, __generic int16* exp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_frexp(double x, __global int* exp);
double2 __attribute__((overloadable))  __spirv_ocl_frexp(double2 x, __global int2* exp);
double3 __attribute__((overloadable))  __spirv_ocl_frexp(double3 x, __global int3* exp);
double4 __attribute__((overloadable))  __spirv_ocl_frexp(double4 x, __global int4* exp);
double8 __attribute__((overloadable))  __spirv_ocl_frexp(double8 x, __global int8* exp);
double16 __attribute__((overloadable)) __spirv_ocl_frexp(double16 x, __global int16* exp);
double __attribute__((overloadable))   __spirv_ocl_frexp(double x, __private int* exp);
double2 __attribute__((overloadable))  __spirv_ocl_frexp(double2 x, __private int2* exp);
double3 __attribute__((overloadable))  __spirv_ocl_frexp(double3 x, __private int3* exp);
double4 __attribute__((overloadable))  __spirv_ocl_frexp(double4 x, __private int4* exp);
double8 __attribute__((overloadable))  __spirv_ocl_frexp(double8 x, __private int8* exp);
double16 __attribute__((overloadable))
                                      __spirv_ocl_frexp(double16 x, __private int16* exp);
double __attribute__((overloadable))  __spirv_ocl_frexp(double x, __local int* exp);
double2 __attribute__((overloadable)) __spirv_ocl_frexp(double2 x, __local int2* exp);
double3 __attribute__((overloadable)) __spirv_ocl_frexp(double3 x, __local int3* exp);
double4 __attribute__((overloadable)) __spirv_ocl_frexp(double4 x, __local int4* exp);
double8 __attribute__((overloadable)) __spirv_ocl_frexp(double8 x, __local int8* exp);
double16 __attribute__((overloadable)) __spirv_ocl_frexp(double16 x, __local int16* exp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable))  __spirv_ocl_frexp(double x, __generic int* exp);
double2 __attribute__((overloadable)) __spirv_ocl_frexp(double2 x, __generic int2* exp);
double3 __attribute__((overloadable)) __spirv_ocl_frexp(double3 x, __generic int3* exp);
double4 __attribute__((overloadable)) __spirv_ocl_frexp(double4 x, __generic int4* exp);
double8 __attribute__((overloadable)) __spirv_ocl_frexp(double8 x, __generic int8* exp);
double16 __attribute__((overloadable))
__spirv_ocl_frexp(double16 x, __generic int16* exp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_hypot(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_hypot(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_hypot(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_hypot(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_hypot(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_hypot(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_hypot(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_hypot(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_hypot(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_hypot(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_hypot(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_hypot(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_hypot(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_hypot(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_hypot(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_hypot(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_hypot(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_hypot(half16 x, half16 y);

int __attribute__((overloadable))   __spirv_ocl_ilogb(float x);
int __attribute__((overloadable))   __spirv_ocl_ilogb(float x);
int2 __attribute__((overloadable))  __spirv_ocl_ilogb(float2 x);
int3 __attribute__((overloadable))  __spirv_ocl_ilogb(float3 x);
int4 __attribute__((overloadable))  __spirv_ocl_ilogb(float4 x);
int8 __attribute__((overloadable))  __spirv_ocl_ilogb(float8 x);
int16 __attribute__((overloadable)) __spirv_ocl_ilogb(float16 x);
#if defined(cl_khr_fp64)
int __attribute__((overloadable))   __spirv_ocl_ilogb(double x);
int2 __attribute__((overloadable))  __spirv_ocl_ilogb(double2 x);
int3 __attribute__((overloadable))  __spirv_ocl_ilogb(double3 x);
int4 __attribute__((overloadable))  __spirv_ocl_ilogb(double4 x);
int8 __attribute__((overloadable))  __spirv_ocl_ilogb(double8 x);
int16 __attribute__((overloadable)) __spirv_ocl_ilogb(double16 x);
#endif // defined(cl_khr_fp64)
int __attribute__((overloadable))   __spirv_ocl_ilogb(half x);
int2 __attribute__((overloadable))  __spirv_ocl_ilogb(half2 x);
int3 __attribute__((overloadable))  __spirv_ocl_ilogb(half3 x);
int4 __attribute__((overloadable))  __spirv_ocl_ilogb(half4 x);
int8 __attribute__((overloadable))  __spirv_ocl_ilogb(half8 x);
int16 __attribute__((overloadable)) __spirv_ocl_ilogb(half16 x);

float __attribute__((overloadable))   __spirv_ocl_ldexp(float x, int n);
float2 __attribute__((overloadable))  __spirv_ocl_ldexp(float2 x, int2 y);
float3 __attribute__((overloadable))  __spirv_ocl_ldexp(float3 x, int3 y);
float4 __attribute__((overloadable))  __spirv_ocl_ldexp(float4 x, int4 y);
float8 __attribute__((overloadable))  __spirv_ocl_ldexp(float8 x, int8 y);
float16 __attribute__((overloadable)) __spirv_ocl_ldexp(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_ldexp(double x, int n);
double2 __attribute__((overloadable))  __spirv_ocl_ldexp(double2 x, int2 y);
double3 __attribute__((overloadable))  __spirv_ocl_ldexp(double3 x, int3 y);
double4 __attribute__((overloadable))  __spirv_ocl_ldexp(double4 x, int4 y);
double8 __attribute__((overloadable))  __spirv_ocl_ldexp(double8 x, int8 y);
double16 __attribute__((overloadable)) __spirv_ocl_ldexp(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_ldexp(half x, int n);
half2 __attribute__((overloadable))    __spirv_ocl_ldexp(half2 x, int2 y);
half3 __attribute__((overloadable))    __spirv_ocl_ldexp(half3 x, int3 y);
half4 __attribute__((overloadable))    __spirv_ocl_ldexp(half4 x, int4 y);
half8 __attribute__((overloadable))    __spirv_ocl_ldexp(half8 x, int8 y);
half16 __attribute__((overloadable))   __spirv_ocl_ldexp(half16 x, int16 y);

float __attribute__((overloadable))   __spirv_ocl_lgamma(float x);
float2 __attribute__((overloadable))  __spirv_ocl_lgamma(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_lgamma(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_lgamma(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_lgamma(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_lgamma(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_lgamma(double x);
double2 __attribute__((overloadable))  __spirv_ocl_lgamma(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_lgamma(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_lgamma(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_lgamma(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_lgamma(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_lgamma(half x);
half2 __attribute__((overloadable))    __spirv_ocl_lgamma(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_lgamma(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_lgamma(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_lgamma(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_lgamma(half16 x);

float __attribute__((overloadable))  __spirv_ocl_lgamma_r(float x, __global int* signp);
float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float2 x, __global int2* signp);
float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float3 x, __global int3* signp);
float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float4 x, __global int4* signp);
float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float8 x, __global int8* signp);
float16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float16 x, __global int16* signp);
float __attribute__((overloadable))  __spirv_ocl_lgamma_r(float x, __local int* signp);
float2 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float2 x, __local int2* signp);
float3 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float3 x, __local int3* signp);
float4 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float4 x, __local int4* signp);
float8 __attribute__((overloadable)) __spirv_ocl_lgamma_r(float8 x, __local int8* signp);
float16 __attribute__((overloadable))
                                    __spirv_ocl_lgamma_r(float16 x, __local int16* signp);
float __attribute__((overloadable)) __spirv_ocl_lgamma_r(float x, __private int* signp);
float2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float2 x, __private int2* signp);
float3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float3 x, __private int3* signp);
float4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float4 x, __private int4* signp);
float8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float8 x, __private int8* signp);
float16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float16 x, __private int16* signp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) __spirv_ocl_lgamma_r(float x, __generic int* signp);
float2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float2 x, __generic int2* signp);
float3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float3 x, __generic int3* signp);
float4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float4 x, __generic int4* signp);
float8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float8 x, __generic int8* signp);
float16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(float16 x, __generic int16* signp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __attribute__((overloadable))  __spirv_ocl_lgamma_r(half x, __global int* signp);
half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half2 x, __global int2* signp);
half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half3 x, __global int3* signp);
half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half4 x, __global int4* signp);
half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half8 x, __global int8* signp);
half16 __attribute__((overloadable))
                                    __spirv_ocl_lgamma_r(half16 x, __global int16* signp);
half __attribute__((overloadable))  __spirv_ocl_lgamma_r(half x, __private int* signp);
half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half2 x, __private int2* signp);
half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half3 x, __private int3* signp);
half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half4 x, __private int4* signp);
half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half8 x, __private int8* signp);
half16 __attribute__((overloadable))
                                   __spirv_ocl_lgamma_r(half16 x, __private int16* signp);
half __attribute__((overloadable)) __spirv_ocl_lgamma_r(half x, __local int* signp);
half2 __attribute__((overloadable))  __spirv_ocl_lgamma_r(half2 x, __local int2* signp);
half3 __attribute__((overloadable))  __spirv_ocl_lgamma_r(half3 x, __local int3* signp);
half4 __attribute__((overloadable))  __spirv_ocl_lgamma_r(half4 x, __local int4* signp);
half8 __attribute__((overloadable))  __spirv_ocl_lgamma_r(half8 x, __local int8* signp);
half16 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half16 x, __local int16* signp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))  __spirv_ocl_lgamma_r(half x, __generic int* signp);
half2 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half2 x, __generic int2* signp);
half3 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half3 x, __generic int3* signp);
half4 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half4 x, __generic int4* signp);
half8 __attribute__((overloadable)) __spirv_ocl_lgamma_r(half8 x, __generic int8* signp);
half16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(half16 x, __generic int16* signp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ocl_lgamma_r(double x, __global int* signp);
double2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double2 x, __global int2* signp);
double3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double3 x, __global int3* signp);
double4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double4 x, __global int4* signp);
double8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double8 x, __global int8* signp);
double16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double16 x, __global int16* signp);
double __attribute__((overloadable)) __spirv_ocl_lgamma_r(double x, __private int* signp);
double2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double2 x, __private int2* signp);
double3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double3 x, __private int3* signp);
double4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double4 x, __private int4* signp);
double8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double8 x, __private int8* signp);
double16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double16 x, __private int16* signp);
double __attribute__((overloadable)) __spirv_ocl_lgamma_r(double x, __local int* signp);
double2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double2 x, __local int2* signp);
double3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double3 x, __local int3* signp);
double4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double4 x, __local int4* signp);
double8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double8 x, __local int8* signp);
double16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double16 x, __local int16* signp);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_ocl_lgamma_r(double x, __generic int* signp);
double2 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double2 x, __generic int2* signp);
double3 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double3 x, __generic int3* signp);
double4 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double4 x, __generic int4* signp);
double8 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double8 x, __generic int8* signp);
double16 __attribute__((overloadable))
__spirv_ocl_lgamma_r(double16 x, __generic int16* signp);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_log(float x);
float2 __attribute__((overloadable))  __spirv_ocl_log(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_log(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_log(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_log(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_log(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_log(double x);
double2 __attribute__((overloadable))  __spirv_ocl_log(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_log(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_log(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_log(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_log(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_log(half x);
half2 __attribute__((overloadable))    __spirv_ocl_log(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_log(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_log(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_log(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_log(half16 x);

float __attribute__((overloadable))   __spirv_ocl_log1p(float a);
float2 __attribute__((overloadable))  __spirv_ocl_log1p(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_log1p(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_log1p(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_log1p(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_log1p(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_log1p(double a);
double2 __attribute__((overloadable))  __spirv_ocl_log1p(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_log1p(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_log1p(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_log1p(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_log1p(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_log1p(half x);
half2 __attribute__((overloadable))    __spirv_ocl_log1p(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_log1p(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_log1p(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_log1p(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_log1p(half16 x);

float __attribute__((overloadable))   __spirv_ocl_log2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_log2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_log2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_log2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_log2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_log2(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_log2(double x);
double2 __attribute__((overloadable))  __spirv_ocl_log2(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_log2(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_log2(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_log2(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_log2(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_log2(half x);
half2 __attribute__((overloadable))    __spirv_ocl_log2(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_log2(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_log2(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_log2(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_log2(half16 x);

float __attribute__((overloadable))   __spirv_ocl_log10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_log10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_log10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_log10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_log10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_log10(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_log10(double x);
double2 __attribute__((overloadable))  __spirv_ocl_log10(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_log10(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_log10(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_log10(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_log10(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_log10(half x);
half2 __attribute__((overloadable))    __spirv_ocl_log10(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_log10(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_log10(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_log10(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_log10(half16 x);

float __attribute__((overloadable))   __spirv_ocl_logb(float x);
float2 __attribute__((overloadable))  __spirv_ocl_logb(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_logb(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_logb(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_logb(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_logb(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_logb(double x);
double2 __attribute__((overloadable))  __spirv_ocl_logb(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_logb(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_logb(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_logb(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_logb(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_logb(half x);
half2 __attribute__((overloadable))    __spirv_ocl_logb(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_logb(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_logb(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_logb(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_logb(half16 x);

float __attribute__((overloadable))   __spirv_ocl_mad(float a, float b, float c);
float2 __attribute__((overloadable))  __spirv_ocl_mad(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable))  __spirv_ocl_mad(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable))  __spirv_ocl_mad(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable))  __spirv_ocl_mad(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable)) __spirv_ocl_mad(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_mad(double a, double b, double c);
double2 __attribute__((overloadable)) __spirv_ocl_mad(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable)) __spirv_ocl_mad(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable)) __spirv_ocl_mad(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable)) __spirv_ocl_mad(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_mad(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))    __spirv_ocl_mad(half x, half y, half z);
half2 __attribute__((overloadable))   __spirv_ocl_mad(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))   __spirv_ocl_mad(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))   __spirv_ocl_mad(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))   __spirv_ocl_mad(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable))  __spirv_ocl_mad(half16 x, half16 y, half16 z);

float __attribute__((overloadable))   __spirv_ocl_maxmag(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_maxmag(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_maxmag(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_maxmag(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_maxmag(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_maxmag(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_maxmag(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_maxmag(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_maxmag(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_maxmag(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_maxmag(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_maxmag(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_maxmag(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_maxmag(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_maxmag(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_maxmag(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_maxmag(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_maxmag(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_minmag(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_minmag(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_minmag(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_minmag(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_minmag(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_minmag(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_minmag(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_minmag(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_minmag(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_minmag(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_minmag(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_minmag(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_minmag(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_minmag(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_minmag(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_minmag(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_minmag(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_minmag(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_modf(float x, __global float* iptr);
float2 __attribute__((overloadable))  __spirv_ocl_modf(float2 x, __global float2* iptr);
float3 __attribute__((overloadable))  __spirv_ocl_modf(float3 x, __global float3* iptr);
float4 __attribute__((overloadable))  __spirv_ocl_modf(float4 x, __global float4* iptr);
float8 __attribute__((overloadable))  __spirv_ocl_modf(float8 x, __global float8* iptr);
float16 __attribute__((overloadable)) __spirv_ocl_modf(float16 x, __global float16* iptr);
float __attribute__((overloadable))   __spirv_ocl_modf(float x, __private float* iptr);
float2 __attribute__((overloadable))  __spirv_ocl_modf(float2 x, __private float2* iptr);
float3 __attribute__((overloadable))  __spirv_ocl_modf(float3 x, __private float3* iptr);
float4 __attribute__((overloadable))  __spirv_ocl_modf(float4 x, __private float4* iptr);
float8 __attribute__((overloadable))  __spirv_ocl_modf(float8 x, __private float8* iptr);
float16 __attribute__((overloadable))
                                     __spirv_ocl_modf(float16 x, __private float16* iptr);
float __attribute__((overloadable))  __spirv_ocl_modf(float x, __local float* iptr);
float2 __attribute__((overloadable)) __spirv_ocl_modf(float2 x, __local float2* iptr);
float3 __attribute__((overloadable)) __spirv_ocl_modf(float3 x, __local float3* iptr);
float4 __attribute__((overloadable)) __spirv_ocl_modf(float4 x, __local float4* iptr);
float8 __attribute__((overloadable)) __spirv_ocl_modf(float8 x, __local float8* iptr);
float16 __attribute__((overloadable)) __spirv_ocl_modf(float16 x, __local float16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))  __spirv_ocl_modf(float x, __generic float* iptr);
float2 __attribute__((overloadable)) __spirv_ocl_modf(float2 x, __generic float2* iptr);
float3 __attribute__((overloadable)) __spirv_ocl_modf(float3 x, __generic float3* iptr);
float4 __attribute__((overloadable)) __spirv_ocl_modf(float4 x, __generic float4* iptr);
float8 __attribute__((overloadable)) __spirv_ocl_modf(float8 x, __generic float8* iptr);
float16 __attribute__((overloadable))
__spirv_ocl_modf(float16 x, __generic float16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __attribute__((overloadable))   __spirv_ocl_modf(half x, __global half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_modf(half2 x, __global half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_modf(half3 x, __global half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_modf(half4 x, __global half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_modf(half8 x, __global half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_modf(half16 x, __global half16* iptr);
half __attribute__((overloadable))   __spirv_ocl_modf(half x, __private half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_modf(half2 x, __private half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_modf(half3 x, __private half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_modf(half4 x, __private half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_modf(half8 x, __private half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_modf(half16 x, __private half16* iptr);
half __attribute__((overloadable))   __spirv_ocl_modf(half x, __local half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_modf(half2 x, __local half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_modf(half3 x, __local half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_modf(half4 x, __local half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_modf(half8 x, __local half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_modf(half16 x, __local half16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))   __spirv_ocl_modf(half x, __generic half* iptr);
half2 __attribute__((overloadable))  __spirv_ocl_modf(half2 x, __generic half2* iptr);
half3 __attribute__((overloadable))  __spirv_ocl_modf(half3 x, __generic half3* iptr);
half4 __attribute__((overloadable))  __spirv_ocl_modf(half4 x, __generic half4* iptr);
half8 __attribute__((overloadable))  __spirv_ocl_modf(half8 x, __generic half8* iptr);
half16 __attribute__((overloadable)) __spirv_ocl_modf(half16 x, __generic half16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_modf(double x, __global double* iptr);
double2 __attribute__((overloadable)) __spirv_ocl_modf(double2 x, __global double2* iptr);
double3 __attribute__((overloadable)) __spirv_ocl_modf(double3 x, __global double3* iptr);
double4 __attribute__((overloadable)) __spirv_ocl_modf(double4 x, __global double4* iptr);
double8 __attribute__((overloadable)) __spirv_ocl_modf(double8 x, __global double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_modf(double16 x, __global double16* iptr);
double __attribute__((overloadable)) __spirv_ocl_modf(double x, __private double* iptr);
double2 __attribute__((overloadable))
__spirv_ocl_modf(double2 x, __private double2* iptr);
double3 __attribute__((overloadable))
__spirv_ocl_modf(double3 x, __private double3* iptr);
double4 __attribute__((overloadable))
__spirv_ocl_modf(double4 x, __private double4* iptr);
double8 __attribute__((overloadable))
__spirv_ocl_modf(double8 x, __private double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_modf(double16 x, __private double16* iptr);
double __attribute__((overloadable))  __spirv_ocl_modf(double x, __local double* iptr);
double2 __attribute__((overloadable)) __spirv_ocl_modf(double2 x, __local double2* iptr);
double3 __attribute__((overloadable)) __spirv_ocl_modf(double3 x, __local double3* iptr);
double4 __attribute__((overloadable)) __spirv_ocl_modf(double4 x, __local double4* iptr);
double8 __attribute__((overloadable)) __spirv_ocl_modf(double8 x, __local double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_modf(double16 x, __local double16* iptr);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_ocl_modf(double x, __generic double* iptr);
double2 __attribute__((overloadable))
__spirv_ocl_modf(double2 x, __generic double2* iptr);
double3 __attribute__((overloadable))
__spirv_ocl_modf(double3 x, __generic double3* iptr);
double4 __attribute__((overloadable))
__spirv_ocl_modf(double4 x, __generic double4* iptr);
double8 __attribute__((overloadable))
__spirv_ocl_modf(double8 x, __generic double8* iptr);
double16 __attribute__((overloadable))
__spirv_ocl_modf(double16 x, __generic double16* iptr);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_nan(int nancode);
float2 __attribute__((overloadable))  __spirv_ocl_nan(int2 x);
float3 __attribute__((overloadable))  __spirv_ocl_nan(int3 x);
float4 __attribute__((overloadable))  __spirv_ocl_nan(int4 x);
float8 __attribute__((overloadable))  __spirv_ocl_nan(int8 x);
float16 __attribute__((overloadable)) __spirv_ocl_nan(int16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_nan(long nancode);
double2 __attribute__((overloadable))  __spirv_ocl_nan(long2 x);
double3 __attribute__((overloadable))  __spirv_ocl_nan(long3 x);
double4 __attribute__((overloadable))  __spirv_ocl_nan(long4 x);
double8 __attribute__((overloadable))  __spirv_ocl_nan(long8 x);
double16 __attribute__((overloadable)) __spirv_ocl_nan(long16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_nan(short nancode);
half2 __attribute__((overloadable))  __spirv_ocl_nan(short2 x);
half3 __attribute__((overloadable))  __spirv_ocl_nan(short3 x);
half4 __attribute__((overloadable))  __spirv_ocl_nan(short4 x);
half8 __attribute__((overloadable))  __spirv_ocl_nan(short8 x);
half16 __attribute__((overloadable)) __spirv_ocl_nan(short16 x);

float __attribute__((overloadable))   __spirv_ocl_nextafter(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_nextafter(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_nextafter(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_nextafter(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_nextafter(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_nextafter(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_nextafter(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_nextafter(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_nextafter(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_nextafter(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_nextafter(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_nextafter(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_nextafter(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_nextafter(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_nextafter(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_nextafter(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_nextafter(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_nextafter(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_pow(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_pow(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_pow(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_pow(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_pow(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_pow(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_pow(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_pow(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_pow(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_pow(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_pow(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_pow(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_pow(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_pow(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_pow(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_pow(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_pow(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_pow(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_pown(float x, int y);
float2 __attribute__((overloadable))  __spirv_ocl_pown(float2 x, int2 y);
float3 __attribute__((overloadable))  __spirv_ocl_pown(float3 x, int3 y);
float4 __attribute__((overloadable))  __spirv_ocl_pown(float4 x, int4 y);
float8 __attribute__((overloadable))  __spirv_ocl_pown(float8 x, int8 y);
float16 __attribute__((overloadable)) __spirv_ocl_pown(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_pown(double x, int y);
double2 __attribute__((overloadable))  __spirv_ocl_pown(double2 x, int2 y);
double3 __attribute__((overloadable))  __spirv_ocl_pown(double3 x, int3 y);
double4 __attribute__((overloadable))  __spirv_ocl_pown(double4 x, int4 y);
double8 __attribute__((overloadable))  __spirv_ocl_pown(double8 x, int8 y);
double16 __attribute__((overloadable)) __spirv_ocl_pown(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_pown(half x, int y);
half2 __attribute__((overloadable))    __spirv_ocl_pown(half2 x, int2 y);
half3 __attribute__((overloadable))    __spirv_ocl_pown(half3 x, int3 y);
half4 __attribute__((overloadable))    __spirv_ocl_pown(half4 x, int4 y);
half8 __attribute__((overloadable))    __spirv_ocl_pown(half8 x, int8 y);
half16 __attribute__((overloadable))   __spirv_ocl_pown(half16 x, int16 y);

float __attribute__((overloadable))   __spirv_ocl_powr(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_powr(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_powr(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_powr(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_powr(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_powr(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_powr(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_powr(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_powr(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_powr(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_powr(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_powr(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_powr(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_powr(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_powr(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_powr(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_powr(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_powr(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_remainder(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_remainder(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_remainder(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_remainder(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_remainder(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_remainder(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_remainder(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_remainder(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_remainder(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_remainder(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_remainder(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_remainder(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_remainder(half y, half x);
half2 __attribute__((overloadable))    __spirv_ocl_remainder(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_remainder(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_remainder(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_remainder(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_remainder(half16 x, half16 y);

float __attribute__((overloadable))
__spirv_ocl_remquo(float xx, float yy, __global int* quo);
float2 __attribute__((overloadable))
__spirv_ocl_remquo(float2 xx, float2 yy, __global int2* quo);
float3 __attribute__((overloadable))
__spirv_ocl_remquo(float3 xx, float3 yy, __global int3* quo);
float4 __attribute__((overloadable))
__spirv_ocl_remquo(float4 xx, float4 yy, __global int4* quo);
float8 __attribute__((overloadable))
__spirv_ocl_remquo(float8 xx, float8 yy, __global int8* quo);
float16 __attribute__((overloadable))
__spirv_ocl_remquo(float16 xx, float16 yy, __global int16* quo);
float __attribute__((overloadable))
__spirv_ocl_remquo(float xx, float yy, __private int* quo);
float2 __attribute__((overloadable))
__spirv_ocl_remquo(float2 xx, float2 yy, __private int2* quo);
float3 __attribute__((overloadable))
__spirv_ocl_remquo(float3 xx, float3 yy, __private int3* quo);
float4 __attribute__((overloadable))
__spirv_ocl_remquo(float4 xx, float4 yy, __private int4* quo);
float8 __attribute__((overloadable))
__spirv_ocl_remquo(float8 xx, float8 yy, __private int8* quo);
float16 __attribute__((overloadable))
__spirv_ocl_remquo(float16 xx, float16 yy, __private int16* quo);
float __attribute__((overloadable))
__spirv_ocl_remquo(float xx, float yy, __local int* quo);
float2 __attribute__((overloadable))
__spirv_ocl_remquo(float2 xx, float2 yy, __local int2* quo);
float3 __attribute__((overloadable))
__spirv_ocl_remquo(float3 xx, float3 yy, __local int3* quo);
float4 __attribute__((overloadable))
__spirv_ocl_remquo(float4 xx, float4 yy, __local int4* quo);
float8 __attribute__((overloadable))
__spirv_ocl_remquo(float8 xx, float8 yy, __local int8* quo);
float16 __attribute__((overloadable))
__spirv_ocl_remquo(float16 xx, float16 yy, __local int16* quo);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))
__spirv_ocl_remquo(float xx, float yy, __generic int* quo);
float2 __attribute__((overloadable))
__spirv_ocl_remquo(float2 xx, float2 yy, __generic int2* quo);
float3 __attribute__((overloadable))
__spirv_ocl_remquo(float3 xx, float3 yy, __generic int3* quo);
float4 __attribute__((overloadable))
__spirv_ocl_remquo(float4 xx, float4 yy, __generic int4* quo);
float8 __attribute__((overloadable))
__spirv_ocl_remquo(float8 xx, float8 yy, __generic int8* quo);
float16 __attribute__((overloadable))
__spirv_ocl_remquo(float16 xx, float16 yy, __generic int16* quo);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half
    __attribute__((overloadable)) __spirv_ocl_remquo(half xx, half yy, __global int* quo);
half2 __attribute__((overloadable))
__spirv_ocl_remquo(half2 xx, half2 yy, __global int2* quo);
half3 __attribute__((overloadable))
__spirv_ocl_remquo(half3 xx, half3 yy, __global int3* quo);
half4 __attribute__((overloadable))
__spirv_ocl_remquo(half4 xx, half4 yy, __global int4* quo);
half8 __attribute__((overloadable))
__spirv_ocl_remquo(half8 xx, half8 yy, __global int8* quo);
half16 __attribute__((overloadable))
__spirv_ocl_remquo(half16 xx, half16 yy, __global int16* quo);
half __attribute__((overloadable))
__spirv_ocl_remquo(half xx, half yy, __private int* quo);
half2 __attribute__((overloadable))
__spirv_ocl_remquo(half2 xx, half2 yy, __private int2* quo);
half3 __attribute__((overloadable))
__spirv_ocl_remquo(half3 xx, half3 yy, __private int3* quo);
half4 __attribute__((overloadable))
__spirv_ocl_remquo(half4 xx, half4 yy, __private int4* quo);
half8 __attribute__((overloadable))
__spirv_ocl_remquo(half8 xx, half8 yy, __private int8* quo);
half16 __attribute__((overloadable))
__spirv_ocl_remquo(half16 xx, half16 yy, __private int16* quo);
half __attribute__((overloadable)) __spirv_ocl_remquo(half xx, half yy, __local int* quo);
half2 __attribute__((overloadable))
__spirv_ocl_remquo(half2 xx, half2 yy, __local int2* quo);
half3 __attribute__((overloadable))
__spirv_ocl_remquo(half3 xx, half3 yy, __local int3* quo);
half4 __attribute__((overloadable))
__spirv_ocl_remquo(half4 xx, half4 yy, __local int4* quo);
half8 __attribute__((overloadable))
__spirv_ocl_remquo(half8 xx, half8 yy, __local int8* quo);
half16 __attribute__((overloadable))
__spirv_ocl_remquo(half16 xx, half16 yy, __local int16* quo);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))
__spirv_ocl_remquo(half xx, half yy, __generic int* quo);
half2 __attribute__((overloadable))
__spirv_ocl_remquo(half2 xx, half2 yy, __generic int2* quo);
half3 __attribute__((overloadable))
__spirv_ocl_remquo(half3 xx, half3 yy, __generic int3* quo);
half4 __attribute__((overloadable))
__spirv_ocl_remquo(half4 xx, half4 yy, __generic int4* quo);
half8 __attribute__((overloadable))
__spirv_ocl_remquo(half8 xx, half8 yy, __generic int8* quo);
half16 __attribute__((overloadable))
__spirv_ocl_remquo(half16 xx, half16 yy, __generic int16* quo);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_ocl_remquo(double xx, double yy, __global int* quo);
double2 __attribute__((overloadable))
__spirv_ocl_remquo(double2 xx, double2 yy, __global int2* quo);
double3 __attribute__((overloadable))
__spirv_ocl_remquo(double3 xx, double3 yy, __global int3* quo);
double4 __attribute__((overloadable))
__spirv_ocl_remquo(double4 xx, double4 yy, __global int4* quo);
double8 __attribute__((overloadable))
__spirv_ocl_remquo(double8 xx, double8 yy, __global int8* quo);
double16 __attribute__((overloadable))
__spirv_ocl_remquo(double16 xx, double16 yy, __global int16* quo);
double __attribute__((overloadable))
__spirv_ocl_remquo(double xx, double yy, __private int* quo);
double2 __attribute__((overloadable))
__spirv_ocl_remquo(double2 xx, double2 yy, __private int2* quo);
double3 __attribute__((overloadable))
__spirv_ocl_remquo(double3 xx, double3 yy, __private int3* quo);
double4 __attribute__((overloadable))
__spirv_ocl_remquo(double4 xx, double4 yy, __private int4* quo);
double8 __attribute__((overloadable))
__spirv_ocl_remquo(double8 xx, double8 yy, __private int8* quo);
double16 __attribute__((overloadable))
__spirv_ocl_remquo(double16 xx, double16 yy, __private int16* quo);
double __attribute__((overloadable))
__spirv_ocl_remquo(double xx, double yy, __local int* quo);
double2 __attribute__((overloadable))
__spirv_ocl_remquo(double2 xx, double2 yy, __local int2* quo);
double3 __attribute__((overloadable))
__spirv_ocl_remquo(double3 xx, double3 yy, __local int3* quo);
double4 __attribute__((overloadable))
__spirv_ocl_remquo(double4 xx, double4 yy, __local int4* quo);
double8 __attribute__((overloadable))
__spirv_ocl_remquo(double8 xx, double8 yy, __local int8* quo);
double16 __attribute__((overloadable))
__spirv_ocl_remquo(double16 xx, double16 yy, __local int16* quo);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable))
__spirv_ocl_remquo(double xx, double yy, __generic int* quo);
double2 __attribute__((overloadable))
__spirv_ocl_remquo(double2 xx, double2 yy, __generic int2* quo);
double3 __attribute__((overloadable))
__spirv_ocl_remquo(double3 xx, double3 yy, __generic int3* quo);
double4 __attribute__((overloadable))
__spirv_ocl_remquo(double4 xx, double4 yy, __generic int4* quo);
double8 __attribute__((overloadable))
__spirv_ocl_remquo(double8 xx, double8 yy, __generic int8* quo);
double16 __attribute__((overloadable))
__spirv_ocl_remquo(double16 xx, double16 yy, __generic int16* quo);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_rint(float x);
float2 __attribute__((overloadable))  __spirv_ocl_rint(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_rint(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_rint(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_rint(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_rint(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_rint(double x);
double2 __attribute__((overloadable))  __spirv_ocl_rint(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_rint(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_rint(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_rint(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_rint(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_rint(half x);
half2 __attribute__((overloadable))    __spirv_ocl_rint(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_rint(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_rint(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_rint(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_rint(half16 x);

float __attribute__((overloadable))   __spirv_ocl_rootn(float x, int n);
float2 __attribute__((overloadable))  __spirv_ocl_rootn(float2 x, int2 y);
float3 __attribute__((overloadable))  __spirv_ocl_rootn(float3 x, int3 y);
float4 __attribute__((overloadable))  __spirv_ocl_rootn(float4 x, int4 y);
float8 __attribute__((overloadable))  __spirv_ocl_rootn(float8 x, int8 y);
float16 __attribute__((overloadable)) __spirv_ocl_rootn(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_rootn(double y, int x);
double2 __attribute__((overloadable))  __spirv_ocl_rootn(double2 x, int2 y);
double3 __attribute__((overloadable))  __spirv_ocl_rootn(double3 x, int3 y);
double4 __attribute__((overloadable))  __spirv_ocl_rootn(double4 x, int4 y);
double8 __attribute__((overloadable))  __spirv_ocl_rootn(double8 x, int8 y);
double16 __attribute__((overloadable)) __spirv_ocl_rootn(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_rootn(half y, int x);
half2 __attribute__((overloadable))    __spirv_ocl_rootn(half2 x, int2 y);
half3 __attribute__((overloadable))    __spirv_ocl_rootn(half3 x, int3 y);
half4 __attribute__((overloadable))    __spirv_ocl_rootn(half4 x, int4 y);
half8 __attribute__((overloadable))    __spirv_ocl_rootn(half8 x, int8 y);
half16 __attribute__((overloadable))   __spirv_ocl_rootn(half16 x, int16 y);

float __attribute__((overloadable))   __spirv_ocl_round(float x);
float2 __attribute__((overloadable))  __spirv_ocl_round(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_round(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_round(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_round(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_round(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_round(double x);
double2 __attribute__((overloadable))  __spirv_ocl_round(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_round(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_round(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_round(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_round(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_round(half x);
half2 __attribute__((overloadable))    __spirv_ocl_round(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_round(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_round(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_round(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_round(half16 x);

float __attribute__((overloadable))   __spirv_ocl_rsqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_rsqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_rsqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_rsqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_rsqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_rsqrt(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_rsqrt(double x);
double2 __attribute__((overloadable))  __spirv_ocl_rsqrt(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_rsqrt(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_rsqrt(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_rsqrt(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_rsqrt(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_rsqrt(half x);
half2 __attribute__((overloadable))    __spirv_ocl_rsqrt(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_rsqrt(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_rsqrt(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_rsqrt(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_rsqrt(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sin(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sin(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sin(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sin(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sin(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sin(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_sin(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sin(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sin(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sin(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sin(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sin(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_sin(half x);
half2 __attribute__((overloadable))    __spirv_ocl_sin(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_sin(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_sin(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_sin(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_sin(half16 x);

float __attribute__((overloadable)) __spirv_ocl_sincos(float x, __private float* cosval);
float __attribute__((overloadable)) __spirv_ocl_sincos(float x, __global float* cosval);
float __attribute__((overloadable)) __spirv_ocl_sincos(float x, __local float* cosval);
float2 __attribute__((overloadable))
__spirv_ocl_sincos(float2 x, __private float2* cosval);
float2 __attribute__((overloadable))
__spirv_ocl_sincos(float2 x, __global float2* cosval);
float2 __attribute__((overloadable)) __spirv_ocl_sincos(float2 x, __local float2* cosval);
float3 __attribute__((overloadable))
__spirv_ocl_sincos(float3 x, __private float3* cosval);
float3 __attribute__((overloadable))
__spirv_ocl_sincos(float3 x, __global float3* cosval);
float3 __attribute__((overloadable)) __spirv_ocl_sincos(float3 x, __local float3* cosval);
float4 __attribute__((overloadable))
__spirv_ocl_sincos(float4 x, __private float4* cosval);
float4 __attribute__((overloadable))
__spirv_ocl_sincos(float4 x, __global float4* cosval);
float4 __attribute__((overloadable)) __spirv_ocl_sincos(float4 x, __local float4* cosval);
float8 __attribute__((overloadable))
__spirv_ocl_sincos(float8 x, __private float8* cosval);
float8 __attribute__((overloadable))
__spirv_ocl_sincos(float8 x, __global float8* cosval);
float8 __attribute__((overloadable)) __spirv_ocl_sincos(float8 x, __local float8* cosval);
float16 __attribute__((overloadable))
__spirv_ocl_sincos(float16 x, __private float16* cosval);
float16 __attribute__((overloadable))
__spirv_ocl_sincos(float16 x, __global float16* cosval);
float16 __attribute__((overloadable))
__spirv_ocl_sincos(float16 x, __local float16* cosval);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) __spirv_ocl_sincos(float x, __generic float* cosval);
float2 __attribute__((overloadable))
__spirv_ocl_sincos(float2 x, __generic float2* cosval);
float3 __attribute__((overloadable))
__spirv_ocl_sincos(float3 x, __generic float3* cosval);
float4 __attribute__((overloadable))
__spirv_ocl_sincos(float4 x, __generic float4* cosval);
float8 __attribute__((overloadable))
__spirv_ocl_sincos(float8 x, __generic float8* cosval);
float16 __attribute__((overloadable))
__spirv_ocl_sincos(float16 x, __generic float16* cosval);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __attribute__((overloadable))  __spirv_ocl_sincos(half x, __private half* cosval);
half __attribute__((overloadable))  __spirv_ocl_sincos(half x, __global half* cosval);
half __attribute__((overloadable))  __spirv_ocl_sincos(half x, __local half* cosval);
half2 __attribute__((overloadable)) __spirv_ocl_sincos(half2 x, __private half2* cosval);
half2 __attribute__((overloadable)) __spirv_ocl_sincos(half2 x, __global half2* cosval);
half2 __attribute__((overloadable)) __spirv_ocl_sincos(half2 x, __local half2* cosval);
half3 __attribute__((overloadable)) __spirv_ocl_sincos(half3 x, __private half3* cosval);
half3 __attribute__((overloadable)) __spirv_ocl_sincos(half3 x, __global half3* cosval);
half3 __attribute__((overloadable)) __spirv_ocl_sincos(half3 x, __local half3* cosval);
half4 __attribute__((overloadable)) __spirv_ocl_sincos(half4 x, __private half4* cosval);
half4 __attribute__((overloadable)) __spirv_ocl_sincos(half4 x, __global half4* cosval);
half4 __attribute__((overloadable)) __spirv_ocl_sincos(half4 x, __local half4* cosval);
half8 __attribute__((overloadable)) __spirv_ocl_sincos(half8 x, __private half8* cosval);
half8 __attribute__((overloadable)) __spirv_ocl_sincos(half8 x, __global half8* cosval);
half8 __attribute__((overloadable)) __spirv_ocl_sincos(half8 x, __local half8* cosval);
half16 __attribute__((overloadable))
__spirv_ocl_sincos(half16 x, __private half16* cosval);
half16 __attribute__((overloadable))
__spirv_ocl_sincos(half16 x, __global half16* cosval);
half16 __attribute__((overloadable)) __spirv_ocl_sincos(half16 x, __local half16* cosval);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))  __spirv_ocl_sincos(half x, __generic half* cosval);
half2 __attribute__((overloadable)) __spirv_ocl_sincos(half2 x, __generic half2* cosval);
half3 __attribute__((overloadable)) __spirv_ocl_sincos(half3 x, __generic half3* cosval);
half4 __attribute__((overloadable)) __spirv_ocl_sincos(half4 x, __generic half4* cosval);
half8 __attribute__((overloadable)) __spirv_ocl_sincos(half8 x, __generic half8* cosval);
half16 __attribute__((overloadable))
__spirv_ocl_sincos(half16 x, __generic half16* cosval);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double
    __attribute__((overloadable)) __spirv_ocl_sincos(double x, __private double* cosval);
double __attribute__((overloadable)) __spirv_ocl_sincos(double x, __local double* cosval);
double __attribute__((overloadable))
__spirv_ocl_sincos(double x, __global double* cosval);
double __attribute__((overloadable)) __spirv_ocl_sincos(double x, __local double* cosval);
double2 __attribute__((overloadable))
__spirv_ocl_sincos(double2 x, __private double2* cosval);
double2 __attribute__((overloadable))
__spirv_ocl_sincos(double2 x, __global double2* cosval);
double2 __attribute__((overloadable))
__spirv_ocl_sincos(double2 x, __local double2* cosval);
double3 __attribute__((overloadable))
__spirv_ocl_sincos(double3 x, __private double3* cosval);
double3 __attribute__((overloadable))
__spirv_ocl_sincos(double3 x, __global double3* cosval);
double3 __attribute__((overloadable))
__spirv_ocl_sincos(double3 x, __local double3* cosval);
double4 __attribute__((overloadable))
__spirv_ocl_sincos(double4 x, __private double4* cosval);
double4 __attribute__((overloadable))
__spirv_ocl_sincos(double4 x, __global double4* cosval);
double4 __attribute__((overloadable))
__spirv_ocl_sincos(double4 x, __local double4* cosval);
double8 __attribute__((overloadable))
__spirv_ocl_sincos(double8 x, __private double8* cosval);
double8 __attribute__((overloadable))
__spirv_ocl_sincos(double8 x, __global double8* cosval);
double8 __attribute__((overloadable))
__spirv_ocl_sincos(double8 x, __local double8* cosval);
double16 __attribute__((overloadable))
__spirv_ocl_sincos(double16 x, __private double16* cosval);
double16 __attribute__((overloadable))
__spirv_ocl_sincos(double16 x, __global double16* cosval);
double16 __attribute__((overloadable))
__spirv_ocl_sincos(double16 x, __local double16* cosval);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double
    __attribute__((overloadable)) __spirv_ocl_sincos(double x, __generic double* cosval);
double2 __attribute__((overloadable))
__spirv_ocl_sincos(double2 x, __generic double2* cosval);
double3 __attribute__((overloadable))
__spirv_ocl_sincos(double3 x, __generic double3* cosval);
double4 __attribute__((overloadable))
__spirv_ocl_sincos(double4 x, __generic double4* cosval);
double8 __attribute__((overloadable))
__spirv_ocl_sincos(double8 x, __generic double8* cosval);
double16 __attribute__((overloadable))
__spirv_ocl_sincos(double16 x, __generic double16* cosval);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable))   __spirv_ocl_sinh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sinh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sinh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sinh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sinh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sinh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_sinh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sinh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sinh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sinh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sinh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sinh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_sinh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_sinh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_sinh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_sinh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_sinh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_sinh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sinpi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sinpi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sinpi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sinpi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sinpi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sinpi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_sinpi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sinpi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sinpi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sinpi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sinpi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sinpi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_sinpi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_sinpi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_sinpi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_sinpi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_sinpi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_sinpi(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sqrt(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_sqrt(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sqrt(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sqrt(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sqrt(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sqrt(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sqrt(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_sqrt(half x);
half2 __attribute__((overloadable))    __spirv_ocl_sqrt(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_sqrt(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_sqrt(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_sqrt(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_sqrt(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sqrt_cr(float a);
float2 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sqrt_cr(float16 x);
#ifdef cl_fp64_basic_ops
double __attribute__((overloadable))   __spirv_ocl_sqrt_cr(double x);
double2 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_sqrt_cr(double16 x);
#endif // cl_fp64_basic_ops
half __attribute__((overloadable))   __spirv_ocl_sqrt_cr(half a);
half2 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(half2 x);
half3 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(half3 x);
half4 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(half4 x);
half8 __attribute__((overloadable))  __spirv_ocl_sqrt_cr(half8 x);
half16 __attribute__((overloadable)) __spirv_ocl_sqrt_cr(half16 x);

float __attribute__((overloadable))   __spirv_ocl_tan(float x);
float2 __attribute__((overloadable))  __spirv_ocl_tan(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_tan(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_tan(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_tan(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_tan(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_tan(double x);
double2 __attribute__((overloadable))  __spirv_ocl_tan(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_tan(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_tan(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_tan(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_tan(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_tan(half x);
half2 __attribute__((overloadable))    __spirv_ocl_tan(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_tan(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_tan(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_tan(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_tan(half16 x);

float __attribute__((overloadable))   __spirv_ocl_tanh(float x);
float2 __attribute__((overloadable))  __spirv_ocl_tanh(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_tanh(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_tanh(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_tanh(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_tanh(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_tanh(double x);
double2 __attribute__((overloadable))  __spirv_ocl_tanh(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_tanh(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_tanh(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_tanh(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_tanh(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_tanh(half x);
half2 __attribute__((overloadable))    __spirv_ocl_tanh(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_tanh(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_tanh(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_tanh(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_tanh(half16 x);

float __attribute__((overloadable))   __spirv_ocl_sigm(float x);
float2 __attribute__((overloadable))  __spirv_ocl_sigm(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_sigm(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_sigm(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_sigm(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_sigm(float16 x);

half __attribute__((overloadable))   __spirv_ocl_sigm(half x);
half2 __attribute__((overloadable))  __spirv_ocl_sigm(half2 x);
half3 __attribute__((overloadable))  __spirv_ocl_sigm(half3 x);
half4 __attribute__((overloadable))  __spirv_ocl_sigm(half4 x);
half8 __attribute__((overloadable))  __spirv_ocl_sigm(half8 x);
half16 __attribute__((overloadable)) __spirv_ocl_sigm(half16 x);

float __attribute__((overloadable))   __spirv_ocl_tanpi(float x);
float2 __attribute__((overloadable))  __spirv_ocl_tanpi(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_tanpi(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_tanpi(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_tanpi(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_tanpi(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_tanpi(double x);
double2 __attribute__((overloadable))  __spirv_ocl_tanpi(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_tanpi(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_tanpi(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_tanpi(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_tanpi(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_tanpi(half x);
half2 __attribute__((overloadable))    __spirv_ocl_tanpi(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_tanpi(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_tanpi(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_tanpi(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_tanpi(half16 x);

float __attribute__((overloadable))   __spirv_ocl_tgamma(float x);
float2 __attribute__((overloadable))  __spirv_ocl_tgamma(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_tgamma(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_tgamma(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_tgamma(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_tgamma(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_tgamma(double x);
double2 __attribute__((overloadable))  __spirv_ocl_tgamma(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_tgamma(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_tgamma(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_tgamma(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_tgamma(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_tgamma(half x);
half2 __attribute__((overloadable))    __spirv_ocl_tgamma(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_tgamma(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_tgamma(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_tgamma(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_tgamma(half16 x);

float __attribute__((overloadable))   __spirv_ocl_trunc(float x);
float2 __attribute__((overloadable))  __spirv_ocl_trunc(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_trunc(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_trunc(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_trunc(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_trunc(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_trunc(double x);
double2 __attribute__((overloadable))  __spirv_ocl_trunc(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_trunc(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_trunc(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_trunc(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_trunc(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_trunc(half x);
half2 __attribute__((overloadable))    __spirv_ocl_trunc(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_trunc(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_trunc(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_trunc(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_trunc(half16 x);

//
//  Native
//        -native_cos,native_divide,native_exp,native_exp2,native_exp10,native_log,native_log2,
//         native_log10,native_powr,native_recip,native_rsqrt,native_sin,native_sqrt,native_tan
//

float __attribute__((overloadable))   __spirv_ocl_native_cos(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_cos(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_cos(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_cos(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_cos(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_cos(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_cos(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_cos(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_cos(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_cos(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_cos(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_cos(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_cos(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_cos(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_cos(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_cos(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_cos(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_cos(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_divide(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_native_divide(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_native_divide(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_native_divide(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_native_divide(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_native_divide(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_divide(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_native_divide(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_native_divide(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_native_divide(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_native_divide(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_native_divide(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_native_divide(half x, half y);
half2 __attribute__((overloadable))  __spirv_ocl_native_divide(half2 x, half2 y);
half3 __attribute__((overloadable))  __spirv_ocl_native_divide(half3 x, half3 y);
half4 __attribute__((overloadable))  __spirv_ocl_native_divide(half4 x, half4 y);
half8 __attribute__((overloadable))  __spirv_ocl_native_divide(half8 x, half8 y);
half16 __attribute__((overloadable)) __spirv_ocl_native_divide(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_native_exp(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_exp(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_exp(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_exp(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_exp(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_exp(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_exp(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_exp(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_exp(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_exp(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_exp(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_exp(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_exp(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_exp(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_exp(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_exp(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_exp(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_exp(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_exp2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_exp2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_exp2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_exp2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_exp2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_exp2(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_exp2(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_exp2(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_exp2(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_exp2(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_exp2(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_exp2(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_exp2(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_exp2(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_exp2(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_exp2(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_exp2(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_exp2(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_exp10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_exp10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_exp10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_exp10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_exp10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_exp10(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_exp10(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_exp10(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_exp10(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_exp10(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_exp10(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_exp10(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_exp10(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_exp10(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_exp10(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_exp10(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_exp10(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_exp10(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_log(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_log(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_log(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_log(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_log(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_log(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_log(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_log(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_log(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_log(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_log(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_log(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_log(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_log(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_log(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_log(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_log(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_log(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_log2(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_log2(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_log2(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_log2(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_log2(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_log2(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_log2(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_log2(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_log2(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_log2(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_log2(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_log2(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_log2(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_log2(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_log2(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_log2(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_log2(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_log2(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_log10(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_log10(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_log10(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_log10(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_log10(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_log10(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_log10(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_log10(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_log10(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_log10(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_log10(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_log10(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_log10(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_log10(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_log10(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_log10(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_log10(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_log10(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_powr(float x, float y);
float2 __attribute__((overloadable))  __spirv_ocl_native_powr(float2 x, float2 y);
float3 __attribute__((overloadable))  __spirv_ocl_native_powr(float3 x, float3 y);
float4 __attribute__((overloadable))  __spirv_ocl_native_powr(float4 x, float4 y);
float8 __attribute__((overloadable))  __spirv_ocl_native_powr(float8 x, float8 y);
float16 __attribute__((overloadable)) __spirv_ocl_native_powr(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_powr(double x, double y);
double2 __attribute__((overloadable))  __spirv_ocl_native_powr(double2 x, double2 y);
double3 __attribute__((overloadable))  __spirv_ocl_native_powr(double3 x, double3 y);
double4 __attribute__((overloadable))  __spirv_ocl_native_powr(double4 x, double4 y);
double8 __attribute__((overloadable))  __spirv_ocl_native_powr(double8 x, double8 y);
double16 __attribute__((overloadable)) __spirv_ocl_native_powr(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_powr(half x, half y);
half2 __attribute__((overloadable))    __spirv_ocl_native_powr(half2 x, half2 y);
half3 __attribute__((overloadable))    __spirv_ocl_native_powr(half3 x, half3 y);
half4 __attribute__((overloadable))    __spirv_ocl_native_powr(half4 x, half4 y);
half8 __attribute__((overloadable))    __spirv_ocl_native_powr(half8 x, half8 y);
half16 __attribute__((overloadable))   __spirv_ocl_native_powr(half16 x, half16 y);

float __attribute__((overloadable))   __spirv_ocl_native_recip(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_recip(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_recip(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_recip(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_recip(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_recip(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_recip(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_recip(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_recip(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_recip(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_recip(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_recip(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_recip(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_recip(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_recip(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_recip(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_recip(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_recip(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_rsqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_rsqrt(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_rsqrt(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_rsqrt(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_rsqrt(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_rsqrt(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_rsqrt(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_rsqrt(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_rsqrt(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_rsqrt(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_sin(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_sin(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_sin(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_sin(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_sin(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_sin(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_sin(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_sin(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_sin(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_sin(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_sin(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_sin(double16 x);
#endif // cl_khr_fp64
half __attribute__((overloadable))     __spirv_ocl_native_sin(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_sin(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_sin(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_sin(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_sin(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_sin(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_sqrt(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_sqrt(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_sqrt(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_sqrt(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_sqrt(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_sqrt(float16 x);
#ifdef cl_fp64_basic_ops
double __attribute__((overloadable))   __spirv_ocl_native_sqrt(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_sqrt(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_sqrt(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_sqrt(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_sqrt(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_sqrt(double16 x);
#endif // cl_fp64_basic_ops
half __attribute__((overloadable))     __spirv_ocl_native_sqrt(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_sqrt(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_sqrt(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_sqrt(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_sqrt(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_sqrt(half16 x);

float __attribute__((overloadable))   __spirv_ocl_native_tan(float x);
float2 __attribute__((overloadable))  __spirv_ocl_native_tan(float2 x);
float3 __attribute__((overloadable))  __spirv_ocl_native_tan(float3 x);
float4 __attribute__((overloadable))  __spirv_ocl_native_tan(float4 x);
float8 __attribute__((overloadable))  __spirv_ocl_native_tan(float8 x);
float16 __attribute__((overloadable)) __spirv_ocl_native_tan(float16 x);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))   __spirv_ocl_native_tan(double x);
double2 __attribute__((overloadable))  __spirv_ocl_native_tan(double2 x);
double3 __attribute__((overloadable))  __spirv_ocl_native_tan(double3 x);
double4 __attribute__((overloadable))  __spirv_ocl_native_tan(double4 x);
double8 __attribute__((overloadable))  __spirv_ocl_native_tan(double8 x);
double16 __attribute__((overloadable)) __spirv_ocl_native_tan(double16 x);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))     __spirv_ocl_native_tan(half x);
half2 __attribute__((overloadable))    __spirv_ocl_native_tan(half2 x);
half3 __attribute__((overloadable))    __spirv_ocl_native_tan(half3 x);
half4 __attribute__((overloadable))    __spirv_ocl_native_tan(half4 x);
half8 __attribute__((overloadable))    __spirv_ocl_native_tan(half8 x);
half16 __attribute__((overloadable))   __spirv_ocl_native_tan(half16 x);

//
//  Relational
//        -bitselect,select
//

char __attribute__((overloadable))   __spirv_ocl_bitselect(char a, char b, char c);
char2 __attribute__((overloadable))  __spirv_ocl_bitselect(char2 x, char2 y, char2 z);
char3 __attribute__((overloadable))  __spirv_ocl_bitselect(char3 x, char3 y, char3 z);
char4 __attribute__((overloadable))  __spirv_ocl_bitselect(char4 x, char4 y, char4 z);
char8 __attribute__((overloadable))  __spirv_ocl_bitselect(char8 x, char8 y, char8 z);
char16 __attribute__((overloadable)) __spirv_ocl_bitselect(char16 x, char16 y, char16 z);
short __attribute__((overloadable))  __spirv_ocl_bitselect(short a, short b, short c);
short2 __attribute__((overloadable)) __spirv_ocl_bitselect(short2 x, short2 y, short2 z);
short3 __attribute__((overloadable)) __spirv_ocl_bitselect(short3 x, short3 y, short3 z);
short4 __attribute__((overloadable)) __spirv_ocl_bitselect(short4 x, short4 y, short4 z);
short8 __attribute__((overloadable)) __spirv_ocl_bitselect(short8 x, short8 y, short8 z);
short16 __attribute__((overloadable))
                                   __spirv_ocl_bitselect(short16 x, short16 y, short16 z);
int __attribute__((overloadable))  __spirv_ocl_bitselect(int a, int b, int c);
int2 __attribute__((overloadable)) __spirv_ocl_bitselect(int2 x, int2 y, int2 z);
int3 __attribute__((overloadable)) __spirv_ocl_bitselect(int3 x, int3 y, int3 z);
int4 __attribute__((overloadable)) __spirv_ocl_bitselect(int4 x, int4 y, int4 z);
int8 __attribute__((overloadable)) __spirv_ocl_bitselect(int8 x, int8 y, int8 z);
int16 __attribute__((overloadable))  __spirv_ocl_bitselect(int16 x, int16 y, int16 z);
long __attribute__((overloadable))   __spirv_ocl_bitselect(long a, long b, long c);
long2 __attribute__((overloadable))  __spirv_ocl_bitselect(long2 x, long2 y, long2 z);
long3 __attribute__((overloadable))  __spirv_ocl_bitselect(long3 x, long3 y, long3 z);
long4 __attribute__((overloadable))  __spirv_ocl_bitselect(long4 x, long4 y, long4 z);
long8 __attribute__((overloadable))  __spirv_ocl_bitselect(long8 x, long8 y, long8 z);
long16 __attribute__((overloadable)) __spirv_ocl_bitselect(long16 x, long16 y, long16 z);
float __attribute__((overloadable))  __spirv_ocl_bitselect(float a, float b, float c);
float2 __attribute__((overloadable)) __spirv_ocl_bitselect(float2 x, float2 y, float2 z);
float3 __attribute__((overloadable)) __spirv_ocl_bitselect(float3 x, float3 y, float3 z);
float4 __attribute__((overloadable)) __spirv_ocl_bitselect(float4 x, float4 y, float4 z);
float8 __attribute__((overloadable)) __spirv_ocl_bitselect(float8 x, float8 y, float8 z);
float16 __attribute__((overloadable))
__spirv_ocl_bitselect(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ocl_bitselect(double a, double b, double c);
double2 __attribute__((overloadable))
__spirv_ocl_bitselect(double2 x, double2 y, double2 z);
double3 __attribute__((overloadable))
__spirv_ocl_bitselect(double3 x, double3 y, double3 z);
double4 __attribute__((overloadable))
__spirv_ocl_bitselect(double4 x, double4 y, double4 z);
double8 __attribute__((overloadable))
__spirv_ocl_bitselect(double8 x, double8 y, double8 z);
double16 __attribute__((overloadable))
__spirv_ocl_bitselect(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))   __spirv_ocl_bitselect(half a, half b, half c);
half2 __attribute__((overloadable))  __spirv_ocl_bitselect(half2 x, half2 y, half2 z);
half3 __attribute__((overloadable))  __spirv_ocl_bitselect(half3 x, half3 y, half3 z);
half4 __attribute__((overloadable))  __spirv_ocl_bitselect(half4 x, half4 y, half4 z);
half8 __attribute__((overloadable))  __spirv_ocl_bitselect(half8 x, half8 y, half8 z);
half16 __attribute__((overloadable)) __spirv_ocl_bitselect(half16 x, half16 y, half16 z);

char __attribute__((overloadable))  __spirv_ocl_select(char a, char b, char c);
short __attribute__((overloadable)) __spirv_ocl_select(short a, short b, short c);
int __attribute__((overloadable))   __spirv_ocl_select(int a, int b, int c);
long __attribute__((overloadable))  __spirv_ocl_select(long a, long b, long c);

float __attribute__((overloadable))   __spirv_ocl_select(float a, float b, int c);
float2 __attribute__((overloadable))  __spirv_ocl_select(float2 a, float2 b, int2 c);
float3 __attribute__((overloadable))  __spirv_ocl_select(float3 a, float3 b, int3 c);
float4 __attribute__((overloadable))  __spirv_ocl_select(float4 a, float4 b, int4 c);
float8 __attribute__((overloadable))  __spirv_ocl_select(float8 a, float8 b, int8 c);
float16 __attribute__((overloadable)) __spirv_ocl_select(float16 a, float16 b, int16 c);

#if defined(cl_khr_fp64)
double __attribute__((overloadable))  __spirv_ocl_select(double a, double b, long c);
double2 __attribute__((overloadable)) __spirv_ocl_select(double2 a, double2 b, long2 c);
double3 __attribute__((overloadable)) __spirv_ocl_select(double3 a, double3 b, long3 c);
double4 __attribute__((overloadable)) __spirv_ocl_select(double4 a, double4 b, long4 c);
double8 __attribute__((overloadable)) __spirv_ocl_select(double8 a, double8 b, long8 c);
double16 __attribute__((overloadable))
__spirv_ocl_select(double16 a, double16 b, long16 c);

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __attribute__((overloadable))   __spirv_ocl_select(half a, half b, short c);
half2 __attribute__((overloadable))  __spirv_ocl_select(half2 a, half2 b, short2 c);
half3 __attribute__((overloadable))  __spirv_ocl_select(half3 a, half3 b, short3 c);
half4 __attribute__((overloadable))  __spirv_ocl_select(half4 a, half4 b, short4 c);
half8 __attribute__((overloadable))  __spirv_ocl_select(half8 a, half8 b, short8 c);
half16 __attribute__((overloadable)) __spirv_ocl_select(half16 a, half16 b, short16 c);

#endif // defined(cl_khr_fp16)

#endif // __SPIRV_MATH_H__
