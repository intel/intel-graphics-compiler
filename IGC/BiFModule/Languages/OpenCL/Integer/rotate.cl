/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
char OVERLOADABLE rotate( char v,
                          char i )
{
    return SPIRV_OCL_BUILTIN(rotate, _i8_i8, )( v, i );
}

INLINE
char2 OVERLOADABLE rotate( char2 v,
                           char2 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v2i8_v2i8, )( v, i );
}

INLINE
char3 OVERLOADABLE rotate( char3 v,
                           char3 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v3i8_v3i8, )( v, i );
}

INLINE
char4 OVERLOADABLE rotate( char4 v,
                           char4 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v4i8_v4i8, )( v, i );
}

INLINE
char8 OVERLOADABLE rotate( char8 v,
                           char8 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v8i8_v8i8, )( v, i );
}

INLINE
char16 OVERLOADABLE rotate( char16 v,
                            char16 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v16i8_v16i8, )( v, i );
}

INLINE
uchar OVERLOADABLE rotate( uchar v,
                           uchar i )
{
    return as_uchar( SPIRV_OCL_BUILTIN(rotate, _i8_i8, )( as_char(v), as_char(i) ) );
}

INLINE
uchar2 OVERLOADABLE rotate( uchar2 v,
                            uchar2 i )
{
    return as_uchar2( SPIRV_OCL_BUILTIN(rotate, _v2i8_v2i8, )( as_char2(v), as_char2(i) ) );
}

INLINE
uchar3 OVERLOADABLE rotate( uchar3 v,
                            uchar3 i )
{
    return as_uchar3( SPIRV_OCL_BUILTIN(rotate, _v3i8_v3i8, )( as_char3(v), as_char3(i) ) );
}

INLINE
uchar4 OVERLOADABLE rotate( uchar4 v,
                            uchar4 i )
{
    return as_uchar4( SPIRV_OCL_BUILTIN(rotate, _v4i8_v4i8, )( as_char4(v), as_char4(i) ) );
}

INLINE
uchar8 OVERLOADABLE rotate( uchar8 v,
                            uchar8 i )
{
    return as_uchar8( SPIRV_OCL_BUILTIN(rotate, _v8i8_v8i8, )( as_char8(v), as_char8(i) ) );
}

INLINE
uchar16 OVERLOADABLE rotate( uchar16 v,
                             uchar16 i )
{
    return as_uchar16( SPIRV_OCL_BUILTIN(rotate, _v16i8_v16i8, )( as_char16(v), as_char16(i) ) );
}

INLINE
short OVERLOADABLE rotate( short v,
                           short i )
{
    return SPIRV_OCL_BUILTIN(rotate, _i16_i16, )( v, i );
}

INLINE
short2 OVERLOADABLE rotate( short2 v,
                            short2 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v2i16_v2i16, )( v, i );
}

INLINE
short3 OVERLOADABLE rotate( short3 v,
                            short3 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v3i16_v3i16, )( v, i );
}

INLINE
short4 OVERLOADABLE rotate( short4 v,
                            short4 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v4i16_v4i16, )( v, i );
}

INLINE
short8 OVERLOADABLE rotate( short8 v,
                            short8 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v8i16_v8i16, )( v, i );
}

INLINE
short16 OVERLOADABLE rotate( short16 v,
                             short16 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v16i16_v16i16, )( v, i );
}

INLINE
ushort OVERLOADABLE rotate( ushort v,
                            ushort i )
{
    return as_ushort( SPIRV_OCL_BUILTIN(rotate, _i16_i16, )( as_short(v), as_short(i) ) );
}

INLINE
ushort2 OVERLOADABLE rotate( ushort2 v,
                             ushort2 i )
{
    return as_ushort2( SPIRV_OCL_BUILTIN(rotate, _v2i16_v2i16, )( as_short2(v), as_short2(i) ) );
}

INLINE
ushort3 OVERLOADABLE rotate( ushort3 v,
                             ushort3 i )
{
    return as_ushort3( SPIRV_OCL_BUILTIN(rotate, _v3i16_v3i16, )( as_short3(v), as_short3(i) ) );
}

INLINE
ushort4 OVERLOADABLE rotate( ushort4 v,
                             ushort4 i )
{
    return as_ushort4( SPIRV_OCL_BUILTIN(rotate, _v4i16_v4i16, )( as_short4(v), as_short4(i) ) );
}

INLINE
ushort8 OVERLOADABLE rotate( ushort8 v,
                             ushort8 i )
{
    return as_ushort8( SPIRV_OCL_BUILTIN(rotate, _v8i16_v8i16, )( as_short8(v), as_short8(i) ) );
}

INLINE
ushort16 OVERLOADABLE rotate( ushort16 v,
                              ushort16 i )
{
    return as_ushort16( SPIRV_OCL_BUILTIN(rotate, _v16i16_v16i16, )( as_short16(v), as_short16(i) ) );
}

