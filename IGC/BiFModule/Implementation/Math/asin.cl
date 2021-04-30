/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/asin_d_la.cl"
#endif //defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_asin_f64( double x )
{
    return __ocl_svml_asin(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, half, half, f16 )

#endif // defined(cl_khr_fp16)
