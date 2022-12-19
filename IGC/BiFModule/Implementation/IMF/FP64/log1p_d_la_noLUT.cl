/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long OneEighth;
    unsigned long ExpMask;
    unsigned long ExpInv;
    unsigned long OneHalf;
    unsigned long Eight;
    unsigned long ConvExp;
    unsigned long ConvConst;
    unsigned long One;
    unsigned long Shifter;
    unsigned long ThreeQuarters;
    unsigned long CoeffD7;
    unsigned long CoeffD6;
    unsigned long CoeffD5;
    unsigned long CoeffD4;
    unsigned long CoeffD3;
    unsigned long CoeffD2;
    unsigned long CoeffP12;
    unsigned long CoeffP11;
    unsigned long CoeffP10;
    unsigned long CoeffP9;
    unsigned long CoeffP8;
    unsigned long CoeffP7;
    unsigned long CoeffP6;
    unsigned long CoeffP5;
    unsigned long CoeffP4;
    unsigned long CoeffP3;
    unsigned long CoeffP2;
    unsigned long LogTwoHi;
    unsigned long LogTwoLo;
    unsigned long MinArg;
    unsigned long MaxArg;
} __internal_dlog1p_la_noLUT_data_t;
static __constant __internal_dlog1p_la_noLUT_data_t __internal_dlog1p_la_noLUT_data = {
    // OneEighth = 0.125
    0x3fc0000000000000uL,
    // ExpMask
    0x7ff0000000000000uL,
    // ExpInv
    0x7fe0000000000000uL,
    // OneHalf = 0.5
    0x3fe0000000000000uL,
    // Eight = 8.0
    0x4020000000000000uL,
    // ConvExp
    0x4138000000000000uL,
    // ConvConst
    0x4138040200000000uL,
    // One = 1.0
    0x3ff0000000000000uL,
    // Shifter = 2^49 + 2^48
    0x4308000000000000uL,
    // ThreeQuarters = 0.75
    0x3fe8000000000000uL,
    // Coefficients for polynomial d
    0x3fd2d69f78622232uL,
    0x3fc2896ff3f66bbduL,
    0x3fc8c588ce05a097uL,
    0x3fd013265a806008uL,
    0x3fd55613246dd7cfuL,
    0x3fdfffdc599639e8uL,
    // Coefficients for polynomial p
    0xbfbabc3280883f85uL,
    0x3fb7569b705ccf22uL,
    0xbfb98321c986c73fuL,
    0x3fbc725277fd96beuL,
    0xbfc00011b12ec07fuL,
    0x3fc2492404508eb2uL,
    0xbfc5555548c81da2uL,
    0x3fc9999999f242c7uL,
    0xbfd000000001dd61uL,
    0x3fd5555555554d65uL,
    0xbfdfffffffffffd9uL,
    // Log2hi ~= log(2) (high part)
    0x3fe62e42fefa4000uL,
    // Log2lo ~= log(2) (low part)
    0xbd48432a1b0e2000uL,
    // MinArg
    0xbfefffffffffffffuL,
    // MaxArg
    0x7fefffffffffffffuL
};

