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
    unsigned int L2;
} __internal_slog10_la_data_avx512_t;
static __constant __internal_slog10_la_data_avx512_t __internal_slog10_la_data_avx512 = {

    0x3f800000u, {
                  0xbdc9ae9bu, 0xbda6fcf4u,
                  0xbd8bac76u, 0xbd6bca30u,
                  0xbd48a99bu, 0xbd2c0a9fu,
                  0xbd1480dbu, 0xbd00faf2u,
                  0xbe823aa9u, 0xbe656348u,
                  0xbe4afbb9u, 0xbe346895u,
                  0xbe20ffffu, 0xbe103a0bu,
                  0xbe01a91cu, 0xbde9e84eu,
                  }

    , {
       0x3e13d888u, 0x3e10a87cu,
       0x3e0b95c3u, 0x3e057f0bu,
       0x3dfde038u, 0x3df080d9u,
       0x3de34c1eu, 0x3dd68333u,
       0x3dac6e8eu, 0x3dd54a51u,
       0x3df30f40u, 0x3e04235du,
       0x3e0b7033u, 0x3e102c90u,
       0x3e12ebadu, 0x3e141ff8u,
       }

    , {
       0xbe5e5a9bu, 0xbe5e2677u,
       0xbe5d83f5u, 0xbe5c6016u,
       0xbe5abd0bu, 0xbe58a6fdu,
       0xbe562e02u, 0xbe5362f8u,
       0xbe68e27cu, 0xbe646747u,
       0xbe619a73u, 0xbe5ff05au,
       0xbe5f0570u, 0xbe5e92d0u,
       0xbe5e662bu, 0xbe5e5c08u,
       }

    , {
       0x3ede5bd8u, 0x3ede5b45u,
       0x3ede57d8u, 0x3ede4eb1u,
       0x3ede3d37u, 0x3ede2166u,
       0x3eddf9d9u, 0x3eddc5bbu,
       0x3ede08edu, 0x3ede32e7u,
       0x3ede4967u, 0x3ede5490u,
       0x3ede597fu, 0x3ede5b50u,
       0x3ede5bcau, 0x3ede5bd9u,
       }
    , 0x3e9a209bu
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
    unsigned int L2H;
    unsigned int L2L;
    unsigned int iBrkValue;
    unsigned int iOffExpoMask;
    unsigned int One;
    unsigned int sPoly[9];
    unsigned int ep_Poly[5];
    unsigned int L2;

    unsigned int sInfs[2];
    unsigned int sOnes[2];
    unsigned int sZeros[2];
} __internal_slog10_la_data_t;
static __constant __internal_slog10_la_data_t __internal_slog10_la_data = {

    {
     0xc217b87cu, 0x39c7ca28u, 0xc217ba7eu, 0x39cc701au, 0xc217bc7eu, 0x39d1aa37u, 0xc217be6eu, 0x39bb6e0du, 0xc217c06eu, 0x39c9b14eu, 0xc217c25eu,
     0x39bc69d0u, 0xc217c45eu, 0x39d38d89u, 0xc217c64eu, 0x39cf1294u, 0xc217c83eu, 0x39ceef2bu, 0xc217ca2eu, 0x39d319a8u, 0xc217cc0eu, 0x39bb8888u,
     0xc217cdfeu, 0x39c83263u, 0xc217cfeeu, 0x39d90df3u, 0xc217d1ceu, 0x39ce1211u, 0xc217d3aeu, 0x39c735b1u, 0xc217d58eu, 0x39c46fe6u, 0xc217d76eu,
     0x39c5b7e1u, 0xc217d94eu, 0x39cb04edu, 0xc217db2eu, 0x39d44e73u, 0xc217dcfeu, 0x39c18bf6u, 0xc217dedeu, 0x39d2b514u, 0xc217e0aeu, 0x39c7c187u,
     0xc217e27eu, 0x39c0a922u, 0xc217e44eu, 0x39bd63d1u, 0xc217e61eu, 0x39bde99au, 0xc217e7eeu, 0x39c2329du, 0xc217e9beu, 0x39ca3711u, 0xc217eb8eu,
     0x39d5ef45u, 0xc217ed4eu, 0x39c553a1u, 0xc217ef1eu, 0x39d85ca2u, 0xc217f0deu, 0x39cf02dfu, 0xc217f29eu, 0x39c93f03u, 0xc217f45eu, 0x39c709cfu,
     0xc217f61eu, 0x39c85c1du, 0xc217f7deu, 0x39cd2edau, 0xc217fb4eu, 0x39c139bdu, 0xc217febeu, 0x39c2f387u, 0xc218022eu, 0x39d22688u, 0xc218058eu,
     0x39ce9e4bu, 0xc21808eeu, 0x39d82789u, 0xc2180c3eu, 0x39ce9024u, 0xc2180f8eu, 0x39d1a71bu, 0xc21812ceu, 0x39c13c84u, 0xc218160eu, 0x39bd2180u,
     0xc218194eu, 0x39c52839u, 0xc2181c8eu, 0x39d923d6u, 0xc2181fbeu, 0x39d8e873u, 0xc21822deu, 0x39c44b1fu, 0xc21825feu, 0x39bb21d0u, 0xc218291eu,
     0x39bd435fu, 0xc2182c3eu, 0x39ca8781u, 0xc2182f4eu, 0x39c2c6c3u, 0xc218325eu, 0x39c5da80u, 0xc218356eu, 0x39d39cdfu, 0xc218386eu, 0x39cbe8cau,
     0xc2183b6eu, 0x39ce99eeu, 0xc2183e5eu, 0x39bb8cafu, 0xc218415eu, 0x39d29e2au, 0xc218444eu, 0x39d3ac2bu, 0xc218472eu, 0x39be952bu, 0xc2184a1eu,
     0x39d3384bu, 0xc2184cfeu, 0x39d1754du, 0xc2184fdeu, 0x39d92c95u, 0xc21852aeu, 0x39ca3f22u, 0xc218557eu, 0x39c48e88u, 0xc218584eu, 0x39c7fcf1u,
     0xc2185b1eu, 0x39d46d15u, 0xc2185ddeu, 0x39c9c239u, 0xc218609eu, 0x39c7e029u, 0xc218635eu, 0x39ceab39u, 0xc218660eu, 0x39be083cu, 0xc21868ceu,
     0x39d5dc87u, 0xc2186b7eu, 0x39d60de7u, 0xc2186e1eu, 0x39be82a6u, 0xc21870ceu, 0x39cf2181u, 0xc218736eu, 0x39c7d1a9u, 0xc218760eu, 0x39c87abfu,
     0xc21878aeu, 0x39d104d3u, 0xc2187b3eu, 0x39c1585fu, 0xc2187ddeu, 0x39d95e46u, 0xc218806eu, 0x39d8ffd1u, 0xc21882eeu, 0x39c026afu, 0xc218857eu,
     0x39cebcedu, 0xc21887feu, 0x39c4acfbu, 0xc2188a7eu, 0x39c1e1a4u, 0xc2188cfeu, 0x39c6460fu, 0xc2188f7eu, 0x39d1c5bdu, 0xc21891eeu, 0x39c44c86u,
     0xc218945eu, 0x39bdc695u, 0xc21896ceu, 0x39be206cu, 0xc218993eu, 0x39c546dbu, 0xc2189baeu, 0x39d32706u, 0xc2189e0eu, 0x39c7ae5bu, 0xc218a06eu,
     0x39c2ca97u, 0xc218a2ceu, 0x39c469c1u, 0xc218a52eu, 0x39cc7a29u, 0xc218a77eu, 0x39baea67u, 0xc218a9deu, 0x39cfa95au, 0xc218ac2eu, 0x39caa624u,
     0xc218ae7eu, 0x39cbd02bu, 0xc218b0ceu, 0x39d31717u, 0xc218b30eu, 0x39c06ad1u, 0xc218b55eu, 0x39d3bb81u, 0xc218b79eu, 0x39ccf98cu, 0xc218b9deu,
     0x39cc1595u, 0xc218bc1eu, 0x39d10079u, 0xc218be4eu, 0x39bbab51u, 0xc218c08eu, 0x39cc076eu, 0xc218c2beu, 0x39c20659u, 0xc218c4eeu, 0x39bd99d1u,
     0xc218c71eu, 0x39beb3ceu, 0xc218c94eu, 0x39c54678u, 0xc218cb7eu, 0x39d1442fu, 0xc218cd9eu, 0x39c29f83u, 0xc218cfceu, 0x39d94b37u, 0xc218d1eeu,
     0x39d53a3fu, 0xc218d40eu, 0x39d65fbdu, 0xc218d61eu, 0x39bcaf04u, 0xc218d83eu, 0x39c81b93u, 0xc218da5eu, 0x39d8991au, 0xc218dc6eu, 0x39ce1b70u,
     0xc218de7eu, 0x39c8969bu, 0xc218e08eu, 0x39c7fecbu, 0xc218e29eu, 0x39cc485bu, 0xc218e4aeu, 0x39d567ccu, 0xc218e6aeu, 0x39c351cau, 0xc218e8beu,
     0x39d5fb29u, 0xc218eabeu, 0x39cd58e1u, 0xc218ecbeu, 0x39c96014u}

    , {
       0x3fE35103u, 0xbf93D7E4u, 0x3aD3D366u}

    , 0x007fffffu, 0x3b000000u, 0x00800000u, 0x7f7fffffu, 0xffffff00u, 0x3ede0000u, 0x3e9a2100u, 0xb64AF600u, 0x3f2aaaabu, 0x007fffffu, 0x3f800000u, {
                                                                                                                                                      0x3d8063B4u,
                                                                                                                                                      0xbd890073u,
                                                                                                                                                      0x3d775317u,
                                                                                                                                                      0xbd91FB27u,
                                                                                                                                                      0x3dB20B96u,
                                                                                                                                                      0xbdDE6E20u,
                                                                                                                                                      0x3e143CE5u,
                                                                                                                                                      0xbe5E5BC5u,
                                                                                                                                                      0x3eDE5BD9u}

    , {
       0x3dCDC127u, 0xbdF4ED71u, 0x3e137DBEu, 0xbe5E0922u, 0x3eDE5CAEu}

    , 0x3e9a209bu, {0x7f800000u, 0xff800000u}

    , {0x3f800000u, 0xbf800000u}

    , {0x00000000u, 0x80000000u}

};

