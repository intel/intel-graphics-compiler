/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
#pragma float_control(precise, on)
static __constant int __scos_ha___ip_h = 0x0517CC1B;
static __constant int __scos_ha___ip_m = 0x727220A9;
static __constant int __scos_ha___ip_l = 0x28;
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c5 = {0xbbe61a2du};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c5l = {0xaeba0fbau};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c4 = {0x3da807fcu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c4l = {0xaf614e97u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c3 = {0xbf196889u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c3l = {0xaf584810u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c2 = {0x402335e0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c2l = {0xb2f6c1feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c1 = {0xc0a55de7u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c1l = {0xb39e345au};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c0 = {0x40490fdbu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __scos_ha___c0l = {0xb3bbe1b6u};
static __constant unsigned int __scos_ha_invpi_tbl[] = {
    0,          0x28BE60DB, 0x9391054A, 0x7F09D5F4,
    0x7D4D3770, 0x36D8A566, 0x4F10E410, 0x7F9458EA};
__attribute__((always_inline)) inline int
__ocl_svml_internal_scos_ha(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  unsigned long IP, IP2;
  long IP_s, IP2_s;
  int ip_low_s;
  unsigned int ip_low;
  int_float x, Rh, Rl, res, scale;
  int mx, sgn_x, ex, ip_h, shift, index, j;
  float Low, R2h, R2l, poly_h, poly_l;
  x.f = xin;
  mx = (x.w & 0x007fffff) | 0x00800000;
  ex = (x.w >> 23) & 0xff;
  // redirect large or very small inputs
  if (__builtin_expect(((unsigned)(ex - 0x7f + 12)) > (20 + 12), (0 == 1))) {
    // small input
    if (__builtin_expect((ex < 0x7f - 11), (1 == 1))) {
      *pres = 1.0f;
      return nRet;
    }
    // Inf/NaN
    if (ex == 0xff) {
      nRet = ((x.w << 1) == 0xff000000) ? 1 : nRet;
      x.w |= 0x00400000;
      *pres = x.f;
      return nRet;
    }
    ex = ex - 0x7f - 23;
    index = 1 + (ex >> 5);
    // expon % 32
    j = ex & 0x1f;
    // x/Pi, scaled by 2^(63-j)
    ip_low = (((unsigned int)__scos_ha_invpi_tbl[index]) * ((unsigned int)mx));
    IP = (((unsigned long)((unsigned int)(__scos_ha_invpi_tbl[index + 1]))) *
          ((unsigned int)(mx))) +
         (((unsigned long)ip_low) << 32);
    // scaled by 2^(95-j)
    IP2 = (((unsigned long)((unsigned int)(__scos_ha_invpi_tbl[index + 2]))) *
           ((unsigned int)(mx))) +
          ((((unsigned long)((unsigned int)(__scos_ha_invpi_tbl[index + 3]))) *
            ((unsigned int)(mx))) >>
           32);
    IP = IP + (IP2 >> 32);
    // scale 2^63
    IP <<= j;
    // shift low part by 32-j, j in [0,31]
    ip_low = (unsigned int)IP2;
    ip_low >>= (31 - j);
    ip_low >>= 1;
    IP |= (unsigned long)ip_low;
  } else // main path
  {
    // products are really unsigned; operands are small enough so that signed
    // MuL works as well x*(23-ex)*(1/Pi)*2^28 p[k] products fit in 31 bits each
    IP_s = (((long)((int)(mx))) * ((int)(__scos_ha___ip_h)));
    IP = (unsigned long)IP_s;
    IP2_s = (((long)((int)(mx))) * ((int)(__scos_ha___ip_m)));
    IP2 = (unsigned long)IP2_s;
    // scale (23-ex)*2^(28+32+7)
    ip_low_s = (((int)mx) * ((int)__scos_ha___ip_l));
    ip_low = (unsigned int)ip_low_s;
    IP2 = (IP2 << 7) + ip_low;
    // (x/Pi)*2^63
    IP <<= (ex - 0x7f + 12);
    // IP3 = IP2 << (37 -0x7f + ex);
    IP2 >>= (27 + 0x7f - ex);
    IP += IP2;
  }
  // add Pi/2
  IP += 0x4000000000000000uL;
  // return to 32-bit, scale 2^31
  ip_h = IP >> 32;
  // fix sign bit
  sgn_x = ((ip_h + 0x40000000) & 0x80000000);
  // reduced argument (signed, high-low), scale 2^32
  ip_h <<= 1;
  Rh.f = (float)ip_h;
  // reduced argument will need to be normalized
  shift = 1 + 30 + 0x7f - ((Rh.w >> 23) & 0xff);
  // correction for shift=0
  shift = (shift >= 1) ? shift : 1;
  // normalize
  IP <<= shift; // IP = (IP << shift) | (IP3 >> (64-shift));
  ip_h = IP >> 32;
  Rh.f = (float)ip_h;
  ip_h -= ((int)Rh.f);
  Rl.f = (float)ip_h;
  // adjust scale
  scale.w = (0x7f - 31 - shift) << 23;
  Rh.f = __spirv_ocl_fma(Rh.f, scale.f, 0.0f);
  Rl.f = __spirv_ocl_fma(Rl.f, scale.f, 0.0f);
  // (Rh+Rl)^2
  {
    R2h = __spirv_ocl_fma(Rh.f, Rh.f, 0.0f);
    R2l = __spirv_ocl_fma(Rh.f, Rh.f, -R2h);
    R2l = __spirv_ocl_fma(Rl.f, Rh.f, R2l);
    R2l = __spirv_ocl_fma(Rh.f, Rl.f, R2l);
  };
  poly_h = __scos_ha___c5.f;
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __scos_ha___c4.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __scos_ha___c3.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __scos_ha___c2.f);
  poly_l = 0;
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(poly_h, R2h, 0.0f);
    __phl = __spirv_ocl_fma(poly_h, R2h, -__ph);
    poly_l = __spirv_ocl_fma(poly_l, R2h, __phl);
    poly_h = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph =
        __spirv_ocl_fma(poly_h, 1.0f, __scos_ha___c1.f);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__scos_ha___c1.f);
    __ahl = __spirv_ocl_fma(poly_h, 1.0f, -__ahh);
    poly_l = (poly_l + __scos_ha___c1l.f) + __ahl;
    poly_h = __ph;
  };
  {
    float __ph, __phl;
    __ph = __spirv_ocl_fma(poly_h, R2h, 0.0f);
    __phl = __spirv_ocl_fma(poly_h, R2h, -__ph);
    poly_l = __spirv_ocl_fma(poly_l, R2h, __phl);
    poly_l = __spirv_ocl_fma(poly_h, R2l, poly_l);
    poly_h = __ph;
  };
  {
    float __ph, __ahl, __ahh;
    __ph =
        __spirv_ocl_fma(poly_h, 1.0f, __scos_ha___c0.f);
    __ahh =
        __spirv_ocl_fma(__ph, 1.0f, -__scos_ha___c0.f);
    __ahl = __spirv_ocl_fma(poly_h, 1.0f, -__ahh);
    poly_l = (poly_l + __scos_ha___c0l.f) + __ahl;
    poly_h = __ph;
  };
  Low = __spirv_ocl_fma(poly_h, Rl.f, 0.0f);
  Low = __spirv_ocl_fma(poly_l, Rh.f, Low);
  res.f = __spirv_ocl_fma(poly_h, Rh.f, Low);
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_cosf_ha(float x) {
  float r;
  __ocl_svml_internal_scos_ha(&x, &r);
  return r;
}
