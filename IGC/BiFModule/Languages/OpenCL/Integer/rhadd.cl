/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
char OVERLOADABLE rhadd( char x,
                         char y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _i8_i8, )( x, y );
}

INLINE
char2 OVERLOADABLE rhadd( char2 x,
                          char2 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v2i8_v2i8, )( x, y );
}

INLINE
char3 OVERLOADABLE rhadd( char3 x,
                          char3 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v3i8_v3i8, )( x, y );
}

INLINE
char4 OVERLOADABLE rhadd( char4 x,
                          char4 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v4i8_v4i8, )( x, y );
}

INLINE
char8 OVERLOADABLE rhadd( char8 x,
                          char8 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v8i8_v8i8, )( x, y );
}

INLINE
char16 OVERLOADABLE rhadd( char16 x,
                           char16 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v16i8_v16i8, )( x, y );
}

INLINE
uchar OVERLOADABLE rhadd( uchar x,
                          uchar y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _i8_i8, )( x, y );
}

INLINE
uchar2 OVERLOADABLE rhadd( uchar2 x,
                           uchar2 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v2i8_v2i8, )( x, y );
}

INLINE
uchar3 OVERLOADABLE rhadd( uchar3 x,
                           uchar3 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v3i8_v3i8, )( x, y );
}

INLINE
uchar4 OVERLOADABLE rhadd( uchar4 x,
                           uchar4 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v4i8_v4i8, )( x, y );
}

INLINE
uchar8 OVERLOADABLE rhadd( uchar8 x,
                           uchar8 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v8i8_v8i8, )( x, y );
}

INLINE
uchar16 OVERLOADABLE rhadd( uchar16 x,
                            uchar16 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v16i8_v16i8, )( x, y );
}

INLINE
short OVERLOADABLE rhadd( short x,
                          short y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _i16_i16, )( x, y );
}

INLINE
short2 OVERLOADABLE rhadd( short2 x,
                           short2 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v2i16_v2i16, )( x, y );
}

INLINE
short3 OVERLOADABLE rhadd( short3 x,
                           short3 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v3i16_v3i16, )( x, y );
}

INLINE
short4 OVERLOADABLE rhadd( short4 x,
                           short4 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v4i16_v4i16, )( x, y );
}

INLINE
short8 OVERLOADABLE rhadd( short8 x,
                           short8 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v8i16_v8i16, )( x, y );
}

INLINE
short16 OVERLOADABLE rhadd( short16 x,
                            short16 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v16i16_v16i16, )( x, y );
}

INLINE
ushort OVERLOADABLE rhadd( ushort x,
                           ushort y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _i16_i16, )( x, y );
}

INLINE
ushort2 OVERLOADABLE rhadd( ushort2 x,
                            ushort2 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v2i16_v2i16, )( x, y );
}

INLINE
ushort3 OVERLOADABLE rhadd( ushort3 x,
                            ushort3 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v3i16_v3i16, )( x, y );
}

INLINE
ushort4 OVERLOADABLE rhadd( ushort4 x,
                            ushort4 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v4i16_v4i16, )( x, y );
}

INLINE
ushort8 OVERLOADABLE rhadd( ushort8 x,
                            ushort8 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v8i16_v8i16, )( x, y );
}

INLINE
ushort16 OVERLOADABLE rhadd( ushort16 x,
                             ushort16 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v16i16_v16i16, )( x, y );
}

INLINE
int OVERLOADABLE rhadd( int x,
                        int y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _i32_i32, )( x, y );
}

INLINE
int2 OVERLOADABLE rhadd( int2 x,
                         int2 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v2i32_v2i32, )( x, y );
}

INLINE
int3 OVERLOADABLE rhadd( int3 x,
                         int3 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v3i32_v3i32, )( x, y );
}

INLINE
int4 OVERLOADABLE rhadd( int4 x,
                         int4 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v4i32_v4i32, )( x, y );
}

INLINE
int8 OVERLOADABLE rhadd( int8 x,
                         int8 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v8i32_v8i32, )( x, y );
}

INLINE
int16 OVERLOADABLE rhadd( int16 x,
                          int16 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v16i32_v16i32, )( x, y );
}

INLINE
uint OVERLOADABLE rhadd( uint x,
                         uint y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _i32_i32, )( x, y );
}

INLINE
uint2 OVERLOADABLE rhadd( uint2 x,
                          uint2 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v2i32_v2i32, )( x, y );
}

INLINE
uint3 OVERLOADABLE rhadd( uint3 x,
                          uint3 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v3i32_v3i32, )( x, y );
}

INLINE
uint4 OVERLOADABLE rhadd( uint4 x,
                          uint4 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v4i32_v4i32, )( x, y );
}

INLINE
uint8 OVERLOADABLE rhadd( uint8 x,
                          uint8 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v8i32_v8i32, )( x, y );
}

INLINE
uint16 OVERLOADABLE rhadd( uint16 x,
                           uint16 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v16i32_v16i32, )( x, y );
}

INLINE
long OVERLOADABLE rhadd( long x,
                         long y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _i64_i64, )( x, y );
}

INLINE
long2 OVERLOADABLE rhadd( long2 x,
                          long2 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v2i64_v2i64, )( x, y );
}

INLINE
long3 OVERLOADABLE rhadd( long3 x,
                          long3 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v3i64_v3i64, )( x, y );
}

INLINE
long4 OVERLOADABLE rhadd( long4 x,
                          long4 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v4i64_v4i64, )( x, y );
}

INLINE
long8 OVERLOADABLE rhadd( long8 x,
                          long8 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v8i64_v8i64, )( x, y );
}

INLINE
long16 OVERLOADABLE rhadd( long16 x,
                           long16 y )
{
    return SPIRV_OCL_BUILTIN(s_rhadd, _v16i64_v16i64, )( x, y );
}

INLINE
ulong OVERLOADABLE rhadd( ulong x,
                          ulong y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _i64_i64, )( x, y );
}

INLINE
ulong2 OVERLOADABLE rhadd( ulong2 x,
                           ulong2 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v2i64_v2i64, )( x, y );
}

INLINE
ulong3 OVERLOADABLE rhadd( ulong3 x,
                           ulong3 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v3i64_v3i64, )( x, y );
}

INLINE
ulong4 OVERLOADABLE rhadd( ulong4 x,
                           ulong4 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v4i64_v4i64, )( x, y );
}

INLINE
ulong8 OVERLOADABLE rhadd( ulong8 x,
                           ulong8 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v8i64_v8i64, )( x, y );
}

INLINE
ulong16 OVERLOADABLE rhadd( ulong16 x,
                            ulong16 y )
{
    return SPIRV_OCL_BUILTIN(u_rhadd, _v16i64_v16i64, )( x, y );
}

