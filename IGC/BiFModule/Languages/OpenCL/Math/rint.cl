/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE rint(float x) { return __spirv_ocl_rint(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE rint(half x) { return __spirv_ocl_rint(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(rint, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE rint(double x) { return __spirv_ocl_rint(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG(rint, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(rint, half, half)

#endif // defined(cl_khr_fp16)
