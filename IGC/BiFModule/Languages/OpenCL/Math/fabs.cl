/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE fabs(float x) { return __spirv_ocl_fabs(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE fabs(half x) { return __spirv_ocl_fabs(x); }
#endif

#if defined(cl_khr_fp64)
INLINE
double OVERLOADABLE fabs(double x) { return __spirv_ocl_fabs(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(fabs, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG(fabs, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(fabs, half, half)

#endif // defined(cl_khr_fp16)
