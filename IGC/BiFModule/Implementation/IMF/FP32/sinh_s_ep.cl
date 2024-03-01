/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  Compute sinh(x) as (exp(x)-exp(-x))/2,
//  *  where exp is calculated as
//  *  exp(M*ln2 + ln2*(j/2^k) + r) = 2^M * 2^(j/2^k) * exp(r)
//  *
//  *  Special cases:
//  *
//  *  sinh(NaN) = quiet NaN, and raise invalid exception
//  *  sinh(INF) = that INF
//  *  sinh(x)   = x for subnormals
//  *  sinh(x) overflows for big x and returns MAXLOG+log(2)
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  /*Shared*/
  unsigned int _sExp_tbl_PH[32];
  unsigned int _sExp_tbl_PL[32];
  unsigned int _sExp_tbl_NH[32];
  unsigned int _sExp_tbl_NL[32];
  unsigned int _sShifter_UISA;
  unsigned int _sShifterP1_UISA;
  unsigned int _iIndexMask_UISA;
  unsigned int _iDomainRange_UISA;
  unsigned int _sPC1_UISA;
  unsigned int _sPC2_UISA;
  unsigned int _sPC3_UISA;
  unsigned int _sInvLn2;
  unsigned int _sLn2hi;
  unsigned int _sLn2lo;
  unsigned int _sSign;
  unsigned int _sOne;
  /* Shared name but with different value */
  unsigned int _sShifter;
  unsigned int _iIndexMask;
  unsigned int _iDomainRange;
  unsigned int _sPC1;
  unsigned int _sPC2;
  unsigned int _sPC3;
  unsigned int _sPC4;
  unsigned int _iHalf;
} __ocl_svml_internal_ssinh_ep_data_t;
static __ocl_svml_internal_ssinh_ep_data_t __ocl_svml_internal_ssinh_ep_data = {
    {/* _sExp_tbl_PH 2^(i/32-1), i=0..31 */
     0x3f000000u, 0x3f02cd87u, 0x3f05aac3u, 0x3f08980fu, 0x3f0b95c2u,
     0x3f0ea43au, 0x3f11c3d3u, 0x3f14f4f0u, 0x3f1837f0u, 0x3f1b8d3au,
     0x3f1ef532u, 0x3f227043u, 0x3f25fed7u, 0x3f29a15bu, 0x3f2d583fu,
     0x3f3123f6u, 0x3f3504f3u, 0x3f38fbafu, 0x3f3d08a4u, 0x3f412c4du,
     0x3f45672au, 0x3f49b9beu, 0x3f4e248cu, 0x3f52a81eu, 0x3f5744fdu,
     0x3f5bfbb8u, 0x3f60ccdfu, 0x3f65b907u, 0x3f6ac0c7u, 0x3f6fe4bau,
     0x3f75257du, 0x3f7a83b3u},
    {/*  for i in [|0,...,31|] do printsingle( 2^(i/32-1) -
        round(2^(i/32-1),SG,RN)); */
     0x00000000u, 0xb2cea7a9u, 0x32cf9891u, 0xb2feda4bu, 0xb1e0aba1u,
     0xb2e97465u, 0x32e75624u, 0xb2ae0212u, 0x32a31b71u, 0xb28c5563u,
     0x32c12342u, 0x3043125au, 0xb2ac9d5eu, 0xb2962b08u, 0xb1adeaf6u,
     0xb2fc5aa8u, 0x324fe77au, 0x328ec5f7u, 0xb2c14fe8u, 0xb256663eu,
     0x318aa837u, 0xb2f323a2u, 0x31a8fc24u, 0xb2dc1daau, 0xb254a58au,
     0xb2d04a1cu, 0xb19eab59u, 0xb1c41be6u, 0xb1c116deu, 0xb2c8464au,
     0x31a92436u, 0xb2123758u},
    {/* 2^(-i/32-1), i=0..31 */
     0x3f000000u, 0x3efa83b3u, 0x3ef5257du, 0x3eefe4bau, 0x3eeac0c7u,
     0x3ee5b907u, 0x3ee0ccdfu, 0x3edbfbb8u, 0x3ed744fdu, 0x3ed2a81eu,
     0x3ece248cu, 0x3ec9b9beu, 0x3ec5672au, 0x3ec12c4du, 0x3ebd08a4u,
     0x3eb8fbafu, 0x3eb504f3u, 0x3eb123f6u, 0x3ead583fu, 0x3ea9a15bu,
     0x3ea5fed7u, 0x3ea27043u, 0x3e9ef532u, 0x3e9b8d3au, 0x3e9837f0u,
     0x3e94f4f0u, 0x3e91c3d3u, 0x3e8ea43au, 0x3e8b95c2u, 0x3e88980fu,
     0x3e85aac3u, 0x3e82cd87u},
    {/* for i in [|0,...,31|] do printsingle( 2^(-i/32-1) -
        round(2^(-i/32-1),SG,RN)); */
     0x00000000u, 0xb1923758u, 0x31292436u, 0xb248464au, 0xb14116deu,
     0xb1441be6u, 0xb11eab59u, 0xb2504a1cu, 0xb1d4a58au, 0xb25c1daau,
     0x3128fc24u, 0xb27323a2u, 0x310aa837u, 0xb1d6663eu, 0xb2414fe8u,
     0x320ec5f7u, 0x31cfe77au, 0xb27c5aa8u, 0xb12deaf6u, 0xb2162b08u,
     0xb22c9d5eu, 0x2fc3125au, 0x32412342u, 0xb20c5563u, 0x32231b71u,
     0xb22e0212u, 0x32675624u, 0xb2697465u, 0xb160aba1u, 0xb27eda4bu,
     0x324f9891u, 0xb24ea7a9u},
    0x48c00000u, /* 1.5*2^18 _sShifter_UISA */
    0x48c00001u, /* 1.5*2^18 _sShifterP1_UISA */
    0x0000001fu, /* _iIndexMask_UISA */
    0x42aeac4fu, /* _iDomainRange_UISA */
    0x3F800000u, /* _sPC1_UISA */
    0x3f000044u, /* _sPC2_UISA */
    0x3e2aaab5u, /* _sPC3_UISA */
    0x3FB8AA3Bu,
    /* _sInvLn2  */ // k=0
    0x3F317000u,    /* _sLn2hi   */
    0x3805FDF4u,    /* _sLn2lo   */
    0x80000000u,    /* _sSign    */
    0x3f800000u,    /* _sOne     */
    0x4b400000u,    /* _sShifter */
    0x0000001fu,    /* _iIndexMask */
    0x42AEAC4Eu,    /* _iDomainRange */
    0x3f7ffbc9u,    /* _sPC1  */
    0x3efffb2fu,    /* _sPC2  */
    0x3e2ccd2eu,    /* _sPC3  */
    0x3d2ca395u,    /* _sPC4  */
    // Integer constants
    0x3f000000u /* _iHalf*/
};              /*sSinh_Table*/
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_Shifter = {0x4ac000feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_L2E = {0x3FB8AA3Bu};
// log(2) high, low
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_L2H = {0x3f317218u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_L2L = {0xb102E308u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_c5 = {0x3c08ba8bu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_c4 = {0x3d2aec4eu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_c3 = {0x3e2aaa9cu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_c2 = {0x3effffe8u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssinh_ep_c1 = {0x3f800000u};
__attribute__((always_inline)) inline int
__ocl_svml_internal_ssinh_ep(float *a, float *r) {
  int nRet = 0;
  float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(*a);
  union {
    unsigned int w;
    float f;
    int i;
  } S, Th, Tlr, Th2, xin, xa, res;
  float N, R, poly;
  int index_mask;
  float ressign = ((*a) > 0.0f) ? 1.0f : -1.0f;
  S.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(x, __ssinh_ep_L2E.f,
                                               __ssinh_ep_Shifter.f);
  N = S.f - __ssinh_ep_Shifter.f;
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )((-N), __ssinh_ep_L2H.f, x);
  R = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )((-N), __ssinh_ep_L2L.f, R);
  // set exponent in place
  Th.w = S.w << 22;
  // index_mask is based on last bit of S.w
  index_mask = 0 - (S.w & 1);
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  // set Tl/Th value
  Tlr.w = index_mask & 0x329302AEu;
  // polynomial
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, __ssinh_ep_c5.f,
                                                __ssinh_ep_c4.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ep_c3.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ep_c2.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, __ssinh_ep_c1.f);
  poly = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(R, poly, Tlr.f);
  xin.f = x;
  xa.w = xin.w & 0x7fffffffu;
  // redirect special cases
  if (xa.w > 0x42AEAC4Fu)
    goto SINHF_SPECIAL;
  res.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Th.f, Th.f);
  *r = ressign * 0.5f * res.f;
  return nRet;
SINHF_SPECIAL:
  if (xa.w > 0x42b2d4fcu) {
    if (xa.w > 0x7f800000u) { // NaN?
      *r = ressign * (x + x);
      return nRet;
    }
    // overflow
    res.w = 0x7f800000;
    *r = ressign * res.f;
    nRet = 3;
    return nRet;
  }
  S.w += 0xfe;
  Th2.w = (S.w >> 2) & 0xff;
  S.w -= (Th2.w << 1);
  Th2.w <<= 23; // second exponent scale
  Th.w = S.w << 22;
  // set Th mantissa
  Th.w ^= (index_mask & 0x7504F3u);
  res.f = 0.5f * SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly, Th.f, Th.f);
  res.f *= Th2.f;
  *r = ressign * res.f;
  return nRet;
}
float __ocl_svml_sinhf_ep(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float sN;
    float sM;
    float sR;
    float sR2;
    float sSinh_r;
    float sOut;
    float sG1;
    float sG2;
    float sXSign;
    float sAbsX;
    unsigned int iM;
    unsigned int iAbsX;
    unsigned int iRangeMask;
    float sInvLn2;
    float sShifter;
    float sPC[4];
    unsigned int iHalf;
    unsigned int iDomainRange;
    sInvLn2 = as_float(__ocl_svml_internal_ssinh_ep_data._sInvLn2);
    sShifter = as_float(__ocl_svml_internal_ssinh_ep_data._sShifter);
    sPC[0] = as_float(__ocl_svml_internal_ssinh_ep_data._sPC1);
    sPC[1] = as_float(__ocl_svml_internal_ssinh_ep_data._sPC2);
    sPC[2] = as_float(__ocl_svml_internal_ssinh_ep_data._sPC3);
    sPC[3] = as_float(__ocl_svml_internal_ssinh_ep_data._sPC4);
    sXSign = as_float(__ocl_svml_internal_ssinh_ep_data._sSign);
    iHalf = (__ocl_svml_internal_ssinh_ep_data._iHalf);
    iDomainRange = (__ocl_svml_internal_ssinh_ep_data._iDomainRange);
    // Compute argument sign and absolute argument
    sXSign = as_float((as_uint(sXSign) & as_uint(va1)));
    sAbsX = as_float((as_uint(sXSign) ^ as_uint(va1)));
    // dM = x/log(2) + RShifter
    sM = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sAbsX, sInvLn2, sShifter);
    iAbsX = as_uint(sAbsX);
    // Check for overflow\underflow
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iAbsX >
                                               (signed int)iDomainRange)));
    vm = 0;
    vm = iRangeMask;
    // Reduced R:
    // sN = sM - RShifter
    sN = (sM - sShifter);
    sOut = as_float(__ocl_svml_internal_ssinh_ep_data._sLn2hi);
    sG1 = as_float(__ocl_svml_internal_ssinh_ep_data._sLn2lo);
    // sR = sX - sN*Log2_hi
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sOut), sN, sAbsX);
    // sR = (sX - sN*Log2_hi) - sN*Log2_lo
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sG1), sN, sR);
    // Compute G1,G2 2^N,2^(-N):
    iM = as_uint(sM);
    // iM now is an EXP(2^N)
    iM = ((unsigned int)(iM) << (23));
    // sR2 = sR^2
    sR2 = (sR * sR);
    // sG1=2^(N-1)
    iAbsX = (iHalf + iM);
    sG1 = as_float(iAbsX);
    // sG2=2^(-N-1)
    iAbsX = (iHalf - iM);
    sG2 = as_float(iAbsX);
    sM = sG1;
    // sG1 = 2^(N-1)+2^(-N-1)
    sG1 = (sG1 + sG2);
    // sG2 = 2^(N-1)-2^(-N-1)
    sG2 = (sM - sG2);
    // sinh(r) = r*(a1+r^2*a3):
    // sSinh_r = (a1+r^2*a3)
    sSinh_r = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPC[2], sR2, sPC[0]);
    // sinh(X) = sG2 + r*(sG1*sSinh_r + sG2*r*(a2+r2*a4)):
    // sOut = (a2+a4*dR2)
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPC[3], sR2, sPC[1]);
    // sG1*sSinh_r
    sSinh_r = (sSinh_r * sG1);
    // sOut = sG2*(a2+r2*a4)
    sOut = (sOut * sG2);
    // sOut = sG1*sSinh_r + sG2*r*(a2+r2*a4)
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sOut, sR, sSinh_r);
    // sOut = sG2 + r*(sG1*sSinh_r + sG2*r*(a2+r2*a4))
    sOut = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sR, sOut, sG2);
    // Set result sign
    vr1 = as_float((as_uint(sXSign) | as_uint(sOut)));
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_ssinh_ep(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
