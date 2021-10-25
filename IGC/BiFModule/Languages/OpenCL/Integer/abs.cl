/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
uchar2 OVERLOADABLE abs( char2 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v2i8, )( x );
}

INLINE
uchar3 OVERLOADABLE abs( char3 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v3i8, )( x );
}

INLINE
uchar4 OVERLOADABLE abs( char4 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v4i8, )( x );
}

INLINE
uchar8 OVERLOADABLE abs( char8 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v8i8, )( x );
}

INLINE
uchar16 OVERLOADABLE abs( char16 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v16i8, )( x );
}

INLINE
uchar OVERLOADABLE abs( uchar x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _i8, )( x );
}

INLINE
uchar2 OVERLOADABLE abs( uchar2 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v2i8, )( x );
}

INLINE
uchar3 OVERLOADABLE abs( uchar3 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v3i8, )( x );
}

INLINE
uchar4 OVERLOADABLE abs( uchar4 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v4i8, )( x );
}

INLINE
uchar8 OVERLOADABLE abs( uchar8 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v8i8, )( x );
}

INLINE
uchar16 OVERLOADABLE abs( uchar16 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v16i8, )( x );
}



INLINE
ushort2 OVERLOADABLE abs( short2 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v2i16, )( x );
}

INLINE
ushort3 OVERLOADABLE abs( short3 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v3i16, )( x );
}

INLINE
ushort4 OVERLOADABLE abs( short4 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v4i16, )( x );
}

INLINE
ushort8 OVERLOADABLE abs( short8 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v8i16, )( x );
}

INLINE
ushort16 OVERLOADABLE abs( short16 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v16i16, )( x );
}

INLINE
ushort OVERLOADABLE abs( ushort x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _i16, )( x );
}

INLINE
ushort2 OVERLOADABLE abs( ushort2 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v2i16, )( x );
}

INLINE
ushort3 OVERLOADABLE abs( ushort3 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v3i16, )( x );
}

INLINE
ushort4 OVERLOADABLE abs( ushort4 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v4i16, )( x );
}

INLINE
ushort8 OVERLOADABLE abs( ushort8 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v8i16, )( x );
}

INLINE
ushort16 OVERLOADABLE abs( ushort16 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v16i16, )( x );
}



INLINE
uint2 OVERLOADABLE abs( int2 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v2i32, )( x );
}

INLINE
uint3 OVERLOADABLE abs( int3 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v3i32, )( x );
}

INLINE
uint4 OVERLOADABLE abs( int4 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v4i32, )( x );
}

INLINE
uint8 OVERLOADABLE abs( int8 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v8i32, )( x );
}

INLINE
uint16 OVERLOADABLE abs( int16 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v16i32, )( x );
}

INLINE
uint OVERLOADABLE abs( uint x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _i32, )( x );
}

INLINE
uint2 OVERLOADABLE abs( uint2 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v2i32, )( x );
}

INLINE
uint3 OVERLOADABLE abs( uint3 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v3i32, )( x );
}

INLINE
uint4 OVERLOADABLE abs( uint4 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v4i32, )( x );
}

INLINE
uint8 OVERLOADABLE abs( uint8 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v8i32, )( x );
}

INLINE
uint16 OVERLOADABLE abs( uint16 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v16i32, )( x );
}

INLINE
ulong OVERLOADABLE abs( long x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _i64, )( x );
}

INLINE
ulong2 OVERLOADABLE abs( long2 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v2i64, )( x );
}

INLINE
ulong3 OVERLOADABLE abs( long3 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v3i64, )( x );
}

INLINE
ulong4 OVERLOADABLE abs( long4 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v4i64, )( x );
}

INLINE
ulong8 OVERLOADABLE abs( long8 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v8i64, )( x );
}

INLINE
ulong16 OVERLOADABLE abs( long16 x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _v16i64, )( x );
}

INLINE
ulong OVERLOADABLE abs( ulong x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _i64, )( x );
}

INLINE
ulong2 OVERLOADABLE abs( ulong2 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v2i64, )( x );
}

INLINE
ulong3 OVERLOADABLE abs( ulong3 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v3i64, )( x );
}

INLINE
ulong4 OVERLOADABLE abs( ulong4 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v4i64, )( x );
}

INLINE
ulong8 OVERLOADABLE abs( ulong8 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v8i64, )( x );
}

INLINE
ulong16 OVERLOADABLE abs( ulong16 x )
{
    return SPIRV_OCL_BUILTIN(u_abs, _v16i64, )( x );
}

