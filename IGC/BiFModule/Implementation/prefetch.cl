/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

//Prefetch function

void __builtin_spirv_OpenCL_prefetch_p1i8_i32(const global uchar* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i8_i32(const global uchar2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i8_i32(const global uchar3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i8_i32(const global uchar4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i8_i32(const global uchar8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i8_i32(const global uchar16* p, uint num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i16_i32(const global ushort* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i16_i32(const global ushort2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i16_i32(const global ushort3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i16_i32(const global ushort4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i16_i32(const global ushort8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i16_i32(const global ushort16* p, uint num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i32_i32(const global uint* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i32_i32(const global uint2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i32_i32(const global uint3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i32_i32(const global uint4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i32_i32(const global uint8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i32_i32(const global uint16* p, uint num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i64_i32(const global ulong* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i64_i32(const global ulong2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i64_i32(const global ulong3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i64_i32(const global ulong4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i64_i32(const global ulong8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i64_i32(const global ulong16* p, uint num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1f32_i32(const global float* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f32_i32(const global float2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f32_i32(const global float3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f32_i32(const global float4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f32_i32(const global float8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f32_i32(const global float16* p, uint num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1f16_i32(const global half* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f16_i32(const global half2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f16_i32(const global half3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f16_i32(const global half4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f16_i32(const global half8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f16_i32(const global half16* p, uint num_elements)
{
}


#if defined(cl_khr_fp64)
void __builtin_spirv_OpenCL_prefetch_p1f64_i32(const global double* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f64_i32(const global double2* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f64_i32(const global double3* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f64_i32(const global double4* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f64_i32(const global double8* p, uint num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f64_i32(const global double16* p, uint num_elements)
{
}
#endif // defined(cl_khr_fp64)


void __builtin_spirv_OpenCL_prefetch_p1i8_i64(const global uchar* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i8_i64(const global uchar2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i8_i64(const global uchar3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i8_i64(const global uchar4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i8_i64(const global uchar8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i8_i64(const global uchar16* p, ulong num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i16_i64(const global ushort* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i16_i64(const global ushort2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i16_i64(const global ushort3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i16_i64(const global ushort4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i16_i64(const global ushort8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i16_i64(const global ushort16* p, ulong num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i32_i64(const global uint* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i32_i64(const global uint2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i32_i64(const global uint3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i32_i64(const global uint4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i32_i64(const global uint8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i32_i64(const global uint16* p, ulong num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1i64_i64(const global ulong* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2i64_i64(const global ulong2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3i64_i64(const global ulong3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4i64_i64(const global ulong4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8i64_i64(const global ulong8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16i64_i64(const global ulong16* p, ulong num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1f32_i64(const global float* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f32_i64(const global float2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f32_i64(const global float3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f32_i64(const global float4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f32_i64(const global float8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f32_i64(const global float16* p, ulong num_elements)
{
}


void __builtin_spirv_OpenCL_prefetch_p1f16_i64(const global half* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f16_i64(const global half2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f16_i64(const global half3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f16_i64(const global half4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f16_i64(const global half8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f16_i64(const global half16* p, ulong num_elements)
{
}


#if defined(cl_khr_fp64)
void __builtin_spirv_OpenCL_prefetch_p1f64_i64(const global double* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v2f64_i64(const global double2* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v3f64_i64(const global double3* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v4f64_i64(const global double4* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v8f64_i64(const global double8* p, ulong num_elements)
{
}

void __builtin_spirv_OpenCL_prefetch_p1v16f64_i64(const global double16* p, ulong num_elements)
{
}
#endif // defined(cl_khr_fp64)

