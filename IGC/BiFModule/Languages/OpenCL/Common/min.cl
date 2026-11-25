/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE min(float x, float y) { return __spirv_ocl_fmin_common(x, y); }

#ifdef cl_khr_fp16
INLINE half OVERLOADABLE min(half x, half y) { return __builtin_IB_HMIN(x, y); }
#endif

#if defined(cl_khr_fp64)
INLINE double OVERLOADABLE min(double x, double y) { return __builtin_IB_dmin(x, y); }
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(min, float, float)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, float, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_2ARGS(min, double, double)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, double, double, double)

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_VECTOR_FUNCTIONS_2ARGS(min, half, half)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, half, half, half)

#endif // defined(cl_khr_fp16)
