/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
int OVERLOADABLE mul24( int x,
                        int y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
int2 OVERLOADABLE mul24( int2 x,
                         int2 y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
int3 OVERLOADABLE mul24( int3 x,
                         int3 y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
int4 OVERLOADABLE mul24( int4 x,
                         int4 y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
int8 OVERLOADABLE mul24( int8 x,
                         int8 y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
int16 OVERLOADABLE mul24( int16 x,
                          int16 y )
{
    return __spirv_ocl_s_mul24( x, y );
}

INLINE
uint OVERLOADABLE mul24( uint x,
                         uint y )
{
    return __spirv_ocl_u_mul24( x, y );
}

INLINE
uint2 OVERLOADABLE mul24( uint2 x,
                          uint2 y )
{
    return __spirv_ocl_u_mul24( x, y );
}

INLINE
uint3 OVERLOADABLE mul24( uint3 x,
                          uint3 y )
{
    return __spirv_ocl_u_mul24( x, y );
}

INLINE
uint4 OVERLOADABLE mul24( uint4 x,
                          uint4 y )
{
    return __spirv_ocl_u_mul24( x, y );
}

INLINE
uint8 OVERLOADABLE mul24( uint8 x,
                          uint8 y )
{
    return __spirv_ocl_u_mul24( x, y );
}

INLINE
uint16 OVERLOADABLE mul24( uint16 x,
                           uint16 y )
{
    return __spirv_ocl_u_mul24( x, y );
}

