/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/erfc_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/erfc_d_la.cl"
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f32, )( float x )
{
    return __ocl_svml_erfcf(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erfc, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f64, )( double x )
{
    return __ocl_svml_erfc(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erfc, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(erfc, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(erfc, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( erfc, half, half, f16 )

#endif // defined(cl_khr_fp16)