/* Macros to access other precomputed constants */
/* Table look-up: all DP constants are presented in hexadecimal form */
static __constant _iml_dp_union_t __dlog1p_la_CoutTab[210] = {
    0x00000000, 0x3FF00000, /* RCPR_Y[0]       =  1.0000000000e+00 */
    0x00000000, 0x00000000, /* LN_RCPR_Y_HI[0] =  0.0000000000e-01 */
    0x00000000, 0x00000000, /* LN_RCPR_Y_LO[0] =  0.0000000000e-01 */
    0x00000000, 0x3FEF8000, /* RCPR_Y[1]       =  9.8437500000e-01 */
    0x58A00000, 0x3F902056, /* LN_RCPR_Y_HI[1] =  1.5748356971e-02 */
    0x6C1B8BDF, 0xBD894F71, /* LN_RCPR_Y_LO[1] = -2.8774507469e-12 */
    0x00000000, 0x3FEF0800, /* RCPR_Y[2]       =  9.6972656250e-01 */
    0x16800000, 0x3F9F7A9B, /* LN_RCPR_Y_HI[2] =  3.0741141556e-02 */
    0x36C720C1, 0xBD7F5EA9, /* LN_RCPR_Y_LO[2] = -1.7831649473e-12 */
    0x00000000, 0x3FEE9000, /* RCPR_Y[3]       =  9.5507812500e-01 */
    0x5A380000, 0x3FA78859, /* LN_RCPR_Y_HI[3] =  4.5962135566e-02 */
    0x3420ECE9, 0xBD74422C, /* LN_RCPR_Y_LO[3] = -1.1515616617e-12 */
    0x00000000, 0x3FEE2000, /* RCPR_Y[4]       =  9.4140625000e-01 */
    0xC0080000, 0x3FAEEA31, /* LN_RCPR_Y_HI[4] =  6.0380510989e-02 */
    0xF93F24EE, 0xBD647844, /* LN_RCPR_Y_LO[4] = -5.8178677744e-13 */
    0x00000000, 0x3FEDB000, /* RCPR_Y[5]       =  9.2773437500e-01 */
    0xF8180000, 0x3FB333D7, /* LN_RCPR_Y_HI[5] =  7.5009821005e-02 */
    0x255F91DF, 0x3D4FA5B5, /* LN_RCPR_Y_LO[5] =  2.2486755795e-13 */
    0x00000000, 0x3FED4000, /* RCPR_Y[6]       =  9.1406250000e-01 */
    0x0AEA0000, 0x3FB700D3, /* LN_RCPR_Y_HI[6] =  8.9856329121e-02 */
    0x8DA99DED, 0x3D681C1E, /* LN_RCPR_Y_LO[6] =  6.8524290121e-13 */
    0x00000000, 0x3FECD800, /* RCPR_Y[7]       =  9.0136718750e-01 */
    0x3ECA0000, 0x3FBA956D, /* LN_RCPR_Y_HI[7] =  1.0384257110e-01 */
    0x29805896, 0x3D6BCC6F, /* LN_RCPR_Y_LO[7] =  7.9008291321e-13 */
    0x00000000, 0x3FEC7000, /* RCPR_Y[8]       =  8.8867187500e-01 */
    0xEE300000, 0x3FBE3707, /* LN_RCPR_Y_HI[8] =  1.1802720609e-01 */
    0x9CCECD58, 0x3D521ED0, /* LN_RCPR_Y_LO[8] =  2.5750595504e-13 */
    0x00000000, 0x3FEC1000, /* RCPR_Y[9]       =  8.7695312500e-01 */
    0xCDCD0000, 0x3FC0CE7E, /* LN_RCPR_Y_HI[9] =  1.3130173730e-01 */
    0x25400ABC, 0xBD5EB9AD, /* LN_RCPR_Y_LO[9] = -4.3663274938e-13 */
    0x00000000, 0x3FEBAC00, /* RCPR_Y[10]       =  8.6474609375e-01 */
    0x0C608000, 0x3FC299D3, /* LN_RCPR_Y_HI[10] =  1.4531934838e-01 */
    0xC311FB94, 0xBD41597F, /* LN_RCPR_Y_LO[10] = -1.2327636329e-13 */
    0x00000000, 0x3FEB5000, /* RCPR_Y[11]       =  8.5351562500e-01 */
    0x9DC98000, 0x3FC4462B, /* LN_RCPR_Y_HI[11] =  1.5839142994e-01 */
    0xD63B93E8, 0x3D59EDE9, /* LN_RCPR_Y_LO[11] =  3.6847821601e-13 */
    0x00000000, 0x3FEAF400, /* RCPR_Y[12]       =  8.4228515625e-01 */
    0xA1A60000, 0x3FC5F830, /* LN_RCPR_Y_HI[12] =  1.7163665669e-01 */
    0xD9C00F91, 0xBD5AB765, /* LN_RCPR_Y_LO[12] = -3.7966284860e-13 */
    0x00000000, 0x3FEA9800, /* RCPR_Y[13]       =  8.3105468750e-01 */
    0x16518000, 0x3FC7B009, /* LN_RCPR_Y_HI[13] =  1.8505967703e-01 */
    0xF2C19F96, 0xBD56B9D7, /* LN_RCPR_Y_LO[13] = -3.2295519303e-13 */
    0x00000000, 0x3FEA4000, /* RCPR_Y[14]       =  8.2031250000e-01 */
    0xDCF70000, 0x3FC95A5A, /* LN_RCPR_Y_HI[14] =  1.9806991376e-01 */
    0x58A0FF6F, 0x3D07F228, /* LN_RCPR_Y_LO[14] =  1.0634128304e-14 */
    0x00000000, 0x3FE9EC00, /* RCPR_Y[15]       =  8.1005859375e-01 */
    0x56110000, 0x3FCAF689, /* LN_RCPR_Y_HI[15] =  2.1064869597e-01 */
    0x117EF527, 0xBD522905, /* LN_RCPR_Y_LO[15] = -2.5807244110e-13 */
    0x00000000, 0x3FE99800, /* RCPR_Y[16]       =  7.9980468750e-01 */
    0x079D8000, 0x3FCC97F8, /* LN_RCPR_Y_HI[16] =  2.2338772175e-01 */
    0xCAE72327, 0xBD5D89D3, /* LN_RCPR_Y_LO[16] = -4.1976573966e-13 */
    0x00000000, 0x3FE94800, /* RCPR_Y[17]       =  7.9003906250e-01 */
    0x7A6B0000, 0x3FCE2A87, /* LN_RCPR_Y_HI[17] =  2.3567288854e-01 */
    0x5DE1C206, 0x3D5608E0, /* LN_RCPR_Y_LO[17] =  3.1313154472e-13 */
    0x00000000, 0x3FE8F800, /* RCPR_Y[18]       =  7.8027343750e-01 */
    0xBE620000, 0x3FCFC218, /* LN_RCPR_Y_HI[18] =  2.4811085983e-01 */
    0x6F1CF6A0, 0x3D34BBA4, /* LN_RCPR_Y_LO[18] =  7.3658333883e-14 */
    0x00000000, 0x3FE8AC00, /* RCPR_Y[19]       =  7.7099609375e-01 */
    0xE97BC000, 0x3FD0A504, /* LN_RCPR_Y_HI[19] =  2.6007197190e-01 */
    0x8CCB79DE, 0xBD47E7B5, /* LN_RCPR_Y_LO[19] = -1.6985605088e-13 */
    0x00000000, 0x3FE86000, /* RCPR_Y[20]       =  7.6171875000e-01 */
    0xCBAD0000, 0x3FD16B5C, /* LN_RCPR_Y_HI[20] =  2.7217788592e-01 */
    0x042D74BF, 0xBD323299, /* LN_RCPR_Y_LO[20] = -6.4651030640e-14 */
    0x00000000, 0x3FE81800, /* RCPR_Y[21]       =  7.5292968750e-01 */
    0xFBEF8000, 0x3FD22981, /* LN_RCPR_Y_HI[21] =  2.8378343204e-01 */
    0x609580DA, 0xBD3A1421, /* LN_RCPR_Y_LO[21] = -9.2649920791e-14 */
    0x00000000, 0x3FE7D000, /* RCPR_Y[22]       =  7.4414062500e-01 */
    0xBCE12000, 0x3FD2E9E2, /* LN_RCPR_Y_HI[22] =  2.9552524991e-01 */
    0x128D1DC2, 0x3D24300C, /* LN_RCPR_Y_LO[22] =  3.5860530920e-14 */
    0x00000000, 0x3FE78C00, /* RCPR_Y[23]       =  7.3583984375e-01 */
    0x802F4000, 0x3FD3A1AC, /* LN_RCPR_Y_HI[23] =  3.0674278753e-01 */
    0x6332A139, 0xBD433898, /* LN_RCPR_Y_LO[23] = -1.3657395391e-13 */
    0x00000000, 0x3FE74400, /* RCPR_Y[24]       =  7.2705078125e-01 */
    0xF41F0000, 0x3FD4668B, /* LN_RCPR_Y_HI[24] =  3.1875895348e-01 */
    0x62C5A8F7, 0xBD39AF17, /* LN_RCPR_Y_LO[24] = -9.1247722585e-14 */
    0x00000000, 0x3FE70400, /* RCPR_Y[25]       =  7.1923828125e-01 */
    0x9AB56000, 0x3FD5178D, /* LN_RCPR_Y_HI[25] =  3.2956256970e-01 */
    0x780B4E27, 0xBD451F56, /* LN_RCPR_Y_LO[25] = -1.5008377233e-13 */
    0x00000000, 0x3FE6C000, /* RCPR_Y[26]       =  7.1093750000e-01 */
    0xDF596000, 0x3FD5D5BD, /* LN_RCPR_Y_HI[26] =  3.4117075740e-01 */
    0x08A465DC, 0xBD0A0B2A, /* LN_RCPR_Y_LO[26] = -1.1565686246e-14 */
    0x00000000, 0x3FE68000, /* RCPR_Y[27]       =  7.0312500000e-01 */
    0x3E9C6000, 0x3FD68AC8, /* LN_RCPR_Y_HI[27] =  3.5222059359e-01 */
    0xC9D5BAE8, 0x3D442834, /* LN_RCPR_Y_LO[27] =  1.4322449351e-13 */
    0x00000000, 0x3FE64400, /* RCPR_Y[28]       =  6.9580078125e-01 */
    0x0F3AE000, 0x3FD73658, /* LN_RCPR_Y_HI[28] =  3.6269189346e-01 */
    0x9D367ECC, 0x3D46CAA5, /* LN_RCPR_Y_LO[28] =  1.6194398406e-13 */
    0x00000000, 0x3FE60400, /* RCPR_Y[29]       =  6.8798828125e-01 */
    0x61CC6000, 0x3FD7EF58, /* LN_RCPR_Y_HI[29] =  3.7398347426e-01 */
    0x353DDE1F, 0x3D4CEF42, /* LN_RCPR_Y_LO[29] =  2.0559272687e-13 */
    0x00000000, 0x3FE5C800, /* RCPR_Y[30]       =  6.8066406250e-01 */
    0xAF432000, 0x3FD89EB3, /* LN_RCPR_Y_HI[30] =  3.8468639484e-01 */
    0x30791139, 0x3D40E8B0, /* LN_RCPR_Y_LO[30] =  1.2014523375e-13 */
    0x00000000, 0x3FE59000, /* RCPR_Y[31]       =  6.7382812500e-01 */
    0x34A04000, 0x3FD94414, /* LN_RCPR_Y_HI[31] =  3.9478020801e-01 */
    0x8DA8002F, 0xBD4B4D1F, /* LN_RCPR_Y_LO[31] = -1.9398713900e-13 */
    0x00000000, 0x3FE55400, /* RCPR_Y[32]       =  6.6650390625e-01 */
    0x0CC0E000, 0x3FD9F724, /* LN_RCPR_Y_HI[32] =  4.0570927854e-01 */
    0xB174879D, 0x3D4B6291, /* LN_RCPR_Y_LO[32] =  1.9458238115e-13 */
    0x00000000, 0x3FE51C00, /* RCPR_Y[33]       =  6.5966796875e-01 */
    0xAE22A000, 0x3FDAA00C, /* LN_RCPR_Y_HI[33] =  4.1601864820e-01 */
    0x1F2FADAB, 0x3D46245C, /* LN_RCPR_Y_LO[33] =  1.5732859046e-13 */
    0x00000000, 0x3FE4E400, /* RCPR_Y[34]       =  6.5283203125e-01 */
    0xBDF08000, 0x3FDB4AB7, /* LN_RCPR_Y_HI[34] =  4.2643540906e-01 */
    0x8C3E8673, 0x3D364639, /* LN_RCPR_Y_LO[34] =  7.9134265754e-14 */
    0x00000000, 0x3FE4B000, /* RCPR_Y[35]       =  6.4648437500e-01 */
    0x9E272000, 0x3FDBEACD, /* LN_RCPR_Y_HI[35] =  4.3620624966e-01 */
    0x923C3257, 0xBD34BAC8, /* LN_RCPR_Y_LO[35] = -7.3646415096e-14 */
    0x00000000, 0x3FE47C00, /* RCPR_Y[36]       =  6.4013671875e-01 */
    0xE019C000, 0x3FDC8C77, /* LN_RCPR_Y_HI[36] =  4.4607350240e-01 */
    0xDBB1EB43, 0xBD44FF3B, /* LN_RCPR_Y_LO[36] = -1.4919270877e-13 */
    0x00000000, 0x3FE44800, /* RCPR_Y[37]       =  6.3378906250e-01 */
    0x93204000, 0x3FDD2FBE, /* LN_RCPR_Y_HI[37] =  4.5603908890e-01 */
    0x5E1C3CAA, 0xBD4671F5, /* LN_RCPR_Y_LO[37] = -1.5948238472e-13 */
    0x00000000, 0x3FE41400, /* RCPR_Y[38]       =  6.2744140625e-01 */
    0x04E1C000, 0x3FDDD4AA, /* LN_RCPR_Y_HI[38] =  4.6610498883e-01 */
    0x2DF01AFD, 0x3D32D851, /* LN_RCPR_Y_LO[38] =  6.6950849131e-14 */
    0x00000000, 0x3FE3E400, /* RCPR_Y[39]       =  6.2158203125e-01 */
    0xA6DA4000, 0x3FDE6E62, /* LN_RCPR_Y_HI[39] =  4.7548738760e-01 */
    0xE3D32749, 0x3D4CA9FB, /* LN_RCPR_Y_LO[39] =  2.0366996825e-13 */
    0x00000000, 0x3FE3B000, /* RCPR_Y[40]       =  6.1523437500e-01 */
    0x7FB06000, 0x3FDF168F, /* LN_RCPR_Y_HI[40] =  4.8575198621e-01 */
    0x0A7C0C6E, 0xBD2D6FB4, /* LN_RCPR_Y_LO[40] = -5.2289445586e-14 */
    0x00000000, 0x3FE38000, /* RCPR_Y[41]       =  6.0937500000e-01 */
    0xAF7A4000, 0x3FDFB358, /* LN_RCPR_Y_HI[41] =  4.9532143723e-01 */
    0xA3C16493, 0x3D41085F, /* LN_RCPR_Y_LO[41] =  1.2102467896e-13 */
    0x00000000, 0x3FE35400, /* RCPR_Y[42]       =  6.0400390625e-01 */
    0xCCB34800, 0x3FE02232, /* LN_RCPR_Y_HI[42] =  5.0417461377e-01 */
    0xFA057734, 0xBD2DC38B, /* LN_RCPR_Y_LO[42] = -5.2871226728e-14 */
    0x00000000, 0x3FE32400, /* RCPR_Y[43]       =  5.9814453125e-01 */
    0x5C40E000, 0x3FE0720E, /* LN_RCPR_Y_HI[43] =  5.1392286318e-01 */
    0xFD3F0109, 0xBD1C762F, /* LN_RCPR_Y_LO[43] = -2.5279040867e-14 */
    0x00000000, 0x3FE2F800, /* RCPR_Y[44]       =  5.9277343750e-01 */
    0xFD23E000, 0x3FE0BBF2, /* LN_RCPR_Y_HI[44] =  5.2294301454e-01 */
    0xA94A1946, 0xBD35F8BF, /* LN_RCPR_Y_LO[44] = -7.8059068610e-14 */
    0x00000000, 0x3FE2C800, /* RCPR_Y[45]       =  5.8691406250e-01 */
    0xCBC08000, 0x3FE10D53, /* LN_RCPR_Y_HI[45] =  5.3287687106e-01 */
    0xB54F6AF7, 0x3D1EFC5C, /* LN_RCPR_Y_LO[45] =  2.7520909654e-14 */
    0x00000000, 0x3FE2A000, /* RCPR_Y[46]       =  5.8203125000e-01 */
    0xF6F29800, 0x3FE151C3, /* LN_RCPR_Y_HI[46] =  5.4123113853e-01 */
    0xA293AE49, 0xBD2EDD97, /* LN_RCPR_Y_LO[46] = -5.4828310811e-14 */
    0x00000000, 0x3FE27400, /* RCPR_Y[47]       =  5.7666015625e-01 */
    0xBA0BA800, 0x3FE19DB6, /* LN_RCPR_Y_HI[47] =  5.5050216996e-01 */
    0xBD2DAECC, 0xBD324C53, /* LN_RCPR_Y_LO[47] = -6.5008097591e-14 */
    0x00000000, 0x3FE24800, /* RCPR_Y[48]       =  5.7128906250e-01 */
    0x6E70E800, 0x3FE1EA5F, /* LN_RCPR_Y_HI[48] =  5.5985995837e-01 */
    0xEB80651C, 0x3D3C1747, /* LN_RCPR_Y_LO[48] =  9.9799070913e-14 */
    0x00000000, 0x3FE22000, /* RCPR_Y[49]       =  5.6640625000e-01 */
    0xD8BEC000, 0x3FE230B0, /* LN_RCPR_Y_HI[49] =  5.6844370206e-01 */
    0x646DE661, 0xBD3B40FE, /* LN_RCPR_Y_LO[49] = -9.6825238382e-14 */
    0x00000000, 0x3FE1F800, /* RCPR_Y[50]       =  5.6152343750e-01 */
    0x1EC94000, 0x3FE2779E, /* LN_RCPR_Y_HI[50] =  5.7710176480e-01 */
    0x1994C90F, 0xBD235B99, /* LN_RCPR_Y_LO[50] = -3.4386369076e-14 */
    0x00000000, 0x3FE1D000, /* RCPR_Y[51]       =  5.5664062500e-01 */
    0xF9842000, 0x3FE2BF29, /* LN_RCPR_Y_HI[51] =  5.8583544477e-01 */
    0x79E2C481, 0xBD3E275C, /* LN_RCPR_Y_LO[51] = -1.0712765723e-13 */
    0x00000000, 0x3FE1A800, /* RCPR_Y[52]       =  5.5175781250e-01 */
    0x344F1000, 0x3FE30757, /* LN_RCPR_Y_HI[52] =  5.9464607445e-01 */
    0x533A1F99, 0xBD2EC82F, /* LN_RCPR_Y_LO[52] = -5.4679766712e-14 */
    0x00000000, 0x3FE18000, /* RCPR_Y[53]       =  5.4687500000e-01 */
    0xAD9D9000, 0x3FE35028, /* LN_RCPR_Y_HI[53] =  6.0353502187e-01 */
    0x1AB60655, 0xBD3BD1F0, /* LN_RCPR_Y_LO[53] = -9.8836743062e-14 */
    0x00000000, 0x3FE15C00, /* RCPR_Y[54]       =  5.4248046875e-01 */
    0xDDE5D000, 0x3FE39240, /* LN_RCPR_Y_HI[54] =  6.1160319652e-01 */
    0x2A914E99, 0xBD26D848, /* LN_RCPR_Y_LO[54] = -4.0580607621e-14 */
    0x00000000, 0x3FE13400, /* RCPR_Y[55]       =  5.3759765625e-01 */
    0x96586000, 0x3FE3DC52, /* LN_RCPR_Y_HI[55] =  6.2064484944e-01 */
    0xC05BC7A1, 0xBD262793, /* LN_RCPR_Y_LO[55] = -3.9354472139e-14 */
    0x00000000, 0x3FE11000, /* RCPR_Y[56]       =  5.3320312500e-01 */
    0xF8472000, 0x3FE41F8F, /* LN_RCPR_Y_HI[56] =  6.2885282985e-01 */
    0x5166B2E1, 0xBD34F784, /* LN_RCPR_Y_LO[56] = -7.4489260136e-14 */
    0x00000000, 0x3FE0EC00, /* RCPR_Y[57]       =  5.2880859375e-01 */
    0xCF40E000, 0x3FE4635B, /* LN_RCPR_Y_HI[57] =  6.3712873916e-01 */
    0x15F69AA9, 0xBD318B95, /* LN_RCPR_Y_LO[57] = -6.2333227002e-14 */
    0x00000000, 0x3FE0C800, /* RCPR_Y[58]       =  5.2441406250e-01 */
    0x7BF1F800, 0x3FE4A7B8, /* LN_RCPR_Y_HI[58] =  6.4547371109e-01 */
    0x4EB6653D, 0x3D34123A, /* LN_RCPR_Y_LO[58] =  7.1307234611e-14 */
    0x00000000, 0x3FE0A800, /* RCPR_Y[59]       =  5.2050781250e-01 */
    0x32C56000, 0x3FE4E4F8, /* LN_RCPR_Y_HI[59] =  6.5295038143e-01 */
    0xDCAF29D2, 0x3D1BADBD, /* LN_RCPR_Y_LO[59] =  2.4583604765e-14 */
    0x00000000, 0x3FE08400, /* RCPR_Y[60]       =  5.1611328125e-01 */
    0x269BC800, 0x3FE52A6D, /* LN_RCPR_Y_HI[60] =  6.6142900029e-01 */
    0x2E12EC6D, 0xBD2FFBBB, /* LN_RCPR_Y_LO[60] = -5.6813797915e-14 */
    0x00000000, 0x3FE06400, /* RCPR_Y[61]       =  5.1220703125e-01 */
    0x0194F000, 0x3FE568AA, /* LN_RCPR_Y_HI[61] =  6.6902637776e-01 */
    0x8CAE0304, 0xBD3C89DB, /* LN_RCPR_Y_LO[61] = -1.0138914174e-13 */
    0x00000000, 0x3FE04000, /* RCPR_Y[62]       =  5.0781250000e-01 */
    0x5C364800, 0x3FE5AF40, /* LN_RCPR_Y_HI[62] =  6.7764299402e-01 */
    0xAC10C9FB, 0x3D2DFA63, /* LN_RCPR_Y_LO[62] =  5.3251773437e-14 */
    0x00000000, 0x3FE02000, /* RCPR_Y[63]       =  5.0390625000e-01 */
    0xAA241800, 0x3FE5EE82, /* LN_RCPR_Y_HI[63] =  6.8536504012e-01 */
    0x0CDA46BE, 0x3D220238, /* LN_RCPR_Y_LO[63] =  3.1989820141e-14 */
    0x00000000, 0x3FE00000, /* RCPR_Y[64]       =  5.0000000000e-01 */
    0xFEFA3800, 0x3FE62E42, /* LN_RCPR_Y_HI[64] =  6.9314718056e-01 */
    0x93C76730, 0x3D2EF357, /* LN_RCPR_Y_LO[64] =  5.4979230187e-14 */
    /* Two parts of the ln(2.0) */
    0xFEFA3800, 0x3FE62E42, /* LN2_HI = 6.931471805598903e-01 */
    0x93C76730, 0x3D2EF357, /* LN2_LO = 5.497923018708371e-14 */
    /* Right Shifter to obtain j */
    0x00000040, 0x42D00000, /* RSJ = 2^(46)+1 */
    /* Right Shifter to obtain YHi */
    0x00000000, 0x41400000, /* RSY = 2^(21) */
    /* "Near 1" path (bound for u=1-x) */
    0x00000000, 0x3F840000, /* NEAR0_BOUND */
    /* Scale for denormalized arguments */
    0x00000000, 0x43B00000, /* DENORM_SCALE = 2^60 */
    /* Double precision constants: 0.0, 1.0 */
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
/*
//      Coefficients for (ln(1+u) - u)/u^2 approximation by
//        polynomial p0(u)
*/
    0x00000000, 0xBFE00000, /* A0 = -.500000000000000 */
    0x55555613, 0x3FD55555, /* A1 =  .333333333333344 */
    0x00000262, 0xBFD00000, /* A2 = -.250000000000034 */
    0x97B36C81, 0x3FC99999, /* A3 =  .199999999115651 */
    0x5228B38F, 0xBFC55555, /* A4 = -.166666665188498 */
    0x2481059D, 0x3FC249C0, /* A5 =  .142875688385215 */
    0x05A2836D, 0xBFC000B4  /* A6 = -.125021460296036 */
};

