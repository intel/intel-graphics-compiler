/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long Frac_PowerD0[16];
    unsigned long poly_coeff1;
    unsigned long poly_coeff2;
    unsigned long poly_coeff3;

    unsigned long poly_coeff4;
    unsigned long poly_coeff5;
    unsigned long poly_coeff6;

    unsigned long add_const;
    unsigned long AbsMask;
    unsigned long Threshold;
    unsigned long _lIndexMask;
} __internal_dexp2_la_data_avx512_t;
static __constant __internal_dexp2_la_data_avx512_t __internal_dexp2_la_data_avx512 = {
    {
     0x3FF0000000000000uL,
     0x3FF0B5586CF9890FuL,
     0x3FF172B83C7D517BuL,
     0x3FF2387A6E756238uL,
     0x3FF306FE0A31B715uL,
     0x3FF3DEA64C123422uL,
     0x3FF4BFDAD5362A27uL,
     0x3FF5AB07DD485429uL,
     0x3FF6A09E667F3BCDuL,
     0x3FF7A11473EB0187uL,
     0x3FF8ACE5422AA0DBuL,
     0x3FF9C49182A3F090uL,
     0x3FFAE89F995AD3ADuL,
     0x3FFC199BDD85529CuL,
     0x3FFD5818DCFBA487uL,
     0x3FFEA4AFA2A490DAuL,
     },

    0x3FE62E42FEFA398BuL,
    0x3FCEBFBDFF84555AuL,
    0x3FAC6B08D4AD86B9uL,
    0x3F83B2AD1B172252uL,
    0x3F55D7472713CD19uL,
    0x3F24A1D7F526371BuL,

    0x42F8000000000000uL,
    0x7fffffffffffffffuL,
    0x408fefff00000000uL,
    0x000000000000000FuL,
};

