/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../ExternalLibraries/libclc/trig.cl"
#include "../IMF/FP32/sin_s_la.cl"
#include "../IMF/FP32/sin_s_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/sin_d_la.cl"
#endif // defined(cl_khr_fp64)

static INLINE float __intel_sin_f32( float x, bool doFast )
{
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)) && doFast)
    {
        return __spirv_ocl_native_sin(x);
    }
    else
    {
        if(BIF_FLAG_CTRL_GET(UseMathWithLUT))
        {
            return __ocl_svml_sinf(x);
        }
        else
        {
            float abs_float = __spirv_ocl_fabs(x);
            if( abs_float > 10000.0f )
            {
                return libclc_sin_f32(x);
            }
            else
            {
                return __ocl_svml_sinf_noLUT(x);
            }
        }
    }
}

INLINE float __attribute__((overloadable)) __spirv_ocl_sin( float x )
{
    return __intel_sin_f32(x, true);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sin, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_sin( double x )
{
    return __ocl_svml_sin(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sin, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sin( half x )
{
    return (half)__spirv_ocl_sin((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( sin, half, half, f16 )

#endif // defined(cl_khr_fp16)

