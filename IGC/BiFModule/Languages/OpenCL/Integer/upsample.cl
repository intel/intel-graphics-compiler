/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
short OVERLOADABLE upsample( char  hi,
                             uchar lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
short2 OVERLOADABLE upsample( char2  hi,
                              uchar2 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
short3 OVERLOADABLE upsample( char3  hi,
                              uchar3 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
short4 OVERLOADABLE upsample( char4  hi,
                              uchar4 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
short8 OVERLOADABLE upsample( char8  hi,
                              uchar8 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
short16 OVERLOADABLE upsample( char16  hi,
                               uchar16 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
ushort OVERLOADABLE upsample( uchar hi,
                              uchar lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ushort2 OVERLOADABLE upsample( uchar2 hi,
                               uchar2 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ushort3 OVERLOADABLE upsample( uchar3 hi,
                               uchar3 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ushort4 OVERLOADABLE upsample( uchar4 hi,
                               uchar4 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ushort8 OVERLOADABLE upsample( uchar8 hi,
                               uchar8 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ushort16 OVERLOADABLE upsample( uchar16 hi,
                                uchar16 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
int OVERLOADABLE upsample( short  hi,
                           ushort lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
int2 OVERLOADABLE upsample( short2  hi,
                            ushort2 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
int3 OVERLOADABLE upsample( short3  hi,
                            ushort3 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
int4 OVERLOADABLE upsample( short4  hi,
                            ushort4 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
int8 OVERLOADABLE upsample( short8  hi,
                            ushort8 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
int16 OVERLOADABLE upsample( short16  hi,
                             ushort16 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
uint OVERLOADABLE upsample( ushort hi,
                            ushort lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
uint2 OVERLOADABLE upsample( ushort2 hi,
                             ushort2 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
uint3 OVERLOADABLE upsample( ushort3 hi,
                             ushort3 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
uint4 OVERLOADABLE upsample( ushort4 hi,
                             ushort4 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
uint8 OVERLOADABLE upsample( ushort8 hi,
                             ushort8 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
uint16 OVERLOADABLE upsample( ushort16 hi,
                              ushort16 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
long OVERLOADABLE upsample( int  hi,
                            uint lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
long2 OVERLOADABLE upsample( int2  hi,
                             uint2 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
long3 OVERLOADABLE upsample( int3  hi,
                             uint3 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
long4 OVERLOADABLE upsample( int4  hi,
                             uint4 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
long8 OVERLOADABLE upsample( int8  hi,
                             uint8 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
long16 OVERLOADABLE upsample( int16  hi,
                              uint16 lo )
{
    return __spirv_ocl_s_upsample( hi, lo );
}

INLINE
ulong OVERLOADABLE upsample( uint hi,
                             uint lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ulong2 OVERLOADABLE upsample( uint2 hi,
                              uint2 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ulong3 OVERLOADABLE upsample( uint3 hi,
                              uint3 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ulong4 OVERLOADABLE upsample( uint4 hi,
                              uint4 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ulong8 OVERLOADABLE upsample( uint8 hi,
                              uint8 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

INLINE
ulong16 OVERLOADABLE upsample( uint16 hi,
                               uint16 lo )
{
    return __spirv_ocl_u_upsample( hi, lo );
}

