/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/tanpi_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/tanpi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __builtin_spirv_OpenCL_tanpi_f32( float x )
{
    bool useNative = __FastRelaxedMath && (!__APIRS);

    if(useNative)
    {
        return __builtin_spirv_OpenCL_native_tan_f32(x * M_PI_F);
    }
    else
    {
        return __ocl_svml_tanpif(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_tanpi_f64( double x )
{
    return __ocl_svml_tanpi(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tanpi_f16( half x )
{
    return __builtin_spirv_OpenCL_tanpi_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, half, half, f16 )

#endif // defined(cl_khr_fp16)
