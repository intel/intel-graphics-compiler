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

float __builtin_spirv_OpenCL_degrees_f32(float r );
float2 __builtin_spirv_OpenCL_degrees_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_degrees_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_degrees_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_degrees_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_degrees_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_degrees_f64(double r );
double2 __builtin_spirv_OpenCL_degrees_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_degrees_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_degrees_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_degrees_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_degrees_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_degrees_f16(half r );
half2 __builtin_spirv_OpenCL_degrees_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_degrees_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_degrees_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_degrees_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_degrees_v16f16(half16 x);

float __builtin_spirv_OpenCL_fclamp_f32_f32_f32(float x, float y, float z);
float2 __builtin_spirv_OpenCL_fclamp_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_fclamp_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_fclamp_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_fclamp_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_fclamp_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fclamp_f64_f64_f64(double x, double y, double z);
double2 __builtin_spirv_OpenCL_fclamp_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_fclamp_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_fclamp_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_fclamp_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_fclamp_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fclamp_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_fclamp_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_fclamp_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_fclamp_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_fclamp_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_fclamp_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_fmax_common_f32_f32(float x, float y);
float2 __builtin_spirv_OpenCL_fmax_common_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmax_common_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmax_common_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmax_common_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmax_common_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fmax_common_f64_f64(double x, double y);
double2 __builtin_spirv_OpenCL_fmax_common_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmax_common_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmax_common_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmax_common_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmax_common_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fmax_common_f16_f16(half x, half y);
half2 __builtin_spirv_OpenCL_fmax_common_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmax_common_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmax_common_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmax_common_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmax_common_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_fmin_common_f32_f32(float x, float y);
half __builtin_spirv_OpenCL_fmin_common_f16_f16(half x, half y);
float2 __builtin_spirv_OpenCL_fmin_common_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmin_common_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmin_common_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmin_common_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmin_common_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fmin_common_f64_f64(double x, double y);
double2 __builtin_spirv_OpenCL_fmin_common_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmin_common_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmin_common_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmin_common_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmin_common_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half2 __builtin_spirv_OpenCL_fmin_common_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmin_common_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmin_common_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmin_common_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmin_common_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_mix_f32_f32_f32(float x, float y, float a );
float2 __builtin_spirv_OpenCL_mix_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_mix_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_mix_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_mix_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_mix_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_mix_f64_f64_f64(double x, double y, double a );
double2 __builtin_spirv_OpenCL_mix_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_mix_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_mix_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_mix_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_mix_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_mix_f16_f16_f16(half x, half y, half a );
half2 __builtin_spirv_OpenCL_mix_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_mix_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_mix_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_mix_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_mix_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_radians_f32(float d );
float2 __builtin_spirv_OpenCL_radians_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_radians_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_radians_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_radians_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_radians_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_radians_f64(double d );
double2 __builtin_spirv_OpenCL_radians_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_radians_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_radians_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_radians_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_radians_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_radians_f16(half d );
half2 __builtin_spirv_OpenCL_radians_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_radians_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_radians_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_radians_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_radians_v16f16(half16 x);

float __builtin_spirv_OpenCL_sign_f32(float x );
float2 __builtin_spirv_OpenCL_sign_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sign_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sign_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sign_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sign_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sign_f64(double x );
double2 __builtin_spirv_OpenCL_sign_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sign_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sign_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sign_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sign_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sign_f16(half x );
half2 __builtin_spirv_OpenCL_sign_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sign_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sign_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sign_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sign_v16f16(half16 x);

float __builtin_spirv_OpenCL_smoothstep_f32_f32_f32(float edge0, float edge1, float x );
float2 __builtin_spirv_OpenCL_smoothstep_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_smoothstep_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_smoothstep_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_smoothstep_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_smoothstep_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_smoothstep_f64_f64_f64(double edge0, double edge1, double x );
double2 __builtin_spirv_OpenCL_smoothstep_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_smoothstep_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_smoothstep_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_smoothstep_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_smoothstep_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_smoothstep_f16_f16_f16(half edge0, half edge1, half x );
half2 __builtin_spirv_OpenCL_smoothstep_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_smoothstep_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_smoothstep_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_smoothstep_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_smoothstep_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_step_f32_f32(float edge, float x );
float2 __builtin_spirv_OpenCL_step_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_step_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_step_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_step_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_step_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_step_f64_f64(double edge, double x );
double2 __builtin_spirv_OpenCL_step_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_step_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_step_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_step_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_step_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_step_f16_f16(half edge, half x );
half2 __builtin_spirv_OpenCL_step_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_step_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_step_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_step_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_step_v16f16_v16f16(half16 x, half16 y);


//
//  Geometric
//        -cross,distance,fast_distance,fast_length,fast_normalize,length,normalize
//

