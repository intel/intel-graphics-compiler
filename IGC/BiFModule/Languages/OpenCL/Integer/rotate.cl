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


INLINE
char OVERLOADABLE rotate( char v,
                          char i )
{
    return as_char(__builtin_spirv_OpenCL_rotate_i8_i8( as_uchar(v), as_uchar(i) ));
}

INLINE
char2 OVERLOADABLE rotate( char2 v,
                           char2 i )
{
    return as_char2(__builtin_spirv_OpenCL_rotate_v2i8_v2i8( as_uchar2(v), as_uchar2(i) ));
}

INLINE
char3 OVERLOADABLE rotate( char3 v,
                           char3 i )
{
    return as_char3(__builtin_spirv_OpenCL_rotate_v3i8_v3i8( as_uchar3(v), as_uchar3(i) ));
}

INLINE
char4 OVERLOADABLE rotate( char4 v,
                           char4 i )
{
    return as_char4(__builtin_spirv_OpenCL_rotate_v4i8_v4i8( as_uchar4(v), as_uchar4(i) ));
}

INLINE
char8 OVERLOADABLE rotate( char8 v,
                           char8 i )
{
    return as_char8(__builtin_spirv_OpenCL_rotate_v8i8_v8i8( as_uchar8(v), as_uchar8(i) ));
}

INLINE
char16 OVERLOADABLE rotate( char16 v,
                            char16 i )
{
    return as_char16(__builtin_spirv_OpenCL_rotate_v16i8_v16i8( as_uchar16(v), as_uchar16(i) ));
}

INLINE
uchar OVERLOADABLE rotate( uchar v,
                           uchar i )
{
    return __builtin_spirv_OpenCL_rotate_i8_i8( v, i );
}

INLINE
uchar2 OVERLOADABLE rotate( uchar2 v,
                            uchar2 i )
{
    return __builtin_spirv_OpenCL_rotate_v2i8_v2i8( v, i );
}

INLINE
uchar3 OVERLOADABLE rotate( uchar3 v,
                            uchar3 i )
{
    return __builtin_spirv_OpenCL_rotate_v3i8_v3i8( v, i );
}

INLINE
uchar4 OVERLOADABLE rotate( uchar4 v,
                            uchar4 i )
{
    return __builtin_spirv_OpenCL_rotate_v4i8_v4i8( v, i );
}

INLINE
uchar8 OVERLOADABLE rotate( uchar8 v,
                            uchar8 i )
{
    return __builtin_spirv_OpenCL_rotate_v8i8_v8i8( v, i );
}

INLINE
uchar16 OVERLOADABLE rotate( uchar16 v,
                             uchar16 i )
{
    return __builtin_spirv_OpenCL_rotate_v16i8_v16i8( v, i );
}

INLINE
short OVERLOADABLE rotate( short v,
                           short i )
{
    return as_short(__builtin_spirv_OpenCL_rotate_i16_i16( as_ushort(v), as_ushort(i) ));
}

INLINE
short2 OVERLOADABLE rotate( short2 v,
                            short2 i )
{
    return as_short2(__builtin_spirv_OpenCL_rotate_v2i16_v2i16( as_ushort2(v), as_ushort2(i) ));
}

INLINE
short3 OVERLOADABLE rotate( short3 v,
                            short3 i )
{
    return as_short3(__builtin_spirv_OpenCL_rotate_v3i16_v3i16( as_ushort3(v), as_ushort3(i) ));
}

INLINE
short4 OVERLOADABLE rotate( short4 v,
                            short4 i )
{
    return as_short4(__builtin_spirv_OpenCL_rotate_v4i16_v4i16( as_ushort4(v), as_ushort4(i) ));
}

INLINE
short8 OVERLOADABLE rotate( short8 v,
                            short8 i )
{
    return as_short8(__builtin_spirv_OpenCL_rotate_v8i16_v8i16( as_ushort8(v), as_ushort8(i) ));
}

INLINE
short16 OVERLOADABLE rotate( short16 v,
                             short16 i )
{
    return as_short16(__builtin_spirv_OpenCL_rotate_v16i16_v16i16( as_ushort16(v), as_ushort16(i) ));
}

INLINE
ushort OVERLOADABLE rotate( ushort v,
                            ushort i )
{
    return __builtin_spirv_OpenCL_rotate_i16_i16( v, i );
}

INLINE
ushort2 OVERLOADABLE rotate( ushort2 v,
                             ushort2 i )
{
    return __builtin_spirv_OpenCL_rotate_v2i16_v2i16( v, i );
}

INLINE
ushort3 OVERLOADABLE rotate( ushort3 v,
                             ushort3 i )
{
    return __builtin_spirv_OpenCL_rotate_v3i16_v3i16( v, i );
}

INLINE
ushort4 OVERLOADABLE rotate( ushort4 v,
                             ushort4 i )
{
    return __builtin_spirv_OpenCL_rotate_v4i16_v4i16( v, i );
}

INLINE
ushort8 OVERLOADABLE rotate( ushort8 v,
                             ushort8 i )
{
    return __builtin_spirv_OpenCL_rotate_v8i16_v8i16( v, i );
}

