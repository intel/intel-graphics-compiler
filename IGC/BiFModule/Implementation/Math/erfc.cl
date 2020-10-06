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

float __builtin_spirv_OpenCL_erfc_f32( float x )
{
    // this algorithm is taken from book "Numerical recipes for C" page.221
    float ret;
    float abs_x;
    float f;
    // equation params
    float p1 = -1.26551223f;
    float p2 = 1.00002368f;
    float p3 = 0.37409196f;
    float p4 = 0.09678418f;
    float p5 = -0.18628806f;
    float p6 = 0.27886807f;
    float p7 = -1.13520398f;
    float p8 = 1.48851587f;
    float p9 = -0.82215223f;
    float p10 = 0.17087277f;
    abs_x = __builtin_spirv_OpenCL_fabs_f32(x);
    f = 1.0f / (1.0f + abs_x / 2.0f);
    ret = f * __builtin_spirv_OpenCL_exp_f32(-abs_x * abs_x + p1 + f * (p2 + f * (p3 + f * (p4 + f * (p5 + f * (p6 + f * (p7 + f * (p8 + f * (p9 + f * p10)))))))));
    return x >= 0.0f ? ret : 2.0f - ret;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_erfc_f64( double x )
{
    return libclc_erfc_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_erfc_f16( half x )
{
    return __builtin_spirv_OpenCL_erfc_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, half, half, f16 )

#endif // defined(cl_khr_fp16)
