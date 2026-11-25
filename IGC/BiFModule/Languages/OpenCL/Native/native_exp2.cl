/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE native_exp2(float x) { return __spirv_ocl_native_exp2(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE native_exp2(half x) { return __spirv_ocl_native_exp2(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(native_exp2, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_exp2(double x) { return __spirv_ocl_native_exp2(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG(native_exp2, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(native_exp2, half, half)

#endif // defined(cl_khr_fp16)
