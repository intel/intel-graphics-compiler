/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )( char  hi,
                                        uchar lo )
{
    return ((short)(hi) << (short)8) | (short)(lo);
}

INLINE
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i8_v2i8, )( char2  hi,
                                             uchar2 lo )
{
    short2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s1, lo.s1);
    return temp;
}

INLINE
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i8_v3i8, )( char3  hi,
                                             uchar3 lo )
{
    short3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s2, lo.s2);
    return temp;
}

INLINE
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i8_v4i8, )( char4  hi,
                                             uchar4 lo )
{
    short4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s3, lo.s3);
    return temp;
}

INLINE
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i8_v8i8, )( char8  hi,
                                             uchar8 lo )
{
    short8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s7, lo.s7);
    return temp;
}

INLINE
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i8_v16i8, )( char16  hi,
                                                uchar16 lo )
{
    short16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_upsample, _i8_i8, )(hi.sf, lo.sf);
    return temp;
}

INLINE
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )( uchar hi,
                                         uchar lo )
{
    return ((ushort)(hi) << (ushort)8) | (ushort)(lo);
}

INLINE
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i8_v2i8, )( uchar2 hi,
                                              uchar2 lo )
{
    ushort2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s1, lo.s1);
    return temp;
}

INLINE
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i8_v3i8, )( uchar3 hi,
                                              uchar3 lo )
{
    ushort3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s2, lo.s2);
    return temp;
}

INLINE
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i8_v4i8, )( uchar4 hi,
                                              uchar4 lo )
{
    ushort4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s3, lo.s3);
    return temp;
}

INLINE
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i8_v8i8, )( uchar8 hi,
                                              uchar8 lo )
{
    ushort8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s7, lo.s7);
    return temp;
}

INLINE
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i8_v16i8, )( uchar16 hi,
                                                 uchar16 lo )
{
    ushort16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_upsample, _i8_i8, )(hi.sf, lo.sf);
    return temp;
}

INLINE
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )( short  hi,
                                        ushort lo )
{
    return ((int)(hi) << (int)16) | (int)(lo);
}

INLINE
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i16_v2i16, )( short2  hi,
                                             ushort2 lo )
{
    int2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s1, lo.s1);
    return temp;
}

INLINE
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i16_v3i16, )( short3  hi,
                                             ushort3 lo )
{
    int3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s2, lo.s2);
    return temp;
}

INLINE
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i16_v4i16, )( short4  hi,
                                             ushort4 lo )
{
    int4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s3, lo.s3);
    return temp;
}

INLINE
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i16_v8i16, )( short8  hi,
                                             ushort8 lo )
{
    int8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s7, lo.s7);
    return temp;
}

INLINE
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i16_v16i16, )( short16  hi,
                                                ushort16 lo )
{
    int16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_upsample, _i16_i16, )(hi.sf, lo.sf);
    return temp;
}

INLINE
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )( ushort hi,
                                         ushort lo )
{
    return ((uint)(hi) << (uint)16) | (uint)(lo);
}

INLINE
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i16_v2i16, )( ushort2 hi,
                                              ushort2 lo )
{
    uint2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s1, lo.s1);
    return temp;
}

INLINE
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i16_v3i16, )( ushort3 hi,
                                              ushort3 lo )
{
    uint3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s2, lo.s2);
    return temp;
}

INLINE
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i16_v4i16, )( ushort4 hi,
                                              ushort4 lo )
{
    uint4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s3, lo.s3);
    return temp;
}

INLINE
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i16_v8i16, )( ushort8 hi,
                                              ushort8 lo )
{
    uint8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s7, lo.s7);
    return temp;
}

INLINE
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i16_v16i16, )( ushort16 hi,
                                                 ushort16 lo )
{
    uint16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_upsample, _i16_i16, )(hi.sf, lo.sf);
    return temp;
}

INLINE
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )( int  hi,
                                         uint lo )
{
    return ((long)(hi) << (long)32) | (long)(lo);
}

INLINE
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v2i32_v2i32, )( int2  hi,
                                              uint2 lo )
{
    long2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s1, lo.s1);
    return temp;
}

INLINE
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v3i32_v3i32, )( int3  hi,
                                              uint3 lo )
{
    long3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s2, lo.s2);
    return temp;
}

INLINE
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v4i32_v4i32, )( int4  hi,
                                              uint4 lo )
{
    long4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s3, lo.s3);
    return temp;
}

INLINE
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v8i32_v8i32, )( int8  hi,
                                              uint8 lo )
{
    long8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s7, lo.s7);
    return temp;
}

INLINE
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_upsample, _v16i32_v16i32, )( int16  hi,
                                                 uint16 lo )
{
    long16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_upsample, _i32_i32, )(hi.sf, lo.sf);
    return temp;
}

INLINE
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )( uint hi,
                                          uint lo )
{
    return ((ulong)(hi) << (ulong)32) | (ulong)(lo);
}

INLINE
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v2i32_v2i32, )( uint2 hi,
                                               uint2 lo )
{
    ulong2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s1, lo.s1);
    return temp;
}

INLINE
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v3i32_v3i32, )( uint3 hi,
                                               uint3 lo )
{
    ulong3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s2, lo.s2);
    return temp;
}

INLINE
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v4i32_v4i32, )( uint4 hi,
                                               uint4 lo )
{
    ulong4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s3, lo.s3);
    return temp;
}

INLINE
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v8i32_v8i32, )( uint8 hi,
                                               uint8 lo )
{
    ulong8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s7, lo.s7);
    return temp;
}

INLINE
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_upsample, _v16i32_v16i32, )( uint16 hi,
                                                  uint16 lo )
{
    ulong16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s0, lo.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s1, lo.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s2, lo.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s3, lo.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s4, lo.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s5, lo.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s6, lo.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s7, lo.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s8, lo.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.s9, lo.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.sa, lo.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.sb, lo.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.sc, lo.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.sd, lo.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.se, lo.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_upsample, _i32_i32, )(hi.sf, lo.sf);
    return temp;
}

