/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../include/exp_for_hyper.cl"
#include "../IMF/FP32/tanh_s_la_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/tanh_d_la.cl"
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f32, )( float x )
{
    float result;

    if( __intel_relaxed_isnan(x) )
    {
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if(BIF_FLAG_CTRL_GET(UseHighAccuracyMath))
    {
        result = __ocl_svml_tanhf_noLUT(x);
    }
    else if(SPIRV_OCL_BUILTIN(fabs, _f32, )(x) < as_float(0x3A71E7A0))     // 0.00092279352247715
    {
        result = x;
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(x) < as_float(0x3EACB527) )   // 0.33731958270072937
    {
        float sinhx, coshx;
        {
            float x2 = x * x;
            float x3 = x * x2;
            float x5 = x3 * x2;
            sinhx = (as_float(0x3C088889) * x5) + (as_float(0x3E2AAAAB) * x3) + x;
        }
        {
            float pexp = __intel_exp_for_tanh( x, -2.0f);
            float nexp = __intel_exp_for_tanh(-x, -2.0f);
            coshx = 2.0f * ( pexp + nexp );
        }
        result = sinhx / coshx;
    }
    else if (SPIRV_OCL_BUILTIN(fabs, _f32, )(x) < as_float(0x41987E0C))    // 19.061546325683594
    {
        float exp2x = __intel_exp_for_hyper(2 * x, 0.0f);
        result = (exp2x - 1) / (exp2x + 1);
    }
    else
    {
        result = (x > 0) ? 1.0f : -1.0f;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f64, )( double x )
{
    return __ocl_svml_tanh(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(tanh, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(tanh, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARG_LOOP( tanh, half, half, f16 )

#endif // defined(cl_khr_fp16)
