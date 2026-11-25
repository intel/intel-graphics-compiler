/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE floor(float x) { return __spirv_ocl_floor(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE floor(half x) { return __spirv_ocl_floor(x); }
#endif

#if defined(cl_khr_fp64)
INLINE
double OVERLOADABLE floor(double x) { return __spirv_ocl_floor(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(floor, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG(floor, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(floor, half, half)

#endif // defined(cl_khr_fp16)
