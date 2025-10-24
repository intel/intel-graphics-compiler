/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/exp2_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp2, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f64, )( double x )
{
    return __ocl_svml_exp2(x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp2, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(exp2, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(exp2, _f32, )((float)x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( exp2, half, half, f16 )

#endif // defined(cl_khr_fp16)
