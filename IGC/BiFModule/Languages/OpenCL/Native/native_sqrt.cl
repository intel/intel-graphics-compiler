/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE native_sqrt(float x) { return __spirv_ocl_native_sqrt(x); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE native_sqrt(half x) { return __spirv_ocl_native_sqrt(x); }
#endif

#if defined(cl_khr_fp64)
INLINE double OVERLOADABLE native_sqrt(double x) { return __spirv_ocl_native_sqrt(x); }
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sqrt, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sqrt, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG(native_sqrt, half, half)

#endif // defined(cl_khr_fp16)
