/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE native_sin(float x) { return __spirv_ocl_native_sin(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE native_sin(half x) { return __spirv_ocl_native_sin(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sin, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_sin(double x) { return __spirv_ocl_native_sin(x); }

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sin, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sin, half, half)

#endif // defined(cl_khr_fp16)
