/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE clamp(float x, float minval, float maxval) { return __spirv_ocl_fclamp(x, minval, maxval); }

#ifdef cl_khr_fp16
INLINE half OVERLOADABLE clamp(half x, half minval, half maxval) { return fmin(fmax(x, minval), maxval); }
#endif

#if defined(cl_khr_fp64)
INLINE double OVERLOADABLE clamp(double x, double minval, double maxval) { return fmin(fmax(x, minval), maxval); }
#endif

GENERATE_VECTOR_FUNCTIONS_3ARGS(clamp, float, float)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS(clamp, float, float, float)

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_3ARGS(clamp, double, double)

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_VECTOR_FUNCTIONS_3ARGS(clamp, half, half)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS(clamp, half, half, half)

#endif // defined(cl_khr_fp16)
