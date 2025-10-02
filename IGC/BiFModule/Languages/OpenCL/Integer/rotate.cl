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
    return __spirv_ocl_rotate( v, i );
}

INLINE
char2 OVERLOADABLE rotate( char2 v,
                           char2 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
char3 OVERLOADABLE rotate( char3 v,
                           char3 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
char4 OVERLOADABLE rotate( char4 v,
                           char4 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
char8 OVERLOADABLE rotate( char8 v,
                           char8 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
char16 OVERLOADABLE rotate( char16 v,
                            char16 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
uchar OVERLOADABLE rotate( uchar v,
                           uchar i )
{
    return as_uchar( __spirv_ocl_rotate( as_char(v), as_char(i) ) );
}

INLINE
uchar2 OVERLOADABLE rotate( uchar2 v,
                            uchar2 i )
{
    return as_uchar2( __spirv_ocl_rotate( as_char2(v), as_char2(i) ) );
}

INLINE
uchar3 OVERLOADABLE rotate( uchar3 v,
                            uchar3 i )
{
    return as_uchar3( __spirv_ocl_rotate( as_char3(v), as_char3(i) ) );
}

INLINE
uchar4 OVERLOADABLE rotate( uchar4 v,
                            uchar4 i )
{
    return as_uchar4( __spirv_ocl_rotate( as_char4(v), as_char4(i) ) );
}

INLINE
uchar8 OVERLOADABLE rotate( uchar8 v,
                            uchar8 i )
{
    return as_uchar8( __spirv_ocl_rotate( as_char8(v), as_char8(i) ) );
}

INLINE
uchar16 OVERLOADABLE rotate( uchar16 v,
                             uchar16 i )
{
    return as_uchar16( __spirv_ocl_rotate( as_char16(v), as_char16(i) ) );
}

INLINE
short OVERLOADABLE rotate( short v,
                           short i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
short2 OVERLOADABLE rotate( short2 v,
                            short2 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
short3 OVERLOADABLE rotate( short3 v,
                            short3 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
short4 OVERLOADABLE rotate( short4 v,
                            short4 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
short8 OVERLOADABLE rotate( short8 v,
                            short8 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
short16 OVERLOADABLE rotate( short16 v,
                             short16 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
ushort OVERLOADABLE rotate( ushort v,
                            ushort i )
{
    return as_ushort( __spirv_ocl_rotate( as_short(v), as_short(i) ) );
}

INLINE
ushort2 OVERLOADABLE rotate( ushort2 v,
                             ushort2 i )
{
    return as_ushort2( __spirv_ocl_rotate( as_short2(v), as_short2(i) ) );
}

INLINE
ushort3 OVERLOADABLE rotate( ushort3 v,
                             ushort3 i )
{
    return as_ushort3( __spirv_ocl_rotate( as_short3(v), as_short3(i) ) );
}

INLINE
ushort4 OVERLOADABLE rotate( ushort4 v,
                             ushort4 i )
{
    return as_ushort4( __spirv_ocl_rotate( as_short4(v), as_short4(i) ) );
}

INLINE
ushort8 OVERLOADABLE rotate( ushort8 v,
                             ushort8 i )
{
    return as_ushort8( __spirv_ocl_rotate( as_short8(v), as_short8(i) ) );
}

INLINE
ushort16 OVERLOADABLE rotate( ushort16 v,
                              ushort16 i )
{
    return as_ushort16( __spirv_ocl_rotate( as_short16(v), as_short16(i) ) );
}

INLINE
int OVERLOADABLE rotate( int v,
                         int i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
int2 OVERLOADABLE rotate( int2 v,
                          int2 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
int3 OVERLOADABLE rotate( int3 v,
                          int3 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
int4 OVERLOADABLE rotate( int4 v,
                          int4 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
int8 OVERLOADABLE rotate( int8 v,
                          int8 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
int16 OVERLOADABLE rotate( int16 v,
                           int16 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
uint OVERLOADABLE rotate( uint v,
                          uint i )
{
    return as_uint( __spirv_ocl_rotate( as_int(v), as_int(i) ) );
}

INLINE
uint2 OVERLOADABLE rotate( uint2 v,
                           uint2 i )
{
    return as_uint2( __spirv_ocl_rotate( as_int2(v), as_int2(i) ) );
}

INLINE
uint3 OVERLOADABLE rotate( uint3 v,
                           uint3 i )
{
    return as_uint3( __spirv_ocl_rotate( as_int3(v), as_int3(i) ) );
}

INLINE
uint4 OVERLOADABLE rotate( uint4 v,
                           uint4 i )
{
    return as_uint4( __spirv_ocl_rotate( as_int4(v), as_int4(i) ) );
}

INLINE
uint8 OVERLOADABLE rotate( uint8 v,
                           uint8 i )
{
    return as_uint8( __spirv_ocl_rotate( as_int8(v), as_int8(i) ) );
}

INLINE
uint16 OVERLOADABLE rotate( uint16 v,
                            uint16 i )
{
    return as_uint16( __spirv_ocl_rotate( as_int16(v), as_int16(i) ) );
}

INLINE
long OVERLOADABLE rotate( long v,
                          long i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
long2 OVERLOADABLE rotate( long2 v,
                           long2 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
long3 OVERLOADABLE rotate( long3 v,
                           long3 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
long4 OVERLOADABLE rotate( long4 v,
                           long4 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
long8 OVERLOADABLE rotate( long8 v,
                           long8 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
long16 OVERLOADABLE rotate( long16 v,
                            long16 i )
{
    return __spirv_ocl_rotate( v, i );
}

INLINE
ulong OVERLOADABLE rotate( ulong v,
                           ulong i )
{
    return as_ulong( __spirv_ocl_rotate( as_long(v), as_long(i) ) );
}

INLINE
ulong2 OVERLOADABLE rotate( ulong2 v,
                            ulong2 i )
{
    return as_ulong2( __spirv_ocl_rotate( as_long2(v), as_long2(i) ) );
}

INLINE
ulong3 OVERLOADABLE rotate( ulong3 v,
                            ulong3 i )
{
    return as_ulong3( __spirv_ocl_rotate( as_long3(v), as_long3(i) ) );
}

INLINE
ulong4 OVERLOADABLE rotate( ulong4 v,
                            ulong4 i )
{
    return as_ulong4( __spirv_ocl_rotate( as_long4(v), as_long4(i) ) );
}

INLINE
ulong8 OVERLOADABLE rotate( ulong8 v,
                            ulong8 i )
{
    return as_ulong8( __spirv_ocl_rotate( as_long8(v), as_long8(i) ) );
}

INLINE
ulong16 OVERLOADABLE rotate( ulong16 v,
                             ulong16 i )
{
    return as_ulong16( __spirv_ocl_rotate( as_long16(v), as_long16(i) ) );
}

