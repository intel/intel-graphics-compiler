/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//  *
//  *  exp10(x)  = 2^x/log10(2) = 2^n * (1 + T[j]) * (1 + P(y))
//  *  where
//  *       x = m*log10(2)/K + y,  y in [-log10(2)/K..log10(2)/K]
//  *       m = n*K + j,           m,n,j - signed integer, j in [-K/2..K/2]
//  *
//  *       values of 2^j/K are tabulated
//  *
//  *       P(y) is a minimax polynomial approximation of exp10(x)-1
//  *       on small interval [-log10(2)/K..log10(2)/K]
//  *
//  * Special cases:
//  *
//  *  exp10(NaN)  = NaN
//  *  exp10(+INF) = +INF
//  *  exp10(-INF) = 0
//  *  exp10(x)    = 1 for subnormals
//  *  For IEEE double
//  *    if x >  3.39782712893383973096e+02 then exp10(x) overflow
//  *    if x < -3.45133219101941108420e+02 then exp10(x) underflow
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned long _dLg2_10;
  unsigned long _dbShifter;
  unsigned long _dPC0;
  unsigned long _dPC1;
  unsigned long _dPC2;
  unsigned long _dPC3;
  unsigned long _dPC4;
  unsigned long _dPC5;
  unsigned long _dPC6;
  // common
  unsigned int _iAbsMask;
  unsigned int _iDomainRange;
} __ocl_svml_internal_dexp10_ep_data_t;
static __ocl_svml_internal_dexp10_ep_data_t __ocl_svml_internal_dexp10_ep_data =
    {
        0x400a934f0979a371uL, /* _dLg2_10 */
        0x4338800000000000uL, /* _dbShifter */
        // poly=2^x ,[-0.5,0.5];//its slightly wider because of rounding of
        // {x*log2(10)} fix leading coefficient to be 1.0
        0x3ff0000000000000uL, /* _dPC0 */
        0x3fe62e4302eeb44duL, /* _dPC1 */
        0x3fcebfbdec29c82auL, /* _dPC2 */
        0x3fac6af6e92be375uL, /* _dPC3 */
        0x3f83b2b9fbd8970fuL, /* _dPC4 */
        0x3f55f07b4611e454uL, /* _dPC5 */
        0x3f24308fabc18decuL, /* _dPC6 */
        // common
        0x7fffffffu, /* _iAbsMask */
        0x40733a70u  /* _iDomainRange */
};                   /*dExp10_Table*/
static __constant _iml_v2_dp_union_t __dexp10_ep__imldExp10Tab[147] = {
    0x00000000, 0x3FF00000, /* T[0] = 1                       */
    0x00000000, 0x00000000, /* D[0] = 0                       */
    0x3E778061, 0x3FF02C9A, /* T[1] = 1.010889286051700475255 */
    0x9CD8DC5D, 0xBC716013, /* D[1] = -1.5070669769260387e-17 */
    0xD3158574, 0x3FF059B0, /* T[2] = 1.021897148654116627142 */
    0x3567F613, 0x3C8CD252, /* D[2] = 4.99974487227263278e-17 */
    0x18759BC8, 0x3FF08745, /* T[3] = 1.033024879021228414899 */
    0x61E6C861, 0x3C60F74E, /* D[3] = 7.35784687124741889e-18 */
    0x6CF9890F, 0x3FF0B558, /* T[4] = 1.044273782427413754803 */
    0x5D837B6D, 0x3C979AA6, /* D[4] = 8.18931763819551438e-17 */
    0x32D3D1A2, 0x3FF0E3EC, /* T[5] = 1.055645178360557157049 */
    0x702F9CD1, 0x3C3EBE3D, /* D[5] = 1.66658814423267471e-18 */
    0xD0125B51, 0x3FF11301, /* T[6] = 1.067140400676823697168 */
    0x2A2FBD0E, 0xBC955652, /* D[6] = -7.4028253094261770e-17 */
    0xAEA92DE0, 0x3FF1429A, /* T[7] = 1.078760797757119860307 */
    0xB9D5F416, 0xBC91C923, /* D[7] = -6.1706547456086947e-17 */
    0x3C7D517B, 0x3FF172B8, /* T[8] = 1.090507732665257689675 */
    0xEAA59348, 0xBC801B15, /* D[8] = -2.7939114859515731e-17 */
    0xEB6FCB75, 0x3FF1A35B, /* T[9] = 1.102382583307840890896 */
    0x3F1353BF, 0x3C8B898C, /* D[9] = 4.77695942525622342e-17 */
    0x3168B9AA, 0x3FF1D487, /* T[10] = 1.114386742595892432206 */
    0x3E3A2F60, 0x3C9AECF7, /* D[10] = 9.34171060990504538e-17 */
    0x88628CD6, 0x3FF2063B, /* T[11] = 1.126521618608241848136 */
    0x44A6C38D, 0x3C8A6F41, /* D[11] = 4.58567032666235091e-17 */
    0x6E756238, 0x3FF2387A, /* T[12] = 1.138788634756691564576 */
    0xE3A8A894, 0x3C968EFD, /* D[12] = 7.82657325863607593e-17 */
    0x65E27CDD, 0x3FF26B45, /* T[13] = 1.151189229952982673311 */
    0x981FE7F2, 0x3C80472B, /* D[13] = 2.82378442595106114e-17 */
    0xF51FDEE1, 0x3FF29E9D, /* T[14] = 1.163724858777577475522 */
    0x6D09AB31, 0x3C82F7E1, /* D[14] = 3.29047266460084171e-17 */
    0xA6E4030B, 0x3FF2D285, /* T[15] = 1.176396991650281220743 */
    0x720C0AB4, 0x3C8B3782, /* D[15] = 4.72136812117012784e-17 */
    0x0A31B715, 0x3FF306FE, /* T[16] = 1.189207115002721026897 */
    0x4DB0ABB6, 0x3C834D75, /* D[16] = 3.34846233362515239e-17 */
    0xB26416FF, 0x3FF33C08, /* T[17] = 1.202156731452703075647 */
    0x5DD3F84A, 0x3C8FDD39, /* D[17] = 5.52755004850524931e-17 */
    0x373AA9CB, 0x3FF371A7, /* T[18] = 1.215247359980468955243 */
    0xCC4B5068, 0xBC924AED, /* D[18] = -6.3465521067294830e-17 */
    0x34E59FF7, 0x3FF3A7DB, /* T[19] = 1.228480536106870024682 */
    0x3E9436D2, 0xBC71D1E8, /* D[19] = -1.5456342819397734e-17 */
    0x4C123422, 0x3FF3DEA6, /* T[20] = 1.241857812073484002013 */
    0xA72A4C6D, 0x3C859F48, /* D[20] = 3.75085420130312695e-17 */
    0x21F72E2A, 0x3FF4160A, /* T[21] = 1.255380757024691096291 */
    0x4817895B, 0xBC58A78F, /* D[21] = -5.3460990091987506e-18 */
    0x6061892D, 0x3FF44E08, /* T[22] = 1.269050957191733219886 */
    0x60C2AC11, 0x3C4363ED, /* D[22] = 2.10230496752157159e-18 */
    0xB5C13CD0, 0x3FF486A2, /* T[23] = 1.282870016078778263591 */
    0xDAA10379, 0x3C6ECCE1, /* D[23] = 1.33575100888345409e-17 */
    0xD5362A27, 0x3FF4BFDA, /* T[24] = 1.296839554651009640551 */
    0xBB7AAFB0, 0x3C7690CE, /* D[24] = 1.95725852931120371e-17 */
    0x769D2CA7, 0x3FF4F9B2, /* T[25] = 1.310961211524764413738 */
    0x0071A38E, 0xBC8F9434, /* D[25] = -5.4780691239267781e-17 */
    0x569D4F82, 0x3FF5342B, /* T[26] = 1.325236643159741323217 */
    0xBD0F385F, 0xBC78DEC6, /* D[26] = -2.1571477251208754e-17 */
    0x36B527DA, 0x3FF56F47, /* T[27] = 1.339667524053302916087 */
    0x18FDD78E, 0x3C933505, /* D[27] = 6.66380458923219497e-17 */
    0xDD485429, 0x3FF5AB07, /* T[28] = 1.354255546936892651289 */
    0xE21C5409, 0x3C9063E1, /* D[28] = 5.68648095791174040e-17 */
    0x15AD2148, 0x3FF5E76F, /* T[29] = 1.369002422974590515992 */
    0x2B64C035, 0x3C9432E6, /* D[29] = 7.00787504690699497e-17 */
    0xB03A5585, 0x3FF6247E, /* T[30] = 1.383909881963832022578 */
    0x3BEF4DA8, 0xBC8C33C5, /* D[30] = -4.8923067513522756e-17 */
    0x82552225, 0x3FF66238, /* T[31] = 1.398979672538311236352 */
    0x78565858, 0xBC93CEDD, /* D[31] = -6.8723037209020178e-17 */
    0x667F3BCD, 0x3FF6A09E, /* T[32] = 1.414213562373095145475 */
    0xBF5E2228, 0xBC93B3EF, /* D[32] = -6.8358086576619225e-17 */
    0x3C651A2F, 0x3FF6DFB2, /* T[33] = 1.429613338391970023267 */
    0xB86DA9EE, 0xBC6367EF, /* D[33] = -8.4160116347171566e-18 */
    0xE8EC5F74, 0x3FF71F75, /* T[34] = 1.445180806977046650275 */
    0x7E5A3ECF, 0xBC781F64, /* D[34] = -2.0923043818433529e-17 */
    0x564267C9, 0x3FF75FEB, /* T[35] = 1.460917794180647044655 */
    0x1E55E68A, 0xBC861932, /* D[35] = -3.8334649686542952e-17 */
    0x73EB0187, 0x3FF7A114, /* T[36] = 1.476826145939499346227 */
    0xB94DA51D, 0xBC7B32DC, /* D[36] = -2.3591094770850052e-17 */
    0x36CF4E62, 0x3FF7E2F3, /* T[37] = 1.492907728291264835008 */
    0xABD66C55, 0x3C65EBE1, /* D[37] = 9.50689710108795902e-18 */
    0x994CCE13, 0x3FF82589, /* T[38] = 1.509164427593422841412 */
    0xF13B3734, 0xBC9369B6, /* D[38] = -6.7352192323746824e-17 */
    0x9B4492ED, 0x3FF868D9, /* T[39] = 1.525598150744538417101 */
    0xD872576E, 0xBC94D450, /* D[39] = -7.2266354721012565e-17 */
    0x422AA0DB, 0x3FF8ACE5, /* T[40] = 1.542210825407940744114 */
    0xC1F0EAB4, 0x3C8DB72F, /* D[40] = 5.15483011707867861e-17 */
    0x99157736, 0x3FF8F1AE, /* T[41] = 1.559004400237836929222 */
    0x59F35F44, 0x3C7BF683, /* D[41] = 2.42539857666898064e-17 */
    0xB0CDC5E5, 0x3FF93737, /* T[42] = 1.575980845107886496592 */
    0x8B6C1E29, 0xBC5DA9B8, /* D[42] = -6.4321317754241888e-18 */
    0x9FDE4E50, 0x3FF97D82, /* T[43] = 1.593142151342266998881 */
    0x22F4F9AA, 0xBC924343, /* D[43] = -6.3361618634012930e-17 */
    0x82A3F090, 0x3FF9C491, /* T[44] = 1.610490331949254283472 */
    0x2B91CE27, 0x3C71AFFC, /* D[44] = 1.53414100536037243e-17 */
    0x7B5DE565, 0x3FFA0C66, /* T[45] = 1.628027421857347833978 */
    0x22622263, 0xBC87C504, /* D[45] = -4.1233673306611485e-17 */
    0xB23E255D, 0x3FFA5503, /* T[46] = 1.645755478153964945776 */
    0xD3BCBB15, 0xBC91BBD1, /* D[46] = -6.1526028915502644e-17 */
    0x5579FDBF, 0x3FFA9E6B, /* T[47] = 1.663676580326736376136 */
    0x6E735AB3, 0x3C846984, /* D[47] = 3.54094826264618289e-17 */
    0x995AD3AD, 0x3FFAE89F, /* T[48] = 1.681792830507429004072 */
    0x92CB3387, 0x3C8C1A77, /* D[48] = 4.87516052622706162e-17 */
    0xB84F15FB, 0x3FFB33A2, /* T[49] = 1.700106353718523477525 */
    0x56DCAEBA, 0xBC55C3D9, /* D[49] = -4.7195396645909715e-18 */
    0xF2FB5E47, 0x3FFB7F76, /* T[50] = 1.718619298122477934143 */
    0x38AD9334, 0xBC68D6F4, /* D[50] = -1.0772487078934056e-17 */
    0x904BC1D2, 0x3FFBCC1E, /* T[51] = 1.73733383527370621735  */
    0x0A5FDDCD, 0x3C74FFD7, /* D[51] = 1.82140544036225899e-17 */
    0xDD85529C, 0x3FFC199B, /* T[52] = 1.756252160373299453511 */
    0x30AF0CB3, 0x3C736EAE, /* D[52] = 1.68548729062897320e-17 */
    0x2E57D14B, 0x3FFC67F1, /* T[53] = 1.775376492526521188253 */
    0xD10959AC, 0x3C84E08F, /* D[53] = 3.62161593533689428e-17 */
    0xDCEF9069, 0x3FFCB720, /* T[54] = 1.7947090750031071682   */
    0x6C921968, 0x3C676B2C, /* D[54] = 1.01562190116415002e-17 */
    0x4A07897C, 0x3FFD072D, /* T[55] = 1.814252175500398855945 */
    0x3FFFFA6F, 0xBC8FAD5D, /* D[55] = -5.4951189661220046e-17 */
    0xDCFBA487, 0x3FFD5818, /* T[56] = 1.834008086409342430656 */
    0xA63D07A7, 0x3C74A385, /* D[56] = 1.79012690760451328e-17 */
    0x03DB3285, 0x3FFDA9E6, /* T[57] = 1.853979125083385470774 */
    0xD5C192AC, 0x3C8E5A50, /* D[57] = 5.26537076855627411e-17 */
    0x337B9B5F, 0x3FFDFC97, /* T[58] = 1.874167634110299962558 */
    0x07B43E1F, 0xBC82D521, /* D[58] = -3.2669241009013180e-17 */
    0xE78B3FF6, 0x3FFE502E, /* T[59] = 1.894575981586965607306 */
    0x603A88D3, 0x3C74B604, /* D[59] = 1.79639326598330224e-17 */
    0xA2A490DA, 0x3FFEA4AF, /* T[60] = 1.915206561397147400072 */
    0x8FD391F0, 0xBC8FF712, /* D[60] = -5.5450656186394270e-17 */
    0xEE615A27, 0x3FFEFA1B, /* T[61] = 1.936061793492294347274 */
    0x41AA2008, 0x3C8EC3BC, /* D[61] = 5.33680587851415081e-17 */
    0x5B6E4540, 0x3FFF5076, /* T[62] = 1.957144124175400179411 */
    0x31D185EE, 0x3C8A64A9, /* D[62] = 4.57849152770600937e-17 */
    0x819E90D8, 0x3FFFA7C1, /* T[63] = 1.978456026387950927869 */
    0x4D91CD9D, 0x3C77893B, /* D[63] = 2.04142788975783020e-17 */
    /* TWO_TO_THE_K_DIV_LN2 = 2^6*log2(10) rounded to double */
    0x0979a371, 0x406a934f, /* 2^6*log2(10) */
    /* Right Shifter */
    0x00000000, 0x43380000, /* RS = 2^52 + 2^51 */
    /* Coefficients for exp10(R) - 1 approximation by polynomial p(R) */
    0xbbb55515, 0x40026bb1, /* A1 */
    0xc73cd20a, 0x40053524, /* _A2 */
    0x91e2bc10, 0x40004705, /* _A3 */
    0xb840f0bf, 0x3ff2bd77, /* A4 */
    0x87c70a85, 0x3fe1427c, /* A5 */
    /* Overflow and Underflow Thresholds */
    0x509f79fe, 0x40734413, /* OVF = 308.2547155599166899 */
    0x46f72a41, 0xc0733a71, /* UDF1 = -307.65265556858878160 */
    0x46e36b52, 0xc07439b7, /* UDF2 = -323.306215343115803659*/
    /* Two parts of 1/(log2(10)*64)*/
    0x509f0000, 0x3f734413, /* LOG_HI = .010830424696223 */
    0xc47c4acd, 0x3d1e7fbc, /* LOG_LO = 2.572804622327669e-14 */
    /* TINY and HUGE_VALUE values to process over- underflow */
    0x00000001, 0x00100000, 0xFFFFFFFF, 0x7FEFFFFF,
    /* Double precision constants: 0.0, 1.0, 2.0 */
    0x00000000, 0x00000000, 0x00000000, 0x3FF00000, 0x00000000, 0x40000000,
    /* UNSCALE multiplier for gradual underflow case */
    0x00000000, 0x3C300000, /* 2^(-60) */
    /* TWO_32H = 2^32 + 2^31 */
    0x00000000, 0x41F80000};
