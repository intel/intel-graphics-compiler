/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_s_abs( char2 x )
{
    uchar2 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    return temp;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_s_abs( char3 x )
{
    uchar3 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    return temp;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_s_abs( char4 x )
{
    uchar4 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    return temp;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_s_abs( char8 x )
{
    uchar8 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    return temp;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_s_abs( char16 x )
{
    uchar16 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    temp.s8 = __spirv_ocl_s_abs(x.s8);
    temp.s9 = __spirv_ocl_s_abs(x.s9);
    temp.sa = __spirv_ocl_s_abs(x.sa);
    temp.sb = __spirv_ocl_s_abs(x.sb);
    temp.sc = __spirv_ocl_s_abs(x.sc);
    temp.sd = __spirv_ocl_s_abs(x.sd);
    temp.se = __spirv_ocl_s_abs(x.se);
    temp.sf = __spirv_ocl_s_abs(x.sf);
    return temp;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_abs( uchar x )
{
    return x;
}

INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_u_abs( uchar2 x )
{
    return x;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_u_abs( uchar3 x )
{
    return x;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_u_abs( uchar4 x )
{
    return x;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_u_abs( uchar8 x )
{
    return x;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_u_abs( uchar16 x )
{
    return x;
}



INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_s_abs( short2 x )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_s_abs( short3 x )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_s_abs( short4 x )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_s_abs( short8 x )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_s_abs( short16 x )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    temp.s8 = __spirv_ocl_s_abs(x.s8);
    temp.s9 = __spirv_ocl_s_abs(x.s9);
    temp.sa = __spirv_ocl_s_abs(x.sa);
    temp.sb = __spirv_ocl_s_abs(x.sb);
    temp.sc = __spirv_ocl_s_abs(x.sc);
    temp.sd = __spirv_ocl_s_abs(x.sd);
    temp.se = __spirv_ocl_s_abs(x.se);
    temp.sf = __spirv_ocl_s_abs(x.sf);
    return temp;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_abs( ushort x )
{
    return x;
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_abs( ushort2 x )
{
    return x;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_abs( ushort3 x )
{
    return x;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_abs( ushort4 x )
{
    return x;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_abs( ushort8 x )
{
    return x;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_abs( ushort16 x )
{
    return x;
}



INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_s_abs( int2 x )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_s_abs( int3 x )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_s_abs( int4 x )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_s_abs( int8 x )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_s_abs( int16 x )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    temp.s8 = __spirv_ocl_s_abs(x.s8);
    temp.s9 = __spirv_ocl_s_abs(x.s9);
    temp.sa = __spirv_ocl_s_abs(x.sa);
    temp.sb = __spirv_ocl_s_abs(x.sb);
    temp.sc = __spirv_ocl_s_abs(x.sc);
    temp.sd = __spirv_ocl_s_abs(x.sd);
    temp.se = __spirv_ocl_s_abs(x.se);
    temp.sf = __spirv_ocl_s_abs(x.sf);
    return temp;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_abs( uint x )
{
    return x;
}

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_abs( uint2 x )
{
    return x;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_abs( uint3 x )
{
    return x;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_abs( uint4 x )
{
    return x;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_abs( uint8 x )
{
    return x;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_abs( uint16 x )
{
    return x;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_s_abs( long x )
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
ulong2 __attribute__((overloadable)) __spirv_ocl_s_abs( long2 x )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_s_abs( long3 x )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_s_abs( long4 x )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_s_abs( long8 x )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_s_abs( long16 x )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_s_abs(x.s0);
    temp.s1 = __spirv_ocl_s_abs(x.s1);
    temp.s2 = __spirv_ocl_s_abs(x.s2);
    temp.s3 = __spirv_ocl_s_abs(x.s3);
    temp.s4 = __spirv_ocl_s_abs(x.s4);
    temp.s5 = __spirv_ocl_s_abs(x.s5);
    temp.s6 = __spirv_ocl_s_abs(x.s6);
    temp.s7 = __spirv_ocl_s_abs(x.s7);
    temp.s8 = __spirv_ocl_s_abs(x.s8);
    temp.s9 = __spirv_ocl_s_abs(x.s9);
    temp.sa = __spirv_ocl_s_abs(x.sa);
    temp.sb = __spirv_ocl_s_abs(x.sb);
    temp.sc = __spirv_ocl_s_abs(x.sc);
    temp.sd = __spirv_ocl_s_abs(x.sd);
    temp.se = __spirv_ocl_s_abs(x.se);
    temp.sf = __spirv_ocl_s_abs(x.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_abs( ulong x )
{
    return x;
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_abs( ulong2 x )
{
    return x;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_abs( ulong3 x )
{
    return x;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_abs( ulong4 x )
{
    return x;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_abs( ulong8 x )
{
    return x;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_abs( ulong16 x )
{
    return x;
}

