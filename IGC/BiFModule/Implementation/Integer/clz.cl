/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE
char __attribute__((overloadable)) __spirv_ocl_clz( char x )
{
    return __spirv_ocl_clz(as_uchar(x)) - ( 32 - sizeof(x) * 8 );
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_clz( short x )
{
    return __spirv_ocl_clz(as_ushort(x)) - ( 32 - sizeof(x) * 8 );
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_clz( int x )
{
    if (x == 0)
        return 32;
    else
        return __builtin_clz(x);
}

INLINE
char2 __attribute__((overloadable)) __spirv_ocl_clz( char2 x )
{
    char2 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    return temp;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_clz( char3 x )
{
    char3 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    return temp;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_clz( char4 x )
{
    char4 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    return temp;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_clz( char8 x )
{
    char8 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    return temp;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_clz( char16 x )
{
    char16 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    temp.s8 = __spirv_ocl_clz(x.s8);
    temp.s9 = __spirv_ocl_clz(x.s9);
    temp.sa = __spirv_ocl_clz(x.sa);
    temp.sb = __spirv_ocl_clz(x.sb);
    temp.sc = __spirv_ocl_clz(x.sc);
    temp.sd = __spirv_ocl_clz(x.sd);
    temp.se = __spirv_ocl_clz(x.se);
    temp.sf = __spirv_ocl_clz(x.sf);
    return temp;
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_clz( short2 x )
{
    short2 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_clz( short3 x )
{
    short3 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_clz( short4 x )
{
    short4 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_clz( short8 x )
{
    short8 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_clz( short16 x )
{
    short16 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    temp.s8 = __spirv_ocl_clz(x.s8);
    temp.s9 = __spirv_ocl_clz(x.s9);
    temp.sa = __spirv_ocl_clz(x.sa);
    temp.sb = __spirv_ocl_clz(x.sb);
    temp.sc = __spirv_ocl_clz(x.sc);
    temp.sd = __spirv_ocl_clz(x.sd);
    temp.se = __spirv_ocl_clz(x.se);
    temp.sf = __spirv_ocl_clz(x.sf);
    return temp;
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_clz( int2 x )
{
    int2 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_clz( int3 x )
{
    int3 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_clz( int4 x )
{
    int4 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_clz( int8 x )
{
    int8 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_clz( int16 x )
{
    int16 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    temp.s8 = __spirv_ocl_clz(x.s8);
    temp.s9 = __spirv_ocl_clz(x.s9);
    temp.sa = __spirv_ocl_clz(x.sa);
    temp.sb = __spirv_ocl_clz(x.sb);
    temp.sc = __spirv_ocl_clz(x.sc);
    temp.sd = __spirv_ocl_clz(x.sd);
    temp.se = __spirv_ocl_clz(x.se);
    temp.sf = __spirv_ocl_clz(x.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_clz( long x )
{
    ulong clz_lo = (ulong)(__spirv_ocl_clz((int)(x)));
    ulong hi = (x & 0xFFFFFFFF00000000UL);
    ulong clz_hi = (ulong)(__spirv_ocl_clz((int)(hi >> 32)));
    return ( hi == 0) ? clz_lo + 32 : clz_hi;
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_clz( long2 x )
{
    long2 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_clz( long3 x )
{
    long3 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_clz( long4 x )
{
    long4 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_clz( long8 x )
{
    long8 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_clz( long16 x )
{
    long16 temp;
    temp.s0 = __spirv_ocl_clz(x.s0);
    temp.s1 = __spirv_ocl_clz(x.s1);
    temp.s2 = __spirv_ocl_clz(x.s2);
    temp.s3 = __spirv_ocl_clz(x.s3);
    temp.s4 = __spirv_ocl_clz(x.s4);
    temp.s5 = __spirv_ocl_clz(x.s5);
    temp.s6 = __spirv_ocl_clz(x.s6);
    temp.s7 = __spirv_ocl_clz(x.s7);
    temp.s8 = __spirv_ocl_clz(x.s8);
    temp.s9 = __spirv_ocl_clz(x.s9);
    temp.sa = __spirv_ocl_clz(x.sa);
    temp.sb = __spirv_ocl_clz(x.sb);
    temp.sc = __spirv_ocl_clz(x.sc);
    temp.sd = __spirv_ocl_clz(x.sd);
    temp.se = __spirv_ocl_clz(x.se);
    temp.sf = __spirv_ocl_clz(x.sf);
    return temp;
}

