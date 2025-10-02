/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
#pragma float_control(precise, on)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c6 = {0x38b5efa8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c5 = {0xba573fc9u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c4 = {0x3baa9d5du};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c3 = {0xbcdc0cd3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c2 = {0x3de71742u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c1 = {0xbec093a2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___c0 = {0x3e0375d4u};
// polynomial coefficients for [.875, 2)
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b7 = {0x3bd995c8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b6 = {0x3c92d52eu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b5 = {0xbd60734bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b4 = {0x3c21a26bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b3 = {0x3e2b2466u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b2 = {0xbea7785bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b1 = {0x3e8d06e3u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___b0 = {0xbdbe9fd3u};
// polynomial coefficients for [1.5, 4.0]
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r13 = {0x360f65a7u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r12 = {0xb68dd176u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r11 = {0xb76ca6edu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r10 = {0x387da91au};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r9 = {0xb89a64e2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r8 = {0xb8c57175u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r7 = {0x3a29cd1au};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r6 = {0xbad8e48au};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r5 = {0x3b349094u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r4 = {0xbb559086u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r3 = {0x3b34e93eu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r2 = {0xbad350a2u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r1 = {0x3a19b01cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __serf_ha___r0 = {0xb8d30572u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_serf_ha(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  union {
    unsigned int w;
    float f;
    int i;
  } x, xa, res;
  unsigned int sgn_x;
  float dR, dR2;
  union {
    unsigned int w;
    float f;
    int i;
  } poly;
  xa.f = xin;
  sgn_x = xa.w & 0x80000000;
  // |xin|
  xa.w ^= sgn_x;
  if (xa.f < 1.5f) {
    if (xa.f < 0.875f) {
      dR = xa.f;
      dR2 = __spirv_ocl_fma(dR, dR, 0.0f);
      // polynomial evaluation
      poly.f = __spirv_ocl_fma(__serf_ha___c6.f, dR2,
                                                      __serf_ha___c5.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR2, __serf_ha___c4.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR2, __serf_ha___c3.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR2, __serf_ha___c2.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR2, __serf_ha___c1.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR2, __serf_ha___c0.f);
      res.f = __spirv_ocl_fma(poly.f, dR, dR);
      res.w ^= sgn_x;
      *pres = res.f;
    } else {
      dR = xa.f - 1.1875f;
      // polynomial evaluation
      poly.f = __spirv_ocl_fma(__serf_ha___b7.f, dR,
                                                      __serf_ha___b6.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b5.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b4.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b3.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b2.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b1.f);
      poly.f =
          __spirv_ocl_fma(poly.f, dR, __serf_ha___b0.f);
      res.f = poly.f + 1.0;
      res.w ^= sgn_x;
      *pres = res.f;
    }
  } else {
    // limit |x| range to [0,4]
    dR = (xa.f > 4.0f) ? 4.0f : xa.f;
    // compiler workaround for NaNs
    dR = (xa.w <= 0x7f800000) ? dR : xa.f;
    dR = dR - 2.75f;
    // polynomial evaluation
    poly.f = __spirv_ocl_fma(__serf_ha___r13.f, dR,
                                                    __serf_ha___r12.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r11.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r10.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r9.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r8.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r7.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r6.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r5.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r4.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r3.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r2.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r1.f);
    poly.f =
        __spirv_ocl_fma(poly.f, dR, __serf_ha___r0.f);
    res.f = poly.f + 1.0;
    res.w ^= sgn_x;
    *pres = res.f;
  }
  return nRet;
}
float __ocl_svml_erff_ha(float x) {
  float r;
  __ocl_svml_internal_serf_ha(&x, &r);
  return r;
}
