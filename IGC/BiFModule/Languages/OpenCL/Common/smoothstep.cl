/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE smoothstep( float edge0, float edge1, float x )
{
    return __spirv_ocl_smoothstep( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, float, float )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE smoothstep( double edge0, double edge1, double x )
{
    return __spirv_ocl_smoothstep( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, double, double )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE smoothstep( half edge0, half edge1, half x )
{
    return __spirv_ocl_smoothstep( edge0, edge1, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( smoothstep, half, half )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( smoothstep, half, half, half )

#endif // defined(cl_khr_fp16)
