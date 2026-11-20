/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

int OVERLOADABLE ilogb(float x) { return __spirv_ocl_ilogb(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(ilogb, int, float)

#if defined(cl_khr_fp64)

int OVERLOADABLE ilogb(double x) { return __spirv_ocl_ilogb(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(ilogb, int, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int OVERLOADABLE ilogb(half x) { return __spirv_ocl_ilogb(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP(ilogb, int, half)

#endif // defined(cl_khr_fp16)
