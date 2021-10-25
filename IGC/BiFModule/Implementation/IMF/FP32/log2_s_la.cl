/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned int One;
    unsigned int coeff4[16];
    unsigned int coeff3[16];
    unsigned int coeff2[16];
    unsigned int coeff1[16];
} __internal_slog2_la_data_avx512_t;
static __constant __internal_slog2_la_data_avx512_t __internal_slog2_la_data_avx512 = {

    0x3f800000u, {
                  0xbea77e4au, 0xbe8aae3du,
                  0xbe67fe32u, 0xbe43d1b6u,
                  0xbe26a589u, 0xbe0ee09bu,
                  0xbdf6a8a1u, 0xbdd63b49u,
                  0xbf584e51u, 0xbf3e80a1u,
                  0xbf2892f0u, 0xbf15d377u,
                  0xbf05b525u, 0xbeef8e30u,
                  0xbed75c8fu, 0xbec24184u,
                  }

    , {
       0x3ef5910cu, 0x3ef045a1u,
       0x3ee7d87eu, 0x3eddbb84u,
       0x3ed2d6dfu, 0x3ec7bbd2u,
       0x3ebcc42fu, 0x3eb22616u,
       0x3e8f3399u, 0x3eb1223eu,
       0x3ec9db4au, 0x3edb7a09u,
       0x3ee79a1au, 0x3eef77cbu,
       0x3ef407a4u, 0x3ef607b4u,
       }

    , {
       0xbf38a934u, 0xbf387de6u,
       0xbf37f6f0u, 0xbf37048bu,
       0xbf35a88au, 0xbf33ed04u,
       0xbf31df56u, 0xbf2f8d82u,
       0xbf416814u, 0xbf3daf58u,
       0xbf3b5c08u, 0xbf39fa2au,
       0xbf393713u, 0xbf38d7e1u,
       0xbf38b2cdu, 0xbf38aa62u,
       }

    , {
       0x3fb8aa3bu, 0x3fb8a9c0u,
       0x3fb8a6e8u, 0x3fb89f4eu,
       0x3fb890cbu, 0x3fb879b1u,
       0x3fb858d8u, 0x3fb82d90u,
       0x3fb8655eu, 0x3fb8883au,
       0x3fb89aeau, 0x3fb8a42fu,
       0x3fb8a848u, 0x3fb8a9c9u,
       0x3fb8aa2fu, 0x3fb8aa3bu,
       }
};

