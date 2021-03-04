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
uchar OVERLOADABLE clz( uchar x )
{
    return __builtin_spirv_OpenCL_clz_i8( x );
}

INLINE
ushort OVERLOADABLE clz( ushort x )
{
    return __builtin_spirv_OpenCL_clz_i16( x );
}

INLINE
uint OVERLOADABLE clz( uint x )
{
    return __builtin_spirv_OpenCL_clz_i32( x );
}

INLINE
char OVERLOADABLE clz( char x )
{
    return as_char(__builtin_spirv_OpenCL_clz_i8( as_uchar(x) ));
}

INLINE
char2 OVERLOADABLE clz( char2 x )
{
    return as_char2(__builtin_spirv_OpenCL_clz_v2i8( as_uchar2(x) ));
}

INLINE
char3 OVERLOADABLE clz( char3 x )
{
    return as_char3(__builtin_spirv_OpenCL_clz_v3i8( as_uchar3(x) ));
}

INLINE
char4 OVERLOADABLE clz( char4 x )
{
    return as_char4(__builtin_spirv_OpenCL_clz_v4i8( as_uchar4(x) ));
}

INLINE
char8 OVERLOADABLE clz( char8 x )
{
    return as_char8(__builtin_spirv_OpenCL_clz_v8i8( as_uchar8(x) ));
}

INLINE
char16 OVERLOADABLE clz( char16 x )
{
    return as_char16(__builtin_spirv_OpenCL_clz_v16i8( as_uchar16(x) ));
}



INLINE
uchar2 OVERLOADABLE clz( uchar2 x )
{
    return __builtin_spirv_OpenCL_clz_v2i8( x );
}

INLINE
uchar3 OVERLOADABLE clz( uchar3 x )
{
    return __builtin_spirv_OpenCL_clz_v3i8( x );
}

INLINE
uchar4 OVERLOADABLE clz( uchar4 x )
{
    return __builtin_spirv_OpenCL_clz_v4i8( x );
}

INLINE
uchar8 OVERLOADABLE clz( uchar8 x )
{
    return __builtin_spirv_OpenCL_clz_v8i8( x );
}

INLINE
uchar16 OVERLOADABLE clz( uchar16 x )
{
    return __builtin_spirv_OpenCL_clz_v16i8( x );
}

INLINE
short OVERLOADABLE clz( short x )
{
    return as_short(__builtin_spirv_OpenCL_clz_i16( as_ushort(x) ));
}

INLINE
short2 OVERLOADABLE clz( short2 x )
{
    return as_short2(__builtin_spirv_OpenCL_clz_v2i16( as_ushort2(x) ));
}

INLINE
short3 OVERLOADABLE clz( short3 x )
{
    return as_short3(__builtin_spirv_OpenCL_clz_v3i16( as_ushort3(x) ));
}

INLINE
short4 OVERLOADABLE clz( short4 x )
{
    return as_short4(__builtin_spirv_OpenCL_clz_v4i16( as_ushort4(x) ));
}

INLINE
short8 OVERLOADABLE clz( short8 x )
{
    return as_short8(__builtin_spirv_OpenCL_clz_v8i16( as_ushort8(x) ));
}

INLINE
short16 OVERLOADABLE clz( short16 x )
{
    return as_short16(__builtin_spirv_OpenCL_clz_v16i16( as_ushort16(x) ));
}



INLINE
ushort2 OVERLOADABLE clz( ushort2 x )
{
    return __builtin_spirv_OpenCL_clz_v2i16( x );
}

INLINE
ushort3 OVERLOADABLE clz( ushort3 x )
{
    return __builtin_spirv_OpenCL_clz_v3i16( x );
}

INLINE
ushort4 OVERLOADABLE clz( ushort4 x )
{
    return __builtin_spirv_OpenCL_clz_v4i16( x );
}

INLINE
ushort8 OVERLOADABLE clz( ushort8 x )
{
    return __builtin_spirv_OpenCL_clz_v8i16( x );
}

