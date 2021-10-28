/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE
uchar __builtin_spirv_OpenCL_bitselect_i8_i8_i8( uchar a,
                                          uchar b,
                                          uchar c )
{
    uchar temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, uchar, uchar, i8 )

INLINE
ushort __builtin_spirv_OpenCL_bitselect_i16_i16_i16( ushort a,
                                              ushort b,
                                              ushort c )
{
    ushort temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, ushort, ushort, i16 )

INLINE
uint __builtin_spirv_OpenCL_bitselect_i32_i32_i32( uint a,
                                            uint b,
                                            uint c )
{
    uint temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, uint, uint, i32 )

INLINE
ulong __builtin_spirv_OpenCL_bitselect_i64_i64_i64( ulong a,
                                             ulong b,
                                             ulong c )
{
    ulong temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, ulong, ulong, i64 )

INLINE
float __builtin_spirv_OpenCL_bitselect_f32_f32_f32( float a,
                                             float b,
                                             float c )
{
    return as_float( __builtin_spirv_OpenCL_bitselect_i32_i32_i32(as_int(a), as_int(b), as_int(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE
double __builtin_spirv_OpenCL_bitselect_f64_f64_f64( double a,
                                              double b,
                                              double c )
{
    return as_double( __builtin_spirv_OpenCL_bitselect_i64_i64_i64(as_long(a), as_long(b), as_long(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

INLINE
half __builtin_spirv_OpenCL_bitselect_f16_f16_f16( half a,
                                            half b,
                                            half c )
{
    return as_half( __builtin_spirv_OpenCL_bitselect_i16_i16_i16(as_short(a), as_short(b), as_short(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, half, half, f16 )

#endif
