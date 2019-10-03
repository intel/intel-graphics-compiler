/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"




INLINE
int2 __builtin_spirv_OpenCL_s_mad24_v2i32_v2i32_v2i32( int2 x,
                                                int2 y,
                                                int2 z )
{
    int2 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    return temp;
}

INLINE
int3 __builtin_spirv_OpenCL_s_mad24_v3i32_v3i32_v3i32( int3 x,
                                                int3 y,
                                                int3 z )
{
    int3 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    return temp;
}

INLINE
int4 __builtin_spirv_OpenCL_s_mad24_v4i32_v4i32_v4i32( int4 x,
                                                int4 y,
                                                int4 z )
{
    int4 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    return temp;
}

INLINE
int8 __builtin_spirv_OpenCL_s_mad24_v8i32_v8i32_v8i32( int8 x,
                                                int8 y,
                                                int8 z )
{
    int8 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s4, y.s4, z.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s5, y.s5, z.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s6, y.s6, z.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s7, y.s7, z.s7);
    return temp;
}

INLINE
int16 __builtin_spirv_OpenCL_s_mad24_v16i32_v16i32_v16i32( int16 x,
                                                    int16 y,
                                                    int16 z )
{
    int16 temp;
    temp.s0 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    temp.s4 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s4, y.s4, z.s4);
    temp.s5 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s5, y.s5, z.s5);
    temp.s6 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s6, y.s6, z.s6);
    temp.s7 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s7, y.s7, z.s7);
    temp.s8 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s8, y.s8, z.s8);
    temp.s9 = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.s9, y.s9, z.s9);
    temp.sa = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.sa, y.sa, z.sa);
    temp.sb = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.sb, y.sb, z.sb);
    temp.sc = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.sc, y.sc, z.sc);
    temp.sd = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.sd, y.sd, z.sd);
    temp.se = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.se, y.se, z.se);
    temp.sf = __builtin_spirv_OpenCL_s_mad24_i32_i32_i32(x.sf, y.sf, z.sf);
    return temp;
}



INLINE
uint2 __builtin_spirv_OpenCL_u_mad24_v2i32_v2i32_v2i32( uint2 x,
                                                 uint2 y,
                                                 uint2 z )
{
    uint2 temp;
    temp.s0 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    return temp;
}

INLINE
uint3 __builtin_spirv_OpenCL_u_mad24_v3i32_v3i32_v3i32( uint3 x,
                                                 uint3 y,
                                                 uint3 z )
{
    uint3 temp;
    temp.s0 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    return temp;
}

INLINE
uint4 __builtin_spirv_OpenCL_u_mad24_v4i32_v4i32_v4i32( uint4 x,
                                                 uint4 y,
                                                 uint4 z )
{
    uint4 temp;
    temp.s0 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    return temp;
}

INLINE
uint8 __builtin_spirv_OpenCL_u_mad24_v8i32_v8i32_v8i32( uint8 x,
                                                 uint8 y,
                                                 uint8 z )
{
    uint8 temp;
    temp.s0 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    temp.s4 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s4, y.s4, z.s4);
    temp.s5 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s5, y.s5, z.s5);
    temp.s6 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s6, y.s6, z.s6);
    temp.s7 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s7, y.s7, z.s7);
    return temp;
}

INLINE
uint16 __builtin_spirv_OpenCL_u_mad24_v16i32_v16i32_v16i32( uint16 x,
                                                     uint16 y,
                                                     uint16 z )
{
    uint16 temp;
    temp.s0 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s0, y.s0, z.s0);
    temp.s1 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s1, y.s1, z.s1);
    temp.s2 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s2, y.s2, z.s2);
    temp.s3 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s3, y.s3, z.s3);
    temp.s4 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s4, y.s4, z.s4);
    temp.s5 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s5, y.s5, z.s5);
    temp.s6 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s6, y.s6, z.s6);
    temp.s7 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s7, y.s7, z.s7);
    temp.s8 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s8, y.s8, z.s8);
    temp.s9 = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.s9, y.s9, z.s9);
    temp.sa = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.sa, y.sa, z.sa);
    temp.sb = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.sb, y.sb, z.sb);
    temp.sc = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.sc, y.sc, z.sc);
    temp.sd = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.sd, y.sd, z.sd);
    temp.se = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.se, y.se, z.se);
    temp.sf = __builtin_spirv_OpenCL_u_mad24_i32_i32_i32(x.sf, y.sf, z.sf);
    return temp;
}

