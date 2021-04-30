/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/sinpi_s_la.cl"
#include "../IMF/FP32/sinpi_s_noLUT.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/sinpi_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __builtin_spirv_OpenCL_sinpi_f32( float x )
{
    bool useNative = __FastRelaxedMath && (!__APIRS);

    if(useNative)
    {
        return __builtin_spirv_OpenCL_sin_f32(x * M_PI_F);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_sinpif(x);
        }
        else
        {
            return __ocl_svml_sinpif_noLUT(x);
        }
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_sinpi_f64( double x )
{
    return __ocl_svml_sinpi(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sinpi_f16( half x )
{
    return __builtin_spirv_OpenCL_sinpi_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, half, half, f16 )

#endif // defined(cl_khr_fp16)
