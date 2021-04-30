/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE ulong __builtin_spirv_OpenCL_popcount_i64( ulong x )
{
    uint2 v = as_uint2( x );
    return __builtin_spirv_OpenCL_popcount_i32( v.x ) + __builtin_spirv_OpenCL_popcount_i32( v.y );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_popcount, uchar, uchar, i8 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_popcount, ushort, ushort, i16 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_popcount, uint, uint, i32 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_popcount, ulong, ulong, i64 )
