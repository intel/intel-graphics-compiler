/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../include/mul_hilo.cl"

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char x,
                                     char y )
{
    int temp = (int)(x * y);
    int numBits = 8 * sizeof(char);
    return (char)(temp >> numBits);
}

INLINE
char2 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char2 x,
                                          char2 y )
{
    char2 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char3 x,
                                          char3 y )
{
    char3 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char4 x,
                                          char4 y )
{
    char4 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char8 x,
                                          char8 y )
{
    char8 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( char16 x,
                                             char16 y )
{
    char16 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_s_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_s_mul_hi(x.sf, y.sf);
    return temp;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar x,
                                      uchar y )
{
    int temp = (int)(x * y);
    int numBits = 8 * sizeof(uchar);
    return (uchar)(temp >> numBits);
}

INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar2 x,
                                           uchar2 y )
{
    uchar2 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar3 x,
                                           uchar3 y )
{
    uchar3 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar4 x,
                                           uchar4 y )
{
    uchar4 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar8 x,
                                           uchar8 y )
{
    uchar8 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uchar16 x,
                                              uchar16 y )
{
    uchar16 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_u_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_u_mul_hi(x.sf, y.sf);
    return temp;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short x,
                                        short y )
{
    int temp = (int)(x * y);
    int numBits = 8 * sizeof(short);
    return (short)(temp >> numBits);
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short2 x,
                                             short2 y )
{
    short2 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short3 x,
                                             short3 y )
{
    short3 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short4 x,
                                             short4 y )
{
    short4 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short8 x,
                                             short8 y )
{
    short8 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( short16 x,
                                                short16 y )
{
    short16 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_s_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_s_mul_hi(x.sf, y.sf);
    return temp;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort x,
                                         ushort y )
{
    int temp = (int)(x * y);
    int numBits = 8 * sizeof(ushort);
    return (ushort)(temp >> numBits);
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort2 x,
                                              ushort2 y )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort3 x,
                                              ushort3 y )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort4 x,
                                              ushort4 y )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort8 x,
                                              ushort8 y )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ushort16 x,
                                                 ushort16 y )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_u_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_u_mul_hi(x.sf, y.sf);
    return temp;
}



INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int2 x,
                                           int2 y )
{
    int2 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int3 x,
                                           int3 y )
{
    int3 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int4 x,
                                           int4 y )
{
    int4 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int8 x,
                                           int8 y )
{
    int8 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int16 x,
                                              int16 y )
{
    int16 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_s_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_s_mul_hi(x.sf, y.sf);
    return temp;
}



INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint2 x,
                                            uint2 y )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint3 x,
                                            uint3 y )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint4 x,
                                            uint4 y )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint8 x,
                                            uint8 y )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint16 x,
                                               uint16 y )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_u_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_u_mul_hi(x.sf, y.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long x,
                                       long y )
{
    ulong temp;
    return ___intc_mul_hilo(x, y, &temp);
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long2 x,
                                            long2 y )
{
    long2 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long3 x,
                                            long3 y )
{
    long3 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long4 x,
                                            long4 y )
{
    long4 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long8 x,
                                            long8 y )
{
    long8 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_s_mul_hi( long16 x,
                                               long16 y )
{
    long16 temp;
    temp.s0 = __spirv_ocl_s_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_s_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_s_mul_hi(x.sf, y.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong x,
                                        ulong y )
{
    ulong temp;
    return ___intc_umul_hilo(x, y, &temp);
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong2 x,
                                             ulong2 y )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong3 x,
                                             ulong3 y )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong4 x,
                                             ulong4 y )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong8 x,
                                             ulong8 y )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_mul_hi( ulong16 x,
                                                ulong16 y )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_u_mul_hi(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_mul_hi(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_mul_hi(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_mul_hi(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_mul_hi(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_mul_hi(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_mul_hi(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_mul_hi(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_mul_hi(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_mul_hi(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_mul_hi(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_mul_hi(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_mul_hi(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_mul_hi(x.sd, y.sd);
    temp.se = __spirv_ocl_u_mul_hi(x.se, y.se);
    temp.sf = __spirv_ocl_u_mul_hi(x.sf, y.sf);
    return temp;
}

