/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/cbrt_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/cbrt_d_la.cl"
#endif

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f32, )( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as rootn(x, 3).
        result = SPIRV_OCL_BUILTIN(rootn, _f32_i32, )( x, 3 );
    }
    else
    {
        result = __ocl_svml_cbrtf(x);
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f64, )( double x )
{
    return __ocl_svml_cbrt(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(cbrt, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(cbrt, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( cbrt, half, half, f16 )

#endif // defined(cl_khr_fp16)
