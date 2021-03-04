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
#include "spirv.h"

INLINE float OVERLOADABLE smoothstep( float edge0, float edge1, float x )
{
    return __builtin_spirv_OpenCL_smoothstep_f32_f32_f32( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, float, float )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE smoothstep( double edge0, double edge1, double x )
{
    return __builtin_spirv_OpenCL_smoothstep_f64_f64_f64( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, double, double )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE smoothstep( half edge0, half edge1, half x )
{
    return __builtin_spirv_OpenCL_smoothstep_f16_f16_f16( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, half, half )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, half, half, half )

#endif // defined(cl_khr_fp16)