float3 __builtin_spirv_OpenCL_cross_v3f32_v3f32(float3 p0, float3 p1 );
float4 __builtin_spirv_OpenCL_cross_v4f32_v4f32(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double3 __builtin_spirv_OpenCL_cross_v3f64_v3f64(double3 p0, double3 p1 );
double4 __builtin_spirv_OpenCL_cross_v4f64_v4f64(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half3 __builtin_spirv_OpenCL_cross_v3f16_v3f16(half3 p0, half3 p1 );
half4 __builtin_spirv_OpenCL_cross_v4f16_v4f16(half4 p0, half4 p1 );

float __builtin_spirv_OpenCL_distance_f32_f32(float p0, float p1 );
float __builtin_spirv_OpenCL_distance_v2f32_v2f32(float2 p0, float2 p1 );
float __builtin_spirv_OpenCL_distance_v3f32_v3f32(float3 p0, float3 p1 );
float __builtin_spirv_OpenCL_distance_v4f32_v4f32(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_distance_f64_f64(double p0, double p1 );
double __builtin_spirv_OpenCL_distance_v2f64_v2f64(double2 p0, double2 p1 );
double __builtin_spirv_OpenCL_distance_v3f64_v3f64(double3 p0, double3 p1 );
double __builtin_spirv_OpenCL_distance_v4f64_v4f64(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_distance_f16_f16(half p0, half p1 );
half __builtin_spirv_OpenCL_distance_v2f16_v2f16(half2 p0, half2 p1 );
half __builtin_spirv_OpenCL_distance_v3f16_v3f16(half3 p0, half3 p1 );
half __builtin_spirv_OpenCL_distance_v4f16_v4f16(half4 p0, half4 p1 );

float __builtin_spirv_OpenCL_fast_distance_f32_f32(float p0, float p1 );
float __builtin_spirv_OpenCL_fast_distance_v2f32_v2f32(float2 p0, float2 p1 );
float __builtin_spirv_OpenCL_fast_distance_v3f32_v3f32(float3 p0, float3 p1 );
float __builtin_spirv_OpenCL_fast_distance_v4f32_v4f32(float4 p0, float4 p1 );

float __builtin_spirv_OpenCL_fast_length_f32(float p );
float __builtin_spirv_OpenCL_fast_length_v2f32(float2 p );
float __builtin_spirv_OpenCL_fast_length_v3f32(float3 p );
float __builtin_spirv_OpenCL_fast_length_v4f32(float4 p );

float __builtin_spirv_OpenCL_fast_normalize_f32(float p );
float2 __builtin_spirv_OpenCL_fast_normalize_v2f32(float2 p );
float3 __builtin_spirv_OpenCL_fast_normalize_v3f32(float3 p );
float4 __builtin_spirv_OpenCL_fast_normalize_v4f32(float4 p );

float __builtin_spirv_OpenCL_length_f32(float p );
float __builtin_spirv_OpenCL_length_v2f32(float2 p );
float __builtin_spirv_OpenCL_length_v3f32(float3 p );
float __builtin_spirv_OpenCL_length_v4f32(float4 p );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_length_f64(double p );
double __builtin_spirv_OpenCL_length_v2f64(double2 p );
double __builtin_spirv_OpenCL_length_v3f64(double3 p );
double __builtin_spirv_OpenCL_length_v4f64(double4 p );
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
half __builtin_spirv_OpenCL_length_f16(half p );
half __builtin_spirv_OpenCL_length_v2f16(half2 p );
half __builtin_spirv_OpenCL_length_v3f16(half3 p );
half __builtin_spirv_OpenCL_length_v4f16(half4 p );
#endif // defined(cl_khr_fp16)

float __builtin_spirv_OpenCL_normalize_f32(float p );
float2 __builtin_spirv_OpenCL_normalize_v2f32(float2 p );
float3 __builtin_spirv_OpenCL_normalize_v3f32(float3 p );
float4 __builtin_spirv_OpenCL_normalize_v4f32(float4 p );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_normalize_f64(double p );
double2 __builtin_spirv_OpenCL_normalize_v2f64(double2 p );
double3 __builtin_spirv_OpenCL_normalize_v3f64(double3 p );
double4 __builtin_spirv_OpenCL_normalize_v4f64(double4 p );
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_normalize_f16(half p );
half2 __builtin_spirv_OpenCL_normalize_v2f16(half2 p );
half3 __builtin_spirv_OpenCL_normalize_v3f16(half3 p );
half4 __builtin_spirv_OpenCL_normalize_v4f16(half4 p );


//
//  Half
//        -half_cos,half_divide,half_exp,half_exp2,half_exp10,half_log,half_log2,half_log10
//         half_powr,half_recip,half_rsqrt,half_sin,half_sqrt,half_tan
//

float __builtin_spirv_OpenCL_half_cos_f32(float x );
float2 __builtin_spirv_OpenCL_half_cos_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_cos_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_cos_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_cos_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_cos_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_divide_f32_f32(float x, float y );
float2 __builtin_spirv_OpenCL_half_divide_v2f32_v2f32(float2 x, float2 y );
float3 __builtin_spirv_OpenCL_half_divide_v3f32_v3f32(float3 x, float3 y );
float4 __builtin_spirv_OpenCL_half_divide_v4f32_v4f32(float4 x, float4 y );
float8 __builtin_spirv_OpenCL_half_divide_v8f32_v8f32(float8 x, float8 y );
float16 __builtin_spirv_OpenCL_half_divide_v16f32_v16f32(float16 x, float16 y );

float __builtin_spirv_OpenCL_half_exp_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_exp2_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp2_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp2_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp2_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp2_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp2_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_exp10_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp10_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp10_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp10_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp10_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp10_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log_f32(float x );
float2 __builtin_spirv_OpenCL_half_log_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log2_f32(float x );
float2 __builtin_spirv_OpenCL_half_log2_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log2_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log2_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log2_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log2_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log10_f32(float x );
float2 __builtin_spirv_OpenCL_half_log10_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log10_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log10_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log10_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log10_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_powr_f32_f32(float x, float y );
float2 __builtin_spirv_OpenCL_half_powr_v2f32_v2f32(float2 x, float2 y );
float3 __builtin_spirv_OpenCL_half_powr_v3f32_v3f32(float3 x, float3 y );
float4 __builtin_spirv_OpenCL_half_powr_v4f32_v4f32(float4 x, float4 y );
float8 __builtin_spirv_OpenCL_half_powr_v8f32_v8f32(float8 x, float8 y );
float16 __builtin_spirv_OpenCL_half_powr_v16f32_v16f32(float16 x, float16 y );

float __builtin_spirv_OpenCL_half_recip_f32(float x );
float2 __builtin_spirv_OpenCL_half_recip_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_recip_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_recip_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_recip_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_recip_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_rsqrt_f32(float x );
float2 __builtin_spirv_OpenCL_half_rsqrt_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_rsqrt_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_rsqrt_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_rsqrt_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_rsqrt_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_sin_f32(float x );
float2 __builtin_spirv_OpenCL_half_sin_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_sin_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_sin_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_sin_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_sin_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_sqrt_f32(float x );
float2 __builtin_spirv_OpenCL_half_sqrt_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_sqrt_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_sqrt_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_sqrt_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_sqrt_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_tan_f32(float x );
float2 __builtin_spirv_OpenCL_half_tan_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_tan_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_tan_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_tan_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_tan_v16f32(float16 x );


//
//  Integer (signed and unsigned )
//        -abs,abs_diff,add_sat,clamp,clz,ctz,hadd,mad_hi,mad_sat,mad24,max,min,mul_hi
//         mul24,popcnt,rhadd,rotate,sub_sat,upsample
//

uchar __builtin_spirv_OpenCL_s_abs_i8( char x );
uchar2 __builtin_spirv_OpenCL_s_abs_v2i8( char2 x );
uchar3 __builtin_spirv_OpenCL_s_abs_v3i8( char3 x );
uchar4 __builtin_spirv_OpenCL_s_abs_v4i8( char4 x );
uchar8 __builtin_spirv_OpenCL_s_abs_v8i8( char8 x );
uchar16 __builtin_spirv_OpenCL_s_abs_v16i8( char16 x );
uchar __builtin_spirv_OpenCL_u_abs_i8( uchar x );
uchar2 __builtin_spirv_OpenCL_u_abs_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_u_abs_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_u_abs_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_u_abs_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_u_abs_v16i8( uchar16 x );
ushort __builtin_spirv_OpenCL_s_abs_i16( short x );
ushort2 __builtin_spirv_OpenCL_s_abs_v2i16( short2 x );
ushort3 __builtin_spirv_OpenCL_s_abs_v3i16( short3 x );
ushort4 __builtin_spirv_OpenCL_s_abs_v4i16( short4 x );
ushort8 __builtin_spirv_OpenCL_s_abs_v8i16( short8 x );
ushort16 __builtin_spirv_OpenCL_s_abs_v16i16( short16 x );
ushort __builtin_spirv_OpenCL_u_abs_i16( ushort x );
ushort2 __builtin_spirv_OpenCL_u_abs_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_u_abs_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_u_abs_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_u_abs_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_u_abs_v16i16( ushort16 x );
uint __builtin_spirv_OpenCL_s_abs_i32( int x );
uint2 __builtin_spirv_OpenCL_s_abs_v2i32( int2 x );
uint3 __builtin_spirv_OpenCL_s_abs_v3i32( int3 x );
uint4 __builtin_spirv_OpenCL_s_abs_v4i32( int4 x );
uint8 __builtin_spirv_OpenCL_s_abs_v8i32( int8 x );
uint16 __builtin_spirv_OpenCL_s_abs_v16i32( int16 x );
uint __builtin_spirv_OpenCL_u_abs_i32( uint x );
uint2 __builtin_spirv_OpenCL_u_abs_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_u_abs_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_u_abs_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_u_abs_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_u_abs_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_s_abs_i64( long x );
ulong2 __builtin_spirv_OpenCL_s_abs_v2i64( long2 x );
ulong3 __builtin_spirv_OpenCL_s_abs_v3i64( long3 x );
ulong4 __builtin_spirv_OpenCL_s_abs_v4i64( long4 x );
ulong8 __builtin_spirv_OpenCL_s_abs_v8i64( long8 x );
ulong16 __builtin_spirv_OpenCL_s_abs_v16i64( long16 x );
ulong __builtin_spirv_OpenCL_u_abs_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_u_abs_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_u_abs_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_u_abs_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_u_abs_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_u_abs_v16i64( ulong16 x );

uchar __builtin_spirv_OpenCL_s_abs_diff_i8_i8( char x, char y );
uchar2 __builtin_spirv_OpenCL_s_abs_diff_v2i8_v2i8( char2 x, char2 y );
uchar3 __builtin_spirv_OpenCL_s_abs_diff_v3i8_v3i8( char3 x, char3 y );
uchar4 __builtin_spirv_OpenCL_s_abs_diff_v4i8_v4i8( char4 x, char4 y );
uchar8 __builtin_spirv_OpenCL_s_abs_diff_v8i8_v8i8( char8 x, char8 y );
uchar16 __builtin_spirv_OpenCL_s_abs_diff_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_abs_diff_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_abs_diff_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_abs_diff_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_abs_diff_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_abs_diff_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_abs_diff_v16i8_v16i8( uchar16 x, uchar16 y );
ushort __builtin_spirv_OpenCL_s_abs_diff_i16_i16( short x, short y );
ushort2 __builtin_spirv_OpenCL_s_abs_diff_v2i16_v2i16( short2 x, short2 y );
ushort3 __builtin_spirv_OpenCL_s_abs_diff_v3i16_v3i16( short3 x, short3 y );
ushort4 __builtin_spirv_OpenCL_s_abs_diff_v4i16_v4i16( short4 x, short4 y );
ushort8 __builtin_spirv_OpenCL_s_abs_diff_v8i16_v8i16( short8 x, short8 y );
ushort16 __builtin_spirv_OpenCL_s_abs_diff_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_abs_diff_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_abs_diff_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_abs_diff_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_abs_diff_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_abs_diff_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_abs_diff_v16i16_v16i16( ushort16 x, ushort16 y );
uint __builtin_spirv_OpenCL_s_abs_diff_i32_i32( int x, int y );
uint2 __builtin_spirv_OpenCL_s_abs_diff_v2i32_v2i32( int2 x, int2 y );
uint3 __builtin_spirv_OpenCL_s_abs_diff_v3i32_v3i32( int3 x, int3 y );
uint4 __builtin_spirv_OpenCL_s_abs_diff_v4i32_v4i32( int4 x, int4 y );
uint8 __builtin_spirv_OpenCL_s_abs_diff_v8i32_v8i32( int8 x, int8 y );
uint16 __builtin_spirv_OpenCL_s_abs_diff_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_abs_diff_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_abs_diff_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_abs_diff_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_abs_diff_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_abs_diff_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_abs_diff_v16i32_v16i32( uint16 x, uint16 y );
ulong __builtin_spirv_OpenCL_s_abs_diff_i64_i64( long x, long y );
ulong2 __builtin_spirv_OpenCL_s_abs_diff_v2i64_v2i64( long2 x, long2 y );
ulong3 __builtin_spirv_OpenCL_s_abs_diff_v3i64_v3i64( long3 x, long3 y );
ulong4 __builtin_spirv_OpenCL_s_abs_diff_v4i64_v4i64( long4 x, long4 y );
ulong8 __builtin_spirv_OpenCL_s_abs_diff_v8i64_v8i64( long8 x, long8 y );
ulong16 __builtin_spirv_OpenCL_s_abs_diff_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_abs_diff_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_abs_diff_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_abs_diff_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_abs_diff_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_abs_diff_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_abs_diff_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_add_sat_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_add_sat_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_add_sat_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_add_sat_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_add_sat_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_add_sat_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_add_sat_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_add_sat_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_add_sat_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_add_sat_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_add_sat_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_add_sat_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_add_sat_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_add_sat_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_add_sat_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_add_sat_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_add_sat_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_add_sat_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_add_sat_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_add_sat_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_add_sat_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_add_sat_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_add_sat_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_add_sat_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_add_sat_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_add_sat_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_add_sat_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_add_sat_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_add_sat_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_add_sat_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_add_sat_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_add_sat_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_add_sat_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_add_sat_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_add_sat_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_add_sat_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_add_sat_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_add_sat_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_add_sat_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_add_sat_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_add_sat_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_add_sat_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_add_sat_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_add_sat_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_add_sat_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_add_sat_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_add_sat_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_add_sat_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_clamp_i8_i8_i8(char x, char minval, char maxval );
uchar __builtin_spirv_OpenCL_u_clamp_i8_i8_i8(uchar x, uchar minval, uchar maxval );
short __builtin_spirv_OpenCL_s_clamp_i16_i16_i16(short x, short minval, short maxval );
ushort __builtin_spirv_OpenCL_u_clamp_i16_i16_i16(ushort x, ushort minval, ushort maxval );
int __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(int x, int minval, int maxval );
uint __builtin_spirv_OpenCL_u_clamp_i32_i32_i32(uint x, uint minval, uint maxval );
long __builtin_spirv_OpenCL_s_clamp_i64_i64_i64(long x, long minval, long maxval );
ulong __builtin_spirv_OpenCL_u_clamp_i64_i64_i64(ulong x, ulong minval, ulong maxval );
char2 __builtin_spirv_OpenCL_s_clamp_v2i8_v2i8_v2i8(char2 x, char2 y, char2 z);
char3 __builtin_spirv_OpenCL_s_clamp_v3i8_v3i8_v3i8(char3 x, char3 y, char3 z);
char4 __builtin_spirv_OpenCL_s_clamp_v4i8_v4i8_v4i8(char4 x, char4 y, char4 z);
char8 __builtin_spirv_OpenCL_s_clamp_v8i8_v8i8_v8i8(char8 x, char8 y, char8 z);
char16 __builtin_spirv_OpenCL_s_clamp_v16i8_v16i8_v16i8(char16 x, char16 y, char16 z);
uchar2 __builtin_spirv_OpenCL_u_clamp_v2i8_v2i8_v2i8(uchar2 x, uchar2 y, uchar2 z);
uchar3 __builtin_spirv_OpenCL_u_clamp_v3i8_v3i8_v3i8(uchar3 x, uchar3 y, uchar3 z);
uchar4 __builtin_spirv_OpenCL_u_clamp_v4i8_v4i8_v4i8(uchar4 x, uchar4 y, uchar4 z);
uchar8 __builtin_spirv_OpenCL_u_clamp_v8i8_v8i8_v8i8(uchar8 x, uchar8 y, uchar8 z);
uchar16 __builtin_spirv_OpenCL_u_clamp_v16i8_v16i8_v16i8(uchar16 x, uchar16 y, uchar16 z);
short2 __builtin_spirv_OpenCL_s_clamp_v2i16_v2i16_v2i16(short2 x, short2 y, short2 z);
short3 __builtin_spirv_OpenCL_s_clamp_v3i16_v3i16_v3i16(short3 x, short3 y, short3 z);
short4 __builtin_spirv_OpenCL_s_clamp_v4i16_v4i16_v4i16(short4 x, short4 y, short4 z);
short8 __builtin_spirv_OpenCL_s_clamp_v8i16_v8i16_v8i16(short8 x, short8 y, short8 z);
short16 __builtin_spirv_OpenCL_s_clamp_v16i16_v16i16_v16i16(short16 x, short16 y, short16 z);
ushort2 __builtin_spirv_OpenCL_u_clamp_v2i16_v2i16_v2i16(ushort2 x, ushort2 y, ushort2 z);
ushort3 __builtin_spirv_OpenCL_u_clamp_v3i16_v3i16_v3i16(ushort3 x, ushort3 y, ushort3 z);
ushort4 __builtin_spirv_OpenCL_u_clamp_v4i16_v4i16_v4i16(ushort4 x, ushort4 y, ushort4 z);
ushort8 __builtin_spirv_OpenCL_u_clamp_v8i16_v8i16_v8i16(ushort8 x, ushort8 y, ushort8 z);
ushort16 __builtin_spirv_OpenCL_u_clamp_v16i16_v16i16_v16i16(ushort16 x, ushort16 y, ushort16 z);
int2 __builtin_spirv_OpenCL_s_clamp_v2i32_v2i32_v2i32(int2 x, int2 y, int2 z);
int3 __builtin_spirv_OpenCL_s_clamp_v3i32_v3i32_v3i32(int3 x, int3 y, int3 z);
int4 __builtin_spirv_OpenCL_s_clamp_v4i32_v4i32_v4i32(int4 x, int4 y, int4 z);
int8 __builtin_spirv_OpenCL_s_clamp_v8i32_v8i32_v8i32(int8 x, int8 y, int8 z);
int16 __builtin_spirv_OpenCL_s_clamp_v16i32_v16i32_v16i32(int16 x, int16 y, int16 z);
uint2 __builtin_spirv_OpenCL_u_clamp_v2i32_v2i32_v2i32(uint2 x, uint2 y, uint2 z);
uint3 __builtin_spirv_OpenCL_u_clamp_v3i32_v3i32_v3i32(uint3 x, uint3 y, uint3 z);
uint4 __builtin_spirv_OpenCL_u_clamp_v4i32_v4i32_v4i32(uint4 x, uint4 y, uint4 z);
uint8 __builtin_spirv_OpenCL_u_clamp_v8i32_v8i32_v8i32(uint8 x, uint8 y, uint8 z);
uint16 __builtin_spirv_OpenCL_u_clamp_v16i32_v16i32_v16i32(uint16 x, uint16 y, uint16 z);
long2 __builtin_spirv_OpenCL_s_clamp_v2i64_v2i64_v2i64(long2 x, long2 y, long2 z);
long3 __builtin_spirv_OpenCL_s_clamp_v3i64_v3i64_v3i64(long3 x, long3 y, long3 z);
long4 __builtin_spirv_OpenCL_s_clamp_v4i64_v4i64_v4i64(long4 x, long4 y, long4 z);
long8 __builtin_spirv_OpenCL_s_clamp_v8i64_v8i64_v8i64(long8 x, long8 y, long8 z);
long16 __builtin_spirv_OpenCL_s_clamp_v16i64_v16i64_v16i64(long16 x, long16 y, long16 z);
ulong2 __builtin_spirv_OpenCL_u_clamp_v2i64_v2i64_v2i64(ulong2 x, ulong2 y, ulong2 z);
ulong3 __builtin_spirv_OpenCL_u_clamp_v3i64_v3i64_v3i64(ulong3 x, ulong3 y, ulong3 z);
ulong4 __builtin_spirv_OpenCL_u_clamp_v4i64_v4i64_v4i64(ulong4 x, ulong4 y, ulong4 z);
ulong8 __builtin_spirv_OpenCL_u_clamp_v8i64_v8i64_v8i64(ulong8 x, ulong8 y, ulong8 z);
ulong16 __builtin_spirv_OpenCL_u_clamp_v16i64_v16i64_v16i64(ulong16 x, ulong16 y, ulong16 z);

uchar __builtin_spirv_OpenCL_clz_i8( uchar x );
ushort __builtin_spirv_OpenCL_clz_i16( ushort x );
uint __builtin_spirv_OpenCL_clz_i32( uint x );
uchar2 __builtin_spirv_OpenCL_clz_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_clz_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_clz_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_clz_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_clz_v16i8( uchar16 x );
ushort2 __builtin_spirv_OpenCL_clz_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_clz_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_clz_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_clz_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_clz_v16i16( ushort16 x );
uint2 __builtin_spirv_OpenCL_clz_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_clz_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_clz_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_clz_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_clz_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_clz_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_clz_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_clz_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_clz_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_clz_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_clz_v16i64( ulong16 x );

uchar __builtin_spirv_OpenCL_ctz_i8(uchar x );
uchar2 __builtin_spirv_OpenCL_ctz_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_ctz_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_ctz_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_ctz_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_ctz_v16i8( uchar16 x );
ushort __builtin_spirv_OpenCL_ctz_i16(ushort x );
ushort2 __builtin_spirv_OpenCL_ctz_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_ctz_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_ctz_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_ctz_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_ctz_v16i16( ushort16 x );
uint __builtin_spirv_OpenCL_ctz_i32(uint x );
uint2 __builtin_spirv_OpenCL_ctz_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_ctz_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_ctz_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_ctz_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_ctz_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_ctz_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_ctz_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_ctz_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_ctz_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_ctz_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_ctz_v16i64( ulong16 x );

char __builtin_spirv_OpenCL_s_hadd_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_hadd_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_hadd_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_hadd_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_hadd_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_hadd_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_hadd_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_hadd_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_hadd_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_hadd_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_hadd_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_hadd_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_hadd_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_hadd_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_hadd_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_hadd_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_hadd_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_hadd_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_hadd_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_hadd_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_hadd_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_hadd_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_hadd_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_hadd_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_hadd_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_hadd_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_hadd_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_hadd_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_hadd_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_hadd_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_hadd_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_hadd_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_hadd_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_hadd_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_hadd_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_hadd_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_hadd_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_hadd_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_hadd_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_hadd_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_hadd_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_hadd_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_hadd_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_hadd_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_hadd_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_hadd_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_hadd_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_hadd_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_mad_hi_i8_i8_i8( char a, char b, char c );
char2 __builtin_spirv_OpenCL_s_mad_hi_v2i8_v2i8_v2i8( char2 a, char2 b, char2 c );
char3 __builtin_spirv_OpenCL_s_mad_hi_v3i8_v3i8_v3i8( char3 a, char3 b, char3 c );
char4 __builtin_spirv_OpenCL_s_mad_hi_v4i8_v4i8_v4i8( char4 a, char4 b, char4 c );
char8 __builtin_spirv_OpenCL_s_mad_hi_v8i8_v8i8_v8i8( char8 a, char8 b, char8 c );
char16 __builtin_spirv_OpenCL_s_mad_hi_v16i8_v16i8_v16i8( char16 a, char16 b, char16 c );
uchar __builtin_spirv_OpenCL_u_mad_hi_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_u_mad_hi_v2i8_v2i8_v2i8( uchar2 a, uchar2 b, uchar2 c );
uchar3 __builtin_spirv_OpenCL_u_mad_hi_v3i8_v3i8_v3i8( uchar3 a, uchar3 b, uchar3 c );
uchar4 __builtin_spirv_OpenCL_u_mad_hi_v4i8_v4i8_v4i8( uchar4 a, uchar4 b, uchar4 c );
uchar8 __builtin_spirv_OpenCL_u_mad_hi_v8i8_v8i8_v8i8( uchar8 a, uchar8 b, uchar8 c );
uchar16 __builtin_spirv_OpenCL_u_mad_hi_v16i8_v16i8_v16i8( uchar16 a, uchar16 b, uchar16 c );
short __builtin_spirv_OpenCL_s_mad_hi_i16_i16_i16( short a, short b, short c );
short2 __builtin_spirv_OpenCL_s_mad_hi_v2i16_v2i16_v2i16( short2 a, short2 b, short2 c );
short3 __builtin_spirv_OpenCL_s_mad_hi_v3i16_v3i16_v3i16( short3 a, short3 b, short3 c );
short4 __builtin_spirv_OpenCL_s_mad_hi_v4i16_v4i16_v4i16( short4 a, short4 b, short4 c );
short8 __builtin_spirv_OpenCL_s_mad_hi_v8i16_v8i16_v8i16( short8 a, short8 b, short8 c );
short16 __builtin_spirv_OpenCL_s_mad_hi_v16i16_v16i16_v16i16( short16 a, short16 b, short16 c );
ushort __builtin_spirv_OpenCL_u_mad_hi_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_u_mad_hi_v2i16_v2i16_v2i16( ushort2 a, ushort2 b, ushort2 c );
ushort3 __builtin_spirv_OpenCL_u_mad_hi_v3i16_v3i16_v3i16( ushort3 a, ushort3 b, ushort3 c );
ushort4 __builtin_spirv_OpenCL_u_mad_hi_v4i16_v4i16_v4i16( ushort4 a, ushort4 b, ushort4 c );
ushort8 __builtin_spirv_OpenCL_u_mad_hi_v8i16_v8i16_v8i16( ushort8 a, ushort8 b, ushort8 c );
ushort16 __builtin_spirv_OpenCL_u_mad_hi_v16i16_v16i16_v16i16( ushort16 a, ushort16 b, ushort16 c );
int __builtin_spirv_OpenCL_s_mad_hi_i32_i32_i32( int a, int b, int c );
int2 __builtin_spirv_OpenCL_s_mad_hi_v2i32_v2i32_v2i32( int2 a, int2 b, int2 c );
int3 __builtin_spirv_OpenCL_s_mad_hi_v3i32_v3i32_v3i32( int3 a, int3 b, int3 c );
int4 __builtin_spirv_OpenCL_s_mad_hi_v4i32_v4i32_v4i32( int4 a, int4 b, int4 c );
int8 __builtin_spirv_OpenCL_s_mad_hi_v8i32_v8i32_v8i32( int8 a, int8 b, int8 c );
int16 __builtin_spirv_OpenCL_s_mad_hi_v16i32_v16i32_v16i32( int16 a, int16 b, int16 c );
uint __builtin_spirv_OpenCL_u_mad_hi_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_u_mad_hi_v2i32_v2i32_v2i32( uint2 a, uint2 b, uint2 c );
uint3 __builtin_spirv_OpenCL_u_mad_hi_v3i32_v3i32_v3i32( uint3 a, uint3 b, uint3 c );
uint4 __builtin_spirv_OpenCL_u_mad_hi_v4i32_v4i32_v4i32( uint4 a, uint4 b, uint4 c );
uint8 __builtin_spirv_OpenCL_u_mad_hi_v8i32_v8i32_v8i32( uint8 a, uint8 b, uint8 c );
uint16 __builtin_spirv_OpenCL_u_mad_hi_v16i32_v16i32_v16i32( uint16 a, uint16 b, uint16 c );
long __builtin_spirv_OpenCL_s_mad_hi_i64_i64_i64( long a, long b, long c );
long2 __builtin_spirv_OpenCL_s_mad_hi_v2i64_v2i64_v2i64( long2 a, long2 b, long2 c );
long3 __builtin_spirv_OpenCL_s_mad_hi_v3i64_v3i64_v3i64( long3 a, long3 b, long3 c );
long4 __builtin_spirv_OpenCL_s_mad_hi_v4i64_v4i64_v4i64( long4 a, long4 b, long4 c );
long8 __builtin_spirv_OpenCL_s_mad_hi_v8i64_v8i64_v8i64( long8 a, long8 b, long8 c );
long16 __builtin_spirv_OpenCL_s_mad_hi_v16i64_v16i64_v16i64( long16 a, long16 b, long16 c );
ulong __builtin_spirv_OpenCL_u_mad_hi_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_u_mad_hi_v2i64_v2i64_v2i64( ulong2 a, ulong2 b, ulong2 c );
ulong3 __builtin_spirv_OpenCL_u_mad_hi_v3i64_v3i64_v3i64( ulong3 a, ulong3 b, ulong3 c );
ulong4 __builtin_spirv_OpenCL_u_mad_hi_v4i64_v4i64_v4i64( ulong4 a, ulong4 b, ulong4 c );
ulong8 __builtin_spirv_OpenCL_u_mad_hi_v8i64_v8i64_v8i64( ulong8 a, ulong8 b, ulong8 c );
ulong16 __builtin_spirv_OpenCL_u_mad_hi_v16i64_v16i64_v16i64( ulong16 a, ulong16 b, ulong16 c );

char __builtin_spirv_OpenCL_s_mad_sat_i8_i8_i8( char a, char b, char c );
char2 __builtin_spirv_OpenCL_s_mad_sat_v2i8_v2i8_v2i8( char2 a, char2 b, char2 c );
char3 __builtin_spirv_OpenCL_s_mad_sat_v3i8_v3i8_v3i8( char3 a, char3 b, char3 c );
char4 __builtin_spirv_OpenCL_s_mad_sat_v4i8_v4i8_v4i8( char4 a, char4 b, char4 c );
char8 __builtin_spirv_OpenCL_s_mad_sat_v8i8_v8i8_v8i8( char8 a, char8 b, char8 c );
char16 __builtin_spirv_OpenCL_s_mad_sat_v16i8_v16i8_v16i8( char16 a, char16 b, char16 c );
uchar __builtin_spirv_OpenCL_u_mad_sat_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_u_mad_sat_v2i8_v2i8_v2i8( uchar2 a, uchar2 b, uchar2 c );
uchar3 __builtin_spirv_OpenCL_u_mad_sat_v3i8_v3i8_v3i8( uchar3 a, uchar3 b, uchar3 c );
uchar4 __builtin_spirv_OpenCL_u_mad_sat_v4i8_v4i8_v4i8( uchar4 a, uchar4 b, uchar4 c );
uchar8 __builtin_spirv_OpenCL_u_mad_sat_v8i8_v8i8_v8i8( uchar8 a, uchar8 b, uchar8 c );
uchar16 __builtin_spirv_OpenCL_u_mad_sat_v16i8_v16i8_v16i8( uchar16 a, uchar16 b, uchar16 c );
short __builtin_spirv_OpenCL_s_mad_sat_i16_i16_i16( short a, short b, short c );
short2 __builtin_spirv_OpenCL_s_mad_sat_v2i16_v2i16_v2i16( short2 a, short2 b, short2 c );
short3 __builtin_spirv_OpenCL_s_mad_sat_v3i16_v3i16_v3i16( short3 a, short3 b, short3 c );
short4 __builtin_spirv_OpenCL_s_mad_sat_v4i16_v4i16_v4i16( short4 a, short4 b, short4 c );
short8 __builtin_spirv_OpenCL_s_mad_sat_v8i16_v8i16_v8i16( short8 a, short8 b, short8 c );
short16 __builtin_spirv_OpenCL_s_mad_sat_v16i16_v16i16_v16i16( short16 a, short16 b, short16 c );
ushort __builtin_spirv_OpenCL_u_mad_sat_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_u_mad_sat_v2i16_v2i16_v2i16( ushort2 a, ushort2 b, ushort2 c );
ushort3 __builtin_spirv_OpenCL_u_mad_sat_v3i16_v3i16_v3i16( ushort3 a, ushort3 b, ushort3 c );
ushort4 __builtin_spirv_OpenCL_u_mad_sat_v4i16_v4i16_v4i16( ushort4 a, ushort4 b, ushort4 c );
ushort8 __builtin_spirv_OpenCL_u_mad_sat_v8i16_v8i16_v8i16( ushort8 a, ushort8 b, ushort8 c );
ushort16 __builtin_spirv_OpenCL_u_mad_sat_v16i16_v16i16_v16i16( ushort16 a, ushort16 b, ushort16 c );
int __builtin_spirv_OpenCL_s_mad_sat_i32_i32_i32( int a, int b, int c );
int2 __builtin_spirv_OpenCL_s_mad_sat_v2i32_v2i32_v2i32( int2 a, int2 b, int2 c );
int3 __builtin_spirv_OpenCL_s_mad_sat_v3i32_v3i32_v3i32( int3 a, int3 b, int3 c );
int4 __builtin_spirv_OpenCL_s_mad_sat_v4i32_v4i32_v4i32( int4 a, int4 b, int4 c );
int8 __builtin_spirv_OpenCL_s_mad_sat_v8i32_v8i32_v8i32( int8 a, int8 b, int8 c );
int16 __builtin_spirv_OpenCL_s_mad_sat_v16i32_v16i32_v16i32( int16 a, int16 b, int16 c );
uint __builtin_spirv_OpenCL_u_mad_sat_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_u_mad_sat_v2i32_v2i32_v2i32( uint2 a, uint2 b, uint2 c );
uint3 __builtin_spirv_OpenCL_u_mad_sat_v3i32_v3i32_v3i32( uint3 a, uint3 b, uint3 c );
uint4 __builtin_spirv_OpenCL_u_mad_sat_v4i32_v4i32_v4i32( uint4 a, uint4 b, uint4 c );
uint8 __builtin_spirv_OpenCL_u_mad_sat_v8i32_v8i32_v8i32( uint8 a, uint8 b, uint8 c );
uint16 __builtin_spirv_OpenCL_u_mad_sat_v16i32_v16i32_v16i32( uint16 a, uint16 b, uint16 c );
long __builtin_spirv_OpenCL_s_mad_sat_i64_i64_i64( long a, long b, long c );
long2 __builtin_spirv_OpenCL_s_mad_sat_v2i64_v2i64_v2i64( long2 a, long2 b, long2 c );
long3 __builtin_spirv_OpenCL_s_mad_sat_v3i64_v3i64_v3i64( long3 a, long3 b, long3 c );
long4 __builtin_spirv_OpenCL_s_mad_sat_v4i64_v4i64_v4i64( long4 a, long4 b, long4 c );
long8 __builtin_spirv_OpenCL_s_mad_sat_v8i64_v8i64_v8i64( long8 a, long8 b, long8 c );
long16 __builtin_spirv_OpenCL_s_mad_sat_v16i64_v16i64_v16i64( long16 a, long16 b, long16 c );
ulong __builtin_spirv_OpenCL_u_mad_sat_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_u_mad_sat_v2i64_v2i64_v2i64( ulong2 a, ulong2 b, ulong2 c );
ulong3 __builtin_spirv_OpenCL_u_mad_sat_v3i64_v3i64_v3i64( ulong3 a, ulong3 b, ulong3 c );
ulong4 __builtin_spirv_OpenCL_u_mad_sat_v4i64_v4i64_v4i64( ulong4 a, ulong4 b, ulong4 c );
ulong8 __builtin_spirv_OpenCL_u_mad_sat_v8i64_v8i64_v8i64( ulong8 a, ulong8 b, ulong8 c );
ulong16 __builtin_spirv_OpenCL_u_mad_sat_v16i64_v16i64_v16i64( ulong16 a, ulong16 b, ulong16 c );

int __builtin_spirv_OpenCL_s_mad24_i32_i32_i32( int x, int y, int z );
int2 __builtin_spirv_OpenCL_s_mad24_v2i32_v2i32_v2i32( int2 x, int2 y, int2 z );
int3 __builtin_spirv_OpenCL_s_mad24_v3i32_v3i32_v3i32( int3 x, int3 y, int3 z );
int4 __builtin_spirv_OpenCL_s_mad24_v4i32_v4i32_v4i32( int4 x, int4 y, int4 z );
int8 __builtin_spirv_OpenCL_s_mad24_v8i32_v8i32_v8i32( int8 x, int8 y, int8 z );
int16 __builtin_spirv_OpenCL_s_mad24_v16i32_v16i32_v16i32( int16 x, int16 y, int16 z );
uint __builtin_spirv_OpenCL_u_mad24_i32_i32_i32( uint x, uint y, uint z );
uint2 __builtin_spirv_OpenCL_u_mad24_v2i32_v2i32_v2i32( uint2 x, uint2 y, uint2 z );
uint3 __builtin_spirv_OpenCL_u_mad24_v3i32_v3i32_v3i32( uint3 x, uint3 y, uint3 z );
uint4 __builtin_spirv_OpenCL_u_mad24_v4i32_v4i32_v4i32( uint4 x, uint4 y, uint4 z );
uint8 __builtin_spirv_OpenCL_u_mad24_v8i32_v8i32_v8i32( uint8 x, uint8 y, uint8 z );
uint16 __builtin_spirv_OpenCL_u_mad24_v16i32_v16i32_v16i32( uint16 x, uint16 y, uint16 z );

char __builtin_spirv_OpenCL_s_max_i8_i8(char x, char y);
char2 __builtin_spirv_OpenCL_s_max_v2i8_v2i8(char2 x, char2 y);
char3 __builtin_spirv_OpenCL_s_max_v3i8_v3i8(char3 x, char3 y);
char4 __builtin_spirv_OpenCL_s_max_v4i8_v4i8(char4 x, char4 y);
char8 __builtin_spirv_OpenCL_s_max_v8i8_v8i8(char8 x, char8 y);
char16 __builtin_spirv_OpenCL_s_max_v16i8_v16i8(char16 x, char16 y);
uchar __builtin_spirv_OpenCL_u_max_i8_i8(uchar x, uchar y);
uchar2 __builtin_spirv_OpenCL_u_max_v2i8_v2i8(uchar2 x, uchar2 y);
uchar3 __builtin_spirv_OpenCL_u_max_v3i8_v3i8(uchar3 x, uchar3 y);
uchar4 __builtin_spirv_OpenCL_u_max_v4i8_v4i8(uchar4 x, uchar4 y);
uchar8 __builtin_spirv_OpenCL_u_max_v8i8_v8i8(uchar8 x, uchar8 y);
uchar16 __builtin_spirv_OpenCL_u_max_v16i8_v16i8(uchar16 x, uchar16 y);
short __builtin_spirv_OpenCL_s_max_i16_i16(short x, short y);
short2 __builtin_spirv_OpenCL_s_max_v2i16_v2i16(short2 x, short2 y);
short3 __builtin_spirv_OpenCL_s_max_v3i16_v3i16(short3 x, short3 y);
short4 __builtin_spirv_OpenCL_s_max_v4i16_v4i16(short4 x, short4 y);
short8 __builtin_spirv_OpenCL_s_max_v8i16_v8i16(short8 x, short8 y);
short16 __builtin_spirv_OpenCL_s_max_v16i16_v16i16(short16 x, short16 y);
ushort __builtin_spirv_OpenCL_u_max_i16_i16(ushort x, ushort y);
ushort2 __builtin_spirv_OpenCL_u_max_v2i16_v2i16(ushort2 x, ushort2 y);
ushort3 __builtin_spirv_OpenCL_u_max_v3i16_v3i16(ushort3 x, ushort3 y);
ushort4 __builtin_spirv_OpenCL_u_max_v4i16_v4i16(ushort4 x, ushort4 y);
ushort8 __builtin_spirv_OpenCL_u_max_v8i16_v8i16(ushort8 x, ushort8 y);
ushort16 __builtin_spirv_OpenCL_u_max_v16i16_v16i16(ushort16 x, ushort16 y);
int __builtin_spirv_OpenCL_s_max_i32_i32(int x, int y);
int2 __builtin_spirv_OpenCL_s_max_v2i32_v2i32(int2 x, int2 y);
int3 __builtin_spirv_OpenCL_s_max_v3i32_v3i32(int3 x, int3 y);
int4 __builtin_spirv_OpenCL_s_max_v4i32_v4i32(int4 x, int4 y);
int8 __builtin_spirv_OpenCL_s_max_v8i32_v8i32(int8 x, int8 y);
int16 __builtin_spirv_OpenCL_s_max_v16i32_v16i32(int16 x, int16 y);
uint __builtin_spirv_OpenCL_u_max_i32_i32(uint x, uint y);
uint2 __builtin_spirv_OpenCL_u_max_v2i32_v2i32(uint2 x, uint2 y);
uint3 __builtin_spirv_OpenCL_u_max_v3i32_v3i32(uint3 x, uint3 y);
uint4 __builtin_spirv_OpenCL_u_max_v4i32_v4i32(uint4 x, uint4 y);
uint8 __builtin_spirv_OpenCL_u_max_v8i32_v8i32(uint8 x, uint8 y);
uint16 __builtin_spirv_OpenCL_u_max_v16i32_v16i32(uint16 x, uint16 y);
long __builtin_spirv_OpenCL_s_max_i64_i64(long x, long y);
long2 __builtin_spirv_OpenCL_s_max_v2i64_v2i64(long2 x, long2 y);
long3 __builtin_spirv_OpenCL_s_max_v3i64_v3i64(long3 x, long3 y);
long4 __builtin_spirv_OpenCL_s_max_v4i64_v4i64(long4 x, long4 y);
long8 __builtin_spirv_OpenCL_s_max_v8i64_v8i64(long8 x, long8 y);
long16 __builtin_spirv_OpenCL_s_max_v16i64_v16i64(long16 x, long16 y);
ulong __builtin_spirv_OpenCL_u_max_i64_i64(ulong x, ulong y);
ulong2 __builtin_spirv_OpenCL_u_max_v2i64_v2i64(ulong2 x, ulong2 y);
ulong3 __builtin_spirv_OpenCL_u_max_v3i64_v3i64(ulong3 x, ulong3 y);
ulong4 __builtin_spirv_OpenCL_u_max_v4i64_v4i64(ulong4 x, ulong4 y);
ulong8 __builtin_spirv_OpenCL_u_max_v8i64_v8i64(ulong8 x, ulong8 y);
ulong16 __builtin_spirv_OpenCL_u_max_v16i64_v16i64(ulong16 x, ulong16 y);

char __builtin_spirv_OpenCL_s_mul_hi_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_mul_hi_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_mul_hi_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_mul_hi_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_mul_hi_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_mul_hi_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_mul_hi_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_mul_hi_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_mul_hi_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_mul_hi_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_mul_hi_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_mul_hi_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_mul_hi_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_mul_hi_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_mul_hi_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_mul_hi_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_mul_hi_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_mul_hi_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_mul_hi_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_mul_hi_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_mul_hi_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_mul_hi_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_mul_hi_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_mul_hi_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_mul_hi_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_mul_hi_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_mul_hi_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_mul_hi_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_mul_hi_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_mul_hi_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_mul_hi_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_mul_hi_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_mul_hi_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_mul_hi_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_mul_hi_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_mul_hi_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_mul_hi_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_mul_hi_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_mul_hi_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_mul_hi_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_mul_hi_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_mul_hi_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_mul_hi_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_mul_hi_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_mul_hi_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_mul_hi_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_mul_hi_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_mul_hi_v16i64_v16i64( ulong16 x, ulong16 y );

int __builtin_spirv_OpenCL_s_mul24_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_mul24_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_mul24_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_mul24_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_mul24_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_mul24_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_mul24_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_mul24_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_mul24_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_mul24_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_mul24_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_mul24_v16i32_v16i32( uint16 x, uint16 y );

uchar __builtin_spirv_OpenCL_popcount_i8(uchar x );
uchar2 __builtin_spirv_OpenCL_popcount_v2i8(uchar2 x);
uchar3 __builtin_spirv_OpenCL_popcount_v3i8(uchar3 x);
uchar4 __builtin_spirv_OpenCL_popcount_v4i8(uchar4 x);
uchar8 __builtin_spirv_OpenCL_popcount_v8i8(uchar8 x);
uchar16 __builtin_spirv_OpenCL_popcount_v16i8(uchar16 x);
ushort __builtin_spirv_OpenCL_popcount_i16(ushort x );
ushort2 __builtin_spirv_OpenCL_popcount_v2i16(ushort2 x);
ushort3 __builtin_spirv_OpenCL_popcount_v3i16(ushort3 x);
ushort4 __builtin_spirv_OpenCL_popcount_v4i16(ushort4 x);
ushort8 __builtin_spirv_OpenCL_popcount_v8i16(ushort8 x);
ushort16 __builtin_spirv_OpenCL_popcount_v16i16(ushort16 x);
uint __builtin_spirv_OpenCL_popcount_i32(uint x );
uint2 __builtin_spirv_OpenCL_popcount_v2i32(uint2 x);
uint3 __builtin_spirv_OpenCL_popcount_v3i32(uint3 x);
uint4 __builtin_spirv_OpenCL_popcount_v4i32(uint4 x);
uint8 __builtin_spirv_OpenCL_popcount_v8i32(uint8 x);
uint16 __builtin_spirv_OpenCL_popcount_v16i32(uint16 x);
ulong __builtin_spirv_OpenCL_popcount_i64(ulong x);
ulong2 __builtin_spirv_OpenCL_popcount_v2i64(ulong2 x);
ulong3 __builtin_spirv_OpenCL_popcount_v3i64(ulong3 x);
ulong4 __builtin_spirv_OpenCL_popcount_v4i64(ulong4 x);
ulong8 __builtin_spirv_OpenCL_popcount_v8i64(ulong8 x);
ulong16 __builtin_spirv_OpenCL_popcount_v16i64(ulong16 x);

char __builtin_spirv_OpenCL_s_rhadd_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_rhadd_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_rhadd_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_rhadd_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_rhadd_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_rhadd_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_rhadd_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_rhadd_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_rhadd_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_rhadd_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_rhadd_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_rhadd_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_rhadd_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_rhadd_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_rhadd_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_rhadd_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_rhadd_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_rhadd_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_rhadd_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_rhadd_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_rhadd_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_rhadd_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_rhadd_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_rhadd_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_rhadd_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_rhadd_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_rhadd_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_rhadd_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_rhadd_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_rhadd_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_rhadd_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_rhadd_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_rhadd_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_rhadd_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_rhadd_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_rhadd_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_rhadd_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_rhadd_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_rhadd_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_rhadd_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_rhadd_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_rhadd_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_rhadd_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_rhadd_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_rhadd_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_rhadd_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_rhadd_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_rhadd_v16i64_v16i64( ulong16 x, ulong16 y );

uchar __builtin_spirv_OpenCL_rotate_i8_i8( uchar v, uchar i );
uchar2 __builtin_spirv_OpenCL_rotate_v2i8_v2i8( uchar2 v, uchar2 i );
uchar3 __builtin_spirv_OpenCL_rotate_v3i8_v3i8( uchar3 v, uchar3 i );
uchar4 __builtin_spirv_OpenCL_rotate_v4i8_v4i8( uchar4 v, uchar4 i );
uchar8 __builtin_spirv_OpenCL_rotate_v8i8_v8i8( uchar8 v, uchar8 i );
uchar16 __builtin_spirv_OpenCL_rotate_v16i8_v16i8( uchar16 v, uchar16 i );
ushort __builtin_spirv_OpenCL_rotate_i16_i16( ushort v, ushort i );
ushort2 __builtin_spirv_OpenCL_rotate_v2i16_v2i16( ushort2 v, ushort2 i );
ushort3 __builtin_spirv_OpenCL_rotate_v3i16_v3i16( ushort3 v, ushort3 i );
ushort4 __builtin_spirv_OpenCL_rotate_v4i16_v4i16( ushort4 v, ushort4 i );
ushort8 __builtin_spirv_OpenCL_rotate_v8i16_v8i16( ushort8 v, ushort8 i );
ushort16 __builtin_spirv_OpenCL_rotate_v16i16_v16i16( ushort16 v, ushort16 i );
uint __builtin_spirv_OpenCL_rotate_i32_i32( uint v, uint i );
uint2 __builtin_spirv_OpenCL_rotate_v2i32_v2i32( uint2 v, uint2 i );
uint3 __builtin_spirv_OpenCL_rotate_v3i32_v3i32( uint3 v, uint3 i );
uint4 __builtin_spirv_OpenCL_rotate_v4i32_v4i32( uint4 v, uint4 i );
uint8 __builtin_spirv_OpenCL_rotate_v8i32_v8i32( uint8 v, uint8 i );
uint16 __builtin_spirv_OpenCL_rotate_v16i32_v16i32( uint16 v, uint16 i );
ulong __builtin_spirv_OpenCL_rotate_i64_i64( ulong v, ulong i );
ulong2 __builtin_spirv_OpenCL_rotate_v2i64_v2i64( ulong2 v, ulong2 i );
ulong3 __builtin_spirv_OpenCL_rotate_v3i64_v3i64( ulong3 v, ulong3 i );
ulong4 __builtin_spirv_OpenCL_rotate_v4i64_v4i64( ulong4 v, ulong4 i );
ulong8 __builtin_spirv_OpenCL_rotate_v8i64_v8i64( ulong8 v, ulong8 i );
ulong16 __builtin_spirv_OpenCL_rotate_v16i64_v16i64( ulong16 v, ulong16 i );

char __builtin_spirv_OpenCL_s_sub_sat_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_sub_sat_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_sub_sat_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_sub_sat_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_sub_sat_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_sub_sat_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_sub_sat_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_sub_sat_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_sub_sat_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_sub_sat_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_sub_sat_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_sub_sat_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_sub_sat_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_sub_sat_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_sub_sat_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_sub_sat_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_sub_sat_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_sub_sat_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_sub_sat_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_sub_sat_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_sub_sat_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_sub_sat_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_sub_sat_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_sub_sat_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_sub_sat_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_sub_sat_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_sub_sat_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_sub_sat_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_sub_sat_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_sub_sat_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_sub_sat_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_sub_sat_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_sub_sat_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_sub_sat_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_sub_sat_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_sub_sat_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_sub_sat_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_sub_sat_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_sub_sat_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_sub_sat_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_sub_sat_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_sub_sat_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_sub_sat_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_sub_sat_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_sub_sat_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_sub_sat_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_sub_sat_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_sub_sat_v16i64_v16i64( ulong16 x, ulong16 y );

short __builtin_spirv_OpenCL_s_upsample_i8_i8( char  hi, uchar lo );
short2 __builtin_spirv_OpenCL_s_upsample_v2i8_v2i8( char2  hi, uchar2 lo );
short3 __builtin_spirv_OpenCL_s_upsample_v3i8_v3i8( char3  hi, uchar3 lo );
short4 __builtin_spirv_OpenCL_s_upsample_v4i8_v4i8( char4  hi, uchar4 lo );
short8 __builtin_spirv_OpenCL_s_upsample_v8i8_v8i8( char8  hi, uchar8 lo );
short16 __builtin_spirv_OpenCL_s_upsample_v16i8_v16i8( char16  hi, uchar16 lo );
ushort __builtin_spirv_OpenCL_u_upsample_i8_i8( uchar hi, uchar lo );
ushort2 __builtin_spirv_OpenCL_u_upsample_v2i8_v2i8( uchar2 hi, uchar2 lo );
ushort3 __builtin_spirv_OpenCL_u_upsample_v3i8_v3i8( uchar3 hi, uchar3 lo );
ushort4 __builtin_spirv_OpenCL_u_upsample_v4i8_v4i8( uchar4 hi, uchar4 lo );
ushort8 __builtin_spirv_OpenCL_u_upsample_v8i8_v8i8( uchar8 hi, uchar8 lo );
ushort16 __builtin_spirv_OpenCL_u_upsample_v16i8_v16i8( uchar16 hi, uchar16 lo );
int __builtin_spirv_OpenCL_s_upsample_i16_i16( short  hi, ushort lo );
int2 __builtin_spirv_OpenCL_s_upsample_v2i16_v2i16( short2  hi, ushort2 lo );
int3 __builtin_spirv_OpenCL_s_upsample_v3i16_v3i16( short3  hi, ushort3 lo );
int4 __builtin_spirv_OpenCL_s_upsample_v4i16_v4i16( short4  hi, ushort4 lo );
int8 __builtin_spirv_OpenCL_s_upsample_v8i16_v8i16( short8  hi, ushort8 lo );
int16 __builtin_spirv_OpenCL_s_upsample_v16i16_v16i16( short16  hi, ushort16 lo );
uint __builtin_spirv_OpenCL_u_upsample_i16_i16( ushort hi, ushort lo );
uint2 __builtin_spirv_OpenCL_u_upsample_v2i16_v2i16( ushort2 hi, ushort2 lo );
uint3 __builtin_spirv_OpenCL_u_upsample_v3i16_v3i16( ushort3 hi, ushort3 lo );
uint4 __builtin_spirv_OpenCL_u_upsample_v4i16_v4i16( ushort4 hi, ushort4 lo );
uint8 __builtin_spirv_OpenCL_u_upsample_v8i16_v8i16( ushort8 hi, ushort8 lo );
uint16 __builtin_spirv_OpenCL_u_upsample_v16i16_v16i16( ushort16 hi, ushort16 lo );
long __builtin_spirv_OpenCL_s_upsample_i32_i32( int  hi, uint lo );
long2 __builtin_spirv_OpenCL_s_upsample_v2i32_v2i32( int2  hi, uint2 lo );
long3 __builtin_spirv_OpenCL_s_upsample_v3i32_v3i32( int3  hi, uint3 lo );
long4 __builtin_spirv_OpenCL_s_upsample_v4i32_v4i32( int4  hi, uint4 lo );
long8 __builtin_spirv_OpenCL_s_upsample_v8i32_v8i32( int8  hi, uint8 lo );
long16 __builtin_spirv_OpenCL_s_upsample_v16i32_v16i32( int16  hi, uint16 lo );
ulong __builtin_spirv_OpenCL_u_upsample_i32_i32( uint hi, uint lo );
ulong2 __builtin_spirv_OpenCL_u_upsample_v2i32_v2i32( uint2 hi, uint2 lo );
ulong3 __builtin_spirv_OpenCL_u_upsample_v3i32_v3i32( uint3 hi, uint3 lo );
ulong4 __builtin_spirv_OpenCL_u_upsample_v4i32_v4i32( uint4 hi, uint4 lo );
ulong8 __builtin_spirv_OpenCL_u_upsample_v8i32_v8i32( uint8 hi, uint8 lo );
ulong16 __builtin_spirv_OpenCL_u_upsample_v16i32_v16i32( uint16 hi, uint16 lo );


//
//  Math_ext
//        -acos,acosh,acospi,asin,asinh,asinpi,atan,atan2,atan2pi,atanh,atanpi,cbrt,ceil,copysign,
//       cos,cosh,cospi,divide_cr,erf,erfc,exp,exp2,exp10,expm1,fabs,fdim,floor,fma,fmax,fmin,
//       fmod,fract,frexp,hypot,ilogb,ldexp,lgamma,lgamma_r,log,log1p,log2,log10,logb,mad,maxmag,minmag,
//         modf,nan,nextafter,pow,pown,powr,remainder,remquo,rint,rootn,round,rsqrt,sin,sincos,
//       sinh,sinpi,sqrt,sqrt_cr,tan,tanh,tanpi,tgamma,trunc
//

float __builtin_spirv_OpenCL_acos_f32(float x );
float2 __builtin_spirv_OpenCL_acos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acos_f64( double x );
double2 __builtin_spirv_OpenCL_acos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_acos_f16(half x );
half2 __builtin_spirv_OpenCL_acos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acos_v16f16(half16 x);

float  __builtin_spirv_OpenCL_acosh_f32( float x );
float2 __builtin_spirv_OpenCL_acosh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acosh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acosh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acosh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acosh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acosh_f64( double x );
double2 __builtin_spirv_OpenCL_acosh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acosh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acosh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acosh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acosh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_acosh_f16( half x );
half2 __builtin_spirv_OpenCL_acosh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acosh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acosh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acosh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acosh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_acospi_f32( float x );
float2 __builtin_spirv_OpenCL_acospi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acospi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acospi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acospi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acospi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acospi_f64( double x );
double2 __builtin_spirv_OpenCL_acospi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acospi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acospi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acospi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acospi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_acospi_f16( half x );
half2 __builtin_spirv_OpenCL_acospi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acospi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acospi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acospi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acospi_v16f16(half16 x);

float __builtin_spirv_OpenCL_asin_f32(float value );
float2 __builtin_spirv_OpenCL_asin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asin_f64( double x );
double2 __builtin_spirv_OpenCL_asin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asin_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_asin_f16(half x );
half2 __builtin_spirv_OpenCL_asin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asin_v16f16(half16 x);

float  __builtin_spirv_OpenCL_asinh_f32( float x );
float2 __builtin_spirv_OpenCL_asinh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asinh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asinh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asinh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asinh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asinh_f64( double x );
double2 __builtin_spirv_OpenCL_asinh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asinh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asinh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asinh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asinh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_asinh_f16( half x );
half2 __builtin_spirv_OpenCL_asinh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asinh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asinh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asinh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asinh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_asinpi_f32( float x );
float2 __builtin_spirv_OpenCL_asinpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asinpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asinpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asinpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asinpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asinpi_f64( double x );
double2 __builtin_spirv_OpenCL_asinpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asinpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asinpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asinpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asinpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_asinpi_f16( half x );
half2 __builtin_spirv_OpenCL_asinpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asinpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asinpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asinpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asinpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_atan_f32(float value );
float2 __builtin_spirv_OpenCL_atan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan_f64( double x );
double2 __builtin_spirv_OpenCL_atan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_atan_f16(half x );
half2 __builtin_spirv_OpenCL_atan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atan_v16f16(half16 x);

float  __builtin_spirv_OpenCL_atan2_f32_f32( float y, float x );
float2 __builtin_spirv_OpenCL_atan2_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_atan2_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_atan2_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_atan2_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_atan2_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan2_f64_f64( double y, double x );
double2 __builtin_spirv_OpenCL_atan2_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_atan2_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_atan2_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_atan2_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_atan2_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atan2_f16_f16( half y, half x );
half2 __builtin_spirv_OpenCL_atan2_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_atan2_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_atan2_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_atan2_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_atan2_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_atan2pi_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_atan2pi_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_atan2pi_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_atan2pi_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_atan2pi_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_atan2pi_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan2pi_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_atan2pi_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_atan2pi_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_atan2pi_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_atan2pi_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_atan2pi_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atan2pi_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_atan2pi_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_atan2pi_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_atan2pi_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_atan2pi_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_atan2pi_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_atanh_f32( float x );
float2 __builtin_spirv_OpenCL_atanh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atanh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atanh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atanh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atanh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atanh_f64( double x );
double2 __builtin_spirv_OpenCL_atanh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atanh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atanh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atanh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atanh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atanh_f16( half x );
half2 __builtin_spirv_OpenCL_atanh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atanh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atanh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atanh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atanh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_atanpi_f32( float x );
float2 __builtin_spirv_OpenCL_atanpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atanpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atanpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atanpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atanpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atanpi_f64( double x );
double2 __builtin_spirv_OpenCL_atanpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atanpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atanpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atanpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atanpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atanpi_f16( half x );
half2 __builtin_spirv_OpenCL_atanpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atanpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atanpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atanpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atanpi_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cbrt_f32( float x );
float2 __builtin_spirv_OpenCL_cbrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cbrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cbrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cbrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cbrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_cbrt_f64( double x );
double2 __builtin_spirv_OpenCL_cbrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cbrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cbrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cbrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cbrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_cbrt_f16( half x );
half2 __builtin_spirv_OpenCL_cbrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cbrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cbrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cbrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cbrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_ceil_f32(float x );
float2 __builtin_spirv_OpenCL_ceil_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_ceil_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_ceil_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_ceil_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_ceil_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_ceil_f64(double x );
double2 __builtin_spirv_OpenCL_ceil_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_ceil_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_ceil_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_ceil_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_ceil_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_ceil_f16(half x );
half2 __builtin_spirv_OpenCL_ceil_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_ceil_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_ceil_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_ceil_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_ceil_v16f16(half16 x);

float  __builtin_spirv_OpenCL_copysign_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_copysign_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_copysign_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_copysign_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_copysign_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_copysign_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_copysign_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_copysign_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_copysign_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_copysign_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_copysign_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_copysign_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_copysign_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_copysign_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_copysign_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_copysign_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_copysign_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_copysign_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_cos_f32( float x );
float2 __builtin_spirv_OpenCL_cos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_cos_f64( double x );
double2 __builtin_spirv_OpenCL_cos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_cos_f16( half x );
half2 __builtin_spirv_OpenCL_cos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cos_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cosh_f32( float x );
float2 __builtin_spirv_OpenCL_cosh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cosh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cosh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cosh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cosh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_cosh_f64( double x );
double2 __builtin_spirv_OpenCL_cosh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cosh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cosh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cosh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cosh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_cosh_f16( half x );
half2 __builtin_spirv_OpenCL_cosh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cosh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cosh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cosh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cosh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cospi_f32( float x );
float2 __builtin_spirv_OpenCL_cospi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cospi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cospi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cospi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cospi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_cospi_f64( double x );
double2 __builtin_spirv_OpenCL_cospi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cospi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cospi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cospi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cospi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_cospi_f16( half x );
half2 __builtin_spirv_OpenCL_cospi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cospi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cospi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cospi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cospi_v16f16(half16 x);

float __builtin_spirv_divide_cr_f32_f32( float a, float b );
float2 __builtin_spirv_divide_cr_v2f32_v2f32( float2 a, float2 b );
float3 __builtin_spirv_divide_cr_v3f32_v3f32( float3 a, float3 b );
float4 __builtin_spirv_divide_cr_v4f32_v4f32( float4 a, float4 b );
float8 __builtin_spirv_divide_cr_v8f32_v8f32( float8 a, float8 b );
float16 __builtin_spirv_divide_cr_v16f32_v16f32( float16 a, float16 b );

#if defined(cl_khr_fp64)
double __builtin_spirv_divide_cr_f64_f64(double a, double b);
#endif // defined(cl_khr_fp64)

float  __builtin_spirv_OpenCL_erf_f32( float x );
float2 __builtin_spirv_OpenCL_erf_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_erf_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_erf_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_erf_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_erf_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_erf_f64( double x );
double2 __builtin_spirv_OpenCL_erf_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_erf_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_erf_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_erf_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_erf_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_erf_f16( half x );
half2 __builtin_spirv_OpenCL_erf_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_erf_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_erf_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_erf_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_erf_v16f16(half16 x);

float __builtin_spirv_OpenCL_erfc_f32( float x );
float2 __builtin_spirv_OpenCL_erfc_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_erfc_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_erfc_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_erfc_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_erfc_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_erfc_f64( double x );
double2 __builtin_spirv_OpenCL_erfc_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_erfc_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_erfc_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_erfc_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_erfc_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_erfc_f16( half x );
half2 __builtin_spirv_OpenCL_erfc_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_erfc_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_erfc_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_erfc_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_erfc_v16f16(half16 x);

float __builtin_spirv_OpenCL_exp_f32(float x);
float2 __builtin_spirv_OpenCL_exp_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp_f64( double x );
double2 __builtin_spirv_OpenCL_exp_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp_f16( half x );
half2 __builtin_spirv_OpenCL_exp_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp_v16f16(half16 x);

float  __builtin_spirv_OpenCL_exp2_f32( float x );
float2 __builtin_spirv_OpenCL_exp2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp2_f64( double x );
double2 __builtin_spirv_OpenCL_exp2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp2_f16( half x );
half2 __builtin_spirv_OpenCL_exp2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp2_v16f16(half16 x);

float  __builtin_spirv_OpenCL_exp10_f32( float x );
float2 __builtin_spirv_OpenCL_exp10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp10_f64( double x );
double2 __builtin_spirv_OpenCL_exp10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp10_f16( half x );
half2 __builtin_spirv_OpenCL_exp10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp10_v16f16(half16 x);

float  __builtin_spirv_OpenCL_expm1_f32( float a );
float2 __builtin_spirv_OpenCL_expm1_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_expm1_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_expm1_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_expm1_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_expm1_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_expm1_f64( double x );
double2 __builtin_spirv_OpenCL_expm1_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_expm1_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_expm1_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_expm1_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_expm1_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_expm1_f16( half x );
half2 __builtin_spirv_OpenCL_expm1_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_expm1_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_expm1_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_expm1_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_expm1_v16f16(half16 x);

float __builtin_spirv_OpenCL_fabs_f32(float x );
float2 __builtin_spirv_OpenCL_fabs_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_fabs_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_fabs_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_fabs_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_fabs_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fabs_f64(double x );
double2 __builtin_spirv_OpenCL_fabs_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_fabs_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_fabs_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_fabs_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_fabs_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fabs_f16(half x );
half2 __builtin_spirv_OpenCL_fabs_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_fabs_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_fabs_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_fabs_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_fabs_v16f16(half16 x);

float  __builtin_spirv_OpenCL_fdim_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fdim_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fdim_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fdim_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fdim_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fdim_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fdim_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fdim_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fdim_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fdim_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fdim_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fdim_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fdim_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fdim_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fdim_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fdim_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fdim_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fdim_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_floor_f32(float x );
float2 __builtin_spirv_OpenCL_floor_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_floor_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_floor_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_floor_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_floor_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_floor_f64(double x );
double2 __builtin_spirv_OpenCL_floor_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_floor_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_floor_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_floor_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_floor_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_floor_f16(half x );
half2 __builtin_spirv_OpenCL_floor_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_floor_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_floor_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_floor_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_floor_v16f16(half16 x);

float  __builtin_spirv_OpenCL_fma_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_fma_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_fma_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_fma_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_fma_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_fma_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fma_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_fma_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_fma_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_fma_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_fma_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_fma_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fma_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_fma_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_fma_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_fma_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_fma_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_fma_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float  __builtin_spirv_OpenCL_fmax_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fmax_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmax_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmax_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmax_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmax_v16f32_v16f32(float16 x, float16 y);
float2 __builtin_spirv_OpenCL_fmax_v2f32_f32(float2 x, float y);
float3 __builtin_spirv_OpenCL_fmax_v3f32_f32(float3 x, float y);
float4 __builtin_spirv_OpenCL_fmax_v4f32_f32(float4 x, float y);
float8 __builtin_spirv_OpenCL_fmax_v8f32_f32(float8 x, float y);
float16 __builtin_spirv_OpenCL_fmax_v16f32_f32(float16 x, float y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmax_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fmax_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmax_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmax_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmax_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmax_v16f64_v16f64(double16 x, double16 y);
double2 __builtin_spirv_OpenCL_fmax_v2f64_f64(double2 x, double y);
double3 __builtin_spirv_OpenCL_fmax_v3f64_f64(double3 x, double y);
double4 __builtin_spirv_OpenCL_fmax_v4f64_f64(double4 x, double y);
double8 __builtin_spirv_OpenCL_fmax_v8f64_f64(double8 x, double y);
double16 __builtin_spirv_OpenCL_fmax_v16f64_f64(double16 x, double y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmax_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmax_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmax_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmax_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmax_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmax_v16f16_v16f16(half16 x, half16 y);
half2 __builtin_spirv_OpenCL_fmax_v2f16_f16(half2 x, half y);
half3 __builtin_spirv_OpenCL_fmax_v3f16_f16(half3 x, half y);
half4 __builtin_spirv_OpenCL_fmax_v4f16_f16(half4 x, half y);
half8 __builtin_spirv_OpenCL_fmax_v8f16_f16(half8 x, half y);
half16 __builtin_spirv_OpenCL_fmax_v16f16_f16(half16 x, half y);

float  __builtin_spirv_OpenCL_fmin_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fmin_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmin_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmin_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmin_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmin_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmin_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fmin_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmin_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmin_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmin_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmin_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmin_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmin_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmin_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmin_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmin_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmin_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_fmod_f32_f32( float xx, float yy );
float2 __builtin_spirv_OpenCL_fmod_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmod_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmod_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmod_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmod_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmod_f64_f64( double xx, double yy );
double2 __builtin_spirv_OpenCL_fmod_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmod_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmod_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmod_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmod_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmod_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmod_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmod_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmod_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmod_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmod_v16f16_v16f16(half16 x, half16 y);

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

float __builtin_spirv_OpenCL_fract_f32_p1f32( float x, __global float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p1v2f32( float2 x, __global float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p1v3f32( float3 x, __global float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p1v4f32( float4 x, __global float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p1v8f32( float8 x, __global float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p1v16f32( float16 x, __global float16* iptr );
float __builtin_spirv_OpenCL_fract_f32_p0f32( float x, __private float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p0v2f32( float2 x, __private float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p0v3f32( float3 x, __private float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p0v4f32( float4 x, __private float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p0v8f32( float8 x, __private float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p0v16f32( float16 x, __private float16* iptr );
float __builtin_spirv_OpenCL_fract_f32_p3f32( float x, __local float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p3v2f32( float2 x, __local float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p3v3f32( float3 x, __local float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p3v4f32( float4 x, __local float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p3v8f32( float8 x, __local float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p3v16f32( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_fract_f32_p4f32( float x, __generic float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p4v2f32( float2 x, __generic float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p4v3f32( float3 x, __generic float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p4v4f32( float4 x, __generic float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p4v8f32( float8 x, __generic float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p4v16f32( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_fract_f16_p1f16( half x, __global half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p1v2f16( half2 x, __global half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p1v3f16( half3 x, __global half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p1v4f16( half4 x, __global half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p1v8f16( half8 x, __global half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p1v16f16( half16 x, __global half16* iptr );
half __builtin_spirv_OpenCL_fract_f16_p0f16( half x, __private half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p0v2f16( half2 x, __private half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p0v3f16( half3 x, __private half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p0v4f16( half4 x, __private half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p0v8f16( half8 x, __private half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p0v16f16( half16 x, __private half16* iptr );
half __builtin_spirv_OpenCL_fract_f16_p3f16( half x, __local half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p3v2f16( half2 x, __local half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p3v3f16( half3 x, __local half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p3v4f16( half4 x, __local half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p3v8f16( half8 x, __local half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p3v16f16( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_fract_f16_p4f16( half x, __generic half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p4v2f16( half2 x, __generic half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p4v3f16( half3 x, __generic half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p4v4f16( half4 x, __generic half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p4v8f16( half8 x, __generic half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p4v16f16( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fract_f64_p1f64( double x, __global double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p1v2f64( double2 x, __global double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p1v3f64( double3 x, __global double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p1v4f64( double4 x, __global double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p1v8f64( double8 x, __global double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p1v16f64( double16 x, __global double16* iptr );
double __builtin_spirv_OpenCL_fract_f64_p0f64( double x, __private double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p0v2f64( double2 x, __private double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p0v3f64( double3 x, __private double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p0v4f64( double4 x, __private double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p0v8f64( double8 x, __private double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p0v16f64( double16 x, __private double16* iptr );
double __builtin_spirv_OpenCL_fract_f64_p3f64( double x, __local double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p3v2f64( double2 x, __local double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p3v3f64( double3 x, __local double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p3v4f64( double4 x, __local double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p3v8f64( double8 x, __local double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p3v16f64( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_fract_f64_p4f64( double x, __generic double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p4v2f64( double2 x, __generic double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p4v3f64( double3 x, __generic double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p4v4f64( double4 x, __generic double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p4v8f64( double8 x, __generic double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p4v16f64( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_frexp_f32_p1i32( float x, __global int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p1v2i32( float2 x, __global int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p1v3i32( float3 x, __global int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p1v4i32( float4 x, __global int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p1v8i32( float8 x, __global int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p1v16i32( float16 x, __global int16* exp );
float __builtin_spirv_OpenCL_frexp_f32_p0i32( float x, __private int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p0v2i32( float2 x, __private int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p0v3i32( float3 x, __private int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p0v4i32( float4 x, __private int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p0v8i32( float8 x, __private int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p0v16i32( float16 x, __private int16* exp );
float __builtin_spirv_OpenCL_frexp_f32_p3i32( float x, __local int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p3v2i32( float2 x, __local int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p3v3i32( float3 x, __local int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p3v4i32( float4 x, __local int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p3v8i32( float8 x, __local int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p3v16i32( float16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_frexp_f32_p4i32( float x, __generic int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p4v2i32( float2 x, __generic int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p4v3i32( float3 x, __generic int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p4v4i32( float4 x, __generic int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p4v8i32( float8 x, __generic int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p4v16i32( float16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_frexp_f16_p1i32( half x, __global int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p1v2i32( half2 x, __global int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p1v3i32( half3 x, __global int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p1v4i32( half4 x, __global int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p1v8i32( half8 x, __global int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p1v16i32( half16 x, __global int16* exp );
half __builtin_spirv_OpenCL_frexp_f16_p0i32( half x, __private int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p0v2i32( half2 x, __private int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p0v3i32( half3 x, __private int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p0v4i32( half4 x, __private int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p0v8i32( half8 x, __private int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p0v16i32( half16 x, __private int16* exp );
half __builtin_spirv_OpenCL_frexp_f16_p3i32( half x, __local int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p3v2i32( half2 x, __local int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p3v3i32( half3 x, __local int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p3v4i32( half4 x, __local int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p3v8i32( half8 x, __local int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p3v16i32( half16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_frexp_f16_p4i32( half x, __generic int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p4v2i32( half2 x, __generic int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p4v3i32( half3 x, __generic int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p4v4i32( half4 x, __generic int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p4v8i32( half8 x, __generic int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p4v16i32( half16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_frexp_f64_p1i32( double x, __global int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p1v2i32( double2 x, __global int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p1v3i32( double3 x, __global int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p1v4i32( double4 x, __global int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p1v8i32( double8 x, __global int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p1v16i32( double16 x, __global int16* exp );
double __builtin_spirv_OpenCL_frexp_f64_p0i32( double x, __private int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p0v2i32( double2 x, __private int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p0v3i32( double3 x, __private int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p0v4i32( double4 x, __private int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p0v8i32( double8 x, __private int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p0v16i32( double16 x, __private int16* exp );
double __builtin_spirv_OpenCL_frexp_f64_p3i32( double x, __local int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p3v2i32( double2 x, __local int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p3v3i32( double3 x, __local int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p3v4i32( double4 x, __local int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p3v8i32( double8 x, __local int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p3v16i32( double16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_frexp_f64_p4i32( double x, __generic int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p4v2i32( double2 x, __generic int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p4v3i32( double3 x, __generic int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p4v4i32( double4 x, __generic int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p4v8i32( double8 x, __generic int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p4v16i32( double16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_hypot_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_hypot_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_hypot_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_hypot_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_hypot_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_hypot_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_hypot_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_hypot_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_hypot_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_hypot_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_hypot_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_hypot_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_hypot_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_hypot_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_hypot_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_hypot_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_hypot_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_hypot_v16f16_v16f16(half16 x, half16 y);

int __builtin_spirv_OpenCL_ilogb_f32( float x );
int __builtin_spirv_OpenCL_ilogb_f32( float x );
int2 __builtin_spirv_OpenCL_ilogb_v2f32(float2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f32(float3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f32(float4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f32(float8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f32(float16 x);
#if defined(cl_khr_fp64)
int __builtin_spirv_OpenCL_ilogb_f64( double x );
int2 __builtin_spirv_OpenCL_ilogb_v2f64(double2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f64(double3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f64(double4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f64(double8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
int __builtin_spirv_OpenCL_ilogb_f16( half x );
int2 __builtin_spirv_OpenCL_ilogb_v2f16(half2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f16(half3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f16(half4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f16(half8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f16(half16 x);

float __builtin_spirv_OpenCL_ldexp_f32_i32( float x, int n );
float2 __builtin_spirv_OpenCL_ldexp_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_ldexp_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_ldexp_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_ldexp_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_ldexp_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_ldexp_f64_i32( double x, int n );
double2 __builtin_spirv_OpenCL_ldexp_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_ldexp_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_ldexp_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_ldexp_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_ldexp_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_ldexp_f16_i32( half x, int n );
half2 __builtin_spirv_OpenCL_ldexp_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_ldexp_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_ldexp_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_ldexp_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_ldexp_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_lgamma_f32( float x );
float2 __builtin_spirv_OpenCL_lgamma_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_lgamma_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_lgamma_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_lgamma_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_lgamma_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_lgamma_f64( double x );
double2 __builtin_spirv_OpenCL_lgamma_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_lgamma_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_lgamma_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_lgamma_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_lgamma_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_lgamma_f16( half x );
half2 __builtin_spirv_OpenCL_lgamma_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_lgamma_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_lgamma_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_lgamma_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_lgamma_v16f16(half16 x);

float __builtin_spirv_OpenCL_lgamma_r_f32_p1i32( float x, __global int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p1v2i32( float2 x, __global int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p1v3i32( float3 x, __global int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p1v4i32( float4 x, __global int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p1v8i32( float8 x, __global int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p1v16i32( float16 x, __global int16* signp );
float __builtin_spirv_OpenCL_lgamma_r_f32_p3i32( float x, __local int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p3v2i32( float2 x, __local int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p3v3i32( float3 x, __local int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p3v4i32( float4 x, __local int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p3v8i32( float8 x, __local int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p3v16i32( float16 x, __local int16* signp );
float __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( float x, __private int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32( float2 x, __private int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32( float3 x, __private int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32( float4 x, __private int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32( float8 x, __private int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32( float16 x, __private int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_lgamma_r_f32_p4i32( float x, __generic int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p4v2i32( float2 x, __generic int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p4v3i32( float3 x, __generic int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p4v4i32( float4 x, __generic int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p4v8i32( float8 x, __generic int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p4v16i32( float16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_lgamma_r_f16_p1i32( half x, __global int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p1v2i32( half2 x, __global int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p1v3i32( half3 x, __global int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p1v4i32( half4 x, __global int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p1v8i32( half8 x, __global int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p1v16i32( half16 x, __global int16* signp );
half __builtin_spirv_OpenCL_lgamma_r_f16_p0i32( half x, __private int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p0v2i32( half2 x, __private int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p0v3i32( half3 x, __private int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p0v4i32( half4 x, __private int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p0v8i32( half8 x, __private int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p0v16i32( half16 x, __private int16* signp );
half __builtin_spirv_OpenCL_lgamma_r_f16_p3i32( half x, __local int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p3v2i32( half2 x, __local int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p3v3i32( half3 x, __local int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p3v4i32( half4 x, __local int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p3v8i32( half8 x, __local int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p3v16i32( half16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_lgamma_r_f16_p4i32( half x, __generic int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p4v2i32( half2 x, __generic int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p4v3i32( half3 x, __generic int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p4v4i32( half4 x, __generic int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p4v8i32( half8 x, __generic int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p4v16i32( half16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_lgamma_r_f64_p1i32( double x, __global int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p1v2i32( double2 x, __global int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p1v3i32( double3 x, __global int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p1v4i32( double4 x, __global int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p1v8i32( double8 x, __global int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p1v16i32( double16 x, __global int16* signp );
double __builtin_spirv_OpenCL_lgamma_r_f64_p0i32( double x, __private int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p0v2i32( double2 x, __private int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p0v3i32( double3 x, __private int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p0v4i32( double4 x, __private int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p0v8i32( double8 x, __private int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p0v16i32( double16 x, __private int16* signp );
double __builtin_spirv_OpenCL_lgamma_r_f64_p3i32( double x, __local int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p3v2i32( double2 x, __local int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p3v3i32( double3 x, __local int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p3v4i32( double4 x, __local int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p3v8i32( double8 x, __local int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p3v16i32( double16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_lgamma_r_f64_p4i32( double x, __generic int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p4v2i32( double2 x, __generic int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p4v3i32( double3 x, __generic int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p4v4i32( double4 x, __generic int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p4v8i32( double8 x, __generic int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p4v16i32( double16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_log_f32( float x );
float2 __builtin_spirv_OpenCL_log_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log_f64( double x );
double2 __builtin_spirv_OpenCL_log_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log_f16( half x );
half2 __builtin_spirv_OpenCL_log_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log_v16f16(half16 x);

float __builtin_spirv_OpenCL_log1p_f32( float a );
float2 __builtin_spirv_OpenCL_log1p_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log1p_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log1p_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log1p_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log1p_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log1p_f64( double a );
double2 __builtin_spirv_OpenCL_log1p_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log1p_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log1p_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log1p_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log1p_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log1p_f16( half x );
half2 __builtin_spirv_OpenCL_log1p_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log1p_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log1p_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log1p_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log1p_v16f16(half16 x);

float __builtin_spirv_OpenCL_log2_f32( float x );
float2 __builtin_spirv_OpenCL_log2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log2_f64( double x );
double2 __builtin_spirv_OpenCL_log2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log2_f16( half x );
half2 __builtin_spirv_OpenCL_log2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log2_v16f16(half16 x);

float __builtin_spirv_OpenCL_log10_f32( float x );
float2 __builtin_spirv_OpenCL_log10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log10_f64( double x );
double2 __builtin_spirv_OpenCL_log10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log10_f16( half x );
half2 __builtin_spirv_OpenCL_log10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log10_v16f16(half16 x);

float __builtin_spirv_OpenCL_logb_f32( float x );
float2 __builtin_spirv_OpenCL_logb_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_logb_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_logb_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_logb_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_logb_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_logb_f64( double x );
double2 __builtin_spirv_OpenCL_logb_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_logb_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_logb_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_logb_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_logb_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_logb_f16( half x );
half2 __builtin_spirv_OpenCL_logb_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_logb_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_logb_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_logb_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_logb_v16f16(half16 x);

float __builtin_spirv_OpenCL_mad_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_mad_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_mad_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_mad_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_mad_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_mad_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_mad_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_mad_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_mad_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_mad_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_mad_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_mad_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_mad_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_mad_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_mad_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_mad_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_mad_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_mad_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_maxmag_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_maxmag_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_maxmag_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_maxmag_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_maxmag_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_maxmag_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_maxmag_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_maxmag_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_maxmag_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_maxmag_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_maxmag_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_maxmag_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_maxmag_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_maxmag_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_maxmag_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_maxmag_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_maxmag_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_maxmag_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_minmag_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_minmag_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_minmag_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_minmag_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_minmag_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_minmag_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_minmag_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_minmag_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_minmag_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_minmag_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_minmag_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_minmag_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_minmag_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_minmag_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_minmag_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_minmag_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_minmag_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_minmag_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_modf_f32_p1f32( float x, __global float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p1v2f32( float2 x, __global float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p1v3f32( float3 x, __global float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p1v4f32( float4 x, __global float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p1v8f32( float8 x, __global float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p1v16f32( float16 x, __global float16* iptr );
float __builtin_spirv_OpenCL_modf_f32_p0f32( float x, __private float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p0v2f32( float2 x, __private float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p0v3f32( float3 x, __private float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p0v4f32( float4 x, __private float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p0v8f32( float8 x, __private float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p0v16f32( float16 x, __private float16* iptr );
float __builtin_spirv_OpenCL_modf_f32_p3f32( float x, __local float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p3v2f32( float2 x, __local float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p3v3f32( float3 x, __local float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p3v4f32( float4 x, __local float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p3v8f32( float8 x, __local float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p3v16f32( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_modf_f32_p4f32( float x, __generic float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p4v2f32( float2 x, __generic float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p4v3f32( float3 x, __generic float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p4v4f32( float4 x, __generic float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p4v8f32( float8 x, __generic float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p4v16f32( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_modf_f16_p1f16( half x, __global half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p1v2f16( half2 x, __global half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p1v3f16( half3 x, __global half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p1v4f16( half4 x, __global half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p1v8f16( half8 x, __global half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p1v16f16( half16 x, __global half16* iptr );
half __builtin_spirv_OpenCL_modf_f16_p0f16( half x, __private half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p0v2f16( half2 x, __private half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p0v3f16( half3 x, __private half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p0v4f16( half4 x, __private half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p0v8f16( half8 x, __private half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p0v16f16( half16 x, __private half16* iptr );
half __builtin_spirv_OpenCL_modf_f16_p3f16( half x, __local half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p3v2f16( half2 x, __local half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p3v3f16( half3 x, __local half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p3v4f16( half4 x, __local half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p3v8f16( half8 x, __local half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p3v16f16( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_modf_f16_p4f16( half x, __generic half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p4v2f16( half2 x, __generic half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p4v3f16( half3 x, __generic half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p4v4f16( half4 x, __generic half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p4v8f16( half8 x, __generic half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p4v16f16( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_modf_f64_p1f64( double x, __global double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p1v2f64( double2 x, __global double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p1v3f64( double3 x, __global double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p1v4f64( double4 x, __global double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p1v8f64( double8 x, __global double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p1v16f64( double16  x, __global double16* iptr );
double __builtin_spirv_OpenCL_modf_f64_p0f64( double x, __private double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p0v2f64( double2 x, __private double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p0v3f64( double3 x, __private double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p0v4f64( double4 x, __private double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p0v8f64( double8 x, __private double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p0v16f64( double16   x, __private double16* iptr );
double __builtin_spirv_OpenCL_modf_f64_p3f64( double x, __local double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p3v2f64( double2 x, __local double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p3v3f64( double3 x, __local double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p3v4f64( double4 x, __local double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p3v8f64( double8 x, __local double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p3v16f64( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_modf_f64_p4f64( double x, __generic double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p4v2f64( double2 x, __generic double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p4v3f64( double3 x, __generic double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p4v4f64( double4 x, __generic double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p4v8f64( double8 x, __generic double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p4v16f64( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_nan_i32( uint nancode );
float2 __builtin_spirv_OpenCL_nan_v2i32(uint2 x);
float3 __builtin_spirv_OpenCL_nan_v3i32(uint3 x);
float4 __builtin_spirv_OpenCL_nan_v4i32(uint4 x);
float8 __builtin_spirv_OpenCL_nan_v8i32(uint8 x);
float16 __builtin_spirv_OpenCL_nan_v16i32(uint16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_nan_i64( ulong nancode );
double2 __builtin_spirv_OpenCL_nan_v2i64(ulong2 x);
double3 __builtin_spirv_OpenCL_nan_v3i64(ulong3 x);
double4 __builtin_spirv_OpenCL_nan_v4i64(ulong4 x);
double8 __builtin_spirv_OpenCL_nan_v8i64(ulong8 x);
double16 __builtin_spirv_OpenCL_nan_v16i64(ulong16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_nan_i16( ushort nancode );
half2 __builtin_spirv_OpenCL_nan_v2i16(ushort2 x);
half3 __builtin_spirv_OpenCL_nan_v3i16(ushort3 x);
half4 __builtin_spirv_OpenCL_nan_v4i16(ushort4 x);
half8 __builtin_spirv_OpenCL_nan_v8i16(ushort8 x);
half16 __builtin_spirv_OpenCL_nan_v16i16(ushort16 x);

float __builtin_spirv_OpenCL_nextafter_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_nextafter_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_nextafter_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_nextafter_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_nextafter_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_nextafter_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_nextafter_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_nextafter_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_nextafter_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_nextafter_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_nextafter_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_nextafter_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_nextafter_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_nextafter_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_nextafter_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_nextafter_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_nextafter_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_nextafter_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_pow_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_pow_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_pow_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_pow_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_pow_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_pow_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_pow_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_pow_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_pow_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_pow_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_pow_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_pow_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_pow_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_pow_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_pow_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_pow_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_pow_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_pow_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_pown_f32_i32( float x, int y );
float2 __builtin_spirv_OpenCL_pown_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_pown_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_pown_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_pown_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_pown_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_pown_f64_i32( double x, int y );
double2 __builtin_spirv_OpenCL_pown_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_pown_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_pown_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_pown_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_pown_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_pown_f16_i32( half x, int y );
half2 __builtin_spirv_OpenCL_pown_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_pown_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_pown_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_pown_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_pown_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_powr_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_powr_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_powr_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_powr_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_powr_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_powr_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_powr_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_powr_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_powr_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_powr_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_powr_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_powr_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_powr_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_powr_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_powr_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_powr_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_powr_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_powr_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_remainder_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_remainder_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_remainder_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_remainder_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_remainder_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_remainder_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_remainder_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_remainder_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_remainder_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_remainder_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_remainder_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_remainder_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_remainder_f16_f16( half y, half x );
half2 __builtin_spirv_OpenCL_remainder_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_remainder_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_remainder_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_remainder_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_remainder_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_remquo_f32_f32_p1i32( float xx, float yy, __global int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p1v2i32( float2 xx, float2 yy, __global int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p1v3i32( float3 xx, float3 yy, __global int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p1v4i32( float4 xx, float4 yy, __global int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p1v8i32( float8 xx, float8 yy, __global int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p1v16i32( float16 xx, float16 yy, __global int16* quo );
float __builtin_spirv_OpenCL_remquo_f32_f32_p0i32( float xx, float yy, __private int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p0v2i32( float2 xx, float2 yy, __private int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p0v3i32( float3 xx, float3 yy, __private int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p0v4i32( float4 xx, float4 yy, __private int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p0v8i32( float8 xx, float8 yy, __private int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p0v16i32( float16 xx, float16 yy, __private int16* quo );
float __builtin_spirv_OpenCL_remquo_f32_f32_p3i32( float xx, float yy, __local int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p3v2i32( float2 xx, float2 yy, __local int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p3v3i32( float3 xx, float3 yy, __local int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p3v4i32( float4 xx, float4 yy, __local int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p3v8i32( float8 xx, float8 yy, __local int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p3v16i32( float16 xx, float16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_remquo_f32_f32_p4i32( float xx, float yy, __generic int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p4v2i32( float2 xx, float2 yy, __generic int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p4v3i32( float3 xx, float3 yy, __generic int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p4v4i32( float4 xx, float4 yy, __generic int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p4v8i32( float8 xx, float8 yy, __generic int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p4v16i32( float16 xx, float16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_remquo_f16_f16_p1i32( half xx, half yy, __global int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p1v2i32( half2 xx, half2 yy, __global int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p1v3i32( half3 xx, half3 yy, __global int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p1v4i32( half4 xx, half4 yy, __global int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p1v8i32( half8 xx, half8 yy, __global int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p1v16i32( half16 xx, half16 yy, __global int16* quo );
half __builtin_spirv_OpenCL_remquo_f16_f16_p0i32( half xx, half yy, __private int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p0v2i32( half2 xx, half2 yy, __private int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p0v3i32( half3 xx, half3 yy, __private int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p0v4i32( half4 xx, half4 yy, __private int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p0v8i32( half8 xx, half8 yy, __private int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p0v16i32( half16 xx, half16 yy, __private int16* quo );
half __builtin_spirv_OpenCL_remquo_f16_f16_p3i32( half xx, half yy, __local int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p3v2i32( half2 xx, half2 yy, __local int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p3v3i32( half3 xx, half3 yy, __local int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p3v4i32( half4 xx, half4 yy, __local int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p3v8i32( half8 xx, half8 yy, __local int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p3v16i32( half16 xx, half16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_remquo_f16_f16_p4i32( half xx, half yy, __generic int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p4v2i32( half2 xx, half2 yy, __generic int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p4v3i32( half3 xx, half3 yy, __generic int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p4v4i32( half4 xx, half4 yy, __generic int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p4v8i32( half8 xx, half8 yy, __generic int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p4v16i32( half16 xx, half16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_remquo_f64_f64_p1i32( double xx, double yy, __global int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p1v2i32( double2 xx, double2 yy, __global int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p1v3i32( double3 xx, double3 yy, __global int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p1v4i32( double4 xx, double4 yy, __global int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p1v8i32( double8 xx, double8 yy, __global int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p1v16i32( double16 xx, double16 yy, __global int16* quo );
double __builtin_spirv_OpenCL_remquo_f64_f64_p0i32( double xx, double yy, __private int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p0v2i32( double2 xx, double2 yy, __private int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p0v3i32( double3 xx, double3 yy, __private int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p0v4i32( double4 xx, double4 yy, __private int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p0v8i32( double8 xx, double8 yy, __private int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p0v16i32( double16 xx, double16 yy, __private int16* quo );
double __builtin_spirv_OpenCL_remquo_f64_f64_p3i32( double xx, double yy, __local int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p3v2i32( double2 xx, double2 yy, __local int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p3v3i32( double3 xx, double3 yy, __local int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p3v4i32( double4 xx, double4 yy, __local int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p3v8i32( double8 xx, double8 yy, __local int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p3v16i32( double16 xx, double16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_remquo_f64_f64_p4i32( double xx, double yy, __generic int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p4v2i32( double2 xx, double2 yy, __generic int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p4v3i32( double3 xx, double3 yy, __generic int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p4v4i32( double4 xx, double4 yy, __generic int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p4v8i32( double8 xx, double8 yy, __generic int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p4v16i32( double16 xx, double16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_rint_f32(float x );
float2 __builtin_spirv_OpenCL_rint_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_rint_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_rint_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_rint_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_rint_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rint_f64( double x );
double2 __builtin_spirv_OpenCL_rint_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_rint_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_rint_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_rint_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_rint_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rint_f16(half x );
half2 __builtin_spirv_OpenCL_rint_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_rint_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_rint_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_rint_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_rint_v16f16(half16 x);

float __builtin_spirv_OpenCL_rootn_f32_i32( float x, int n );
float2 __builtin_spirv_OpenCL_rootn_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_rootn_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_rootn_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_rootn_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_rootn_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rootn_f64_i32( double y, int x );
double2 __builtin_spirv_OpenCL_rootn_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_rootn_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_rootn_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_rootn_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_rootn_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rootn_f16_i32( half y, int x );
half2 __builtin_spirv_OpenCL_rootn_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_rootn_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_rootn_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_rootn_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_rootn_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_round_f32( float x );
float2 __builtin_spirv_OpenCL_round_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_round_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_round_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_round_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_round_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_round_f64( double x );
double2 __builtin_spirv_OpenCL_round_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_round_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_round_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_round_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_round_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_round_f16( half x );
half2 __builtin_spirv_OpenCL_round_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_round_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_round_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_round_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_round_v16f16(half16 x);

float __builtin_spirv_OpenCL_rsqrt_f32( float x );
float2 __builtin_spirv_OpenCL_rsqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_rsqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_rsqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_rsqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_rsqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rsqrt_f64( double x );
double2 __builtin_spirv_OpenCL_rsqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_rsqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_rsqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_rsqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_rsqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rsqrt_f16( half x );
half2 __builtin_spirv_OpenCL_rsqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_rsqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_rsqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_rsqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_rsqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_sin_f32( float x );
float2 __builtin_spirv_OpenCL_sin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sin_f64( double x );
double2 __builtin_spirv_OpenCL_sin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sin_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sin_f16( half x );
half2 __builtin_spirv_OpenCL_sin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sin_v16f16(half16 x);

float __builtin_spirv_OpenCL_sincos_f32_p0f32( float x, __private float* cosval );
float __builtin_spirv_OpenCL_sincos_f32_p1f32( float x, __global float* cosval );
float __builtin_spirv_OpenCL_sincos_f32_p3f32( float x, __local float* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_sincos_f32_p4f32( float x, __generic float* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_sincos_f16_p0f16( half x, __private half* cosval );
half __builtin_spirv_OpenCL_sincos_f16_p1f16( half x, __global half* cosval );
half __builtin_spirv_OpenCL_sincos_f16_p3f16( half x, __local half* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_sincos_f16_p4f16( half x, __generic half* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sincos_f64_p0f64( double x, __private double* cosval );
double __builtin_spirv_OpenCL_sincos_f64_p3f64( double x, __local double* cosval );
double __builtin_spirv_OpenCL_sincos_f64_p1f64( double x, __global double* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_sincos_f64_p4f64( double x, __generic double* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_sinh_f32( float x );
float2 __builtin_spirv_OpenCL_sinh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sinh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sinh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sinh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sinh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sinh_f64( double x );
double2 __builtin_spirv_OpenCL_sinh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sinh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sinh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sinh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sinh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sinh_f16( half x );
half2 __builtin_spirv_OpenCL_sinh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sinh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sinh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sinh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sinh_v16f16(half16 x);

float __builtin_spirv_OpenCL_sinpi_f32( float x );
float2 __builtin_spirv_OpenCL_sinpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sinpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sinpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sinpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sinpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sinpi_f64( double x );
double2 __builtin_spirv_OpenCL_sinpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sinpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sinpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sinpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sinpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sinpi_f16( half x );
half2 __builtin_spirv_OpenCL_sinpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sinpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sinpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sinpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sinpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_sqrt_f32( float x );
float2 __builtin_spirv_OpenCL_sqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sqrt_f64( double x );
double2 __builtin_spirv_OpenCL_sqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sqrt_f16( half x );
half2 __builtin_spirv_OpenCL_sqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_sqrt_cr_f32( float a );
float2 __builtin_spirv_OpenCL_sqrt_cr_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sqrt_cr_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sqrt_cr_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sqrt_cr_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sqrt_cr_v16f32(float16 x);
#ifdef cl_fp64_basic_ops
double __builtin_spirv_OpenCL_sqrt_cr_f64( double x );
double2 __builtin_spirv_OpenCL_sqrt_cr_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sqrt_cr_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sqrt_cr_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sqrt_cr_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sqrt_cr_v16f64(double16 x);
#endif // cl_fp64_basic_ops
half __builtin_spirv_OpenCL_sqrt_cr_f16( half a );
half2 __builtin_spirv_OpenCL_sqrt_cr_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sqrt_cr_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sqrt_cr_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sqrt_cr_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sqrt_cr_v16f16(half16 x);

float __builtin_spirv_OpenCL_tan_f32( float x );
float2 __builtin_spirv_OpenCL_tan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tan_f64( double x );
double2 __builtin_spirv_OpenCL_tan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tan_f16( half x );
half2 __builtin_spirv_OpenCL_tan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tan_v16f16(half16 x);

float __builtin_spirv_OpenCL_tanh_f32( float x );
float2 __builtin_spirv_OpenCL_tanh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tanh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tanh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tanh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tanh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tanh_f64( double x );
double2 __builtin_spirv_OpenCL_tanh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tanh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tanh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tanh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tanh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tanh_f16( half x );
half2 __builtin_spirv_OpenCL_tanh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tanh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tanh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tanh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tanh_v16f16(half16 x);

float __builtin_spirv_OpenCL_tanpi_f32( float x );
float2 __builtin_spirv_OpenCL_tanpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tanpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tanpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tanpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tanpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tanpi_f64( double x );
double2 __builtin_spirv_OpenCL_tanpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tanpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tanpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tanpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tanpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tanpi_f16( half x );
half2 __builtin_spirv_OpenCL_tanpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tanpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tanpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tanpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tanpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_tgamma_f32( float x );
float2 __builtin_spirv_OpenCL_tgamma_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tgamma_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tgamma_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tgamma_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tgamma_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tgamma_f64( double x );
double2 __builtin_spirv_OpenCL_tgamma_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tgamma_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tgamma_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tgamma_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tgamma_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tgamma_f16( half x );
half2 __builtin_spirv_OpenCL_tgamma_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tgamma_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tgamma_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tgamma_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tgamma_v16f16(half16 x);

float __builtin_spirv_OpenCL_trunc_f32(float x );
float2 __builtin_spirv_OpenCL_trunc_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_trunc_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_trunc_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_trunc_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_trunc_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_trunc_f64(double x );
double2 __builtin_spirv_OpenCL_trunc_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_trunc_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_trunc_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_trunc_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_trunc_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_trunc_f16(half x );
half2 __builtin_spirv_OpenCL_trunc_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_trunc_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_trunc_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_trunc_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_trunc_v16f16(half16 x);

//
//  Native
//        -native_cos,native_divide,native_exp,native_exp2,native_exp10,native_log,native_log2,
//         native_log10,native_powr,native_recip,native_rsqrt,native_sin,native_sqrt,native_tan
//

float __builtin_spirv_OpenCL_native_cos_f32(float x );
float2 __builtin_spirv_OpenCL_native_cos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_cos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_cos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_cos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_cos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_cos_f64( double x );
double2 __builtin_spirv_OpenCL_native_cos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_cos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_cos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_cos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_cos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_cos_f16(half x );
half2 __builtin_spirv_OpenCL_native_cos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_cos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_cos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_cos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_cos_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_divide_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_native_divide_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_native_divide_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_native_divide_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_native_divide_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_native_divide_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_divide_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_native_divide_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_native_divide_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_native_divide_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_native_divide_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_native_divide_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_divide_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_native_divide_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_native_divide_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_native_divide_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_native_divide_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_native_divide_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_native_exp_f32( float x );
float2 __builtin_spirv_OpenCL_native_exp_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp_f16( half x );
half2 __builtin_spirv_OpenCL_native_exp_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_exp2_f32(float x );
float2 __builtin_spirv_OpenCL_native_exp2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp2_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp2_f16(half x );
half2 __builtin_spirv_OpenCL_native_exp2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp2_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_exp10_f32( float x );
float2 __builtin_spirv_OpenCL_native_exp10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp10_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp10_f16( half x );
half2 __builtin_spirv_OpenCL_native_exp10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp10_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log_f32( float x );
float2 __builtin_spirv_OpenCL_native_log_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log_f64( double x );
double2 __builtin_spirv_OpenCL_native_log_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log_f16( half x );
half2 __builtin_spirv_OpenCL_native_log_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log2_f32(float x );
float2 __builtin_spirv_OpenCL_native_log2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log2_f64( double x );
double2 __builtin_spirv_OpenCL_native_log2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log2_f16(half x );
half2 __builtin_spirv_OpenCL_native_log2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log2_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log10_f32( float x );
float2 __builtin_spirv_OpenCL_native_log10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log10_f64( double x );
double2 __builtin_spirv_OpenCL_native_log10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log10_f16( half x );
half2 __builtin_spirv_OpenCL_native_log10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log10_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_powr_f32_f32(float x, float y);
float2 __builtin_spirv_OpenCL_native_powr_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_native_powr_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_native_powr_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_native_powr_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_native_powr_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_powr_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_native_powr_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_native_powr_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_native_powr_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_native_powr_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_native_powr_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_powr_f16_f16(half x, half y);
half2 __builtin_spirv_OpenCL_native_powr_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_native_powr_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_native_powr_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_native_powr_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_native_powr_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_native_recip_f32(float x );
float2 __builtin_spirv_OpenCL_native_recip_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_recip_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_recip_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_recip_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_recip_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_recip_f64( double x );
double2 __builtin_spirv_OpenCL_native_recip_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_recip_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_recip_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_recip_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_recip_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_recip_f16(half x );
half2 __builtin_spirv_OpenCL_native_recip_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_recip_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_recip_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_recip_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_recip_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_rsqrt_f32(float x );
float2 __builtin_spirv_OpenCL_native_rsqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_rsqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_rsqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_rsqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_rsqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_rsqrt_f64(double x );
double2 __builtin_spirv_OpenCL_native_rsqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_rsqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_rsqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_rsqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_rsqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_rsqrt_f16(half x );
half2 __builtin_spirv_OpenCL_native_rsqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_rsqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_rsqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_rsqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_rsqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_sin_f32(float x );
float2 __builtin_spirv_OpenCL_native_sin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_sin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_sin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_sin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_sin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_sin_f64( double x );
double2 __builtin_spirv_OpenCL_native_sin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_sin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_sin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_sin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_sin_v16f64(double16 x);
#endif // cl_khr_fp64
half __builtin_spirv_OpenCL_native_sin_f16(half x );
half2 __builtin_spirv_OpenCL_native_sin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_sin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_sin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_sin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_sin_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_sqrt_f32(float x );
float2 __builtin_spirv_OpenCL_native_sqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_sqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_sqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_sqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_sqrt_v16f32(float16 x);
#ifdef cl_fp64_basic_ops
double __builtin_spirv_OpenCL_native_sqrt_f64(double x );
double2 __builtin_spirv_OpenCL_native_sqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_sqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_sqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_sqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_sqrt_v16f64(double16 x);
#endif // cl_fp64_basic_ops
half __builtin_spirv_OpenCL_native_sqrt_f16(half x );
half2 __builtin_spirv_OpenCL_native_sqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_sqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_sqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_sqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_sqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_tan_f32(float x );
float2 __builtin_spirv_OpenCL_native_tan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_tan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_tan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_tan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_tan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_tan_f64( double x );
double2 __builtin_spirv_OpenCL_native_tan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_tan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_tan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_tan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_tan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_tan_f16(half x );
half2 __builtin_spirv_OpenCL_native_tan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_tan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_tan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_tan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_tan_v16f16(half16 x);

//
//  Relational
//        -bitselect,select
//

uchar __builtin_spirv_OpenCL_bitselect_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_bitselect_v2i8_v2i8_v2i8(uchar2 x, uchar2 y, uchar2 z);
uchar3 __builtin_spirv_OpenCL_bitselect_v3i8_v3i8_v3i8(uchar3 x, uchar3 y, uchar3 z);
uchar4 __builtin_spirv_OpenCL_bitselect_v4i8_v4i8_v4i8(uchar4 x, uchar4 y, uchar4 z);
uchar8 __builtin_spirv_OpenCL_bitselect_v8i8_v8i8_v8i8(uchar8 x, uchar8 y, uchar8 z);
uchar16 __builtin_spirv_OpenCL_bitselect_v16i8_v16i8_v16i8(uchar16 x, uchar16 y, uchar16 z);
ushort __builtin_spirv_OpenCL_bitselect_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_bitselect_v2i16_v2i16_v2i16(ushort2 x, ushort2 y, ushort2 z);
ushort3 __builtin_spirv_OpenCL_bitselect_v3i16_v3i16_v3i16(ushort3 x, ushort3 y, ushort3 z);
ushort4 __builtin_spirv_OpenCL_bitselect_v4i16_v4i16_v4i16(ushort4 x, ushort4 y, ushort4 z);
ushort8 __builtin_spirv_OpenCL_bitselect_v8i16_v8i16_v8i16(ushort8 x, ushort8 y, ushort8 z);
ushort16 __builtin_spirv_OpenCL_bitselect_v16i16_v16i16_v16i16(ushort16 x, ushort16 y, ushort16 z);
uint __builtin_spirv_OpenCL_bitselect_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_bitselect_v2i32_v2i32_v2i32(uint2 x, uint2 y, uint2 z);
uint3 __builtin_spirv_OpenCL_bitselect_v3i32_v3i32_v3i32(uint3 x, uint3 y, uint3 z);
uint4 __builtin_spirv_OpenCL_bitselect_v4i32_v4i32_v4i32(uint4 x, uint4 y, uint4 z);
uint8 __builtin_spirv_OpenCL_bitselect_v8i32_v8i32_v8i32(uint8 x, uint8 y, uint8 z);
uint16 __builtin_spirv_OpenCL_bitselect_v16i32_v16i32_v16i32(uint16 x, uint16 y, uint16 z);
ulong __builtin_spirv_OpenCL_bitselect_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_bitselect_v2i64_v2i64_v2i64(ulong2 x, ulong2 y, ulong2 z);
ulong3 __builtin_spirv_OpenCL_bitselect_v3i64_v3i64_v3i64(ulong3 x, ulong3 y, ulong3 z);
ulong4 __builtin_spirv_OpenCL_bitselect_v4i64_v4i64_v4i64(ulong4 x, ulong4 y, ulong4 z);
ulong8 __builtin_spirv_OpenCL_bitselect_v8i64_v8i64_v8i64(ulong8 x, ulong8 y, ulong8 z);
ulong16 __builtin_spirv_OpenCL_bitselect_v16i64_v16i64_v16i64(ulong16 x, ulong16 y, ulong16 z);
float __builtin_spirv_OpenCL_bitselect_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_bitselect_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_bitselect_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_bitselect_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_bitselect_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_bitselect_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_bitselect_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_bitselect_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_bitselect_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_bitselect_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_bitselect_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_bitselect_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_bitselect_f16_f16_f16( half a, half b, half c );
half2 __builtin_spirv_OpenCL_bitselect_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_bitselect_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_bitselect_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_bitselect_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_bitselect_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

uchar __builtin_spirv_OpenCL_select_i8_i8_i8( uchar a, uchar b, uchar c );
ushort __builtin_spirv_OpenCL_select_i16_i16_i16( ushort a, ushort b, ushort c );
uint __builtin_spirv_OpenCL_select_i32_i32_i32( uint a, uint b, uint c );
ulong __builtin_spirv_OpenCL_select_i64_i64_i64( ulong a, ulong b, ulong c );

float __builtin_spirv_OpenCL_select_f32_f32_i32( float a, float b, uint c );
float2 __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32(float2 a, float2 b, uint2 c);
float3 __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32(float3 a, float3 b, uint3 c);
float4 __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32(float4 a, float4 b, uint4 c);
float8 __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32(float8 a, float8 b, uint8 c);
float16 __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32(float16 a, float16 b, uint16 c);

#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_select_f64_f64_i64( double a, double b, ulong c );
double2 __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64(double2 a, double2 b, ulong2 c);
double3 __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64(double3 a, double3 b, ulong3 c);
double4 __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64(double4 a, double4 b, ulong4 c);
double8 __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64(double8 a, double8 b, ulong8 c);
double16 __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64(double16 a, double16 b, ulong16 c);

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __builtin_spirv_OpenCL_select_f16_f16_i16( half a, half b, ushort c );
half2 __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16(half2 a, half2 b, ushort2 c);
half3 __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16(half3 a, half3 b, ushort3 c);
half4 __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16(half4 a, half4 b, ushort4 c);
half8 __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16(half8 a, half8 b, ushort8 c);
half16 __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16(half16 a, half16 b, ushort16 c);

#endif // defined(cl_khr_fp16)

#endif // __SPIRV_MATH_H__

