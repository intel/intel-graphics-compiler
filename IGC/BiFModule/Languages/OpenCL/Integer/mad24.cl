/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"




INLINE
int2 OVERLOADABLE mad24( int2 x,
                         int2 y,
                         int2 z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}

INLINE
int3 OVERLOADABLE mad24( int3 x,
                         int3 y,
                         int3 z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}

INLINE
int4 OVERLOADABLE mad24( int4 x,
                         int4 y,
                         int4 z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}

INLINE
int8 OVERLOADABLE mad24( int8 x,
                         int8 y,
                         int8 z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}

INLINE
int16 OVERLOADABLE mad24( int16 x,
                          int16 y,
                          int16 z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}



INLINE
uint2 OVERLOADABLE mad24( uint2 x,
                          uint2 y,
                          uint2 z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

INLINE
uint3 OVERLOADABLE mad24( uint3 x,
                          uint3 y,
                          uint3 z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

INLINE
uint4 OVERLOADABLE mad24( uint4 x,
                          uint4 y,
                          uint4 z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

INLINE
uint8 OVERLOADABLE mad24( uint8 x,
                          uint8 y,
                          uint8 z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

INLINE
uint16 OVERLOADABLE mad24( uint16 x,
                           uint16 y,
                           uint16 z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

