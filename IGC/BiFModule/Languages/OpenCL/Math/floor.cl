/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG( floor, float, float )

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( floor, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( floor, half, half )

#endif // defined(cl_khr_fp16)
