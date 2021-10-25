/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE nan( uint nancode )
{
    return SPIRV_OCL_BUILTIN(nan, _i32, )( as_int(nancode) );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, float, uint )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE nan( ulong nancode )
{
    return SPIRV_OCL_BUILTIN(nan, _i64, )( as_long(nancode) );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, double, ulong )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE nan( ushort nancode )
{
    return SPIRV_OCL_BUILTIN(nan, _i16, )( nancode );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, half, ushort )

#endif // defined(cl_khr_fp16)
