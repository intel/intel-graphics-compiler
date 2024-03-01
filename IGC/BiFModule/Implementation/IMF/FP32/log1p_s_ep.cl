/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
// *
// *   1+x = 2^k*(xh + xl) is computed in high-low parts; xh in [1,2)
// *   Get short reciprocal approximation Rcp ~ 1/xh
// *   R = (Rcp*xh - 1.0) + Rcp*xl
// *   log1p(x) = k*log(2.0) - log(Rcp) + poly(R)
// *      log(Rcp) is tabulated
// *
//
******************************************************************************
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
  unsigned int sPoly[4];
  unsigned int iHiDelta;
  unsigned int iLoRange;
  unsigned int iBrkValue;
  unsigned int iOffExpoMask;
  unsigned int sLn2;
  /* scalar part follow */
  unsigned int sInfs[2];
  unsigned int sOnes[2];
  unsigned int sZeros[2];
} __ocl_svml_internal_slog1p_ep_data_t;
static __ocl_svml_internal_slog1p_ep_data_t __ocl_svml_internal_slog1p_ep_data =
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
            0x3eaa7160u /*  3.3289623260498046875000000e-01 P1 */
            ,
            0xbe88e8feu /* -2.6740258932113647460937500e-01 P2 */
            ,
            0x3e612933u /*  2.1988372504711151123046875e-01 P3 */
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
        /*== sLn2 = SP ln(2) ==*/
        ,
        0x3f317218u
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
/* Macros to access other precomputed constants */
/* Table look-up: all SP constants are presented in hexadecimal form */
static __constant _iml_v2_sp_union_t __slog1p_ep_CoutTab[210] = {
    0x3F800000, /* RCPR_Y[0]       =  1.0000000000e+00 */
    0x00000000, /* LN_RCPR_Y_HI[0] =  0.0000000000e-01 */
    0x00000000, /* LN_RCPR_Y_LO[0] =  0.0000000000e-01 */
    0x3F7C0000, /* RCPR_Y[1]       =  9.8437500000e-01 */
    0x3C810000, /* LN_RCPR_Y_HI[1] =  1.5747070313e-02 */
    0x35ACB127, /* LN_RCPR_Y_LO[1] =  1.2866556744e-06 */
    0x3F780000, /* RCPR_Y[2]       =  9.6875000000e-01 */
    0x3D020000, /* LN_RCPR_Y_HI[2] =  3.1738281250e-02 */
    0x372EC4F4, /* LN_RCPR_Y_LO[2] =  1.0417064914e-05 */
    0x3F740000, /* RCPR_Y[3]       =  9.5312500000e-01 */
    0x3D44C000, /* LN_RCPR_Y_HI[3] =  4.8034667969e-02 */
    0xB7D57AD8, /* LN_RCPR_Y_LO[3] = -2.5448782253e-05 */
    0x3F700000, /* RCPR_Y[4]       =  9.3750000000e-01 */
    0x3D843000, /* LN_RCPR_Y_HI[4] =  6.4544677734e-02 */
    0xB6CE94C4, /* LN_RCPR_Y_LO[4] = -6.1565970100e-06 */
    0x3F6E0000, /* RCPR_Y[5]       =  9.2968750000e-01 */
    0x3D955000, /* LN_RCPR_Y_HI[5] =  7.2906494141e-02 */
    0x349488E3, /* LN_RCPR_Y_LO[5] =  2.7666746405e-07 */
    0x3F6A0000, /* RCPR_Y[6]       =  9.1406250000e-01 */
    0x3DB80000, /* LN_RCPR_Y_HI[6] =  8.9843750000e-02 */
    0x37530AEB, /* LN_RCPR_Y_LO[6] =  1.2579122085e-05 */
    0x3F660000, /* RCPR_Y[7]       =  8.9843750000e-01 */
    0x3DDB5000, /* LN_RCPR_Y_HI[7] =  1.0708618164e-01 */
    0x37488DAD, /* LN_RCPR_Y_LO[7] =  1.1953915418e-05 */
    0x3F640000, /* RCPR_Y[8]       =  8.9062500000e-01 */
    0x3DED4000, /* LN_RCPR_Y_HI[8] =  1.1584472656e-01 */
    0xB7589C7C, /* LN_RCPR_Y_LO[8] = -1.2911037629e-05 */
    0x3F600000, /* RCPR_Y[9]       =  8.7500000000e-01 */
    0x3E08BC00, /* LN_RCPR_Y_HI[9] =  1.3352966309e-01 */
    0x35E8227E, /* LN_RCPR_Y_LO[9] =  1.7295385533e-06 */
    0x3F5E0000, /* RCPR_Y[10]       =  8.6718750000e-01 */
    0x3E11EC00, /* LN_RCPR_Y_HI[10] =  1.4250183105e-01 */
    0xB5ED5B64, /* LN_RCPR_Y_LO[10] = -1.7684474187e-06 */
    0x3F5A0000, /* RCPR_Y[11]       =  8.5156250000e-01 */
    0x3E248800, /* LN_RCPR_Y_HI[11] =  1.6067504883e-01 */
    0x36F60CCF, /* LN_RCPR_Y_LO[11] =  7.3328624239e-06 */
    0x3F580000, /* RCPR_Y[12]       =  8.4375000000e-01 */
    0x3E2DFC00, /* LN_RCPR_Y_HI[12] =  1.6990661621e-01 */
    0xB6FE52AF, /* LN_RCPR_Y_LO[12] = -7.5794155237e-06 */
    0x3F540000, /* RCPR_Y[13]       =  8.2812500000e-01 */
    0x3E412000, /* LN_RCPR_Y_HI[13] =  1.8859863281e-01 */
    0xB6FA6AB9, /* LN_RCPR_Y_LO[13] = -7.4630047493e-06 */
    0x3F520000, /* RCPR_Y[14]       =  8.2031250000e-01 */
    0x3E4AD400, /* LN_RCPR_Y_HI[14] =  1.9807434082e-01 */
    0xB6948C24, /* LN_RCPR_Y_LO[14] = -4.4270582293e-06 */
    0x3F500000, /* RCPR_Y[15]       =  8.1250000000e-01 */
    0x3E54A000, /* LN_RCPR_Y_HI[15] =  2.0764160156e-01 */
    0xB6161BA9, /* LN_RCPR_Y_LO[15] = -2.2367842121e-06 */
    0x3F4C0000, /* RCPR_Y[16]       =  7.9687500000e-01 */
    0x3E688000, /* LN_RCPR_Y_HI[16] =  2.2705078125e-01 */
    0x36DFC995, /* LN_RCPR_Y_LO[16] =  6.6693851295e-06 */
    0x3F4A0000, /* RCPR_Y[17]       =  7.8906250000e-01 */
    0x3E729800, /* LN_RCPR_Y_HI[17] =  2.3690795898e-01 */
    0x35EFFE71, /* LN_RCPR_Y_LO[17] =  1.7880939822e-06 */
    0x3F480000, /* RCPR_Y[18]       =  7.8125000000e-01 */
    0x3E7CC800, /* LN_RCPR_Y_HI[18] =  2.4685668945e-01 */
    0x3663659E, /* LN_RCPR_Y_LO[18] =  3.3884784898e-06 */
    0x3F460000, /* RCPR_Y[19]       =  7.7343750000e-01 */
    0x3E838A00, /* LN_RCPR_Y_HI[19] =  2.5691223145e-01 */
    0xB5F3F655, /* LN_RCPR_Y_LO[19] = -1.8176602907e-06 */
    0x3F440000, /* RCPR_Y[20]       =  7.6562500000e-01 */
    0x3E88BC00, /* LN_RCPR_Y_HI[20] =  2.6705932617e-01 */
    0x3668227E, /* LN_RCPR_Y_LO[20] =  3.4590771065e-06 */
    0x3F400000, /* RCPR_Y[21]       =  7.5000000000e-01 */
    0x3E934B00, /* LN_RCPR_Y_HI[21] =  2.8768157959e-01 */
    0x35044D37, /* LN_RCPR_Y_LO[21] =  4.9286194326e-07 */
    0x3F3E0000, /* RCPR_Y[22]       =  7.4218750000e-01 */
    0x3E98A800, /* LN_RCPR_Y_HI[22] =  2.9815673828e-01 */
    0xB661E2CA, /* LN_RCPR_Y_LO[22] = -3.3659621295e-06 */
    0x3F3C0000, /* RCPR_Y[23]       =  7.3437500000e-01 */
    0x3E9E1300, /* LN_RCPR_Y_HI[23] =  3.0873870850e-01 */
    0xB6588CCD, /* LN_RCPR_Y_LO[23] = -3.2268465020e-06 */
    0x3F3A0000, /* RCPR_Y[24]       =  7.2656250000e-01 */
    0x3EA38C00, /* LN_RCPR_Y_HI[24] =  3.1942749023e-01 */
    0x365C271C, /* LN_RCPR_Y_LO[24] =  3.2805319279e-06 */
    0x3F380000, /* RCPR_Y[25]       =  7.1875000000e-01 */
    0x3EA91500, /* LN_RCPR_Y_HI[25] =  3.3023834229e-01 */
    0x3660738A, /* LN_RCPR_Y_LO[25] =  3.3445853660e-06 */
    0x3F360000, /* RCPR_Y[26]       =  7.1093750000e-01 */
    0x3EAEAE00, /* LN_RCPR_Y_HI[26] =  3.4117126465e-01 */
    0xB50829A8, /* LN_RCPR_Y_LO[26] = -5.0724565881e-07 */
    0x3F340000, /* RCPR_Y[27]       =  7.0312500000e-01 */
    0x3EB45600, /* LN_RCPR_Y_HI[27] =  3.5221862793e-01 */
    0x3603E9C7, /* LN_RCPR_Y_LO[27] =  1.9656597487e-06 */
    0x3F320000, /* RCPR_Y[28]       =  6.9531250000e-01 */
    0x3EBA0F00, /* LN_RCPR_Y_HI[28] =  3.6339569092e-01 */
    0xB5F12731, /* LN_RCPR_Y_LO[28] = -1.7967305439e-06 */
    0x3F300000, /* RCPR_Y[29]       =  6.8750000000e-01 */
    0x3EBFD800, /* LN_RCPR_Y_HI[29] =  3.7469482422e-01 */
    0xB5B884FD, /* LN_RCPR_Y_LO[29] = -1.3747772982e-06 */
    0x3F2E0000, /* RCPR_Y[30]       =  6.7968750000e-01 */
    0x3EC5B200, /* LN_RCPR_Y_HI[30] =  3.8612365723e-01 */
    0xB5CAEE9A, /* LN_RCPR_Y_LO[30] = -1.5119615000e-06 */
    0x3F2C0000, /* RCPR_Y[31]       =  6.7187500000e-01 */
    0x3ECB9D00, /* LN_RCPR_Y_HI[31] =  3.9768218994e-01 */
    0x3550C4D6, /* LN_RCPR_Y_LO[31] =  7.7772472196e-07 */
    0x3F2A0000, /* RCPR_Y[32]       =  6.6406250000e-01 */
    0x3ED19A00, /* LN_RCPR_Y_HI[32] =  4.0937805176e-01 */
    0x3580449F, /* LN_RCPR_Y_LO[32] =  9.5567145308e-07 */
    0x3F280000, /* RCPR_Y[33]       =  6.5625000000e-01 */
    0x3ED7A900, /* LN_RCPR_Y_HI[33] =  4.2121124268e-01 */
    0x3615248D, /* LN_RCPR_Y_LO[33] =  2.2224005534e-06 */
    0x3F280000, /* RCPR_Y[34]       =  6.5625000000e-01 */
    0x3ED7A900, /* LN_RCPR_Y_HI[34] =  4.2121124268e-01 */
    0x3615248D, /* LN_RCPR_Y_LO[34] =  2.2224005534e-06 */
    0x3F260000, /* RCPR_Y[35]       =  6.4843750000e-01 */
    0x3EDDCB00, /* LN_RCPR_Y_HI[35] =  4.3318939209e-01 */
    0x348DC071, /* LN_RCPR_Y_LO[35] =  2.6403316156e-07 */
    0x3F240000, /* RCPR_Y[36]       =  6.4062500000e-01 */
    0x3EE40000, /* LN_RCPR_Y_HI[36] =  4.4531250000e-01 */
    0xB5C71755, /* LN_RCPR_Y_LO[36] = -1.4833445903e-06 */
    0x3F220000, /* RCPR_Y[37]       =  6.3281250000e-01 */
    0x3EEA4800, /* LN_RCPR_Y_HI[37] =  4.5758056641e-01 */
    0x3511B7BF, /* LN_RCPR_Y_LO[37] =  5.4284095086e-07 */
    0x3F200000, /* RCPR_Y[38]       =  6.2500000000e-01 */
    0x3EF0A400, /* LN_RCPR_Y_HI[38] =  4.7000122070e-01 */
    0x3621A272, /* LN_RCPR_Y_LO[38] =  2.4085425139e-06 */
    0x3F200000, /* RCPR_Y[39]       =  6.2500000000e-01 */
    0x3EF0A400, /* LN_RCPR_Y_HI[39] =  4.7000122070e-01 */
    0x3621A272, /* LN_RCPR_Y_LO[39] =  2.4085425139e-06 */
    0x3F1E0000, /* RCPR_Y[40]       =  6.1718750000e-01 */
    0x3EF71500, /* LN_RCPR_Y_HI[40] =  4.8258209229e-01 */
    0x34AB5A0A, /* LN_RCPR_Y_LO[40] =  3.1916744092e-07 */
    0x3F1C0000, /* RCPR_Y[41]       =  6.0937500000e-01 */
    0x3EFD9B00, /* LN_RCPR_Y_HI[41] =  4.9532318115e-01 */
    0xB5EA10B7, /* LN_RCPR_Y_LO[41] = -1.7439223257e-06 */
    0x3F1A0000, /* RCPR_Y[42]       =  6.0156250000e-01 */
    0x3F021B00, /* LN_RCPR_Y_HI[42] =  5.0822448730e-01 */
    0x34BE7604, /* LN_RCPR_Y_LO[42] =  3.5476125504e-07 */
    0x3F1A0000, /* RCPR_Y[43]       =  6.0156250000e-01 */
    0x3F021B00, /* LN_RCPR_Y_HI[43] =  5.0822448730e-01 */
    0x34BE7604, /* LN_RCPR_Y_LO[43] =  3.5476125504e-07 */
    0x3F180000, /* RCPR_Y[44]       =  5.9375000000e-01 */
    0x3F0573C0, /* LN_RCPR_Y_HI[44] =  5.2129745483e-01 */
    0xB50E97D6, /* LN_RCPR_Y_LO[44] = -5.3120072607e-07 */
    0x3F160000, /* RCPR_Y[45]       =  5.8593750000e-01 */
    0x3F08D7C0, /* LN_RCPR_Y_HI[45] =  5.3454208374e-01 */
    0x338F1D6B, /* LN_RCPR_Y_LO[45] =  6.6643075058e-08 */
    0x3F140000, /* RCPR_Y[46]       =  5.7812500000e-01 */
    0x3F0C4780, /* LN_RCPR_Y_HI[46] =  5.4796600342e-01 */
    0xB55F86E2, /* LN_RCPR_Y_LO[46] = -8.3270253981e-07 */
    0x3F140000, /* RCPR_Y[47]       =  5.7812500000e-01 */
    0x3F0C4780, /* LN_RCPR_Y_HI[47] =  5.4796600342e-01 */
    0xB55F86E2, /* LN_RCPR_Y_LO[47] = -8.3270253981e-07 */
    0x3F120000, /* RCPR_Y[48]       =  5.7031250000e-01 */
    0x3F0FC300, /* LN_RCPR_Y_HI[48] =  5.6156921387e-01 */
    0x35D7F186, /* LN_RCPR_Y_LO[48] =  1.6089040855e-06 */
    0x3F100000, /* RCPR_Y[49]       =  5.6250000000e-01 */
    0x3F134B00, /* LN_RCPR_Y_HI[49] =  5.7536315918e-01 */
    0x35844D37, /* LN_RCPR_Y_LO[49] =  9.8572388652e-07 */
    0x3F100000, /* RCPR_Y[50]       =  5.6250000000e-01 */
    0x3F134B00, /* LN_RCPR_Y_HI[50] =  5.7536315918e-01 */
    0x35844D37, /* LN_RCPR_Y_LO[50] =  9.8572388652e-07 */
    0x3F0E0000, /* RCPR_Y[51]       =  5.5468750000e-01 */
    0x3F16DFC0, /* LN_RCPR_Y_HI[51] =  5.8935165405e-01 */
    0xB5AA13C8, /* LN_RCPR_Y_LO[51] = -1.2671744116e-06 */
    0x3F0E0000, /* RCPR_Y[52]       =  5.5468750000e-01 */
    0x3F16DFC0, /* LN_RCPR_Y_HI[52] =  5.8935165405e-01 */
    0xB5AA13C8, /* LN_RCPR_Y_LO[52] = -1.2671744116e-06 */
    0x3F0C0000, /* RCPR_Y[53]       =  5.4687500000e-01 */
    0x3F1A8140, /* LN_RCPR_Y_HI[53] =  6.0353469849e-01 */
    0x34AD9D8D, /* LN_RCPR_Y_LO[53] =  3.2338394362e-07 */
    0x3F0A0000, /* RCPR_Y[54]       =  5.3906250000e-01 */
    0x3F1E3040, /* LN_RCPR_Y_HI[54] =  6.1792373657e-01 */
    0x32C36BFB, /* LN_RCPR_Y_LO[54] =  2.2750091588e-08 */
    0x3F0A0000, /* RCPR_Y[55]       =  5.3906250000e-01 */
    0x3F1E3040, /* LN_RCPR_Y_HI[55] =  6.1792373657e-01 */
    0x32C36BFB, /* LN_RCPR_Y_LO[55] =  2.2750091588e-08 */
    0x3F080000, /* RCPR_Y[56]       =  5.3125000000e-01 */
    0x3F21ED00, /* LN_RCPR_Y_HI[56] =  6.3252258301e-01 */
    0xB2D06DC4, /* LN_RCPR_Y_LO[56] = -2.4264302567e-08 */
    0x3F080000, /* RCPR_Y[57]       =  5.3125000000e-01 */
    0x3F21ED00, /* LN_RCPR_Y_HI[57] =  6.3252258301e-01 */
    0xB2D06DC4, /* LN_RCPR_Y_LO[57] = -2.4264302567e-08 */
    0x3F060000, /* RCPR_Y[58]       =  5.2343750000e-01 */
    0x3F25B800, /* LN_RCPR_Y_HI[58] =  6.4733886719e-01 */
    0xB5A41A3D, /* LN_RCPR_Y_LO[58] = -1.2226588524e-06 */
    0x3F060000, /* RCPR_Y[59]       =  5.2343750000e-01 */
    0x3F25B800, /* LN_RCPR_Y_HI[59] =  6.4733886719e-01 */
    0xB5A41A3D, /* LN_RCPR_Y_LO[59] = -1.2226588524e-06 */
    0x3F040000, /* RCPR_Y[60]       =  5.1562500000e-01 */
    0x3F299180, /* LN_RCPR_Y_HI[60] =  6.6237640381e-01 */
    0xB56CBCC4, /* LN_RCPR_Y_LO[60] = -8.8191541181e-07 */
    0x3F040000, /* RCPR_Y[61]       =  5.1562500000e-01 */
    0x3F299180, /* LN_RCPR_Y_HI[61] =  6.6237640381e-01 */
    0xB56CBCC4, /* LN_RCPR_Y_LO[61] = -8.8191541181e-07 */
    0x3F020000, /* RCPR_Y[62]       =  5.0781250000e-01 */
    0x3F2D7A00, /* LN_RCPR_Y_HI[62] =  6.7764282227e-01 */
    0x34386C94, /* LN_RCPR_Y_LO[62] =  1.7175835865e-07 */
    0x3F020000, /* RCPR_Y[63]       =  5.0781250000e-01 */
    0x3F2D7A00, /* LN_RCPR_Y_HI[63] =  6.7764282227e-01 */
    0x34386C94, /* LN_RCPR_Y_LO[63] =  1.7175835865e-07 */
    0x3F000000, /* RCPR_Y[64]       =  5.0000000000e-01 */
    0x3F317200, /* LN_RCPR_Y_HI[64] =  6.9314575195e-01 */
    0x35BFBE8E, /* LN_RCPR_Y_LO[64] =  1.4286067653e-06 */
    /* Two parts of the ln(2.0) */
    0x3F317200, /* LN2_HI = 6.931457519531250e-01 */
    0x35BFBE8E, /* LN2_LO = 1.428606765330187e-06 */
    /* Right Shifter to obtain j */
    0x48000040, /* RSJ = 2^(17)+1 */
    /* Right Shifter to obtain YHi */
    0x46000000, /* RSY = 2^(7) */
    /* "Near 1" path (bound for u=1-x) */
    0x3C200000, /* NEAR0_BOUND */
    /* Scale for denormalized arguments */
    0x53800000, /* DENORM_SCALE = 2^40 */
    /* Single precision constants: 0.0, 1.0 */
    0x00000000, 0x3F800000,
    /*
    //      Coefficients for (ln(1+u) - u)/u^2 approximation by
    //        polynomial p0(u)
    */
    0xBF000000, /* A0 = -.500000000000000 */
    0x3EAAAAAB, /* A1 =  .333333333333344 */
    0xBE800000, /* A2 = -.250000000000034 */
    0x3E4CCCCD, /* A3 =  .199999999115651 */
    0xBE2AAAAB, /* A4 = -.166666665188498 */
    0x3E124E01, /* A5 =  .142875688385215 */
    0xBE0005A0, /* A6 = -.125021460296036 */
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_slog1p_ep(float *a, float *r) {
  float fap1, *ap1 = (&fap1);
  float x, y, u;
  float fP;
  float fAbsU;
  float fN, fNLn2Hi, fNLn2Lo;
  float fRcprY, fLnRcprYHi, fLnRcprYLo, fWHi, fWLo;
  float fYHi, fYLo, fUHi, fuLo, fResHi, fResLo;
  float fTmp;
  int iN, j;
  int i;
  int nRet = 0;
  int isDenorm = 0;
  *ap1 = (*a) + 1.0f;
  /* Filter out Infs and NaNs */
  if ((((((_iml_v2_sp_union_t *)&ap1[0])->hex[0] >> 23) & 0xFF) != 0xFF)) {
    /* Here if argument is finite float precision number */
    /*
    //          Copy argument into temporary variable x,
    //            and initially set iN equal to 0
    */
    x = ap1[0];
    iN = 0;
    /* Check if x is denormalized number or [+/-]0 */
    if (((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) == 0) {
      /* Here if argument is denormalized or [+/-]0 */
      isDenorm = 1;
      /* Scale x and properly adjust iN */
      x *= ((__constant float *)__slog1p_ep_CoutTab)[200];
      iN -= 40;
    }
    /* Starting from this point x is finite normalized number */
    if (x > ((__constant float *)__slog1p_ep_CoutTab)[201]) {
      /* Here if x is positive finite normalized number */
      /* Get absolute value of u=x-1 */
      u = x - 1.0f;
      fAbsU = u;
      (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] =
           (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      /* Check if ap1[0] falls into "Near 1" range */
      if (fAbsU > ((__constant float *)__slog1p_ep_CoutTab)[199]) {
        /* 6) "Main" path */
        /* ap1) Range reduction */
        /* Get N taking into account denormalized arguments */
        iN += ((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) - 0x7F;
        fN = (float)iN;
        /*
        //                  Compute N*Ln2Hi and N*Ln2Lo. Notice that N*Ln2Hi
        //                    is error-free for any N
        */
        if (isDenorm == 1) {
          fNLn2Hi = (fN * (((__constant float *)__slog1p_ep_CoutTab)[195] +
                           ((__constant float *)__slog1p_ep_CoutTab)[196]));
          fNLn2Lo = 0.0f;
        } else {
          fNLn2Hi = (fN * ((__constant float *)__slog1p_ep_CoutTab)[195]);
          fNLn2Lo = (fN * ((__constant float *)__slog1p_ep_CoutTab)[196]);
        }
        /* Get y */
        y = x;
        (((_iml_v2_sp_union_t *)&y)->hex[0] =
             (((_iml_v2_sp_union_t *)&y)->hex[0] & 0x807FFFFF) |
             (((_iml_uint32_t)(0x7F) & 0xFF) << 23));
        /* Obtain j */
        fTmp = (y + ((__constant float *)__slog1p_ep_CoutTab)[197]);
        j = (((_iml_v2_sp_union_t *)&fTmp)->hex[0] & 0x007FFFFF) &
            ((1 << (6 + 1)) - 1);
        /* Get table values of RcprY, LnRcprYHi, LnRcprYLo */
        fRcprY = ((__constant float *)__slog1p_ep_CoutTab)[3 * (j)];
        fLnRcprYHi = ((__constant float *)__slog1p_ep_CoutTab)[3 * (j) + 1];
        fLnRcprYLo = ((__constant float *)__slog1p_ep_CoutTab)[3 * (j) + 2];
        /* Calculate WHi and WLo */
        fWHi = (fNLn2Hi + fLnRcprYHi);
        fTmp = (fWHi - fNLn2Hi);
        fTmp = (fLnRcprYHi - fTmp);
        fWLo = (fNLn2Lo + fLnRcprYLo);
        fWLo = (fTmp + fWLo);
        ;
        /* Get YHi and YLo */
        fTmp = (y + ((__constant float *)__slog1p_ep_CoutTab)[198]);
        fYHi = (fTmp - ((__constant float *)__slog1p_ep_CoutTab)[198]);
        fYLo = (y - fYHi);
        /* Get UHi, uLo and U */
        fUHi = (fRcprY * fYHi - 1.0f);
        fuLo = (fRcprY * fYLo);
        u = (fUHi + fuLo);
        /* b) Approximation */
        fP = ((((((((__constant float *)__slog1p_ep_CoutTab)[209] * u +
                   ((__constant float *)__slog1p_ep_CoutTab)[208]) *
                      u +
                  ((__constant float *)__slog1p_ep_CoutTab)[207]) *
                     u +
                 ((__constant float *)__slog1p_ep_CoutTab)[206]) *
                    u +
                ((__constant float *)__slog1p_ep_CoutTab)[205]) *
                   u +
               ((__constant float *)__slog1p_ep_CoutTab)[204]) *
                  u +
              ((__constant float *)__slog1p_ep_CoutTab)[203]);
        fP = (fP * u * u);
        /* c) Reconstruction */
        fResHi = (fWHi + fUHi);
        // fResLo = _(_(fWLo + fuLo) + fP);
        fResLo = (fWLo + fuLo);
        fTmp = (fResLo - fWLo);
        fTmp = (fuLo - fTmp);
        fP = (fTmp + fP);
        r[0] = (fResHi + fResLo);
        fTmp = (r[0] - fResHi);
        fTmp = (fResLo - fTmp);
        fTmp = (fTmp + fP);
        r[0] = (r[0] + fTmp);
      } else {
        /* 5) "Near 1" path (|u|<=NEAR0_BOUND) */
        fP = ((((((((__constant float *)__slog1p_ep_CoutTab)[209] * u +
                   ((__constant float *)__slog1p_ep_CoutTab)[208]) *
                      u +
                  ((__constant float *)__slog1p_ep_CoutTab)[207]) *
                     u +
                 ((__constant float *)__slog1p_ep_CoutTab)[206]) *
                    u +
                ((__constant float *)__slog1p_ep_CoutTab)[205]) *
                   u +
               ((__constant float *)__slog1p_ep_CoutTab)[204]) *
                  u +
              ((__constant float *)__slog1p_ep_CoutTab)[203]);
        fP = (fP * u * u);
        fP = (fP + u);
        r[0] = fP;
      }
    } else {
      /* Path 3) or 4). Here if argument is negative number or +/-0 */
      if (x == ((__constant float *)__slog1p_ep_CoutTab)[201]) {
        /* Path 3). Here if argument is +/-0 */
        r[0] = -((__constant float *)__slog1p_ep_CoutTab)[202] /
               ((__constant float *)__slog1p_ep_CoutTab)[201];
        nRet = 2;
      } else {
        /* Path 4). Here if argument is negative number */
        r[0] = ((__constant float *)__slog1p_ep_CoutTab)[201] /
               ((__constant float *)__slog1p_ep_CoutTab)[201];
        nRet = 1;
      }
    }
  } else {
    /* Path 1) or 2). Here if argument is NaN or +/-Infinity */
    if (((((_iml_v2_sp_union_t *)&ap1[0])->hex[0] >> 31) == 1) &&
        ((((_iml_v2_sp_union_t *)&ap1[0])->hex[0] & 0x007FFFFF) == 0)) {
      /* Path 2). Here if argument is -Infinity */
      r[0] = ((__constant float *)__slog1p_ep_CoutTab)[201] /
             ((__constant float *)__slog1p_ep_CoutTab)[201];
      nRet = 1;
    } else {
      /* Path 1). Here if argument is NaN or +Infinity */
      r[0] = (ap1[0] * ap1[0]);
    }
  }
  return nRet;
}
float __ocl_svml_log1pf_ep(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float SignMask;
    float SgnMask;
    unsigned int iHiDelta;
    unsigned int iLoRange;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    float One;
    unsigned int iOne;
    float sExp;
    float X;
    float Xl;
    float A;
    float B;
    float Rl;
    float Rh;
    float Rlh;
    float sR2;
    float Kh;
    float NaNMask;
    unsigned int iNaNMask;
    float sLn2;
    float sPoly[4];
    unsigned int iRangeMask;
    unsigned int iX;
    unsigned int iXTest;
    float sN;
    unsigned int iN;
    float sR;
    unsigned int iR;
    float sP;
    unsigned int iExp;
    One = as_float(__ocl_svml_internal_slog1p_ep_data.sOne);
    SgnMask = as_float(__ocl_svml_internal_slog1p_ep_data.SgnMask);
    SignMask = as_float((~(as_uint(SgnMask)) & as_uint(va1)));
    NaNMask = as_float(((unsigned int)(-(signed int)(!(va1 == va1)))));
    iNaNMask = as_uint(NaNMask);
    // compute 1+x as high, low parts
    A = ((One > va1) ? One : va1);
    B = ((One < va1) ? One : va1);
    X = (A + B);
    Xl = (A - X);
    Xl = (Xl + B);
    iHiDelta = (__ocl_svml_internal_slog1p_ep_data.iHiDelta);
    iLoRange = (__ocl_svml_internal_slog1p_ep_data.iLoRange);
    iX = as_uint(X);
    /* check argument value ranges */
    iXTest = (iX + iHiDelta);
    iRangeMask = ((unsigned int)(-(signed int)((signed int)iXTest <
                                               (signed int)iLoRange)));
    /* reduction: compute r,n */
    iBrkValue = (__ocl_svml_internal_slog1p_ep_data.iBrkValue);
    iOffExpoMask = (__ocl_svml_internal_slog1p_ep_data.iOffExpoMask);
    /* preserve mantissa, set input exponent to 2^(-10) */
    iX = (iX - iBrkValue);
    iR = (iX & iOffExpoMask);
    iN = ((signed int)iX >> (23));
    iR = (iR + iBrkValue);
    // N - exponential part
    sN = ((float)((int)(iN)));
    // R - reduced argument (X's mantissa)
    sR = as_float(iR);
    iExp = ((unsigned int)(iN) << (23));
    iOne = as_uint(One);
    /* Exp = exponent bits */
    iExp = (iOne - iExp);
    sExp = as_float(iExp);
    // Rl - low part of reduced argument
    Rl = (Xl * sExp);
    iRangeMask = (iRangeMask | iNaNMask);
    /* combine and get argument value range mask */
    vm = 0;
    vm = iRangeMask;
    // Rh = R - 1
    Rh = (sR - One);
    // R = Rh + Rl
    sR = (Rh + Rl);
    /* polynomial evaluation: */
    sPoly[3] = as_float(__ocl_svml_internal_slog1p_ep_data.sPoly[3]);
    sPoly[2] = as_float(__ocl_svml_internal_slog1p_ep_data.sPoly[2]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sPoly[3], sR, sPoly[2]);
    sPoly[1] = as_float(__ocl_svml_internal_slog1p_ep_data.sPoly[1]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[1]);
    sPoly[0] = as_float(__ocl_svml_internal_slog1p_ep_data.sPoly[0]);
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sPoly[0]);
    // P = P*R
    sP = (sP * sR);
    // P = P*R + R
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sP, sR, sR);
    /* final reconstruction */
    sLn2 = as_float(__ocl_svml_internal_slog1p_ep_data.sLn2);
    // Result = N*log(2) + P
    vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(sN, sLn2, sP);
    vr1 = as_float((as_uint(vr1) | as_uint(SignMask)));
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_slog1p_ep(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
