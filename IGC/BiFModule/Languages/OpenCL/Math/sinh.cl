/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

float OVERLOADABLE sinh(float x) { return __spirv_ocl_sinh(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(sinh, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE sinh(double x) { return __spirv_ocl_sinh(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(sinh, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE sinh(half x) { return __spirv_ocl_sinh(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(sinh, half, half)

#endif // defined(cl_khr_fp16)
