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

INLINE float OVERLOADABLE pown( float x, int y )
{
    return __builtin_spirv_OpenCL_pown_f32_i32( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, float, float, int )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE pown( double x, int y )
{
    return __builtin_spirv_OpenCL_pown_f64_i32( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, double, double, int )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE pown( half x, int y )
{
    return __builtin_spirv_OpenCL_pown_f16_i32( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, half, half, int )

#endif // defined(cl_khr_fp16)
