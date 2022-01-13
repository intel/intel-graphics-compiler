/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )( float a, float b, float c )
{
    // If -cl-mad-enable flag set later optimizations will decide how to combine it into mad
    if(__MadEnable)
    {
        return a * b + c;
    }
    else
    {
        return SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(a,b,c);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(mad, _f64_f64_f64, )( double a, double b, double c )
{
    // If -cl-mad-enable flag set later optimizations will decide how to combine it into mad
    if(__MadEnable)
    {
        return a * b + c;
    }
    else
    {
        return SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(a,b,c);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, half, half, f16 )

#endif // defined(cl_khr_fp16)