static __constant _iml_v2_sp_union_t __slog10_la_CoutTab[212] = {
    0x42DE5C00,
    0x00000000,
    0x00000000,
    0x42DAE290,
    0x3BE00000,
    0x366A02B9,
    0x42D76920,
    0x3C620000,
    0xB6BDF222,
    0x42D3EFB0,
    0x3CAB0000,
    0xB7C85B7F,
    0x42D07640,
    0x3CE58000,
    0x3763F336,
    0x42CEB988,
    0x3D01C000,
    0xB76EDF0D,
    0x42CB4018,
    0x3D1FE000,
    0xB7041CCE,
    0x42C7C6A8,
    0x3D3E8000,
    0x366028BF,
    0x42C609F0,
    0x3D4E0000,
    0x374BD5F8,
    0x42C29080,
    0x3D6D8000,
    0x370F6BB3,
    0x42C0D3C8,
    0x3D7D8000,
    0xB63258D0,
    0x42BD5A58,
    0x3D8EE800,
    0x36A8C191,
    0x42BB9DA0,
    0x3D972000,
    0xB6B17E91,
    0x42B82430,
    0x3DA7C000,
    0xB6AA4C88,
    0x42B66778,
    0x3DB02800,
    0x36E6BDD3,
    0x42B4AAC0,
    0x3DB8B000,
    0xB63CC726,
    0x42B13150,
    0x3DC9F000,
    0x36FBC1F8,
    0x42AF7498,
    0x3DD2B800,
    0xB5BE6D05,
    0x42ADB7E0,
    0x3DDB9000,
    0x35E68B8B,
    0x42ABFB28,
    0x3DE48000,
    0x36286799,
    0x42AA3E70,
    0x3DED8800,
    0x35F6BB35,
    0x42A6C500,
    0x3DFFE000,
    0xB47510E2,
    0x42A50848,
    0x3E049800,
    0x34968666,
    0x42A34B90,
    0x3E094C00,
    0x36527D9F,
    0x42A18ED8,
    0x3E0E0E00,
    0x35E9955C,
    0x429FD220,
    0x3E12DE00,
    0xB63EF528,
    0x429E1568,
    0x3E17BA00,
    0xB5FE30B1,
    0x429C58B0,
    0x3E1CA400,
    0xB5FF086B,
    0x429A9BF8,
    0x3E219C00,
    0xB5E321F9,
    0x4298DF40,
    0x3E26A200,
    0xB37B14B7,
    0x42972288,
    0x3E2BB800,
    0xB6627A11,
    0x429565D0,
    0x3E30DC00,
    0xB636333E,
    0x4293A918,
    0x3E360E00,
    0x365AFCDE,
    0x4291EC60,
    0x3E3B5200,
    0x353976FD,
    0x4291EC60,
    0x3E3B5200,
    0x353976FD,
    0x42902FA8,
    0x3E40A600,
    0xB5B6DE43,
    0x428E72F0,
    0x3E460A00,
    0xB5BC896B,
    0x428CB638,
    0x3E4B7E00,
    0x360DB1D0,
    0x428AF980,
    0x3E510400,
    0x365427DE,
    0x428AF980,
    0x3E510400,
    0x365427DE,
    0x42893CC8,
    0x3E569C00,
    0x3664E163,
    0x42878010,
    0x3E5C4800,
    0xB64C1834,
    0x4285C358,
    0x3E620400,
    0x356709EA,
    0x4285C358,
    0x3E620400,
    0x356709EA,
    0x428406A0,
    0x3E67D400,
    0x35E765CC,
    0x428249E8,
    0x3E6DB800,
    0x35C7E96E,
    0x42808D30,
    0x3E73B000,
    0x36120236,
    0x42808D30,
    0x3E73B000,
    0x36120236,
    0x427DA0F0,
    0x3E79BE00,
    0xB5EA454F,
    0x427A2780,
    0x3E7FE000,
    0xB4F510E2,
    0x427A2780,
    0x3E7FE000,
    0xB4F510E2,
    0x4276AE10,
    0x3E830C00,
    0x353A13F5,
    0x4276AE10,
    0x3E830C00,
    0x353A13F5,
    0x427334A0,
    0x3E863380,
    0x348EB55F,
    0x426FBB30,
    0x3E896680,
    0x3546E726,
    0x426FBB30,
    0x3E896680,
    0x3546E726,
    0x426C41C0,
    0x3E8CA580,
    0x356F7BDB,
    0x426C41C0,
    0x3E8CA580,
    0x356F7BDB,
    0x4268C850,
    0x3E8FF100,
    0xB4D2869F,
    0x4268C850,
    0x3E8FF100,
    0xB4D2869F,
    0x42654EE0,
    0x3E934900,
    0xB499EB08,
    0x42654EE0,
    0x3E934900,
    0xB499EB08,
    0x4261D570,
    0x3E96AE00,
    0x34BB05C3,
    0x4261D570,
    0x3E96AE00,
    0x34BB05C3,
    0x425E5C00,
    0x3E9A2080,
    0x355427DE,

    0x3E9A0000,
    0x39826A14,

    0x48000040,

    0x46000000,

    0x3BC00000,

    0x53800000,

    0x00000000,
    0x3F800000,
    0x42DE5C00,

    0xBF7F0000,
    0xB7935D5A,
    0x33E23666,
    0xB04353B9,
    0x2CB3E701,
    0xA92C998B,
    0x25AA5BFD,
    0xA22B5DAE,
};

