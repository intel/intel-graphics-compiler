/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i8_i8_i8, )( char x, char minval, char maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i8_i8, )(SPIRV_OCL_BUILTIN(s_max, _i8_i8, )(x, minval), maxval);
}

INLINE uchar SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i8_i8_i8, )( uchar x, uchar minval, uchar maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i8_i8, )(SPIRV_OCL_BUILTIN(u_max, _i8_i8, )(x, minval), maxval);
}

INLINE short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i16_i16_i16, )( short x, short minval, short maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i16_i16, )(SPIRV_OCL_BUILTIN(s_max, _i16_i16, )(x, minval), maxval);
}

INLINE ushort SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i16_i16_i16, )( ushort x, ushort minval, ushort maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i16_i16, )(SPIRV_OCL_BUILTIN(u_max, _i16_i16, )(x, minval), maxval);
}

INLINE int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i32_i32_i32, )( int x, int minval, int maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i32_i32, )(SPIRV_OCL_BUILTIN(s_max, _i32_i32, )(x, minval), maxval);
}

INLINE uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i32_i32_i32, )( uint x, uint minval, uint maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i32_i32, )(SPIRV_OCL_BUILTIN(u_max, _i32_i32, )(x, minval), maxval);
}

INLINE long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_clamp, _i64_i64_i64, )( long x, long minval, long maxval )
{
    return SPIRV_OCL_BUILTIN(s_min, _i64_i64, )(SPIRV_OCL_BUILTIN(s_max, _i64_i64, )(x, minval), maxval);
}

INLINE ulong SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_clamp, _i64_i64_i64, )( ulong x, ulong minval, ulong maxval )
{
    return SPIRV_OCL_BUILTIN(u_min, _i64_i64, )(SPIRV_OCL_BUILTIN(u_max, _i64_i64, )(x, minval), maxval);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, char, char, i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, uchar, uchar, i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, short, short, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, ushort, ushort, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, int, int, i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, uint, uint, i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, long, long, i64 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, ulong, ulong, i64 )

