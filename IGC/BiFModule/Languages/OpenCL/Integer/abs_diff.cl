/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
uchar OVERLOADABLE abs_diff( char x,
                             char y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar2 OVERLOADABLE abs_diff( char2 x,
                              char2 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar3 OVERLOADABLE abs_diff( char3 x,
                              char3 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar4 OVERLOADABLE abs_diff( char4 x,
                              char4 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar8 OVERLOADABLE abs_diff( char8 x,
                              char8 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar16 OVERLOADABLE abs_diff( char16 x,
                               char16 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uchar OVERLOADABLE abs_diff( uchar x,
                             uchar y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uchar2 OVERLOADABLE abs_diff( uchar2 x,
                              uchar2 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uchar3 OVERLOADABLE abs_diff( uchar3 x,
                              uchar3 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uchar4 OVERLOADABLE abs_diff( uchar4 x,
                              uchar4 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uchar8 OVERLOADABLE abs_diff( uchar8 x,
                              uchar8 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uchar16 OVERLOADABLE abs_diff( uchar16 x,
                               uchar16 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort OVERLOADABLE abs_diff( short x,
                              short y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort2 OVERLOADABLE abs_diff( short2 x,
                               short2 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort3 OVERLOADABLE abs_diff( short3 x,
                               short3 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort4 OVERLOADABLE abs_diff( short4 x,
                               short4 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort8 OVERLOADABLE abs_diff( short8 x,
                               short8 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort16 OVERLOADABLE abs_diff( short16 x,
                                short16 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ushort OVERLOADABLE abs_diff( ushort x,
                              ushort y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort2 OVERLOADABLE abs_diff( ushort2 x,
                               ushort2 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort3 OVERLOADABLE abs_diff( ushort3 x,
                               ushort3 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort4 OVERLOADABLE abs_diff( ushort4 x,
                               ushort4 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort8 OVERLOADABLE abs_diff( ushort8 x,
                               ushort8 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ushort16 OVERLOADABLE abs_diff( ushort16 x,
                                ushort16 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint OVERLOADABLE abs_diff( int x,
                            int y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint2 OVERLOADABLE abs_diff( int2 x,
                             int2 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint3 OVERLOADABLE abs_diff( int3 x,
                             int3 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint4 OVERLOADABLE abs_diff( int4 x,
                             int4 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint8 OVERLOADABLE abs_diff( int8 x,
                             int8 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint16 OVERLOADABLE abs_diff( int16 x,
                              int16 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
uint OVERLOADABLE abs_diff( uint x,
                            uint y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint2 OVERLOADABLE abs_diff( uint2 x,
                             uint2 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint3 OVERLOADABLE abs_diff( uint3 x,
                             uint3 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint4 OVERLOADABLE abs_diff( uint4 x,
                             uint4 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint8 OVERLOADABLE abs_diff( uint8 x,
                             uint8 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
uint16 OVERLOADABLE abs_diff( uint16 x,
                              uint16 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong OVERLOADABLE abs_diff( long x,
                             long y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong2 OVERLOADABLE abs_diff( long2 x,
                              long2 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong3 OVERLOADABLE abs_diff( long3 x,
                              long3 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong4 OVERLOADABLE abs_diff( long4 x,
                              long4 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong8 OVERLOADABLE abs_diff( long8 x,
                              long8 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong16 OVERLOADABLE abs_diff( long16 x,
                               long16 y )
{
    return __spirv_ocl_s_abs_diff( x, y );
}

INLINE
ulong OVERLOADABLE abs_diff( ulong x,
                             ulong y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong2 OVERLOADABLE abs_diff( ulong2 x,
                              ulong2 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong3 OVERLOADABLE abs_diff( ulong3 x,
                              ulong3 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong4 OVERLOADABLE abs_diff( ulong4 x,
                              ulong4 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong8 OVERLOADABLE abs_diff( ulong8 x,
                              ulong8 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

INLINE
ulong16 OVERLOADABLE abs_diff( ulong16 x,
                               ulong16 y )
{
    return __spirv_ocl_u_abs_diff( x, y );
}

