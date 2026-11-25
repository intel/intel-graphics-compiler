/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE mad(float a, float b, float c) { return __spirv_ocl_mad(a, b, c); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE mad(half a, half b, half c) { return __spirv_ocl_mad(a, b, c); }
#endif

GENERATE_VECTOR_FUNCTIONS_3ARGS(mad, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE mad(double a, double b, double c) { return __spirv_ocl_mad(a, b, c); }

GENERATE_VECTOR_FUNCTIONS_3ARGS(mad, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_3ARGS(mad, half, half)

#endif // defined(cl_khr_fp16)
