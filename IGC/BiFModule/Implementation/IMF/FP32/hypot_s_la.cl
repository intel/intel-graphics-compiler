/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
__attribute__((always_inline)) inline int
__ocl_svml_internal_shypot_la(float *pa, float *pb, float *pres) {
  int nRet = 0;
  float x = *pa, y = *pb;
  volatile union {
    unsigned int w;
    float f;
    int i;
  } ax, ay;
  union {
    unsigned int w;
    float f;
    int i;
  } expon0, res;
  float h12, l12, t0, t1, sum;
  float RS, Sh, RS2, eps;
  // take absolute values
  ax.f = x;
  ax.w &= 0x7fffffffu;
  ay.f = y;
  ay.w &= 0x7fffffffu;
  // eliminate special cases
  if ((ax.w >= 0x7f800000) || (ay.w >= 0x7f800000)) {
    if ((ax.w == 0x7f800000) || (ay.w == 0x7f800000)) {
      res.w = 0x7f800000;
      *pres = res.f;
      return nRet;
    }
    // return NaN
    *pres = x + y;
    return nRet;
  }
  // Order inputs based on absolute value
  h12 = SPIRV_OCL_BUILTIN(fmax, _f32_f32, )(ax.f, ay.f);
  l12 = SPIRV_OCL_BUILTIN(fmin, _f32_f32, )(ax.f, ay.f);
  // scale inputs
  expon0.f = h12;
  expon0.w &= 0x7f800000;
  // 2^(-expon_t0+ ((t0>=2)?1:(-1)))
  expon0.w = 0x7e800000 + ((expon0.w & 0x40000000) >> 6) - expon0.w;
  t0 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(h12, expon0.f, 0.0f);
  t1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(l12, expon0.f, 0.0f);
  sum = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(t1, t1, 0.0f);
  sum = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(t0, t0, sum);
  RS = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, )(sum);
  // Sh ~ sqrt(sum)
  Sh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sum, RS, 0.0f);
  RS2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(RS, 0.5f, 0.0f);
  // sum - Sh^2
  eps = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Sh, -Sh, sum);
  // sqrt(sum) ~ Sh + eps*RS2
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(RS2, eps, Sh);
  // 2^(-expon0)
  expon0.w = 0x7f000000 - expon0.w;
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(res.f, expon0.f, 0.0f);
  res.w = (sum != 0.0f) ? res.w : 0;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_hypotf_la(float x, float y) {
  float r;
  __ocl_svml_internal_shypot_la(&x, &y, &r);
  return r;
}
