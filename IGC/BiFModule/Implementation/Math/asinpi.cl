/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/asinpi_s_la.h"

#if defined(cl_khr_fp64)
#include "../IMF/FP64/asinpi_d_la.h"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_asinpi(float x) { return __ocl_svml_asinpif(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asinpi, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_asinpi(double x) { return __ocl_svml_asinpi(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asinpi, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_asinpi(half x) { return M_1_PI_H * __spirv_ocl_asin(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(asinpi, half, half, f16)

#endif // defined(cl_khr_fp16)
