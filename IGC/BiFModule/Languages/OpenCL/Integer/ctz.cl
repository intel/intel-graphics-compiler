/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
char OVERLOADABLE ctz( char x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
char2 OVERLOADABLE ctz( char2 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
char3 OVERLOADABLE ctz( char3 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
char4 OVERLOADABLE ctz( char4 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
char8 OVERLOADABLE ctz( char8 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
char16 OVERLOADABLE ctz( char16 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
uchar2 OVERLOADABLE ctz( uchar2 x )
{
    return as_uchar2( __spirv_ocl_ctz( as_char2(x) ) );
}

INLINE
uchar3 OVERLOADABLE ctz( uchar3 x )
{
    return as_uchar3( __spirv_ocl_ctz( as_char3(x) ) );
}

INLINE
uchar4 OVERLOADABLE ctz( uchar4 x )
{
    return as_uchar4( __spirv_ocl_ctz( as_char4(x) ) );
}

INLINE
uchar8 OVERLOADABLE ctz( uchar8 x )
{
    return as_uchar8( __spirv_ocl_ctz( as_char8(x) ) );
}

INLINE
uchar16 OVERLOADABLE ctz( uchar16 x )
{
    return as_uchar16( __spirv_ocl_ctz( as_char16(x) ) );
}

INLINE
short OVERLOADABLE ctz( short x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
short2 OVERLOADABLE ctz( short2 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
short3 OVERLOADABLE ctz( short3 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
short4 OVERLOADABLE ctz( short4 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
short8 OVERLOADABLE ctz( short8 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
short16 OVERLOADABLE ctz( short16 x )
{
    return __spirv_ocl_ctz( x );
}



INLINE
ushort2 OVERLOADABLE ctz( ushort2 x )
{
    return as_ushort2( __spirv_ocl_ctz( as_short2(x) ) );
}

INLINE
ushort3 OVERLOADABLE ctz( ushort3 x )
{
    return as_ushort3( __spirv_ocl_ctz( as_short3(x) ) );
}

INLINE
ushort4 OVERLOADABLE ctz( ushort4 x )
{
    return as_ushort4( __spirv_ocl_ctz( as_short4(x) ) );
}

INLINE
ushort8 OVERLOADABLE ctz( ushort8 x )
{
    return as_ushort8( __spirv_ocl_ctz( as_short8(x) ) );
}

INLINE
ushort16 OVERLOADABLE ctz( ushort16 x )
{
    return as_ushort16( __spirv_ocl_ctz( as_short16(x) ) );
}

INLINE
int OVERLOADABLE ctz( int x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
int2 OVERLOADABLE ctz( int2 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
int3 OVERLOADABLE ctz( int3 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
int4 OVERLOADABLE ctz( int4 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
int8 OVERLOADABLE ctz( int8 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
int16 OVERLOADABLE ctz( int16 x )
{
    return __spirv_ocl_ctz( x );
}



INLINE
uint2 OVERLOADABLE ctz( uint2 x )
{
    return as_uint2( __spirv_ocl_ctz( as_int2(x) ) );
}

INLINE
uint3 OVERLOADABLE ctz( uint3 x )
{
    return as_uint3( __spirv_ocl_ctz( as_int3(x) ) );
}

INLINE
uint4 OVERLOADABLE ctz( uint4 x )
{
    return as_uint4( __spirv_ocl_ctz( as_int4(x) ) );
}

INLINE
uint8 OVERLOADABLE ctz( uint8 x )
{
    return as_uint8( __spirv_ocl_ctz( as_int8(x) ) );
}

INLINE
uint16 OVERLOADABLE ctz( uint16 x )
{
    return as_uint16( __spirv_ocl_ctz( as_int16(x) ) );
}

INLINE
long OVERLOADABLE ctz( long x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
long2 OVERLOADABLE ctz( long2 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
long3 OVERLOADABLE ctz( long3 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
long4 OVERLOADABLE ctz( long4 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
long8 OVERLOADABLE ctz( long8 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
long16 OVERLOADABLE ctz( long16 x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
ulong OVERLOADABLE ctz( ulong x )
{
    return as_ulong( __spirv_ocl_ctz( as_long(x) ) );
}

INLINE
ulong2 OVERLOADABLE ctz( ulong2 x )
{
    return as_ulong2( __spirv_ocl_ctz( as_long2(x) ) );
}

INLINE
ulong3 OVERLOADABLE ctz( ulong3 x )
{
    return as_ulong3( __spirv_ocl_ctz( as_long3(x) ) );
}

INLINE
ulong4 OVERLOADABLE ctz( ulong4 x )
{
    return as_ulong4( __spirv_ocl_ctz( as_long4(x) ) );
}

INLINE
ulong8 OVERLOADABLE ctz( ulong8 x )
{
    return as_ulong8( __spirv_ocl_ctz( as_long8(x) ) );
}

INLINE
ulong16 OVERLOADABLE ctz( ulong16 x )
{
    return as_ulong16( __spirv_ocl_ctz( as_long16(x) ) );
}

