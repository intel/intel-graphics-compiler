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
uchar2 OVERLOADABLE abs( char2 x )
{
    return __builtin_spirv_OpenCL_s_abs_v2i8( x );
}

INLINE
uchar3 OVERLOADABLE abs( char3 x )
{
    return __builtin_spirv_OpenCL_s_abs_v3i8( x );
}

INLINE
uchar4 OVERLOADABLE abs( char4 x )
{
    return __builtin_spirv_OpenCL_s_abs_v4i8( x );
}

INLINE
uchar8 OVERLOADABLE abs( char8 x )
{
    return __builtin_spirv_OpenCL_s_abs_v8i8( x );
}

INLINE
uchar16 OVERLOADABLE abs( char16 x )
{
    return __builtin_spirv_OpenCL_s_abs_v16i8( x );
}

INLINE
uchar OVERLOADABLE abs( uchar x )
{
    return __builtin_spirv_OpenCL_u_abs_i8( x );
}

INLINE
uchar2 OVERLOADABLE abs( uchar2 x )
{
    return __builtin_spirv_OpenCL_u_abs_v2i8( x );
}

INLINE
uchar3 OVERLOADABLE abs( uchar3 x )
{
    return __builtin_spirv_OpenCL_u_abs_v3i8( x );
}

INLINE
uchar4 OVERLOADABLE abs( uchar4 x )
{
    return __builtin_spirv_OpenCL_u_abs_v4i8( x );
}

INLINE
uchar8 OVERLOADABLE abs( uchar8 x )
{
    return __builtin_spirv_OpenCL_u_abs_v8i8( x );
}

INLINE
uchar16 OVERLOADABLE abs( uchar16 x )
{
    return __builtin_spirv_OpenCL_u_abs_v16i8( x );
}



INLINE
ushort2 OVERLOADABLE abs( short2 x )
{
    return __builtin_spirv_OpenCL_s_abs_v2i16( x );
}

INLINE
ushort3 OVERLOADABLE abs( short3 x )
{
    return __builtin_spirv_OpenCL_s_abs_v3i16( x );
}

INLINE
ushort4 OVERLOADABLE abs( short4 x )
{
    return __builtin_spirv_OpenCL_s_abs_v4i16( x );
}

INLINE
ushort8 OVERLOADABLE abs( short8 x )
{
    return __builtin_spirv_OpenCL_s_abs_v8i16( x );
}

INLINE
ushort16 OVERLOADABLE abs( short16 x )
{
    return __builtin_spirv_OpenCL_s_abs_v16i16( x );
}

INLINE
ushort OVERLOADABLE abs( ushort x )
{
    return __builtin_spirv_OpenCL_u_abs_i16( x );
}

INLINE
ushort2 OVERLOADABLE abs( ushort2 x )
{
    return __builtin_spirv_OpenCL_u_abs_v2i16( x );
}

INLINE
ushort3 OVERLOADABLE abs( ushort3 x )
{
    return __builtin_spirv_OpenCL_u_abs_v3i16( x );
}

INLINE
ushort4 OVERLOADABLE abs( ushort4 x )
{
    return __builtin_spirv_OpenCL_u_abs_v4i16( x );
}

INLINE
ushort8 OVERLOADABLE abs( ushort8 x )
{
    return __builtin_spirv_OpenCL_u_abs_v8i16( x );
}

INLINE
ushort16 OVERLOADABLE abs( ushort16 x )
{
    return __builtin_spirv_OpenCL_u_abs_v16i16( x );
}



INLINE
uint2 OVERLOADABLE abs( int2 x )
{
    return __builtin_spirv_OpenCL_s_abs_v2i32( x );
}

INLINE
uint3 OVERLOADABLE abs( int3 x )
{
    return __builtin_spirv_OpenCL_s_abs_v3i32( x );
}

INLINE
uint4 OVERLOADABLE abs( int4 x )
{
    return __builtin_spirv_OpenCL_s_abs_v4i32( x );
}

INLINE
uint8 OVERLOADABLE abs( int8 x )
{
    return __builtin_spirv_OpenCL_s_abs_v8i32( x );
}

INLINE
uint16 OVERLOADABLE abs( int16 x )
{
    return __builtin_spirv_OpenCL_s_abs_v16i32( x );
}

INLINE
uint OVERLOADABLE abs( uint x )
{
    return __builtin_spirv_OpenCL_u_abs_i32( x );
}

INLINE
uint2 OVERLOADABLE abs( uint2 x )
{
    return __builtin_spirv_OpenCL_u_abs_v2i32( x );
}

INLINE
uint3 OVERLOADABLE abs( uint3 x )
{
    return __builtin_spirv_OpenCL_u_abs_v3i32( x );
}

INLINE
uint4 OVERLOADABLE abs( uint4 x )
{
    return __builtin_spirv_OpenCL_u_abs_v4i32( x );
}

INLINE
uint8 OVERLOADABLE abs( uint8 x )
{
    return __builtin_spirv_OpenCL_u_abs_v8i32( x );
}

INLINE
uint16 OVERLOADABLE abs( uint16 x )
{
    return __builtin_spirv_OpenCL_u_abs_v16i32( x );
}

INLINE
ulong OVERLOADABLE abs( long x )
{
    return __builtin_spirv_OpenCL_s_abs_i64( x );
}

INLINE
ulong2 OVERLOADABLE abs( long2 x )
{
    return __builtin_spirv_OpenCL_s_abs_v2i64( x );
}

INLINE
ulong3 OVERLOADABLE abs( long3 x )
{
    return __builtin_spirv_OpenCL_s_abs_v3i64( x );
}

INLINE
ulong4 OVERLOADABLE abs( long4 x )
{
    return __builtin_spirv_OpenCL_s_abs_v4i64( x );
}

INLINE
ulong8 OVERLOADABLE abs( long8 x )
{
    return __builtin_spirv_OpenCL_s_abs_v8i64( x );
}

INLINE
ulong16 OVERLOADABLE abs( long16 x )
{
    return __builtin_spirv_OpenCL_s_abs_v16i64( x );
}

INLINE
ulong OVERLOADABLE abs( ulong x )
{
    return __builtin_spirv_OpenCL_u_abs_i64( x );
}

INLINE
ulong2 OVERLOADABLE abs( ulong2 x )
{
    return __builtin_spirv_OpenCL_u_abs_v2i64( x );
}

INLINE
ulong3 OVERLOADABLE abs( ulong3 x )
{
    return __builtin_spirv_OpenCL_u_abs_v3i64( x );
}

INLINE
ulong4 OVERLOADABLE abs( ulong4 x )
{
    return __builtin_spirv_OpenCL_u_abs_v4i64( x );
}

INLINE
ulong8 OVERLOADABLE abs( ulong8 x )
{
    return __builtin_spirv_OpenCL_u_abs_v8i64( x );
}

INLINE
ulong16 OVERLOADABLE abs( ulong16 x )
{
    return __builtin_spirv_OpenCL_u_abs_v16i64( x );
}

