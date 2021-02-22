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
#include "../../Headers/spirv.h"

INLINE uchar __builtin_spirv_OpenCL_select_i8_i8_i8( uchar a, uchar b, uchar c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE uchar __intel_vector_select_helper( uchar a, uchar b, uchar c )
{
    return as_char(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uchar, uchar, i8, i8 )

INLINE ushort __builtin_spirv_OpenCL_select_i16_i16_i16( ushort a, ushort b, ushort c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE ushort __intel_vector_select_helper( ushort a, ushort b, ushort c )
{
    return as_short(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ushort, ushort, i16, i16 )

INLINE uint __builtin_spirv_OpenCL_select_i32_i32_i32( uint a, uint b, uint c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE uint __intel_vector_select_helper( uint a, uint b, uint c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uint, uint, i32, i32 )

INLINE ulong __builtin_spirv_OpenCL_select_i64_i64_i64( ulong a, ulong b, ulong c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE ulong __intel_vector_select_helper( ulong a, ulong b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ulong, ulong, i64, i64 )

INLINE float __builtin_spirv_OpenCL_select_f32_f32_i32( float a, float b, uint c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE float __intel_vector_select_helper( float a, float b, uint c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, float, uint, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_select_f64_f64_i64( double a, double b, ulong c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE double __intel_vector_select_helper( double a, double b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, double, ulong, f64, i64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_select_f16_f16_i16( half a, half b, ushort c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE half __intel_vector_select_helper( half a, half b, ushort c )
{
    return as_short(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, half, ushort, f16, i16 )

#endif // defined(cl_khr_fp16)
