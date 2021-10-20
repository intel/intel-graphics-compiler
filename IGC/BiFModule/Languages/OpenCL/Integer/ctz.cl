/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
char OVERLOADABLE ctz( char x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i8, )( x );
}

INLINE
char2 OVERLOADABLE ctz( char2 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v2i8, )( x );
}

INLINE
char3 OVERLOADABLE ctz( char3 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v3i8, )( x );
}

INLINE
char4 OVERLOADABLE ctz( char4 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v4i8, )( x );
}

INLINE
char8 OVERLOADABLE ctz( char8 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v8i8, )( x );
}

INLINE
char16 OVERLOADABLE ctz( char16 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v16i8, )( x );
}

INLINE
uchar2 OVERLOADABLE ctz( uchar2 x )
{
    return as_uchar2( SPIRV_OCL_BUILTIN(ctz, _v2i8, )( as_char2(x) ) );
}

INLINE
uchar3 OVERLOADABLE ctz( uchar3 x )
{
    return as_uchar3( SPIRV_OCL_BUILTIN(ctz, _v3i8, )( as_char3(x) ) );
}

INLINE
uchar4 OVERLOADABLE ctz( uchar4 x )
{
    return as_uchar4( SPIRV_OCL_BUILTIN(ctz, _v4i8, )( as_char4(x) ) );
}

INLINE
uchar8 OVERLOADABLE ctz( uchar8 x )
{
    return as_uchar8( SPIRV_OCL_BUILTIN(ctz, _v8i8, )( as_char8(x) ) );
}

INLINE
uchar16 OVERLOADABLE ctz( uchar16 x )
{
    return as_uchar16( SPIRV_OCL_BUILTIN(ctz, _v16i8, )( as_char16(x) ) );
}

INLINE
short OVERLOADABLE ctz( short x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i16, )( x );
}

INLINE
short2 OVERLOADABLE ctz( short2 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v2i16, )( x );
}

INLINE
short3 OVERLOADABLE ctz( short3 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v3i16, )( x );
}

INLINE
short4 OVERLOADABLE ctz( short4 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v4i16, )( x );
}

INLINE
short8 OVERLOADABLE ctz( short8 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v8i16, )( x );
}

INLINE
short16 OVERLOADABLE ctz( short16 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v16i16, )( x );
}



INLINE
ushort2 OVERLOADABLE ctz( ushort2 x )
{
    return as_ushort2( SPIRV_OCL_BUILTIN(ctz, _v2i16, )( as_short2(x) ) );
}

INLINE
ushort3 OVERLOADABLE ctz( ushort3 x )
{
    return as_ushort3( SPIRV_OCL_BUILTIN(ctz, _v3i16, )( as_short3(x) ) );
}

INLINE
ushort4 OVERLOADABLE ctz( ushort4 x )
{
    return as_ushort4( SPIRV_OCL_BUILTIN(ctz, _v4i16, )( as_short4(x) ) );
}

INLINE
ushort8 OVERLOADABLE ctz( ushort8 x )
{
    return as_ushort8( SPIRV_OCL_BUILTIN(ctz, _v8i16, )( as_short8(x) ) );
}

INLINE
ushort16 OVERLOADABLE ctz( ushort16 x )
{
    return as_ushort16( SPIRV_OCL_BUILTIN(ctz, _v16i16, )( as_short16(x) ) );
}

INLINE
int OVERLOADABLE ctz( int x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i32, )( x );
}

INLINE
int2 OVERLOADABLE ctz( int2 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v2i32, )( x );
}

INLINE
int3 OVERLOADABLE ctz( int3 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v3i32, )( x );
}

INLINE
int4 OVERLOADABLE ctz( int4 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v4i32, )( x );
}

INLINE
int8 OVERLOADABLE ctz( int8 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v8i32, )( x );
}

INLINE
int16 OVERLOADABLE ctz( int16 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v16i32, )( x );
}



INLINE
uint2 OVERLOADABLE ctz( uint2 x )
{
    return as_uint2( SPIRV_OCL_BUILTIN(ctz, _v2i32, )( as_int2(x) ) );
}

INLINE
uint3 OVERLOADABLE ctz( uint3 x )
{
    return as_uint3( SPIRV_OCL_BUILTIN(ctz, _v3i32, )( as_int3(x) ) );
}

INLINE
uint4 OVERLOADABLE ctz( uint4 x )
{
    return as_uint4( SPIRV_OCL_BUILTIN(ctz, _v4i32, )( as_int4(x) ) );
}

INLINE
uint8 OVERLOADABLE ctz( uint8 x )
{
    return as_uint8( SPIRV_OCL_BUILTIN(ctz, _v8i32, )( as_int8(x) ) );
}

INLINE
uint16 OVERLOADABLE ctz( uint16 x )
{
    return as_uint16( SPIRV_OCL_BUILTIN(ctz, _v16i32, )( as_int16(x) ) );
}

INLINE
long OVERLOADABLE ctz( long x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i64, )( x );
}

INLINE
long2 OVERLOADABLE ctz( long2 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v2i64, )( x );
}

INLINE
long3 OVERLOADABLE ctz( long3 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v3i64, )( x );
}

INLINE
long4 OVERLOADABLE ctz( long4 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v4i64, )( x );
}

INLINE
long8 OVERLOADABLE ctz( long8 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v8i64, )( x );
}

INLINE
long16 OVERLOADABLE ctz( long16 x )
{
    return SPIRV_OCL_BUILTIN(ctz, _v16i64, )( x );
}

INLINE
ulong OVERLOADABLE ctz( ulong x )
{
    return as_ulong( SPIRV_OCL_BUILTIN(ctz, _i64, )( as_long(x) ) );
}

INLINE
ulong2 OVERLOADABLE ctz( ulong2 x )
{
    return as_ulong2( SPIRV_OCL_BUILTIN(ctz, _v2i64, )( as_long2(x) ) );
}

INLINE
ulong3 OVERLOADABLE ctz( ulong3 x )
{
    return as_ulong3( SPIRV_OCL_BUILTIN(ctz, _v3i64, )( as_long3(x) ) );
}

INLINE
ulong4 OVERLOADABLE ctz( ulong4 x )
{
    return as_ulong4( SPIRV_OCL_BUILTIN(ctz, _v4i64, )( as_long4(x) ) );
}

INLINE
ulong8 OVERLOADABLE ctz( ulong8 x )
{
    return as_ulong8( SPIRV_OCL_BUILTIN(ctz, _v8i64, )( as_long8(x) ) );
}

INLINE
ulong16 OVERLOADABLE ctz( ulong16 x )
{
    return as_ulong16( SPIRV_OCL_BUILTIN(ctz, _v16i64, )( as_long16(x) ) );
}

