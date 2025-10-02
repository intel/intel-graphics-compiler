/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
// *
// *   Get short reciprocal approximation Rcp ~ 1/mantissa(x)
// *   R = Rcp*x - 1.0
// *   log10(x) = k*log10(2.0) - log10(Rcp) + poly_approximation(R)
// *      log10(Rcp) is tabulated
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
  unsigned int L2H;
  unsigned int L2L;
  unsigned int iBrkValue;
  unsigned int iOffExpoMask;
  unsigned int One;
  unsigned int sPoly[9];
  unsigned int ep_Poly[5];
  unsigned int L2;
  /* scalar part follow */
  unsigned int sInfs[2];
  unsigned int sOnes[2];
  unsigned int sZeros[2];
} __ocl_svml_internal_slog10_ep_data_t;
static __ocl_svml_internal_slog10_ep_data_t __ocl_svml_internal_slog10_ep_data =
    {
        /* Log_HA_table */
        {0xc217b87cu, 0x39c7ca28u, 0xc217ba7eu, 0x39cc701au, 0xc217bc7eu,
         0x39d1aa37u, 0xc217be6eu, 0x39bb6e0du, 0xc217c06eu, 0x39c9b14eu,
         0xc217c25eu, 0x39bc69d0u, 0xc217c45eu, 0x39d38d89u, 0xc217c64eu,
         0x39cf1294u, 0xc217c83eu, 0x39ceef2bu, 0xc217ca2eu, 0x39d319a8u,
         0xc217cc0eu, 0x39bb8888u, 0xc217cdfeu, 0x39c83263u, 0xc217cfeeu,
         0x39d90df3u, 0xc217d1ceu, 0x39ce1211u, 0xc217d3aeu, 0x39c735b1u,
         0xc217d58eu, 0x39c46fe6u, 0xc217d76eu, 0x39c5b7e1u, 0xc217d94eu,
         0x39cb04edu, 0xc217db2eu, 0x39d44e73u, 0xc217dcfeu, 0x39c18bf6u,
         0xc217dedeu, 0x39d2b514u, 0xc217e0aeu, 0x39c7c187u, 0xc217e27eu,
         0x39c0a922u, 0xc217e44eu, 0x39bd63d1u, 0xc217e61eu, 0x39bde99au,
         0xc217e7eeu, 0x39c2329du, 0xc217e9beu, 0x39ca3711u, 0xc217eb8eu,
         0x39d5ef45u, 0xc217ed4eu, 0x39c553a1u, 0xc217ef1eu, 0x39d85ca2u,
         0xc217f0deu, 0x39cf02dfu, 0xc217f29eu, 0x39c93f03u, 0xc217f45eu,
         0x39c709cfu, 0xc217f61eu, 0x39c85c1du, 0xc217f7deu, 0x39cd2edau,
         0xc217fb4eu, 0x39c139bdu, 0xc217febeu, 0x39c2f387u, 0xc218022eu,
         0x39d22688u, 0xc218058eu, 0x39ce9e4bu, 0xc21808eeu, 0x39d82789u,
         0xc2180c3eu, 0x39ce9024u, 0xc2180f8eu, 0x39d1a71bu, 0xc21812ceu,
         0x39c13c84u, 0xc218160eu, 0x39bd2180u, 0xc218194eu, 0x39c52839u,
         0xc2181c8eu, 0x39d923d6u, 0xc2181fbeu, 0x39d8e873u, 0xc21822deu,
         0x39c44b1fu, 0xc21825feu, 0x39bb21d0u, 0xc218291eu, 0x39bd435fu,
         0xc2182c3eu, 0x39ca8781u, 0xc2182f4eu, 0x39c2c6c3u, 0xc218325eu,
         0x39c5da80u, 0xc218356eu, 0x39d39cdfu, 0xc218386eu, 0x39cbe8cau,
         0xc2183b6eu, 0x39ce99eeu, 0xc2183e5eu, 0x39bb8cafu, 0xc218415eu,
         0x39d29e2au, 0xc218444eu, 0x39d3ac2bu, 0xc218472eu, 0x39be952bu,
         0xc2184a1eu, 0x39d3384bu, 0xc2184cfeu, 0x39d1754du, 0xc2184fdeu,
         0x39d92c95u, 0xc21852aeu, 0x39ca3f22u, 0xc218557eu, 0x39c48e88u,
         0xc218584eu, 0x39c7fcf1u, 0xc2185b1eu, 0x39d46d15u, 0xc2185ddeu,
         0x39c9c239u, 0xc218609eu, 0x39c7e029u, 0xc218635eu, 0x39ceab39u,
         0xc218660eu, 0x39be083cu, 0xc21868ceu, 0x39d5dc87u, 0xc2186b7eu,
         0x39d60de7u, 0xc2186e1eu, 0x39be82a6u, 0xc21870ceu, 0x39cf2181u,
         0xc218736eu, 0x39c7d1a9u, 0xc218760eu, 0x39c87abfu, 0xc21878aeu,
         0x39d104d3u, 0xc2187b3eu, 0x39c1585fu, 0xc2187ddeu, 0x39d95e46u,
         0xc218806eu, 0x39d8ffd1u, 0xc21882eeu, 0x39c026afu, 0xc218857eu,
         0x39cebcedu, 0xc21887feu, 0x39c4acfbu, 0xc2188a7eu, 0x39c1e1a4u,
         0xc2188cfeu, 0x39c6460fu, 0xc2188f7eu, 0x39d1c5bdu, 0xc21891eeu,
         0x39c44c86u, 0xc218945eu, 0x39bdc695u, 0xc21896ceu, 0x39be206cu,
         0xc218993eu, 0x39c546dbu, 0xc2189baeu, 0x39d32706u, 0xc2189e0eu,
         0x39c7ae5bu, 0xc218a06eu, 0x39c2ca97u, 0xc218a2ceu, 0x39c469c1u,
         0xc218a52eu, 0x39cc7a29u, 0xc218a77eu, 0x39baea67u, 0xc218a9deu,
         0x39cfa95au, 0xc218ac2eu, 0x39caa624u, 0xc218ae7eu, 0x39cbd02bu,
         0xc218b0ceu, 0x39d31717u, 0xc218b30eu, 0x39c06ad1u, 0xc218b55eu,
         0x39d3bb81u, 0xc218b79eu, 0x39ccf98cu, 0xc218b9deu, 0x39cc1595u,
         0xc218bc1eu, 0x39d10079u, 0xc218be4eu, 0x39bbab51u, 0xc218c08eu,
         0x39cc076eu, 0xc218c2beu, 0x39c20659u, 0xc218c4eeu, 0x39bd99d1u,
         0xc218c71eu, 0x39beb3ceu, 0xc218c94eu, 0x39c54678u, 0xc218cb7eu,
         0x39d1442fu, 0xc218cd9eu, 0x39c29f83u, 0xc218cfceu, 0x39d94b37u,
         0xc218d1eeu, 0x39d53a3fu, 0xc218d40eu, 0x39d65fbdu, 0xc218d61eu,
         0x39bcaf04u, 0xc218d83eu, 0x39c81b93u, 0xc218da5eu, 0x39d8991au,
         0xc218dc6eu, 0x39ce1b70u, 0xc218de7eu, 0x39c8969bu, 0xc218e08eu,
         0x39c7fecbu, 0xc218e29eu, 0x39cc485bu, 0xc218e4aeu, 0x39d567ccu,
         0xc218e6aeu, 0x39c351cau, 0xc218e8beu, 0x39d5fb29u, 0xc218eabeu,
         0x39cd58e1u, 0xc218ecbeu, 0x39c96014u}
        /*== ha_poly_coeff[3] ==*/
        ,
        {
            0x3fE35103u /* coeff3 */
            ,
            0xbf93D7E4u /* coeff2 */
            ,
            0x3aD3D366u /* coeff1 */
        }
        /*== ExpMask ==*/
        ,
        0x007fffffu
        /*== Two10 ==*/
        ,
        0x3b000000u
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
        0x3ede0000u
        /*== L2H ==*/
        ,
        0x3e9a2100u
        /*== L2L ==*/
        ,
        0xb64AF600u
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
            0x3d8063B4u /* coeff9 */
            ,
            0xbd890073u /* coeff8 */
            ,
            0x3d775317u /* coeff7 */
            ,
            0xbd91FB27u /* coeff6 */
            ,
            0x3dB20B96u /* coeff5 */
            ,
            0xbdDE6E20u /* coeff4 */
            ,
            0x3e143CE5u /* coeff3 */
            ,
            0xbe5E5BC5u /* coeff2 */
            ,
            0x3eDE5BD9u /* coeff1 */
        }
        /*== ep_poly[4] ==*/
        ,
        {
            0x3dCDC127u /* coeff5 */
            ,
            0xbdF4ED71u /* coeff4 */
            ,
            0x3e137DBEu /* coeff3 */
            ,
            0xbe5E0922u /* coeff2 */
            ,
            0x3eDE5CAEu /* coeff1 */
        }
        /*== L2 ==*/
        ,
        0x3e9a209bu
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
static __constant _iml_v2_sp_union_t __slog10_ep_CoutTab[212] = {
    0x42DE5C00, /* B[0]            =  1.1117968750e+02 */
    0x00000000, /* LG_RCPR_Y_HI[0] =  0.0000000000e-01 */
    0x00000000, /* LG_RCPR_Y_LO[0] =  0.0000000000e-01 */
    0x42DAE290, /* B[1]            =  1.0944250488e+02 */
    0x3BE00000, /* LG_RCPR_Y_HI[1] =  6.8359375000e-03 */
    0x366A02B9, /* LG_RCPR_Y_LO[1] =  3.4870301988e-06 */
    0x42D76920, /* B[2]            =  1.0770532227e+02 */
    0x3C620000, /* LG_RCPR_Y_HI[2] =  1.3793945313e-02 */
    0xB6BDF222, /* LG_RCPR_Y_LO[2] = -5.6608269006e-06 */
    0x42D3EFB0, /* B[3]            =  1.0596813965e+02 */
    0x3CAB0000, /* LG_RCPR_Y_HI[3] =  2.0874023438e-02 */
    0xB7C85B7F, /* LG_RCPR_Y_LO[3] = -2.3884464099e-05 */
    0x42D07640, /* B[4]            =  1.0423095703e+02 */
    0x3CE58000, /* LG_RCPR_Y_HI[4] =  2.8015136719e-02 */
    0x3763F336, /* LG_RCPR_Y_LO[4] =  1.3586881323e-05 */
    0x42CEB988, /* B[5]            =  1.0336236572e+02 */
    0x3D01C000, /* LG_RCPR_Y_HI[5] =  3.1677246094e-02 */
    0xB76EDF0D, /* LG_RCPR_Y_LO[5] = -1.4237838514e-05 */
    0x42CB4018, /* B[6]            =  1.0162518311e+02 */
    0x3D1FE000, /* LG_RCPR_Y_HI[6] =  3.9031982422e-02 */
    0xB7041CCE, /* LG_RCPR_Y_LO[6] = -7.8745197243e-06 */
    0x42C7C6A8, /* B[7]            =  9.9888000488e+01 */
    0x3D3E8000, /* LG_RCPR_Y_HI[7] =  4.6508789063e-02 */
    0x366028BF, /* LG_RCPR_Y_LO[7] =  3.3402318422e-06 */
    0x42C609F0, /* B[8]            =  9.9019409180e+01 */
    0x3D4E0000, /* LG_RCPR_Y_HI[8] =  5.0292968750e-02 */
    0x374BD5F8, /* LG_RCPR_Y_LO[8] =  1.2149561371e-05 */
    0x42C29080, /* B[9]            =  9.7282226563e+01 */
    0x3D6D8000, /* LG_RCPR_Y_HI[9] =  5.7983398438e-02 */
    0x370F6BB3, /* LG_RCPR_Y_LO[9] =  8.5485398813e-06 */
    0x42C0D3C8, /* B[10]            =  9.6413635254e+01 */
    0x3D7D8000, /* LG_RCPR_Y_HI[10] =  6.1889648438e-02 */
    0xB63258D0, /* LG_RCPR_Y_LO[10] = -2.6575762604e-06 */
    0x42BD5A58, /* B[11]            =  9.4676452637e+01 */
    0x3D8EE800, /* LG_RCPR_Y_HI[11] =  6.9778442383e-02 */
    0x36A8C191, /* LG_RCPR_Y_LO[11] =  5.0293242566e-06 */
    0x42BB9DA0, /* B[12]            =  9.3807861328e+01 */
    0x3D972000, /* LG_RCPR_Y_HI[12] =  7.3791503906e-02 */
    0xB6B17E91, /* LG_RCPR_Y_LO[12] = -5.2897453315e-06 */
    0x42B82430, /* B[13]            =  9.2070678711e+01 */
    0x3DA7C000, /* LG_RCPR_Y_HI[13] =  8.1909179688e-02 */
    0xB6AA4C88, /* LG_RCPR_Y_LO[13] = -5.0753042160e-06 */
    0x42B66778, /* B[14]            =  9.1202087402e+01 */
    0x3DB02800, /* LG_RCPR_Y_HI[14] =  8.6013793945e-02 */
    0x36E6BDD3, /* LG_RCPR_Y_LO[14] =  6.8766325967e-06 */
    0x42B4AAC0, /* B[15]            =  9.0333496094e+01 */
    0x3DB8B000, /* LG_RCPR_Y_HI[15] =  9.0179443359e-02 */
    0xB63CC726, /* LG_RCPR_Y_LO[15] = -2.8130102692e-06 */
    0x42B13150, /* B[16]            =  8.8596313477e+01 */
    0x3DC9F000, /* LG_RCPR_Y_HI[16] =  9.8602294922e-02 */
    0x36FBC1F8, /* LG_RCPR_Y_LO[16] =  7.5029638538e-06 */
    0x42AF7498, /* B[17]            =  8.7727722168e+01 */
    0x3DD2B800, /* LG_RCPR_Y_HI[17] =  1.0289001465e-01 */
    0xB5BE6D05, /* LG_RCPR_Y_LO[17] = -1.4187831994e-06 */
    0x42ADB7E0, /* B[18]            =  8.6859130859e+01 */
    0x3DDB9000, /* LG_RCPR_Y_HI[18] =  1.0720825195e-01 */
    0x35E68B8B, /* LG_RCPR_Y_LO[18] =  1.7176947722e-06 */
    0x42ABFB28, /* B[19]            =  8.5990539551e+01 */
    0x3DE48000, /* LG_RCPR_Y_HI[19] =  1.1157226563e-01 */
    0x36286799, /* LG_RCPR_Y_LO[19] =  2.5094252578e-06 */
    0x42AA3E70, /* B[20]            =  8.5121948242e+01 */
    0x3DED8800, /* LG_RCPR_Y_HI[20] =  1.1598205566e-01 */
    0x35F6BB35, /* LG_RCPR_Y_LO[20] =  1.8382912685e-06 */
    0x42A6C500, /* B[21]            =  8.3384765625e+01 */
    0x3DFFE000, /* LG_RCPR_Y_HI[21] =  1.2493896484e-01 */
    0xB47510E2, /* LG_RCPR_Y_LO[21] = -2.2823545009e-07 */
    0x42A50848, /* B[22]            =  8.2516174316e+01 */
    0x3E049800, /* LG_RCPR_Y_HI[22] =  1.2948608398e-01 */
    0x34968666, /* LG_RCPR_Y_LO[22] =  2.8037464972e-07 */
    0x42A34B90, /* B[23]            =  8.1647583008e+01 */
    0x3E094C00, /* LG_RCPR_Y_HI[23] =  1.3407897949e-01 */
    0x36527D9F, /* LG_RCPR_Y_LO[23] =  3.1365559607e-06 */
    0x42A18ED8, /* B[24]            =  8.0778991699e+01 */
    0x3E0E0E00, /* LG_RCPR_Y_HI[24] =  1.3872528076e-01 */
    0x35E9955C, /* LG_RCPR_Y_LO[24] =  1.7403322090e-06 */
    0x429FD220, /* B[25]            =  7.9910400391e+01 */
    0x3E12DE00, /* LG_RCPR_Y_HI[25] =  1.4342498779e-01 */
    0xB63EF528, /* LG_RCPR_Y_LO[25] = -2.8454905987e-06 */
    0x429E1568, /* B[26]            =  7.9041809082e+01 */
    0x3E17BA00, /* LG_RCPR_Y_HI[26] =  1.4817047119e-01 */
    0xB5FE30B1, /* LG_RCPR_Y_LO[26] = -1.8938645781e-06 */
    0x429C58B0, /* B[27]            =  7.8173217773e+01 */
    0x3E1CA400, /* LG_RCPR_Y_HI[27] =  1.5296936035e-01 */
    0xB5FF086B, /* LG_RCPR_Y_LO[27] = -1.9001430474e-06 */
    0x429A9BF8, /* B[28]            =  7.7304626465e+01 */
    0x3E219C00, /* LG_RCPR_Y_HI[28] =  1.5782165527e-01 */
    0xB5E321F9, /* LG_RCPR_Y_LO[28] = -1.6922705299e-06 */
    0x4298DF40, /* B[29]            =  7.6436035156e+01 */
    0x3E26A200, /* LG_RCPR_Y_HI[29] =  1.6272735596e-01 */
    0xB37B14B7, /* LG_RCPR_Y_LO[29] = -5.8459331598e-08 */
    0x42972288, /* B[30]            =  7.5567443848e+01 */
    0x3E2BB800, /* LG_RCPR_Y_HI[30] =  1.6769409180e-01 */
    0xB6627A11, /* LG_RCPR_Y_LO[30] = -3.3747676298e-06 */
    0x429565D0, /* B[31]            =  7.4698852539e+01 */
    0x3E30DC00, /* LG_RCPR_Y_HI[31] =  1.7271423340e-01 */
    0xB636333E, /* LG_RCPR_Y_LO[31] = -2.7149940252e-06 */
    0x4293A918, /* B[32]            =  7.3830261230e+01 */
    0x3E360E00, /* LG_RCPR_Y_HI[32] =  1.7778778076e-01 */
    0x365AFCDE, /* LG_RCPR_Y_LO[32] =  3.2631719478e-06 */
    0x4291EC60, /* B[33]            =  7.2961669922e+01 */
    0x3E3B5200, /* LG_RCPR_Y_HI[33] =  1.8292999268e-01 */
    0x353976FD, /* LG_RCPR_Y_LO[33] =  6.9091021260e-07 */
    0x4291EC60, /* B[34]            =  7.2961669922e+01 */
    0x3E3B5200, /* LG_RCPR_Y_HI[34] =  1.8292999268e-01 */
    0x353976FD, /* LG_RCPR_Y_LO[34] =  6.9091021260e-07 */
    0x42902FA8, /* B[35]            =  7.2093078613e+01 */
    0x3E40A600, /* LG_RCPR_Y_HI[35] =  1.8813323975e-01 */
    0xB5B6DE43, /* LG_RCPR_Y_LO[35] = -1.3624743360e-06 */
    0x428E72F0, /* B[36]            =  7.1224487305e+01 */
    0x3E460A00, /* LG_RCPR_Y_HI[36] =  1.9339752197e-01 */
    0xB5BC896B, /* LG_RCPR_Y_LO[36] = -1.4047085415e-06 */
    0x428CB638, /* B[37]            =  7.0355895996e+01 */
    0x3E4B7E00, /* LG_RCPR_Y_HI[37] =  1.9872283936e-01 */
    0x360DB1D0, /* LG_RCPR_Y_LO[37] =  2.1114137780e-06 */
    0x428AF980, /* B[38]            =  6.9487304688e+01 */
    0x3E510400, /* LG_RCPR_Y_HI[38] =  2.0411682129e-01 */
    0x365427DE, /* LG_RCPR_Y_LO[38] =  3.1613667488e-06 */
    0x428AF980, /* B[39]            =  6.9487304688e+01 */
    0x3E510400, /* LG_RCPR_Y_HI[39] =  2.0411682129e-01 */
    0x365427DE, /* LG_RCPR_Y_LO[39] =  3.1613667488e-06 */
    0x42893CC8, /* B[40]            =  6.8618713379e+01 */
    0x3E569C00, /* LG_RCPR_Y_HI[40] =  2.0957946777e-01 */
    0x3664E163, /* LG_RCPR_Y_LO[40] =  3.4105839859e-06 */
    0x42878010, /* B[41]            =  6.7750122070e+01 */
    0x3E5C4800, /* LG_RCPR_Y_HI[41] =  2.1511840820e-01 */
    0xB64C1834, /* LG_RCPR_Y_LO[41] = -3.0412456908e-06 */
    0x4285C358, /* B[42]            =  6.6881530762e+01 */
    0x3E620400, /* LG_RCPR_Y_HI[42] =  2.2071838379e-01 */
    0x356709EA, /* LG_RCPR_Y_LO[42] =  8.6068632754e-07 */
    0x4285C358, /* B[43]            =  6.6881530762e+01 */
    0x3E620400, /* LG_RCPR_Y_HI[43] =  2.2071838379e-01 */
    0x356709EA, /* LG_RCPR_Y_LO[43] =  8.6068632754e-07 */
    0x428406A0, /* B[44]            =  6.6012939453e+01 */
    0x3E67D400, /* LG_RCPR_Y_HI[44] =  2.2639465332e-01 */
    0x35E765CC, /* LG_RCPR_Y_LO[44] =  1.7240467969e-06 */
    0x428249E8, /* B[45]            =  6.5144348145e+01 */
    0x3E6DB800, /* LG_RCPR_Y_HI[45] =  2.3214721680e-01 */
    0x35C7E96E, /* LG_RCPR_Y_LO[45] =  1.4894592368e-06 */
    0x42808D30, /* B[46]            =  6.4275756836e+01 */
    0x3E73B000, /* LG_RCPR_Y_HI[46] =  2.3797607422e-01 */
    0x36120236, /* LG_RCPR_Y_LO[46] =  2.1756982278e-06 */
    0x42808D30, /* B[47]            =  6.4275756836e+01 */
    0x3E73B000, /* LG_RCPR_Y_HI[47] =  2.3797607422e-01 */
    0x36120236, /* LG_RCPR_Y_LO[47] =  2.1756982278e-06 */
    0x427DA0F0, /* B[48]            =  6.3407165527e+01 */
    0x3E79BE00, /* LG_RCPR_Y_HI[48] =  2.4388885498e-01 */
    0xB5EA454F, /* LG_RCPR_Y_LO[48] = -1.7454530052e-06 */
    0x427A2780, /* B[49]            =  6.2538574219e+01 */
    0x3E7FE000, /* LG_RCPR_Y_HI[49] =  2.4987792969e-01 */
    0xB4F510E2, /* LG_RCPR_Y_LO[49] = -4.5647090019e-07 */
    0x427A2780, /* B[50]            =  6.2538574219e+01 */
    0x3E7FE000, /* LG_RCPR_Y_HI[50] =  2.4987792969e-01 */
    0xB4F510E2, /* LG_RCPR_Y_LO[50] = -4.5647090019e-07 */
    0x4276AE10, /* B[51]            =  6.1669982910e+01 */
    0x3E830C00, /* LG_RCPR_Y_HI[51] =  2.5595092773e-01 */
    0x353A13F5, /* LG_RCPR_Y_LO[51] =  6.9319440854e-07 */
    0x4276AE10, /* B[52]            =  6.1669982910e+01 */
    0x3E830C00, /* LG_RCPR_Y_HI[52] =  2.5595092773e-01 */
    0x353A13F5, /* LG_RCPR_Y_LO[52] =  6.9319440854e-07 */
    0x427334A0, /* B[53]            =  6.0801391602e+01 */
    0x3E863380, /* LG_RCPR_Y_HI[53] =  2.6211166382e-01 */
    0x348EB55F, /* LG_RCPR_Y_LO[53] =  2.6581525958e-07 */
    0x426FBB30, /* B[54]            =  5.9932800293e+01 */
    0x3E896680, /* LG_RCPR_Y_HI[54] =  2.6836013794e-01 */
    0x3546E726, /* LG_RCPR_Y_LO[54] =  7.4097113156e-07 */
    0x426FBB30, /* B[55]            =  5.9932800293e+01 */
    0x3E896680, /* LG_RCPR_Y_HI[55] =  2.6836013794e-01 */
    0x3546E726, /* LG_RCPR_Y_LO[55] =  7.4097113156e-07 */
    0x426C41C0, /* B[56]            =  5.9064208984e+01 */
    0x3E8CA580, /* LG_RCPR_Y_HI[56] =  2.7470016479e-01 */
    0x356F7BDB, /* LG_RCPR_Y_LO[56] =  8.9214671561e-07 */
    0x426C41C0, /* B[57]            =  5.9064208984e+01 */
    0x3E8CA580, /* LG_RCPR_Y_HI[57] =  2.7470016479e-01 */
    0x356F7BDB, /* LG_RCPR_Y_LO[57] =  8.9214671561e-07 */
    0x4268C850, /* B[58]            =  5.8195617676e+01 */
    0x3E8FF100, /* LG_RCPR_Y_HI[58] =  2.8113555908e-01 */
    0xB4D2869F, /* LG_RCPR_Y_LO[58] = -3.9213497871e-07 */
    0x4268C850, /* B[59]            =  5.8195617676e+01 */
    0x3E8FF100, /* LG_RCPR_Y_HI[59] =  2.8113555908e-01 */
    0xB4D2869F, /* LG_RCPR_Y_LO[59] = -3.9213497871e-07 */
    0x42654EE0, /* B[60]            =  5.7327026367e+01 */
    0x3E934900, /* LG_RCPR_Y_HI[60] =  2.8766632080e-01 */
    0xB499EB08, /* LG_RCPR_Y_LO[60] = -2.8669478525e-07 */
    0x42654EE0, /* B[61]            =  5.7327026367e+01 */
    0x3E934900, /* LG_RCPR_Y_HI[61] =  2.8766632080e-01 */
    0xB499EB08, /* LG_RCPR_Y_LO[61] = -2.8669478525e-07 */
    0x4261D570, /* B[62]            =  5.6458435059e+01 */
    0x3E96AE00, /* LG_RCPR_Y_HI[62] =  2.9429626465e-01 */
    0x34BB05C3, /* LG_RCPR_Y_LO[62] =  3.4835656493e-07 */
    0x4261D570, /* B[63]            =  5.6458435059e+01 */
    0x3E96AE00, /* LG_RCPR_Y_HI[63] =  2.9429626465e-01 */
    0x34BB05C3, /* LG_RCPR_Y_LO[63] =  3.4835656493e-07 */
    0x425E5C00, /* B[64]            =  5.5589843750e+01 */
    0x3E9A2080, /* LG_RCPR_Y_HI[64] =  3.0102920532e-01 */
    0x355427DE, /* LG_RCPR_Y_LO[64] =  7.9034168721e-07 */
    /* Two parts of the lg(2.0) */
    0x3E9A0000, /* LG2_HI = 3.007812500000000e-01 */
    0x39826A14, /* LG2_LO = 2.487456658855081e-04 */
    /* Right Shifter to obtain j */
    0x48000040, /* RSJ = 2^(17)+1 */
    /* Right Shifter to obtain YHi */
    0x46000000, /* RSY = 2^(13) */
    /* "Near 1" path (bound for u=1-x) */
    0x3BC00000, /* NEAR0_BOUND */
    /* Scale for denormalized arguments */
    0x53800000, /* DENORM_SCALE = 2^40 */
    /* Constants: 0.0, 1.0 */
    0x00000000, 0x3F800000, 0x42DE5C00,
    /* Coefficients for polynomial approximation */
    0xBF7F0000, /* A0 = -9.9609376055e-01 */
    0xB7935D5A, /* A1 = -1.7567235269e-05 */
    0x33E23666, /* A2 = 1.0533839807e-07 */
    0xB04353B9, /* A3 = -7.1059561626e-10 */
    0x2CB3E701, /* A4 = 5.1131326151e-12 */
    0xA92C998B, /* A5 = -3.8324849174e-14 */
    0x25AA5BFD, /* A6 = 2.9552632596e-16 */
    0xA22B5DAE, /* A7 = -2.3224414867e-18 */
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_slog10_ep(float *a, float *r) {
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
    if (((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) == 0) {
      /* Here if argument is denormalized or [+/-]0 */
      /* Scale x and properly adjust iN */
      x *= ((__constant float *)__slog10_ep_CoutTab)[200];
      iN -= 40;
    }
    /* Starting from this point x is finite normalized number */
    if (x > ((__constant float *)__slog10_ep_CoutTab)[201]) {
      /* Here if x is positive finite normalized number */
      /* Get absolute value of u=x-1 */
      u = (x - 1.0f);
      fAbsU = u;
      (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] =
           (((_iml_v2_sp_union_t *)&fAbsU)->hex[0] & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      /* Check if (*a) falls into "Near 1" range */
      if (fAbsU > ((__constant float *)__slog10_ep_CoutTab)[199]) {
        /* 6) "Main" path */
        /* a) Range reduction */
        /* Get N taking into account denormalized arguments */
        iN += ((((_iml_v2_sp_union_t *)&x)->hex[0] >> 23) & 0xFF) - 0x7F;
        fN = (float)iN;
        /*
        //                      Compute N*Lg2Hi and N*Lg2Lo. Notice that N*Lg2Hi
        //                        is error-free for any N
        */
        fNLg2Hi = (fN * ((__constant float *)__slog10_ep_CoutTab)[195]);
        fNLg2Lo = (fN * ((__constant float *)__slog10_ep_CoutTab)[196]);
        // printf("val=%.20e\n",fNLg2Lo);
        /* Get y */
        y = x;
        (((_iml_v2_sp_union_t *)&y)->hex[0] =
             (((_iml_v2_sp_union_t *)&y)->hex[0] & 0x807FFFFF) |
             (((_iml_uint32_t)(0x7F) & 0xFF) << 23));
        /* Obtain j */
        fTmp = (y + ((__constant float *)__slog10_ep_CoutTab)[197]);
        j = (((_iml_v2_sp_union_t *)&fTmp)->hex[0] & 0x007FFFFF) &
            ((1 << (6 + 1)) - 1);
        /* Get table values of B, LgRcprYHi, LgRcprYLo */
        fB = ((__constant float *)__slog10_ep_CoutTab)[3 * (j)];
        fLgRcprYHi = ((__constant float *)__slog10_ep_CoutTab)[3 * (j) + 1];
        fLgRcprYLo = ((__constant float *)__slog10_ep_CoutTab)[3 * (j) + 2];
        /* Calculate WHi and WLo */
        fWHi = (fNLg2Hi + fLgRcprYHi);
        fWLo = (fNLg2Lo + fLgRcprYLo);
        /* Get YHi and YLo */
        fTmp = (y + ((__constant float *)__slog10_ep_CoutTab)[198]);
        fYHi = (fTmp - ((__constant float *)__slog10_ep_CoutTab)[198]);
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
          fQHi =
              (BHi.f * fYHi - ((__constant float *)__slog10_ep_CoutTab)[203]);
          fQLo = (fB * fYLo);
          fQLo = (BLo.f * fYHi + fQLo);
        }
        q = (fQHi + fQLo);
        /* b) Approximation */
        fP = (((((((((__constant float *)__slog10_ep_CoutTab)[211] * q +
                    ((__constant float *)__slog10_ep_CoutTab)[210]) *
                       q +
                   ((__constant float *)__slog10_ep_CoutTab)[209]) *
                      q +
                  ((__constant float *)__slog10_ep_CoutTab)[208]) *
                     q +
                 ((__constant float *)__slog10_ep_CoutTab)[207]) *
                    q +
                ((__constant float *)__slog10_ep_CoutTab)[206]) *
                   q +
               ((__constant float *)__slog10_ep_CoutTab)[205]) *
                  q +
              ((__constant float *)__slog10_ep_CoutTab)[204]);
        /* c) Reconstruction */
        fResHi = (fWHi + fQHi);
        fResLo = (fWLo + fP * fQLo);
        fResLo = (fResLo + fQLo);
        fResLo = (fResLo + fP * fQHi);
        (*r) = (fResHi + fResLo);
      } else {
        /* 5) "Near 1" path (|u|<=NEAR0_BOUND) */
        /* Calculate q */
        q = (u * ((__constant float *)__slog10_ep_CoutTab)[203]);
        fP = (((((((((__constant float *)__slog10_ep_CoutTab)[211] * q +
                    ((__constant float *)__slog10_ep_CoutTab)[210]) *
                       q +
                   ((__constant float *)__slog10_ep_CoutTab)[209]) *
                      q +
                  ((__constant float *)__slog10_ep_CoutTab)[208]) *
                     q +
                 ((__constant float *)__slog10_ep_CoutTab)[207]) *
                    q +
                ((__constant float *)__slog10_ep_CoutTab)[206]) *
                   q +
               ((__constant float *)__slog10_ep_CoutTab)[205]) *
                  q +
              ((__constant float *)__slog10_ep_CoutTab)[204]);
        fP = (fP * q);
        fP = (fP + q);
        (*r) = (float)fP;
      }
    } else {
      /* Path 3) or 4). Here if argument is negative number or +/-0 */
      if (x == ((__constant float *)__slog10_ep_CoutTab)[201]) {
        /* Path 3). Here if argument is +/-0 */
        (*r) = (float)(-((__constant float *)__slog10_ep_CoutTab)[202] /
                       ((__constant float *)__slog10_ep_CoutTab)[201]);
        nRet = 2;
      } else {
        /* Path 4). Here if argument is negative number */
        (*r) = (float)(((__constant float *)__slog10_ep_CoutTab)[201] /
                       ((__constant float *)__slog10_ep_CoutTab)[201]);
        nRet = 1;
      }
    }
  } else {
    /* Path 1) or 2). Here if argument is NaN or +/-Infinity */
    if (((((_iml_v2_sp_union_t *)&(*a))->hex[0] >> 31) == 1) &&
        ((((_iml_v2_sp_union_t *)&(*a))->hex[0] & 0x007FFFFF) == 0)) {
      /* Path 2). Here if argument is -Infinity */
      (*r) = (float)(((__constant float *)__slog10_ep_CoutTab)[201] /
                     ((__constant float *)__slog10_ep_CoutTab)[201]);
      nRet = 1;
    } else {
      /* Path 1). Here if argument is NaN or +Infinity */
      (*r) = (*a) * (*a);
    }
  }
  return nRet;
}
float __ocl_svml_log10f_ep(float x) {
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
    float L2;
    /* reduction: compute r,n */
    iBrkValue = (__ocl_svml_internal_slog10_ep_data.iBrkValue);
    iOffExpoMask = (__ocl_svml_internal_slog10_ep_data.iOffExpoMask);
    iX = as_uint(va1);
    iX = (iX - iBrkValue);
    /* preserve mantissa, set input exponent to 2^(-10) */
    iR = (iX & iOffExpoMask);
    iN = ((signed int)iX >> (23));
    iR = (iR + iBrkValue);
    FpExpon = ((float)((int)(iN)));
    sR = as_float(iR);
    MinNorm = as_float(__ocl_svml_internal_slog10_ep_data.MinNorm);
    MaxNorm = as_float(__ocl_svml_internal_slog10_ep_data.MaxNorm);
    BrMask1 = as_float(((unsigned int)(-(signed int)(va1 < MinNorm))));
    BrMask2 = as_float(((unsigned int)(-(signed int)(!(va1 <= MaxNorm)))));
    /* check if argument is in [MinNorm, MaxNorm] */
    /* combine and get argument value range mask */
    BrMask1 = as_float((as_uint(BrMask1) | as_uint(BrMask2)));
    BrMask = as_uint(BrMask1);
    vm = 0;
    vm = BrMask;
    One = as_float(__ocl_svml_internal_slog10_ep_data.One);
    // Range reduction R = R - 1
    sR = (sR - One);
    sPoly[4] = as_float(__ocl_svml_internal_slog10_ep_data.ep_Poly[0]);
    sPoly[3] = as_float(__ocl_svml_internal_slog10_ep_data.ep_Poly[1]);
    sP = __spirv_ocl_fma(sPoly[4], sR, sPoly[3]);
    sPoly[2] = as_float(__ocl_svml_internal_slog10_ep_data.ep_Poly[2]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[2]);
    sPoly[1] = as_float(__ocl_svml_internal_slog10_ep_data.ep_Poly[3]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[1]);
    sPoly[0] = as_float(__ocl_svml_internal_slog10_ep_data.ep_Poly[4]);
    sP = __spirv_ocl_fma(sP, sR, sPoly[0]);
    L2 = as_float(__ocl_svml_internal_slog10_ep_data.L2);
    FpExpon = (FpExpon * L2);
    vr1 = __spirv_ocl_fma(sP, sR, FpExpon);
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_slog10_ep(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
