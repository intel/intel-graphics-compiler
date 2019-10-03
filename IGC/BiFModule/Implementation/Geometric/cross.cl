/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float3 __builtin_spirv_OpenCL_cross_v3f32_v3f32(float3 p0, float3 p1 ){
    float3 result;
    result.x = __builtin_spirv_OpenCL_fma_f32_f32_f32(p0.y, p1.z, -p0.z * p1.y );
    result.y = __builtin_spirv_OpenCL_fma_f32_f32_f32(p0.z, p1.x, -p0.x * p1.z );
    result.z = __builtin_spirv_OpenCL_fma_f32_f32_f32(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE float4 __builtin_spirv_OpenCL_cross_v4f32_v4f32(float4 p0, float4 p1 ){
    float4 result;
    result.xyz = __builtin_spirv_OpenCL_cross_v3f32_v3f32( p0.xyz, p1.xyz );
    result.w = 0.0f;

    return result;
}

#if defined(cl_khr_fp64)

INLINE double3 __builtin_spirv_OpenCL_cross_v3f64_v3f64(double3 p0, double3 p1 ){
    double3 result;
    result.x = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.y, p1.z, -p0.z * p1.y );
    result.y = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.z, p1.x, -p0.x * p1.z );
    result.z = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE double4 __builtin_spirv_OpenCL_cross_v4f64_v4f64(double4 p0, double4 p1 ){
    double4 result;
    result.xyz = __builtin_spirv_OpenCL_cross_v3f64_v3f64( p0.xyz, p1.xyz );

    result.w = (half) 0.0;

    return result;
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half3 __builtin_spirv_OpenCL_cross_v3f16_v3f16(half3 p0, half3 p1 ){
    float3 ret = __builtin_spirv_OpenCL_cross_v3f32_v3f32(__builtin_spirv_OpFConvert_v3f32_v3f16(p0), __builtin_spirv_OpFConvert_v3f32_v3f16(p1));
    return __builtin_spirv_OpFConvert_v3f16_v3f32(ret);
}

INLINE half4 __builtin_spirv_OpenCL_cross_v4f16_v4f16(half4 p0, half4 p1 ){
    half4 result;
    result.xyz = __builtin_spirv_OpenCL_cross_v3f16_v3f16( p0.xyz, p1.xyz );
    result.w = (half)0.0f;

    return result;
}

#endif // defined(cl_khr_fp16)
