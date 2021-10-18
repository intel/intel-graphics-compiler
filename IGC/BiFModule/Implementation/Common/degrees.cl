/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f32, )(float r ){
    return ONE_EIGHTY_OVER_PI_FLT * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, float, float, f32 )

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f64, )(double r ){
    return ONE_EIGHTY_OVER_PI_DBL * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(degrees, _f16, )(half r ){
    return ONE_EIGHTY_OVER_PI_HLF * r;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, half, half, f16 )

#endif // defined(cl_khr_fp16)
