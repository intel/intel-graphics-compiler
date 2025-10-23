/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE long __attribute__((overloadable)) __spirv_ocl_popcount( long x )
{
    int2 v = as_int2( x );
    return __spirv_ocl_popcount( v.x ) + __spirv_ocl_popcount( v.y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, char,  char,  i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, short, short, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, int,   int,   i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, long,  long,  i64 )
