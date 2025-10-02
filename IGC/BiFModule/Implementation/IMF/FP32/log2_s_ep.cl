/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
// *
// *   Get short reciprocal approximation Rcp ~ 1/mantissa(x)
// *   R = Rcp*x - 1.0
// *   log2(x) = k - log2(Rcp) + poly_approximation(R)
// *      log2(Rcp) is tabulated
// *
//
******************************************************************************
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned int Log_HA_table[(1 << 8) + 2];
  unsigned int ha_poly_coeff[3];
  unsigned int ExpMask;
  unsigned int Two10;
  unsigned int MinNorm;
  unsigned int MaxNorm;
  unsigned int HalfMask;
  unsigned int CLH;
  unsigned int iBrkValue;
  unsigned int iOffExpoMask;
  unsigned int One;
  unsigned int sPoly[9];
  unsigned int ep_Poly[5];
  /* scalar part follow */
  unsigned int sInfs[2];
  unsigned int sOnes[2];
  unsigned int sZeros[2];
} __ocl_svml_internal_slog2_ep_data_t;
static __ocl_svml_internal_slog2_ep_data_t __ocl_svml_internal_slog2_ep_data = {
    /* Log_HA_table */
    {0xc2fc0000u, 0x00000000u, 0xc2fc0400u, 0xb6eb07d9u, 0xc2fc0800u,
     0x37ec18d1u, 0xc2fc0bf0u, 0xb78f3a71u, 0xc2fc0fe0u, 0xb7b076feu,
     0xc2fc13d0u, 0x3784b71fu, 0xc2fc17b0u, 0xb7f34d44u, 0xc2fc1b98u,
     0x37e3ed42u, 0xc2fc1f70u, 0x355ccb60u, 0xc2fc2348u, 0x37645758u,
     0xc2fc2718u, 0x3689621au, 0xc2fc2ae0u, 0xb7ebea5du, 0xc2fc2ea8u,
     0xb7bbe728u, 0xc2fc3270u, 0x37af24dbu, 0xc2fc3628u, 0xb7adf709u,
     0xc2fc39e0u, 0xb7d65f2fu, 0xc2fc3d98u, 0x36cb5092u, 0xc2fc4148u,
     0x37552d13u, 0xc2fc44f0u, 0xb6c86327u, 0xc2fc4898u, 0x3733a188u,
     0xc2fc4c38u, 0x35b68f67u, 0xc2fc4fd8u, 0x37dfd085u, 0xc2fc5370u,
     0x37d432e1u, 0xc2fc5700u, 0xb651cfdfu, 0xc2fc5a90u, 0x360e51d6u,
     0xc2fc5e18u, 0xb7aa760fu, 0xc2fc61a0u, 0xb7234cffu, 0xc2fc6520u,
     0xb7e668fdu, 0xc2fc68a0u, 0xb756abbeu, 0xc2fc6c18u, 0xb7e2fc1eu,
     0xc2fc6f90u, 0xb71fc2f6u, 0xc2fc7300u, 0xb7b482b2u, 0xc2fc7670u,
     0xb61a9d07u, 0xc2fc79d8u, 0xb75d7ee2u, 0xc2fc7d40u, 0x36db5cf5u,
     0xc2fc80a0u, 0xb693a750u, 0xc2fc8400u, 0x377756b2u, 0xc2fc8758u,
     0x363266b2u, 0xc2fc8ab0u, 0x37a8b8a6u, 0xc2fc8e00u, 0x36c2d256u,
     0xc2fc9150u, 0x37ac0e19u, 0xc2fc9498u, 0x3644d145u, 0xc2fc97e0u,
     0x37686a41u, 0xc2fc9b20u, 0xb7067ac7u, 0xc2fc9e60u, 0xb5fd2a77u,
     0xc2fca198u, 0xb7f398eeu, 0xc2fca4d0u, 0xb7f0979au, 0xc2fca808u,
     0xb58cf4b1u, 0xc2fcab38u, 0xb6f8e7f0u, 0xc2fcae68u, 0x375a69c0u,
     0xc2fcb190u, 0xb586fc11u, 0xc2fcb4b8u, 0x373de2cfu, 0xc2fcb7d8u,
     0xb73d105bu, 0xc2fcbaf8u, 0xb7056dddu, 0xc2fcbe18u, 0x37b090b4u,
     0xc2fcc130u, 0x3773005eu, 0xc2fcc440u, 0xb7e9b13fu, 0xc2fcc758u,
     0x3785395cu, 0xc2fcca68u, 0x37c4828du, 0xc2fccd70u, 0xb6b63673u,
     0xc2fcd078u, 0xb725534eu, 0xc2fcd380u, 0x3727027cu, 0xc2fcd680u,
     0xb6f2d37cu, 0xc2fcd980u, 0xb51dd8f4u, 0xc2fcdc80u, 0x37f935cfu,
     0xc2fcdf78u, 0x37bc202fu, 0xc2fce268u, 0xb7bdc376u, 0xc2fce560u,
     0x3789fbebu, 0xc2fce850u, 0x3791d41du, 0xc2fceb38u, 0xb7a7c066u,
     0xc2fcee28u, 0x37dbbd7au, 0xc2fcf108u, 0xb7e52ea4u, 0xc2fcf3f0u,
     0x36201ac7u, 0xc2fcf9b0u, 0x36bbdfa8u, 0xc2fcff60u, 0xb7decd5fu,
     0xc2fd0510u, 0x37df8e14u, 0xc2fd0aa8u, 0xb7a0da0bu, 0xc2fd1040u,
     0x37955516u, 0xc2fd15c8u, 0x376f670fu, 0xc2fd1b48u, 0x37fc14c5u,
     0xc2fd20b8u, 0x36442fa3u, 0xc2fd2620u, 0xb6f2cf29u, 0xc2fd2b80u,
     0xb5d1cfdfu, 0xc2fd30d8u, 0x379dfa99u, 0xc2fd3620u, 0xb7091303u,
     0xc2fd3b60u, 0xb7bd93fcu, 0xc2fd4098u, 0xb7d5df20u, 0xc2fd45c8u,
     0xb795f62bu, 0xc2fd4af0u, 0xb546d45du, 0xc2fd5010u, 0x37d12e6au,
     0xc2fd5520u, 0xb63e66b1u, 0xc2fd5a28u, 0xb7c90426u, 0xc2fd5f30u,
     0x37b5d5a4u, 0xc2fd6428u, 0x373a6affu, 0xc2fd6918u, 0x3696dc9au,
     0xc2fd6e00u, 0x3581c0a8u, 0xc2fd72e0u, 0xb4ad6a5fu, 0xc2fd77b8u,
     0xb45a03bbu, 0xc2fd7c88u, 0x3509b65au, 0xc2fd8150u, 0x3589e351u,
     0xc2fd8610u, 0x3515a79cu, 0xc2fd8ac8u, 0xb5df5425u, 0xc2fd8f78u,
     0xb6d667c3u, 0xc2fd9420u, 0xb770e5bau, 0xc2fd98c0u, 0xb7dc9441u,
     0xc2fd9d60u, 0x37980ac0u, 0xc2fda1f0u, 0xb681c27bu, 0xc2fda680u,
     0x37f43a8au, 0xc2fdab00u, 0xb6be5f85u, 0xc2fdaf80u, 0x375d1cb6u,
     0xc2fdb3f8u, 0x37c93d2du, 0xc2fdb868u, 0x37db1d98u, 0xc2fdbcd0u,
     0x379ef11bu, 0xc2fdc130u, 0x35f911cau, 0xc2fdc588u, 0xb7d811a6u,
     0xc2fdc9e0u, 0xb6677d2bu, 0xc2fdce30u, 0x36f053a0u, 0xc2fdd278u,
     0x36b8be27u, 0xc2fdd6b8u, 0xb716adf4u, 0xc2fddaf8u, 0x37cadee4u,
     0xc2fddf28u, 0xb793bd0du, 0xc2fde358u, 0xb7574a57u, 0xc2fde780u,
     0xb7c14076u, 0xc2fdeba8u, 0x374e3a0eu, 0xc2fdefc0u, 0xb7f6ce3du,
     0xc2fdf3d8u, 0xb7df31afu, 0xc2fdf7f0u, 0x37a9d4fbu, 0xc2fdfbf8u,
     0xb73f8d4au, 0xc2fe0000u, 0x80000000u}
    /*== ha_poly_coeff[3] ==*/
    ,
    {
        0x3e25C86Au /* coeff3 */
        ,
        0xbeB2BB8Cu /* coeff2 */
        ,
        0x3b6CD7E0u /* coeff1 */
    }
    /*== ExpMask ==*/
    ,
    0x007fffffu
    /*== Two10 ==*/
    ,
    0x3c000000u
    /*== MinNorm ==*/
    ,
    0x00800000u
    /*== MaxNorm ==*/
    ,
    0x7f7fffffu
    /*== HalfMask ==*/
    ,
    0xffffff00u
    /*== CLH ==*/
    ,
    0x3fb80000u
    /*== iBrkValue = SP 2/3 ==*/
    ,
    0x3f2aaaabu
    /*== iOffExpoMask = SP significand mask ==*/
    ,
    0x007fffffu
    /*== sOne = SP 1.0 ==*/
    ,
    0x3f800000u
    /*== spoly[9] ==*/
    ,
    {
        0x3e554012u /* coeff9 */
        ,
        0xbe638E14u /* coeff8 */
        ,
        0x3e4D660Bu /* coeff7 */
        ,
        0xbe727824u /* coeff6 */
        ,
        0x3e93DD07u /* coeff5 */
        ,
        0xbeB8B969u /* coeff4 */
        ,
        0x3eF637C0u /* coeff3 */
        ,
        0xbf38AA2Bu /* coeff2 */
        ,
        0x3fB8AA3Bu /* coeff1 */
    }
    /*== ep_poly[4] ==*/
    ,
    {
        0x3eAAE01Bu /* coeff5 */
        ,
        0xbeCB686Cu /* coeff4 */
        ,
        0x3eF4FA41u /* coeff3 */
        ,
        0xbf38658Au /* coeff2 */
        ,
        0x3fB8AAEDu /* coeff1 */
    }
    /* scalar part follow */
    /*== dInfs = DP infinity, +/- ==*/
    ,
    {0x7f800000u, 0xff800000u}
    /*== dOnes = DP one, +/- ==*/
    ,
    {0x3f800000u, 0xbf800000u}
    /*== dZeros = DP zero +/- ==*/
    ,
    {0x00000000u, 0x80000000u}}; /*slog10_Table*/
