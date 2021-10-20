/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE char __builtin_spirv_OpenCL_s_clamp_i8_i8_i8( char x, char minval, char maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i8_i8, )(__builtin_spirv_OpenCL_s_max_i8_i8(x, minval), maxval);
}

INLINE uchar __builtin_spirv_OpenCL_u_clamp_i8_i8_i8( uchar x, uchar minval, uchar maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i8_i8, )(__builtin_spirv_OpenCL_u_max_i8_i8(x, minval), maxval);
}

INLINE short __builtin_spirv_OpenCL_s_clamp_i16_i16_i16( short x, short minval, short maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i16_i16, )(__builtin_spirv_OpenCL_s_max_i16_i16(x, minval), maxval);
}

INLINE ushort __builtin_spirv_OpenCL_u_clamp_i16_i16_i16( ushort x, ushort minval, ushort maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i16_i16, )(__builtin_spirv_OpenCL_u_max_i16_i16(x, minval), maxval);
}

INLINE int __builtin_spirv_OpenCL_s_clamp_i32_i32_i32( int x, int minval, int maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i32_i32, )(__builtin_spirv_OpenCL_s_max_i32_i32(x, minval), maxval);
}

INLINE uint __builtin_spirv_OpenCL_u_clamp_i32_i32_i32( uint x, uint minval, uint maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i32_i32, )(__builtin_spirv_OpenCL_u_max_i32_i32(x, minval), maxval);
}

INLINE long __builtin_spirv_OpenCL_s_clamp_i64_i64_i64( long x, long minval, long maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i64_i64, )(__builtin_spirv_OpenCL_s_max_i64_i64(x, minval), maxval);
}

INLINE ulong __builtin_spirv_OpenCL_u_clamp_i64_i64_i64( ulong x, ulong minval, ulong maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i64_i64, )(__builtin_spirv_OpenCL_u_max_i64_i64(x, minval), maxval);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, char, char, i8 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, uchar, uchar, i8 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, short, short, i16 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, ushort, ushort, i16 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, int, int, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, uint, uint, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_s_clamp, long, long, i64 )
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_u_clamp, ulong, ulong, i64 )

