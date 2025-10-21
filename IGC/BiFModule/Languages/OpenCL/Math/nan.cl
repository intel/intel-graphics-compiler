/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE nan( uint nancode )
{
    return __spirv_ocl_nan( as_int(nancode) );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, float, uint )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE nan( ulong nancode )
{
    return __spirv_ocl_nan( as_long(nancode) );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, double, ulong )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE nan( ushort nancode )
{
    return __spirv_ocl_nan( nancode );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( nan, half, ushort )

#endif // defined(cl_khr_fp16)
