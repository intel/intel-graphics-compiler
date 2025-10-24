/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )( char x,
                                   char y )
{
    return (char)(((int)x + (int)y) >> (int)(1));
}

INLINE
char2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i8_v2i8, )( char2 x,
                                        char2 y )
{
    char2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s1, y.s1);
    return temp;
}

INLINE
char3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i8_v3i8, )( char3 x,
                                        char3 y )
{
    char3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s2, y.s2);
    return temp;
}

INLINE
char4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i8_v4i8, )( char4 x,
                                        char4 y )
{
    char4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s3, y.s3);
    return temp;
}

INLINE
char8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i8_v8i8, )( char8 x,
                                        char8 y )
{
    char8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s7, y.s7);
    return temp;
}

INLINE
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i8_v16i8, )( char16 x,
                                           char16 y )
{
    char16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_hadd, _i8_i8, )(x.sf, y.sf);
    return temp;
}

INLINE
uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )( uchar x,
                                    uchar y )
{
    return (uchar)(((uint)x + (uint)y) >> (uint)(1));
}

INLINE
uchar2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i8_v2i8, )( uchar2 x,
                                         uchar2 y )
{
    uchar2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s1, y.s1);
    return temp;
}

INLINE
uchar3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i8_v3i8, )( uchar3 x,
                                         uchar3 y )
{
    uchar3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s2, y.s2);
    return temp;
}

INLINE
uchar4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i8_v4i8, )( uchar4 x,
                                         uchar4 y )
{
    uchar4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s3, y.s3);
    return temp;
}

INLINE
uchar8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i8_v8i8, )( uchar8 x,
                                         uchar8 y )
{
    uchar8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s7, y.s7);
    return temp;
}

INLINE
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i8_v16i8, )( uchar16 x,
                                            uchar16 y )
{
    uchar16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_hadd, _i8_i8, )(x.sf, y.sf);
    return temp;
}

INLINE
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )( short x,
                                      short y )
{
    return (short)(((int)x + (int)y) >> (int)(1));
}

INLINE
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i16_v2i16, )( short2 x,
                                           short2 y )
{
    short2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s1, y.s1);
    return temp;
}

INLINE
short3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i16_v3i16, )( short3 x,
                                           short3 y )
{
    short3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s2, y.s2);
    return temp;
}

INLINE
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i16_v4i16, )( short4 x,
                                           short4 y )
{
    short4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s3, y.s3);
    return temp;
}

INLINE
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i16_v8i16, )( short8 x,
                                           short8 y )
{
    short8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s7, y.s7);
    return temp;
}

INLINE
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i16_v16i16, )( short16 x,
                                              short16 y )
{
    short16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_hadd, _i16_i16, )(x.sf, y.sf);
    return temp;
}

INLINE
ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )( ushort x,
                                       ushort y )
{
    return (ushort)(((uint)x + (uint)y) >> (uint)(1));
}

INLINE
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i16_v2i16, )( ushort2 x,
                                            ushort2 y )
{
    ushort2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s1, y.s1);
    return temp;
}

INLINE
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i16_v3i16, )( ushort3 x,
                                            ushort3 y )
{
    ushort3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s2, y.s2);
    return temp;
}

INLINE
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i16_v4i16, )( ushort4 x,
                                            ushort4 y )
{
    ushort4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s3, y.s3);
    return temp;
}

INLINE
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i16_v8i16, )( ushort8 x,
                                            ushort8 y )
{
    ushort8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s7, y.s7);
    return temp;
}

INLINE
ushort16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i16_v16i16, )( ushort16 x,
                                               ushort16 y )
{
    ushort16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_hadd, _i16_i16, )(x.sf, y.sf);
    return temp;
}

INLINE
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )( int x,
                                    int y )
{
    return ((long)x + (long)y) >> (1);
}

INLINE
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i32_v2i32, )( int2 x,
                                         int2 y )
{
    int2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s1, y.s1);
    return temp;
}

INLINE
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i32_v3i32, )( int3 x,
                                         int3 y )
{
    int3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s2, y.s2);
    return temp;
}

