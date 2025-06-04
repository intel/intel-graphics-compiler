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
char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i8_i8_i8, )( char a, char b, char c )
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
short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i16_i16_i16, )( short a, short b, short c )
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
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i32_i32_i32, )( int a, int b, int c )
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
long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _i64_i64_i64, )( long a, long b, long c )
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
float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f32_f32_f32, )( float a, float b, float c )
{
    return as_float( SPIRV_OCL_BUILTIN(bitselect, _i32_i32_i32, )(as_int(a), as_int(b), as_int(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE
double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f64_f64_f64, )( double a, double b, double c )
{
    return as_double( SPIRV_OCL_BUILTIN(bitselect, _i64_i64_i64, )(as_long(a), as_long(b), as_long(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

INLINE
half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(bitselect, _f16_f16_f16, )( half a, half b, half c )
{
    return as_half( SPIRV_OCL_BUILTIN(bitselect, _i16_i16_i16, )(as_short(a), as_short(b), as_short(c)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( bitselect, half, half, f16 )

#endif
