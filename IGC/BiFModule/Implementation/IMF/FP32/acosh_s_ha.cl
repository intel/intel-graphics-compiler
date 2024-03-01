/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  Compute acosh(x) as log(x + sqrt(x*x - 1))
//  *
//  *  Special cases:
//  *
//  *  acosh(NaN)  = quiet NaN, and raise invalid exception
//  *  acosh(-INF) = NaN
//  *  acosh(+INF) = +INF
//  *  acosh(x)    = NaN if x < 1
//  *  acosh(1)    = +0
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned int Log_HA_table[(1 << 8) + 2];
  unsigned int SgnMask;
  unsigned int XThreshold;
  unsigned int XhMask;
  unsigned int ExpMask0;
  unsigned int ExpMask2;
  unsigned int ha_poly_coeff[2];
  unsigned int ExpMask;
  unsigned int Two10;
  unsigned int MinLog1p;
  unsigned int MaxLog1p;
  unsigned int HalfMask;
  unsigned int L2H;
  unsigned int L2L;
  unsigned int sOne;
  unsigned int sPoly[10];
  unsigned int iHiDelta;
  unsigned int iLoRange;
  unsigned int iBrkValue;
  unsigned int iOffExpoMask;
  unsigned int sBigThreshold;
  unsigned int sC2;
  unsigned int sC3;
  unsigned int sHalf;
  unsigned int sLargestFinite;
  unsigned int sLittleThreshold;
  unsigned int sSign;
  unsigned int sThirtyOne;
  unsigned int sTopMask11;
  unsigned int sTopMask12;
  unsigned int sTopMask8;
  unsigned int XScale;
  unsigned int sLn2Hi;
  unsigned int sLn2Lo;
  /* scalar part follow */
  unsigned int sInfs[2];
  unsigned int sOnes[2];
  unsigned int sZeros[2];
} __ocl_svml_internal_sacosh_ha_data_t;
static __ocl_svml_internal_sacosh_ha_data_t __ocl_svml_internal_sacosh_ha_data =
    {
        /* Log_HA_table */
        {0xc2aeac38u, 0xb93cbf08u, 0xc2aeb034u, 0xb93ce972u, 0xc2aeb424u,
         0xb95e1069u, 0xc2aeb814u, 0xb9412b26u, 0xc2aebbfcu, 0xb9272b41u,
         0xc2aebfd4u, 0xb950fcd7u, 0xc2aec3acu, 0xb93f86b8u, 0xc2aec77cu,
         0xb933aa90u, 0xc2aecb44u, 0xb92e4507u, 0xc2aecf04u, 0xb9302df1u,
         0xc2aed2bcu, 0xb93a3869u, 0xc2aed66cu, 0xb94d32f7u, 0xc2aeda1cu,
         0xb929e7b5u, 0xc2aeddbcu, 0xb9511c6au, 0xc2aee15cu, 0xb94392acu,
         0xc2aee4f4u, 0xb94207fdu, 0xc2aee884u, 0xb94d35eau, 0xc2aeec14u,
         0xb925d225u, 0xc2aeef94u, 0xb94c8ea1u, 0xc2aef314u, 0xb94219adu,
         0xc2aef68cu, 0xb9471e0bu, 0xc2aef9fcu, 0xb95c430bu, 0xc2aefd6cu,
         0xb9422ca0u, 0xc2af00d4u, 0xb9397b7bu, 0xc2af0434u, 0xb942cd1cu,
         0xc2af0794u, 0xb91ebbeau, 0xc2af0ae4u, 0xb94ddf49u, 0xc2af0e34u,
         0xb950cbabu, 0xc2af1184u, 0xb92812a5u, 0xc2af14c4u, 0xb9544303u,
         0xc2af1804u, 0xb955e8d7u, 0xc2af1b44u, 0xb92d8d8du, 0xc2af1e74u,
         0xb95bb7fau, 0xc2af21acu, 0xb920ec71u, 0xc2af24d4u, 0xb93dacccu,
         0xc2af27fcu, 0xb9327882u, 0xc2af2b1cu, 0xb93fccb3u, 0xc2af2e3cu,
         0xb9262434u, 0xc2af3154u, 0xb925f7a4u, 0xc2af3464u, 0xb93fbd72u,
         0xc2af3774u, 0xb933e9f2u, 0xc2af3a7cu, 0xb942ef61u, 0xc2af3d84u,
         0xb92d3dfbu, 0xc2af4084u, 0xb93343ffu, 0xc2af437cu, 0xb9556dbfu,
         0xc2af4674u, 0xb95425adu, 0xc2af496cu, 0xb92fd461u, 0xc2af4c5cu,
         0xb928e0a9u, 0xc2af4f44u, 0xb93faf8eu, 0xc2af522cu, 0xb934a465u,
         0xc2af550cu, 0xb94820d2u, 0xc2af57ecu, 0xb93a84d8u, 0xc2af5ac4u,
         0xb94c2eddu, 0xc2af5d9cu, 0xb93d7bb5u, 0xc2af606cu, 0xb94ec6aeu,
         0xc2af633cu, 0xb9406992u, 0xc2af6604u, 0xb952bcb6u, 0xc2af68ccu,
         0xb94616feu, 0xc2af6b8cu, 0xb95acde8u, 0xc2af6e4cu, 0xb951358fu,
         0xc2af710cu, 0xb929a0b7u, 0xc2af73c4u, 0xb92460d4u, 0xc2af7674u,
         0xb941c60fu, 0xc2af7924u, 0xb9421f4du, 0xc2af7bd4u, 0xb925ba37u,
         0xc2af7e7cu, 0xb92ce340u, 0xc2af811cu, 0xb957e5adu, 0xc2af83c4u,
         0xb9270b99u, 0xc2af865cu, 0xb95a9dfau, 0xc2af88fcu, 0xb932e4acu,
         0xc2af8b94u, 0xb9302671u, 0xc2af8e24u, 0xb952a8fau, 0xc2af90b4u,
         0xb95ab0eeu, 0xc2af9344u, 0xb94881e8u, 0xc2af95ccu, 0xb95c5e87u,
         0xc2af9854u, 0xb9568869u, 0xc2af9adcu, 0xb9374037u, 0xc2af9d5cu,
         0xb93ec5a6u, 0xc2af9fdcu, 0xb92d577du, 0xc2afa254u, 0xb9433399u,
         0xc2afa4ccu, 0xb94096f3u, 0xc2afa744u, 0xb925bda3u, 0xc2afa9b4u,
         0xb932e2e5u, 0xc2afac24u, 0xb928411du, 0xc2afae8cu, 0xb94611dau,
         0xc2afb0f4u, 0xb94c8ddbu, 0xc2afb35cu, 0xb93bed15u, 0xc2afb5bcu,
         0xb95466b2u, 0xc2afb81cu, 0xb9563119u, 0xc2afba7cu, 0xb94181f0u,
         0xc2afbcd4u, 0xb9568e1eu, 0xc2afbf2cu, 0xb95589d1u, 0xc2afc184u,
         0xb93ea881u, 0xc2afc3d4u, 0xb9521cf3u, 0xc2afc624u, 0xb950193bu,
         0xc2afc874u, 0xb938cec0u, 0xc2afcabcu, 0xb94c6e3fu, 0xc2afcd04u,
         0xb94b27d0u, 0xc2afcf4cu, 0xb9352ae6u, 0xc2afd18cu, 0xb94aa653u,
         0xc2afd3ccu, 0xb94bc84cu, 0xc2afd60cu, 0xb938be68u, 0xc2afd844u,
         0xb951b5a9u, 0xc2afda7cu, 0xb956da79u, 0xc2afdcb4u, 0xb94858aeu,
         0xc2afdeecu, 0xb9265b90u, 0xc2afe11cu, 0xb9310dd5u, 0xc2afe34cu,
         0xb92899abu, 0xc2afe574u, 0xb94d28b2u, 0xc2afe7a4u, 0xb91ee407u,
         0xc2afe9c4u, 0xb95df440u, 0xc2afebecu, 0xb94a8170u, 0xc2afee14u,
         0xb924b32au, 0xc2aff034u, 0xb92cb084u, 0xc2aff254u, 0xb922a015u,
         0xc2aff46cu, 0xb946a7fcu, 0xc2aff684u, 0xb958eddfu, 0xc2aff89cu,
         0xb95996edu, 0xc2affab4u, 0xb948c7e3u, 0xc2affcccu, 0xb926a508u,
         0xc2affedcu, 0xb9335235u, 0xc2b000ecu, 0xb92ef2d4u, 0xc2b002f4u,
         0xb959a9e1u, 0xc2b00504u, 0xb93399eeu, 0xc2b0070cu, 0xb93ce522u,
         0xc2b00914u, 0xb935ad3du, 0xc2b00b14u, 0xb95e1399u, 0xc2b00d1cu,
         0xb936392bu, 0xc2b00f1cu, 0xb93e3e84u}
        /*== SgnMask ==*/
        ,
        0x7fffffffu
        /*== XThreshold ==*/
        ,
        0x39800000u
        /*== XhMask ==*/
        ,
        0xffffff00u
        /*== ExpMask0 ==*/
        ,
        0x7f800000u
        /*== ExpMask2 ==*/
        ,
        0x7b000000u
        /*== ha_poly_coeff[2] ==*/
        ,
        {
            // VHEX_BROADCAST( S, 3fE35103 )    /* coeff3 */
            0x3eAAAB39u /* coeff2 */
            ,
            0xbf000036u /* coeff1 */
        }
        /*== ExpMask ==*/
        ,
        0x007fffffu
        /*== Two10 ==*/
        ,
        0x3b800000u
        /*== MinLog1p ==*/
        ,
        0xbf7fffffu
        /*== MaxLog1p ==*/
        ,
        0x7a800000u
        /*== HalfMask ==*/
        ,
        0xffffff00u
        /*== L2H ==*/
        ,
        0x3f317200u
        /*== L2L ==*/
        ,
        0x35bfbe00u
        /*== sOne = SP 1.0 ==*/
        ,
        0x3f800000u
        /*== sPoly[] = SP polynomial ==*/
        ,
        {
            0xbf000000u /* -5.0000000000000000000000000e-01 P0 */
            ,
            0x3eaaaaabu /*  3.3333334326744079589843750e-01 P1 */
            ,
            0xbe7fff87u /* -2.4999819695949554443359375e-01 P2 */
            ,
            0x3e4ccbbfu /*  1.9999597966670989990234375e-01 P3 */
            ,
            0xbe2acc84u /* -1.6679579019546508789062500e-01 P4 */
            ,
            0x3e127a46u /*  1.4304456114768981933593750e-01 P5 */
            ,
            0xbdf9c4feu /* -1.2195776402950286865234375e-01 P6 */
            ,
            0x3ddc3f2au /*  1.0754235088825225830078125e-01 P7 */
            ,
            0xbe038892u /* -1.2845066189765930175781250e-01 P8 */
            ,
            0x3df5e812u /*  1.2007154524326324462890625e-01 P9 */
        }
        /*== iHiDelta = SP 80000000-7f000000 ==*/
        ,
        0x01000000u
        /*== iLoRange = SP 00800000+iHiDelta ==*/
        ,
        0x01800000u
        /*== iBrkValue = SP 2/3 ==*/
        ,
        0x3f2aaaabu
        /*== iOffExpoMask = SP significand mask ==*/
        ,
        0x007fffffu
        /*== sBigThreshold ==*/
        ,
        0x4E800000u
        /*== sC2 ==*/
        ,
        0x3EC00000u
        /*== sC3 ==*/
        ,
        0x3EA00000u
        /*== sHalf ==*/
        ,
        0x3F000000u
        /*== sLargestFinite ==*/
        ,
        0x7F7FFFFFu
        /*== sLittleThreshold ==*/
        ,
        0x3D800000u
        /*== sSign ==*/
        ,
        0x80000000u
        /*== sThirtyOne ==*/
        ,
        0x41F80000u
        /*== sTopMask11 ==*/
        ,
        0xFFFFE000u
        /*== sTopMask12 ==*/
        ,
        0xFFFFF000u
        /*== sTopMask8 ==*/
        ,
        0xFFFF0000u
        /*== XScale ==*/
        ,
        0x30800000u
        /*== sLn2 = SP ln(2) ==*/
        ,
        0x3f317200u,
        0x35bfbe8eu
        /* scalar part follow */
        /*== sInfs = SP infinity, +/- ==*/
        ,
        {0x7f800000u, 0xff800000u}
        /*== sOnes = SP one, +/- ==*/
        ,
        {0x3f800000u, 0xbf800000u}
        /*== sZeros = SP zero +/- ==*/
        ,
        {0x00000000u, 0x80000000u}}; /*sLn_Table*/
