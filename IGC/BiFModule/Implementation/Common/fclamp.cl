/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fclamp, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fclamp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fclamp, half, half, f16 )

#endif // defined(cl_khr_fp16)
