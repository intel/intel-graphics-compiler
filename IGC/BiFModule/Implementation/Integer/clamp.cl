/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE char __attribute__((overloadable)) __spirv_ocl_s_clamp( char x, char minval, char maxval )
{
    return __spirv_ocl_s_min(__spirv_ocl_s_max(x, minval), maxval);
}

INLINE uchar __attribute__((overloadable)) __spirv_ocl_u_clamp( uchar x, uchar minval, uchar maxval )
{
    return __spirv_ocl_u_min(__spirv_ocl_u_max(x, minval), maxval);
}

INLINE short __attribute__((overloadable)) __spirv_ocl_s_clamp( short x, short minval, short maxval )
{
    return __spirv_ocl_s_min(__spirv_ocl_s_max(x, minval), maxval);
}

INLINE ushort __attribute__((overloadable)) __spirv_ocl_u_clamp( ushort x, ushort minval, ushort maxval )
{
    return __spirv_ocl_u_min(__spirv_ocl_u_max(x, minval), maxval);
}

INLINE int __attribute__((overloadable)) __spirv_ocl_s_clamp( int x, int minval, int maxval )
{
    return __spirv_ocl_s_min(__spirv_ocl_s_max(x, minval), maxval);
}

INLINE uint __attribute__((overloadable)) __spirv_ocl_u_clamp( uint x, uint minval, uint maxval )
{
    return __spirv_ocl_u_min(__spirv_ocl_u_max(x, minval), maxval);
}

INLINE long __attribute__((overloadable)) __spirv_ocl_s_clamp( long x, long minval, long maxval )
{
    return __spirv_ocl_s_min(__spirv_ocl_s_max(x, minval), maxval);
}

INLINE ulong __attribute__((overloadable)) __spirv_ocl_u_clamp( ulong x, ulong minval, ulong maxval )
{
    return __spirv_ocl_u_min(__spirv_ocl_u_max(x, minval), maxval);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, char, char, i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, uchar, uchar, i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, short, short, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, ushort, ushort, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, int, int, i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, uint, uint, i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( s_clamp, long, long, i64 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( u_clamp, ulong, ulong, i64 )

