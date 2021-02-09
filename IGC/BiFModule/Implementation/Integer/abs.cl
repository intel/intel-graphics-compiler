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


INLINE
uchar2 __builtin_spirv_OpenCL_s_abs_v2i8( char2 x )
{
    uchar2 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i8(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i8(x.s1);
    return temp;
}

INLINE
uchar3 __builtin_spirv_OpenCL_s_abs_v3i8( char3 x )
{
    uchar3 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i8(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i8(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i8(x.s2);
    return temp;
}

INLINE
uchar4 __builtin_spirv_OpenCL_s_abs_v4i8( char4 x )
{
    uchar4 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i8(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i8(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i8(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i8(x.s3);
    return temp;
}

INLINE
uchar8 __builtin_spirv_OpenCL_s_abs_v8i8( char8 x )
{
    uchar8 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i8(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i8(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i8(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i8(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i8(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i8(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i8(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i8(x.s7);
    return temp;
}

INLINE
uchar16 __builtin_spirv_OpenCL_s_abs_v16i8( char16 x )
{
    uchar16 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i8(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i8(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i8(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i8(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i8(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i8(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i8(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i8(x.s7);
    temp.s8 = __builtin_spirv_OpenCL_s_abs_i8(x.s8);
    temp.s9 = __builtin_spirv_OpenCL_s_abs_i8(x.s9);
    temp.sa = __builtin_spirv_OpenCL_s_abs_i8(x.sa);
    temp.sb = __builtin_spirv_OpenCL_s_abs_i8(x.sb);
    temp.sc = __builtin_spirv_OpenCL_s_abs_i8(x.sc);
    temp.sd = __builtin_spirv_OpenCL_s_abs_i8(x.sd);
    temp.se = __builtin_spirv_OpenCL_s_abs_i8(x.se);
    temp.sf = __builtin_spirv_OpenCL_s_abs_i8(x.sf);
    return temp;
}

INLINE
uchar __builtin_spirv_OpenCL_u_abs_i8( uchar x )
{
    return x;
}

INLINE
uchar2 __builtin_spirv_OpenCL_u_abs_v2i8( uchar2 x )
{
    return x;
}

INLINE
uchar3 __builtin_spirv_OpenCL_u_abs_v3i8( uchar3 x )
{
    return x;
}

INLINE
uchar4 __builtin_spirv_OpenCL_u_abs_v4i8( uchar4 x )
{
    return x;
}

INLINE
uchar8 __builtin_spirv_OpenCL_u_abs_v8i8( uchar8 x )
{
    return x;
}

INLINE
uchar16 __builtin_spirv_OpenCL_u_abs_v16i8( uchar16 x )
{
    return x;
}



INLINE
ushort2 __builtin_spirv_OpenCL_s_abs_v2i16( short2 x )
{
    ushort2 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i16(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i16(x.s1);
    return temp;
}

INLINE
ushort3 __builtin_spirv_OpenCL_s_abs_v3i16( short3 x )
{
    ushort3 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i16(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i16(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i16(x.s2);
    return temp;
}

INLINE
ushort4 __builtin_spirv_OpenCL_s_abs_v4i16( short4 x )
{
    ushort4 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i16(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i16(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i16(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i16(x.s3);
    return temp;
}

INLINE
ushort8 __builtin_spirv_OpenCL_s_abs_v8i16( short8 x )
{
    ushort8 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i16(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i16(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i16(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i16(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i16(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i16(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i16(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i16(x.s7);
    return temp;
}

INLINE
ushort16 __builtin_spirv_OpenCL_s_abs_v16i16( short16 x )
{
    ushort16 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i16(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i16(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i16(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i16(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i16(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i16(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i16(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i16(x.s7);
    temp.s8 = __builtin_spirv_OpenCL_s_abs_i16(x.s8);
    temp.s9 = __builtin_spirv_OpenCL_s_abs_i16(x.s9);
    temp.sa = __builtin_spirv_OpenCL_s_abs_i16(x.sa);
    temp.sb = __builtin_spirv_OpenCL_s_abs_i16(x.sb);
    temp.sc = __builtin_spirv_OpenCL_s_abs_i16(x.sc);
    temp.sd = __builtin_spirv_OpenCL_s_abs_i16(x.sd);
    temp.se = __builtin_spirv_OpenCL_s_abs_i16(x.se);
    temp.sf = __builtin_spirv_OpenCL_s_abs_i16(x.sf);
    return temp;
}

INLINE
ushort __builtin_spirv_OpenCL_u_abs_i16( ushort x )
{
    return x;
}

INLINE
ushort2 __builtin_spirv_OpenCL_u_abs_v2i16( ushort2 x )
{
    return x;
}

INLINE
ushort3 __builtin_spirv_OpenCL_u_abs_v3i16( ushort3 x )
{
    return x;
}

INLINE
ushort4 __builtin_spirv_OpenCL_u_abs_v4i16( ushort4 x )
{
    return x;
}

INLINE
ushort8 __builtin_spirv_OpenCL_u_abs_v8i16( ushort8 x )
{
    return x;
}

INLINE
ushort16 __builtin_spirv_OpenCL_u_abs_v16i16( ushort16 x )
{
    return x;
}



INLINE
uint2 __builtin_spirv_OpenCL_s_abs_v2i32( int2 x )
{
    uint2 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i32(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i32(x.s1);
    return temp;
}

INLINE
uint3 __builtin_spirv_OpenCL_s_abs_v3i32( int3 x )
{
    uint3 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i32(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i32(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i32(x.s2);
    return temp;
}

INLINE
uint4 __builtin_spirv_OpenCL_s_abs_v4i32( int4 x )
{
    uint4 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i32(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i32(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i32(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i32(x.s3);
    return temp;
}

INLINE
uint8 __builtin_spirv_OpenCL_s_abs_v8i32( int8 x )
{
    uint8 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i32(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i32(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i32(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i32(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i32(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i32(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i32(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i32(x.s7);
    return temp;
}

INLINE
uint16 __builtin_spirv_OpenCL_s_abs_v16i32( int16 x )
{
    uint16 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i32(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i32(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i32(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i32(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i32(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i32(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i32(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i32(x.s7);
    temp.s8 = __builtin_spirv_OpenCL_s_abs_i32(x.s8);
    temp.s9 = __builtin_spirv_OpenCL_s_abs_i32(x.s9);
    temp.sa = __builtin_spirv_OpenCL_s_abs_i32(x.sa);
    temp.sb = __builtin_spirv_OpenCL_s_abs_i32(x.sb);
    temp.sc = __builtin_spirv_OpenCL_s_abs_i32(x.sc);
    temp.sd = __builtin_spirv_OpenCL_s_abs_i32(x.sd);
    temp.se = __builtin_spirv_OpenCL_s_abs_i32(x.se);
    temp.sf = __builtin_spirv_OpenCL_s_abs_i32(x.sf);
    return temp;
}

INLINE
uint __builtin_spirv_OpenCL_u_abs_i32( uint x )
{
    return x;
}

INLINE
uint2 __builtin_spirv_OpenCL_u_abs_v2i32( uint2 x )
{
    return x;
}

INLINE
uint3 __builtin_spirv_OpenCL_u_abs_v3i32( uint3 x )
{
    return x;
}

INLINE
uint4 __builtin_spirv_OpenCL_u_abs_v4i32( uint4 x )
{
    return x;
}

INLINE
uint8 __builtin_spirv_OpenCL_u_abs_v8i32( uint8 x )
{
    return x;
}

INLINE
uint16 __builtin_spirv_OpenCL_u_abs_v16i32( uint16 x )
{
    return x;
}

INLINE
ulong __builtin_spirv_OpenCL_s_abs_i64( long x )
{
    return (ulong)((x >= 0) ? x : -x);
}

INLINE
ulong2 __builtin_spirv_OpenCL_s_abs_v2i64( long2 x )
{
    ulong2 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i64(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i64(x.s1);
    return temp;
}

INLINE
ulong3 __builtin_spirv_OpenCL_s_abs_v3i64( long3 x )
{
    ulong3 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i64(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i64(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i64(x.s2);
    return temp;
}

INLINE
ulong4 __builtin_spirv_OpenCL_s_abs_v4i64( long4 x )
{
    ulong4 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i64(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i64(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i64(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i64(x.s3);
    return temp;
}

INLINE
ulong8 __builtin_spirv_OpenCL_s_abs_v8i64( long8 x )
{
    ulong8 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i64(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i64(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i64(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i64(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i64(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i64(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i64(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i64(x.s7);
    return temp;
}

INLINE
ulong16 __builtin_spirv_OpenCL_s_abs_v16i64( long16 x )
{
    ulong16 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_abs_i64(x.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_abs_i64(x.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_abs_i64(x.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_abs_i64(x.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_abs_i64(x.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_abs_i64(x.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_abs_i64(x.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_abs_i64(x.s7);
    temp.s8 = __builtin_spirv_OpenCL_s_abs_i64(x.s8);
    temp.s9 = __builtin_spirv_OpenCL_s_abs_i64(x.s9);
    temp.sa = __builtin_spirv_OpenCL_s_abs_i64(x.sa);
    temp.sb = __builtin_spirv_OpenCL_s_abs_i64(x.sb);
    temp.sc = __builtin_spirv_OpenCL_s_abs_i64(x.sc);
    temp.sd = __builtin_spirv_OpenCL_s_abs_i64(x.sd);
    temp.se = __builtin_spirv_OpenCL_s_abs_i64(x.se);
    temp.sf = __builtin_spirv_OpenCL_s_abs_i64(x.sf);
    return temp;
}

INLINE
ulong __builtin_spirv_OpenCL_u_abs_i64( ulong x )
{
    return x;
}

INLINE
ulong2 __builtin_spirv_OpenCL_u_abs_v2i64( ulong2 x )
{
    return x;
}

INLINE
ulong3 __builtin_spirv_OpenCL_u_abs_v3i64( ulong3 x )
{
    return x;
}

INLINE
ulong4 __builtin_spirv_OpenCL_u_abs_v4i64( ulong4 x )
{
    return x;
}

INLINE
ulong8 __builtin_spirv_OpenCL_u_abs_v8i64( ulong8 x )
{
    return x;
}

INLINE
ulong16 __builtin_spirv_OpenCL_u_abs_v16i64( ulong16 x )
{
    return x;
}

