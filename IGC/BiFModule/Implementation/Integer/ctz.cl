/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
char2 __attribute__((overloadable)) __spirv_ocl_ctz( char2 x )
{
    char2 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    return temp;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_ctz( char3 x )
{
    char3 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    return temp;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_ctz( char4 x )
{
    char4 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    return temp;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_ctz( char8 x )
{
    char8 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    return temp;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_ctz( char16 x )
{
    char16 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    temp.s8 = __spirv_ocl_ctz(x.s8);
    temp.s9 = __spirv_ocl_ctz(x.s9);
    temp.sa = __spirv_ocl_ctz(x.sa);
    temp.sb = __spirv_ocl_ctz(x.sb);
    temp.sc = __spirv_ocl_ctz(x.sc);
    temp.sd = __spirv_ocl_ctz(x.sd);
    temp.se = __spirv_ocl_ctz(x.se);
    temp.sf = __spirv_ocl_ctz(x.sf);
    return temp;
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_ctz( short2 x )
{
    short2 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_ctz( short3 x )
{
    short3 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_ctz( short4 x )
{
    short4 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_ctz( short8 x )
{
    short8 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_ctz( short16 x )
{
    short16 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    temp.s8 = __spirv_ocl_ctz(x.s8);
    temp.s9 = __spirv_ocl_ctz(x.s9);
    temp.sa = __spirv_ocl_ctz(x.sa);
    temp.sb = __spirv_ocl_ctz(x.sb);
    temp.sc = __spirv_ocl_ctz(x.sc);
    temp.sd = __spirv_ocl_ctz(x.sd);
    temp.se = __spirv_ocl_ctz(x.se);
    temp.sf = __spirv_ocl_ctz(x.sf);
    return temp;
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_ctz( int2 x )
{
    int2 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_ctz( int3 x )
{
    int3 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_ctz( int4 x )
{
    int4 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_ctz( int8 x )
{
    int8 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_ctz( int16 x )
{
    int16 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    temp.s8 = __spirv_ocl_ctz(x.s8);
    temp.s9 = __spirv_ocl_ctz(x.s9);
    temp.sa = __spirv_ocl_ctz(x.sa);
    temp.sb = __spirv_ocl_ctz(x.sb);
    temp.sc = __spirv_ocl_ctz(x.sc);
    temp.sd = __spirv_ocl_ctz(x.sd);
    temp.se = __spirv_ocl_ctz(x.se);
    temp.sf = __spirv_ocl_ctz(x.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_ctz( long x )
{
    int2 i2 = as_int2(x);
    ulong ctz_lo = (ulong)(__spirv_ocl_ctz(i2.x));
    ulong ctz_hi = (ulong)(__spirv_ocl_ctz(i2.y));
    ulong result = (i2.x == 0) ? ctz_hi + ctz_lo : ctz_lo;
    return result;
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_ctz( long2 x )
{
    long2 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_ctz( long3 x )
{
    long3 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_ctz( long4 x )
{
    long4 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_ctz( long8 x )
{
    long8 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_ctz( long16 x )
{
    long16 temp;
    temp.s0 = __spirv_ocl_ctz(x.s0);
    temp.s1 = __spirv_ocl_ctz(x.s1);
    temp.s2 = __spirv_ocl_ctz(x.s2);
    temp.s3 = __spirv_ocl_ctz(x.s3);
    temp.s4 = __spirv_ocl_ctz(x.s4);
    temp.s5 = __spirv_ocl_ctz(x.s5);
    temp.s6 = __spirv_ocl_ctz(x.s6);
    temp.s7 = __spirv_ocl_ctz(x.s7);
    temp.s8 = __spirv_ocl_ctz(x.s8);
    temp.s9 = __spirv_ocl_ctz(x.s9);
    temp.sa = __spirv_ocl_ctz(x.sa);
    temp.sb = __spirv_ocl_ctz(x.sb);
    temp.sc = __spirv_ocl_ctz(x.sc);
    temp.sd = __spirv_ocl_ctz(x.sd);
    temp.se = __spirv_ocl_ctz(x.se);
    temp.sf = __spirv_ocl_ctz(x.sf);
    return temp;
}

