/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE
uchar OVERLOADABLE clz( uchar x )
{
    return as_uchar( SPIRV_OCL_BUILTIN(clz, _i8, )( as_char(x) ) );
}

INLINE
ushort OVERLOADABLE clz( ushort x )
{
    return as_ushort( SPIRV_OCL_BUILTIN(clz, _i16, )( as_short(x) ) );
}

INLINE
uint OVERLOADABLE clz( uint x )
{
    return as_uint( SPIRV_OCL_BUILTIN(clz, _i32, )( as_int(x) ) );
}

INLINE
char OVERLOADABLE clz( char x )
{
    return SPIRV_OCL_BUILTIN(clz, _i8, )( x );
}

INLINE
char2 OVERLOADABLE clz( char2 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v2i8, )( x );
}

INLINE
char3 OVERLOADABLE clz( char3 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v3i8, )( x );
}

INLINE
char4 OVERLOADABLE clz( char4 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v4i8, )( x );
}

INLINE
char8 OVERLOADABLE clz( char8 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v8i8, )( x );
}

INLINE
char16 OVERLOADABLE clz( char16 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v16i8, )( x );
}



INLINE
uchar2 OVERLOADABLE clz( uchar2 x )
{
    return as_uchar2( SPIRV_OCL_BUILTIN(clz, _v2i8, )( as_char2(x) ) );
}

INLINE
uchar3 OVERLOADABLE clz( uchar3 x )
{
    return as_uchar3( SPIRV_OCL_BUILTIN(clz, _v3i8, )( as_char3(x) ) );
}

INLINE
uchar4 OVERLOADABLE clz( uchar4 x )
{
    return as_uchar4( SPIRV_OCL_BUILTIN(clz, _v4i8, )( as_char4(x) ) );
}

INLINE
uchar8 OVERLOADABLE clz( uchar8 x )
{
    return as_uchar8( SPIRV_OCL_BUILTIN(clz, _v8i8, )( as_char8(x) ) );
}

INLINE
uchar16 OVERLOADABLE clz( uchar16 x )
{
    return as_uchar16( SPIRV_OCL_BUILTIN(clz, _v16i8, )( as_char16(x) ) );
}

INLINE
short OVERLOADABLE clz( short x )
{
    return SPIRV_OCL_BUILTIN(clz, _i16, )( x );
}

INLINE
short2 OVERLOADABLE clz( short2 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v2i16, )( x );
}

INLINE
short3 OVERLOADABLE clz( short3 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v3i16, )( x );
}

INLINE
short4 OVERLOADABLE clz( short4 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v4i16, )( x );
}

INLINE
short8 OVERLOADABLE clz( short8 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v8i16, )( x );
}

INLINE
short16 OVERLOADABLE clz( short16 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v16i16, )( x );
}



INLINE
ushort2 OVERLOADABLE clz( ushort2 x )
{
    return as_ushort2( SPIRV_OCL_BUILTIN(clz, _v2i16, )( as_short2(x) ) );
}

INLINE
ushort3 OVERLOADABLE clz( ushort3 x )
{
    return as_ushort3( SPIRV_OCL_BUILTIN(clz, _v3i16, )( as_short3(x) ) );
}

INLINE
ushort4 OVERLOADABLE clz( ushort4 x )
{
    return as_ushort4( SPIRV_OCL_BUILTIN(clz, _v4i16, )( as_short4(x) ) );
}

INLINE
ushort8 OVERLOADABLE clz( ushort8 x )
{
    return as_ushort8( SPIRV_OCL_BUILTIN(clz, _v8i16, )( as_short8(x) ) );
}

INLINE
ushort16 OVERLOADABLE clz( ushort16 x )
{
    return as_ushort16( SPIRV_OCL_BUILTIN(clz, _v16i16, )( as_short16(x) ) );
}

INLINE
int OVERLOADABLE clz( int x )
{
    return SPIRV_OCL_BUILTIN(clz, _i32, )( x );
}

INLINE
int2 OVERLOADABLE clz( int2 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v2i32, )( x );
}

INLINE
int3 OVERLOADABLE clz( int3 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v3i32, )( x );
}

INLINE
int4 OVERLOADABLE clz( int4 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v4i32, )( x );
}

INLINE
int8 OVERLOADABLE clz( int8 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v8i32, )( x );
}

INLINE
int16 OVERLOADABLE clz( int16 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v16i32, )( x );
}



INLINE
uint2 OVERLOADABLE clz( uint2 x )
{
    return as_uint2( SPIRV_OCL_BUILTIN(clz, _v2i32, )( as_int2(x) ) );
}

INLINE
uint3 OVERLOADABLE clz( uint3 x )
{
    return as_uint3( SPIRV_OCL_BUILTIN(clz, _v3i32, )( as_int3(x) ) );
}

INLINE
uint4 OVERLOADABLE clz( uint4 x )
{
    return as_uint4( SPIRV_OCL_BUILTIN(clz, _v4i32, )( as_int4(x) ) );
}

INLINE
uint8 OVERLOADABLE clz( uint8 x )
{
    return as_uint8( SPIRV_OCL_BUILTIN(clz, _v8i32, )( as_int8(x) ) );
}

INLINE
uint16 OVERLOADABLE clz( uint16 x )
{
    return as_uint16( SPIRV_OCL_BUILTIN(clz, _v16i32, )( as_int16(x) ) );
}

INLINE
long OVERLOADABLE clz( long x )
{
    return SPIRV_OCL_BUILTIN(clz, _i64, )( x );
}

INLINE
long2 OVERLOADABLE clz( long2 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v2i64, )( x );
}

INLINE
long3 OVERLOADABLE clz( long3 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v3i64, )( x );
}

INLINE
long4 OVERLOADABLE clz( long4 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v4i64, )( x );
}

INLINE
long8 OVERLOADABLE clz( long8 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v8i64, )( x );
}

INLINE
long16 OVERLOADABLE clz( long16 x )
{
    return SPIRV_OCL_BUILTIN(clz, _v16i64, )( x );
}

INLINE
ulong OVERLOADABLE clz( ulong x )
{
    return as_ulong( SPIRV_OCL_BUILTIN(clz, _i64, )( as_long(x) ) );
}

INLINE
ulong2 OVERLOADABLE clz( ulong2 x )
{
    return as_ulong2( SPIRV_OCL_BUILTIN(clz, _v2i64, )( as_long2(x) ) );
}

INLINE
ulong3 OVERLOADABLE clz( ulong3 x )
{
    return as_ulong3( SPIRV_OCL_BUILTIN(clz, _v3i64, )( as_long3(x) ) );
}

INLINE
ulong4 OVERLOADABLE clz( ulong4 x )
{
    return as_ulong4( SPIRV_OCL_BUILTIN(clz, _v4i64, )( as_long4(x) ) );
}

INLINE
ulong8 OVERLOADABLE clz( ulong8 x )
{
    return as_ulong8( SPIRV_OCL_BUILTIN(clz, _v8i64, )( as_long8(x) ) );
}

INLINE
ulong16 OVERLOADABLE clz( ulong16 x )
{
    return as_ulong16( SPIRV_OCL_BUILTIN(clz, _v16i64, )( as_long16(x) ) );
}

