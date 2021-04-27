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

INLINE float __builtin_spirv_OpenCL_nan_i32( uint nancode )
{
    return as_float( FLOAT_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, float, uint, i32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_nan_i64( ulong nancode )
{
    return as_double( DOUBLE_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, double, ulong, i64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_nan_i16( ushort nancode )
{
    return as_half( HALF_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, half, ushort, i16 )

#endif // defined(cl_khr_fp16)
