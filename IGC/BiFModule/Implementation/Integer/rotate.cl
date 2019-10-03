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
uchar __builtin_spirv_OpenCL_rotate_i8_i8( uchar v,
                                    uchar i )
{
    uchar temp = i % 8;
    return (v << temp) | (v >> (8 - temp));
}

INLINE
uchar2 __builtin_spirv_OpenCL_rotate_v2i8_v2i8( uchar2 v,
                                         uchar2 i )
{
    uchar2 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s1, i.s1);
    return temp;
}

INLINE
uchar3 __builtin_spirv_OpenCL_rotate_v3i8_v3i8( uchar3 v,
                                         uchar3 i )
{
    uchar3 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s2, i.s2);
    return temp;
}

INLINE
uchar4 __builtin_spirv_OpenCL_rotate_v4i8_v4i8( uchar4 v,
                                         uchar4 i )
{
    uchar4 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s3, i.s3);
    return temp;
}

INLINE
uchar8 __builtin_spirv_OpenCL_rotate_v8i8_v8i8( uchar8 v,
                                         uchar8 i )
{
    uchar8 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s7, i.s7);
    return temp;
}

INLINE
uchar16 __builtin_spirv_OpenCL_rotate_v16i8_v16i8( uchar16 v,
                                            uchar16 i )
{
    uchar16 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s7, i.s7);
    temp.s8 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s8, i.s8);
    temp.s9 = __builtin_spirv_OpenCL_rotate_i8_i8(v.s9, i.s9);
    temp.sa = __builtin_spirv_OpenCL_rotate_i8_i8(v.sa, i.sa);
    temp.sb = __builtin_spirv_OpenCL_rotate_i8_i8(v.sb, i.sb);
    temp.sc = __builtin_spirv_OpenCL_rotate_i8_i8(v.sc, i.sc);
    temp.sd = __builtin_spirv_OpenCL_rotate_i8_i8(v.sd, i.sd);
    temp.se = __builtin_spirv_OpenCL_rotate_i8_i8(v.se, i.se);
    temp.sf = __builtin_spirv_OpenCL_rotate_i8_i8(v.sf, i.sf);
    return temp;
}

INLINE
ushort __builtin_spirv_OpenCL_rotate_i16_i16( ushort v,
                                       ushort i )
{
    ushort temp = i % 16;
    return (v << temp) | (v >> (16 - temp));
}

INLINE
ushort2 __builtin_spirv_OpenCL_rotate_v2i16_v2i16( ushort2 v,
                                            ushort2 i )
{
    ushort2 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s1, i.s1);
    return temp;
}

INLINE
ushort3 __builtin_spirv_OpenCL_rotate_v3i16_v3i16( ushort3 v,
                                            ushort3 i )
{
    ushort3 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s2, i.s2);
    return temp;
}

INLINE
ushort4 __builtin_spirv_OpenCL_rotate_v4i16_v4i16( ushort4 v,
                                            ushort4 i )
{
    ushort4 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s3, i.s3);
    return temp;
}

INLINE
ushort8 __builtin_spirv_OpenCL_rotate_v8i16_v8i16( ushort8 v,
                                            ushort8 i )
{
    ushort8 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s7, i.s7);
    return temp;
}

INLINE
ushort16 __builtin_spirv_OpenCL_rotate_v16i16_v16i16( ushort16 v,
                                               ushort16 i )
{
    ushort16 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s7, i.s7);
    temp.s8 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s8, i.s8);
    temp.s9 = __builtin_spirv_OpenCL_rotate_i16_i16(v.s9, i.s9);
    temp.sa = __builtin_spirv_OpenCL_rotate_i16_i16(v.sa, i.sa);
    temp.sb = __builtin_spirv_OpenCL_rotate_i16_i16(v.sb, i.sb);
    temp.sc = __builtin_spirv_OpenCL_rotate_i16_i16(v.sc, i.sc);
    temp.sd = __builtin_spirv_OpenCL_rotate_i16_i16(v.sd, i.sd);
    temp.se = __builtin_spirv_OpenCL_rotate_i16_i16(v.se, i.se);
    temp.sf = __builtin_spirv_OpenCL_rotate_i16_i16(v.sf, i.sf);
    return temp;
}

