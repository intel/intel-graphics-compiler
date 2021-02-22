/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_distance_f32_f32(float p0, float p1 ){
    return __builtin_spirv_OpenCL_length_f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_distance_v2f32_v2f32(float2 p0, float2 p1 ){
    return __builtin_spirv_OpenCL_length_v2f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_distance_v3f32_v3f32(float3 p0, float3 p1 ){
    return __builtin_spirv_OpenCL_length_v3f32( p0 - p1 );
}

INLINE float __builtin_spirv_OpenCL_distance_v4f32_v4f32(float4 p0, float4 p1 ){
    return __builtin_spirv_OpenCL_length_v4f32( p0 - p1 );
}

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_distance_f64_f64(double p0, double p1 ){
    return __builtin_spirv_OpenCL_length_f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v2f64_v2f64(double2 p0, double2 p1 ){
    return __builtin_spirv_OpenCL_length_v2f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v3f64_v3f64(double3 p0, double3 p1 ){
    return __builtin_spirv_OpenCL_length_v3f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v4f64_v4f64(double4 p0, double4 p1 ){
    return __builtin_spirv_OpenCL_length_v4f64( p0 - p1 );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_distance_f16_f16(half p0, half p1 ){
    return __builtin_spirv_OpenCL_length_f16( p0 - p1 );
}

INLINE half __builtin_spirv_OpenCL_distance_v2f16_v2f16(half2 p0, half2 p1 ){
    return __builtin_spirv_OpenCL_length_v2f16( p0 - p1 );
}

INLINE half __builtin_spirv_OpenCL_distance_v3f16_v3f16(half3 p0, half3 p1 ){
    return __builtin_spirv_OpenCL_length_v3f16( p0 - p1 );
}

INLINE half __builtin_spirv_OpenCL_distance_v4f16_v4f16(half4 p0, half4 p1 ){
    return __builtin_spirv_OpenCL_length_v4f16( p0 - p1 );
}

#endif // defined(cl_khr_fp16)

