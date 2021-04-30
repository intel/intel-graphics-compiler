/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/atan_d_la.cl"
#endif // defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atan_f64( double x )
{
    return __ocl_svml_atan(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, half, half, f16 )

#endif // defined(cl_khr_fp16)
