/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

INLINE
float __attribute__((overloadable)) __spirv_ocl_acos(float x) {
  // The LA acos implementation (IMF/FP32/acos_s_la.cl)
  // seems to be slower on Mandelbulb algorithm..
  float temp1 = 0.0f;
  float temp2 = 0.0f;
  float temp4 = 0.0f;

  float destTemp = 0.0f;

  bool flag = false;

  temp1 = -__spirv_ocl_fabs(x) + 1.0f;

  temp1 = temp1 * 0.5f;

  flag = __spirv_ocl_fabs(x) > 0.575f;

  if (flag) {
    temp1 = __spirv_ocl_sqrt(temp1);
  } else {
    temp1 = __spirv_ocl_fabs(x);
  }

  temp2 = temp1 * temp1;

  destTemp = temp2 * -0.5011622905731201f;

  temp4 = temp2 + -5.478654384613037f;

  destTemp = destTemp + 0.9152014851570129f;

  temp4 = temp2 * temp4;

  destTemp = temp2 * destTemp;

  temp4 = temp4 + 5.491230487823486f;

  destTemp = temp1 * destTemp;

  temp4 = __spirv_ocl_native_recip(temp4);

  destTemp = temp4 * destTemp;

  destTemp = temp1 + destTemp;

  if (flag) {
    destTemp = destTemp * 2.0f;
  } else {
    destTemp = -destTemp + 1.5707963705062866f;
  }

  if (x <= 0.0) {
    destTemp = -destTemp + 3.1415927410125732f;
  }

  return destTemp;
}

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_acos(half x) { return (half)__spirv_ocl_acos((float)x); }
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(acos, float, float, f32)

#if defined(cl_khr_fp64)
#include "../IMF/FP64/acos_d_la.h"
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_acos(double x) { return __ocl_svml_acos(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(acos, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(acos, half, half, f16)

#endif // defined(cl_khr_fp16)
