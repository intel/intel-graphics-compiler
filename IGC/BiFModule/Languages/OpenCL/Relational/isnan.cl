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

INLINE int OVERLOADABLE isnan( float x )
{
    return SPIRV_BUILTIN(IsNan, _f32, )( x );
}

static INLINE int OVERLOADABLE __intel_vector_isnan_helper( float x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, int, float )

#if defined(cl_khr_fp64)

INLINE int OVERLOADABLE isnan( double x )
{
    return SPIRV_BUILTIN(IsNan, _f64, )( x );
}

static INLINE long OVERLOADABLE __intel_vector_isnan_helper( double x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, long, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE isnan( half x )
{
    return SPIRV_BUILTIN(IsNan, _f16, )( x );
}

static INLINE short OVERLOADABLE __intel_vector_isnan_helper( half x )
{
    return x != x ? -1 : 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( isnan, __intel_vector_isnan_helper, short, half )

#endif // defined(cl_khr_fp16)
