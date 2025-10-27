/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../include/mul_hilo.cl"
#include "../ExternalLibraries/libclc/mad_sat.cl"

INLINE
char2 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( char2 a,
                                                char2 b,
                                                char2 c )
{
    char2 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( char3 a,
                                                char3 b,
                                                char3 c )
{
    char3 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( char4 a,
                                                char4 b,
                                                char4 c )
{
    char4 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( char8 a,
                                                char8 b,
                                                char8 c )
{
    char8 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( char16 a,
                                                    char16 b,
                                                    char16 c )
{
    char16 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_s_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_s_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_s_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_s_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_s_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_s_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_s_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_s_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}


INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uchar2 a,
                                                 uchar2 b,
                                                 uchar2 c )
{
    uchar2 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uchar3 a,
                                                 uchar3 b,
                                                 uchar3 c )
{
    uchar3 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uchar4 a,
                                                 uchar4 b,
                                                 uchar4 c )
{
    uchar4 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uchar8 a,
                                                 uchar8 b,
                                                 uchar8 c )
{
    uchar8 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uchar16 a,
                                                     uchar16 b,
                                                     uchar16 c )
{
    uchar16 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_u_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_u_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_u_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_u_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_u_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_u_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_u_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_u_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}



INLINE
short2 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( short2 a,
                                                    short2 b,
                                                    short2 c )
{
    short2 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( short3 a,
                                                    short3 b,
                                                    short3 c )
{
    short3 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( short4 a,
                                                    short4 b,
                                                    short4 c )
{
    short4 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( short8 a,
                                                    short8 b,
                                                    short8 c )
{
    short8 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( short16 a,
                                                        short16 b,
                                                        short16 c )
{
    short16 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_s_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_s_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_s_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_s_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_s_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_s_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_s_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_s_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}



INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ushort2 a,
                                                     ushort2 b,
                                                     ushort2 c )
{
    ushort2 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ushort3 a,
                                                     ushort3 b,
                                                     ushort3 c )
{
    ushort3 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ushort4 a,
                                                     ushort4 b,
                                                     ushort4 c )
{
    ushort4 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ushort8 a,
                                                     ushort8 b,
                                                     ushort8 c )
{
    ushort8 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ushort16 a,
                                                         ushort16 b,
                                                         ushort16 c )
{
    ushort16 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_u_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_u_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_u_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_u_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_u_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_u_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_u_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_u_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( int2 a,
                                                  int2 b,
                                                  int2 c )
{
    int2 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( int3 a,
                                                  int3 b,
                                                  int3 c )
{
    int3 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( int4 a,
                                                  int4 b,
                                                  int4 c )
{
    int4 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( int8 a,
                                                  int8 b,
                                                  int8 c )
{
    int8 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( int16 a,
                                                      int16 b,
                                                      int16 c )
{
    int16 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_s_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_s_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_s_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_s_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_s_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_s_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_s_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_s_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}



INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint2 a,
                                                   uint2 b,
                                                   uint2 c )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint3 a,
                                                   uint3 b,
                                                   uint3 c )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint4 a,
                                                   uint4 b,
                                                   uint4 c )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint8 a,
                                                   uint8 b,
                                                   uint8 c )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint16 a,
                                                       uint16 b,
                                                       uint16 c )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_u_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_u_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_u_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_u_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_u_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_u_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_u_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_u_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long a,
                                            long b,
                                            long c )
{
    return libclc_mad_sat(a, b, c);
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long2 a,
                                                   long2 b,
                                                   long2 c )
{
    long2 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long3 a,
                                                   long3 b,
                                                   long3 c )
{
    long3 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long4 a,
                                                   long4 b,
                                                   long4 c )
{
    long4 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long8 a,
                                                   long8 b,
                                                   long8 c )
{
    long8 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_s_mad_sat( long16 a,
                                                       long16 b,
                                                       long16 c )
{
    long16 temp;
    temp.s0 = __spirv_ocl_s_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_s_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_s_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_s_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_s_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_s_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_s_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_s_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_s_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_s_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_s_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_s_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_s_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_s_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_s_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_s_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong a,
                                             ulong b,
                                             ulong c )
{
    ulong lo;
    ulong hi;
    hi = ___intc_umul_hilo(a, b, &lo);
    return (hi == 0) ? __spirv_ocl_u_add_sat(lo, c) :
    ULONG_MAX;
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong2 a,
                                                    ulong2 b,
                                                    ulong2 c )
{
    ulong2 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    return temp;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong3 a,
                                                    ulong3 b,
                                                    ulong3 c )
{
    ulong3 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    return temp;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong4 a,
                                                    ulong4 b,
                                                    ulong4 c )
{
    ulong4 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    return temp;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong8 a,
                                                    ulong8 b,
                                                    ulong8 c )
{
    ulong8 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    return temp;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_mad_sat( ulong16 a,
                                                        ulong16 b,
                                                        ulong16 c )
{
    ulong16 temp;
    temp.s0 = __spirv_ocl_u_mad_sat(a.s0, b.s0, c.s0);
    temp.s1 = __spirv_ocl_u_mad_sat(a.s1, b.s1, c.s1);
    temp.s2 = __spirv_ocl_u_mad_sat(a.s2, b.s2, c.s2);
    temp.s3 = __spirv_ocl_u_mad_sat(a.s3, b.s3, c.s3);
    temp.s4 = __spirv_ocl_u_mad_sat(a.s4, b.s4, c.s4);
    temp.s5 = __spirv_ocl_u_mad_sat(a.s5, b.s5, c.s5);
    temp.s6 = __spirv_ocl_u_mad_sat(a.s6, b.s6, c.s6);
    temp.s7 = __spirv_ocl_u_mad_sat(a.s7, b.s7, c.s7);
    temp.s8 = __spirv_ocl_u_mad_sat(a.s8, b.s8, c.s8);
    temp.s9 = __spirv_ocl_u_mad_sat(a.s9, b.s9, c.s9);
    temp.sa = __spirv_ocl_u_mad_sat(a.sa, b.sa, c.sa);
    temp.sb = __spirv_ocl_u_mad_sat(a.sb, b.sb, c.sb);
    temp.sc = __spirv_ocl_u_mad_sat(a.sc, b.sc, c.sc);
    temp.sd = __spirv_ocl_u_mad_sat(a.sd, b.sd, c.sd);
    temp.se = __spirv_ocl_u_mad_sat(a.se, b.se, c.se);
    temp.sf = __spirv_ocl_u_mad_sat(a.sf, b.sf, c.sf);
    return temp;
}