INLINE
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i32_v4i32, )( int4 x,
                                         int4 y )
{
    int4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s3, y.s3);
    return temp;
}

INLINE
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i32_v8i32, )( int8 x,
                                         int8 y )
{
    int8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s7, y.s7);
    return temp;
}

INLINE
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i32_v16i32, )( int16 x,
                                            int16 y )
{
    int16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_hadd, _i32_i32, )(x.sf, y.sf);
    return temp;
}

INLINE
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )( uint x,
                                     uint y )
{
    return ((long)x + (long)y) >> (1);
}

INLINE
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i32_v2i32, )( uint2 x,
                                          uint2 y )
{
    uint2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s1, y.s1);
    return temp;
}

INLINE
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i32_v3i32, )( uint3 x,
                                          uint3 y )
{
    uint3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s2, y.s2);
    return temp;
}

INLINE
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i32_v4i32, )( uint4 x,
                                          uint4 y )
{
    uint4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s3, y.s3);
    return temp;
}

INLINE
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i32_v8i32, )( uint8 x,
                                          uint8 y )
{
    uint8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s7, y.s7);
    return temp;
}

INLINE
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i32_v16i32, )( uint16 x,
                                             uint16 y )
{
    uint16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_hadd, _i32_i32, )(x.sf, y.sf);
    return temp;
}

INLINE
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )( long x,
                                     long y )
{
    long carry = (x & 0x1L) + (y & 0x1L);
    return (x >> 1) + (y >> 1) + (carry >> 1);
}

INLINE
long2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v2i64_v2i64, )( long2 x,
                                          long2 y )
{
    long2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s1, y.s1);
    return temp;
}

INLINE
long3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v3i64_v3i64, )( long3 x,
                                          long3 y )
{
    long3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s2, y.s2);
    return temp;
}

INLINE
long4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v4i64_v4i64, )( long4 x,
                                          long4 y )
{
    long4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s3, y.s3);
    return temp;
}

INLINE
long8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v8i64_v8i64, )( long8 x,
                                          long8 y )
{
    long8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s7, y.s7);
    return temp;
}

INLINE
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_hadd, _v16i64_v16i64, )( long16 x,
                                             long16 y )
{
    long16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(s_hadd, _i64_i64, )(x.sf, y.sf);
    return temp;
}

INLINE
ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )( ulong x,
                                      ulong y )
{
    ulong hi = 0;
    ulong result = (ulong)x + (ulong)y;
    if ((ulong)result < (ulong)x)
        hi = 1;
    return ((result >> 1) & 0x7FFFFFFFFFFFFFFF)|(hi << 63);
}

INLINE
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v2i64_v2i64, )( ulong2 x,
                                           ulong2 y )
{
    ulong2 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s1, y.s1);
    return temp;
}

INLINE
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v3i64_v3i64, )( ulong3 x,
                                           ulong3 y )
{
    ulong3 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s2, y.s2);
    return temp;
}

INLINE
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v4i64_v4i64, )( ulong4 x,
                                           ulong4 y )
{
    ulong4 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s3, y.s3);
    return temp;
}

INLINE
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v8i64_v8i64, )( ulong8 x,
                                           ulong8 y )
{
    ulong8 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s7, y.s7);
    return temp;
}

INLINE
ulong16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_hadd, _v16i64_v16i64, )( ulong16 x,
                                              ulong16 y )
{
    ulong16 temp;
    temp.s0 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s0, y.s0);
    temp.s1 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s1, y.s1);
    temp.s2 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s2, y.s2);
    temp.s3 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s3, y.s3);
    temp.s4 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s4, y.s4);
    temp.s5 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s5, y.s5);
    temp.s6 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s6, y.s6);
    temp.s7 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s7, y.s7);
    temp.s8 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s8, y.s8);
    temp.s9 = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.s9, y.s9);
    temp.sa = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.sa, y.sa);
    temp.sb = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.sb, y.sb);
    temp.sc = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.sc, y.sc);
    temp.sd = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.sd, y.sd);
    temp.se = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.se, y.se);
    temp.sf = SPIRV_OCL_BUILTIN(u_hadd, _i64_i64, )(x.sf, y.sf);
    return temp;
}