/* Macros to access other precomputed constants */
/* Table look-up: all DP constants are presented in hexadecimal form */
static __constant _iml_v2_sp_union_t __slog2_ep_CoutTab[212] = {
    0x43b8aa40, /* B[0]            = 369.33007812500 */
    0x00000000, /* LG_RCPR_Y_HI[0] = 0.00000000000 */
    0x00000000, /* LG_RCPR_Y_LO[0] = 0.00000000000 */
    0x43b5c797, /* B[1]            = 363.55929565430 */
    0x3cba0000, /* LG_RCPR_Y_HI[1] = 0.02270507813 */
    0x377ba188, /* LG_RCPR_Y_LO[1] = 0.00001499838 */
    0x43b31319, /* B[2]            = 358.14920043945 */
    0x3d358000, /* LG_RCPR_Y_HI[2] = 0.04431152344 */
    0x3821c52f, /* LG_RCPR_Y_LO[2] = 0.00003856903 */
    0x43b05e9a, /* B[3]            = 352.73907470703 */
    0x3d87c000, /* LG_RCPR_Y_HI[3] = 0.06628417969 */
    0x37d31a33, /* LG_RCPR_Y_LO[3] = 0.00002516536 */
    0x43add846, /* B[4]            = 347.68963623047 */
    0x3db26000, /* LG_RCPR_Y_HI[4] = 0.08709716797 */
    0x37626c06, /* LG_RCPR_Y_LO[4] = 0.00001349580 */
    0x43ab51f2, /* B[5]            = 342.64019775391 */
    0x3ddda000, /* LG_RCPR_Y_HI[5] = 0.10821533203 */
    0x35817c93, /* LG_RCPR_Y_LO[5] = 0.00000096475 */
    0x43a8cb9f, /* B[6]            = 337.59078979492 */
    0x3e04c000, /* LG_RCPR_Y_HI[6] = 0.12963867188 */
    0xb66398d0, /* LG_RCPR_Y_LO[6] = -0.00000339146 */
    0x43a67375, /* B[7]            = 332.90200805664 */
    0x3e196800, /* LG_RCPR_Y_HI[7] = 0.14981079102 */
    0x361f2349, /* LG_RCPR_Y_LO[7] = 0.00000237134 */
    0x43a41b4c, /* B[8]            = 328.21325683594 */
    0x3e2e6000, /* LG_RCPR_Y_HI[8] = 0.17028808594 */
    0xb7358bf1, /* LG_RCPR_Y_LO[8] = -0.00001082102 */
    0x43a1f14d, /* B[9]            = 323.88516235352 */
    0x3e41f800, /* LG_RCPR_Y_HI[9] = 0.18942260742 */
    0x36c13371, /* LG_RCPR_Y_LO[9] = 0.00000575784 */
    0x439fb039, /* B[10]            = 319.37673950195 */
    0x3e56b000, /* LG_RCPR_Y_HI[10] = 0.20965576172 */
    0xb68ee3fc, /* LG_RCPR_Y_LO[10] = -0.00000425847 */
    0x439d9d50, /* B[11]            = 315.22900390625 */
    0x3e6a0000, /* LG_RCPR_Y_HI[11] = 0.22851562500 */
    0xb6aaf16d, /* LG_RCPR_Y_LO[11] = -0.00000509450 */
    0x439b8a66, /* B[12]            = 311.08123779297 */
    0x3e7d9000, /* LG_RCPR_Y_HI[12] = 0.24761962891 */
    0xb493e2d1, /* LG_RCPR_Y_LO[12] = -0.00000027546 */
    0x4399777d, /* B[13]            = 306.93350219727 */
    0x3e88b200, /* LG_RCPR_Y_HI[13] = 0.26698303223 */
    0x35dcef23, /* LG_RCPR_Y_LO[13] = 0.00000164609 */
    0x43977ba9, /* B[14]            = 302.96609497070 */
    0x3e924e00, /* LG_RCPR_Y_HI[14] = 0.28575134277 */
    0x3652b13d, /* LG_RCPR_Y_LO[14] = 0.00000313956 */
    0x439596ea, /* B[15]            = 299.17901611328 */
    0x3e9b9a00, /* LG_RCPR_Y_HI[15] = 0.30390930176 */
    0xb6fabe1f, /* LG_RCPR_Y_LO[15] = -0.00000747271 */
    0x4393b22b, /* B[16]            = 295.39193725586 */
    0x3ea50200, /* LG_RCPR_Y_HI[16] = 0.32228088379 */
    0xb50d0b48, /* LG_RCPR_Y_LO[16] = -0.00000052543 */
    0x4391e481, /* B[17]            = 291.78518676758 */
    0x3eae1600, /* LG_RCPR_Y_HI[17] = 0.34001159668 */
    0xb6fb4af6, /* LG_RCPR_Y_LO[17] = -0.00000748911 */
    0x439016d7, /* B[18]            = 288.17843627930 */
    0x3eb74600, /* LG_RCPR_Y_HI[18] = 0.35795593262 */
    0xb6ffdeef, /* LG_RCPR_Y_LO[18] = -0.00000762555 */
    0x438e6043, /* B[19]            = 284.75204467773 */
    0x3ec01a00, /* LG_RCPR_Y_HI[19] = 0.37519836426 */
    0x36cf5cc8, /* LG_RCPR_Y_LO[19] = 0.00000617988 */
    0x438ca9af, /* B[20]            = 281.32565307617 */
    0x3ec90c00, /* LG_RCPR_Y_HI[20] = 0.39266967773 */
    0x32124dd4, /* LG_RCPR_Y_LO[20] = 0.00000000852 */
    0x438b0a30, /* B[21]            = 278.07958984375 */
    0x3ed19e00, /* LG_RCPR_Y_HI[21] = 0.40940856934 */
    0x3692fe59, /* LG_RCPR_Y_LO[21] = 0.00000438075 */
    0x43896ab1, /* B[22]            = 274.83352661133 */
    0x3eda4a00, /* LG_RCPR_Y_HI[22] = 0.42634582520 */
    0x36ea748c, /* LG_RCPR_Y_LO[22] = 0.00000698731 */
    0x4387e247, /* B[23]            = 271.76779174805 */
    0x3ee29400, /* LG_RCPR_Y_HI[23] = 0.44253540039 */
    0x35710ee2, /* LG_RCPR_Y_LO[23] = 0.00000089801 */
    0x438642c8, /* B[24]            = 268.52172851563 */
    0x3eeb7400, /* LG_RCPR_Y_HI[24] = 0.45986938477 */
    0x362cea8d, /* LG_RCPR_Y_LO[24] = 0.00000257665 */
    0x4384d173, /* B[25]            = 265.63632202148 */
    0x3ef37000, /* LG_RCPR_Y_HI[25] = 0.47546386719 */
    0xb6bb4eef, /* LG_RCPR_Y_LO[25] = -0.00000558222 */
    0x4383490a, /* B[26]            = 262.57061767578 */
    0x3efc0200, /* LG_RCPR_Y_HI[26] = 0.49220275879 */
    0x362e8d0d, /* LG_RCPR_Y_LO[26] = 0.00000260101 */
    0x4381d7b5, /* B[27]            = 259.68521118164 */
    0x3f021600, /* LG_RCPR_Y_HI[27] = 0.50814819336 */
    0xb5ad1961, /* LG_RCPR_Y_LO[27] = -0.00000128969 */
    0x43807d76, /* B[28]            = 256.98016357422 */
    0x3f05f400, /* LG_RCPR_Y_HI[28] = 0.52325439453 */
    0xb520a698, /* LG_RCPR_Y_LO[28] = -0.00000059847 */
    0x437e1843, /* B[29]            = 254.09477233887 */
    0x3f0a2000, /* LG_RCPR_Y_HI[29] = 0.53955078125 */
    0xb6e00fcd, /* LG_RCPR_Y_LO[29] = -0.00000667756 */
    0x437b63c4, /* B[30]            = 251.38970947266 */
    0x3f0e1400, /* LG_RCPR_Y_HI[30] = 0.55499267578 */
    0xb6fc627b, /* LG_RCPR_Y_LO[30] = -0.00000752165 */
    0x4378dd70, /* B[31]            = 248.86499023438 */
    0x3f11ce00, /* LG_RCPR_Y_HI[31] = 0.56954956055 */
    0xb60dbf88, /* LG_RCPR_Y_LO[31] = -0.00000211221 */
    0x437628f2, /* B[32]            = 246.15994262695 */
    0x3f15d700, /* LG_RCPR_Y_HI[32] = 0.58531188965 */
    0x3640e84c, /* LG_RCPR_Y_LO[32] = 0.00000287454 */
    0x4373a29e, /* B[33]            = 243.63522338867 */
    0x3f19a600, /* LG_RCPR_Y_HI[33] = 0.60018920898 */
    0xb59cced4, /* LG_RCPR_Y_LO[33] = -0.00000116831 */
    0x43711c4a, /* B[34]            = 241.11050415039 */
    0x3f1d7f00, /* LG_RCPR_Y_HI[34] = 0.61521911621 */
    0xb6405abc, /* LG_RCPR_Y_LO[34] = -0.00000286630 */
    0x436ec421, /* B[35]            = 238.76612854004 */
    0x3f211a00, /* LG_RCPR_Y_HI[35] = 0.62930297852 */
    0x37214eb9, /* LG_RCPR_Y_LO[35] = 0.00000961468 */
    0x436c6bf7, /* B[36]            = 236.42173767090 */
    0x3f24c000, /* LG_RCPR_Y_HI[36] = 0.64355468750 */
    0xb6df6565, /* LG_RCPR_Y_LO[36] = -0.00000665772 */
    0x436a13ce, /* B[37]            = 234.07736206055 */
    0x3f286e00, /* LG_RCPR_Y_HI[37] = 0.65792846680 */
    0xb6525f63, /* LG_RCPR_Y_LO[37] = -0.00000313480 */
    0x4367bba5, /* B[38]            = 231.73298645020 */
    0x3f2c2600, /* LG_RCPR_Y_HI[38] = 0.67245483398 */
    0xb6faec18, /* LG_RCPR_Y_LO[38] = -0.00000747807 */
    0x436591a6, /* B[39]            = 229.56893920898 */
    0x3f2f9c00, /* LG_RCPR_Y_HI[39] = 0.68597412109 */
    0x3719ee59, /* LG_RCPR_Y_LO[39] = 0.00000917501 */
    0x4363397d, /* B[40]            = 227.22456359863 */
    0x3f336700, /* LG_RCPR_Y_HI[40] = 0.70079040527 */
    0x35d3929b, /* LG_RCPR_Y_LO[40] = 0.00000157634 */
    0x43610f7e, /* B[41]            = 225.06051635742 */
    0x3f36f000, /* LG_RCPR_Y_HI[41] = 0.71459960938 */
    0xb5f561c1, /* LG_RCPR_Y_LO[41] = -0.00000182824 */
    0x435f13aa, /* B[42]            = 223.07681274414 */
    0x3f3a3500, /* LG_RCPR_Y_HI[42] = 0.72737121582 */
    0xb586531d, /* LG_RCPR_Y_LO[42] = -0.00000100080 */
    0x435ce9ab, /* B[43]            = 220.91276550293 */
    0x3f3dce00, /* LG_RCPR_Y_HI[43] = 0.74142456055 */
    0x371dcc96, /* LG_RCPR_Y_LO[43] = 0.00000940556 */
    0x435aedd7, /* B[44]            = 218.92906188965 */
    0x3f412400, /* LG_RCPR_Y_HI[44] = 0.75445556641 */
    0xb70acad0, /* LG_RCPR_Y_LO[44] = -0.00000827266 */
    0x4358c3d8, /* B[45]            = 216.76501464844 */
    0x3f44ce00, /* LG_RCPR_Y_HI[45] = 0.76876831055 */
    0x37304eb4, /* LG_RCPR_Y_LO[45] = 0.00001050874 */
    0x4356f62f, /* B[46]            = 214.96165466309 */
    0x3f47e400, /* LG_RCPR_Y_HI[46] = 0.78082275391 */
    0x3712644d, /* LG_RCPR_Y_LO[46] = 0.00000872563 */
    0x4354fa5a, /* B[47]            = 212.97793579102 */
    0x3f4b5100, /* LG_RCPR_Y_HI[47] = 0.79420471191 */
    0x3608d068, /* LG_RCPR_Y_LO[47] = 0.00000203869 */
    0x4352fe86, /* B[48]            = 210.99423217773 */
    0x3f4ec600, /* LG_RCPR_Y_HI[48] = 0.80770874023 */
    0xb5d0ab43, /* LG_RCPR_Y_LO[48] = -0.00000155471 */
    0x435130dd, /* B[49]            = 209.19087219238 */
    0x3f51f200, /* LG_RCPR_Y_HI[49] = 0.82009887695 */
    0xb705a9de, /* LG_RCPR_Y_LO[49] = -0.00000796697 */
    0x434f6333, /* B[50]            = 207.38749694824 */
    0x3f552400, /* LG_RCPR_Y_HI[50] = 0.83258056641 */
    0x35acd72a, /* LG_RCPR_Y_LO[50] = 0.00000128776 */
    0x434d9589, /* B[51]            = 205.58412170410 */
    0x3f585e00, /* LG_RCPR_Y_HI[51] = 0.84518432617 */
    0xb6236cd4, /* LG_RCPR_Y_LO[51] = -0.00000243522 */
    0x434bc7e0, /* B[52]            = 203.78076171875 */
    0x3f5b9f00, /* LG_RCPR_Y_HI[52] = 0.85789489746 */
    0xb6032e95, /* LG_RCPR_Y_LO[52] = -0.00000195476 */
    0x4349fa36, /* B[53]            = 201.97738647461 */
    0x3f5ee800, /* LG_RCPR_Y_HI[53] = 0.87072753906 */
    0xb73119b5, /* LG_RCPR_Y_LO[53] = -0.00001055601 */
    0x43485ab7, /* B[54]            = 200.35435485840 */
    0x3f61e200, /* LG_RCPR_Y_HI[54] = 0.88235473633 */
    0x36111bbc, /* LG_RCPR_Y_LO[54] = 0.00000216228 */
    0x43468d0d, /* B[55]            = 198.55097961426 */
    0x3f653900, /* LG_RCPR_Y_HI[55] = 0.89540100098 */
    0x3483c7a3, /* LG_RCPR_Y_LO[55] = 0.00000024546 */
    0x4344ed8e, /* B[56]            = 196.92794799805 */
    0x3f684100, /* LG_RCPR_Y_HI[56] = 0.90724182129 */
    0x358b4a3b, /* LG_RCPR_Y_LO[56] = 0.00000103779 */
    0x43434e0f, /* B[57]            = 195.30491638184 */
    0x3f6b5000, /* LG_RCPR_Y_HI[57] = 0.91918945313 */
    0xb6ea3c0b, /* LG_RCPR_Y_LO[57] = -0.00000698073 */
    0x4341ae90, /* B[58]            = 193.68188476563 */
    0x3f6e6400, /* LG_RCPR_Y_HI[58] = 0.93121337891 */
    0x370bf961, /* LG_RCPR_Y_LO[58] = 0.00000834311 */
    0x43403d3c, /* B[59]            = 192.23919677734 */
    0x3f712800, /* LG_RCPR_Y_HI[59] = 0.94201660156 */
    0xb70ba8b0, /* LG_RCPR_Y_LO[59] = -0.00000832432 */
    0x433e9dbd, /* B[60]            = 190.61616516113 */
    0x3f744900, /* LG_RCPR_Y_HI[60] = 0.95423889160 */
    0x35c23715, /* LG_RCPR_Y_LO[60] = 0.00000144702 */
    0x433d2c68, /* B[61]            = 189.17346191406 */
    0x3f771800, /* LG_RCPR_Y_HI[61] = 0.96520996094 */
    0xb715b634, /* LG_RCPR_Y_LO[61] = -0.00000892351 */
    0x433b8ce9, /* B[62]            = 187.55043029785 */
    0x3f7a4600, /* LG_RCPR_Y_HI[62] = 0.97763061523 */
    0x35d2f47a, /* LG_RCPR_Y_LO[62] = 0.00000157174 */
    0x433a1b95, /* B[63]            = 186.10774230957 */
    0x3f7d2000, /* LG_RCPR_Y_HI[63] = 0.98876953125 */
    0x3657a488, /* LG_RCPR_Y_LO[63] = 0.00000321333 */
    0x4338aa40, /* B[64]            = 184.66503906250 */
    0x3f800000, /* LG_RCPR_Y_HI[64] = 1.00000000000 */
    0x00000000, /* LG_RCPR_Y_LO[64] = 0.00000000000 */
    /* Two parts of the lg(2.0) */
    0x3F800000, /* LG2_HI = 1.000000000000000e+00 */
    0x00000000, /* LG2_LO = 0.000000000000000e-01 */
    /* Right Shifter to obtain j */
    0x48000040, /* RSJ = 2^(17)+1 */
    /* Right Shifter to obtain YHi */
    0x46000000, /* RSY = 2^(13) */
    /* "Near 1" path (bound for u=1-x) */
    0x3BC00000, /* NEAR0_BOUND */
    /* Scale for denormalized arguments */
    0x4D000000, /* DENORM_SCALE = 2^27 */
    /* Constants: 0.0, 1.0 */
    0x00000000, 0x3F800000, 0x43B8AA40,
    /* Coefficients for polynomial approximation */
    0xBF7F0000, /* A0 = -9.9609375156e-01 */
    0xB6B1720F, /* A1 = -5.2882890802e-06 */
    0x3223FE93, /* _A2 = 9.5457322929e-09 */
    0xADAA8223, /* _A3 = -1.9384555019e-11 */
    0x293D1988, /* A4 = 4.1988575782e-14 */
    0xA4DA74DC, /* A5 = -9.4740398233e-17 */
    0x2081CD9D, /* A6 = 2.1989512445e-19 */
    0x9C1D865E, /* A7 = -5.2120609317e-22 */
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_slog2_ep(float *a, float *r) {
  float x, y, u, q;
  float fP;
  float fAbsU;
  float fN, fNLg2Hi, fNLg2Lo;
  float fB, fLgRcprYHi, fLgRcprYLo, fWHi, fWLo;
  float fYHi, fYLo, fUHi, fuLo, fResHi, fResLo;
  float fQHi, fQLo;
  float fTmp;
  int iN, j;
  int i;
  int nRet = 0;
  /* Filter out Infs and NaNs */
  if ((((((_iml_v2_sp_union_t *)&(*a))->hex[0] >> 23) & 0xFF) != 0xFF)) {
    /* Here if argument is finite float precision number */
    /*
    //              Copy argument into temporary variable x,
    //                and initially set iN equal to 0
    */
    x = (*a);
    iN = 0;
    /* Check if x is denormalized number or [+/-]0 */
    if ((x != ((__constant float *)__slog2_ep_CoutTab)[201]) &&
        (((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) == 0)) {
      /* Here if argument is denormalized or [+/-]0 */
      /* Scale x and properly adjust iN */
      x = (x * ((__constant float *)__slog2_ep_CoutTab)[200]);
      iN -= 27;
    }
    /* Starting from this point x is finite normalized number */
    if (x > ((__constant float *)__slog2_ep_CoutTab)[201]) {
      /* Here if x is positive finite normalized number */
      /* Get absolute value of u=x-1 */
      u = (x - 1.0f);
      fAbsU = u;
      (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] =
           (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      /* Check if (*a) falls into "Near 1" range */
      if (fAbsU > ((__constant float *)__slog2_ep_CoutTab)[199]) {
        /* 6) "Main" path */
        /* a) Range reduction */
        /* Get N taking into account denormalized arguments */
        iN += ((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) - 0x7F;
        fN = (float)iN;
        /*
        //                      Compute N*Lg2Hi and N*Lg2Lo. Notice that N*Lg2Hi
        //                        is error-free for any N
        */
        fNLg2Hi = (fN * ((__constant float *)__slog2_ep_CoutTab)[195]);
        y = x;
        (((_iml_v2_sp_union_t *)&y)->hex[0] =
             (((_iml_v2_sp_union_t *)&y)->hex[0] & 0x807FFFFF) |
             (((_iml_uint32_t)(0x7F) & 0xFF) << 23));
        /* Obtain j */
        fTmp = (y + ((__constant float *)__slog2_ep_CoutTab)[197]);
        j = (((_iml_v2_sp_union_t *)&fTmp)->hex[0] & 0x007FFFFF) &
            ((1 << (6 + 1)) - 1);
        /* Get table values of B, LgRcprYHi, LgRcprYLo */
        fB = ((__constant float *)__slog2_ep_CoutTab)[3 * (j)];
        fLgRcprYHi = ((__constant float *)__slog2_ep_CoutTab)[3 * (j) + 1];
        fLgRcprYLo = ((__constant float *)__slog2_ep_CoutTab)[3 * (j) + 2];
        /* Calculate WHi and WLo */
        fWHi = (fNLg2Hi + fLgRcprYHi);
        /* Get YHi and YLo */
        fTmp = (y + ((__constant float *)__slog2_ep_CoutTab)[198]);
        fYHi = (fTmp - ((__constant float *)__slog2_ep_CoutTab)[198]);
        fYLo = (y - fYHi);
        /* Get QHi, QLo and q */
        {
          /* Split fB in high-low parts */
          union {
            int w;
            float f;
          } BHi, BLo;
          /* Initialize high-low parts */
          BHi.f = fB;
          BHi.w &= 0xffffe000;
          BLo.f = (fB - BHi.f);
          /* Restore QHi, QLo */
          fQHi = (BHi.f * fYHi - ((__constant float *)__slog2_ep_CoutTab)[203]);
          fQLo = (fB * fYLo);
          fQLo = (BLo.f * fYHi + fQLo);
        }
        q = (fQHi + fQLo);
        /* b) Approximation */
        fP = (((((((((__constant float *)__slog2_ep_CoutTab)[211] * q +
                    ((__constant float *)__slog2_ep_CoutTab)[210]) *
                       q +
                   ((__constant float *)__slog2_ep_CoutTab)[209]) *
                      q +
                  ((__constant float *)__slog2_ep_CoutTab)[208]) *
                     q +
                 ((__constant float *)__slog2_ep_CoutTab)[207]) *
                    q +
                ((__constant float *)__slog2_ep_CoutTab)[206]) *
                   q +
               ((__constant float *)__slog2_ep_CoutTab)[205]) *
                  q +
              ((__constant float *)__slog2_ep_CoutTab)[204]);
        /* c) Reconstruction */
        fResHi = (fWHi + fQHi);
        fResLo = (fLgRcprYLo + fP * fQLo);
        fResLo = (fResLo + fQLo);
        fResLo = (fResLo + fP * fQHi);
        (*r) = (fResHi + fResLo);
      } else {
        /* 5) "Near 1" path (|u|<=NEAR0_BOUND) */
        /* Calculate q */
        q = (u * ((__constant float *)__slog2_ep_CoutTab)[203]);
        fP = (((((((((__constant float *)__slog2_ep_CoutTab)[211] * q +
                    ((__constant float *)__slog2_ep_CoutTab)[210]) *
                       q +
                   ((__constant float *)__slog2_ep_CoutTab)[209]) *
                      q +
                  ((__constant float *)__slog2_ep_CoutTab)[208]) *
                     q +
                 ((__constant float *)__slog2_ep_CoutTab)[207]) *
                    q +
                ((__constant float *)__slog2_ep_CoutTab)[206]) *
                   q +
               ((__constant float *)__slog2_ep_CoutTab)[205]) *
                  q +
              ((__constant float *)__slog2_ep_CoutTab)[204]);
        fP = (fP * q);
        fP = (fP + q);
        (*r) = (float)fP;
      }
    } else {
      /* Path 3) or 4). Here if argument is negative number or +/-0 */
      if (x == ((__constant float *)__slog2_ep_CoutTab)[201]) {
        /* Path 3). Here if argument is +/-0 */
        (*r) = (float)(-((__constant float *)__slog2_ep_CoutTab)[202] /
                       ((__constant float *)__slog2_ep_CoutTab)[201]);
        nRet = 2;
      } else {
        /* Path 4). Here if argument is negative number */
        (*r) = (float)(((__constant float *)__slog2_ep_CoutTab)[201] /
                       ((__constant float *)__slog2_ep_CoutTab)[201]);
        nRet = 1;
      }
    }
  } else {
    /* Path 1) or 2). Here if argument is NaN or +/-Infinity */
    if (((((_iml_v2_sp_union_t *)&(*a))->hex[0] >> 31) == 1) &&
        ((((_iml_v2_sp_union_t *)&(*a))->hex[0] & 0x007FFFFF) == 0)) {
      /* Path 2). Here if argument is -Infinity */
      (*r) = (float)(((__constant float *)__slog2_ep_CoutTab)[201] /
                     ((__constant float *)__slog2_ep_CoutTab)[201]);
      nRet = 1;
    } else {
      /* Path 1). Here if argument is NaN or +Infinity */
      (*r) = (*a) * (*a);
    }
  }
  return nRet;
}
float __ocl_svml_log2f_ep(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float FpExpon;
    unsigned int BrMask;
    float BrMask1;
    float BrMask2;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    float MinNorm;
    float MaxNorm;
    float One;
    float sR;
    float L2H;
    float L2L;
    float Kh;
    float Kl;
    float sPoly[9];
    unsigned int iX;
    unsigned int iN;
    unsigned int iR;
    float sP;
    float sP78;
    float sP56;
    float sP34;
    float sP12;
    float sR2;
    /* reduction: compute r,n */
    iBrkValue = (__ocl_svml_internal_slog2_ep_data.iBrkValue);
    iOffExpoMask = (__ocl_svml_internal_slog2_ep_data.iOffExpoMask);
    iX = as_uint(va1);
    iX = (iX - iBrkValue);
    /* preserve mantissa, set input exponent to 2^(-10) */
    iR = (iX & iOffExpoMask);
    iN = ((signed int)iX >> (23));
    iR = (iR + iBrkValue);
    /* Expon = exponent bits */
    FpExpon = ((float)((int)(iN)));
    // R = argument mantissa
    sR = as_float(iR);
    MinNorm = as_float(__ocl_svml_internal_slog2_ep_data.MinNorm);
    MaxNorm = as_float(__ocl_svml_internal_slog2_ep_data.MaxNorm);
    /* check if argument is in [MinNorm, MaxNorm] */
    BrMask1 = as_float(((unsigned int)(-(signed int)(va1 < MinNorm))));
    BrMask2 = as_float(((unsigned int)(-(signed int)(!(va1 <= MaxNorm)))));
    BrMask1 = as_float((as_uint(BrMask1) | as_uint(BrMask2)));
    BrMask = as_uint(BrMask1);
    /* combine and get argument value range mask */
    vm = 0;
    vm = BrMask;
    One = as_float(__ocl_svml_internal_slog2_ep_data.One);
    // R = R - 1
    sR = (sR - One);
    sPoly[4] = as_float(__ocl_svml_internal_slog2_ep_data.ep_Poly[0]);
    sPoly[3] = as_float(__ocl_svml_internal_slog2_ep_data.ep_Poly[1]);
    sP = __spirv_ocl_fma(sPoly[4], sR, sPoly[3]);
    sPoly[2] = as_float(__ocl_svml_internal_slog2_ep_data.ep_Poly[2]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[2]);
    sPoly[1] = as_float(__ocl_svml_internal_slog2_ep_data.ep_Poly[3]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[1]);
    sPoly[0] = as_float(__ocl_svml_internal_slog2_ep_data.ep_Poly[4]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[0]);
    vr1 = __spirv_ocl_fma(sP, sR, FpExpon);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_slog2_ep(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
