/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i8, )( char2 x )
{
    uchar2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s1);
    return temp;
}

INLINE
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i8, )( char3 x )
{
    uchar3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s2);
    return temp;
}

INLINE
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i8, )( char4 x )
{
    uchar4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s3);
    return temp;
}

INLINE
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i8, )( char8 x )
{
    uchar8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s7);
    return temp;
}

INLINE
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i8, )( char16 x )
{
    uchar16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_abs, _i8, )(x.sf);
    return temp;
}

INLINE
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i8, )( uchar x )
{
    return x;
}

INLINE
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i8, )( uchar2 x )
{
    return x;
}

INLINE
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i8, )( uchar3 x )
{
    return x;
}

INLINE
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i8, )( uchar4 x )
{
    return x;
}

INLINE
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i8, )( uchar8 x )
{
    return x;
}

INLINE
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i8, )( uchar16 x )
{
    return x;
}



INLINE
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i16, )( short2 x )
{
    ushort2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s1);
    return temp;
}

INLINE
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i16, )( short3 x )
{
    ushort3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s2);
    return temp;
}

INLINE
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i16, )( short4 x )
{
    ushort4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s3);
    return temp;
}

INLINE
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i16, )( short8 x )
{
    ushort8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s7);
    return temp;
}

INLINE
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i16, )( short16 x )
{
    ushort16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_abs, _i16, )(x.sf);
    return temp;
}

INLINE
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i16, )( ushort x )
{
    return x;
}

INLINE
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i16, )( ushort2 x )
{
    return x;
}

INLINE
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i16, )( ushort3 x )
{
    return x;
}

INLINE
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i16, )( ushort4 x )
{
    return x;
}

INLINE
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i16, )( ushort8 x )
{
    return x;
}

INLINE
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i16, )( ushort16 x )
{
    return x;
}



INLINE
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i32, )( int2 x )
{
    uint2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s1);
    return temp;
}

INLINE
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i32, )( int3 x )
{
    uint3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s2);
    return temp;
}

INLINE
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i32, )( int4 x )
{
    uint4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s3);
    return temp;
}

INLINE
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i32, )( int8 x )
{
    uint8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s7);
    return temp;
}

INLINE
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i32, )( int16 x )
{
    uint16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_abs, _i32, )(x.sf);
    return temp;
}

INLINE
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i32, )( uint x )
{
    return x;
}

INLINE
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i32, )( uint2 x )
{
    return x;
}

INLINE
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i32, )( uint3 x )
{
    return x;
}

INLINE
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i32, )( uint4 x )
{
    return x;
}

INLINE
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i32, )( uint8 x )
{
    return x;
}

INLINE
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i32, )( uint16 x )
{
    return x;
}

INLINE
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _i64, )( long x )
{
    // -x would overflow if x = 0x8000000000000000, resulting undefined behavior
    // -ux would never overflow and is well-defined.
    // In llvm, -x could be "sub nsw  0, x", where -ux is "sub 0, ux". "nsw" indicates
    // the result might be undefined.
    // With this, this function will always return -INT64_MIN for x = INT64_MIN
    // Note that the standard lib int abs(int) has undefined behavior when x = MIN.
    ulong ux = (ulong)x;
    return (ulong)((x >= 0) ? ux : -ux);
}

INLINE
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v2i64, )( long2 x )
{
    ulong2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s1);
    return temp;
}

INLINE
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v3i64, )( long3 x )
{
    ulong3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s2);
    return temp;
}

INLINE
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v4i64, )( long4 x )
{
    ulong4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s3);
    return temp;
}

INLINE
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v8i64, )( long8 x )
{
    ulong8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s7);
    return temp;
}

INLINE
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_abs, _v16i64, )( long16 x )
{
    ulong16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_abs, _i64, )(x.sf);
    return temp;
}

INLINE
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _i64, )( ulong x )
{
    return x;
}

INLINE
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v2i64, )( ulong2 x )
{
    return x;
}

INLINE
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v3i64, )( ulong3 x )
{
    return x;
}

INLINE
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v4i64, )( ulong4 x )
{
    return x;
}

INLINE
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v8i64, )( ulong8 x )
{
    return x;
}

INLINE
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_abs, _v16i64, )( ulong16 x )
{
    return x;
}

