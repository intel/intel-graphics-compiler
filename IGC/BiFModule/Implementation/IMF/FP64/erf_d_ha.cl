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
} __derf_ha___c12 = {0x3dd0579ab18bb034uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c11 = {0xbe1386ac0e80f6f6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c10 = {0x3e4f7a41239424f0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c9 = {0xbe85f1d36c342bdcuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c8 = {0x3ebb9def19e10c03uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c7 = {0xbeef4d1df394dad1uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c6 = {0x3f1f9a321a11df20uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c5 = {0xbf4c02db3d8ed259uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c4 = {0x3f7565bcd0db02e5uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c3 = {0xbf9b82ce3128499cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c2 = {0x3fbce2f21a042b20uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c1 = {0xbfd812746b0379e6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___c0 = {0x3fc06eba8214db69uL};
// [.875, 2.5)
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r222 = {0x3e0aea9d809c0cc0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r221 = {0xbe1585e5389852a1uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r220 = {0xbe44e3c7bb4e6baduL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r219 = {0x3e60787bd6c23cdeuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r218 = {0x3e6c4a4f934ec97auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r217 = {0xbe9afbf4b1061a86uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r216 = {0x3e8c8f2572501791uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r215 = {0x3ecb5d5d47f87268uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r214 = {0xbee030574affb335uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r213 = {0xbee9d5c236c6c400uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r212 = {0x3f153fc67d1daecfuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r211 = {0xbf1037f1525a68bfuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r210 = {0xbf3b88dd5886d772uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r29 = {0x3f5319f780b3f6bbuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r28 = {0x3f23a95ae24531a6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r27 = {0xbf79be731fd97edbuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r26 = {0x3f87e8755da3fd73uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r25 = {0x3f66981061df9ac9uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r24 = {0xbfa9647a30b1671duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r23 = {0x3fba3687c1eaf1b3uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r22 = {0xbfbc435059d0978auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r21 = {0x3fb0bf97e95f2a64uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r20 = {0xbf916b24cb8f8f92uL};
// [2.5, 4)
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r319 = {0xbe0a2f1867e76952uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r318 = {0x3e21198ad6cc5141uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r317 = {0x3e2958c6ce203b89uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r316 = {0xbe5daf579fcb0d86uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r315 = {0x3e71dca84cc9234cuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r314 = {0x3e2ec6c1a451cbf6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r313 = {0xbea265e39074a9d0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r312 = {0x3ebff2dd7650fef1uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r311 = {0xbec806fecef280dcuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r310 = {0xbec3892b6d2cf282uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r39 = {0x3efb2c31c4d013eauL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r38 = {0xbf156747a650e49auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r37 = {0x3f2647f722962588uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r36 = {0xbf3145464e6e271fuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r35 = {0x3f3490a4d22a7f1auL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r34 = {0xbf32c7d5ef077ce8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r33 = {0x3f29aa489e3d085buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r32 = {0xbf18de3cd2908194uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r31 = {0x3efe9b5e8d00c879uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r30 = {0xbed20c13035510bauL};
// [4, 6)
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r417 = {0x3d987b4417eaf36duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r416 = {0xbdc085b75720c6a6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r415 = {0x3dd78ecc9429a326uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r414 = {0xbdeb70e185e567c1uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r413 = {0x3dfd4cd2d3b0acb6uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r412 = {0xbe0b6e867ec36d97uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r411 = {0x3e162553731107f0uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r410 = {0xbe1f3c344c6247c2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r49 = {0x3e234d95061910ebuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r48 = {0xbe24bd9ec47f0fbfuL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r47 = {0x3e232d1bda20cff8uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r46 = {0xbe1e231ca9b2d22duL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r45 = {0x3e13c4f1db039226uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r44 = {0xbe0516ddf2b5a8e2uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r43 = {0x3df196e40460a88buL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r42 = {0xbdd589af770a029euL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r41 = {0x3db13af23e58ca63uL};
static __constant union {
  unsigned long w;
  unsigned int w32[2];
  int s32[2];
  double f;
} __derf_ha___r40 = {0xBD7B0C1B6D3B24ADuL};
__attribute__((always_inline)) inline int
__ocl_svml_internal_derf_ha(double *a, double *pres) {
  int nRet = 0;
  double xin = *a;
  union {
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
  } x, xa, res, cpoly;
  unsigned long sgn_x;
  double dR, dR2;
  xa.f = xin;
  sgn_x = xa.w & 0x8000000000000000uL;
  // |xin|
  xa.w ^= sgn_x;
  if (xa.f < 2.5) {
    if (xa.f < .875) {
      dR2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(xa.f, xa.f, 0.0);
      // polynomial evaluation
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derf_ha___c12.f, dR2,
                                                       __derf_ha___c11.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c10.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c9.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c8.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c7.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c6.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c5.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c4.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c3.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c2.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___c1.f);
      res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                     __derf_ha___c0.f);
      res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(res.f, xin, xin);
      *pres = res.f;
      return nRet;
    } else //(0.875 <= xa.f < 2.5)
    {
      dR2 = xa.f - 1.6875;
      // polynomial evaluation
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derf_ha___r222.f, dR2,
                                                       __derf_ha___r221.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r220.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r219.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r218.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r217.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r216.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r215.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r214.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r213.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r212.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r211.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r210.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r29.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r28.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r27.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r26.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r25.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r24.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r23.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r22.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r21.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r20.f);
      res.f = cpoly.f + 1.0;
      res.w ^= sgn_x;
      *pres = res.f;
      return nRet;
    }
  } else // 2.5 <= x
  {
    if (xa.f < 4.0) {
      dR2 = xa.f - 3.25;
      // polynomial evaluation
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derf_ha___r319.f, dR2,
                                                       __derf_ha___r318.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r317.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r316.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r315.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r314.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r313.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r312.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r311.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r310.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r39.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r38.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r37.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r36.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r35.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r34.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r33.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r32.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r31.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r30.f);
      res.f = cpoly.f + 1.0;
      res.w ^= sgn_x;
      *pres = res.f;
      return nRet;
    } else {
      // limit |x| range to [0,6]
      dR = (xa.f > 6.0) ? 6.0 : xa.f;
      dR = (xa.w <= 0x7ff0000000000000uL) ? dR : xa.f;
      dR2 = dR - 5.0;
      // polynomial evaluation
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(__derf_ha___r417.f, dR2,
                                                       __derf_ha___r416.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r415.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r414.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r413.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r412.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r411.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r410.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r49.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r48.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r47.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r46.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r45.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r44.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r43.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r42.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r41.f);
      cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cpoly.f, dR2,
                                                       __derf_ha___r40.f);
      res.f = cpoly.f + 1.0;
      res.w ^= sgn_x;
      *pres = res.f;
      return nRet;
    }
  }
  return nRet;
}
double __ocl_svml_erf_ha(double x) {
  double r;
  __ocl_svml_internal_derf_ha(&x, &r);
  return r;
}
