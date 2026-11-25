/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE max(float x, float y) { return __spirv_ocl_fmax_common(x, y); }

#ifdef cl_khr_fp16
INLINE half OVERLOADABLE max(half x, half y) { return __builtin_IB_HMAX(x, y); }
#endif

#if defined(cl_khr_fp64)
INLINE double OVERLOADABLE max(double x, double y) { return (x >= y) ? x : y; }
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(max, float, float)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, float, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_2ARGS(max, double, double)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, double, double, double)

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_VECTOR_FUNCTIONS_2ARGS(max, half, half)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, half, half, half)

#endif // defined(cl_khr_fp16)
