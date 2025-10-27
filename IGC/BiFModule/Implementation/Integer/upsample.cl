/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
short __attribute__((overloadable)) __spirv_ocl_s_upsample( char  hi,
                                        uchar lo )
{
    return ((short)(hi) << (short)8) | (short)(lo);
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_s_upsample( char2  hi,
                                             uchar2 lo )
{
    short2 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_s_upsample( char3  hi,
                                             uchar3 lo )
{
    short3 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_s_upsample( char4  hi,
                                             uchar4 lo )
{
    short4 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_s_upsample( char8  hi,
                                             uchar8 lo )
{
    short8 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_s_upsample( char16  hi,
                                                uchar16 lo )
{
    short16 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_s_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_s_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_s_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_s_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_s_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_s_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_s_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_s_upsample(hi.sf, lo.sf);
    return temp;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar hi,
                                         uchar lo )
{
    return ((ushort)(hi) << (ushort)8) | (ushort)(lo);
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar2 hi,
                                              uchar2 lo )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar3 hi,
                                              uchar3 lo )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar4 hi,
                                              uchar4 lo )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar8 hi,
                                              uchar8 lo )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_upsample( uchar16 hi,
                                                 uchar16 lo )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_u_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_u_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_u_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_u_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_u_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_u_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_u_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_u_upsample(hi.sf, lo.sf);
    return temp;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_upsample( short  hi,
                                        ushort lo )
{
    return ((int)(hi) << (int)16) | (int)(lo);
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_upsample( short2  hi,
                                             ushort2 lo )
{
    int2 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_upsample( short3  hi,
                                             ushort3 lo )
{
    int3 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_upsample( short4  hi,
                                             ushort4 lo )
{
    int4 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_upsample( short8  hi,
                                             ushort8 lo )
{
    int8 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_upsample( short16  hi,
                                                ushort16 lo )
{
    int16 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_s_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_s_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_s_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_s_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_s_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_s_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_s_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_s_upsample(hi.sf, lo.sf);
    return temp;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort hi,
                                         ushort lo )
{
    return ((uint)(hi) << (uint)16) | (uint)(lo);
}

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort2 hi,
                                              ushort2 lo )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort3 hi,
                                              ushort3 lo )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort4 hi,
                                              ushort4 lo )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort8 hi,
                                              ushort8 lo )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_upsample( ushort16 hi,
                                                 ushort16 lo )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_u_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_u_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_u_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_u_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_u_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_u_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_u_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_u_upsample(hi.sf, lo.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_upsample( int  hi,
                                         uint lo )
{
    return ((long)(hi) << (long)32) | (long)(lo);
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_s_upsample( int2  hi,
                                              uint2 lo )
{
    long2 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_s_upsample( int3  hi,
                                              uint3 lo )
{
    long3 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_s_upsample( int4  hi,
                                              uint4 lo )
{
    long4 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_s_upsample( int8  hi,
                                              uint8 lo )
{
    long8 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_s_upsample( int16  hi,
                                                 uint16 lo )
{
    long16 temp;
    temp.s0 = __spirv_ocl_s_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_s_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_s_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_s_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_s_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_s_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_s_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_s_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_s_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_s_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_s_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_s_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_s_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_s_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_s_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_s_upsample(hi.sf, lo.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_upsample( uint hi,
                                          uint lo )
{
    return ((ulong)(hi) << (ulong)32) | (ulong)(lo);
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_upsample( uint2 hi,
                                               uint2 lo )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_upsample( uint3 hi,
                                               uint3 lo )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_upsample( uint4 hi,
                                               uint4 lo )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_upsample( uint8 hi,
                                               uint8 lo )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_upsample( uint16 hi,
                                                  uint16 lo )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_u_upsample(hi.s0, lo.s0);
    temp.s1 = __spirv_ocl_u_upsample(hi.s1, lo.s1);
    temp.s2 = __spirv_ocl_u_upsample(hi.s2, lo.s2);
    temp.s3 = __spirv_ocl_u_upsample(hi.s3, lo.s3);
    temp.s4 = __spirv_ocl_u_upsample(hi.s4, lo.s4);
    temp.s5 = __spirv_ocl_u_upsample(hi.s5, lo.s5);
    temp.s6 = __spirv_ocl_u_upsample(hi.s6, lo.s6);
    temp.s7 = __spirv_ocl_u_upsample(hi.s7, lo.s7);
    temp.s8 = __spirv_ocl_u_upsample(hi.s8, lo.s8);
    temp.s9 = __spirv_ocl_u_upsample(hi.s9, lo.s9);
    temp.sa = __spirv_ocl_u_upsample(hi.sa, lo.sa);
    temp.sb = __spirv_ocl_u_upsample(hi.sb, lo.sb);
    temp.sc = __spirv_ocl_u_upsample(hi.sc, lo.sc);
    temp.sd = __spirv_ocl_u_upsample(hi.sd, lo.sd);
    temp.se = __spirv_ocl_u_upsample(hi.se, lo.se);
    temp.sf = __spirv_ocl_u_upsample(hi.sf, lo.sf);
    return temp;
}

