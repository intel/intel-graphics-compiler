/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE trunc(float x) { return __spirv_ocl_trunc(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE trunc(half x) { return __spirv_ocl_trunc(x); }
#endif

#if defined(cl_khr_fp64)
INLINE
double OVERLOADABLE trunc(double x) { return __spirv_ocl_trunc(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(trunc, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG(trunc, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(trunc, half, half)

#endif // defined(cl_khr_fp16)
