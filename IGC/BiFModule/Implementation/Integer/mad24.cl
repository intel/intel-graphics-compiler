/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"




INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_mad24( int2 x,
                                                int2 y,
                                                int2 z )
{
    int2 temp;
    temp.s0 = __spirv_ocl_s_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_s_mad24(x.s1, y.s1, z.s1);
    return temp;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_mad24( int3 x,
                                                int3 y,
                                                int3 z )
{
    int3 temp;
    temp.s0 = __spirv_ocl_s_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_s_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_s_mad24(x.s2, y.s2, z.s2);
    return temp;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_mad24( int4 x,
                                                int4 y,
                                                int4 z )
{
    int4 temp;
    temp.s0 = __spirv_ocl_s_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_s_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_s_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_s_mad24(x.s3, y.s3, z.s3);
    return temp;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_mad24( int8 x,
                                                int8 y,
                                                int8 z )
{
    int8 temp;
    temp.s0 = __spirv_ocl_s_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_s_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_s_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_s_mad24(x.s3, y.s3, z.s3);
    temp.s4 = __spirv_ocl_s_mad24(x.s4, y.s4, z.s4);
    temp.s5 = __spirv_ocl_s_mad24(x.s5, y.s5, z.s5);
    temp.s6 = __spirv_ocl_s_mad24(x.s6, y.s6, z.s6);
    temp.s7 = __spirv_ocl_s_mad24(x.s7, y.s7, z.s7);
    return temp;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_mad24( int16 x,
                                                    int16 y,
                                                    int16 z )
{
    int16 temp;
    temp.s0 = __spirv_ocl_s_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_s_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_s_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_s_mad24(x.s3, y.s3, z.s3);
    temp.s4 = __spirv_ocl_s_mad24(x.s4, y.s4, z.s4);
    temp.s5 = __spirv_ocl_s_mad24(x.s5, y.s5, z.s5);
    temp.s6 = __spirv_ocl_s_mad24(x.s6, y.s6, z.s6);
    temp.s7 = __spirv_ocl_s_mad24(x.s7, y.s7, z.s7);
    temp.s8 = __spirv_ocl_s_mad24(x.s8, y.s8, z.s8);
    temp.s9 = __spirv_ocl_s_mad24(x.s9, y.s9, z.s9);
    temp.sa = __spirv_ocl_s_mad24(x.sa, y.sa, z.sa);
    temp.sb = __spirv_ocl_s_mad24(x.sb, y.sb, z.sb);
    temp.sc = __spirv_ocl_s_mad24(x.sc, y.sc, z.sc);
    temp.sd = __spirv_ocl_s_mad24(x.sd, y.sd, z.sd);
    temp.se = __spirv_ocl_s_mad24(x.se, y.se, z.se);
    temp.sf = __spirv_ocl_s_mad24(x.sf, y.sf, z.sf);
    return temp;
}



INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_mad24( uint2 x,
                                                 uint2 y,
                                                 uint2 z )
{
    uint2 temp;
    temp.s0 = __spirv_ocl_u_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_u_mad24(x.s1, y.s1, z.s1);
    return temp;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_mad24( uint3 x,
                                                 uint3 y,
                                                 uint3 z )
{
    uint3 temp;
    temp.s0 = __spirv_ocl_u_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_u_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_u_mad24(x.s2, y.s2, z.s2);
    return temp;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_mad24( uint4 x,
                                                 uint4 y,
                                                 uint4 z )
{
    uint4 temp;
    temp.s0 = __spirv_ocl_u_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_u_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_u_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_u_mad24(x.s3, y.s3, z.s3);
    return temp;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_mad24( uint8 x,
                                                 uint8 y,
                                                 uint8 z )
{
    uint8 temp;
    temp.s0 = __spirv_ocl_u_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_u_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_u_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_u_mad24(x.s3, y.s3, z.s3);
    temp.s4 = __spirv_ocl_u_mad24(x.s4, y.s4, z.s4);
    temp.s5 = __spirv_ocl_u_mad24(x.s5, y.s5, z.s5);
    temp.s6 = __spirv_ocl_u_mad24(x.s6, y.s6, z.s6);
    temp.s7 = __spirv_ocl_u_mad24(x.s7, y.s7, z.s7);
    return temp;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad24( uint16 x,
                                                     uint16 y,
                                                     uint16 z )
{
    uint16 temp;
    temp.s0 = __spirv_ocl_u_mad24(x.s0, y.s0, z.s0);
    temp.s1 = __spirv_ocl_u_mad24(x.s1, y.s1, z.s1);
    temp.s2 = __spirv_ocl_u_mad24(x.s2, y.s2, z.s2);
    temp.s3 = __spirv_ocl_u_mad24(x.s3, y.s3, z.s3);
    temp.s4 = __spirv_ocl_u_mad24(x.s4, y.s4, z.s4);
    temp.s5 = __spirv_ocl_u_mad24(x.s5, y.s5, z.s5);
    temp.s6 = __spirv_ocl_u_mad24(x.s6, y.s6, z.s6);
    temp.s7 = __spirv_ocl_u_mad24(x.s7, y.s7, z.s7);
    temp.s8 = __spirv_ocl_u_mad24(x.s8, y.s8, z.s8);
    temp.s9 = __spirv_ocl_u_mad24(x.s9, y.s9, z.s9);
    temp.sa = __spirv_ocl_u_mad24(x.sa, y.sa, z.sa);
    temp.sb = __spirv_ocl_u_mad24(x.sb, y.sb, z.sb);
    temp.sc = __spirv_ocl_u_mad24(x.sc, y.sc, z.sc);
    temp.sd = __spirv_ocl_u_mad24(x.sd, y.sd, z.sd);
    temp.se = __spirv_ocl_u_mad24(x.se, y.se, z.se);
    temp.sf = __spirv_ocl_u_mad24(x.sf, y.sf, z.sf);
    return temp;
}

