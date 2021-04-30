/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __builtin_spirv_OpenCL_nan_i32( uint nancode )
{
    return as_float( FLOAT_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, float, uint, i32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_nan_i64( ulong nancode )
{
    return as_double( DOUBLE_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, double, ulong, i64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_nan_i16( ushort nancode )
{
    return as_half( HALF_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, half, ushort, i16 )

#endif // defined(cl_khr_fp16)
