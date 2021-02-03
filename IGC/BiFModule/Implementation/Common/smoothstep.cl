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

INLINE float __builtin_spirv_OpenCL_smoothstep_f32_f32_f32(float edge0, float edge1, float x ){
    float div = (x - edge0) / (edge1 - edge0);
    float temp = __builtin_spirv_OpenCL_fclamp_f32_f32_f32( div, 0.0f, 1.0f );
    return temp * temp * __builtin_spirv_OpenCL_mad_f32_f32_f32( -2.0f, temp, 3.0f );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_smoothstep_f16_f16_f16(half edge0, half edge1, half x ){
    half div = (x - edge0) / (edge1 - edge0);
    half temp = __builtin_spirv_OpenCL_fclamp_f16_f16_f16( div, (half)0.0f, (half)1.0f );
    return temp * temp * __builtin_spirv_OpenCL_mad_f16_f16_f16( (half)-2.0f, temp, (half)3.0f );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, half, half, f16 )

#endif // defined(cl_khr_fp16)
