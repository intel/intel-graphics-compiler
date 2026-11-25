/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE atan(float value) { return __spirv_ocl_atan(value); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE atan(half x) { return __spirv_ocl_atan(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(atan, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atan(double x) { return __spirv_ocl_atan(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG(atan, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(atan, half, half)

#endif // defined(cl_khr_fp16)