INLINE
uint __builtin_spirv_OpenCL_rotate_i32_i32( uint v,
                                     uint i )
{
    uint temp = i % 32;
    return (v << temp) | (v >> (32 - temp));
}

INLINE
uint2 __builtin_spirv_OpenCL_rotate_v2i32_v2i32( uint2 v,
                                          uint2 i )
{
    uint2 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s1, i.s1);
    return temp;
}

INLINE
uint3 __builtin_spirv_OpenCL_rotate_v3i32_v3i32( uint3 v,
                                          uint3 i )
{
    uint3 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s2, i.s2);
    return temp;
}

INLINE
uint4 __builtin_spirv_OpenCL_rotate_v4i32_v4i32( uint4 v,
                                          uint4 i )
{
    uint4 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s3, i.s3);
    return temp;
}

INLINE
uint8 __builtin_spirv_OpenCL_rotate_v8i32_v8i32( uint8 v,
                                          uint8 i )
{
    uint8 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s7, i.s7);
    return temp;
}

INLINE
uint16 __builtin_spirv_OpenCL_rotate_v16i32_v16i32( uint16 v,
                                             uint16 i )
{
    uint16 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s7, i.s7);
    temp.s8 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s8, i.s8);
    temp.s9 = __builtin_spirv_OpenCL_rotate_i32_i32(v.s9, i.s9);
    temp.sa = __builtin_spirv_OpenCL_rotate_i32_i32(v.sa, i.sa);
    temp.sb = __builtin_spirv_OpenCL_rotate_i32_i32(v.sb, i.sb);
    temp.sc = __builtin_spirv_OpenCL_rotate_i32_i32(v.sc, i.sc);
    temp.sd = __builtin_spirv_OpenCL_rotate_i32_i32(v.sd, i.sd);
    temp.se = __builtin_spirv_OpenCL_rotate_i32_i32(v.se, i.se);
    temp.sf = __builtin_spirv_OpenCL_rotate_i32_i32(v.sf, i.sf);
    return temp;
}

INLINE
ulong __builtin_spirv_OpenCL_rotate_i64_i64( ulong v,
                                      ulong i )
{
    ulong temp = i % 64;
    return (v << temp) | (v >> (64 - temp));
}

INLINE
ulong2 __builtin_spirv_OpenCL_rotate_v2i64_v2i64( ulong2 v,
                                           ulong2 i )
{
    ulong2 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s1, i.s1);
    return temp;
}

INLINE
ulong3 __builtin_spirv_OpenCL_rotate_v3i64_v3i64( ulong3 v,
                                           ulong3 i )
{
    ulong3 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s2, i.s2);
    return temp;
}

INLINE
ulong4 __builtin_spirv_OpenCL_rotate_v4i64_v4i64( ulong4 v,
                                           ulong4 i )
{
    ulong4 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s3, i.s3);
    return temp;
}

INLINE
ulong8 __builtin_spirv_OpenCL_rotate_v8i64_v8i64( ulong8 v,
                                           ulong8 i )
{
    ulong8 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s7, i.s7);
    return temp;
}

INLINE
ulong16 __builtin_spirv_OpenCL_rotate_v16i64_v16i64( ulong16 v,
                                              ulong16 i )
{
    ulong16 temp;
    temp.s0 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s0, i.s0);
    temp.s1 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s1, i.s1);
    temp.s2 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s2, i.s2);
    temp.s3 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s3, i.s3);
    temp.s4 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s4, i.s4);
    temp.s5 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s5, i.s5);
    temp.s6 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s6, i.s6);
    temp.s7 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s7, i.s7);
    temp.s8 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s8, i.s8);
    temp.s9 = __builtin_spirv_OpenCL_rotate_i64_i64(v.s9, i.s9);
    temp.sa = __builtin_spirv_OpenCL_rotate_i64_i64(v.sa, i.sa);
    temp.sb = __builtin_spirv_OpenCL_rotate_i64_i64(v.sb, i.sb);
    temp.sc = __builtin_spirv_OpenCL_rotate_i64_i64(v.sc, i.sc);
    temp.sd = __builtin_spirv_OpenCL_rotate_i64_i64(v.sd, i.sd);
    temp.se = __builtin_spirv_OpenCL_rotate_i64_i64(v.se, i.se);
    temp.sf = __builtin_spirv_OpenCL_rotate_i64_i64(v.sf, i.sf);
    return temp;
}

