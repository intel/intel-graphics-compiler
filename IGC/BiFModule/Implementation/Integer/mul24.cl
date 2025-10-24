/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _i32_i32, )( int x,
                                     int y )
{
    return x * y;
}

INLINE
int2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v2i32_v2i32, )( int2 x,
                                          int2 y )
{
    return x * y;
}

INLINE
int3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v3i32_v3i32, )( int3 x,
                                          int3 y )
{
    return x * y;
}

INLINE
int4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v4i32_v4i32, )( int4 x,
                                          int4 y )
{
    return x * y;
}

INLINE
int8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v8i32_v8i32, )( int8 x,
                                          int8 y )
{
    return x * y;
}

INLINE
int16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_mul24, _v16i32_v16i32, )( int16 x,
                                             int16 y )
{
    return x * y;
}

INLINE
uint SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _i32_i32, )( uint x,
                                      uint y )
{
    return x * y;
}

INLINE
uint2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v2i32_v2i32, )( uint2 x,
                                           uint2 y )
{
    return x * y;
}

INLINE
uint3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v3i32_v3i32, )( uint3 x,
                                           uint3 y )
{
    return x * y;
}

INLINE
uint4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v4i32_v4i32, )( uint4 x,
                                           uint4 y )
{
    return x * y;
}

INLINE
uint8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v8i32_v8i32, )( uint8 x,
                                           uint8 y )
{
    return x * y;
}

INLINE
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_mul24, _v16i32_v16i32, )( uint16 x,
                                              uint16 y )
{
    return x * y;
}

