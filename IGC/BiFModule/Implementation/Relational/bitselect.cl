/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

// Bitselect can be implemented with the following boolean function:
//   s0 & s1 | ~s0 & s2
// where s0 = c, s1 = b, s2 = a
// This maps to boolean function 0xD8.

INLINE
char __attribute__((overloadable)) __spirv_ocl_bitselect( char a, char b, char c )
{
    if (BIF_FLAG_CTRL_GET(UseBfn))
    {
        return (char) __builtin_IB_bfn_i16((short)as_uchar(c), (short)as_uchar(b), (short)as_uchar(a), 0xD8);
    }
    else
    {
        char temp;
        temp = (c & b) | (~c & a);
        return temp;
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, char, char, i8 )

INLINE
short __attribute__((overloadable)) __spirv_ocl_bitselect( short a, short b, short c )
{
    if (BIF_FLAG_CTRL_GET(UseBfn))
    {
        return __builtin_IB_bfn_i16(c, b, a, 0xD8);
    }
    else
    {
        short temp;
        temp = (c & b) | (~c & a);
        return temp;
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, short, short, i16 )

INLINE
int __attribute__((overloadable)) __spirv_ocl_bitselect( int a, int b, int c )
{
    if (BIF_FLAG_CTRL_GET(UseBfn))
    {
        return __builtin_IB_bfn_i32(c, b, a, 0xD8);
    }
    else
    {
        int temp;
        temp = (c & b) | (~c & a);
        return temp;
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, int, int, i32 )

INLINE
long __attribute__((overloadable)) __spirv_ocl_bitselect( long a, long b, long c )
{
    if (BIF_FLAG_CTRL_GET(UseBfn))
    {
        int2 tmpA = as_int2(a);
        int2 tmpB = as_int2(b);
        int2 tmpC = as_int2(c);
        int2 tmpResult;
        tmpResult.s0 = __builtin_IB_bfn_i32(tmpC.s0, tmpB.s0, tmpA.s0, 0xD8);
        tmpResult.s1 = __builtin_IB_bfn_i32(tmpC.s1, tmpB.s1, tmpA.s1, 0xD8);
        return as_long(tmpResult);
    }
    else
    {
        long temp;
        temp = (c & b) | (~c & a);
        return temp;
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, long, long, i64 )

INLINE
float __attribute__((overloadable)) __spirv_ocl_bitselect( float a, float b, float c )
{
    return as_float( __spirv_ocl_bitselect(as_int(a), as_int(b), as_int(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE
double __attribute__((overloadable)) __spirv_ocl_bitselect( double a, double b, double c )
{
    return as_double( __spirv_ocl_bitselect(as_long(a), as_long(b), as_long(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

INLINE
half __attribute__((overloadable)) __spirv_ocl_bitselect( half a, half b, half c )
{
    return as_half( __spirv_ocl_bitselect(as_short(a), as_short(b), as_short(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, half, half, f16 )

#endif

