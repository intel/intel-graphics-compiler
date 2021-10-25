/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE fmax( float x, float y )
{
    return SPIRV_OCL_BUILTIN(fmax, _f32_f32, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmax, float, float )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmax, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fmax( double x, double y )
{
    return SPIRV_OCL_BUILTIN(fmax, _f64_f64, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmax, double, double )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmax, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fmax( half x, half y )
{
    return SPIRV_OCL_BUILTIN(fmax, _f16_f16, )( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( fmax, half, half )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( fmax, half, half, half )

#endif // defined(cl_khr_fp16)
