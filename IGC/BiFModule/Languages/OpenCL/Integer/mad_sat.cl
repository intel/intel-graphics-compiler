/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE
char2 OVERLOADABLE mad_sat( char2 a,
                            char2 b,
                            char2 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v2i8_v2i8_v2i8, )( a, b, c );
}

INLINE
char3 OVERLOADABLE mad_sat( char3 a,
                            char3 b,
                            char3 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v3i8_v3i8_v3i8, )( a, b, c );
}

INLINE
char4 OVERLOADABLE mad_sat( char4 a,
                            char4 b,
                            char4 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v4i8_v4i8_v4i8, )( a, b, c );
}

INLINE
char8 OVERLOADABLE mad_sat( char8 a,
                            char8 b,
                            char8 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v8i8_v8i8_v8i8, )( a, b, c );
}

INLINE
char16 OVERLOADABLE mad_sat( char16 a,
                             char16 b,
                             char16 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v16i8_v16i8_v16i8, )( a, b, c );
}


INLINE
uchar2 OVERLOADABLE mad_sat( uchar2 a,
                             uchar2 b,
                             uchar2 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v2i8_v2i8_v2i8, )( a, b, c );
}

INLINE
uchar3 OVERLOADABLE mad_sat( uchar3 a,
                             uchar3 b,
                             uchar3 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v3i8_v3i8_v3i8, )( a, b, c );
}

INLINE
uchar4 OVERLOADABLE mad_sat( uchar4 a,
                             uchar4 b,
                             uchar4 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v4i8_v4i8_v4i8, )( a, b, c );
}

INLINE
uchar8 OVERLOADABLE mad_sat( uchar8 a,
                             uchar8 b,
                             uchar8 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v8i8_v8i8_v8i8, )( a, b, c );
}

INLINE
uchar16 OVERLOADABLE mad_sat( uchar16 a,
                              uchar16 b,
                              uchar16 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v16i8_v16i8_v16i8, )( a, b, c );
}



INLINE
short2 OVERLOADABLE mad_sat( short2 a,
                             short2 b,
                             short2 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v2i16_v2i16_v2i16, )( a, b, c );
}

INLINE
short3 OVERLOADABLE mad_sat( short3 a,
                             short3 b,
                             short3 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v3i16_v3i16_v3i16, )( a, b, c );
}

INLINE
short4 OVERLOADABLE mad_sat( short4 a,
                             short4 b,
                             short4 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v4i16_v4i16_v4i16, )( a, b, c );
}

INLINE
short8 OVERLOADABLE mad_sat( short8 a,
                             short8 b,
                             short8 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v8i16_v8i16_v8i16, )( a, b, c );
}

INLINE
short16 OVERLOADABLE mad_sat( short16 a,
                              short16 b,
                              short16 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v16i16_v16i16_v16i16, )( a, b, c );
}



INLINE
ushort2 OVERLOADABLE mad_sat( ushort2 a,
                              ushort2 b,
                              ushort2 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v2i16_v2i16_v2i16, )( a, b, c );
}

INLINE
ushort3 OVERLOADABLE mad_sat( ushort3 a,
                              ushort3 b,
                              ushort3 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v3i16_v3i16_v3i16, )( a, b, c );
}

INLINE
ushort4 OVERLOADABLE mad_sat( ushort4 a,
                              ushort4 b,
                              ushort4 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v4i16_v4i16_v4i16, )( a, b, c );
}

INLINE
ushort8 OVERLOADABLE mad_sat( ushort8 a,
                              ushort8 b,
                              ushort8 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v8i16_v8i16_v8i16, )( a, b, c );
}

INLINE
ushort16 OVERLOADABLE mad_sat( ushort16 a,
                               ushort16 b,
                               ushort16 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v16i16_v16i16_v16i16, )( a, b, c );
}

