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

INLINE float __builtin_spirv_OpenCL_minmag_f32_f32( float x, float y )
{
    float fx = __builtin_spirv_OpenCL_fabs_f32(x);
    float fy = __builtin_spirv_OpenCL_fabs_f32(y);
    float m = __builtin_spirv_OpenCL_fmin_f32_f32(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_minmag_f64_f64( double x, double y )
{
    double fx = __builtin_spirv_OpenCL_fabs_f64(x);
    double fy = __builtin_spirv_OpenCL_fabs_f64(y);
    double m = __builtin_spirv_OpenCL_fmin_f64_f64(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_minmag_f16_f16( half x, half y )
{
    half fx = __builtin_spirv_OpenCL_fabs_f16(x);
    half fy = __builtin_spirv_OpenCL_fabs_f16(y);
    half m = __builtin_spirv_OpenCL_fmin_f16_f16(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, half, half, f16 )

#endif // defined(cl_khr_fp16)
