/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


INLINE long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(popcount, _i64, )( long x )
{
    int2 v = as_int2( x );
    return SPIRV_OCL_BUILTIN(popcount, _i32, )( v.x ) + SPIRV_OCL_BUILTIN(popcount, _i32, )( v.y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, char,  char,  i8 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, short, short, i16 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, int,   int,   i32 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( popcount, long,  long,  i64 )