INLINE
int2 OVERLOADABLE mad_sat( int2 a,
                           int2 b,
                           int2 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v2i32_v2i32_v2i32, )( a, b, c );
}

INLINE
int3 OVERLOADABLE mad_sat( int3 a,
                           int3 b,
                           int3 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v3i32_v3i32_v3i32, )( a, b, c );
}

INLINE
int4 OVERLOADABLE mad_sat( int4 a,
                           int4 b,
                           int4 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v4i32_v4i32_v4i32, )( a, b, c );
}

INLINE
int8 OVERLOADABLE mad_sat( int8 a,
                           int8 b,
                           int8 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v8i32_v8i32_v8i32, )( a, b, c );
}

INLINE
int16 OVERLOADABLE mad_sat( int16 a,
                            int16 b,
                            int16 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v16i32_v16i32_v16i32, )( a, b, c );
}



INLINE
uint2 OVERLOADABLE mad_sat( uint2 a,
                            uint2 b,
                            uint2 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v2i32_v2i32_v2i32, )( a, b, c );
}

INLINE
uint3 OVERLOADABLE mad_sat( uint3 a,
                            uint3 b,
                            uint3 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v3i32_v3i32_v3i32, )( a, b, c );
}

INLINE
uint4 OVERLOADABLE mad_sat( uint4 a,
                            uint4 b,
                            uint4 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v4i32_v4i32_v4i32, )( a, b, c );
}

INLINE
uint8 OVERLOADABLE mad_sat( uint8 a,
                            uint8 b,
                            uint8 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v8i32_v8i32_v8i32, )( a, b, c );
}

INLINE
uint16 OVERLOADABLE mad_sat( uint16 a,
                             uint16 b,
                             uint16 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v16i32_v16i32_v16i32, )( a, b, c );
}

INLINE
long OVERLOADABLE mad_sat( long a,
                           long b,
                           long c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _i64_i64_i64, )( a, b, c );
}

INLINE
long2 OVERLOADABLE mad_sat( long2 a,
                            long2 b,
                            long2 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v2i64_v2i64_v2i64, )( a, b, c );
}

INLINE
long3 OVERLOADABLE mad_sat( long3 a,
                            long3 b,
                            long3 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v3i64_v3i64_v3i64, )( a, b, c );
}

INLINE
long4 OVERLOADABLE mad_sat( long4 a,
                            long4 b,
                            long4 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v4i64_v4i64_v4i64, )( a, b, c );
}

INLINE
long8 OVERLOADABLE mad_sat( long8 a,
                            long8 b,
                            long8 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v8i64_v8i64_v8i64, )( a, b, c );
}

INLINE
long16 OVERLOADABLE mad_sat( long16 a,
                             long16 b,
                             long16 c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _v16i64_v16i64_v16i64, )( a, b, c );
}

INLINE
ulong OVERLOADABLE mad_sat( ulong a,
                            ulong b,
                            ulong c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _i64_i64_i64, )( a, b, c );
}

INLINE
ulong2 OVERLOADABLE mad_sat( ulong2 a,
                             ulong2 b,
                             ulong2 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v2i64_v2i64_v2i64, )( a, b, c );
}

INLINE
ulong3 OVERLOADABLE mad_sat( ulong3 a,
                             ulong3 b,
                             ulong3 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v3i64_v3i64_v3i64, )( a, b, c );
}

INLINE
ulong4 OVERLOADABLE mad_sat( ulong4 a,
                             ulong4 b,
                             ulong4 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v4i64_v4i64_v4i64, )( a, b, c );
}

INLINE
ulong8 OVERLOADABLE mad_sat( ulong8 a,
                             ulong8 b,
                             ulong8 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v8i64_v8i64_v8i64, )( a, b, c );
}

INLINE
ulong16 OVERLOADABLE mad_sat( ulong16 a,
                              ulong16 b,
                              ulong16 c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _v16i64_v16i64_v16i64, )( a, b, c );
}

