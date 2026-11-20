/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/atanpi_s_la.h"

#if defined(cl_khr_fp64)
#include "../IMF/FP64/atanpi_d_la.h"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_atanpi(float x) { return __ocl_svml_atanpif(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atanpi, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_atanpi(double x) { return __ocl_svml_atanpi(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atanpi, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_atanpi(half x) { return M_1_PI_H * __spirv_ocl_atan(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atanpi, half, half, f16)

#endif // defined(cl_khr_fp16)