INLINE
ushort16 OVERLOADABLE clz( ushort16 x )
{
    return __builtin_spirv_OpenCL_clz_v16i16( x );
}

INLINE
int OVERLOADABLE clz( int x )
{
    return as_int(__builtin_spirv_OpenCL_clz_i32( as_uint(x) ));
}

INLINE
int2 OVERLOADABLE clz( int2 x )
{
    return as_int2(__builtin_spirv_OpenCL_clz_v2i32( as_uint2(x) ));
}

INLINE
int3 OVERLOADABLE clz( int3 x )
{
    return as_int3(__builtin_spirv_OpenCL_clz_v3i32( as_uint3(x) ));
}

INLINE
int4 OVERLOADABLE clz( int4 x )
{
    return as_int4(__builtin_spirv_OpenCL_clz_v4i32( as_uint4(x) ));
}

INLINE
int8 OVERLOADABLE clz( int8 x )
{
    return as_int8(__builtin_spirv_OpenCL_clz_v8i32( as_uint8(x) ));
}

INLINE
int16 OVERLOADABLE clz( int16 x )
{
    return as_int16(__builtin_spirv_OpenCL_clz_v16i32( as_uint16(x) ));
}



INLINE
uint2 OVERLOADABLE clz( uint2 x )
{
    return __builtin_spirv_OpenCL_clz_v2i32( x );
}

INLINE
uint3 OVERLOADABLE clz( uint3 x )
{
    return __builtin_spirv_OpenCL_clz_v3i32( x );
}

INLINE
uint4 OVERLOADABLE clz( uint4 x )
{
    return __builtin_spirv_OpenCL_clz_v4i32( x );
}

INLINE
uint8 OVERLOADABLE clz( uint8 x )
{
    return __builtin_spirv_OpenCL_clz_v8i32( x );
}

INLINE
uint16 OVERLOADABLE clz( uint16 x )
{
    return __builtin_spirv_OpenCL_clz_v16i32( x );
}

INLINE
long OVERLOADABLE clz( long x )
{
    return as_long(__builtin_spirv_OpenCL_clz_i64( as_ulong(x) ));
}

INLINE
long2 OVERLOADABLE clz( long2 x )
{
    return as_long2(__builtin_spirv_OpenCL_clz_v2i64( as_ulong2(x) ));
}

INLINE
long3 OVERLOADABLE clz( long3 x )
{
    return as_long3(__builtin_spirv_OpenCL_clz_v3i64( as_ulong3(x) ));
}

INLINE
long4 OVERLOADABLE clz( long4 x )
{
    return as_long4(__builtin_spirv_OpenCL_clz_v4i64( as_ulong4(x) ));
}

INLINE
long8 OVERLOADABLE clz( long8 x )
{
    return as_long8(__builtin_spirv_OpenCL_clz_v8i64( as_ulong8(x) ));
}

INLINE
long16 OVERLOADABLE clz( long16 x )
{
    return as_long16(__builtin_spirv_OpenCL_clz_v16i64( as_ulong16(x) ));
}

INLINE
ulong OVERLOADABLE clz( ulong x )
{
    return __builtin_spirv_OpenCL_clz_i64( x );
}

INLINE
ulong2 OVERLOADABLE clz( ulong2 x )
{
    return __builtin_spirv_OpenCL_clz_v2i64( x );
}

INLINE
ulong3 OVERLOADABLE clz( ulong3 x )
{
    return __builtin_spirv_OpenCL_clz_v3i64( x );
}

INLINE
ulong4 OVERLOADABLE clz( ulong4 x )
{
    return __builtin_spirv_OpenCL_clz_v4i64( x );
}

INLINE
ulong8 OVERLOADABLE clz( ulong8 x )
{
    return __builtin_spirv_OpenCL_clz_v8i64( x );
}

INLINE
ulong16 OVERLOADABLE clz( ulong16 x )
{
    return __builtin_spirv_OpenCL_clz_v16i64( x );
}

