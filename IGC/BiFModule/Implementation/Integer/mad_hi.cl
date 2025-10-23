/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
char __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char a,
                                        char b,
                                        char c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
char2 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char2 a,
                                               char2 b,
                                               char2 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
char3 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char3 a,
                                               char3 b,
                                               char3 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
char4 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char4 a,
                                               char4 b,
                                               char4 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
char8 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char8 a,
                                               char8 b,
                                               char8 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
char16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( char16 a,
                                                   char16 b,
                                                   char16 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar a,
                                         uchar b,
                                         uchar c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uchar2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar2 a,
                                                uchar2 b,
                                                uchar2 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uchar3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar3 a,
                                                uchar3 b,
                                                uchar3 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uchar4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar4 a,
                                                uchar4 b,
                                                uchar4 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uchar8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar8 a,
                                                uchar8 b,
                                                uchar8 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uchar16 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uchar16 a,
                                                    uchar16 b,
                                                    uchar16 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short a,
                                            short b,
                                            short c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
short2 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short2 a,
                                                   short2 b,
                                                   short2 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
short3 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short3 a,
                                                   short3 b,
                                                   short3 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
short4 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short4 a,
                                                   short4 b,
                                                   short4 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
short8 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short8 a,
                                                   short8 b,
                                                   short8 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
short16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( short16 a,
                                                       short16 b,
                                                       short16 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort a,
                                             ushort b,
                                             ushort c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ushort2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort2 a,
                                                    ushort2 b,
                                                    ushort2 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ushort3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort3 a,
                                                    ushort3 b,
                                                    ushort3 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ushort4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort4 a,
                                                    ushort4 b,
                                                    ushort4 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ushort8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort8 a,
                                                    ushort8 b,
                                                    ushort8 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ushort16 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ushort16 a,
                                                        ushort16 b,
                                                        ushort16 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int a,
                                          int b,
                                          int c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
int2 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int2 a,
                                                 int2 b,
                                                 int2 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
int3 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int3 a,
                                                 int3 b,
                                                 int3 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
int4 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int4 a,
                                                 int4 b,
                                                 int4 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
int8 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int8 a,
                                                 int8 b,
                                                 int8 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
int16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( int16 a,
                                                     int16 b,
                                                     int16 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint a,
                                           uint b,
                                           uint c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uint2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint2 a,
                                                  uint2 b,
                                                  uint2 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uint3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint3 a,
                                                  uint3 b,
                                                  uint3 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uint4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint4 a,
                                                  uint4 b,
                                                  uint4 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uint8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint8 a,
                                                  uint8 b,
                                                  uint8 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
uint16 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( uint16 a,
                                                      uint16 b,
                                                      uint16 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long a,
                                           long b,
                                           long c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
long2 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long2 a,
                                                  long2 b,
                                                  long2 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
long3 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long3 a,
                                                  long3 b,
                                                  long3 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
long4 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long4 a,
                                                  long4 b,
                                                  long4 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
long8 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long8 a,
                                                  long8 b,
                                                  long8 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
long16 __attribute__((overloadable)) __spirv_ocl_s_mad_hi( long16 a,
                                                      long16 b,
                                                      long16 c )
{
    return __spirv_ocl_s_mul_hi(a, b) + c;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong a,
                                            ulong b,
                                            ulong c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ulong2 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong2 a,
                                                   ulong2 b,
                                                   ulong2 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ulong3 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong3 a,
                                                   ulong3 b,
                                                   ulong3 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ulong4 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong4 a,
                                                   ulong4 b,
                                                   ulong4 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ulong8 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong8 a,
                                                   ulong8 b,
                                                   ulong8 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

INLINE
ulong16 __attribute__((overloadable)) __spirv_ocl_u_mad_hi( ulong16 a,
                                                       ulong16 b,
                                                       ulong16 c )
{
    return __spirv_ocl_u_mul_hi(a, b) + c;
}

