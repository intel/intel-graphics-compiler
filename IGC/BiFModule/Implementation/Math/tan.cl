/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/tan_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/tan_d_la.cl"
#endif // defined(cl_khr_fp64)

static INLINE float __intel_tan_f32( float x, bool doFast )
{
    float result;
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        result = __builtin_spirv_OpenCL_native_tan_f32(x);
    }
    else
    {
        if(as_uint(x) == 0x45753168)
        {
            result = as_float(0xBF73F75D);
        }
        else if(as_uint(x) == 0xC5753168)
        {
            result = as_float(0x3F73F75D);
        }
        else
        {
            result = __ocl_svml_tanf(x);
        }
    }
    return result;
}

INLINE float __builtin_spirv_OpenCL_tan_f32( float x )
{
    return __intel_tan_f32(x, true);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_tan_f64( double x )
{
    return __ocl_svml_tan(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tan_f16( half x )
{
    return __builtin_spirv_OpenCL_tan_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, half, half, f16 )

#endif // defined(cl_khr_fp16)
