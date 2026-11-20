/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/erfc_s_la.h"

#if defined(cl_khr_fp64)
#include "../IMF/FP64/erfc_d_la.h"
#endif // defined(cl_khr_fp64)

float __attribute__((overloadable)) __spirv_ocl_erfc(float x) { return __ocl_svml_erfcf(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(erfc, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_erfc(double x) { return __ocl_svml_erfc(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(erfc, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_erfc(half x) { return __spirv_ocl_erfc((float)x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(erfc, half, half, f16)

#endif // defined(cl_khr_fp16)
