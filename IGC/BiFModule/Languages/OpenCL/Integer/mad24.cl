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
#include "spirv.h"




INLINE
int2 OVERLOADABLE mad24( int2 x,
                         int2 y,
                         int2 z )
{
    return __builtin_spirv_OpenCL_s_mad24_v2i32_v2i32_v2i32( x, y, z );
}

INLINE
int3 OVERLOADABLE mad24( int3 x,
                         int3 y,
                         int3 z )
{
    return __builtin_spirv_OpenCL_s_mad24_v3i32_v3i32_v3i32( x, y, z );
}

INLINE
int4 OVERLOADABLE mad24( int4 x,
                         int4 y,
                         int4 z )
{
    return __builtin_spirv_OpenCL_s_mad24_v4i32_v4i32_v4i32( x, y, z );
}

INLINE
int8 OVERLOADABLE mad24( int8 x,
                         int8 y,
                         int8 z )
{
    return __builtin_spirv_OpenCL_s_mad24_v8i32_v8i32_v8i32( x, y, z );
}

INLINE
int16 OVERLOADABLE mad24( int16 x,
                          int16 y,
                          int16 z )
{
    return __builtin_spirv_OpenCL_s_mad24_v16i32_v16i32_v16i32( x, y, z );
}



INLINE
uint2 OVERLOADABLE mad24( uint2 x,
                          uint2 y,
                          uint2 z )
{
    return __builtin_spirv_OpenCL_u_mad24_v2i32_v2i32_v2i32( x, y, z );
}

INLINE
uint3 OVERLOADABLE mad24( uint3 x,
                          uint3 y,
                          uint3 z )
{
    return __builtin_spirv_OpenCL_u_mad24_v3i32_v3i32_v3i32( x, y, z );
}

INLINE
uint4 OVERLOADABLE mad24( uint4 x,
                          uint4 y,
                          uint4 z )
{
    return __builtin_spirv_OpenCL_u_mad24_v4i32_v4i32_v4i32( x, y, z );
}

INLINE
uint8 OVERLOADABLE mad24( uint8 x,
                          uint8 y,
                          uint8 z )
{
    return __builtin_spirv_OpenCL_u_mad24_v8i32_v8i32_v8i32( x, y, z );
}

INLINE
uint16 OVERLOADABLE mad24( uint16 x,
                           uint16 y,
                           uint16 z )
{
    return __builtin_spirv_OpenCL_u_mad24_v16i32_v16i32_v16i32( x, y, z );
}