/*
//
//   Implementation of HA (High Accuracy) version of double precision vector
//   natural logarithm function starts here.
//
*/
inline int __ocl_svml_internal_dlog1p_noLUT_cout (double *a0, double *r)
{
    double x, y, u, da, *a = (&da);
    double dbP;
    double dbAbsU;
    double dbN, dbNLn2Hi, dbNLn2Lo;
    double dbRcprY, dbLnRcprYHi, dbLnRcprYLo, dbWHi, dbWLo;
    double dbYHi, dbYLo, dbUHi, dbuLo, dbResHi, dbResLo;
    double dbTmp;
    int iN, j;
    int i;
    int nRet = 0;
    (*a) = (*a0) + 1.0;
    /* Filter out Infs and NaNs */
    if ((((((_iml_dp_union_t *) & a[0])->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {
        /* Here if argument is finite double precision number */
/*
//          Copy argument into temporary variable x,
//            and initially set iN equal to 0
*/
        x = a[0];
        iN = 0;
        /* Check if x is denormalized number or [+/-]0 */
        if (((((_iml_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) == 0)
        {
            /* Here if argument is denormalized or [+/-]0 */
            /* Scale x and properly adjust iN */
            x *= ((__constant double *) __dlog1p_la_CoutTab)[200];
            iN -= 60;
        }
        /* Starting from this point x is finite normalized number */
        if (x > ((__constant double *) __dlog1p_la_CoutTab)[201])
        {
            /* Here if x is positive finite normalized number */
            /* Get absolute value of u=x-1 */
            u = (x - 1.0);
            dbAbsU = u;
            (((_iml_dp_union_t *) & dbAbsU)->dwords.hi_dword =
             (((_iml_dp_union_t *) & dbAbsU)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
            /* Check if a[0] falls into "Near 1" range */
            if (dbAbsU > ((__constant double *) __dlog1p_la_CoutTab)[199])
            {
                /* 6) "Main" path */
                /* a) Range reduction */
                /* Get N taking into account denormalized arguments */
                iN += ((((_iml_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) - 0x3FF;
                dbN = (double) iN;
/*
//                  Compute N*Ln2Hi and N*Ln2Lo. Notice that N*Ln2Hi
//                    is error-free for any N
*/
                dbNLn2Hi = (dbN * ((__constant double *) __dlog1p_la_CoutTab)[195]);
                dbNLn2Lo = (dbN * ((__constant double *) __dlog1p_la_CoutTab)[196]);
                /* Get y */
                y = x;
                (((_iml_dp_union_t *) & y)->dwords.hi_dword =
                 (((_iml_dp_union_t *) & y)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));
                /* Obtain j */
                dbTmp = (y + ((__constant double *) __dlog1p_la_CoutTab)[197]);
                j = (((_iml_dp_union_t *) & dbTmp)->dwords.lo_dword) & ((1 << (6 + 1)) - 1);
                /* Get table values of RcprY, LnRcprYHi, LnRcprYLo */
                dbRcprY = ((__constant double *) __dlog1p_la_CoutTab)[3 * (j)];
                dbLnRcprYHi = ((__constant double *) __dlog1p_la_CoutTab)[3 * (j) + 1];
                dbLnRcprYLo = ((__constant double *) __dlog1p_la_CoutTab)[3 * (j) + 2];
                /* Calculate WHi and WLo */
                dbWHi = (dbNLn2Hi + dbLnRcprYHi);
                dbWLo = (dbNLn2Lo + dbLnRcprYLo);
                /* Get YHi and YLo */
                dbTmp = (y + ((__constant double *) __dlog1p_la_CoutTab)[198]);
                dbYHi = (dbTmp - ((__constant double *) __dlog1p_la_CoutTab)[198]);
                dbYLo = (y - dbYHi);
                /* Get UHi, uLo and U */
                dbUHi = ((dbRcprY * dbYHi) - 1.0);
                dbuLo = (dbRcprY * dbYLo);
                u = (dbUHi + dbuLo);
                /* b) Approximation */
                dbP =
                    (((((((__constant double *) __dlog1p_la_CoutTab)[209] * u + ((__constant double *) __dlog1p_la_CoutTab)[208]) * u +
                        ((__constant double *) __dlog1p_la_CoutTab)[207]) * u + ((__constant double *) __dlog1p_la_CoutTab)[206]) * u +
                      ((__constant double *) __dlog1p_la_CoutTab)[205]) * u + ((__constant double *) __dlog1p_la_CoutTab)[204]) * u +
                    ((__constant double *) __dlog1p_la_CoutTab)[203];
                dbP = (dbP * u * u);
                /* c) Reconstruction */
                dbResHi = (dbWHi + dbUHi);
                dbResLo = ((dbWLo + dbuLo) + dbP);
                r[0] = (dbResHi + dbResLo);
            }
            else
            {
                /* 5) "Near 1" path (|u|<=NEAR0_BOUND) */
                dbP =
                    (((((((__constant double *) __dlog1p_la_CoutTab)[209] * u + ((__constant double *) __dlog1p_la_CoutTab)[208]) * u +
                        ((__constant double *) __dlog1p_la_CoutTab)[207]) * u + ((__constant double *) __dlog1p_la_CoutTab)[206]) * u +
                      ((__constant double *) __dlog1p_la_CoutTab)[205]) * u + ((__constant double *) __dlog1p_la_CoutTab)[204]) * u +
                    ((__constant double *) __dlog1p_la_CoutTab)[203];
                dbP = (dbP * u * u);
                dbP = (dbP + u);
                r[0] = dbP;
            }
        }
        else
        {
            /* Path 3) or 4). Here if argument is negative number or +/-0 */
            if (x == ((__constant double *) __dlog1p_la_CoutTab)[201])
            {
                /* Path 3). Here if argument is +/-0 */
                r[0] = -((__constant double *) __dlog1p_la_CoutTab)[202] / ((__constant double *) __dlog1p_la_CoutTab)[201];
                nRet = 2;
            }
            else
            {
                /* Path 4). Here if argument is negative number */
                r[0] = ((__constant double *) __dlog1p_la_CoutTab)[201] / ((__constant double *) __dlog1p_la_CoutTab)[201];
                nRet = 1;
            }
        }
    }
    else
    {
        /* Path 1) or 2). Here if argument is NaN or +/-Infinity */
        if (((((_iml_dp_union_t *) & a[0])->dwords.hi_dword >> 31) == 1)
            && (((((_iml_dp_union_t *) & a[0])->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & a[0])->dwords.lo_dword) == 0)))
        {
            /* Path 2). Here if argument is -Infinity */
            r[0] = ((__constant double *) __dlog1p_la_CoutTab)[201] / ((__constant double *) __dlog1p_la_CoutTab)[201];
            nRet = 1;
        }
        else
        {
            /* Path 1). Here if argument is NaN or +Infinity */
            r[0] = a[0] * a[0];
        }
    }
    return nRet;
}

double __ocl_svml_log1p_noLUT (double a)
{
    unsigned int vm;
    double va1;
    double vr1;
    double r;
    va1 = a;;
    {
        double OneEighth;
        double t;
        unsigned long ExpMask;
        unsigned long ExpInv;
        unsigned long lt;
        unsigned long te;
        unsigned long ls;
        double s;
        double OneHalf;
        double tt;
        unsigned long ltt;
        double ss;
        double Eight;
        unsigned long ConvExp;
        double ConvConst;
        double sss;
        double k;
        double One;
        double q;
        double r;
        double Shifter;
        double ThreeQuarters;
        double sm;
        double mm;
        double m;
        double b;
        double nz;
        double z;
        double w;
        double ww;
        double v;
        double dg;
        double du;
        double cdg;
        double cdu;
        double dd;
        double pg;
        double pu;
        double pgg;
        double pug;
        double pgu;
        double puu;
        double cpgg;
        double cpug;
        double cpgu;
        double cpuu;
        double pp;
        double a;
        double mpzhi;
        double mpzlo;
        double LogTwoHi;
        double LogTwoLo;
        double lkhi;
        double lklo;
        double tts;
        double yhi;
        double tlo;
        double ttlo;
        double tttlo;
        double ttttlo;
        double ylo;
        double y;
        double MinArg;
        double MaxArg;
        double MaskArg;
        unsigned long MaskArgTmp;
        double MaskMin;
        double MaskMax;
// Compute t  = round(0.125 * x + 0.125) ~= 1/8 * (x + 1.0)
        OneEighth = as_double (__internal_dlog1p_la_noLUT_data.OneEighth);
        t = (OneEighth * va1);
        t = (t + OneEighth);
// We have 2^-56 <= t < 2^1021
//
// Compute exponent E of t
//
// Compute s = 2^-E
//
// We have s an integer power of two such that
//
// 2^-1023 <= s <= 2^53
//
        ExpMask = (__internal_dlog1p_la_noLUT_data.ExpMask);
        ExpInv = (__internal_dlog1p_la_noLUT_data.ExpInv);
        lt = as_ulong (t);
        te = (lt & ExpMask);
        ls = (ExpInv - te);
        s = as_double (ls);
// Compute tt = round(s * t + 0.5)
//
        OneHalf = as_double (__internal_dlog1p_la_noLUT_data.OneHalf);
        tt = (s * t);
        tt = (tt + OneHalf);
// Rescale s to make it become s = 2^-E * 0.125
        s = (s * OneEighth);
// We have 1.5 <= tt <= 2.5
//
// Compute exponent F of tt
//
// We have F \in { 0, 1 }
//
// Compute ss = s * 2^-F
//
        ltt = as_ulong (tt);
        te = (ltt & ExpMask);
        ls = (ExpInv - te);
        ss = as_double (ls);
        ss = (ss * s);
// We have ss an integer power of two such that
//
// 2^-1024 <= ss <= 2^53
//
// We compute k = -log2(ss)  where k is a FP number
//
// We have k an integer such that 53 <= k <= 1024.
//
        Eight = as_double (__internal_dlog1p_la_noLUT_data.Eight);
        ConvExp = (__internal_dlog1p_la_noLUT_data.ConvExp);
        ConvConst = as_double (__internal_dlog1p_la_noLUT_data.ConvConst);
        sss = (ss * Eight);
        ls = as_ulong (sss);
        ls = ((unsigned long) (ls) >> (20));
        ls = (ls | ConvExp);
        sss = as_double (ls);
        k = (ConvConst - sss);
// Now we compute
//
// r = round(ss * x - round(1 - ss))
//
// That quantity is often exact.
//
        One = as_double (__internal_dlog1p_la_noLUT_data.One);
        q = (One - ss);
        r = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ss, va1, -(q));
// We have -0.25 <= r <= 0.5
//
// We compute
//
// sm = round((2^49 + 2^48) + r)
// mm = round(sm - (2^49 + 2^48))
// m  = round(mm * 0.75)
//
// It is pretty easy to show that
//
// m is "close" to 3/4 * nearestint(8.0 * r) * 0.125
//
        Shifter = as_double (__internal_dlog1p_la_noLUT_data.Shifter);
        ThreeQuarters = as_double (__internal_dlog1p_la_noLUT_data.ThreeQuarters);
        sm = (Shifter + r);
        mm = (sm - Shifter);
        m = (mm * ThreeQuarters);
// Compute
//
// b = m - r
// z = -round(r * m + b)
//
        b = (m - r);
        nz = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (r, m, b);
        z = (-(nz));
// We will need m + z = r * (1 - m) later.
//
// We compute it as mpzhi + mpzlo
//
// The subtraction 1 - m is exact (only a couple of
// cases to test).
//
        a = (One - m);
        mpzhi = (r * a);
        mpzlo = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (r, a, -(mpzhi));
// It can be shown that -114/1000 <= z <= 78/1000
//
// We have (in the absence of rounding errors)
//
// log(1 + x) = k * log(2) + (-log(1 - m)) + log(1 + z)
//
// We replace -log(1 - m) by a polynomial that
// needs to be accurate only at the couple of points m
// that are possible.
//
// We replace log(1 + z) by another polynomial.
//
// We will be running the two polynomial evaluations in
// parallel.
//
// We start by computing w = z^2 and v = m^2 in native precision
//
        w = (nz * nz);
        ww = (w * w);
        v = (m * m);
// Polynomial evaluation of d(m) = m + v * dd(m)
//
        du = as_double (__internal_dlog1p_la_noLUT_data.CoeffD7);
        dg = as_double (__internal_dlog1p_la_noLUT_data.CoeffD6);
        cdu = as_double (__internal_dlog1p_la_noLUT_data.CoeffD5);
        cdg = as_double (__internal_dlog1p_la_noLUT_data.CoeffD4);
        du = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (v, du, cdu);
        dg = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (v, dg, cdg);
        cdu = as_double (__internal_dlog1p_la_noLUT_data.CoeffD3);
        cdg = as_double (__internal_dlog1p_la_noLUT_data.CoeffD2);
        du = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (v, du, cdu);
        dg = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (v, dg, cdg);
        dd = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (m, du, dg);
// Polynomial evaluation of p(z) = z + w * pp(z)
//
        pgg = as_double (__internal_dlog1p_la_noLUT_data.CoeffP12);
        puu = as_double (__internal_dlog1p_la_noLUT_data.CoeffP11);
        pug = as_double (__internal_dlog1p_la_noLUT_data.CoeffP10);
        pgu = as_double (__internal_dlog1p_la_noLUT_data.CoeffP9);
        cpgg = as_double (__internal_dlog1p_la_noLUT_data.CoeffP8);
        cpuu = as_double (__internal_dlog1p_la_noLUT_data.CoeffP7);
        cpug = as_double (__internal_dlog1p_la_noLUT_data.CoeffP6);
        cpgu = as_double (__internal_dlog1p_la_noLUT_data.CoeffP5);
        pgg = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, pgg, cpgg);
        puu = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, puu, cpuu);
        pug = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, pug, cpug);
        pgu = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, pgu, cpgu);
        cpgg = as_double (__internal_dlog1p_la_noLUT_data.CoeffP4);
        cpuu = as_double (__internal_dlog1p_la_noLUT_data.CoeffP3);
        cpug = as_double (__internal_dlog1p_la_noLUT_data.CoeffP2);
        pgg = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, pgg, cpgg);
        puu = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, puu, cpuu);
        pug = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (ww, pug, cpug);
        pg = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (w, pgg, pug);
        pu = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (w, pgu, puu);
        pp = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (z, pu, pg);
// Multiplication of LogTwoHi + LogTwoLo with k
//
// k holds on 12 bits. The two constants have trailing
// zeros. Elementwise multiplication is exact
//
        LogTwoHi = as_double (__internal_dlog1p_la_noLUT_data.LogTwoHi);
        LogTwoLo = as_double (__internal_dlog1p_la_noLUT_data.LogTwoLo);
        lkhi = (k * LogTwoHi);
        lklo = (k * LogTwoLo);
// Here we have to do the final summation
//
// log(1 + x) ~= lkhi + lklo + m + dd + z + pp
//             = (lkhi + mpzhi) + (mpzlo + lklo + dd + pp)
//
        yhi = (lkhi + mpzhi);
        tts = (yhi - lkhi);
        tlo = (mpzhi - tts);
// Lower parts
        ttlo = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (pp, w, lklo);
        tttlo = SPIRV_OCL_BUILTIN (fma, _f64_f64_f64,) (dd, v, mpzlo);
        ttttlo = (ttlo + tttlo);
        ylo = (tlo + ttttlo);
// Final summation
        vr1 = (yhi + ylo);
// Compute rangemask for main-path arguments: -1.0 < x < 2^1024
        MinArg = as_double (__internal_dlog1p_la_noLUT_data.MinArg);
        MaxArg = as_double (__internal_dlog1p_la_noLUT_data.MaxArg);
        MaskMin = as_double ((unsigned long) ((va1 < MinArg) ? 0xffffffffffffffff : 0x0));
        MaskMax = as_double ((unsigned long) (((!(va1 <= MaxArg)) ? 0xffffffffffffffff : 0x0)));
        MaskArg = as_double ((as_ulong (MaskMin) | as_ulong (MaskMax)));
        MaskArgTmp = as_ulong (MaskArg);
        vm = 0;
        vm = MaskArgTmp;
    }
    if (__builtin_expect ((vm) != 0, 0))
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __ocl_svml_internal_dlog1p_noLUT_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;
    return r;
}
