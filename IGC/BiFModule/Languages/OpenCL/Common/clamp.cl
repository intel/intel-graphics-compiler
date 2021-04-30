/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, float, float )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, float, float, float )

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, double, double )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, half, half )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, half, half, half )

#endif // defined(cl_khr_fp16)
