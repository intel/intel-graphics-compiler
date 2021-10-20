/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE
char OVERLOADABLE mul_hi( char x,
                          char y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _i8_i8, )( x, y );
}

INLINE
char2 OVERLOADABLE mul_hi( char2 x,
                           char2 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v2i8_v2i8, )( x, y );
}

INLINE
char3 OVERLOADABLE mul_hi( char3 x,
                           char3 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v3i8_v3i8, )( x, y );
}

INLINE
char4 OVERLOADABLE mul_hi( char4 x,
                           char4 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v4i8_v4i8, )( x, y );
}

INLINE
char8 OVERLOADABLE mul_hi( char8 x,
                           char8 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v8i8_v8i8, )( x, y );
}

INLINE
char16 OVERLOADABLE mul_hi( char16 x,
                            char16 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v16i8_v16i8, )( x, y );
}

INLINE
uchar OVERLOADABLE mul_hi( uchar x,
                           uchar y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _i8_i8, )( x, y );
}

INLINE
uchar2 OVERLOADABLE mul_hi( uchar2 x,
                            uchar2 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v2i8_v2i8, )( x, y );
}

INLINE
uchar3 OVERLOADABLE mul_hi( uchar3 x,
                            uchar3 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v3i8_v3i8, )( x, y );
}

INLINE
uchar4 OVERLOADABLE mul_hi( uchar4 x,
                            uchar4 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v4i8_v4i8, )( x, y );
}

INLINE
uchar8 OVERLOADABLE mul_hi( uchar8 x,
                            uchar8 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v8i8_v8i8, )( x, y );
}

INLINE
uchar16 OVERLOADABLE mul_hi( uchar16 x,
                             uchar16 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v16i8_v16i8, )( x, y );
}

INLINE
short OVERLOADABLE mul_hi( short x,
                           short y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _i16_i16, )( x, y );
}

INLINE
short2 OVERLOADABLE mul_hi( short2 x,
                            short2 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v2i16_v2i16, )( x, y );
}

INLINE
short3 OVERLOADABLE mul_hi( short3 x,
                            short3 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v3i16_v3i16, )( x, y );
}

INLINE
short4 OVERLOADABLE mul_hi( short4 x,
                            short4 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v4i16_v4i16, )( x, y );
}

INLINE
short8 OVERLOADABLE mul_hi( short8 x,
                            short8 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v8i16_v8i16, )( x, y );
}

INLINE
short16 OVERLOADABLE mul_hi( short16 x,
                             short16 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v16i16_v16i16, )( x, y );
}

INLINE
ushort OVERLOADABLE mul_hi( ushort x,
                            ushort y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _i16_i16, )( x, y );
}

INLINE
ushort2 OVERLOADABLE mul_hi( ushort2 x,
                             ushort2 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v2i16_v2i16, )( x, y );
}

INLINE
ushort3 OVERLOADABLE mul_hi( ushort3 x,
                             ushort3 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v3i16_v3i16, )( x, y );
}

INLINE
ushort4 OVERLOADABLE mul_hi( ushort4 x,
                             ushort4 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v4i16_v4i16, )( x, y );
}

INLINE
ushort8 OVERLOADABLE mul_hi( ushort8 x,
                             ushort8 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v8i16_v8i16, )( x, y );
}

INLINE
ushort16 OVERLOADABLE mul_hi( ushort16 x,
                              ushort16 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v16i16_v16i16, )( x, y );
}



INLINE
int2 OVERLOADABLE mul_hi( int2 x,
                          int2 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v2i32_v2i32, )( x, y );
}

INLINE
int3 OVERLOADABLE mul_hi( int3 x,
                          int3 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v3i32_v3i32, )( x, y );
}

INLINE
int4 OVERLOADABLE mul_hi( int4 x,
                          int4 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v4i32_v4i32, )( x, y );
}

INLINE
int8 OVERLOADABLE mul_hi( int8 x,
                          int8 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v8i32_v8i32, )( x, y );
}

INLINE
int16 OVERLOADABLE mul_hi( int16 x,
                           int16 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v16i32_v16i32, )( x, y );
}



INLINE
uint2 OVERLOADABLE mul_hi( uint2 x,
                           uint2 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v2i32_v2i32, )( x, y );
}

INLINE
uint3 OVERLOADABLE mul_hi( uint3 x,
                           uint3 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v3i32_v3i32, )( x, y );
}

INLINE
uint4 OVERLOADABLE mul_hi( uint4 x,
                           uint4 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v4i32_v4i32, )( x, y );
}

INLINE
uint8 OVERLOADABLE mul_hi( uint8 x,
                           uint8 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v8i32_v8i32, )( x, y );
}

INLINE
uint16 OVERLOADABLE mul_hi( uint16 x,
                            uint16 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v16i32_v16i32, )( x, y );
}

INLINE
long OVERLOADABLE mul_hi( long x,
                          long y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _i64_i64, )( x, y );
}

INLINE
long2 OVERLOADABLE mul_hi( long2 x,
                           long2 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v2i64_v2i64, )( x, y );
}

INLINE
long3 OVERLOADABLE mul_hi( long3 x,
                           long3 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v3i64_v3i64, )( x, y );
}

INLINE
long4 OVERLOADABLE mul_hi( long4 x,
                           long4 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v4i64_v4i64, )( x, y );
}

INLINE
long8 OVERLOADABLE mul_hi( long8 x,
                           long8 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v8i64_v8i64, )( x, y );
}

INLINE
long16 OVERLOADABLE mul_hi( long16 x,
                            long16 y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _v16i64_v16i64, )( x, y );
}

INLINE
ulong OVERLOADABLE mul_hi( ulong x,
                           ulong y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _i64_i64, )( x, y );
}

INLINE
ulong2 OVERLOADABLE mul_hi( ulong2 x,
                            ulong2 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v2i64_v2i64, )( x, y );
}

INLINE
ulong3 OVERLOADABLE mul_hi( ulong3 x,
                            ulong3 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v3i64_v3i64, )( x, y );
}

INLINE
ulong4 OVERLOADABLE mul_hi( ulong4 x,
                            ulong4 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v4i64_v4i64, )( x, y );
}

INLINE
ulong8 OVERLOADABLE mul_hi( ulong8 x,
                            ulong8 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v8i64_v8i64, )( x, y );
}

INLINE
ulong16 OVERLOADABLE mul_hi( ulong16 x,
                             ulong16 y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _v16i64_v16i64, )( x, y );
}

