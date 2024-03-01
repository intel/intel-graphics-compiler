/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//      For    0.0    <= x <=  7.0/16.0: atan(x) = atan(0.0) + atan(s), where
s=(x-0.0)/(1.0+0.0*x)
//      For  7.0/16.0 <= x <= 11.0/16.0: atan(x) = atan(0.5) + atan(s), where
s=(x-0.5)/(1.0+0.5*x)
//      For 11.0/16.0 <= x <= 19.0/16.0: atan(x) = atan(1.0) + atan(s), where
s=(x-1.0)/(1.0+1.0*x)
//      For 19.0/16.0 <= x <= 39.0/16.0: atan(x) = atan(1.5) + atan(s), where
s=(x-1.5)/(1.0+1.5*x)
//      For 39.0/16.0 <= x <=    inf   : atan(x) = atan(inf) + atan(s), where
s=-1.0/x
//      Where atan(s) ~= s+s^3*Poly11(s^2) on interval |s|<7.0/0.16.
// --
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned long ATAN_TBL[6][4];
  unsigned long dAtan1o2Hi;
  unsigned long dAtan1o2Lo;
  unsigned long dAtanOneHi;
  unsigned long dAtanOneLo;
  unsigned long dAtan3o2Hi;
  unsigned long dAtan3o2Lo;
  unsigned long dAtanInfHi;
  unsigned long dAtanInfLo;
  unsigned long dHalf;
  unsigned long dThreeHalf;
  unsigned long dSIGN_MASK;
  unsigned long dABS_MASK;
  unsigned long dPiHI;
  unsigned long dPiLO;
  unsigned long dHIGH_26_MASK;
  unsigned long dATAN_BOUND1;
  unsigned long dATAN_BOUND3;
  unsigned long dATAN_BOUND4;
  unsigned long dATAN_BOUND2;
  unsigned int i4;
  unsigned int idEXP_MASK;
  unsigned int id2_BIAS;
  unsigned int isMANTISSA_MASK;
  unsigned int isONE;
  unsigned int idBIAS;
  unsigned long dONE;
  unsigned long dATAN_A11;
  unsigned long dATAN_A10;
  unsigned long dATAN_A09;
  unsigned long dATAN_A08;
  unsigned long dATAN_A07;
  unsigned long dATAN_A06;
  unsigned long dATAN_A05;
  unsigned long dATAN_A04;
  unsigned long dATAN_A03;
  unsigned long dATAN_A02;
  unsigned long dATAN_A01;
  unsigned long dATAN_A00;
  unsigned int iABSMASK;
  unsigned int iCHK_WORK_SUB;
  unsigned int iCHK_WORK_CMP;
  unsigned long dABSMASK;
  unsigned long dZERO;
  unsigned int sONE;
  unsigned long dHIGH_20_MASK;
  unsigned long dPI;
  unsigned long dPIO2;
  unsigned int iBias;
  unsigned int iZero;
} __ocl_svml_internal_datan2_ha_data_t;
static __ocl_svml_internal_datan2_ha_data_t __ocl_svml_internal_datan2_ha_data =
    {
        {
            {0x3FF0000000000000uL, 0x3FF921FB54442D18uL, 0x3C91A62633145C07uL,
             0x0000000000000000uL},
            {0x3FF8000000000000uL, 0x3FEF730BD281F69BuL, 0x3C7007887AF0CBBDuL,
             0xffffffffffffffffuL},
            {0x3FF0000000000000uL, 0x3FE921FB54442D18uL, 0x3C81A62633145C07uL,
             0xffffffffffffffffuL},
            {0x3FE0000000000000uL, 0x3FDDAC670561BB4FuL, 0x3C7A2B7F222F65E2uL,
             0xffffffffffffffffuL},
            {0x0000000000000000uL, 0x0000000000000000uL, 0x0000000000000000uL,
             0xffffffffffffffffuL},
            {0x0000000000000000uL, 0x0000000000000000uL, 0x0000000000000000uL,
             0x0000000000000000uL},
        },
        0x3FDDAC670561BB4FuL, // dAtan1o2Hi
        0x3C7A2B7F222F65E2uL, // dAtan1o2Lo
        0x3FE921FB54442D18uL, // dAtanOneHi
        0x3C81A62633145C07uL, // dAtanOneLo
        0x3FEF730BD281F69BuL, // dAtan3o2Hi
        0x3C7007887AF0CBBDuL, // dAtan3o2Lo
        0x3FF921FB54442D18uL, // dAtanInfHi
        0x3C91A62633145C07uL, // dAtanInfLo
        0x3FE0000000000000uL, // dHalf
        0x3FF8000000000000uL, // dThreeHalf
        0x8000000000000000uL, // dSIGN_MASK
        0x7fffffffffffffffuL, // dABS_MASK
        0x400921FB54442D18uL, // dPiHI
        0x3CA1A64000000000uL, // dPiLO
        0xfffffffff8000000uL, // dHIGH_26_MASK
        0x3FDC000000000000uL, // dATAN_BOUND1
        0x3FF3000000000000uL, // dATAN_BOUND3
        0x4003800000000000uL, // dATAN_BOUND4
        0x3FE6000000000000uL, // dATAN_BOUND2
        0x00000004u,          // i4
        0xfff00000u,          // idEXP_MASK
        0x7fe00000u,          // id2_BIAS
        0x007fffffu,          // iS_MANTISSA_MASK
        0x3f800000u,          // iS_ONE
        0x07f00000u,          // idBIAS
        0x3FF0000000000000uL, // dONE
        0x3F8BE4FBE6733718uL, // dATAN_A11
        0xbFA04CD71F92185EuL, // dATAN_A10
        0x3FA6AD5558FE19C9uL, // dATAN_A09
        0xbFAA9E755CA13D23uL, // dATAN_A08
        0x3FAE12F1EDF7C393uL, // dATAN_A07
        0xbFB1108D326C68EDuL, // dATAN_A06
        0x3FB3B132B731E73AuL, // dATAN_A05
        0xbFB745D119677A4FuL, // dATAN_A04
        0x3FBC71C719F99F96uL, // dATAN_A03
        0xbFC2492492441A21uL, // dATAN_A02
        0x3FC9999999998F43uL, // dATAN_A01
        0xbFD5555555555552uL, // dATAN_A00
        0x7fffffffu,          // iABSMASK
        0x80300000u,          // iCHK_WORK_SUB
        0xfdd00000u,          // iCHK_WORK_CMP
        0x7fffffffffffffffuL, // dABSMASK
        0x0000000000000000uL, // dZERO
        0x3f800000u,          // S_ONE
        0xffffffff00000000uL, // dHIGH_20_MASK
        0x400921FB54442D18uL, // dPI
        0x3FF921FB54442D18uL, // dPIO2
        0x3BF00000u,
        0x00000000u,
}; /*dAtan2_Table*/
#pragma float_control(precise, on)
static __constant _iml_v2_dp_union_t __datan2_ha_CoutTab[251] = {
    0xE8000000, 0x3FC3D6EE, /* atan(B[  0])hi = +1.549967415631e-01 */
    0x8B0D1D86, 0x3DF8CC4D, /* atan(B[  0])lo = +3.608592409707e-10 */
    0x50000000, 0x3FCB90D7, /* atan(B[  1])hi = +2.153576985002e-01 */
    0x1022F622, 0x3E149305, /* atan(B[  1])lo = +1.197581645437e-09 */
    0x38000000, 0x3FD36277, /* atan(B[  2])hi = +3.028848692775e-01 */
    0x8658E951, 0xBE0F0286, /* atan(B[  2])lo = -9.025058588437e-10 */
    0xC0000000, 0x3FDA64EE, /* atan(B[  3])hi = +4.124104380608e-01 */
    0xE5B6427D, 0x3E2E611F, /* atan(B[  3])lo = +3.536626808853e-09 */
    0xA8000000, 0x3FE1E00B, /* atan(B[  4])hi = +5.585993081331e-01 */
    0x9F9B5C83, 0x3E3EF7F5, /* atan(B[  4])lo = +7.210437130796e-09 */
    0xC8000000, 0x3FE700A7, /* atan(B[  5])hi = +7.188300043344e-01 */
    0x618C34D2, 0xBE343DCE, /* atan(B[  5])lo = -4.712825262649e-09 */
    0x58000000, 0x3FECAC7C, /* atan(B[  6])hi = +8.960553854704e-01 */
    0x6F251EC5, 0xBE0EE418, /* atan(B[  6])lo = -8.990463636494e-10 */
    0x30000000, 0x3FF0D38F, /* atan(B[  7])hi = +1.051650226116e+00 */
    0x0B7A1B84, 0xBE4D22FB, /* atan(B[  7])lo = -1.356780675246e-08 */
    0x78000000, 0x3FF30B6D, /* atan(B[  8])hi = +1.190289944410e+00 */
    0x8589532C, 0x3E36A4DA, /* atan(B[  8])lo = +5.272207636248e-09 */
    0x00000000, 0x3FF4AE11, /* atan(B[  9])hi = +1.292496681213e+00 */
    0xDA7607D1, 0xBE4CD3B2, /* atan(B[  9])lo = -1.342359363835e-08 */
    0x18000000, 0x3FF5F973, /* atan(B[ 10])hi = +1.373400777578e+00 */
    0x4931151A, 0xBE46D5BD, /* atan(B[ 10])lo = -1.063333802097e-08 */
    0x78000000, 0x3FF6DCC5, /* atan(B[ 11])hi = +1.428899258375e+00 */
    0xE5AC6F37, 0x3E4DAB2F, /* atan(B[ 11])lo = +1.381556484974e-08 */
    0x30000000, 0x3FF789BD, /* atan(B[ 12])hi = +1.471127688885e+00 */
    0x63E8AA08, 0xBE4F4FFD, /* atan(B[ 12])lo = -1.458100051557e-08 */
    0x08000000, 0x3FF7FDE8, /* atan(B[ 13])hi = +1.499488860369e+00 */
    0xFFBFDCA2, 0x3E1C30A7, /* atan(B[ 13])lo = +1.640877655104e-09 */
    0x28000000, 0x3FF8555A, /* atan(B[ 14])hi = +1.520837932825e+00 */
    0x3B43DC6C, 0xBE1E19F8, /* atan(B[ 14])lo = -1.752134643155e-09 */
    0x18000000, 0x3FF88FC2, /* atan(B[ 15])hi = +1.535097211599e+00 */
    0x6FF107D4, 0x3E259D3B, /* atan(B[ 15])lo = +2.516222674259e-09 */
    0x60000000, 0x3FF8BB9A, /* atan(B[ 16])hi = +1.545801520348e+00 */
    0x27F43144, 0x3E4B8C7A, /* atan(B[ 16])lo = +1.282838124489e-08 */
    0xC0000000, 0x3FF8D8D8, /* atan(B[ 17])hi = +1.552941083908e+00 */
    0x2931C287, 0xBE2359D2, /* atan(B[ 17])lo = -2.252736949720e-09 */
    0xD0000000, 0x3FF8EEC8, /* atan(B[ 18])hi = +1.558296978474e+00 */
    0x607FD6F0, 0xBE07FCCD, /* atan(B[ 18])lo = -6.981283892985e-10 */
    0x48000000, 0x3FF8FD69, /* atan(B[ 19])hi = +1.561867982149e+00 */
    0x8310236B, 0x3E4679B5, /* atan(B[ 19])lo = +1.046593575868e-08 */
    0xD0000000, 0x3FF90861, /* atan(B[ 20])hi = +1.564546406269e+00 */
    0x9CFF61AF, 0x3E205B36, /* atan(B[ 20])lo = +1.904124170821e-09 */
    0x38000000, 0x3FF90FB2, /* atan(B[ 21])hi = +1.566332072020e+00 */
    0x3F24AB6A, 0xBE160576, /* atan(B[ 21])lo = -1.281810432599e-09 */
    0x88000000, 0x3FF9152E, /* atan(B[ 22])hi = +1.567671328783e+00 */
    0xBC43465D, 0x3E419361, /* atan(B[ 22])lo = +8.184327778349e-09 */
    0xC0000000, 0x3FF918D6, /* atan(B[ 23])hi = +1.568564176559e+00 */
    0x1114E411, 0x3E47CE4F, /* atan(B[ 23])lo = +1.108548322808e-08 */
    0xF0000000, 0x3FF91B94, /* atan(B[ 24])hi = +1.569233834743e+00 */
    0x773951DF, 0xBE3CAD7B, /* atan(B[ 24])lo = -6.677039244062e-09 */
    0x08000000, 0x3FF91D69, /* atan(B[ 25])hi = +1.569680243731e+00 */
    0xFAA44CE6, 0x3E49FB97, /* atan(B[ 25])lo = +1.209917839922e-08 */
    0x20000000, 0x3FF91EC8, /* atan(B[ 26])hi = +1.570015072823e+00 */
    0x8B23FFA5, 0x3E31BE61, /* atan(B[ 26])lo = +4.131271479643e-09 */
    0x30000000, 0x3FF91FB2, /* atan(B[ 27])hi = +1.570238292217e+00 */
    0x87FC4A01, 0xBE128841, /* atan(B[ 27])lo = -1.078718919694e-09 */
    0xB8000000, 0x3FF92061, /* atan(B[ 28])hi = +1.570405691862e+00 */
    0x29392773, 0x3E455F88, /* atan(B[ 28])lo = +9.952658509095e-09 */
    0xC0000000, 0x3FF920D6, /* atan(B[ 29])hi = +1.570517301559e+00 */
    0x6C4176AE, 0x3E3FB87E, /* atan(B[ 29])lo = +7.385546122670e-09 */
    0x88000000, 0x3FF9212E, /* atan(B[ 30])hi = +1.570601016283e+00 */
    0x76D7A426, 0xBE210E80, /* atan(B[ 30])lo = -1.985655132280e-09 */
    0x08000000, 0x3FF92169, /* atan(B[ 31])hi = +1.570656806231e+00 */
    0xD9D720C0, 0x3E48FD55, /* atan(B[ 31])lo = +1.163668522209e-08 */
    0xF0000000, 0x3FF92194, /* atan(B[ 32])hi = +1.570698678493e+00 */
    0xC02EEAC5, 0xBE41119E, /* atan(B[ 32])lo = -7.948292695772e-09 */
    0x30000000, 0x3FF921B2, /* atan(B[ 33])hi = +1.570726573467e+00 */
    0xB22521F8, 0xBE138683, /* atan(B[ 33])lo = -1.136530599303e-09 */
    0x20000000, 0x3FF921C8, /* atan(B[ 34])hi = +1.570747494698e+00 */
    0x3D6A380D, 0x3E310FA9, /* atan(B[ 34])lo = +3.972364623557e-09 */
    0xC0000000, 0x3FF921D6, /* atan(B[ 35])hi = +1.570761442184e+00 */
    0x5ADD7895, 0x3E3FB08C, /* atan(B[ 35])lo = +7.378319661704e-09 */
    0xB8000000, 0x3FF921E1, /* atan(B[ 36])hi = +1.570771902800e+00 */
    0xA42B3618, 0x3E45549C, /* atan(B[ 36])lo = +9.932795146628e-09 */
    0x08000000, 0x3FF921E9, /* atan(B[ 37])hi = +1.570778876543e+00 */
    0xB8C09601, 0x3E48FCD6, /* atan(B[ 37])lo = +1.163578191444e-08 */
    0x88000000, 0x3FF921EE, /* atan(B[ 38])hi = +1.570784121752e+00 */
    0x3964E770, 0xBE2113F6, /* atan(B[ 38])lo = -1.988138052759e-09 */
    0x30000000, 0x3FF921F2, /* atan(B[ 39])hi = +1.570787608624e+00 */
    0xD33BBF6E, 0xBE138702, /* atan(B[ 39])lo = -1.136643512761e-09 */
    0xF0000000, 0x3FF921F4, /* atan(B[ 40])hi = +1.570790231228e+00 */
    0x6E436174, 0xBE4111CA, /* atan(B[ 40])lo = -7.948603060837e-09 */
    0xC0000000, 0x3FF921F6, /* atan(B[ 41])hi = +1.570791959763e+00 */
    0x61D4C384, 0x3E3FB088, /* atan(B[ 41])lo = +7.378305547522e-09 */
    0x20000000, 0x3FF921F8, /* atan(B[ 42])hi = +1.570793271065e+00 */
    0x51E51998, 0x3E310F9E, /* atan(B[ 42])lo = +3.972325827924e-09 */
    0x08000000, 0x3FF921F9, /* atan(B[ 43])hi = +1.570794135332e+00 */
    0x79300AAF, 0x3E48FCD6, /* atan(B[ 43])lo = +1.163578015017e-08 */
    0xB8000000, 0x3FF921F9, /* atan(B[ 44])hi = +1.570794790983e+00 */
    0xF572E42D, 0x3E45549B, /* atan(B[ 44])lo = +9.932790297173e-09 */
    0x30000000, 0x3FF921FA, /* atan(B[ 45])hi = +1.570795238018e+00 */
    0x12CC4AC0, 0xBE138703, /* atan(B[ 45])lo = -1.136643733295e-09 */
    0x88000000, 0x3FF921FA, /* atan(B[ 46])hi = +1.570795565844e+00 */
    0x90C11066, 0xBE2113F6, /* atan(B[ 46])lo = -1.988138658941e-09 */
    0xC0000000, 0x3FF921FA, /* atan(B[ 47])hi = +1.570795774460e+00 */
    0x5FD83F2A, 0x3E3FB088, /* atan(B[ 47])lo = +7.378305519955e-09 */
    0xF0000000, 0x3FF921FA, /* atan(B[ 48])hi = +1.570795953274e+00 */
    0x70FE42BC, 0xBE4111CA, /* atan(B[ 48])lo = -7.948603136610e-09 */
    0x08000000, 0x3FF921FB, /* atan(B[ 49])hi = +1.570796042681e+00 */
    0x79104269, 0x3E48FCD6, /* atan(B[ 49])lo = +1.163578014672e-08 */
    0x20000000, 0x3FF921FB, /* atan(B[ 50])hi = +1.570796132088e+00 */
    0x51366146, 0x3E310F9E, /* atan(B[ 50])lo = +3.972325818452e-09 */
    0x30000000, 0x3FF921FB, /* atan(B[ 51])hi = +1.570796191692e+00 */
    0x12EC1305, 0xBE138703, /* atan(B[ 51])lo = -1.136643733725e-09 */
    0x38000000, 0x3FF921FB, /* atan(B[ 52])hi = +1.570796221495e+00 */
    0xF567F8A8, 0x3E45549B, /* atan(B[ 52])lo = +9.932790295989e-09 */
    0x40000000, 0x3FF921FB, /* atan(B[ 53])hi = +1.570796251297e+00 */
    0x5FD740E8, 0x3E3FB088, /* atan(B[ 53])lo = +7.378305519901e-09 */
    0x48000000, 0x3FF921FB, /* atan(B[ 54])hi = +1.570796281099e+00 */
    0x90C68629, 0xBE2113F6, /* atan(B[ 54])lo = -1.988138659089e-09 */
    0x48000000, 0x3FF921FB, /* atan(B[ 55])hi = +1.570796281099e+00 */
    0x79103285, 0x3E48FCD6, /* atan(B[ 55])lo = +1.163578014671e-08 */
    0x50000000, 0x3FF921FB, /* atan(B[ 56])hi = +1.570796310902e+00 */
    0x70FE6E6A, 0xBE4111CA, /* atan(B[ 56])lo = -7.948603136628e-09 */
    0x50000000, 0x3FF921FB, /* atan(B[ 57])hi = +1.570796310902e+00 */
    0x12EC22EA, 0xBE138703, /* atan(B[ 57])lo = -1.136643733726e-09 */
    0x50000000, 0x3FF921FB, /* atan(B[ 58])hi = +1.570796310902e+00 */
    0x5136565A, 0x3E310F9E, /* atan(B[ 58])lo = +3.972325818450e-09 */
    0x50000000, 0x3FF921FB, /* atan(B[ 59])hi = +1.570796310902e+00 */
    0x5FD74068, 0x3E3FB088, /* atan(B[ 59])lo = +7.378305519901e-09 */
    0x50000000, 0x3FF921FB, /* atan(B[ 60])hi = +1.570796310902e+00 */
    0xF567F7FA, 0x3E45549B, /* atan(B[ 60])lo = +9.932790295989e-09 */
    0x50000000, 0x3FF921FB, /* atan(B[ 61])hi = +1.570796310902e+00 */
    0x7910327D, 0x3E48FCD6, /* atan(B[ 61])lo = +1.163578014671e-08 */
    0x50000000, 0x3FF921FB, /* atan(B[ 62])hi = +1.570796310902e+00 */
    0x5BCE5E60, 0x3E4BBB02, /* atan(B[ 62])lo = +1.291302253476e-08 */
    0x50000000, 0x3FF921FB, /* atan(B[ 63])hi = +1.570796310902e+00 */
    0x9DA27BA2, 0x3E4D8F1F, /* atan(B[ 63])lo = +1.376451746012e-08 */
    0x50000000, 0x3FF921FB, /* atan(B[ 64])hi = +1.570796310902e+00 */
    0x8F019193, 0x3E4EEE35, /* atan(B[ 64])lo = +1.440313865414e-08 */
    0x50000000, 0x3FF921FB, /* atan(B[ 65])hi = +1.570796310902e+00 */
    0x2FEBA034, 0x3E4FD844, /* atan(B[ 65])lo = +1.482888611682e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 66])hi = +1.570796340704e+00 */
    0xD764D4D3, 0xBE4F7830, /* atan(B[ 66])lo = -1.465412567386e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 67])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4F0329, /* atan(B[ 67])lo = -1.444125194252e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 68])hi = +1.570796340704e+00 */
    0x0A980806, 0xBE4EAB64, /* atan(B[ 68])lo = -1.428159664401e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 69])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4E70E0, /* atan(B[ 69])lo = -1.417515977834e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 70])hi = +1.570796340704e+00 */
    0xA431A1A0, 0xBE4E44FD, /* atan(B[ 70])lo = -1.409533212909e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 71])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4E27BB, /* atan(B[ 71])lo = -1.404211369626e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 72])hi = +1.570796340704e+00 */
    0x70FE6E6D, 0xBE4E11CA, /* atan(B[ 72])lo = -1.400219987163e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 73])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4E0329, /* atan(B[ 73])lo = -1.397559065521e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 74])hi = +1.570796340704e+00 */
    0xD764D4D3, 0xBE4DF830, /* atan(B[ 74])lo = -1.395563374290e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 75])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DF0E0, /* atan(B[ 75])lo = -1.394232913469e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 76])hi = +1.570796340704e+00 */
    0x0A980806, 0xBE4DEB64, /* atan(B[ 76])lo = -1.393235067853e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 77])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4DE7BB, /* atan(B[ 77])lo = -1.392569837443e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 78])hi = +1.570796340704e+00 */
    0xA431A1A0, 0xBE4DE4FD, /* atan(B[ 78])lo = -1.392070914635e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 79])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4DE329, /* atan(B[ 79])lo = -1.391738299430e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 80])hi = +1.570796340704e+00 */
    0x70FE6E6D, 0xBE4DE1CA, /* atan(B[ 80])lo = -1.391488838026e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 81])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DE0E0, /* atan(B[ 81])lo = -1.391322530423e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 82])hi = +1.570796340704e+00 */
    0xD764D4D3, 0xBE4DE030, /* atan(B[ 82])lo = -1.391197799721e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 83])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4DDFBB, /* atan(B[ 83])lo = -1.391114645920e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 84])hi = +1.570796340704e+00 */
    0x0A980806, 0xBE4DDF64, /* atan(B[ 84])lo = -1.391052280569e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 85])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4DDF29, /* atan(B[ 85])lo = -1.391010703668e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 86])hi = +1.570796340704e+00 */
    0xA431A1A0, 0xBE4DDEFD, /* atan(B[ 86])lo = -1.390979520993e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 87])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DDEE0, /* atan(B[ 87])lo = -1.390958732543e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 88])hi = +1.570796340704e+00 */
    0x70FE6E6D, 0xBE4DDECA, /* atan(B[ 88])lo = -1.390943141205e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 89])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4DDEBB, /* atan(B[ 89])lo = -1.390932746980e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 90])hi = +1.570796340704e+00 */
    0xD764D4D3, 0xBE4DDEB0, /* atan(B[ 90])lo = -1.390924951311e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 91])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4DDEA9, /* atan(B[ 91])lo = -1.390919754198e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 92])hi = +1.570796340704e+00 */
    0x0A980806, 0xBE4DDEA4, /* atan(B[ 92])lo = -1.390915856364e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 93])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DDEA0, /* atan(B[ 93])lo = -1.390913257807e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 94])hi = +1.570796340704e+00 */
    0xA431A1A0, 0xBE4DDE9D, /* atan(B[ 94])lo = -1.390911308890e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 95])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4DDE9B, /* atan(B[ 95])lo = -1.390910009612e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 96])hi = +1.570796340704e+00 */
    0x70FE6E6D, 0xBE4DDE9A, /* atan(B[ 96])lo = -1.390909035153e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 97])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4DDE99, /* atan(B[ 97])lo = -1.390908385514e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 98])hi = +1.570796340704e+00 */
    0xD764D4D3, 0xBE4DDE98, /* atan(B[ 98])lo = -1.390907898285e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[ 99])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DDE98, /* atan(B[ 99])lo = -1.390907573466e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[100])hi = +1.570796340704e+00 */
    0x0A980806, 0xBE4DDE98, /* atan(B[100])lo = -1.390907329851e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[101])hi = +1.570796340704e+00 */
    0xD0145FCC, 0xBE4DDE97, /* atan(B[101])lo = -1.390907167441e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[102])hi = +1.570796340704e+00 */
    0xA431A1A0, 0xBE4DDE97, /* atan(B[102])lo = -1.390907045634e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[103])hi = +1.570796340704e+00 */
    0x86EFCD83, 0xBE4DDE97, /* atan(B[103])lo = -1.390906964429e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[104])hi = +1.570796340704e+00 */
    0x70FE6E6D, 0xBE4DDE97, /* atan(B[104])lo = -1.390906903525e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[105])hi = +1.570796340704e+00 */
    0x625D845E, 0xBE4DDE97, /* atan(B[105])lo = -1.390906862923e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[106])hi = +1.570796340704e+00 */
    0x5764D4D3, 0xBE4DDE97, /* atan(B[106])lo = -1.390906832471e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[107])hi = +1.570796340704e+00 */
    0x50145FCC, 0xBE4DDE97, /* atan(B[107])lo = -1.390906812170e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[108])hi = +1.570796340704e+00 */
    0x4A980806, 0xBE4DDE97, /* atan(B[108])lo = -1.390906796944e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[109])hi = +1.570796340704e+00 */
    0x46EFCD83, 0xBE4DDE97, /* atan(B[109])lo = -1.390906786793e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[110])hi = +1.570796340704e+00 */
    0x4431A1A0, 0xBE4DDE97, /* atan(B[110])lo = -1.390906779180e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[111])hi = +1.570796340704e+00 */
    0x425D845E, 0xBE4DDE97, /* atan(B[111])lo = -1.390906774105e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[112])hi = +1.570796340704e+00 */
    0x40FE6E6D, 0xBE4DDE97, /* atan(B[112])lo = -1.390906770298e-08 */
    0x58000000, 0x3FF921FB, /* atan(B[113])hi = +1.570796340704e+00 */
    0x40145FCC, 0xBE4DDE97, /* atan(B[113])lo = -1.390906767761e-08 */
    /* Coefficients of AtanPoly */
    0x55555555, 0xBFD55555, /* A00 = -3.3333333333333330e-01 */
    0x999991BB, 0x3FC99999, /* A01 = +1.9999999999994408e-01 */
    0x92382A27, 0xBFC24924, /* A02 = -1.4285714282625968e-01 */
    0xF9B157D3, 0x3FBC71C6, /* A03 = +1.1111110301985087e-01 */
    0x69CCA475, 0xBFB745BE, /* A04 = -9.0907955961158590e-02 */
    0x1048AFD1, 0x3FB3AB7C, /* A05 = +7.6835397697038190e-02 */
    0x34239994, 0xBFB029BD, /* A06 = -6.3136887768996455e-02 */
    /* Other constants */
    0x00000000, 0x3FC00000, /* BOUND1 = 2^(-3) */
    0x00000000, 0x3FF00000, /* ONE = 1.0 */
    0x00000000, 0x40000000, /* TWO = 2.0 */
    0x54442D18, 0x3FE921FB, /* PIO4HI  = high part of Pi/4 */
    0x33145C07, 0x3C81A626, /* PIO4LO  = low  part of Pi/4 */
    0x54442D18, 0x3FF921FB, /* PIO2HI  = high part of Pi/2 */
    0x33145C07, 0x3C91A626, /* PIO2LO  = low  part of Pi/2 */
    0x54442D18, 0x400921FB, /* PIHI    = high part of Pi   */
    0x33145C07, 0x3CA1A626, /* PILO    = low  part of Pi   */
    0x7F3321D2, 0x4002D97C, /* PI3O4HI = high part of Pi*3/4 */
    0x4C9E8A0A, 0x3C9A7939, /* PI3O4LO = low  part of Pi*3/4 */
    0x00000000, 0xBFF00000, /* MONE = -1.0 */
    0x00000000, 0x00000000, /* ZERO = 0.0 */
    0x00000000, 0x7FD00000, /* TWO_POW_BIASM1 = 2^(BIASM1) */
    0x00000000, 0x00100000, /* TWO_POW_MBIASM1 = 2^(-BIASM1) */
    /* Constant used to obtain high bits */
    0x02000000, 0x41A00000, /* T27 = 2^27+1 */
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_datan2_ha(double *a, double *b, double *r) {
  int nRet = 0;
  double dbY, dbX;
  double dbAY, dbAX, dbZPHi, dbZPLo, dbYOX, dbAY1, dbTwoPowN, dbAX1, dbB, dbXHi,
      dbXLo, dbBXHi, dbBXLo, dbTmp1, dbUHi, dbuLo, dbYHi, dbYLo, dbBYHi, dbBYLo,
      dbVHi, dbVLo, dbTmp2, dbR0, dbE, dbQHi, dbQLo, dbTHi, dbTLo, dbT2,
      dbAtanPoly, dbAtanPolyHi, dbAtanPolyLo, dbRHi, dbRLo, dbRes;
  double dbVTmp1, dbVTmp2, dbVTmp3;
  int i, iSignY, iSignX, iSign, iJ, iEY, iEX, iEY1;
  i = 0;
  /* Flash denormal arg to zero in case of DAZFTZ mode */
  dbY = (((__constant double *)__datan2_ha_CoutTab)[236]);
  dbY = (dbY * (a[0]));
  dbX = (((__constant double *)__datan2_ha_CoutTab)[236]);
  dbX = (dbX * (b[0]));
  /* Get signs of arguments */
  iSignY = (((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword >> 31);
  iSignX = (((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword >> 31);
  /* Get biased exponents of arguments */
  iEY = ((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword >> 20) & 0x7FF);
  iEX = ((((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword >> 20) & 0x7FF);
  /* Filter out INFs and NaNs */
  if ((((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF) &&
      (((((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)) {
    /* Filter out zeros */
    if (((iEY != 0) ||
         !(((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword & 0x000FFFFF) == 0) &&
           ((((_iml_v2_dp_union_t *)&dbY)->dwords.lo_dword) == 0))) &&
        ((iEX != 0) ||
         !(((((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword & 0x000FFFFF) == 0) &&
           ((((_iml_v2_dp_union_t *)&dbX)->dwords.lo_dword) == 0)))) {
      /* Here if arguments are finite nonzero double precision */
      /* numbers                                               */
      /* Get absolute values of arguments */
      dbAY = dbY;
      (((_iml_v2_dp_union_t *)&dbAY)->dwords.hi_dword =
           (((_iml_v2_dp_union_t *)&dbAY)->dwords.hi_dword & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      dbAX = dbX;
      (((_iml_v2_dp_union_t *)&dbAX)->dwords.hi_dword =
           (((_iml_v2_dp_union_t *)&dbAX)->dwords.hi_dword & 0x7FFFFFFF) |
           ((_iml_uint32_t)(0) << 31));
      if (iEY - iEX > -54) {
        /* Here if -105 < ex(y) - ex(x) */
        if (iEY - iEX < 54) {
          /* Path 10). Here if -105 < ex(y) - ex(x) < 105 */
          /* Obtain ZP */
          if (iSignX == 0) {
            /* If x>0 then ZP=0 */
            dbZPHi = ((__constant double *)__datan2_ha_CoutTab)[247];
            dbZPLo = ((__constant double *)__datan2_ha_CoutTab)[247];
          } else {
            /* If x<0 then ZP=Pi */
            dbZPHi = ((__constant double *)__datan2_ha_CoutTab)[242];
            dbZPLo = ((__constant double *)__datan2_ha_CoutTab)[243];
          }
          dbYOX = dbAY / dbAX;
          /* Represent AY in the form AY = 2^EY1 * AY1, */
          /* where -BIASM1 <= iEY1 <= BIASM1.           */
          if (iEY > 0) {
            /* Here if AY is normalized */
            if (iEY < 0x7FF - 1) {
              /* Here if binary_exponent(AY)<=BIASM1 */
              dbAY1 = dbAY;
              (((_iml_v2_dp_union_t *)&dbAY1)->dwords.hi_dword =
                   (((_iml_v2_dp_union_t *)&dbAY1)->dwords.hi_dword & 0x800FFFFF) |
                   (((_iml_uint32_t)(0x3FF) & 0x7FF) << 20));
              iEY1 = iEY - 0x3FF;
            } else {
              /* Here if binary_exponent(AY)=IML_DP_BIAS */
              dbAY1 = dbAY * ((__constant double *)__datan2_ha_CoutTab)[249];
              iEY1 = (0x3FF - 1);
            }
          } else {
            /* Here if AY is denormalized */
            dbAY1 = dbAY * ((__constant double *)__datan2_ha_CoutTab)[248];
            iEY1 = -(0x3FF - 1);
          }
          /* AX1 := AX * 2^(-EY1) */
          dbTwoPowN = ((__constant double *)__datan2_ha_CoutTab)[236];
          (((_iml_v2_dp_union_t *)&dbTwoPowN)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbTwoPowN)->dwords.hi_dword & 0x800FFFFF) |
               (((_iml_uint32_t)(0x3FF - iEY1) & 0x7FF) << 20));
          dbAX1 = dbAX * dbTwoPowN;
          if (dbYOX >= ((__constant double *)__datan2_ha_CoutTab)[235]) {
            /* Path 10.2). Here if BOUND1 <= YOX   */
            /*             and ex(y) - ex(x) < 105 */
            /* 10.2.a) atan() argument reduction */
            /* 10.2.a.1) Getting index j */
            iJ = ((((_iml_v2_dp_union_t *)&dbYOX)->dwords.hi_dword >> 20) & 0x7FF);
            iJ = iJ << 20;
            iJ = iJ |
                 (((_iml_v2_dp_union_t *)&dbYOX)->dwords.hi_dword & 0x000FFFFF);
            iJ = iJ - 0x3FC00000;
            iJ = iJ >> 19;
            if (iJ > 113)
              iJ = 113;
            /* 10.2.a.2) Getting base point b */
            dbB = dbYOX;
            (((_iml_v2_dp_union_t *)&dbB)->dwords.lo_dword = (0));
            (((_iml_v2_dp_union_t *)&dbB)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbB)->dwords.hi_dword & 0xFFF00000) |
                 ((((((_iml_v2_dp_union_t *)&dbB)->dwords.hi_dword & 0x000FFFFF) &
                    0x80000) |
                   0x40000) &
                  0x000FFFFF));
            /* 10.2.a.3) Getting t in multiprecision */
            /* bx := b * AX1 */
            dbVTmp1 =
                ((dbAX1) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAX1));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAX1)-dbVTmp1);
            dbXHi = dbVTmp1;
            dbXLo = dbVTmp2;
            ;
            dbBXHi = dbXHi * dbB;
            dbBXLo = dbXLo * dbB;
            /* u := AY1 - bx */
            dbBXHi = dbBXHi * ((__constant double *)__datan2_ha_CoutTab)[246];
            dbBXLo = dbBXLo * ((__constant double *)__datan2_ha_CoutTab)[246];
            dbVTmp1 = ((dbBXHi) + (dbBXLo));
            dbTmp1 = ((dbBXHi)-dbVTmp1);
            dbVTmp2 = (dbTmp1 + (dbBXLo));
            dbBXHi = dbVTmp1;
            dbBXLo = dbVTmp2;
            ;
            dbVTmp1 = ((dbAY1) + (dbBXHi));
            dbVTmp2 = ((dbAY1)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbBXHi) + dbVTmp2);
            dbVTmp3 = ((dbAY1)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbUHi = dbVTmp1;
            dbuLo = dbVTmp3;
            ;
            dbuLo = dbuLo + dbBXLo;
            /* Rebreak u before multiplication */
            dbVTmp1 =
                ((dbUHi) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbUHi));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbUHi)-dbVTmp1);
            dbUHi = dbVTmp1;
            dbTmp1 = dbVTmp2;
            ;
            dbuLo = dbuLo + dbTmp1;
            /* by := b * AY1 */
            dbVTmp1 =
                ((dbAY1) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAY1));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAY1)-dbVTmp1);
            dbYHi = dbVTmp1;
            dbYLo = dbVTmp2;
            ;
            dbBYHi = dbYHi * dbB;
            dbBYLo = dbYLo * dbB;
            /* v := AX1 + by */
            dbVTmp1 = ((dbBYHi) + (dbBYLo));
            dbTmp1 = ((dbBYHi)-dbVTmp1);
            dbVTmp2 = (dbTmp1 + (dbBYLo));
            dbBYHi = dbVTmp1;
            dbBYLo = dbVTmp2;
            ;
            dbVTmp1 = ((dbAX1) + (dbBYHi));
            dbVTmp2 = ((dbAX1)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbBYHi) + dbVTmp2);
            dbVTmp3 = ((dbAX1)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbVHi = dbVTmp1;
            dbVLo = dbVTmp3;
            ;
            dbVLo = dbVLo + dbBYLo;
            /* Rebreak v before inversion */
            dbVTmp1 =
                ((dbVHi) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbVHi));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbVHi)-dbVTmp1);
            dbVHi = dbVTmp1;
            dbTmp1 = dbVTmp2;
            ;
            dbVLo = dbVLo + dbTmp1;
            /* Calculating q := 1 / v */
            dbTmp1 = (((__constant double *)__datan2_ha_CoutTab)[236] / dbVHi);
            dbVTmp2 =
                (dbTmp1 * ((__constant double *)__datan2_ha_CoutTab)[250]);
            dbVTmp3 = (dbVTmp2 - dbTmp1);
            dbVTmp3 = (dbVTmp2 - dbVTmp3);
            dbTmp1 = (dbVHi * dbVTmp3);
            dbTmp1 = (((__constant double *)__datan2_ha_CoutTab)[236] - dbTmp1);
            dbVTmp2 = (dbVLo * dbVTmp3);
            dbVTmp2 = (dbTmp1 - dbVTmp2);
            dbTmp1 =
                (((__constant double *)__datan2_ha_CoutTab)[236] + dbVTmp2);
            dbQHi = dbVTmp3;
            dbTmp1 = (dbTmp1 * dbVTmp2);
            dbQLo = (dbTmp1 * dbVTmp3);
            ;
            /* Calculating t := u * q */
            dbTmp1 = ((dbUHi) * (dbQHi));
            dbTmp2 = ((dbuLo) * (dbQLo));
            dbTmp2 = (dbTmp2 + (dbUHi) * (dbQLo));
            dbVTmp1 = (dbTmp2 + (dbuLo) * (dbQHi));
            dbTHi = dbTmp1;
            dbTLo = dbVTmp1;
            ;
            dbVTmp1 = ((dbTHi) + (dbTLo));
            dbTmp1 = ((dbTHi)-dbVTmp1);
            dbVTmp2 = (dbTmp1 + (dbTLo));
            dbTHi = dbVTmp1;
            dbTLo = dbVTmp2;
            ;
            /* 10.2.b) atan() approximation */
            dbT2 = dbTHi * dbTHi;
            dbAtanPoly =
                ((((((((__constant double *)__datan2_ha_CoutTab)[234]) * dbT2 +
                     ((__constant double *)__datan2_ha_CoutTab)[233]) *
                        dbT2 +
                    ((__constant double *)__datan2_ha_CoutTab)[232]) *
                       dbT2 +
                   ((__constant double *)__datan2_ha_CoutTab)[231]) *
                      dbT2 +
                  ((__constant double *)__datan2_ha_CoutTab)[230]) *
                     dbT2 +
                 ((__constant double *)__datan2_ha_CoutTab)[229]) *
                    dbT2 +
                ((__constant double *)__datan2_ha_CoutTab)[228];
            /* 10.2.c) atan() reconstruction */
            /* AtanPoly := AtanPoly * t2 */
            dbAtanPoly = dbAtanPoly * dbT2;
            /* R := AtanPoly * t */
            dbVTmp1 =
                ((dbTHi) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbTHi));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbTHi)-dbVTmp1);
            dbTHi = dbVTmp1;
            dbTmp1 = dbVTmp2;
            ;
            dbTLo = dbTLo + dbTmp1;
            dbVTmp1 = ((dbAtanPoly) *
                       (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAtanPoly));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAtanPoly)-dbVTmp1);
            dbAtanPolyHi = dbVTmp1;
            dbAtanPolyLo = dbVTmp2;
            ;
            dbTmp1 = ((dbAtanPolyHi) * (dbTHi));
            dbTmp2 = ((dbAtanPolyLo) * (dbTLo));
            dbTmp2 = (dbTmp2 + (dbAtanPolyHi) * (dbTLo));
            dbVTmp1 = (dbTmp2 + (dbAtanPolyLo) * (dbTHi));
            dbRHi = dbTmp1;
            dbRLo = dbVTmp1;
            ;
            /* Q := R + t */
            dbVTmp1 = ((dbRHi) + (dbTHi));
            dbVTmp2 = ((dbRHi)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbTHi) + dbVTmp2);
            dbVTmp3 = ((dbRHi)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbQHi = dbVTmp1;
            dbQLo = dbVTmp3;
            ;
            dbQLo = dbQLo + dbTLo;
            dbQLo = dbQLo + dbRLo;
            /* Q := Q + atan(b) */
            dbVTmp1 = ((dbQHi) + (((__constant double *)
                                       __datan2_ha_CoutTab)[0 + 2 * (iJ) + 0]));
            dbVTmp2 = ((dbQHi)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((((__constant double *)
                             __datan2_ha_CoutTab)[0 + 2 * (iJ) + 0]) +
                       dbVTmp2);
            dbVTmp3 = ((dbQHi)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbQHi = dbVTmp1;
            dbTmp1 = dbVTmp3;
            ;
            dbQLo = dbQLo + dbTmp1;
            dbQLo =
                dbQLo +
                ((__constant double *)__datan2_ha_CoutTab)[0 + 2 * (iJ) + 1];
            /* 10.2.d) atan2() reconstruction */
            /* Q := sign(x) * Q */
            (((_iml_v2_dp_union_t *)&dbQHi)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbQHi)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignX) << 31));
            iSign =
                iSignX ^ (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword >> 31);
            (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSign) << 31));
            /* Res := Q + ZP */
            dbVTmp1 = ((dbQHi) + (dbZPHi));
            dbVTmp2 = ((dbQHi)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbZPHi) + dbVTmp2);
            dbVTmp3 = ((dbQHi)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbQHi = dbVTmp1;
            dbTmp1 = dbVTmp3;
            ;
            dbQLo = dbQLo + dbTmp1;
            dbQLo = dbQLo + dbZPLo;
            dbRes = dbQHi + dbQLo;
            /* Res := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          } else {
            /* Path 10.1). Here if -105 < ex(y) - ex(x) and  */
            /*             YOX < BOUND1                      */
            /* Calculating t := |y/x| */
            /* u := |y| */
            dbVTmp1 =
                ((dbAY1) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAY1));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAY1)-dbVTmp1);
            dbUHi = dbVTmp1;
            dbuLo = dbVTmp2;
            ;
            /* v := |x| */
            dbVTmp1 =
                ((dbAX1) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAX1));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAX1)-dbVTmp1);
            dbVHi = dbVTmp1;
            dbVLo = dbVTmp2;
            ;
            /* q := 1/v */
            dbTmp1 = (((__constant double *)__datan2_ha_CoutTab)[236] / dbVHi);
            dbVTmp2 =
                (dbTmp1 * ((__constant double *)__datan2_ha_CoutTab)[250]);
            dbVTmp3 = (dbVTmp2 - dbTmp1);
            dbVTmp3 = (dbVTmp2 - dbVTmp3);
            dbTmp1 = (dbVHi * dbVTmp3);
            dbTmp1 = (((__constant double *)__datan2_ha_CoutTab)[236] - dbTmp1);
            dbVTmp2 = (dbVLo * dbVTmp3);
            dbVTmp2 = (dbTmp1 - dbVTmp2);
            dbTmp1 =
                (((__constant double *)__datan2_ha_CoutTab)[236] + dbVTmp2);
            dbQHi = dbVTmp3;
            dbTmp1 = (dbTmp1 * dbVTmp2);
            dbQLo = (dbTmp1 * dbVTmp3);
            ;
            /* t := u * q */
            dbTmp1 = ((dbUHi) * (dbQHi));
            dbTmp2 = ((dbuLo) * (dbQLo));
            dbTmp2 = (dbTmp2 + (dbUHi) * (dbQLo));
            dbVTmp1 = (dbTmp2 + (dbuLo) * (dbQHi));
            dbTHi = dbTmp1;
            dbTLo = dbVTmp1;
            ;
            dbVTmp1 = ((dbTHi) + (dbTLo));
            dbTmp1 = ((dbTHi)-dbVTmp1);
            dbVTmp2 = (dbTmp1 + (dbTLo));
            dbTHi = dbVTmp1;
            dbTLo = dbVTmp2;
            ;
            /* atan() approximation */
            dbT2 = dbYOX * dbYOX;
            dbAtanPoly =
                ((((((((__constant double *)__datan2_ha_CoutTab)[234]) * dbT2 +
                     ((__constant double *)__datan2_ha_CoutTab)[233]) *
                        dbT2 +
                    ((__constant double *)__datan2_ha_CoutTab)[232]) *
                       dbT2 +
                   ((__constant double *)__datan2_ha_CoutTab)[231]) *
                      dbT2 +
                  ((__constant double *)__datan2_ha_CoutTab)[230]) *
                     dbT2 +
                 ((__constant double *)__datan2_ha_CoutTab)[229]) *
                    dbT2 +
                ((__constant double *)__datan2_ha_CoutTab)[228];
            /* AtanPoly := AtanPoly * t^2 */
            dbAtanPoly = dbAtanPoly * dbT2;
            /* atan() reconstruction: */
            /* Q := t + t * (t2 * AtanPoly) */
            /* R := AtanPoly * t */
            dbVTmp1 =
                ((dbTHi) * (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbTHi));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbTHi)-dbVTmp1);
            dbTHi = dbVTmp1;
            dbTmp1 = dbVTmp2;
            ;
            dbTLo = dbTLo + dbTmp1;
            dbVTmp1 = ((dbAtanPoly) *
                       (((__constant double *)__datan2_ha_CoutTab)[250]));
            dbVTmp2 = (dbVTmp1 - (dbAtanPoly));
            dbVTmp1 = (dbVTmp1 - dbVTmp2);
            dbVTmp2 = ((dbAtanPoly)-dbVTmp1);
            dbAtanPolyHi = dbVTmp1;
            dbAtanPolyLo = dbVTmp2;
            ;
            dbTmp1 = ((dbAtanPolyHi) * (dbTHi));
            dbTmp2 = ((dbAtanPolyLo) * (dbTLo));
            dbTmp2 = (dbTmp2 + (dbAtanPolyHi) * (dbTLo));
            dbVTmp1 = (dbTmp2 + (dbAtanPolyLo) * (dbTHi));
            dbRHi = dbTmp1;
            dbRLo = dbVTmp1;
            ;
            /* Q := R + t */
            dbVTmp1 = ((dbRHi) + (dbTHi));
            dbVTmp2 = ((dbRHi)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbTHi) + dbVTmp2);
            dbVTmp3 = ((dbRHi)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbQHi = dbVTmp1;
            dbQLo = dbVTmp3;
            ;
            dbQLo = dbQLo + dbTLo;
            dbQLo = dbQLo + dbRLo;
            /* atan2() reconstruction: */
            /* (*r) := sign(y) * ( ZP + sign(x) * Q ) */
            /* Q := sign(x) * Q */
            (((_iml_v2_dp_union_t *)&dbQHi)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbQHi)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignX) << 31));
            iSign =
                iSignX ^ (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword >> 31);
            (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbQLo)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSign) << 31));
            /* Res := ZP + Q */
            dbVTmp1 = ((dbQHi) + (dbZPHi));
            dbVTmp2 = ((dbQHi)-dbVTmp1);
            dbVTmp3 = (dbVTmp1 + dbVTmp2);
            dbVTmp2 = ((dbZPHi) + dbVTmp2);
            dbVTmp3 = ((dbQHi)-dbVTmp3);
            dbVTmp3 = (dbVTmp2 + dbVTmp3);
            dbQHi = dbVTmp1;
            dbTmp1 = dbVTmp3;
            ;
            dbQLo = dbQLo + dbTmp1;
            dbQLo = dbQLo + dbZPLo;
            dbRes = dbQHi + dbQLo;
            /* Res := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          }
        } else if (iEY - iEX < 74) {
          /* Path 11). Here if 54 <= ex(y) - ex(x) < 125 */
          /* Res := Pi/2 - x/|y| */
          dbTmp1 = dbX / dbAY;
          dbRes = ((__constant double *)__datan2_ha_CoutTab)[241] - dbTmp1;
          dbRes = dbRes + ((__constant double *)__datan2_ha_CoutTab)[240];
          /* (*r) := sign(y) * Res */
          (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(iSignY) << 31));
          (*r) = dbRes;
        } else {
          /* Path 12). Here if 74 <= ex(y) - ex(x) */
          /* Res := Pi/2 */
          dbRes = ((__constant double *)__datan2_ha_CoutTab)[240] +
                  ((__constant double *)__datan2_ha_CoutTab)[241];
          /* (*r) := sign(y) * Res */
          (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(iSignY) << 31));
          (*r) = dbRes;
        }
      } else {
        /* Path 13). Here if ex(y) - ex(x) <= -54 */
        if (iSignX == 0) {
          /* Path 13.1). Here if ex(y) - ex(x) <= -54 and x>0 */
          dbRes = dbAY / dbAX;
          if (((((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword >> 20) & 0x7FF) !=
              0) {
            /* Normalized result */
            /* Explicitly raise Inexact flag */
            dbVTmp1 = ((__constant double *)__datan2_ha_CoutTab)[236] + dbRes;
            dbRes = dbRes * dbVTmp1;
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          } else {
            /* Denormalized result */
            /* Explicitly raise Inexact and Underflow flags */
            dbVTmp1 = dbRes * dbRes;
            dbRes = dbRes + dbVTmp1;
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          }
        } else {
          /* Path 13.2). Here if ex(y) - ex(x) <= -54 and x<0 */
          /* Res := Pi */
          dbRes = ((__constant double *)__datan2_ha_CoutTab)[242] +
                  ((__constant double *)__datan2_ha_CoutTab)[243];
          /* (*r) := sign(y) * Res */
          (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(iSignY) << 31));
          (*r) = dbRes;
        }
      }
    } else {
      /* Here if one of arguments is zero */
      if ((iEY != 0) ||
          !(((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword & 0x000FFFFF) == 0) &&
            ((((_iml_v2_dp_union_t *)&dbY)->dwords.lo_dword) == 0))) {
        /* Path 7). Here if x=0, y is finite nonzero */
        /* Res := Pi/2 */
        dbRes = ((__constant double *)__datan2_ha_CoutTab)[240] +
                ((__constant double *)__datan2_ha_CoutTab)[241];
        /* (*r) := sign(y) * Res */
        (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
             ((_iml_uint32_t)(iSignY) << 31));
        (*r) = dbRes;
      } else {
        /* Here if y=0, x is finite */
        if (iSignX == 0) {
          /* Path 8). Here if y=0, x>=+0 */
          /* Res := 0 */
          dbRes = ((__constant double *)__datan2_ha_CoutTab)[247];
          /* (*r) := sign(y) * Res */
          (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(iSignY) << 31));
          (*r) = dbRes;
        } else {
          /* Path 9). Here if y=0, x<=-0 */
          /* Res := Pi */
          dbRes = ((__constant double *)__datan2_ha_CoutTab)[242] +
                  ((__constant double *)__datan2_ha_CoutTab)[243];
          /* (*r) := sign(y) * Res */
          (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
               (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
               ((_iml_uint32_t)(iSignY) << 31));
          (*r) = dbRes;
        }
      }
    }
  } else {
    /* Here if one of arguments is Inf or NaN */
    if (((iEY == 0x7FF) &&
         !(((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword & 0x000FFFFF) == 0) &&
           ((((_iml_v2_dp_union_t *)&dbY)->dwords.lo_dword) == 0))) ||
        ((iEX == 0x7FF) &&
         !(((((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword & 0x000FFFFF) == 0) &&
           ((((_iml_v2_dp_union_t *)&dbX)->dwords.lo_dword) == 0)))) {
      /* Path 1). Here if one of arguments is NaN */
      (*r) = dbY + dbX;
    } else {
      /* Here if none of arguments is NaN */
      /* and one of arguments is Inf      */
      if ((((((_iml_v2_dp_union_t *)&dbX)->dwords.hi_dword >> 20) & 0x7FF) !=
           0x7FF)) {
        /* Path 2). x is finite, y=Inf */
        /* Res := Pi/2 */
        dbRes = ((__constant double *)__datan2_ha_CoutTab)[240] +
                ((__constant double *)__datan2_ha_CoutTab)[241];
        /* (*r) := sign(y) * Res */
        (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
             ((_iml_uint32_t)(iSignY) << 31));
        (*r) = dbRes;
      } else {
        /* x=Inf */
        if ((((((_iml_v2_dp_union_t *)&dbY)->dwords.hi_dword >> 20) & 0x7FF) !=
             0x7FF)) {
          /* y is finite, x=Inf */
          if (iSignX == 0) {
            /* Path 3). Here if y is finite, x=+Inf */
            /* Res := 0 */
            dbRes = ((__constant double *)__datan2_ha_CoutTab)[247];
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          } else {
            /* Path 4). Here if y is finite, x=-Inf */
            /* Res := Pi */
            dbRes = ((__constant double *)__datan2_ha_CoutTab)[242] +
                    ((__constant double *)__datan2_ha_CoutTab)[243];
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          }
        } else {
          /* y=Inf, x=Inf */
          if (iSignX == 0) {
            /* Path 5). Here if y=Inf, x=+Inf */
            /* Res := Pi/4 */
            dbRes = ((__constant double *)__datan2_ha_CoutTab)[238] +
                    ((__constant double *)__datan2_ha_CoutTab)[239];
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          } else {
            /* Path 6). Here if y=Inf, x=-Inf */
            /* Res := Pi*3/4 */
            dbRes = ((__constant double *)__datan2_ha_CoutTab)[244] +
                    ((__constant double *)__datan2_ha_CoutTab)[245];
            /* (*r) := sign(y) * Res */
            (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword =
                 (((_iml_v2_dp_union_t *)&dbRes)->dwords.hi_dword & 0x7FFFFFFF) |
                 ((_iml_uint32_t)(iSignY) << 31));
            (*r) = dbRes;
          }
        }
      }
    }
  }
  return nRet;
}
double __ocl_svml_atan2_ha(double x, double y) {
  double r;
  unsigned int vm;
  double va1;
  double va2;
  double vr1;
  va1 = x;
  va2 = y;
  {
    unsigned int mSpecArgs;
    double dSignX;
    double dSignY;
    double dAX;
    double dAY;
    double dYHi;
    double dYLo;
    double dXHi;
    double dXLo;
    double dPiOrZeroHi;
    double dPiOrZeroLo;
    unsigned long lAX;
    unsigned long lAY;
    unsigned int iAX;
    unsigned int iAY;
    unsigned int iAX1;
    unsigned int iAY1;
    unsigned int iXBad;
    unsigned int iYBad;
    unsigned int iXYBad;
    double dB1X;
    double dB2X;
    double dB3X;
    double dB4X;
    double dBase;
    double dAtanBaseHi;
    double dAtanBaseLo;
    double dC;
    double dCYHi;
    double dCYLo;
    double dA1Hi;
    double dA1Lo;
    double dA2Hi;
    double dALoHi;
    double dA2Lo;
    double dAHi;
    double dA2HiLo;
    double dALo;
    double dCXHi;
    double dCXLo;
    double dB1Hi;
    double dB1Lo;
    double dB2Hi;
    double dMB1LoHi;
    double dB2Lo;
    double dBHi;
    double dB2HiLo;
    double dBLo;
    double dR0;
    double dEHi;
    double dE;
    double dInv;
    double dInvLo;
    double dT0;
    double dT1;
    double dRLo;
    double dR;
    double dR2;
    double dR4;
    double dP1;
    double dP2;
    double dP3;
    double dSIGN_MASK;
    double dABS_MASK;
    double dPiHI;
    double dPiLO;
    double dHIGH_26_MASK;
    double dATAN_BOUND1;
    double dATAN_BOUND3;
    double dATAN_BOUND4;
    double dATAN_BOUND2;
    double dATAN_A11;
    double dATAN_A10;
    double dATAN_A09;
    double dATAN_A08;
    double dATAN_A07;
    double dATAN_A06;
    double dATAN_A05;
    double dATAN_A04;
    double dATAN_A03;
    double dATAN_A02;
    double dATAN_A01;
    double dATAN_A00;
    unsigned int iCHK_WORK_SUB;
    unsigned int iCHK_WORK_CMP;
    double dZERO;
    double dONE;
    double dXNegMask;
    unsigned long lB1X;
    unsigned long lB2X;
    unsigned long lB3X;
    unsigned long lB4X;
    unsigned int iB1X;
    unsigned int iB2X;
    unsigned int iB3X;
    unsigned int iB4X;
    unsigned int iB1AXLessAY1;
    unsigned int iB2AXLessAY1;
    unsigned int iB3AXLessAY1;
    unsigned int iB4AXLessAY1;
    unsigned int iB1AXLessAY;
    unsigned int iB2AXLessAY;
    unsigned int iB3AXLessAY;
    unsigned int iB4AXLessAY;
    unsigned int i4;
    unsigned int iIndex;
    unsigned int iBias;
    unsigned int iZero;
    /* Get r0~=1/B */
    /* Cannot be replaced by VQRCP(D, dR0, dB); */
    // Argument Absolute values
    dABS_MASK = as_double(__ocl_svml_internal_datan2_ha_data.dABS_MASK);
    dAX = as_double((as_ulong(va2) & as_ulong(dABS_MASK)));
    dAY = as_double((as_ulong(va1) & as_ulong(dABS_MASK)));
    // Argument signs
    dSignX = as_double((as_ulong(va2) ^ as_ulong(dAX)));
    dSignY = as_double((as_ulong(va1) ^ as_ulong(dAY)));
    // Get PiOrZero = Pi (if x<0), or Zero (if x>0)
    dZERO = as_double(__ocl_svml_internal_datan2_ha_data.dZERO);
    dPiHI = as_double(__ocl_svml_internal_datan2_ha_data.dPiHI);
    dPiLO = as_double(__ocl_svml_internal_datan2_ha_data.dPiLO);
    dXNegMask =
        as_double((unsigned long)((va2 < dZERO) ? 0xffffffffffffffff : 0x0));
    dPiHI = as_double((as_ulong(dXNegMask) & as_ulong(dPiHI)));
    dPiLO = as_double((as_ulong(dXNegMask) & as_ulong(dPiLO)));
    // Check if y and x are on main path.
    lAX = as_ulong(dAX);
    lAY = as_ulong(dAY);
    iAX = ((unsigned int)((unsigned long)lAX >> 32));
    iAY = ((unsigned int)((unsigned long)lAY >> 32));
    iCHK_WORK_SUB = (__ocl_svml_internal_datan2_ha_data.iCHK_WORK_SUB);
    iCHK_WORK_CMP = (__ocl_svml_internal_datan2_ha_data.iCHK_WORK_CMP);
    iAX1 = (iAX - iCHK_WORK_SUB);
    iAY1 = (iAY - iCHK_WORK_SUB);
    iXBad = ((unsigned int)(-(signed int)((signed int)iAX1 >=
                                          (signed int)iCHK_WORK_CMP)));
    iYBad = ((unsigned int)(-(signed int)((signed int)iAY1 >=
                                          (signed int)iCHK_WORK_CMP)));
    iXYBad = (iXBad | iYBad);
    vm = 0;
    mSpecArgs = 0;
    mSpecArgs = iXYBad;
    // Determining table index (to which interval is subject y/x)
    // Reduction of |y/x| to the interval [0;7/16].
    // 0. If 39/16 < |y/x| <  Inf  then a=0*y-  1*x, b=0*x+  1*y,
    // AtanHi+AtanLo=atan(Inf).
    // 1. If 19/16 < |y/x| < 39/16 then a=1*y-1.5*x, b=1*x+1.5*y,
    // AtanHi+AtanLo=atan(3/2).
    // 2. If 11/16 < |y/x| < 19/16 then a=1*y-1.0*x, b=1*x+1.0*y,
    // AtanHi+AtanLo=atan( 1 ).
    // 3. If  7/16 < |y/x| < 11/16 then a=1*y-0.5*x, b=1*x+0.5*y,
    // AtanHi+AtanLo=atan(1/2).
    // 4. If   0   < |y/x| <  7/16 then a=1*y-0  *x, b=1*x+0  *y,
    // AtanHi+AtanLo=atan( 0 ). Hence common formulas are:       a=c*y-  d*x,
    // b=c*x+  d*y (c is mask in our case) (b is always positive)
    dATAN_BOUND1 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_BOUND1);
    dATAN_BOUND2 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_BOUND2);
    dATAN_BOUND3 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_BOUND3);
    dATAN_BOUND4 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_BOUND4);
    dONE = as_double(__ocl_svml_internal_datan2_ha_data.dONE);
    dB1X = (dAX * dATAN_BOUND1);
    dB2X = (dAX * dATAN_BOUND2);
    dB3X = (dAX * dATAN_BOUND3);
    dB4X = (dAX * dATAN_BOUND4);
    lB1X = as_ulong(dB1X);
    lB2X = as_ulong(dB2X);
    lB3X = as_ulong(dB3X);
    lB4X = as_ulong(dB4X);
    iB1X = ((unsigned int)((unsigned long)lB1X >> 32));
    iB2X = ((unsigned int)((unsigned long)lB2X >> 32));
    iB3X = ((unsigned int)((unsigned long)lB3X >> 32));
    iB4X = ((unsigned int)((unsigned long)lB4X >> 32));
    iB1AXLessAY1 = (iB1X - iAY);
    iB2AXLessAY1 = (iB2X - iAY);
    iB3AXLessAY1 = (iB3X - iAY);
    iB4AXLessAY1 = (iB4X - iAY);
    iB1AXLessAY = ((signed int)iB1AXLessAY1 >> (31));
    iB2AXLessAY = ((signed int)iB2AXLessAY1 >> (31));
    iB3AXLessAY = ((signed int)iB3AXLessAY1 >> (31));
    iB4AXLessAY = ((signed int)iB4AXLessAY1 >> (31));
    iB1AXLessAY = (iB1AXLessAY + iB2AXLessAY);
    iB3AXLessAY = (iB3AXLessAY + iB4AXLessAY);
    iIndex = (iB1AXLessAY + iB3AXLessAY);
    i4 = (__ocl_svml_internal_datan2_ha_data.i4);
    iIndex = (iIndex + i4);
    // And load table values
    dBase = as_double(
        ((unsigned long *)(__ocl_svml_internal_datan2_ha_data
                               .ATAN_TBL))[(((0 + iIndex) * (4 * 8)) >> (3)) +
                                           0]);
    dAtanBaseHi = as_double(
        ((unsigned long *)(__ocl_svml_internal_datan2_ha_data
                               .ATAN_TBL))[(((0 + iIndex) * (4 * 8)) >> (3)) +
                                           1]);
    dAtanBaseLo = as_double(
        ((unsigned long *)(__ocl_svml_internal_datan2_ha_data
                               .ATAN_TBL))[(((0 + iIndex) * (4 * 8)) >> (3)) +
                                           2]);
    dC = as_double(
        ((unsigned long *)(__ocl_svml_internal_datan2_ha_data
                               .ATAN_TBL))[(((0 + iIndex) * (4 * 8)) >> (3)) +
                                           3]);
    dCYHi = as_double((as_ulong(dC) & as_ulong(dAY)));
    dAHi = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dBase), dAX, dCYHi);
    dCXHi = as_double((as_ulong(dC) & as_ulong(dAX)));
    dBHi = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dBase, dAY, dCXHi);
    // Divide r:=a*(1/b), where a==AHi+ALo, b==BHi+BLo, 1/b~=InvHi+InvLo
    // Get r0~=1/BHi
    {
      unsigned long lB;
      unsigned int iB;
      unsigned int ieb;
      unsigned int imeb;
      unsigned int iB1;
      unsigned int iMB;
      float sMB;
      float sR0;
      unsigned int iR0;
      unsigned long lR0;
      unsigned int idEXP_MASK;
      unsigned int id2_BIAS;
      unsigned int isMANTISSA_MASK;
      unsigned int isONE;
      unsigned int idBIAS;
      double dHIGH_20_MASK;
      idEXP_MASK = (__ocl_svml_internal_datan2_ha_data.idEXP_MASK);
      id2_BIAS = (__ocl_svml_internal_datan2_ha_data.id2_BIAS);
      isMANTISSA_MASK = (__ocl_svml_internal_datan2_ha_data.isMANTISSA_MASK);
      isONE = (__ocl_svml_internal_datan2_ha_data.isONE);
      idBIAS = (__ocl_svml_internal_datan2_ha_data.idBIAS);
      dHIGH_20_MASK =
          as_double(__ocl_svml_internal_datan2_ha_data.dHIGH_20_MASK);
      lB = as_ulong(dBHi);
      iB = ((unsigned int)((unsigned long)lB >> 32));
      ieb = (iB & idEXP_MASK);
      imeb = (id2_BIAS - ieb);
      iB1 = ((unsigned int)(iB) << (3));
      iMB = (iB1 & isMANTISSA_MASK);
      iMB = (iMB | isONE);
      sMB = as_float(iMB);
      sR0 = (1.0f / (sMB));
      iR0 = as_uint(sR0);
      iR0 = ((unsigned int)(iR0) >> (3));
      iR0 = (iR0 - idBIAS);
      iR0 = (iR0 + imeb);
      lR0 = (((unsigned long)(unsigned int)iR0 << 32) |
             (unsigned long)(unsigned int)iR0);
      dR0 = as_double(lR0);
      dR0 = as_double((as_ulong(dR0) & as_ulong(dHIGH_20_MASK)));
    };
    // Now refine r0 by several iterations (hidden in polynomial)
    // e = 1-Bhi*r0
    dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dBHi), dR0, dONE);
    // e + e^2
    dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dE, dE, dE);
    // r0 ~= 1/Bhi*(1-e)(1+e) or 1/Bhi*(1-e)(1+e+e^2)
    dR0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR0, dE, dR0);
    // e' = 1-Bhi*r0
    dE = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dBHi), dR0, dONE);
    // r0 ~= 1/Bhi*(1-e')(1+e') = 1/Bhi(1-e'^2)
    dR0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR0, dE, dR0);
    // Now 1/B ~= R0 + InvLo
    // Get r:=a*(1/b), where a==AHi+ALo, 1/b~=InvHi+InvLo
    dR = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dR0, dAHi, dZERO);
    dInvLo = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-(dBHi), dR, dAHi);
    dRLo = (dInvLo * dR0);
    // Atan polynomial approximation
    dATAN_A11 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A11);
    dATAN_A10 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A10);
    dATAN_A09 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A09);
    dATAN_A08 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A08);
    dATAN_A07 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A07);
    dATAN_A06 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A06);
    dATAN_A05 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A05);
    dATAN_A04 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A04);
    dATAN_A03 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A03);
    dATAN_A02 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A02);
    dATAN_A01 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A01);
    dATAN_A00 = as_double(__ocl_svml_internal_datan2_ha_data.dATAN_A00);
    dR2 = (dR * dR);
    dR4 = (dR2 * dR2);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dATAN_A11, dR4, dATAN_A09);
    dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dATAN_A10, dR4, dATAN_A08);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR4, dATAN_A07);
    dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP2, dR4, dATAN_A06);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR4, dATAN_A05);
    dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP2, dR4, dATAN_A04);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR4, dATAN_A03);
    dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP2, dR4, dATAN_A02);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR4, dATAN_A01);
    dP2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP2, dR4, dATAN_A00);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR2, dP2);
    dP1 = (dP1 * dR2);
    //  Res = ( (RLo + AtanBaseLo + PiOrZeroLo*sx + Poly(R^2)*R^3 + RHi +
    //  AtanBaseHi) * sx + PiOrZeroHi) * sy
    // Get PiOrZero = Pi (if x<0), or Zero (if x>0)
    dPiLO = as_double((as_ulong(dPiLO) ^ as_ulong(dSignX)));
    dRLo = (dRLo + dAtanBaseLo);
    dRLo = (dRLo + dPiLO);
    dP1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dP1, dR, dRLo);
    dP1 = (dR + dP1);
    dP1 = (dP1 + dAtanBaseHi);
    dP1 = as_double((as_ulong(dP1) ^ as_ulong(dSignX)));
    dP1 = (dP1 + dPiHI);
    vr1 = as_double((as_ulong(dP1) | as_ulong(dSignY)));
    /* =========== Special branch for fast (vector) processing of zero arguments
     * ================ */
    if (__builtin_expect((mSpecArgs) != 0, 0)) {
      double dBZero;
      double dSpecRes;
      double dSpecArgsMask;
      double dXnotNAN;
      double dYnotNAN;
      double dXYnotNAN;
      unsigned long lXYnotNAN;
      unsigned int iXYnotNAN;
      unsigned long lZERO;
      unsigned int iZERO;
      unsigned long lXYBad;
      unsigned long lARG2;
      unsigned int iARG2;
      unsigned int iXNeg;
      unsigned long lXNeg;
      double dAXZERO;
      double dAYZERO;
      double dAXAYZERO;
      unsigned long lAXAYZERO;
      unsigned int iAXAYZERO;
      unsigned int iCallout;
      unsigned int iAXAYZEROnotNAN;
      unsigned long lAXAYZEROnotNAN;
      double dM;
      double dXNeg;
      double dSX;
      double dSY;
      double dP;
      double dB;
      double dPIO2;
      double dPI;
      dPIO2 = as_double(__ocl_svml_internal_datan2_ha_data.dPIO2);
      dPI = as_double(__ocl_svml_internal_datan2_ha_data.dPI);
      dSIGN_MASK = as_double(__ocl_svml_internal_datan2_ha_data.dSIGN_MASK);
      dSX = as_double((as_ulong(va2) & as_ulong(dSIGN_MASK)));
      dSY = as_double((as_ulong(va1) & as_ulong(dSIGN_MASK)));
      // 1) If y<x then PIO2=0
      // 2) If y>x then PIO2=Pi/2
      dM = as_double(
          (unsigned long)(((!(dAY < dAX)) ? 0xffffffffffffffff : 0x0)));
      dPIO2 = as_double((as_ulong(dM) & as_ulong(dPIO2)));
      dB = as_double(
          (((~as_ulong(dM)) & as_ulong(dAX)) | (as_ulong(dM) & as_ulong(dAY))));
      /* Check if both X & Y are not NaNs:  iXYnotNAN */
      dXnotNAN = as_double((unsigned long)(((va2 == va2) & (va2 == va2))
                                               ? 0xffffffffffffffff
                                               : 0x0));
      dYnotNAN = as_double((unsigned long)(((va1 == va1) & (va1 == va1))
                                               ? 0xffffffffffffffff
                                               : 0x0));
      dXYnotNAN = as_double((as_ulong(dXnotNAN) & as_ulong(dYnotNAN)));
      lXYnotNAN = as_ulong(dXYnotNAN);
      iXYnotNAN = ((unsigned int)((unsigned long)lXYnotNAN >> 32));
      /* Check if at least on of Y or Y is zero: iAXAYZERO */
      dAXZERO =
          as_double((unsigned long)((dAX == dZERO) ? 0xffffffffffffffff : 0x0));
      dAYZERO =
          as_double((unsigned long)((dAY == dZERO) ? 0xffffffffffffffff : 0x0));
      dAXAYZERO = as_double((as_ulong(dAXZERO) | as_ulong(dAYZERO)));
      lAXAYZERO = as_ulong(dAXAYZERO);
      iAXAYZERO = ((unsigned int)((unsigned long)lAXAYZERO >> 32));
      /* Check if at least on of Y or Y is zero and not NaN: iAXAYZEROnotNAN */
      iAXAYZEROnotNAN = (iAXAYZERO & iXYnotNAN);
      iCallout = (~(iAXAYZEROnotNAN)&iXYBad);
      vm = 0;
      vm = iCallout;
      /* -------- Path for zero arguments (at least one of both) --------------
       */
      /* Check if both args are zeros (den. is zero) */
      dBZero =
          as_double((unsigned long)((dB == dZERO) ? 0xffffffffffffffff : 0x0));
      /* Set sPIO2 to zero if den. is zero */
      dPIO2 = dPIO2;
      dPIO2 = as_double((((~as_ulong(dBZero)) & as_ulong(dPIO2)) |
                         (as_ulong(dBZero) & as_ulong(dZERO))));
      dP = as_double((as_ulong(dPIO2) | as_ulong(dSX)));
      /* Res = sign(Y)*(X<0)?(PIO2+PI):PIO2 */
      lZERO = as_ulong(dZERO);
      iZERO = ((unsigned int)((unsigned long)lZERO >> 32));
      lARG2 = as_ulong(va2);
      iARG2 = ((unsigned int)((unsigned long)lARG2 >> 32));
      iXNeg = ((
          unsigned int)(-(signed int)((signed int)iARG2 < (signed int)iZERO)));
      lXNeg = (((unsigned long)(unsigned int)iXNeg << 32) |
               (unsigned long)(unsigned int)iXNeg);
      dXNeg = as_double(lXNeg);
      dPI = as_double((as_ulong(dXNeg) & as_ulong(dPI)));
      dP = (dP + dPI);
      dSpecRes = as_double((as_ulong(dP) | as_ulong(dSY)));
      /* Merge results from main and spec path */
      lAXAYZEROnotNAN = (((unsigned long)(unsigned int)iAXAYZEROnotNAN << 32) |
                         (unsigned long)(unsigned int)iAXAYZEROnotNAN);
      dSpecArgsMask = as_double(lAXAYZEROnotNAN);
      vr1 = as_double((((~as_ulong(dSpecArgsMask)) & as_ulong(vr1)) |
                       (as_ulong(dSpecArgsMask) & as_ulong(dSpecRes))));
    }
    /* =========== Special branch for fast (vector) processing of zero arguments
     * ================ */
  }
  if (__builtin_expect((vm) != 0, 0)) {
    double __cout_a1;
    double __cout_a2;
    double __cout_r1;
    ((double *)&__cout_a1)[0] = va1;
    ((double *)&__cout_a2)[0] = va2;
    ((double *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_datan2_ha(&__cout_a1, &__cout_a2, &__cout_r1);
    vr1 = ((double *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
