/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, half, half, f16 )

#endif // defined(cl_khr_fp16)
