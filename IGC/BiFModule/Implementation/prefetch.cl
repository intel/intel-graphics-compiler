/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//Prefetch function

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i8_i32, )( global char* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i32, )( global char2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i32, )( global char3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i32, )( global char4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i32, )( global char8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i32, )( global char16* p, int num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i16_i32, )( global short* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i32, )( global short2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i32, )( global short3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i32, )( global short4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i32, )( global short8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i32, )( global short16* p, int num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i32_i32, )( global int* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i32, )( global int2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i32, )( global int3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i32, )( global int4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i32, )( global int8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i32, )( global int16* p, int num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i64_i32, )( global long* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i32, )( global long2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i32, )( global long3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i32, )( global long4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i32, )( global long8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i32, )( global long16* p, int num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f32_i32, )( global float* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i32, )( global float2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i32, )( global float3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i32, )( global float4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i32, )( global float8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i32, )( global float16* p, int num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f16_i32, )( global half* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i32, )( global half2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i32, )( global half3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i32, )( global half4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i32, )( global half8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i32, )( global half16* p, int num_elements)
{
}


#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f64_i32, )( global double* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i32, )( global double2* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i32, )( global double3* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i32, )( global double4* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i32, )( global double8* p, int num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i32, )( global double16* p, int num_elements)
{
}
#endif // defined(cl_khr_fp64)


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i8_i64, )( global char* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i64, )( global char2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i64, )( global char3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i64, )( global char4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i64, )( global char8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i64, )( global char16* p, long num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i16_i64, )( global short* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i64, )( global short2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i64, )( global short3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i64, )( global short4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i64, )( global short8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i64, )( global short16* p, long num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i32_i64, )( global int* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i64, )( global int2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i64, )( global int3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i64, )( global int4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i64, )( global int8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i64, )( global int16* p, long num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1i64_i64, )( global long* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i64, )( global long2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i64, )( global long3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i64, )( global long4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i64, )( global long8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i64, )( global long16* p, long num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f32_i64, )( global float* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i64, )( global float2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i64, )( global float3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i64, )( global float4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i64, )( global float8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i64, )( global float16* p, long num_elements)
{
}


void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f16_i64, )( global half* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i64, )( global half2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i64, )( global half3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i64, )( global half4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i64, )( global half8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i64, )( global half16* p, long num_elements)
{
}


#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1f64_i64, )( global double* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i64, )( global double2* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i64, )( global double3* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i64, )( global double4* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i64, )( global double8* p, long num_elements)
{
}

void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i64, )( global double16* p, long num_elements)
{
}
#endif // defined(cl_khr_fp64)

