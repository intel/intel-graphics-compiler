/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
char __attribute__((overloadable)) __spirv_ocl_rotate( char v, char i )
{
    uchar temp = as_uchar(i) % 8;
    uchar uv = as_uchar(v);
    return (uv << temp) | (uv >> (8 - temp));
}

INLINE
char2 __attribute__((overloadable)) __spirv_ocl_rotate( char2 v, char2 i )
{
    char2 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    return temp;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_rotate( char3 v, char3 i )
{
    char3 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    return temp;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_rotate( char4 v, char4 i )
{
    char4 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    return temp;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_rotate( char8 v, char8 i )
{
    char8 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    return temp;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_rotate( char16 v, char16 i )
{
    char16 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    temp.s8 = __spirv_ocl_rotate(v.s8, i.s8);
    temp.s9 = __spirv_ocl_rotate(v.s9, i.s9);
    temp.sa = __spirv_ocl_rotate(v.sa, i.sa);
    temp.sb = __spirv_ocl_rotate(v.sb, i.sb);
    temp.sc = __spirv_ocl_rotate(v.sc, i.sc);
    temp.sd = __spirv_ocl_rotate(v.sd, i.sd);
    temp.se = __spirv_ocl_rotate(v.se, i.se);
    temp.sf = __spirv_ocl_rotate(v.sf, i.sf);
    return temp;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_rotate( short v, short i )
{
    ushort temp = as_ushort(i) % 16;
    ushort uv = as_ushort(v);
    return (uv << temp) | (uv >> (16 - temp));
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_rotate( short2 v, short2 i )
{
    short2 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_rotate( short3 v, short3 i )
{
    short3 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_rotate( short4 v, short4 i )
{
    short4 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_rotate( short8 v, short8 i )
{
    short8 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_rotate( short16 v, short16 i )
{
    short16 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    temp.s8 = __spirv_ocl_rotate(v.s8, i.s8);
    temp.s9 = __spirv_ocl_rotate(v.s9, i.s9);
    temp.sa = __spirv_ocl_rotate(v.sa, i.sa);
    temp.sb = __spirv_ocl_rotate(v.sb, i.sb);
    temp.sc = __spirv_ocl_rotate(v.sc, i.sc);
    temp.sd = __spirv_ocl_rotate(v.sd, i.sd);
    temp.se = __spirv_ocl_rotate(v.se, i.se);
    temp.sf = __spirv_ocl_rotate(v.sf, i.sf);
    return temp;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_rotate( int v, int i )
{
    uint temp = as_uint(i) % 32;
    uint uv = as_uint(v);
    return (uv << temp) | (uv >> (32 - temp));
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_rotate( int2 v, int2 i )
{
    int2 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_rotate( int3 v, int3 i )
{
    int3 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_rotate( int4 v, int4 i )
{
    int4 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_rotate( int8 v, int8 i )
{
    int8 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_rotate( int16 v, int16 i )
{
    int16 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    temp.s8 = __spirv_ocl_rotate(v.s8, i.s8);
    temp.s9 = __spirv_ocl_rotate(v.s9, i.s9);
    temp.sa = __spirv_ocl_rotate(v.sa, i.sa);
    temp.sb = __spirv_ocl_rotate(v.sb, i.sb);
    temp.sc = __spirv_ocl_rotate(v.sc, i.sc);
    temp.sd = __spirv_ocl_rotate(v.sd, i.sd);
    temp.se = __spirv_ocl_rotate(v.se, i.se);
    temp.sf = __spirv_ocl_rotate(v.sf, i.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_rotate( long v, long i )
{
    ulong shift_left = as_ulong(i) % 64;
    if (shift_left == 0)
    {
        return v;
    }
    ulong uv = as_ulong(v);
    return (uv << shift_left) | (uv >> (64 - shift_left));
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_rotate( long2 v, long2 i )
{
    long2 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_rotate( long3 v, long3 i )
{
    long3 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_rotate( long4 v, long4 i )
{
    long4 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_rotate( long8 v, long8 i )
{
    long8 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_rotate( long16 v, long16 i )
{
    long16 temp;
    temp.s0 = __spirv_ocl_rotate(v.s0, i.s0);
    temp.s1 = __spirv_ocl_rotate(v.s1, i.s1);
    temp.s2 = __spirv_ocl_rotate(v.s2, i.s2);
    temp.s3 = __spirv_ocl_rotate(v.s3, i.s3);
    temp.s4 = __spirv_ocl_rotate(v.s4, i.s4);
    temp.s5 = __spirv_ocl_rotate(v.s5, i.s5);
    temp.s6 = __spirv_ocl_rotate(v.s6, i.s6);
    temp.s7 = __spirv_ocl_rotate(v.s7, i.s7);
    temp.s8 = __spirv_ocl_rotate(v.s8, i.s8);
    temp.s9 = __spirv_ocl_rotate(v.s9, i.s9);
    temp.sa = __spirv_ocl_rotate(v.sa, i.sa);
    temp.sb = __spirv_ocl_rotate(v.sb, i.sb);
    temp.sc = __spirv_ocl_rotate(v.sc, i.sc);
    temp.sd = __spirv_ocl_rotate(v.sd, i.sd);
    temp.se = __spirv_ocl_rotate(v.se, i.se);
    temp.sf = __spirv_ocl_rotate(v.sf, i.sf);
    return temp;
}

