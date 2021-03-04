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

INLINE INLINE float OVERLOADABLE round( float x )
{
    return __builtin_spirv_OpenCL_round_f32( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, float, float )

#if defined(cl_khr_fp64)

INLINE INLINE double OVERLOADABLE round( double x )
{
    return __builtin_spirv_OpenCL_round_f64( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE INLINE half OVERLOADABLE round( half x )
{
    return __builtin_spirv_OpenCL_round_f16( x );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( round, half, half )

#endif // defined(cl_khr_fp16)
