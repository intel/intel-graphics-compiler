/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_copysign( float x, float y )
{
    return as_float( (int)((as_int(y) & FLOAT_SIGN_MASK) + (as_int(x) & ~FLOAT_SIGN_MASK)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( copysign, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_copysign( double x, double y )
{
    return as_double( (long)((as_long(y) & DOUBLE_SIGN_MASK) + (as_long(x) & ~DOUBLE_SIGN_MASK)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( copysign, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_copysign( half x, half y )
{
    return as_half( (short)((as_short(y) & HALF_SIGN_MASK) + (as_short(x) & ~HALF_SIGN_MASK)) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( copysign, half, half, f16 )

#endif // defined(cl_khr_fp16)