INLINE
int OVERLOADABLE rotate( int v,
                         int i )
{
    return SPIRV_OCL_BUILTIN(rotate, _i32_i32, )( v, i );
}

INLINE
int2 OVERLOADABLE rotate( int2 v,
                          int2 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v2i32_v2i32, )( v, i );
}

INLINE
int3 OVERLOADABLE rotate( int3 v,
                          int3 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v3i32_v3i32, )( v, i );
}

INLINE
int4 OVERLOADABLE rotate( int4 v,
                          int4 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v4i32_v4i32, )( v, i );
}

INLINE
int8 OVERLOADABLE rotate( int8 v,
                          int8 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v8i32_v8i32, )( v, i );
}

INLINE
int16 OVERLOADABLE rotate( int16 v,
                           int16 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v16i32_v16i32, )( v, i );
}

INLINE
uint OVERLOADABLE rotate( uint v,
                          uint i )
{
    return as_uint( SPIRV_OCL_BUILTIN(rotate, _i32_i32, )( as_int(v), as_int(i) ) );
}

INLINE
uint2 OVERLOADABLE rotate( uint2 v,
                           uint2 i )
{
    return as_uint2( SPIRV_OCL_BUILTIN(rotate, _v2i32_v2i32, )( as_int2(v), as_int2(i) ) );
}

INLINE
uint3 OVERLOADABLE rotate( uint3 v,
                           uint3 i )
{
    return as_uint3( SPIRV_OCL_BUILTIN(rotate, _v3i32_v3i32, )( as_int3(v), as_int3(i) ) );
}

INLINE
uint4 OVERLOADABLE rotate( uint4 v,
                           uint4 i )
{
    return as_uint4( SPIRV_OCL_BUILTIN(rotate, _v4i32_v4i32, )( as_int4(v), as_int4(i) ) );
}

INLINE
uint8 OVERLOADABLE rotate( uint8 v,
                           uint8 i )
{
    return as_uint8( SPIRV_OCL_BUILTIN(rotate, _v8i32_v8i32, )( as_int8(v), as_int8(i) ) );
}

INLINE
uint16 OVERLOADABLE rotate( uint16 v,
                            uint16 i )
{
    return as_uint16( SPIRV_OCL_BUILTIN(rotate, _v16i32_v16i32, )( as_int16(v), as_int16(i) ) );
}

INLINE
long OVERLOADABLE rotate( long v,
                          long i )
{
    return SPIRV_OCL_BUILTIN(rotate, _i64_i64, )( v, i );
}

INLINE
long2 OVERLOADABLE rotate( long2 v,
                           long2 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v2i64_v2i64, )( v, i );
}

INLINE
long3 OVERLOADABLE rotate( long3 v,
                           long3 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v3i64_v3i64, )( v, i );
}

INLINE
long4 OVERLOADABLE rotate( long4 v,
                           long4 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v4i64_v4i64, )( v, i );
}

INLINE
long8 OVERLOADABLE rotate( long8 v,
                           long8 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v8i64_v8i64, )( v, i );
}

INLINE
long16 OVERLOADABLE rotate( long16 v,
                            long16 i )
{
    return SPIRV_OCL_BUILTIN(rotate, _v16i64_v16i64, )( v, i );
}

INLINE
ulong OVERLOADABLE rotate( ulong v,
                           ulong i )
{
    return as_ulong( SPIRV_OCL_BUILTIN(rotate, _i64_i64, )( as_long(v), as_long(i) ) );
}

INLINE
ulong2 OVERLOADABLE rotate( ulong2 v,
                            ulong2 i )
{
    return as_ulong2( SPIRV_OCL_BUILTIN(rotate, _v2i64_v2i64, )( as_long2(v), as_long2(i) ) );
}

INLINE
ulong3 OVERLOADABLE rotate( ulong3 v,
                            ulong3 i )
{
    return as_ulong3( SPIRV_OCL_BUILTIN(rotate, _v3i64_v3i64, )( as_long3(v), as_long3(i) ) );
}

INLINE
ulong4 OVERLOADABLE rotate( ulong4 v,
                            ulong4 i )
{
    return as_ulong4( SPIRV_OCL_BUILTIN(rotate, _v4i64_v4i64, )( as_long4(v), as_long4(i) ) );
}

INLINE
ulong8 OVERLOADABLE rotate( ulong8 v,
                            ulong8 i )
{
    return as_ulong8( SPIRV_OCL_BUILTIN(rotate, _v8i64_v8i64, )( as_long8(v), as_long8(i) ) );
}

INLINE
ulong16 OVERLOADABLE rotate( ulong16 v,
                             ulong16 i )
{
    return as_ulong16( SPIRV_OCL_BUILTIN(rotate, _v16i64_v16i64, )( as_long16(v), as_long16(i) ) );
}