typedef struct
{
    unsigned long _dbT[(1 << 7)];
    unsigned long _dbShifter;

    unsigned long _dPC1;
    unsigned long _dPC2;
    unsigned long _dPC3;
    unsigned long _dPC4;

    unsigned long _lIndexMask;
    unsigned int _iAbsMask;
    unsigned int _iDomainRange;
} __internal_dexp2_la_data_t;
static __constant __internal_dexp2_la_data_t __internal_dexp2_la_data = {
    {
     0x3ff0000000000000uL, 0x3ff0163da9fb3335uL,
     0x3ff02c9a3e778061uL, 0x3ff04315e86e7f85uL,
     0x3ff059b0d3158574uL, 0x3ff0706b29ddf6deuL,
     0x3ff0874518759bc8uL, 0x3ff09e3ecac6f383uL,
     0x3ff0b5586cf9890fuL, 0x3ff0cc922b7247f7uL,
     0x3ff0e3ec32d3d1a2uL, 0x3ff0fb66affed31buL,
     0x3ff11301d0125b51uL, 0x3ff12abdc06c31ccuL,
     0x3ff1429aaea92de0uL, 0x3ff15a98c8a58e51uL,
     0x3ff172b83c7d517buL, 0x3ff18af9388c8deauL,
     0x3ff1a35beb6fcb75uL, 0x3ff1bbe084045cd4uL,
     0x3ff1d4873168b9aauL, 0x3ff1ed5022fcd91duL,
     0x3ff2063b88628cd6uL, 0x3ff21f49917ddc96uL,
     0x3ff2387a6e756238uL, 0x3ff251ce4fb2a63fuL,
     0x3ff26b4565e27cdduL, 0x3ff284dfe1f56381uL,
     0x3ff29e9df51fdee1uL, 0x3ff2b87fd0dad990uL,
     0x3ff2d285a6e4030buL, 0x3ff2ecafa93e2f56uL,
     0x3ff306fe0a31b715uL, 0x3ff32170fc4cd831uL,
     0x3ff33c08b26416ffuL, 0x3ff356c55f929ff1uL,
     0x3ff371a7373aa9cbuL, 0x3ff38cae6d05d866uL,
     0x3ff3a7db34e59ff7uL, 0x3ff3c32dc313a8e5uL,
     0x3ff3dea64c123422uL, 0x3ff3fa4504ac801cuL,
     0x3ff4160a21f72e2auL, 0x3ff431f5d950a897uL,
     0x3ff44e086061892duL, 0x3ff46a41ed1d0057uL,
     0x3ff486a2b5c13cd0uL, 0x3ff4a32af0d7d3deuL,
     0x3ff4bfdad5362a27uL, 0x3ff4dcb299fddd0duL,
     0x3ff4f9b2769d2ca7uL, 0x3ff516daa2cf6642uL,
     0x3ff5342b569d4f82uL, 0x3ff551a4ca5d920fuL,
     0x3ff56f4736b527dauL, 0x3ff58d12d497c7fduL,
     0x3ff5ab07dd485429uL, 0x3ff5c9268a5946b7uL,
     0x3ff5e76f15ad2148uL, 0x3ff605e1b976dc09uL,
     0x3ff6247eb03a5585uL, 0x3ff6434634ccc320uL,
     0x3ff6623882552225uL, 0x3ff68155d44ca973uL,
     0x3ff6a09e667f3bcduL, 0x3ff6c012750bdabfuL,
     0x3ff6dfb23c651a2fuL, 0x3ff6ff7df9519484uL,
     0x3ff71f75e8ec5f74uL, 0x3ff73f9a48a58174uL,
     0x3ff75feb564267c9uL, 0x3ff780694fde5d3fuL,
     0x3ff7a11473eb0187uL, 0x3ff7c1ed0130c132uL,
     0x3ff7e2f336cf4e62uL, 0x3ff80427543e1a12uL,
     0x3ff82589994cce13uL, 0x3ff8471a4623c7aduL,
     0x3ff868d99b4492eduL, 0x3ff88ac7d98a6699uL,
     0x3ff8ace5422aa0dbuL, 0x3ff8cf3216b5448cuL,
     0x3ff8f1ae99157736uL, 0x3ff9145b0b91ffc6uL,
     0x3ff93737b0cdc5e5uL, 0x3ff95a44cbc8520fuL,
     0x3ff97d829fde4e50uL, 0x3ff9a0f170ca07bauL,
     0x3ff9c49182a3f090uL, 0x3ff9e86319e32323uL,
     0x3ffa0c667b5de565uL, 0x3ffa309bec4a2d33uL,
     0x3ffa5503b23e255duL, 0x3ffa799e1330b358uL,
     0x3ffa9e6b5579fdbfuL, 0x3ffac36bbfd3f37auL,
     0x3ffae89f995ad3aduL, 0x3ffb0e07298db666uL,
     0x3ffb33a2b84f15fbuL, 0x3ffb59728de5593auL,
     0x3ffb7f76f2fb5e47uL, 0x3ffba5b030a1064auL,
     0x3ffbcc1e904bc1d2uL, 0x3ffbf2c25bd71e09uL,
     0x3ffc199bdd85529cuL, 0x3ffc40ab5fffd07auL,
     0x3ffc67f12e57d14buL, 0x3ffc8f6d9406e7b5uL,
     0x3ffcb720dcef9069uL, 0x3ffcdf0b555dc3fauL,
     0x3ffd072d4a07897cuL, 0x3ffd2f87080d89f2uL,
     0x3ffd5818dcfba487uL, 0x3ffd80e316c98398uL,
     0x3ffda9e603db3285uL, 0x3ffdd321f301b460uL,
     0x3ffdfc97337b9b5fuL, 0x3ffe264614f5a129uL,
     0x3ffe502ee78b3ff6uL, 0x3ffe7a51fbc74c83uL,
     0x3ffea4afa2a490dauL, 0x3ffecf482d8e67f1uL,
     0x3ffefa1bee615a27uL, 0x3fff252b376bba97uL,
     0x3fff50765b6e4540uL, 0x3fff7bfdad9cbe14uL,
     0x3fffa7c1819e90d8uL, 0x3fffd3c22b8f71f1uL},

    0x42c8000000000000uL,

    0x3fe62e42fefa3685uL,
    0x3fcebfbdff82ca48uL,
    0x3fac6b09b180f045uL,
    0x3f83b2ab5bb1268fuL,

    0x000000000000007fuL,

    0x7fffffffu,
    0x408fefffu
};

