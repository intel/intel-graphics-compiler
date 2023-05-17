/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp022 = {0x3dd246505f3689e0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp021 = {0xbdf2fda03640b3cduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp020 = {0x3e07c5e45b7bae89uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp019 = {0xbe257e7ca71fa31buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp018 = {0x3e4592e7c3fda873uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp017 = {0xbe63778d35674d2euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp016 = {0x3e80e9deb3f56b61uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp015 = {0xbe9cef11c0c7f533uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp014 = {0x3eb8274b6f06add9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp013 = {0xbed39c76b6915163uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp012 = {0x3eeef08e7eb82100uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp011 = {0xbf07ab1afe5b4a8euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp010 = {0x3f2184fc29bd0f13uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp09 = {0xbf39082246483e26uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp08 = {0x3f5135262eb4ac05uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp07 = {0xbf66af265490f481uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp06 = {0x3f7c8cb958c5052cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp05 = {0xbf910fcf1b5668f7uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp04 = {0x3fa33cad0ef5f11duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp03 = {0xbfb44837f8906dcbuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp02 = {0x3fc3c27283c32cb8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp01 = {0xbfd17c4e3f17c052uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp00 = {0x3fdb5d8780f956b2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp22 = {0xc0ac276c3d1db6c3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp21 = {0x40d4df49bd7ac3f0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp20 = {0xc0ecd227966a1099uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp19 = {0x40f887b9efd9ed0duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp18 = {0xc0fcaca638f55e37uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp17 = {0x40f8374d75394b22uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp16 = {0xc0ee28054df30e06uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp15 = {0x40db9213cfc9a317uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp14 = {0xc0c1e1dc3c0770c5uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp13 = {0x409e566f466330e8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp12 = {0xc06ea09aa733280auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp11 = {0x4046671557785e22uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp10 = {0xc03530fb954cc684uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp9 = {0x3fd5a70649b7fce4uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp8 = {0x400d7acca9d7915cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp7 = {0x3f41522846bb5367uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp6 = {0xbff0ecfb04e94f14uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp5 = {0xbea5ed7d5f3b4beduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp4 = {0x3fdb14c316dcd6c9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp3 = {0xbe036213f47795a8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp2 = {0xbfd20dd750412359uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp1 = {0xbd15a264a54b1e56uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp0 = {0x3fe20dd750429b6duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___cp0_l = {0x3c75E79ED14C7A19uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___Shifter = {0x43380000000007feuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___L2E = {0x3ff71547652B82FEuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___L2H = {0x3fe62E42FEFA39EFuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___L2L = {0x3c7ABC9E3B39803FuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___Smax = {0x43380000000007ffuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___Inf = {0x7ff0000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce11 = {0x3e5af8da0090dd28uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce10 = {0x3e928b4062a7f01fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce9 = {0x3ec71ddd95f14ecauL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce8 = {0x3efa01991abb9723uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce7 = {0x3f2a01a01bec1aceuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce6 = {0x3f56c16c187fc36auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce5 = {0x3f8111111110c4c4uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce4 = {0x3fa555555554f0cduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce3 = {0x3fc555555555556auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce2 = {0x3fe0000000000011uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce1 = {0x3ff0000000000000uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derfc_la___ce0 = {0x3ff0000000000000uL};

__attribute__((always_inline))
static inline int __ocl_svml_internal_derfc(double *px, double *pres)
{
  int nRet = 0;
  double xin = *px;
  int_double xa, res, apoly, poly, S, Te, Te2, N, two;
  long sgn_x;
  double R, x2h, mx2l, eps;
  float rf;
  xa.f = xin;
  sgn_x = xa.w & 0x8000000000000000uL;
  // |xin|
  xa.w ^= sgn_x;
  if (xa.f > 2.0) {
    // R = 1.0 / xa.f;
    // limit input range
    xa.f = SPIRV_OCL_BUILTIN(fmin, _f64_f64, )(xa.f, 27.5);
    rf = (float)xa.f;
    R = (double)1.0f / (rf);
    eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(xa.f, -R, 1.0);
    // eps + eps^2
    eps = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(eps, eps, eps);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(R, eps, R);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derfc_la___cp22.f, R,
                                                     __derfc_la___cp21.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp20.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp19.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp18.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp17.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp16.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp15.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp14.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp13.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp12.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp11.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp10.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp9.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp8.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp7.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp6.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp5.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp4.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp3.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp2.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp1.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp0_l.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        R, __derfc_la___cp0.f,
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, 0.0));
  } else {
    R = xa.f - 1.0;
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derfc_la___cp022.f, R,
                                                     __derfc_la___cp021.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp020.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp019.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp018.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp017.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp016.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp015.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp014.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp013.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp012.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp011.f);
    apoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R,
                                                     __derfc_la___cp010.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp09.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp08.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp07.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp06.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp05.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp04.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp03.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp02.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp01.f);
    apoly.f =
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(apoly.f, R, __derfc_la___cp00.f);
    res.f = apoly.f;
  }
  // exp calculation
  x2h = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(xa.f, -xa.f, 0.0);
  mx2l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(xa.f, xa.f, x2h);
  // x2h*L2E + Shifter
  S.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x2h, __derfc_la___L2E.f,
                                               __derfc_la___Shifter.f);
  // (int)(x2h*L2E)
  N.f = S.f - __derfc_la___Shifter.f;
  // x^2 - N*log(2)
  R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-N.f, __derfc_la___L2H.f, x2h);
  R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-N.f, __derfc_la___L2L.f, R);
  R = R - mx2l;
  // 2^(N)
  Te2.w = S.w & 1;
  Te.w = (S.w ^ Te2.w) << 51;
  Te2.w = (Te2.w << 52) + Te.w;
  // exp(R)-1
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derfc_la___ce11.f, R,
                                                  __derfc_la___ce10.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce9.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce8.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce7.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce6.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce5.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce4.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce3.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce2.f);
  poly.f =
      SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, __derfc_la___ce1.f);
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, R, 0.0);
  // poly*apoly*Te
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, apoly.f, 0.0);
  poly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(poly.f, Te.f, 0.0);
  // Te*apoly + (poly*apoly*Te)
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Te.f, apoly.f, poly.f);
  // additional scaling (to treat underflow cases)
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(res.f, Te2.f, 0.0);
  // apply sign
  res.w ^= sgn_x;
  // corection based on sign
  sgn_x = ((long)sgn_x) >> 63;
  two.w = sgn_x & 0x4000000000000000uL;
  res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(res.f, 1.0, two.f);
  nRet = (res.w < 0x001000000000uL) ? 4 : nRet;
  *pres = res.f;
  return nRet;
}

double __ocl_svml_erfc_noLUT(double x)
{
  double r;
  __ocl_svml_internal_derfc(&x, &r);
  return r;
}
