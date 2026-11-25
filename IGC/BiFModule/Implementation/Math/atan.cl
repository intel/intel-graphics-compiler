/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)
#include "../IMF/FP64/atan_d_la.h"
#endif // defined(cl_khr_fp64)

INLINE
float __attribute__((overloadable)) __spirv_ocl_atan(float value) {
  // The LA atan implementation (IMF/FP32/atan_s_la.cl)
  // seems to be slower on Mandelbulb algorithm..
  float temp1 = 0.0f;
  float temp2 = 0.0f;
  float temp3 = 0.0f;
  float temp4 = 0.0f;

  float destTemp = 0.0f;

  bool flag = __spirv_ocl_fabs(value) > 1.0f;

  temp1 = __spirv_ocl_fabs(value);

  if (flag) {
    temp1 = __spirv_ocl_native_recip(temp1);
  }

  temp2 = temp1 * temp1;

  destTemp = temp2 * -0.8233629465103149f;

  temp4 = temp2 + 11.33538818359375f;

  destTemp = destTemp + -5.674867153167725f;

  temp4 = temp4 * temp2;

  destTemp = temp2 * destTemp;

  temp4 = temp4 + 28.84246826171875f;

  destTemp = destTemp + -6.565555095672607f;

  temp4 = temp4 * temp2;

  destTemp = temp2 * destTemp;

  temp4 = temp4 + 19.696670532226562f;

  destTemp = temp1 * destTemp;

  temp4 = __spirv_ocl_native_recip(temp4);

  destTemp = temp4 * destTemp;

  destTemp = destTemp + temp1;

  if (flag) {
    destTemp = -destTemp + 1.5707963705062866f;
  }

  if (__spirv_SignBitSet(value)) {
    destTemp = -__spirv_ocl_fabs(destTemp);
  }

  return destTemp;
}

#ifdef cl_khr_fp16
INLINE
half __attribute__((overloadable)) __spirv_ocl_atan(half x) { return (half)__spirv_ocl_atan((float)x); }
#endif

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atan, float, float, f32)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_atan(double x) { return __ocl_svml_atan(x); }

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atan, double, double, f64)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS(atan, half, half, f16)

#endif // defined(cl_khr_fp16)
