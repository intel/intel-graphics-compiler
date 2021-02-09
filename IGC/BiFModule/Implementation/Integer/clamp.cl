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

INLINE char __builtin_spirv_OpenCL_s_clamp_i8_i8_i8( char x, char minval, char maxval )
{
    return __builtin_spirv_OpenCL_s_min_i8_i8(__builtin_spirv_OpenCL_s_max_i8_i8(x, minval), maxval);
}

INLINE uchar __builtin_spirv_OpenCL_u_clamp_i8_i8_i8( uchar x, uchar minval, uchar maxval )
{
    return __builtin_spirv_OpenCL_u_min_i8_i8(__builtin_spirv_OpenCL_u_max_i8_i8(x, minval), maxval);
}

INLINE short __builtin_spirv_OpenCL_s_clamp_i16_i16_i16( short x, short minval, short maxval )
{
    return __builtin_spirv_OpenCL_s_min_i16_i16(__builtin_spirv_OpenCL_s_max_i16_i16(x, minval), maxval);
}

INLINE ushort __builtin_spirv_OpenCL_u_clamp_i16_i16_i16( ushort x, ushort minval, ushort maxval )
{
    return __builtin_spirv_OpenCL_u_min_i16_i16(__builtin_spirv_OpenCL_u_max_i16_i16(x, minval), maxval);
}

INLINE int __builtin_spirv_OpenCL_s_clamp_i32_i32_i32( int x, int minval, int maxval )
{
    return __builtin_spirv_OpenCL_s_min_i32_i32(__builtin_spirv_OpenCL_s_max_i32_i32(x, minval), maxval);
}

INLINE uint __builtin_spirv_OpenCL_u_clamp_i32_i32_i32( uint x, uint minval, uint maxval )
{
    return __builtin_spirv_OpenCL_u_min_i32_i32(__builtin_spirv_OpenCL_u_max_i32_i32(x, minval), maxval);
}

INLINE long __builtin_spirv_OpenCL_s_clamp_i64_i64_i64( long x, long minval, long maxval )
{
    return __builtin_spirv_OpenCL_s_min_i64_i64(__builtin_spirv_OpenCL_s_max_i64_i64(x, minval), maxval);
}

INLINE ulong __builtin_spirv_OpenCL_u_clamp_i64_i64_i64( ulong x, ulong minval, ulong maxval )
{
    return __builtin_spirv_OpenCL_u_min_i64_i64(__builtin_spirv_OpenCL_u_max_i64_i64(x, minval), maxval);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, char, char, i8 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, uchar, uchar, i8 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, short, short, i16 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, ushort, ushort, i16 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, int, int, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, uint, uint, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, long, long, i64 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, ulong, ulong, i64 )