static _iml_v2_sp_union_t __sacosh_ha__iml_sacosh_cout_tab[3] = {
    /* Other simple constants */
    0x3F800000, /* ONE  = 1.0f */
    0x00000000, /* ZERO = 0.0f */
    0x7F800000  /* INF  = 0x7f800000 */
};
#pragma float_control(push)
#pragma float_control(precise, on)
// For x = 1 we return +0
// For x = +inf we return +inf
// For NaNs just return a NaN; for others return a NaN and signal invalid.
__attribute__((always_inline)) inline int
__ocl_svml_internal_sacosh_ha(float *a, float *r) {
  int nRet = 0;
  float purex = *a;
  // First deal with NaN inputs: return a NaN and set necessary flags
  if ((((((_iml_v2_sp_union_t *)&purex)->hex[0] >> 23) & 0xFF) == 0xFF) &&
      ((((_iml_v2_sp_union_t *)&purex)->hex[0] & 0x007FFFFF) != 0x0)) {
    (*r) = purex * purex;
    return nRet;
  }
  // For x = +1, return +0
  if (((_iml_v2_sp_union_t *)&(purex))->hex[0] ==
      ((_iml_v2_sp_union_t *)&(((float *)__sacosh_ha__iml_sacosh_cout_tab)[0]))
          ->hex[0]) {
    (*r) = (float)((float *)__sacosh_ha__iml_sacosh_cout_tab)[1];
    return nRet;
  }
  // For x = +infinity, return +infinity.
  if (((_iml_v2_sp_union_t *)&(purex))->hex[0] ==
      ((_iml_v2_sp_union_t *)&(((float *)__sacosh_ha__iml_sacosh_cout_tab)[2]))
          ->hex[0]) {
    (*r) = (float)((float *)__sacosh_ha__iml_sacosh_cout_tab)[2];
    return nRet;
  }
  // Otherwise return NaN and set invalid
  {
    (*r) = (float)(((float *)__sacosh_ha__iml_sacosh_cout_tab)[2] *
                   ((float *)__sacosh_ha__iml_sacosh_cout_tab)[1]);
    nRet = 1;
    return nRet;
  }
}
#pragma float_control(pop)
float __ocl_svml_acoshf_ha(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float SgnMask;
    float FpExponPlus;
    float sU;
    float sUHi;
    float suLo;
    float sV;
    float sVHi;
    float sVLo;
    float sVTmp;
    float sTmp1;
    float sTmp2;
    float sTmp3;
    float sTmp4;
    float sTmp5;
    float sTmp6;
    float sTmp7;
    float sTmp8;
    float sY;
    float sW;
    float sZ;
    float sR;
    float sS;
    float sT;
    float sE;
    float sTopMask8;
    float sTopMask12;
    float sC1;
    float sC2;
    float sC3;
    float sPol1;
    float sPol2;
    float sCorr;
    float sTmpf1;
    float sTmpf2;
    float sTmpf3;
    float sTmpf4;
    float sTmpf5;
    float sTmpf6;
    float sH;
    float sL;
    float XScale;
    float sThirtyOne;
    float sInfinityMask;
    float sTooSmallMask;
    float sSpecialMask;
    unsigned int iSpecialMask;
    float sBigThreshold;
    float sModerateMask;
    float sLargestFinite;
    float ExpMask;
    float Two10;
    float Mantissa;
    float DblRcp;
    unsigned int IExpon;
    float FpExpon;
    float MinLog1p;
    float MaxLog1p;
    float One;
    float R;
    unsigned int Index;
    float THL[2];
    float L2H;
    float L2L;
    float Kh;
    float Kl;
    float poly_coeff[4];
    float dP;
    float Rh;
    float Rl;
    float Rlh;
    float XThreshold;
    float XhMask;
    float XMask;
    float Xabs;
    float X;
    float Xl;
    unsigned int ExpMask0;
    unsigned int ExpMask2;
    unsigned int Expon;
    unsigned int ExpX;
    float FpExpX;
    float DblRcp1;
    float A;
    float B;
    float dR2;
    float sZero;
    // Load constants, always including One = 1
    One = as_float(__ocl_svml_internal_sacosh_ha_data.sOne);
    SgnMask = as_float(__ocl_svml_internal_sacosh_ha_data.SgnMask);
    XThreshold = as_float(__ocl_svml_internal_sacosh_ha_data.XThreshold);
    XhMask = as_float(__ocl_svml_internal_sacosh_ha_data.XhMask);
    sZero = as_float(0);
    sLargestFinite =
        as_float(__ocl_svml_internal_sacosh_ha_data.sLargestFinite);
    sInfinityMask =
        as_float(((unsigned int)(-(signed int)(!(va1 <= sLargestFinite)))));
    sTooSmallMask = as_float(((unsigned int)(-(signed int)(!(va1 > One)))));
    sSpecialMask = as_float((as_uint(sInfinityMask) | as_uint(sTooSmallMask)));
    iSpecialMask = as_uint(sSpecialMask);
    vm = 0;
    vm = iSpecialMask;
    // The following computation can go wrong for very large X, e.g.
    // the X^2 - 1 = U * V can overflow. But for large X we have
    // acosh(X) / log(2 X) - 1 =~= 1/(4 * X^2), so for X >= 2^30
    // we can just later stick X back into the log and tweak up the exponent.
    // Actually we scale X by 2^-30 and tweak the exponent up by 31,
    // to stay in the safe range for the later log computation.
    // Compute a flag now telling us when to do this.
    sBigThreshold = as_float(__ocl_svml_internal_sacosh_ha_data.sBigThreshold);
    sModerateMask =
        as_float(((unsigned int)(-(signed int)(va1 < sBigThreshold))));
    // sU is needed later on
    sU = (va1 - One);
    // dTmp5-dTmp6=rounded(x^2)-1, dTmp4=x^2-rounded(x^2)
    // dTmp5 + (dTmp4-dTmp6) = x^2-1
    sTmp1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(va1, va1, sZero);
    sTmp4 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(va1, va1, -(sTmp1));
    sTmp5 = (sTmp1 - One);
    sTmp6 = (sTmp5 - sTmp1);
    sTmp6 = (sTmp6 + One);
    sTmp7 = (sTmp4 - sTmp6);
    // Finally, express Y + W = U * V accurately where Y has <= 8 bits
    sTopMask8 = as_float(__ocl_svml_internal_sacosh_ha_data.sTopMask8);
    sY = as_float((as_uint(sTmp5) & as_uint(sTopMask8)));
    sTmp8 = (sTmp5 - sY);
    sW = (sTmp8 + sTmp7);
    // Compute R = 1/sqrt(Y + W) * (1 + d)
    // Force R to <= 8 significant bits.
    // This means that R * Y and R^2 * Y are exactly representable.
    sZ = (1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, )(sY));
    sR = as_float((as_uint(sZ) & as_uint(sTopMask8)));
    // Compute S = (Y/sqrt(Y + W)) * (1 + d)
    //     and T = (W/sqrt(Y + W)) * (1 + d)
    //
    // so that S + T = sqrt(Y + W) * (1 + d)
    // S is exact, and the rounding error in T is OK.
    sS = (sY * sR);
    sT = (sW * sR);
    // Compute e = -(2 * d + d^2)
    // The first FMR is exact, and the rounding error in the other is acceptable
    // since d and e are ~ 2^-8
    sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sS), sR, One);
    sE = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-(sT), sR, sE);
    // Now       1 / (1 + d)
    //         = 1 / (1 + (sqrt(1 - e) - 1))
    //         = 1 / sqrt(1 - e)
    //         = 1 + 1/2 * e + 3/8 * e^2 + 5/16 * e^3 + 35/128 * e^4 + ...
    //
    // So compute the first three nonconstant terms of that, so that
    // we have a relative correction (1 + Corr) to apply to S etc.
    //
    // C1 = 1/2
    // C2 = 3/8
    // C3 = 5/16
    sC3 = as_float(__ocl_svml_internal_sacosh_ha_data.sC3);
    sC2 = as_float(__ocl_svml_internal_sacosh_ha_data.sC2);
    sC2 = as_float(__ocl_svml_internal_sacosh_ha_data.sC2);
    sPol2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sC3, sE, sC2);
    sC1 = as_float(__ocl_svml_internal_sacosh_ha_data.sHalf);
    sPol1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPol2, sE, sC1);
    sCorr = (sPol1 * sE);
    // Now get the final argument to the log1p function:
    //
    //     [X + sqrt(X^2 - 1)] - 1
    //   = (X - 1) + sqrt(Y + W)
    //   = U + sqrt(Y + W)
    //   = U + (S + T) * (1 + Corr)
    //   = [U + S] + [T + (S + T) * Corr]
    //
    // The bottom part is computed directly as sTmpf4:
    sTmpf1 = (sS + sT);
    sTmpf2 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sTmpf1, sCorr, sT);
    // The top part uses a compensated summation sTmpf3 + sTmpf5. Note that
    //
    // U / S =~= (X - 1) / sqrt(X^2 - 1) = sqrt[(X - 1) / (X + 1)]
    //       = sqrt(1 - 2 / (X + 1)).
    //
    // As this is certainly <= 1, even taking into account the fact
    // that S is approximate we are OK until relatively large X,
    // and then as usual the well-conditioned log function kicks in.
    // So we can treat S as the dominant component.
    sTmpf3 = (sU + sS);
    sTmpf4 = (sS - sTmpf3);
    sTmpf5 = (sTmpf4 + sU);
    // Now finally accumulate the high and low parts of the
    // argument to log1p with a final compensated summation: H + L
    sH = (sTmpf2 + sTmpf3);
    sTmpf6 = (sTmpf3 - sH);
    sL = (sTmpf6 + sTmpf2);
    // Now we feed into the log1p code, using H in place of _VARG1 and
    // also adding L into Xl.
    // compute 1+x as high, low parts
    A = ((One > sH) ? One : sH);
    B = ((One < sH) ? One : sH);
    Xabs = as_float((as_uint(sH) & as_uint(SgnMask)));
    XMask = as_float(((unsigned int)(-(signed int)(Xabs < XThreshold))));
    XhMask = as_float((as_uint(XhMask) | as_uint(XMask)));
    X = (A + B);
    X = as_float((as_uint(X) & as_uint(XhMask)));
    Xl = (A - X);
    Xl = (Xl + B);
    // Now multiplex to the case X = 2^-30 * input, Xl = sL = 0 in the "big"
    // case.
    XScale = as_float(__ocl_svml_internal_sacosh_ha_data.XScale);
    XScale = (va1 * XScale);
    X = as_float((((~as_uint(sModerateMask)) & as_uint(XScale)) |
                  (as_uint(sModerateMask) & as_uint(X))));
    Xl = as_float((as_uint(Xl) & as_uint(sModerateMask)));
    sL = as_float((as_uint(sL) & as_uint(sModerateMask)));
    // Now resume the main code.
    ExpMask = as_float(__ocl_svml_internal_sacosh_ha_data.ExpMask);
    Two10 = as_float(__ocl_svml_internal_sacosh_ha_data.Two10);
    /* preserve mantissa, set input exponent to 2^(-8) */
    Mantissa = as_float((as_uint(X) & as_uint(ExpMask)));
    Mantissa = as_float((as_uint(Mantissa) | as_uint(Two10)));
    MinLog1p = as_float(__ocl_svml_internal_sacosh_ha_data.MinLog1p);
    MaxLog1p = as_float(__ocl_svml_internal_sacosh_ha_data.MaxLog1p);
    ExpMask0 = (__ocl_svml_internal_sacosh_ha_data.ExpMask0);
    ExpMask2 = (__ocl_svml_internal_sacosh_ha_data.ExpMask2);
    /* reciprocal approximation good to at least 7.9 bits */
    DblRcp = (1.0f / (Mantissa));
    /* exponent of X needed to scale Xl */
    Expon = as_uint(X);
    ExpX = (Expon & ExpMask0);
    /* 2^ (-8-exp(X) ) */
    ExpX = (ExpMask2 - ExpX);
    /* exponent bits */
    IExpon = as_uint(X);
    IExpon = ((unsigned int)(IExpon) >> (23));
    /* round reciprocal to nearest integer, will have 1+7 mantissa bits */
    DblRcp = SPIRV_OCL_BUILTIN(rint, _f32, )(DblRcp);
    /* scale DblRcp */
    FpExpX = as_float(ExpX);
    DblRcp1 = (FpExpX * DblRcp);
    /* biased exponent in DP format */
    FpExpon = ((float)((int)(IExpon)));
    // Add 31 to the exponent in the "large" case to get log(2 * input)
    sThirtyOne = as_float(__ocl_svml_internal_sacosh_ha_data.sThirtyOne);
    FpExponPlus = (FpExpon + sThirtyOne);
    FpExpon = as_float((((~as_uint(sModerateMask)) & as_uint(FpExponPlus)) |
                        (as_uint(sModerateMask) & as_uint(FpExpon))));
    /* argument reduction */
    Rh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(X, DblRcp1, -(One));
    Rl = (Xl * DblRcp1);
    R = (Rh + Rl);
    Rlh = (R - Rh);
    Rl = (Rl - Rlh);
    Rl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sL, DblRcp1, Rl);
    /* prepare table index */
    Index = as_uint(DblRcp);
    /* table lookup */
    Index = ((unsigned int)(Index) >> (23 - 7 - 3));

    THL[0] =
        as_float(((unsigned int *)((char *)(&__ocl_svml_internal_sacosh_ha_data
                                                 .Log_HA_table[0]) -
                                   0x21800))[Index >> 2]);
    THL[1] =
        as_float(((unsigned int *)((char *)(&__ocl_svml_internal_sacosh_ha_data
                                                 .Log_HA_table[0]) -
                                   0x21800))[(Index >> 2) + 1]);

    /* exponent*log(2.0) */
    L2H = as_float(__ocl_svml_internal_sacosh_ha_data.L2H);
    L2L = as_float(__ocl_svml_internal_sacosh_ha_data.L2L);
    Kh = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(FpExpon, L2H, THL[0]);
    Kl = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(FpExpon, L2L, THL[1]);
    /* polynomial */
    // VLOAD_CONST( S, poly_coeff[2], TAB.ha_poly_coeff[0] );
    poly_coeff[1] =
        as_float(__ocl_svml_internal_sacosh_ha_data.ha_poly_coeff[0]);
    poly_coeff[0] =
        as_float(__ocl_svml_internal_sacosh_ha_data.ha_poly_coeff[1]);
    // VQFMA( S, P12, poly_coeff[2], dR, poly_coeff[1] );
    dP =
        SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(poly_coeff[1], R, poly_coeff[0]);
    dR2 = (R * R);
    dP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(dP, dR2, Rl);
    /* reconstruction */
    THL[0] = (Kh + R);
    Rh = (THL[0] - Kh);
    Rl = (R - Rh);
    Kl = (Kl + Rl);
    Kl = (Kl + dP);
    vr1 = (Kl + THL[0]);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_sacosh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