static __constant _iml_v2_dp_union_t _imldExp2HATab_v2[144] = {

    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0x3E778061, 0x3FF02C9A,
    0x9CD8DC5D, 0xBC716013,
    0xD3158574, 0x3FF059B0,
    0x3567F613, 0x3C8CD252,
    0x18759BC8, 0x3FF08745,
    0x61E6C861, 0x3C60F74E,
    0x6CF9890F, 0x3FF0B558,
    0x5D837B6D, 0x3C979AA6,
    0x32D3D1A2, 0x3FF0E3EC,
    0x702F9CD1, 0x3C3EBE3D,
    0xD0125B51, 0x3FF11301,
    0x2A2FBD0E, 0xBC955652,
    0xAEA92DE0, 0x3FF1429A,
    0xB9D5F416, 0xBC91C923,
    0x3C7D517B, 0x3FF172B8,
    0xEAA59348, 0xBC801B15,
    0xEB6FCB75, 0x3FF1A35B,
    0x3F1353BF, 0x3C8B898C,
    0x3168B9AA, 0x3FF1D487,
    0x3E3A2F60, 0x3C9AECF7,
    0x88628CD6, 0x3FF2063B,
    0x44A6C38D, 0x3C8A6F41,
    0x6E756238, 0x3FF2387A,
    0xE3A8A894, 0x3C968EFD,
    0x65E27CDD, 0x3FF26B45,
    0x981FE7F2, 0x3C80472B,
    0xF51FDEE1, 0x3FF29E9D,
    0x6D09AB31, 0x3C82F7E1,
    0xA6E4030B, 0x3FF2D285,
    0x720C0AB4, 0x3C8B3782,
    0x0A31B715, 0x3FF306FE,
    0x4DB0ABB6, 0x3C834D75,
    0xB26416FF, 0x3FF33C08,
    0x5DD3F84A, 0x3C8FDD39,
    0x373AA9CB, 0x3FF371A7,
    0xCC4B5068, 0xBC924AED,
    0x34E59FF7, 0x3FF3A7DB,
    0x3E9436D2, 0xBC71D1E8,
    0x4C123422, 0x3FF3DEA6,
    0xA72A4C6D, 0x3C859F48,
    0x21F72E2A, 0x3FF4160A,
    0x4817895B, 0xBC58A78F,
    0x6061892D, 0x3FF44E08,
    0x60C2AC11, 0x3C4363ED,
    0xB5C13CD0, 0x3FF486A2,
    0xDAA10379, 0x3C6ECCE1,
    0xD5362A27, 0x3FF4BFDA,
    0xBB7AAFB0, 0x3C7690CE,
    0x769D2CA7, 0x3FF4F9B2,
    0x0071A38E, 0xBC8F9434,
    0x569D4F82, 0x3FF5342B,
    0xBD0F385F, 0xBC78DEC6,
    0x36B527DA, 0x3FF56F47,
    0x18FDD78E, 0x3C933505,
    0xDD485429, 0x3FF5AB07,
    0xE21C5409, 0x3C9063E1,
    0x15AD2148, 0x3FF5E76F,
    0x2B64C035, 0x3C9432E6,
    0xB03A5585, 0x3FF6247E,
    0x3BEF4DA8, 0xBC8C33C5,
    0x82552225, 0x3FF66238,
    0x78565858, 0xBC93CEDD,
    0x667F3BCD, 0x3FF6A09E,
    0xBF5E2228, 0xBC93B3EF,
    0x3C651A2F, 0x3FF6DFB2,
    0xB86DA9EE, 0xBC6367EF,
    0xE8EC5F74, 0x3FF71F75,
    0x7E5A3ECF, 0xBC781F64,
    0x564267C9, 0x3FF75FEB,
    0x1E55E68A, 0xBC861932,
    0x73EB0187, 0x3FF7A114,
    0xB94DA51D, 0xBC7B32DC,
    0x36CF4E62, 0x3FF7E2F3,
    0xABD66C55, 0x3C65EBE1,
    0x994CCE13, 0x3FF82589,
    0xF13B3734, 0xBC9369B6,
    0x9B4492ED, 0x3FF868D9,
    0xD872576E, 0xBC94D450,
    0x422AA0DB, 0x3FF8ACE5,
    0xC1F0EAB4, 0x3C8DB72F,
    0x99157736, 0x3FF8F1AE,
    0x59F35F44, 0x3C7BF683,
    0xB0CDC5E5, 0x3FF93737,
    0x8B6C1E29, 0xBC5DA9B8,
    0x9FDE4E50, 0x3FF97D82,
    0x22F4F9AA, 0xBC924343,
    0x82A3F090, 0x3FF9C491,
    0x2B91CE27, 0x3C71AFFC,
    0x7B5DE565, 0x3FFA0C66,
    0x22622263, 0xBC87C504,
    0xB23E255D, 0x3FFA5503,
    0xD3BCBB15, 0xBC91BBD1,
    0x5579FDBF, 0x3FFA9E6B,
    0x6E735AB3, 0x3C846984,
    0x995AD3AD, 0x3FFAE89F,
    0x92CB3387, 0x3C8C1A77,
    0xB84F15FB, 0x3FFB33A2,
    0x56DCAEBA, 0xBC55C3D9,
    0xF2FB5E47, 0x3FFB7F76,
    0x38AD9334, 0xBC68D6F4,
    0x904BC1D2, 0x3FFBCC1E,
    0x0A5FDDCD, 0x3C74FFD7,
    0xDD85529C, 0x3FFC199B,
    0x30AF0CB3, 0x3C736EAE,
    0x2E57D14B, 0x3FFC67F1,
    0xD10959AC, 0x3C84E08F,
    0xDCEF9069, 0x3FFCB720,
    0x6C921968, 0x3C676B2C,
    0x4A07897C, 0x3FFD072D,
    0x3FFFFA6F, 0xBC8FAD5D,
    0xDCFBA487, 0x3FFD5818,
    0xA63D07A7, 0x3C74A385,
    0x03DB3285, 0x3FFDA9E6,
    0xD5C192AC, 0x3C8E5A50,
    0x337B9B5F, 0x3FFDFC97,
    0x07B43E1F, 0xBC82D521,
    0xE78B3FF6, 0x3FFE502E,
    0x603A88D3, 0x3C74B604,
    0xA2A490DA, 0x3FFEA4AF,
    0x8FD391F0, 0xBC8FF712,
    0xEE615A27, 0x3FFEFA1B,
    0x41AA2008, 0x3C8EC3BC,
    0x5B6E4540, 0x3FFF5076,
    0x31D185EE, 0x3C8A64A9,
    0x819E90D8, 0x3FFFA7C1,
    0x4D91CD9D, 0x3C77893B,

    0x00000000, 0x42D80000,

    0xfefa39ef, 0x3fe62e42,
    0xff82a23a, 0x3fcebfbd,
    0xd7076268, 0x3fac6b08,
    0x33f8b48b, 0x3f83b2ad,
    0xc4d8440a, 0x3f55d870,

    0x00000000, 0x40900000,
    0x00000000, 0xC08FF000,
    0x00000000, 0xC090CC00,

    0x00000001, 0x00100000,
    0xFFFFFFFF, 0x7FEFFFFF,

    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x40000000,

    0x00000000, 0x39B00000,
};

