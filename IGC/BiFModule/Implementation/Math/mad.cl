/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE float __attribute__((overloadable)) __spirv_ocl_mad( float a, float b, float c )
{
    // If -cl-mad-enable flag set later optimizations will decide how to combine it into mad
    if(BIF_FLAG_CTRL_GET(MadEnable))
    {
        return a * b + c;
    }
    else
    {
        return __spirv_ocl_fma(a,b,c);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_mad( double a, double b, double c )
{
    // If -cl-mad-enable flag set later optimizations will decide how to combine it into mad
    if(BIF_FLAG_CTRL_GET(MadEnable))
    {
        return a * b + c;
    }
    else
    {
        return __spirv_ocl_fma(a,b,c);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mad, half, half, f16 )

#endif // defined(cl_khr_fp16)