__attribute__((always_inline))
inline int __internal_slog10_la_cout (float *a, float *r)
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

        if (((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) == 0)
        {

            x *= ((__constant float *) __slog10_la_CoutTab)[200];
            iN -= 40;
        }

        if (x > ((__constant float *) __slog10_la_CoutTab)[201])
        {

            u = (x - 1.0f);
            fAbsU = u;
            (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] = (((_iml_v2_sp_union_t *) & fAbsU)->hex[0] & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            if (fAbsU > ((__constant float *) __slog10_la_CoutTab)[199])
            {

                iN += ((((_iml_v2_sp_union_t *) & x)->hex[0] >> 23) & 0xFF) - 0x7F;
                fN = (float) iN;

                fNLg2Hi = (fN * ((__constant float *) __slog10_la_CoutTab)[195]);
                fNLg2Lo = (fN * ((__constant float *) __slog10_la_CoutTab)[196]);

                y = x;
                (((_iml_v2_sp_union_t *) & y)->hex[0] = (((_iml_v2_sp_union_t *) & y)->hex[0] & 0x807FFFFF) | (((_iml_uint32_t) (0x7F) & 0xFF) << 23));

                fTmp = (y + ((__constant float *) __slog10_la_CoutTab)[197]);
                j = (((_iml_v2_sp_union_t *) & fTmp)->hex[0] & 0x007FFFFF) & ((1 << (6 + 1)) - 1);

                fB = ((__constant float *) __slog10_la_CoutTab)[3 * (j)];
                fLgRcprYHi = ((__constant float *) __slog10_la_CoutTab)[3 * (j) + 1];
                fLgRcprYLo = ((__constant float *) __slog10_la_CoutTab)[3 * (j) + 2];

                fWHi = (fNLg2Hi + fLgRcprYHi);
                fWLo = (fNLg2Lo + fLgRcprYLo);

                fTmp = (y + ((__constant float *) __slog10_la_CoutTab)[198]);
                fYHi = (fTmp - ((__constant float *) __slog10_la_CoutTab)[198]);
                fYLo = (y - fYHi);

                fQHi = (fB * fYHi - ((__constant float *) __slog10_la_CoutTab)[203]);
                fQLo = (fB * fYLo);
                q = (fQHi + fQLo);

                fP = (((((((((__constant float *) __slog10_la_CoutTab)[211] * q + ((__constant float *) __slog10_la_CoutTab)[210]) * q +
                           ((__constant float *) __slog10_la_CoutTab)[209]) * q + ((__constant float *) __slog10_la_CoutTab)[208]) * q +
                         ((__constant float *) __slog10_la_CoutTab)[207]) * q + ((__constant float *) __slog10_la_CoutTab)[206]) * q +
                       ((__constant float *) __slog10_la_CoutTab)[205]) * q + ((__constant float *) __slog10_la_CoutTab)[204]);

                fResHi = (fWHi + fQHi);
                fResLo = (fWLo + fP * fQLo);
                fResLo = (fResLo + fQLo);
                fResLo = (fResLo + fP * fQHi);

                (*r) = (fResHi + fResLo);
            }
            else
            {

                q = (u * ((__constant float *) __slog10_la_CoutTab)[203]);

                fP = (((((((((__constant float *) __slog10_la_CoutTab)[211] * q + ((__constant float *) __slog10_la_CoutTab)[210]) * q +
                           ((__constant float *) __slog10_la_CoutTab)[209]) * q + ((__constant float *) __slog10_la_CoutTab)[208]) * q +
                         ((__constant float *) __slog10_la_CoutTab)[207]) * q + ((__constant float *) __slog10_la_CoutTab)[206]) * q +
                       ((__constant float *) __slog10_la_CoutTab)[205]) * q + ((__constant float *) __slog10_la_CoutTab)[204]);

                fP = (fP * q);
                fP = (fP + q);

                (*r) = (float) fP;
            }
        }
        else
        {

            if (x == ((__constant float *) __slog10_la_CoutTab)[201])
            {

                (*r) = (float) (-((__constant float *) __slog10_la_CoutTab)[202] / ((__constant float *) __slog10_la_CoutTab)[201]);
                nRet = 2;
            }
            else
            {

                (*r) = (float) (((__constant float *) __slog10_la_CoutTab)[201] / ((__constant float *) __slog10_la_CoutTab)[201]);
                nRet = 1;
            }
        }
    }
    else
    {

        if (((((_iml_v2_sp_union_t *) & (*a))->hex[0] >> 31) == 1) && ((((_iml_v2_sp_union_t *) & (*a))->hex[0] & 0x007FFFFF) == 0))
        {

            (*r) = (float) (((__constant float *) __slog10_la_CoutTab)[201] / ((__constant float *) __slog10_la_CoutTab)[201]);
            nRet = 1;
        }
        else
        {

            (*r) = (*a) * (*a);
        }
    }

    return nRet;
}