INLINE
ushort16 OVERLOADABLE rotate( ushort16 v,
                              ushort16 i )
{
    return __builtin_spirv_OpenCL_rotate_v16i16_v16i16( v, i );
}

INLINE
int OVERLOADABLE rotate( int v,
                         int i )
{
    return as_int(__builtin_spirv_OpenCL_rotate_i32_i32( as_uint(v), as_uint(i) ));
}

INLINE
int2 OVERLOADABLE rotate( int2 v,
                          int2 i )
{
    return as_int2(__builtin_spirv_OpenCL_rotate_v2i32_v2i32( as_uint2(v), as_uint2(i) ));
}

INLINE
int3 OVERLOADABLE rotate( int3 v,
                          int3 i )
{
    return as_int3(__builtin_spirv_OpenCL_rotate_v3i32_v3i32( as_uint3(v), as_uint3(i) ));
}

INLINE
int4 OVERLOADABLE rotate( int4 v,
                          int4 i )
{
    return as_int4(__builtin_spirv_OpenCL_rotate_v4i32_v4i32( as_uint4(v), as_uint4(i) ));
}

INLINE
int8 OVERLOADABLE rotate( int8 v,
                          int8 i )
{
    return as_int8(__builtin_spirv_OpenCL_rotate_v8i32_v8i32( as_uint8(v), as_uint8(i) ));
}

INLINE
int16 OVERLOADABLE rotate( int16 v,
                           int16 i )
{
    return as_int16(__builtin_spirv_OpenCL_rotate_v16i32_v16i32( as_uint16(v), as_uint16(i) ));
}

INLINE
uint OVERLOADABLE rotate( uint v,
                          uint i )
{
    return __builtin_spirv_OpenCL_rotate_i32_i32( v, i );
}

INLINE
uint2 OVERLOADABLE rotate( uint2 v,
                           uint2 i )
{
    return __builtin_spirv_OpenCL_rotate_v2i32_v2i32( v, i );
}

INLINE
uint3 OVERLOADABLE rotate( uint3 v,
                           uint3 i )
{
    return __builtin_spirv_OpenCL_rotate_v3i32_v3i32( v, i );
}

INLINE
uint4 OVERLOADABLE rotate( uint4 v,
                           uint4 i )
{
    return __builtin_spirv_OpenCL_rotate_v4i32_v4i32( v, i );
}

INLINE
uint8 OVERLOADABLE rotate( uint8 v,
                           uint8 i )
{
    return __builtin_spirv_OpenCL_rotate_v8i32_v8i32( v, i );
}

INLINE
uint16 OVERLOADABLE rotate( uint16 v,
                            uint16 i )
{
    return __builtin_spirv_OpenCL_rotate_v16i32_v16i32( v, i );
}

INLINE
long OVERLOADABLE rotate( long v,
                          long i )
{
    return as_long(__builtin_spirv_OpenCL_rotate_i64_i64( as_ulong(v), as_ulong(i) ));
}

INLINE
long2 OVERLOADABLE rotate( long2 v,
                           long2 i )
{
    return as_long2(__builtin_spirv_OpenCL_rotate_v2i64_v2i64( as_ulong2(v), as_ulong2(i) ));
}

INLINE
long3 OVERLOADABLE rotate( long3 v,
                           long3 i )
{
    return as_long3(__builtin_spirv_OpenCL_rotate_v3i64_v3i64( as_ulong3(v), as_ulong3(i) ));
}

INLINE
long4 OVERLOADABLE rotate( long4 v,
                           long4 i )
{
    return as_long4(__builtin_spirv_OpenCL_rotate_v4i64_v4i64( as_ulong4(v), as_ulong4(i) ));
}

INLINE
long8 OVERLOADABLE rotate( long8 v,
                           long8 i )
{
    return as_long8(__builtin_spirv_OpenCL_rotate_v8i64_v8i64( as_ulong8(v), as_ulong8(i) ));
}

INLINE
long16 OVERLOADABLE rotate( long16 v,
                            long16 i )
{
    return as_long16(__builtin_spirv_OpenCL_rotate_v16i64_v16i64( as_ulong16(v), as_ulong16(i) ));
}

INLINE
ulong OVERLOADABLE rotate( ulong v,
                           ulong i )
{
    return __builtin_spirv_OpenCL_rotate_i64_i64( v, i );
}

INLINE
ulong2 OVERLOADABLE rotate( ulong2 v,
                            ulong2 i )
{
    return __builtin_spirv_OpenCL_rotate_v2i64_v2i64( v, i );
}

INLINE
ulong3 OVERLOADABLE rotate( ulong3 v,
                            ulong3 i )
{
    return __builtin_spirv_OpenCL_rotate_v3i64_v3i64( v, i );
}

INLINE
ulong4 OVERLOADABLE rotate( ulong4 v,
                            ulong4 i )
{
    return __builtin_spirv_OpenCL_rotate_v4i64_v4i64( v, i );
}

INLINE
ulong8 OVERLOADABLE rotate( ulong8 v,
                            ulong8 i )
{
    return __builtin_spirv_OpenCL_rotate_v8i64_v8i64( v, i );
}

INLINE
ulong16 OVERLOADABLE rotate( ulong16 v,
                             ulong16 i )
{
    return __builtin_spirv_OpenCL_rotate_v16i64_v16i64( v, i );
}

