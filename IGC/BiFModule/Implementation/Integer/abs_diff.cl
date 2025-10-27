/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
uchar __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char x,
                                        char y )
{
    uchar result;
    if (x > y)
    {
        result = (uchar)x - y;
    }
    else
    {
        result = (uchar)y - x;
    }
    return result;
}

INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char2 x,
                                             char2 y )
{
    uchar2 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char3 x,
                                             char3 y )
{
    uchar3 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char4 x,
                                             char4 y )
{
    uchar4 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char8 x,
                                             char8 y )
{
    uchar8 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( char16 x,
                                                char16 y )
{
    uchar16 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_s_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_s_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar x,
                                        uchar y )
{
    uchar result;
    if (x > y)
    {
        result = x - y;
    }
    else
    {
        result = y - x;
    }
    return result;
}

INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar2 x,
                                             uchar2 y )
{
    uchar2 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar3 x,
                                             uchar3 y )
{
    uchar3 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar4 x,
                                             uchar4 y )
{
    uchar4 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar8 x,
                                             uchar8 y )
{
    uchar8 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uchar16 x,
                                                uchar16 y )
{
    uchar16 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_u_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_u_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short x,
                                           short y )
{
    ushort result;
    if (x > y)
    {
        result = (ushort)x - y;
    }
    else
    {
        result = (ushort)y - x;
    }
    return result;
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short2 x,
                                                short2 y )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short3 x,
                                                short3 y )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short4 x,
                                                short4 y )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short8 x,
                                                short8 y )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( short16 x,
                                                   short16 y )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_s_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_s_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort x,
                                           ushort y )
{
    ushort result;
    if (x > y)
    {
        result = x - y;
    }
    else
    {
        result = y - x;
    }
    return result;
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort2 x,
                                                ushort2 y )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort3 x,
                                                ushort3 y )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort4 x,
                                                ushort4 y )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort8 x,
                                                ushort8 y )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ushort16 x,
                                                   ushort16 y )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_u_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_u_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int x,
                                         int y )
{
    uint result;
    if (x > y)
    {
        result = (uint)x - y;
    }
    else
    {
        result = (uint)y - x;
    }
    return result;
}

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int2 x,
                                              int2 y )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int3 x,
                                              int3 y )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int4 x,
                                              int4 y )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int8 x,
                                              int8 y )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( int16 x,
                                                 int16 y )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_s_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_s_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint x,
                                         uint y )
{
    uint result;
    if (x > y)
    {
        result = x - y;
    }
    else
    {
        result = y - x;
    }
    return result;
}

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint2 x,
                                              uint2 y )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint3 x,
                                              uint3 y )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint4 x,
                                              uint4 y )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint8 x,
                                              uint8 y )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( uint16 x,
                                                 uint16 y )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_u_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_u_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long x,
                                          long y )
{
    ulong result;
    if (x > y)
    {
        result = (ulong)x - y;
    }
    else
    {
        result = (ulong)y - x;
    }
    return result;
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long2 x,
                                               long2 y )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long3 x,
                                               long3 y )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long4 x,
                                               long4 y )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long8 x,
                                               long8 y )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_s_abs_diff( long16 x,
                                                  long16 y )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_s_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_s_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_s_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_s_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_s_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_s_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_s_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_s_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_s_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_s_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_s_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_s_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_s_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_s_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_s_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_s_abs_diff(x.sf, y.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong x,
                                          ulong y )
{
    ulong result;
    if (x > y)
    {
        result = x - y;
    }
    else
    {
        result = y - x;
    }
    return result;
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong2 x,
                                               ulong2 y )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong3 x,
                                               ulong3 y )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong4 x,
                                               ulong4 y )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong8 x,
                                               ulong8 y )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_abs_diff( ulong16 x,
                                                  ulong16 y )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_u_abs_diff(x.s0, y.s0);
    temp.s1 = __spirv_ocl_u_abs_diff(x.s1, y.s1);
    temp.s2 = __spirv_ocl_u_abs_diff(x.s2, y.s2);
    temp.s3 = __spirv_ocl_u_abs_diff(x.s3, y.s3);
    temp.s4 = __spirv_ocl_u_abs_diff(x.s4, y.s4);
    temp.s5 = __spirv_ocl_u_abs_diff(x.s5, y.s5);
    temp.s6 = __spirv_ocl_u_abs_diff(x.s6, y.s6);
    temp.s7 = __spirv_ocl_u_abs_diff(x.s7, y.s7);
    temp.s8 = __spirv_ocl_u_abs_diff(x.s8, y.s8);
    temp.s9 = __spirv_ocl_u_abs_diff(x.s9, y.s9);
    temp.sa = __spirv_ocl_u_abs_diff(x.sa, y.sa);
    temp.sb = __spirv_ocl_u_abs_diff(x.sb, y.sb);
    temp.sc = __spirv_ocl_u_abs_diff(x.sc, y.sc);
    temp.sd = __spirv_ocl_u_abs_diff(x.sd, y.sd);
    temp.se = __spirv_ocl_u_abs_diff(x.se, y.se);
    temp.sf = __spirv_ocl_u_abs_diff(x.sf, y.sf);
    return temp;
}

