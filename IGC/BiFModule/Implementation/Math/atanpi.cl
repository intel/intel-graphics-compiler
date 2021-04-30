/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/atanpi_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/atanpi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __builtin_spirv_OpenCL_atanpi_f32( float x )
{
    return __ocl_svml_atanpif(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atanpi_f64( double x )
{
    return __ocl_svml_atanpi(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atanpi_f16( half x )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_atan_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, half, half, f16 )

#endif // defined(cl_khr_fp16)
