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
#include "spirv.h"

INLINE float OVERLOADABLE mix( float x, float y, float a )
{
    return __builtin_spirv_OpenCL_mix_f32_f32_f32( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, float, float )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE mix( double x, double y, double a )
{
    return __builtin_spirv_OpenCL_mix_f64_f64_f64( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, double, double )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE mix( half x, half y, half a )
{
    return __builtin_spirv_OpenCL_mix_f16_f16_f16( x, y, a );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( mix, half, half )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( mix, half, half, half )

#endif // defined(cl_khr_fp16)
