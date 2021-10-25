/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i8_i8, )( char v, char i )
{
    uchar temp = as_uchar(i) % 8;
    uchar uv = as_uchar(v);
    return (uv << temp) | (uv >> (8 - temp));
}

INLINE
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i8_v2i8, )( char2 v, char2 i )
{
    char2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s1, i.s1);
    return temp;
}

INLINE
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i8_v3i8, )( char3 v, char3 i )
{
    char3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s2, i.s2);
    return temp;
}

INLINE
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i8_v4i8, )( char4 v, char4 i )
{
    char4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s3, i.s3);
    return temp;
}

INLINE
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i8_v8i8, )( char8 v, char8 i )
{
    char8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s7, i.s7);
    return temp;
}

INLINE
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i8_v16i8, )( char16 v, char16 i )
{
    char16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s7, i.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s8, i.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.s9, i.s9);
    temp.sa = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.sa, i.sa);
    temp.sb = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.sb, i.sb);
    temp.sc = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.sc, i.sc);
    temp.sd = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.sd, i.sd);
    temp.se = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.se, i.se);
    temp.sf = SPIRV_OCL_BUILTIN(rotate, _i8_i8, )(v.sf, i.sf);
    return temp;
}

INLINE
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i16_i16, )( short v, short i )
{
    ushort temp = as_ushort(i) % 16;
    ushort uv = as_ushort(v);
    return (uv << temp) | (uv >> (16 - temp));
}

INLINE
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i16_v2i16, )( short2 v, short2 i )
{
    short2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s1, i.s1);
    return temp;
}

INLINE
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i16_v3i16, )( short3 v, short3 i )
{
    short3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s2, i.s2);
    return temp;
}

INLINE
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i16_v4i16, )( short4 v, short4 i )
{
    short4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s3, i.s3);
    return temp;
}

INLINE
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i16_v8i16, )( short8 v, short8 i )
{
    short8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s7, i.s7);
    return temp;
}

INLINE
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i16_v16i16, )( short16 v, short16 i )
{
    short16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s7, i.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s8, i.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.s9, i.s9);
    temp.sa = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.sa, i.sa);
    temp.sb = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.sb, i.sb);
    temp.sc = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.sc, i.sc);
    temp.sd = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.sd, i.sd);
    temp.se = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.se, i.se);
    temp.sf = SPIRV_OCL_BUILTIN(rotate, _i16_i16, )(v.sf, i.sf);
    return temp;
}

INLINE
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i32_i32, )( int v, int i )
{
    uint temp = as_uint(i) % 32;
    uint uv = as_uint(v);
    return (uv << temp) | (uv >> (32 - temp));
}

INLINE
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i32_v2i32, )( int2 v, int2 i )
{
    int2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s1, i.s1);
    return temp;
}

INLINE
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i32_v3i32, )( int3 v, int3 i )
{
    int3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s2, i.s2);
    return temp;
}

INLINE
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i32_v4i32, )( int4 v, int4 i )
{
    int4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s3, i.s3);
    return temp;
}

INLINE
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i32_v8i32, )( int8 v, int8 i )
{
    int8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s7, i.s7);
    return temp;
}

INLINE
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i32_v16i32, )( int16 v, int16 i )
{
    int16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s7, i.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s8, i.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.s9, i.s9);
    temp.sa = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.sa, i.sa);
    temp.sb = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.sb, i.sb);
    temp.sc = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.sc, i.sc);
    temp.sd = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.sd, i.sd);
    temp.se = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.se, i.se);
    temp.sf = SPIRV_OCL_BUILTIN(rotate, _i32_i32, )(v.sf, i.sf);
    return temp;
}

INLINE
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _i64_i64, )( long v, long i )
{
    ulong temp = as_ulong(i) % 64;
    ulong uv = as_ulong(v);
    return as_ulong((uv << temp) | (uv >> (64 - temp)));
}

INLINE
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v2i64_v2i64, )( long2 v, long2 i )
{
    long2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s1, i.s1);
    return temp;
}

INLINE
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v3i64_v3i64, )( long3 v, long3 i )
{
    long3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s2, i.s2);
    return temp;
}

INLINE
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v4i64_v4i64, )( long4 v, long4 i )
{
    long4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s3, i.s3);
    return temp;
}

INLINE
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v8i64_v8i64, )( long8 v, long8 i )
{
    long8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s7, i.s7);
    return temp;
}

INLINE
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rotate, _v16i64_v16i64, )( long16 v, long16 i )
{
    long16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s0, i.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s1, i.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s2, i.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s3, i.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s4, i.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s5, i.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s6, i.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s7, i.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s8, i.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.s9, i.s9);
    temp.sa = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.sa, i.sa);
    temp.sb = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.sb, i.sb);
    temp.sc = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.sc, i.sc);
    temp.sd = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.sd, i.sd);
    temp.se = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.se, i.se);
    temp.sf = SPIRV_OCL_BUILTIN(rotate, _i64_i64, )(v.sf, i.sf);
    return temp;
}

