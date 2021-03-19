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

INLINE int OVERLOADABLE isfinite( float x )
{
    return __builtin_spirv_OpIsFinite_f32( x );
}

static INLINE int OVERLOADABLE __intel_vector_isfinite_helper( float x )
{
    return fabs(x) < (float)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isfinite, __intel_vector_isfinite_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isfinite( double x )
{
    return __builtin_spirv_OpIsFinite_f64( x );
}

static INLINE long OVERLOADABLE __intel_vector_isfinite_helper( double x )
{
    return fabs(x) < (double)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isfinite, __intel_vector_isfinite_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isfinite( half x )
{
    return __builtin_spirv_OpIsFinite_f16( x );
}

static INLINE short OVERLOADABLE __intel_vector_isfinite_helper( half x )
{
    return fabs(x) < (half)(INFINITY) ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isfinite, __intel_vector_isfinite_helper, short, half )

#endif // defined(cl_khr_fp16)
