/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE
char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i8, )( char x )
{
    return SPIRV_OCL_BUILTIN(clz, _i32, )(as_uchar(x)) - ( 32 - sizeof(x) * 8 );
}

INLINE
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i16, )( short x )
{
    return SPIRV_OCL_BUILTIN(clz, _i32, )(as_ushort(x)) - ( 32 - sizeof(x) * 8 );
}

INLINE
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i32, )( int x )
{
    if (x == 0)
        return 32;
    else
        return __builtin_clz(x);
}

INLINE
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i8, )( char2 x )
{
    char2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s1);
    return temp;
}

INLINE
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i8, )( char3 x )
{
    char3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s2);
    return temp;
}

INLINE
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i8, )( char4 x )
{
    char4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s3);
    return temp;
}

INLINE
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i8, )( char8 x )
{
    char8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s7);
    return temp;
}

INLINE
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i8, )( char16 x )
{
    char16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(clz, _i8, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(clz, _i8, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(clz, _i8, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(clz, _i8, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(clz, _i8, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(clz, _i8, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(clz, _i8, )(x.sf);
    return temp;
}

INLINE
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i16, )( short2 x )
{
    short2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s1);
    return temp;
}

INLINE
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i16, )( short3 x )
{
    short3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s2);
    return temp;
}

INLINE
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i16, )( short4 x )
{
    short4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s3);
    return temp;
}

INLINE
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i16, )( short8 x )
{
    short8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s7);
    return temp;
}

INLINE
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i16, )( short16 x )
{
    short16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(clz, _i16, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(clz, _i16, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(clz, _i16, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(clz, _i16, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(clz, _i16, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(clz, _i16, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(clz, _i16, )(x.sf);
    return temp;
}

INLINE
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i32, )( int2 x )
{
    int2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s1);
    return temp;
}

INLINE
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i32, )( int3 x )
{
    int3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s2);
    return temp;
}

INLINE
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i32, )( int4 x )
{
    int4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s3);
    return temp;
}

INLINE
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i32, )( int8 x )
{
    int8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s7);
    return temp;
}

INLINE
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i32, )( int16 x )
{
    int16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(clz, _i32, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(clz, _i32, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(clz, _i32, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(clz, _i32, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(clz, _i32, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(clz, _i32, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(clz, _i32, )(x.sf);
    return temp;
}

INLINE
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _i64, )( long x )
{
    ulong clz_lo = (ulong)(SPIRV_OCL_BUILTIN(clz, _i32, )((int)(x)));
    ulong hi = (x & 0xFFFFFFFF00000000UL);
    ulong clz_hi = (ulong)(SPIRV_OCL_BUILTIN(clz, _i32, )((int)(hi >> 32)));
    return ( hi == 0) ? clz_lo + 32 : clz_hi;
}

INLINE
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v2i64, )( long2 x )
{
    long2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s1);
    return temp;
}

INLINE
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v3i64, )( long3 x )
{
    long3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s2);
    return temp;
}

INLINE
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v4i64, )( long4 x )
{
    long4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s3);
    return temp;
}

INLINE
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v8i64, )( long8 x )
{
    long8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s7);
    return temp;
}

INLINE
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(clz, _v16i64, )( long16 x )
{
    long16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(clz, _i64, )(x.s9);
    temp.sa = SPIRV_OCL_BUILTIN(clz, _i64, )(x.sa);
    temp.sb = SPIRV_OCL_BUILTIN(clz, _i64, )(x.sb);
    temp.sc = SPIRV_OCL_BUILTIN(clz, _i64, )(x.sc);
    temp.sd = SPIRV_OCL_BUILTIN(clz, _i64, )(x.sd);
    temp.se = SPIRV_OCL_BUILTIN(clz, _i64, )(x.se);
    temp.sf = SPIRV_OCL_BUILTIN(clz, _i64, )(x.sf);
    return temp;
}