typedef struct
{
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

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_slog2_la_data_t;
static __constant __internal_slog2_la_data_t __internal_slog2_la_data = {

    {
     0xc2fc0000u, 0x00000000u, 0xc2fc0400u, 0xb6eb07d9u, 0xc2fc0800u, 0x37ec18d1u, 0xc2fc0bf0u, 0xb78f3a71u, 0xc2fc0fe0u, 0xb7b076feu, 0xc2fc13d0u,
     0x3784b71fu, 0xc2fc17b0u, 0xb7f34d44u, 0xc2fc1b98u, 0x37e3ed42u, 0xc2fc1f70u, 0x355ccb60u, 0xc2fc2348u, 0x37645758u, 0xc2fc2718u, 0x3689621au,
     0xc2fc2ae0u, 0xb7ebea5du, 0xc2fc2ea8u, 0xb7bbe728u, 0xc2fc3270u, 0x37af24dbu, 0xc2fc3628u, 0xb7adf709u, 0xc2fc39e0u, 0xb7d65f2fu, 0xc2fc3d98u,
     0x36cb5092u, 0xc2fc4148u, 0x37552d13u, 0xc2fc44f0u, 0xb6c86327u, 0xc2fc4898u, 0x3733a188u, 0xc2fc4c38u, 0x35b68f67u, 0xc2fc4fd8u, 0x37dfd085u,
     0xc2fc5370u, 0x37d432e1u, 0xc2fc5700u, 0xb651cfdfu, 0xc2fc5a90u, 0x360e51d6u, 0xc2fc5e18u, 0xb7aa760fu, 0xc2fc61a0u, 0xb7234cffu, 0xc2fc6520u,
     0xb7e668fdu, 0xc2fc68a0u, 0xb756abbeu, 0xc2fc6c18u, 0xb7e2fc1eu, 0xc2fc6f90u, 0xb71fc2f6u, 0xc2fc7300u, 0xb7b482b2u, 0xc2fc7670u, 0xb61a9d07u,
     0xc2fc79d8u, 0xb75d7ee2u, 0xc2fc7d40u, 0x36db5cf5u, 0xc2fc80a0u, 0xb693a750u, 0xc2fc8400u, 0x377756b2u, 0xc2fc8758u, 0x363266b2u, 0xc2fc8ab0u,
     0x37a8b8a6u, 0xc2fc8e00u, 0x36c2d256u, 0xc2fc9150u, 0x37ac0e19u, 0xc2fc9498u, 0x3644d145u, 0xc2fc97e0u, 0x37686a41u, 0xc2fc9b20u, 0xb7067ac7u,
     0xc2fc9e60u, 0xb5fd2a77u, 0xc2fca198u, 0xb7f398eeu, 0xc2fca4d0u, 0xb7f0979au, 0xc2fca808u, 0xb58cf4b1u, 0xc2fcab38u, 0xb6f8e7f0u, 0xc2fcae68u,
     0x375a69c0u, 0xc2fcb190u, 0xb586fc11u, 0xc2fcb4b8u, 0x373de2cfu, 0xc2fcb7d8u, 0xb73d105bu, 0xc2fcbaf8u, 0xb7056dddu, 0xc2fcbe18u, 0x37b090b4u,
     0xc2fcc130u, 0x3773005eu, 0xc2fcc440u, 0xb7e9b13fu, 0xc2fcc758u, 0x3785395cu, 0xc2fcca68u, 0x37c4828du, 0xc2fccd70u, 0xb6b63673u, 0xc2fcd078u,
     0xb725534eu, 0xc2fcd380u, 0x3727027cu, 0xc2fcd680u, 0xb6f2d37cu, 0xc2fcd980u, 0xb51dd8f4u, 0xc2fcdc80u, 0x37f935cfu, 0xc2fcdf78u, 0x37bc202fu,
     0xc2fce268u, 0xb7bdc376u, 0xc2fce560u, 0x3789fbebu, 0xc2fce850u, 0x3791d41du, 0xc2fceb38u, 0xb7a7c066u, 0xc2fcee28u, 0x37dbbd7au, 0xc2fcf108u,
     0xb7e52ea4u, 0xc2fcf3f0u, 0x36201ac7u, 0xc2fcf9b0u, 0x36bbdfa8u, 0xc2fcff60u, 0xb7decd5fu, 0xc2fd0510u, 0x37df8e14u, 0xc2fd0aa8u, 0xb7a0da0bu,
     0xc2fd1040u, 0x37955516u, 0xc2fd15c8u, 0x376f670fu, 0xc2fd1b48u, 0x37fc14c5u, 0xc2fd20b8u, 0x36442fa3u, 0xc2fd2620u, 0xb6f2cf29u, 0xc2fd2b80u,
     0xb5d1cfdfu, 0xc2fd30d8u, 0x379dfa99u, 0xc2fd3620u, 0xb7091303u, 0xc2fd3b60u, 0xb7bd93fcu, 0xc2fd4098u, 0xb7d5df20u, 0xc2fd45c8u, 0xb795f62bu,
     0xc2fd4af0u, 0xb546d45du, 0xc2fd5010u, 0x37d12e6au, 0xc2fd5520u, 0xb63e66b1u, 0xc2fd5a28u, 0xb7c90426u, 0xc2fd5f30u, 0x37b5d5a4u, 0xc2fd6428u,
     0x373a6affu, 0xc2fd6918u, 0x3696dc9au, 0xc2fd6e00u, 0x3581c0a8u, 0xc2fd72e0u, 0xb4ad6a5fu, 0xc2fd77b8u, 0xb45a03bbu, 0xc2fd7c88u, 0x3509b65au,
     0xc2fd8150u, 0x3589e351u, 0xc2fd8610u, 0x3515a79cu, 0xc2fd8ac8u, 0xb5df5425u, 0xc2fd8f78u, 0xb6d667c3u, 0xc2fd9420u, 0xb770e5bau, 0xc2fd98c0u,
     0xb7dc9441u, 0xc2fd9d60u, 0x37980ac0u, 0xc2fda1f0u, 0xb681c27bu, 0xc2fda680u, 0x37f43a8au, 0xc2fdab00u, 0xb6be5f85u, 0xc2fdaf80u, 0x375d1cb6u,
     0xc2fdb3f8u, 0x37c93d2du, 0xc2fdb868u, 0x37db1d98u, 0xc2fdbcd0u, 0x379ef11bu, 0xc2fdc130u, 0x35f911cau, 0xc2fdc588u, 0xb7d811a6u, 0xc2fdc9e0u,
     0xb6677d2bu, 0xc2fdce30u, 0x36f053a0u, 0xc2fdd278u, 0x36b8be27u, 0xc2fdd6b8u, 0xb716adf4u, 0xc2fddaf8u, 0x37cadee4u, 0xc2fddf28u, 0xb793bd0du,
     0xc2fde358u, 0xb7574a57u, 0xc2fde780u, 0xb7c14076u, 0xc2fdeba8u, 0x374e3a0eu, 0xc2fdefc0u, 0xb7f6ce3du, 0xc2fdf3d8u, 0xb7df31afu, 0xc2fdf7f0u,
     0x37a9d4fbu, 0xc2fdfbf8u, 0xb73f8d4au, 0xc2fe0000u, 0x80000000u}

    , {
       0x3e25C86Au, 0xbeB2BB8Cu, 0x3b6CD7E0u}

    , 0x007fffffu, 0x3c000000u, 0x00800000u, 0x7f7fffffu, 0xffffff00u, 0x3fb80000u, 0x3f2aaaabu, 0x007fffffu, 0x3f800000u, {
                                                                                                                            0x3e554012u, 0xbe638E14u,
                                                                                                                            0x3e4D660Bu, 0xbe727824u,
                                                                                                                            0x3e93DD07u, 0xbeB8B969u,
                                                                                                                            0x3eF637C0u, 0xbf38AA2Bu,
                                                                                                                            0x3fB8AA3Bu}

    , {
       0x3eAAE01Bu, 0xbeCB686Cu, 0x3eF4FA41u, 0xbf38658Au, 0x3fB8AAEDu}

    , {0x7f800000u, 0xff800000u}

    , {0x3f800000u, 0xbf800000u}

    , {0x00000000u, 0x80000000u}

};

static __constant _iml_v2_sp_union_t __slog2_la_CoutTab[212] = {
    0x43b8aa40,
    0x00000000,
    0x00000000,
    0x43b5c797,
    0x3cba0000,
    0x377ba188,
    0x43b31319,
    0x3d358000,
    0x3821c52f,
    0x43b05e9a,
    0x3d87c000,
    0x37d31a33,
    0x43add846,
    0x3db26000,
    0x37626c06,
    0x43ab51f2,
    0x3ddda000,
    0x35817c93,
    0x43a8cb9f,
    0x3e04c000,
    0xb66398d0,
    0x43a67375,
    0x3e196800,
    0x361f2349,
    0x43a41b4c,
    0x3e2e6000,
    0xb7358bf1,
    0x43a1f14d,
    0x3e41f800,
    0x36c13371,
    0x439fb039,
    0x3e56b000,
    0xb68ee3fc,
    0x439d9d50,
    0x3e6a0000,
    0xb6aaf16d,
    0x439b8a66,
    0x3e7d9000,
    0xb493e2d1,
    0x4399777d,
    0x3e88b200,
    0x35dcef23,
    0x43977ba9,
    0x3e924e00,
    0x3652b13d,
    0x439596ea,
    0x3e9b9a00,
    0xb6fabe1f,
    0x4393b22b,
    0x3ea50200,
    0xb50d0b48,
    0x4391e481,
    0x3eae1600,
    0xb6fb4af6,
    0x439016d7,
    0x3eb74600,
    0xb6ffdeef,
    0x438e6043,
    0x3ec01a00,
    0x36cf5cc8,
    0x438ca9af,
    0x3ec90c00,
    0x32124dd4,
    0x438b0a30,
    0x3ed19e00,
    0x3692fe59,
    0x43896ab1,
    0x3eda4a00,
    0x36ea748c,
    0x4387e247,
    0x3ee29400,
    0x35710ee2,
    0x438642c8,
    0x3eeb7400,
    0x362cea8d,
    0x4384d173,
    0x3ef37000,
    0xb6bb4eef,
    0x4383490a,
    0x3efc0200,
    0x362e8d0d,
    0x4381d7b5,
    0x3f021600,
    0xb5ad1961,
    0x43807d76,
    0x3f05f400,
    0xb520a698,
    0x437e1843,
    0x3f0a2000,
    0xb6e00fcd,
    0x437b63c4,
    0x3f0e1400,
    0xb6fc627b,
    0x4378dd70,
    0x3f11ce00,
    0xb60dbf88,
    0x437628f2,
    0x3f15d700,
    0x3640e84c,
    0x4373a29e,
    0x3f19a600,
    0xb59cced4,
    0x43711c4a,
    0x3f1d7f00,
    0xb6405abc,
    0x436ec421,
    0x3f211a00,
    0x37214eb9,
    0x436c6bf7,
    0x3f24c000,
    0xb6df6565,
    0x436a13ce,
    0x3f286e00,
    0xb6525f63,
    0x4367bba5,
    0x3f2c2600,
    0xb6faec18,
    0x436591a6,
    0x3f2f9c00,
    0x3719ee59,
    0x4363397d,
    0x3f336700,
    0x35d3929b,
    0x43610f7e,
    0x3f36f000,
    0xb5f561c1,
    0x435f13aa,
    0x3f3a3500,
    0xb586531d,
    0x435ce9ab,
    0x3f3dce00,
    0x371dcc96,
    0x435aedd7,
    0x3f412400,
    0xb70acad0,
    0x4358c3d8,
    0x3f44ce00,
    0x37304eb4,
    0x4356f62f,
    0x3f47e400,
    0x3712644d,
    0x4354fa5a,
    0x3f4b5100,
    0x3608d068,
    0x4352fe86,
    0x3f4ec600,
    0xb5d0ab43,
    0x435130dd,
    0x3f51f200,
    0xb705a9de,
    0x434f6333,
    0x3f552400,
    0x35acd72a,
    0x434d9589,
    0x3f585e00,
    0xb6236cd4,
    0x434bc7e0,
    0x3f5b9f00,
    0xb6032e95,
    0x4349fa36,
    0x3f5ee800,
    0xb73119b5,
    0x43485ab7,
    0x3f61e200,
    0x36111bbc,
    0x43468d0d,
    0x3f653900,
    0x3483c7a3,
    0x4344ed8e,
    0x3f684100,
    0x358b4a3b,
    0x43434e0f,
    0x3f6b5000,
    0xb6ea3c0b,
    0x4341ae90,
    0x3f6e6400,
    0x370bf961,
    0x43403d3c,
    0x3f712800,
    0xb70ba8b0,
    0x433e9dbd,
    0x3f744900,
    0x35c23715,
    0x433d2c68,
    0x3f771800,
    0xb715b634,
    0x433b8ce9,
    0x3f7a4600,
    0x35d2f47a,
    0x433a1b95,
    0x3f7d2000,
    0x3657a488,
    0x4338aa40,
    0x3f800000,
    0x00000000,

    0x3F800000,
    0x00000000,

    0x48000040,

    0x46000000,

    0x3BC00000,

    0x4D000000,

    0x00000000,
    0x3F800000,
    0x43B8AA40,

    0xBF7F0000,
    0xB6B1720F,
    0x3223FE93,
    0xADAA8223,
    0x293D1988,
    0xA4DA74DC,
    0x2081CD9D,
    0x9C1D865E,
};

__attribute__((always_inline))
inline int __internal_slog2_la_cout (float *a, float *r)
{
    float x, y, u, q;
    float fP;
    float fAbsU;
    float fN, fNLg2Hi, fNLg2Lo;
    float fB, fLgRcprYHi, fLgRcprYLo, fWHi, fWLo;
    float fYHi, fYLo, fUHi, fULo, fResHi, fResLo;
    float fQHi, fQLo;
    float fTmp;
    int iN, j;
    int i;
    int nRet = 0;

    if ((((((_iml_v2_sp_union_t *) & (*a))->hex[0] >> 23) & 0xFF) != 0xFF))
    {

        x = (*a);
        iN = 0;

        if ((x != ((__constant float *) __slog2_la_CoutTab)[201]) && (((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) == 0))
        {

            x = (x * ((__constant float *) __slog2_la_CoutTab)[200]);
            iN -= 27;
        }

        if (x > ((__constant float *) __slog2_la_CoutTab)[201])
        {

            u = (x - 1.0f);
            fAbsU = u;
            (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] = (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (fAbsU > ((__constant float *) __slog2_la_CoutTab)[199])
            {

                iN += ((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) - 0x7F;
                fN = (float) iN;

                fNLg2Hi = (fN * ((__constant float *) __slog2_la_CoutTab)[195]);

                y = x;
                (((_iml_v2_sp_union_t *) & y)->hex[0] = (((_iml_v2_sp_union_t *) & y)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (0x7F) & 0xFF) << 23));

                fTmp = (y + ((__constant float *) __slog2_la_CoutTab)[197]);
                j = (((_iml_v2_sp_union_t *) & fTmp)->hex[0] & 0x007FFFFF) & ((1 << (6 + 1)) - 1);

                fB = ((__constant float *) __slog2_la_CoutTab)[3 * (j)];
                fLgRcprYHi = ((__constant float *) __slog2_la_CoutTab)[3 * (j) + 1];
                fLgRcprYLo = ((__constant float *) __slog2_la_CoutTab)[3 * (j) + 2];

                fWHi = (fNLg2Hi + fLgRcprYHi);

                fTmp = (y + ((__constant float *) __slog2_la_CoutTab)[198]);
                fYHi = (fTmp - ((__constant float *) __slog2_la_CoutTab)[198]);
                fYLo = (y - fYHi);

                fQHi = (fB * fYHi - ((__constant float *) __slog2_la_CoutTab)[203]);
                fQLo = (fB * fYLo);
                q = (fQHi + fQLo);

                fP = (((((((((__constant float *) __slog2_la_CoutTab)[211] * q + ((__constant float *) __slog2_la_CoutTab)[210]) * q +
                           ((__constant float *) __slog2_la_CoutTab)[209]) * q + ((__constant float *) __slog2_la_CoutTab)[208]) * q +
                         ((__constant float *) __slog2_la_CoutTab)[207]) * q + ((__constant float *) __slog2_la_CoutTab)[206]) * q +
                       ((__constant float *) __slog2_la_CoutTab)[205]) * q + ((__constant float *) __slog2_la_CoutTab)[204]);

                fResHi = (fWHi + fQHi);
                fResLo = (fLgRcprYLo + fP * fQLo);
                fResLo = (fResLo + fQLo);
                fResLo = (fResLo + fP * fQHi);

                (*r) = (fResHi + fResLo);
            }
            else
            {

                q = (u * ((__constant float *) __slog2_la_CoutTab)[203]);

                fP = (((((((((__constant float *) __slog2_la_CoutTab)[211] * q + ((__constant float *) __slog2_la_CoutTab)[210]) * q +
                           ((__constant float *) __slog2_la_CoutTab)[209]) * q + ((__constant float *) __slog2_la_CoutTab)[208]) * q +
                         ((__constant float *) __slog2_la_CoutTab)[207]) * q + ((__constant float *) __slog2_la_CoutTab)[206]) * q +
                       ((__constant float *) __slog2_la_CoutTab)[205]) * q + ((__constant float *) __slog2_la_CoutTab)[204]);

                fP = (fP * q);
                fP = (fP + q);

                (*r) = (float) fP;
            }
        }
        else
        {

            if (x == ((__constant float *) __slog2_la_CoutTab)[201])
            {

                (*r) = (float) (-((__constant float *) __slog2_la_CoutTab)[202] / ((__constant float *) __slog2_la_CoutTab)[201]);
                nRet = 2;
            }
            else
            {

                (*r) = (float) (((__constant float *) __slog2_la_CoutTab)[201] / ((__constant float *) __slog2_la_CoutTab)[201]);
                nRet = 1;
            }
        }
    }
    else
    {

        if (((((_iml_v2_sp_union_t *) & (*a))->hex[0] >> 31) == 1) && ((((_iml_v2_sp_union_t *) & (*a))->hex[0] & 0x007FFFFF) == 0))
        {

            (*r) = (float) (((__constant float *) __slog2_la_CoutTab)[201] / ((__constant float *) __slog2_la_CoutTab)[201]);
            nRet = 1;
        }
        else
        {

            (*r) = (*a) * (*a);
        }
    }

    return nRet;
}

float __ocl_svml_log2f (float a)
{

    float va1;
    float vr1;
    unsigned int vm;

    float r;

    va1 = a;;

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

        iBrkValue = (__internal_slog2_la_data.iBrkValue);
        iOffExpoMask = (__internal_slog2_la_data.iOffExpoMask);
        iX = as_uint (va1);
        iX = (iX - iBrkValue);
        iR = (iX & iOffExpoMask);
        iN = ((signed int) iX >> (23));
        iR = (iR + iBrkValue);
        FpExpon = ((float) ((int) (iN)));
        sR = as_float (iR);

        MinNorm = as_float (__internal_slog2_la_data.MinNorm);
        MaxNorm = as_float (__internal_slog2_la_data.MaxNorm);
        BrMask1 = as_float (((unsigned int) (-(signed int) (va1 < MinNorm))));
        BrMask2 = as_float (((unsigned int) (-(signed int) (!(va1 <= MaxNorm)))));
        BrMask1 = as_float ((as_uint (BrMask1) | as_uint (BrMask2)));
        BrMask = as_uint (BrMask1);

        vm = 0;
        vm = BrMask;

        One = as_float (__internal_slog2_la_data.One);
        sR = (sR - One);

        sPoly[8] = as_float (__internal_slog2_la_data.sPoly[0]);
        sPoly[7] = as_float (__internal_slog2_la_data.sPoly[1]);
        sPoly[6] = as_float (__internal_slog2_la_data.sPoly[2]);
        sPoly[5] = as_float (__internal_slog2_la_data.sPoly[3]);
        sP78 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[8], sR, sPoly[7]);
        sP56 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[6], sR, sPoly[5]);
        sR2 = (sR * sR);
        sPoly[4] = as_float (__internal_slog2_la_data.sPoly[4]);
        sPoly[3] = as_float (__internal_slog2_la_data.sPoly[5]);
        sP34 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[4], sR, sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP78, sR2, sP56);
        sPoly[2] = as_float (__internal_slog2_la_data.sPoly[6]);
        sPoly[1] = as_float (__internal_slog2_la_data.sPoly[7]);
        sP12 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[2], sR, sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sP34);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sP12);
        sPoly[0] = as_float (__internal_slog2_la_data.sPoly[8]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);

        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, FpExpon);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_slog2_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}
