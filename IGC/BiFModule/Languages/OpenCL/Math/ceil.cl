/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_1ARG(ceil, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG(ceil, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(ceil, half, half)

#endif // defined(cl_khr_fp16)
