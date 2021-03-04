/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
short OVERLOADABLE upsample( char  hi,
                             uchar lo )
{
    return __builtin_spirv_OpenCL_s_upsample_i8_i8( hi, lo );
}

INLINE
short2 OVERLOADABLE upsample( char2  hi,
                              uchar2 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v2i8_v2i8( hi, lo );
}

INLINE
short3 OVERLOADABLE upsample( char3  hi,
                              uchar3 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v3i8_v3i8( hi, lo );
}

INLINE
short4 OVERLOADABLE upsample( char4  hi,
                              uchar4 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v4i8_v4i8( hi, lo );
}

INLINE
short8 OVERLOADABLE upsample( char8  hi,
                              uchar8 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v8i8_v8i8( hi, lo );
}

INLINE
short16 OVERLOADABLE upsample( char16  hi,
                               uchar16 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v16i8_v16i8( hi, lo );
}

INLINE
ushort OVERLOADABLE upsample( uchar hi,
                              uchar lo )
{
    return __builtin_spirv_OpenCL_u_upsample_i8_i8( hi, lo );
}

INLINE
ushort2 OVERLOADABLE upsample( uchar2 hi,
                               uchar2 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v2i8_v2i8( hi, lo );
}

INLINE
ushort3 OVERLOADABLE upsample( uchar3 hi,
                               uchar3 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v3i8_v3i8( hi, lo );
}

INLINE
ushort4 OVERLOADABLE upsample( uchar4 hi,
                               uchar4 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v4i8_v4i8( hi, lo );
}

INLINE
ushort8 OVERLOADABLE upsample( uchar8 hi,
                               uchar8 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v8i8_v8i8( hi, lo );
}

INLINE
ushort16 OVERLOADABLE upsample( uchar16 hi,
                                uchar16 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v16i8_v16i8( hi, lo );
}

INLINE
int OVERLOADABLE upsample( short  hi,
                           ushort lo )
{
    return __builtin_spirv_OpenCL_s_upsample_i16_i16( hi, lo );
}

INLINE
int2 OVERLOADABLE upsample( short2  hi,
                            ushort2 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v2i16_v2i16( hi, lo );
}

INLINE
int3 OVERLOADABLE upsample( short3  hi,
                            ushort3 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v3i16_v3i16( hi, lo );
}

INLINE
int4 OVERLOADABLE upsample( short4  hi,
                            ushort4 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v4i16_v4i16( hi, lo );
}

INLINE
int8 OVERLOADABLE upsample( short8  hi,
                            ushort8 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v8i16_v8i16( hi, lo );
}

INLINE
int16 OVERLOADABLE upsample( short16  hi,
                             ushort16 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v16i16_v16i16( hi, lo );
}

INLINE
uint OVERLOADABLE upsample( ushort hi,
                            ushort lo )
{
    return __builtin_spirv_OpenCL_u_upsample_i16_i16( hi, lo );
}

INLINE
uint2 OVERLOADABLE upsample( ushort2 hi,
                             ushort2 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v2i16_v2i16( hi, lo );
}

INLINE
uint3 OVERLOADABLE upsample( ushort3 hi,
                             ushort3 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v3i16_v3i16( hi, lo );
}

INLINE
uint4 OVERLOADABLE upsample( ushort4 hi,
                             ushort4 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v4i16_v4i16( hi, lo );
}

INLINE
uint8 OVERLOADABLE upsample( ushort8 hi,
                             ushort8 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v8i16_v8i16( hi, lo );
}

INLINE
uint16 OVERLOADABLE upsample( ushort16 hi,
                              ushort16 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v16i16_v16i16( hi, lo );
}

INLINE
long OVERLOADABLE upsample( int  hi,
                            uint lo )
{
    return __builtin_spirv_OpenCL_s_upsample_i32_i32( hi, lo );
}

INLINE
long2 OVERLOADABLE upsample( int2  hi,
                             uint2 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v2i32_v2i32( hi, lo );
}

INLINE
long3 OVERLOADABLE upsample( int3  hi,
                             uint3 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v3i32_v3i32( hi, lo );
}

INLINE
long4 OVERLOADABLE upsample( int4  hi,
                             uint4 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v4i32_v4i32( hi, lo );
}

INLINE
long8 OVERLOADABLE upsample( int8  hi,
                             uint8 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v8i32_v8i32( hi, lo );
}

INLINE
long16 OVERLOADABLE upsample( int16  hi,
                              uint16 lo )
{
    return __builtin_spirv_OpenCL_s_upsample_v16i32_v16i32( hi, lo );
}

INLINE
ulong OVERLOADABLE upsample( uint hi,
                             uint lo )
{
    return __builtin_spirv_OpenCL_u_upsample_i32_i32( hi, lo );
}

INLINE
ulong2 OVERLOADABLE upsample( uint2 hi,
                              uint2 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v2i32_v2i32( hi, lo );
}

INLINE
ulong3 OVERLOADABLE upsample( uint3 hi,
                              uint3 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v3i32_v3i32( hi, lo );
}

INLINE
ulong4 OVERLOADABLE upsample( uint4 hi,
                              uint4 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v4i32_v4i32( hi, lo );
}

INLINE
ulong8 OVERLOADABLE upsample( uint8 hi,
                              uint8 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v8i32_v8i32( hi, lo );
}

INLINE
ulong16 OVERLOADABLE upsample( uint16 hi,
                               uint16 lo )
{
    return __builtin_spirv_OpenCL_u_upsample_v16i32_v16i32( hi, lo );
}