float __ocl_svml_log10f (float a)
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

        float L2;

        iBrkValue = (__internal_slog10_la_data.iBrkValue);
        iOffExpoMask = (__internal_slog10_la_data.iOffExpoMask);
        iX = as_uint (va1);
        iX = (iX - iBrkValue);
        iR = (iX & iOffExpoMask);
        iN = ((signed int) iX >> (23));
        iR = (iR + iBrkValue);
        FpExpon = ((float) ((int) (iN)));
        sR = as_float (iR);

        MinNorm = as_float (__internal_slog10_la_data.MinNorm);
        MaxNorm = as_float (__internal_slog10_la_data.MaxNorm);
        BrMask1 = as_float (((unsigned int) (-(signed int) (va1 < MinNorm))));
        BrMask2 = as_float (((unsigned int) (-(signed int) (!(va1 <= MaxNorm)))));
        BrMask1 = as_float ((as_uint (BrMask1) | as_uint (BrMask2)));
        BrMask = as_uint (BrMask1);

        vm = 0;
        vm = BrMask;

        One = as_float (__internal_slog10_la_data.One);
        sR = (sR - One);

        sPoly[8] = as_float (__internal_slog10_la_data.sPoly[0]);
        sPoly[7] = as_float (__internal_slog10_la_data.sPoly[1]);
        sPoly[6] = as_float (__internal_slog10_la_data.sPoly[2]);
        sPoly[5] = as_float (__internal_slog10_la_data.sPoly[3]);
        sP78 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[8], sR, sPoly[7]);
        sP56 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[6], sR, sPoly[5]);
        sR2 = (sR * sR);
        sPoly[4] = as_float (__internal_slog10_la_data.sPoly[4]);
        sPoly[3] = as_float (__internal_slog10_la_data.sPoly[5]);
        sP34 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[4], sR, sPoly[3]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP78, sR2, sP56);
        sPoly[2] = as_float (__internal_slog10_la_data.sPoly[6]);
        sPoly[1] = as_float (__internal_slog10_la_data.sPoly[7]);
        sP12 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sPoly[2], sR, sPoly[1]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sP34);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR2, sP12);
        sPoly[0] = as_float (__internal_slog10_la_data.sPoly[8]);
        sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, sPoly[0]);

        L2H = as_float (__internal_slog10_la_data.L2H);
        L2L = as_float (__internal_slog10_la_data.L2L);
        Kl = (FpExpon * L2L);

        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (sP, sR, Kl);
        vr1 = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, ) (FpExpon, L2H, vr1);
    }

    if ((vm) != 0)
    {
        float _vapi_arg1[1];
        float _vapi_res1[1];
        ((float *) _vapi_arg1)[0] = va1;
        ((float *) _vapi_res1)[0] = vr1;
        __internal_slog10_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((float *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}