__attribute__((always_inline))
inline int __internal_dexp2_la_cout (double *a, double *r)
{

    int nRet = 0;
    double w, Nj;
    double R, p, scale, dbIn;
    _iml_uint32_t N, j;
    scale = ((__constant double *) _imldExp2HATab_v2)[139];
    dbIn = (*a);

    if ((((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {
        if (dbIn < ((__constant double *) _imldExp2HATab_v2)[134])
        {
            if (dbIn > ((__constant double *) _imldExp2HATab_v2)[136])
            {

                Nj = (dbIn + ((__constant double *) _imldExp2HATab_v2)[128]);
                w = (Nj - ((__constant double *) _imldExp2HATab_v2)[128]);
                R = (dbIn - w);

                p = (((((((__constant double *) _imldExp2HATab_v2)[133] * R + ((__constant double *) _imldExp2HATab_v2)[132]) * R +
                        ((__constant double *) _imldExp2HATab_v2)[131]) * R + ((__constant double *) _imldExp2HATab_v2)[130]) * R +
                      ((__constant double *) _imldExp2HATab_v2)[129]) * R);

                N = (((_iml_v2_dp_union_t *) & Nj)->dwords.lo_dword);
                j = N & ((1 << 6) - 1);
                N = N >> 6;
                N = N + 0x3FF;

                p = (p + ((__constant double *) _imldExp2HATab_v2)[2 * j + 1]);
                p = (p * ((__constant double *) _imldExp2HATab_v2)[2 * j]);
                p = (p + ((__constant double *) _imldExp2HATab_v2)[2 * j]);

                if (dbIn >= ((__constant double *) _imldExp2HATab_v2)[135])
                {

                    N = N & 0x7FF;

                    if (N <= (0x7FF - 1))
                    {

                        (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N) & 0x7FF) << 20));

                        (*r) = (scale * p);
                    }
                    else
                    {

                        (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                         (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N - 1) & 0x7FF) << 20));

                        p = (p * scale);
                        (*r) = (((__constant double *) _imldExp2HATab_v2)[141] * p);
                    }
                }
                else
                {

                    N = (N + 100);
                    N = N & 0x7FF;

                    (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword =
                     (((_iml_v2_dp_union_t *) & scale)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N) & 0x7FF) << 20));

                    p *= scale;
                    (*r) = (p * ((__constant double *) _imldExp2HATab_v2)[142]);
                    nRet = 4;
                }
            }
            else
            {

                (*r) = ((__constant double *) _imldExp2HATab_v2)[137] * ((__constant double *) _imldExp2HATab_v2)[137];
                nRet = 4;
            }
        }
        else
        {

            (*r) = ((__constant double *) _imldExp2HATab_v2)[138] * ((__constant double *) _imldExp2HATab_v2)[138];
            nRet = 3;
        }
    }
    else
    {

        if (((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword >> 31) == 1)
            && (((((_iml_v2_dp_union_t *) & dbIn)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_v2_dp_union_t *) & dbIn)->dwords.lo_dword) == 0)))
        {

            (*r) = ((__constant double *) _imldExp2HATab_v2)[139];
        }
        else
        {

            (*r) = dbIn * dbIn;
        }
    }

    return nRet;
}

