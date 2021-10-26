/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../ExternalLibraries/libclc/trig.cl"
#include "../IMF/FP32/cos_s_la.cl"
#include "../IMF/FP32/cos_s_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/cos_d_la.cl"
#endif // defined(cl_khr_fp64)

static INLINE float __intel_cos_f32( float x, bool doFast )
{
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        return SPIRV_OCL_BUILTIN(native_cos, _f32, )(x);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_cosf(x);
        }
        else
        {
            float abs_float = SPIRV_OCL_BUILTIN(fabs, _f32, )(x);
            if( abs_float > 10000.0f )
            {
                return libclc_cos_f32(x);
            }
            else
            {
                return __ocl_svml_cosf_noLUT(x);
            }
        }
    }
}

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f32, )( float x )
{
    return __intel_cos_f32(x, true);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cos, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f64, )( double x )
{
    return __ocl_svml_cos(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cos, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cos, _f16, )( half x )
{
    return (half)SPIRV_OCL_BUILTIN(cos, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( cos, half, half, f16 )

#endif // defined(cl_khr_fp16)
