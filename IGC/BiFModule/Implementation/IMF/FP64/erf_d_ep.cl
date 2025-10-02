/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
#pragma float_control(precise, on)
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b13 = {0xbbf04820f8782ea7uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b12 = {0x3c78c6a6ee56a902uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b11 = {0x3d27d9f72ac50c4euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b10 = {0x3d95b9d8ba22f87fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b9 = {0x3ded59e0b9dba72cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b8 = {0x3e40ec0d169c5661uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b7 = {0x3e88ca50312635b8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b6 = {0x3ed1666b61a7daa8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b5 = {0x3f0eab6025e6fe88uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b4 = {0x3f4c99b8f2e353d0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b3 = {0x3f7c3fbd04a37ceeuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b2 = {0x3fb0f935cf0c418cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b1 = {0x3fcc0406ecc19ca4uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___b0 = {0x3ff20dd750429b6euL};
// divisor coefficients
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a13 = {0xbc3dfa7e57cc8417uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a12 = {0x3ce0a482679e10e7uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a11 = {0x3d63909d10c4a13buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a10 = {0x3dc2a830735cd361uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a9 = {0x3e17313932e30be7uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a8 = {0x3e653467a0d8d5e2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a7 = {0x3eae22653b5c366auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a6 = {0x3ef0f7db44b1d85buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a5 = {0x3f2e53e7405862b5uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a4 = {0x3f6538c5ce02a52duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a3 = {0x3f968ff8f52c8468uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a2 = {0x3fc13830f163e2e9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a1 = {0x3fe0dfad7312eba5uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ep___a0 = {0x3ff0000000000000uL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_derf_ep(double *a, double *pres) {
  int nRet = 0;
  double xin = *a;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, xa, res;
  unsigned long sgn_x;
  float fy, fa;
  double dR, dR2, eps;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } apoly, bpoly, Y, Q, Ql;
  xa.f = xin;
  sgn_x = xa.w & 0x8000000000000000uL;
  // |xin|
  xa.w ^= sgn_x;
  // limit |x| range to [0,6]
  dR = (6.0 < xa.f) ? 6.0 : xa.f;
  dR2 = __spirv_ocl_fma(dR, dR, 0.0);
  // fixup for NaNs
  dR = (xa.w > 0x7ff0000000000000uL) ? xa.f : dR;
  // polynomial evaluation
  bpoly.f = __spirv_ocl_fma(__derf_ep___b13.f, dR2,
                                                   __derf_ep___b12.f);
  apoly.f = __spirv_ocl_fma(__derf_ep___a13.f, dR2,
                                                   __derf_ep___a12.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b11.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a11.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b10.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a10.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b9.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a9.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b8.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a8.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b7.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a7.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b6.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a6.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b5.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a5.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b4.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a4.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b3.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a3.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b2.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a2.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b1.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a1.f);
  bpoly.f =
      __spirv_ocl_fma(bpoly.f, dR2, __derf_ep___b0.f);
  apoly.f =
      __spirv_ocl_fma(apoly.f, dR2, __derf_ep___a0.f);
  fa = (float)apoly.f;
  fy = 1.0f / fa;
  Y.f = (double)fy;
  eps = __spirv_ocl_fma(-Y.f, apoly.f, 1.0);
  eps = __spirv_ocl_fma(eps, eps, eps);
  Y.f = __spirv_ocl_fma(Y.f, eps, Y.f);
  Y.f = __spirv_ocl_fma(Y.f, bpoly.f, 0.0);
  res.f = __spirv_ocl_fma(Y.f, dR, 0.0);
  //    res.f = (res.f > 1.0) ? 1.0 : res.f;
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}
double __ocl_svml_erf_ep(double x) {
  double r;
  __ocl_svml_internal_derf_ep(&x, &r);
  return r;
}