__attribute__((always_inline)) inline int
__ocl_svml_internal_dexp10_ep(double *a, double *r) {
  int nRet = 0;
  double w, Nj;
  double udfResHi, udfResLo;
  double udfScaledResHi, udfScaledResLo, tmp1, tmp2;
  double R, RHi, RLo, p, scale, tmp3, dbIn;
  _iml_uint32_t N, j;
  //_IML_DCOREFN_PROLOG2_IN_C(0, _IML_MODEX87_NEAR53_IN_C, _MODE_UNCHANGED, uf,
  //pA, pRes)
  /* Set all bits of scale to 0.                                           */
  /* Only bits of exponent field will be updated then before each use of   */
  /* scale. Notice, that all other bits (i.e. sign and significand) should */
  /* be kept 0 across iterations. Otherwise, they should be explicitly set */
  /* to 0 before each use of scale                                         */
  scale = ((__constant double *)__dexp10_ep__imldExp10Tab)[142];
  dbIn = (*a);
  /* Filter out INFs and NaNs */
  if ((((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 20) & 0x7FF) !=
       0x7FF)) {
    /* Here if argument is finite double precision number */
    /* Check if dbIn falls into "Near 0" range */
    if (((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 20) & 0x7FF) >
        (0x3FF - 53)) {
      /* Here if argument is finite double precision number */
      /* and doesn't fall into "Near 0" range               */
      if (dbIn <= ((__constant double *)__dexp10_ep__imldExp10Tab)[135]) {
        /* Here if argument is finite and exp doesn't overflow */
        if (dbIn >= ((__constant double *)__dexp10_ep__imldExp10Tab)[137]) {
          /* Here if argument is finite and exp doesn't    */
          /* oferflow and completely undeflow. But gradual */
          /* underflow is still possible: exp still may    */
          /* produce denormalized result. Thus, here if    */
          /* 6.* paths                                     */
          /* Range Reduction part */
          w = dbIn * ((__constant double *)__dexp10_ep__imldExp10Tab)[128];
          Nj = (w + ((__constant double *)__dexp10_ep__imldExp10Tab)[129]);
          w = (Nj - ((__constant double *)__dexp10_ep__imldExp10Tab)[129]);
          RHi = (w * ((__constant double *)__dexp10_ep__imldExp10Tab)[138]);
          RLo = (w * ((__constant double *)__dexp10_ep__imldExp10Tab)[139]);
          R = (dbIn - RHi);
          R = (R - RLo);
          /* Approximation part: polynomial series */
          p = (((((((__constant double *)__dexp10_ep__imldExp10Tab)[134] * R +
                   ((__constant double *)__dexp10_ep__imldExp10Tab)[133]) *
                      R +
                  ((__constant double *)__dexp10_ep__imldExp10Tab)[132]) *
                     R +
                 ((__constant double *)__dexp10_ep__imldExp10Tab)[131]) *
                    R +
                ((__constant double *)__dexp10_ep__imldExp10Tab)[130]) *
               R);
          /* Final reconstruction starts here */
          /* get N and j from Nj's significand */
          N = (((_iml_v2_dp_union_t *)&Nj)->dwords.lo_dword);
          j = N & ((1 << 6) - 1);
          N = N >> 6;
          N = N + 0x3FF;
          /* T[j] * ( D[j] + p) */
          p = (p +
               ((__constant double *)__dexp10_ep__imldExp10Tab)[2 * j + 1]) *
              ((__constant double *)__dexp10_ep__imldExp10Tab)[2 * j];
          if (dbIn >= ((__constant double *)__dexp10_ep__imldExp10Tab)[136]) {
            /* Here if exp(dbIn) is always finite and */
            /* normalized. Paths 6.2 and 6.3          */
            /* (T[j] + T[j] * ( D[j] + p)) */
            p = (p + ((__constant double *)__dexp10_ep__imldExp10Tab)[2 * j]);
            N = N & 0x7FF;
            /* Check if path 6.3 or 6.2 should follow */
            if (N <= (0x7FF - 1)) {
              /* Path 6.3 */
              /* scale = 2^N */
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N)&0x7FF) << 20));
              /* scale * (T[j] + T[j] * ( D[j] + p)) */
              (*r) = (scale * p);
            } else {
              /* Path 6.2: Case "scale overflow" */
              /* scale = 2^(N - 1) */
              (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(N - 1) & 0x7FF) << 20));
              /* 2.0*(scale * (T[j] + T[j] * ( D[j] + p))) */
              p = (p * scale);
              (*r) =
                  (((__constant double *)__dexp10_ep__imldExp10Tab)[144] * p);
            }
          } else //(dbIn < UDF1)
          {
            /* Here if exp gradually underflows */
            N = (N + 60);
            N = N & 0x7FF;
            /* scale = 2^(N + UNSCALE_SHIFT_EXP) */
            (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&scale)->dwords.hi_dword & 0x800FFFFF) |
                 (((_iml_uint32_t)(N)&0x7FF) << 20));
            /* 2^(N + UNSCALE_SHIFT_EXP) * (T[j]*(D[j] + p) */
            p *= scale;
            /* (2^(N + UNSCALE_SHIFT_EXP) * T[j]) */
            tmp3 = (scale *
                    ((__constant double *)__dexp10_ep__imldExp10Tab)[2 * j]);
            /* Check if path 6.1.1 or 6.1.2 should follow */
            if (N <= (60 - 10)) {
              /* Here if "small" denormalized result */
              /* path 6.1.1 */
              p = (p + tmp3);
              (*r) =
                  (p * ((__constant double *)__dexp10_ep__imldExp10Tab)[145]);
            } else {
              /* Here if "large" denormalized result */
              /* path 6.1.2 */
              /* Get high and low parts of scaled answer */
              udfScaledResHi = (p + tmp3);
              udfScaledResLo = (tmp3 - udfScaledResHi);
              udfScaledResLo += p;
              /* udfResHi is udfScaledResHi rounded to */
              /* 32 most significant bits of mantissa  */
              tmp1 = (((__constant double *)__dexp10_ep__imldExp10Tab)[146] *
                      udfScaledResHi);
              tmp2 = (udfScaledResHi + tmp1);
              udfResHi = (tmp2 - tmp1);
              /* udfResLo = (udfScaledResHi - udfResHi) + */
              /*            + udfScaledResLo              */
              udfResLo = (udfScaledResHi - udfResHi);
              udfResLo += udfScaledResLo;
              /* Return (*r) = (UNSCALE * udfResHi) +  */
              /*               + (UNSCALE * udfResLo)  */
              udfResHi =
                  (udfResHi *
                   ((__constant double *)__dexp10_ep__imldExp10Tab)[145]);
              udfResLo =
                  (udfResLo *
                   ((__constant double *)__dexp10_ep__imldExp10Tab)[145]);
              (*r) = (udfResHi + udfResLo);
            }
            nRet = 4;
          }
        } else {
          /* Here if argument is finite but exp completely  */
          /* underflows in round to nearest mode and should */
          /* be rounded 0.0                                 */
          (*r) = ((__constant double *)__dexp10_ep__imldExp10Tab)[140] *
                 ((__constant double *)__dexp10_ep__imldExp10Tab)[140];
          nRet = 4;
        }
      } else {
        /* Here if exp overflows */
        (*r) = ((__constant double *)__dexp10_ep__imldExp10Tab)[141] *
               ((__constant double *)__dexp10_ep__imldExp10Tab)[141];
        nRet = 3;
      }
    } else {
      /* "Near 0" path */
      (*r) = ((__constant double *)__dexp10_ep__imldExp10Tab)[143] + dbIn;
    }
  } else {
    /* Here if argument is NaN or Infinity */
    if (((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword >> 31) == 1) &&
        (((((_iml_v2_dp_union_t *)&dbIn)->dwords.hi_dword & 0x000FFFFF) == 0) &&
         ((((_iml_v2_dp_union_t *)&dbIn)->dwords.lo_dword) == 0))) {
      /* Here if argument is negative infinity */
      (*r) = ((__constant double *)__dexp10_ep__imldExp10Tab)[142];
    } else {
      /* Here if argument is positive infinity or NaN */
      (*r) = dbIn * dbIn;
    }
  }
  return nRet;
}
double __ocl_svml_exp10_ep(double x) {
  double r;
  unsigned int vm;
  double va1;
  double vr1;
  va1 = x;
  {
    double dN;
    double dM;
    double dR;
    double dZ;
    unsigned long lM;
    unsigned long lX;
    unsigned int iRangeMask;
    unsigned int iAbsX;
    double dLg2_10;
    double dbShifter;
    double dPC[7];
    unsigned int iAbsMask;
    unsigned int iDomainRange;
    dLg2_10 = as_double(__ocl_svml_internal_dexp10_ep_data._dLg2_10);
    dbShifter = as_double(__ocl_svml_internal_dexp10_ep_data._dbShifter);
    dPC[0] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC0);
    dPC[1] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC1);
    dPC[2] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC2);
    dPC[3] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC3);
    dPC[4] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC4);
    dPC[5] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC5);
    dPC[6] = as_double(__ocl_svml_internal_dexp10_ep_data._dPC6);
    iAbsMask = (__ocl_svml_internal_dexp10_ep_data._iAbsMask);
    iDomainRange = (__ocl_svml_internal_dexp10_ep_data._iDomainRange);
    /* ...............Reduction + R............. */
    dM = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(va1, dLg2_10, dbShifter);
    dN = (dM - dbShifter);
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(va1, dLg2_10, -(dN));
    /* ...............Check for overflow\underflow ............. */
    lX = as_ulong(va1); // lX = *(longlong*)&_VARG1
    iAbsX = ((unsigned int)((unsigned long)lX >>
                            32)); // iAbsX = (int)(lX>>32); //hi bits
    iAbsX = (iAbsX & iAbsMask);   // iAbsX = iAbsX&iAbsMask;
    iRangeMask = ((unsigned int)(-(
        signed int)((signed int)iAbsX >
                    (signed int)
                        iDomainRange))); // iRangeMask = (iAbsX>iDomainRange)
    vm = 0;
    vm = iRangeMask; // M = iRangeMask?1:0
    /* ............... 2^N ..................................... */
    lM = as_ulong(dM);                  // lM = *(longlong*)&dM
    lM = ((unsigned long)(lM) << (52)); // lM = lM<<(52-K)
    /* ................... Polynomial .......................... */
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(
        dPC[6], dR, dPC[5]); // dN = dPC[5]+dR*dPC[6]
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, dR,
                                                dPC[4]); // dN = dPC[4]+dR*(...)
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, dR,
                                                dPC[3]); // dN = dPC[3]+dR*(...)
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, dR,
                                                dPC[2]); // dN = dPC[2]+dR*(...)
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, dR,
                                                dPC[1]); // dN = dPC[1]+dR*(...)
    dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dN, dR,
                                                dPC[0]); // dN = dPC[0]+dR*(...)
    /* ..................quick 2^N............................... */
    lX = as_ulong(dN);
    lX = (lX + lM);
    /* ................... Finish Poly+Reconstruction ............ */
    vr1 = as_double(lX);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    double __cout_a1;
    double __cout_r1;
    ((double *)&__cout_a1)[0] = va1;
    ((double *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_dexp10_ep(&__cout_a1, &__cout_r1);
    vr1 = ((double *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