double __ocl_svml_exp2 (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        double dN;
        double dM;
        double dR;
        double dT;
        unsigned long lM;
        unsigned long lX;
        unsigned int iAbsX;
        unsigned int iRangeMask;
        unsigned long lIndex;

        double dbShifter;
        double dPC[4];
        unsigned long lIndexMask;
        unsigned int iAbsMask;
        unsigned int iDomainRange;

        dbShifter = as_double (__internal_dexp2_la_data._dbShifter);
        lIndexMask = (__internal_dexp2_la_data._lIndexMask);
        dPC[0] = as_double (__internal_dexp2_la_data._dPC1);
        dPC[1] = as_double (__internal_dexp2_la_data._dPC2);
        dPC[2] = as_double (__internal_dexp2_la_data._dPC3);
        dPC[3] = as_double (__internal_dexp2_la_data._dPC4);
        iAbsMask = (__internal_dexp2_la_data._iAbsMask);
        iDomainRange = (__internal_dexp2_la_data._iDomainRange);

        dM = (va1 + dbShifter);
        dN = (dM - dbShifter);

        lX = as_ulong (va1);
        iAbsX = ((unsigned int) ((unsigned long) lX >> 32));
        iAbsX = (iAbsX & iAbsMask);
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iAbsX > (signed int) iDomainRange)));
        vm = 0;
        vm = iRangeMask;

        lM = as_ulong (dM);
        lIndex = (lM & lIndexMask);
        lM = (~(lIndexMask) & lM);
        dT = as_double (((__constant unsigned long *) (__internal_dexp2_la_data._dbT))[(((0 + lIndex) * (1 * 8)) >> (3)) + 0]);

        lM = ((unsigned long) (lM) << ((52 - 7)));

        dR = (va1 - dN);

        dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dPC[3], dR, dPC[2]);
        dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dN, dR, dPC[1]);
        dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dN, dR, dPC[0]);
        dR = (dR * dT);

        dN = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dN, dR, dT);

        lX = as_ulong (dN);
        lX = (lX + lM);

        vr1 = as_double (lX);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dexp2_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}
