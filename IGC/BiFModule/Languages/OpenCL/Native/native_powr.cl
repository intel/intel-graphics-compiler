/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
float OVERLOADABLE native_powr(float x, float y) { return __spirv_ocl_native_powr(x, y); }

#ifdef cl_khr_fp16
INLINE
half OVERLOADABLE native_powr(half x, half y) { return __spirv_ocl_native_powr(x, y); }
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(native_powr, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE native_powr(double x, double y) { return __spirv_ocl_native_powr(x, y); }

GENERATE_VECTOR_FUNCTIONS_2ARGS(native_powr, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_2ARGS(native_powr, half, half)

#endif // defined(cl_khr_fp16)
