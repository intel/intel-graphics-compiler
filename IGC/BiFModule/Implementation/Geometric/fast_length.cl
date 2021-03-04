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

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_fast_length_f32(float p ){
    return __builtin_spirv_OpenCL_fabs_f32(p);
}

INLINE float __builtin_spirv_OpenCL_fast_length_v2f32(float2 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v2f32_v2f32, )( p, p ) );
}

INLINE float __builtin_spirv_OpenCL_fast_length_v3f32(float3 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v3f32_v3f32, )( p, p ) );
}

INLINE float __builtin_spirv_OpenCL_fast_length_v4f32(float4 p ){
    return __builtin_spirv_OpenCL_native_sqrt_f32( SPIRV_BUILTIN(Dot, _v4f32_v4f32, )( p, p ) );
}
