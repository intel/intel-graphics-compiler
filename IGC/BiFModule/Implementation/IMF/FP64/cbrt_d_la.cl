/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long etbl_H[8];
    unsigned long etbl_L[8];
    unsigned long cbrt_tbl_H[16];
    unsigned long cbrt_tbl_L[16];
    unsigned long BiasL;
    unsigned long SZero;
    unsigned long OneThird;
    unsigned long Bias3;
    unsigned long Three;
    unsigned long One;
    unsigned long poly_coeff10;
    unsigned long poly_coeff9;
    unsigned long poly_coeff8;
    unsigned long poly_coeff7;
    unsigned long poly_coeff6;
    unsigned long poly_coeff5;
    unsigned long poly_coeff4;
    unsigned long poly_coeff3;
    unsigned long poly_coeff2;
    unsigned long poly_coeff1;
} __internal_dcbrt_la_data_avx512_t;
static __constant __internal_dcbrt_la_data_avx512_t __internal_dcbrt_la_data_avx512 = {
    {
     0x3ff0000000000000uL,
     0x3ff428a2f98d728buL,
     0x3ff965fea53d6e3duL,
     0x0000000000000000uL,
     0xbff0000000000000uL,
     0xbff428a2f98d728buL,
     0xbff965fea53d6e3duL,
     0x0000000000000000uL,
     }
    , {
       0x0000000000000000uL,
       0xbc7ddc22548ea41euL,
       0xbc9f53e999952f09uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x3c7ddc22548ea41euL,
       0x3c9f53e999952f09uL,
       0x0000000000000000uL,
       }
    , {
       0x3ff428a2f98d728buL,
       0x3ff361f35ca116ffuL,
       0x3ff2b6b5edf6b54auL,
       0x3ff220e6dd675180uL,
       0x3ff19c3b38e975a8uL,
       0x3ff12589c21fb842uL,
       0x3ff0ba6ee5f9aad4uL,
       0x3ff059123d3a9848uL,
       0x3ff0000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       }
    , {
       0xbc7ddc22548ea41euL,
       0x3c934f1f2588cb24uL,
       0xbc9623da69e513d4uL,
       0x3c930b0a26a8bb5cuL,
       0xbc76b70b4d3bd257uL,
       0xbc9e13c8505a4a7auL,
       0x3c8dcc718f7857e5uL,
       0x3c770e4a1da627b9uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       0x0000000000000000uL,
       }

    , 0x4338000000000000uL, 0x8000000000000000uL, 0x3fd5555555555556uL, 0x4320000000000000uL, 0x4008000000000000uL, 0x3ff0000000000000uL,
        0xbf882e3b6adeca62uL, 0x3f8bda24bae48875uL, 0xbf9036b87c71d55fuL, 0x3f9374ed9398b914uL, 0xbf98090d77f2468euL, 0x3f9ee71141dcf569uL,
        0xbfa511e8d2b0363euL, 0x3faf9add3c0b7e31uL, 0xbfbc71c71c71c741uL, 0x3fd5555555555557uL
};

typedef struct
{
    unsigned long _dRcp[32];

    unsigned long _dCbrtHiLo[96];

    unsigned long _dA8;
    unsigned long _dA7;
    unsigned long _dA6;
    unsigned long _dA5;
    unsigned long _dA4;
    unsigned long _dA3;
    unsigned long _dA2;
    unsigned long _dA1;

    unsigned long _dNeg65Div64;
    unsigned long _dSgnf6Mask;
    unsigned long _dNegOne;
    unsigned long _dMantissaMask;
    unsigned long _lExpHiMask;
    unsigned long _lExpLoMask;
    unsigned long _l1556;

    unsigned int _iRcpIndexMask;
    unsigned int _iAbsMask;
    unsigned int _iSignMask;
    unsigned int _iBias;
    unsigned int _iSub;
    unsigned int _iCmp;

} __internal_dcbrt_la_data_t;
static __constant __internal_dcbrt_la_data_t __internal_dcbrt_la_data = {
    {
     0xBFEF81F81F81F820uL,
     0xBFEE9131ABF0B767uL,
     0xBFEDAE6076B981DBuL,
     0xBFECD85689039B0BuL,
     0xBFEC0E070381C0E0uL,
     0xBFEB4E81B4E81B4FuL,
     0xBFEA98EF606A63BEuL,
     0xBFE9EC8E951033D9uL,
     0xBFE948B0FCD6E9E0uL,
     0xBFE8ACB90F6BF3AAuL,
     0xBFE8181818181818uL,
     0xBFE78A4C8178A4C8uL,
     0xBFE702E05C0B8170uL,
     0xBFE6816816816817uL,
     0xBFE6058160581606uL,
     0xBFE58ED2308158EDuL,
     0xBFE51D07EAE2F815uL,
     0xBFE4AFD6A052BF5BuL,
     0xBFE446F86562D9FBuL,
     0xBFE3E22CBCE4A902uL,
     0xBFE3813813813814uL,
     0xBFE323E34A2B10BFuL,
     0xBFE2C9FB4D812CA0uL,
     0xBFE27350B8812735uL,
     0xBFE21FB78121FB78uL,
     0xBFE1CF06ADA2811DuL,
     0xBFE1811811811812uL,
     0xBFE135C81135C811uL,
     0xBFE0ECF56BE69C90uL,
     0xBFE0A6810A6810A7uL,
     0xBFE0624DD2F1A9FCuL,
     0xBFE0204081020408uL,
     },
    {
     0x3FF01539221D4C97uL,
     0x3FF03F06771A2E33uL,
     0x3FF06800E629D671uL,
     0x3FF090328731DEB2uL,
     0x3FF0B7A4B1BD64ACuL,
     0x3FF0DE601024FB87uL,
     0x3FF1046CB0597000uL,
     0x3FF129D212A9BA9BuL,
     0x3FF14E9736CDAF38uL,
     0x3FF172C2A772F507uL,
     0x3FF1965A848001D3uL,
     0x3FF1B9648C38C55DuL,
     0x3FF1DBE6236A0C45uL,
     0x3FF1FDE45CBB1F9FuL,
     0x3FF21F63FF409042uL,
     0x3FF240698C6746E5uL,
     0x3FF260F9454BB99BuL,
     0x3FF281172F8E7073uL,
     0x3FF2A0C719B4B6D0uL,
     0x3FF2C00C9F2263ECuL,
     0x3FF2DEEB2BB7FB78uL,
     0x3FF2FD65FF1EFBBCuL,
     0x3FF31B802FCCF6A2uL,
     0x3FF3393CADC50708uL,
     0x3FF3569E451E4C2AuL,
     0x3FF373A7A0554CDEuL,
     0x3FF3905B4A6D76CEuL,
     0x3FF3ACBBB0E756B6uL,
     0x3FF3C8CB258FA340uL,
     0x3FF3E48BE02AC0CEuL,
     0x3FF4000000000000uL,
     0x3FF41B298D47800EuL,
     0x3FF443604B34D9B2uL,
     0x3FF4780B20906571uL,
     0x3FF4ABAC3EE06706uL,
     0x3FF4DE505DA66B8DuL,
     0x3FF51003420A5C07uL,
     0x3FF540CFD6FD11C1uL,
     0x3FF570C04260716BuL,
     0x3FF59FDDF7A45F38uL,
     0x3FF5CE31C83539DFuL,
     0x3FF5FBC3F20966A4uL,
     0x3FF6289C2C8F1B70uL,
     0x3FF654C1B4316DCFuL,
     0x3FF6803B54A34E44uL,
     0x3FF6AB0F72182659uL,
     0x3FF6D544118C08BCuL,
     0x3FF6FEDEE0388D4AuL,
     0x3FF727E53A4F645EuL,
     0x3FF7505C31104114uL,
     0x3FF77848904CD549uL,
     0x3FF79FAEE36B2534uL,
     0x3FF7C69379F4605BuL,
     0x3FF7ECFA6BBCA391uL,
     0x3FF812E79CAE7EB9uL,
     0x3FF8385EC043C71DuL,
     0x3FF85D635CB41B9DuL,
     0x3FF881F8CDE083DBuL,
     0x3FF8A6224802B8A8uL,
     0x3FF8C9E2DA25E5E4uL,
     0x3FF8ED3D706E1010uL,
     0x3FF91034D632B6DFuL,
     0x3FF932CBB7F0CF2DuL,
     0x3FF95504A517BF3AuL,
     0x3FF987AF34F8BB19uL,
     0x3FF9CA0A8337B317uL,
     0x3FFA0B1709CC13D5uL,
     0x3FFA4AE4CE6419EDuL,
     0x3FFA8982A5567031uL,
     0x3FFAC6FE500AB570uL,
     0x3FFB036497A15A17uL,
     0x3FFB3EC164671755uL,
     0x3FFB791FD288C46FuL,
     0x3FFBB28A44693BE4uL,
     0x3FFBEB0A72EB6E31uL,
     0x3FFC22A97BF5F697uL,
     0x3FFC596FEF6AF983uL,
     0x3FFC8F65DAC655A3uL,
     0x3FFCC492D38CE8D9uL,
     0x3FFCF8FE00B19367uL,
     0x3FFD2CAE230F8709uL,
     0x3FFD5FA99D15208FuL,
     0x3FFD91F679B6E505uL,
     0x3FFDC39A72BF2302uL,
     0x3FFDF49AF68C1570uL,
     0x3FFE24FD2D4C23B8uL,
     0x3FFE54C5FDC5EC73uL,
     0x3FFE83FA11B81DBBuL,
     0x3FFEB29DD9DBAF25uL,
     0x3FFEE0B59191D374uL,
     0x3FFF0E454245E4BFuL,
     0x3FFF3B50C68A9DD3uL,
     0x3FFF67DBCCF922DCuL,
     0x3FFF93E9DAD7A4A6uL,
     0x3FFFBF7E4E8CC9CBuL,
     0x3FFFEA9C61E47CD3uL,
     },

    0xBF9036DE5C9CC8E7uL,
    0x3F93750AD588F115uL,
    0xBF98090D6221A247uL,
    0x3F9EE7113506AC12uL,
    0xBFA511E8D2B3183BuL,
    0x3FAF9ADD3C0CA458uL,
    0xBFBC71C71C71C71CuL,
    0x3FD5555555555555uL,

    0xBFF0400000000000uL,
    0x000FC00000000000uL,
    0xBFF0000000000000uL,
    0x000FFFFFFFFFFFFFuL,
    0xFFF0000000000000uL,
    0x00000000000007FFuL,
    0x0000000000001556uL,

    0x000F8000u,
    0x7FFFFFFFu,
    0x00000800u,
    0x000002AAu,
    0x80100000u,
    0xffdfffffu,

};

static __constant _iml_v2_dp_union_t _vmldCbrtHATab_v2[57] = {
    0x00000000, 0x3FF00000,
    0x00000000, 0x3FEFA000,
    0x00000000, 0x3FEF4000,
    0x00000000, 0x3FEF0000,
    0x00000000, 0x3FEEC000,
    0x00000000, 0x3FEE6000,
    0x00000000, 0x3FEE2000,
    0x00000000, 0x3FEDE000,
    0x00000000, 0x3FEDA000,
    0x00000000, 0x3FED6000,
    0x00000000, 0x3FED2000,
    0x00000000, 0x3FECE000,
    0x00000000, 0x3FECC000,
    0x00000000, 0x3FEC8000,
    0x00000000, 0x3FEC4000,
    0x00000000, 0x3FEC2000,
    0x00000000, 0x3FEBE000,
    0x00000000, 0x3FEBC000,
    0x00000000, 0x3FEB8000,
    0x00000000, 0x3FEB6000,
    0x00000000, 0x3FEB2000,
    0x00000000, 0x3FEB0000,
    0x00000000, 0x3FEAE000,
    0x00000000, 0x3FEAA000,
    0x00000000, 0x3FEA8000,
    0x00000000, 0x3FEA6000,
    0x00000000, 0x3FEA2000,
    0x00000000, 0x3FEA0000,
    0x00000000, 0x3FE9E000,
    0x00000000, 0x3FE9C000,
    0x00000000, 0x3FE9A000,
    0x00000000, 0x3FE98000,

    0x55555555, 0x3FE55555,
    0x55555555, 0x3C855555,
    0x71C71C72, 0x3FE1C71C,
    0x3C0CA458, 0x3FDF9ADD,
    0x21B64151, 0x3FDCF8A0,
    0x0E65D690, 0x3FDB0A2F,
    0x3843BC6C, 0x3FD9899E,
    0x8AEB2D5A, 0x3FD8524D,
    0xA52160CC, 0x3FD74EDF,
    0xD7E740C4, 0x3FD671E0,
    0x593D6946, 0x3FD5B259,
    0x10B7ABE7, 0x3FD50A09,
    0x5E79AE3D, 0x3FD4746C,

    0x00000000, 0x52B00000,
    0x02000000, 0x41A00000,
    0x00000200, 0x42A00000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0xF8000000, 0x3FF428A2,
    0xAE223DDB, 0x3E38D728,
    0xA0000000, 0x3FF965FE,
    0xF20AC166, 0x3E54F5B8,
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0xBFF00000
};

__attribute__((always_inline))
inline int __internal_dcbrt_la_cout (double *a, double *r)
{
    double TwoPowN;
    double x;
    double e, eHi, eLo;
    double s, sHi, sLo, s2;
    double pHi, pLo, rHi, rLo;
    double y, yHi, yLo, ys2Hi, ys2Lo;
    double dSign, res;
    double t1, t2;
    double v1, v2, v3;

    int i;
    int jRoot, jRec;
    int M, N, unscale;

    int nRet = 0;

    if ((((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))
    {

        if ((*a) != ((__constant double *) _vmldCbrtHATab_v2)[54])
        {

            dSign = ((__constant double *) _vmldCbrtHATab_v2)[55 + ((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 31))];

            x = (*a);
            (((_iml_v2_dp_union_t *) & x)->dwords.hi_dword = (((_iml_v2_dp_union_t *) & x)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

            unscale = 0;

            if (((((_iml_v2_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) == 0)
            {

                x *= ((__constant double *) _vmldCbrtHATab_v2)[45];

                unscale = 100;
            }

            M = ((((_iml_v2_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF);

            jRoot = M % 3;
            N = (M - jRoot - 0x3FF) / 3;

            TwoPowN = ((__constant double *) _vmldCbrtHATab_v2)[55 + (0)];
            (((_iml_v2_dp_union_t *) & TwoPowN)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & TwoPowN)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (N + 0x3FF - unscale) & 0x7FF) << 20));

            y = x;
            (((_iml_v2_dp_union_t *) & y)->dwords.hi_dword =
             (((_iml_v2_dp_union_t *) & y)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));

            jRec = (((_iml_v2_dp_union_t *) & y)->dwords.hi_dword & 0x000FFFFF);
            jRec = jRec >> 15;

            s = ((__constant double *) _vmldCbrtHATab_v2)[jRec];
            s2 = s * s;

            v1 = ((y) * (((__constant double *) _vmldCbrtHATab_v2)[47]));
            v2 = (v1 - (y));
            v1 = (v1 - v2);
            v2 = ((y) - v1);
            yHi = v1;
            yLo = v2;;

            ys2Hi = yHi * s2;
            ys2Lo = yLo * s2;

            eHi = ((__constant double *) _vmldCbrtHATab_v2)[55 + (0)] - ys2Hi * s;
            eLo = -ys2Lo * s;

            v1 = ((eHi) * (((__constant double *) _vmldCbrtHATab_v2)[46]));
            v2 = (v1 - (eHi));
            v1 = (v1 - v2);
            v2 = ((eHi) - v1);
            eHi = v1;
            t1 = v2;;

            eLo += t1;

            e = eHi + eLo;

            pHi =
                ((((__constant double *) _vmldCbrtHATab_v2)[44] * e + ((__constant double *) _vmldCbrtHATab_v2)[43]) * e +
                 ((__constant double *) _vmldCbrtHATab_v2)[42]) * e + ((__constant double *) _vmldCbrtHATab_v2)[41];
            pHi =
                ((pHi * e + ((__constant double *) _vmldCbrtHATab_v2)[40]) * e + ((__constant double *) _vmldCbrtHATab_v2)[39]) * e +
                ((__constant double *) _vmldCbrtHATab_v2)[38];
            pHi =
                ((pHi * e + ((__constant double *) _vmldCbrtHATab_v2)[37]) * e + ((__constant double *) _vmldCbrtHATab_v2)[36]) * e +
                ((__constant double *) _vmldCbrtHATab_v2)[35];
            pHi = (pHi * e + ((__constant double *) _vmldCbrtHATab_v2)[34]) * e;

            v1 = ((((__constant double *) _vmldCbrtHATab_v2)[32]) + (pHi));
            v2 = ((((__constant double *) _vmldCbrtHATab_v2)[32]) - v1);
            v3 = (v1 + v2);
            v2 = ((pHi) + v2);
            v3 = ((((__constant double *) _vmldCbrtHATab_v2)[32]) - v3);
            v3 = (v2 + v3);
            pHi = v1;
            pLo = v3;;
            pLo += ((__constant double *) _vmldCbrtHATab_v2)[33];

            v1 = ((pHi) * (((__constant double *) _vmldCbrtHATab_v2)[46]));
            v2 = (v1 - (pHi));
            v1 = (v1 - v2);
            v2 = ((pHi) - v1);
            pHi = v1;
            t1 = v2;;
            pLo += t1;

            t1 = ((pHi) * (eHi));
            t2 = ((pLo) * (eLo));
            t2 = (t2 + (pHi) * (eLo));
            v1 = (t2 + (pLo) * (eHi));
            pHi = t1;
            pLo = v1;;

            v1 = ((pHi) * (((__constant double *) _vmldCbrtHATab_v2)[46]));
            v2 = (v1 - (pHi));
            v1 = (v1 - v2);
            v2 = ((pHi) - v1);
            pHi = v1;
            t1 = v2;;
            pLo += t1;

            t1 = ((pHi) * (ys2Hi));
            t2 = ((pLo) * (ys2Lo));
            t2 = (t2 + (pHi) * (ys2Lo));
            v1 = (t2 + (pLo) * (ys2Hi));
            pHi = t1;
            pLo = v1;;

            v1 = ((ys2Hi) + (pHi));
            v2 = ((ys2Hi) - v1);
            v3 = (v1 + v2);
            v2 = ((pHi) + v2);
            v3 = ((ys2Hi) - v3);
            v3 = (v2 + v3);
            rHi = v1;
            rLo = v3;;

            v1 = ((rHi) * (((__constant double *) _vmldCbrtHATab_v2)[46]));
            v2 = (v1 - (rHi));
            v1 = (v1 - v2);
            v2 = ((rHi) - v1);
            rHi = v1;
            t1 = v2;;
            rLo += t1;
            rLo += pLo;
            rLo += ys2Lo;

            t1 = ((rHi) * (((__constant double *) _vmldCbrtHATab_v2)[48 + (jRoot) * 2]));
            t2 = ((rLo) * (((__constant double *) _vmldCbrtHATab_v2)[49 + (jRoot) * 2]));
            t2 = (t2 + (rHi) * (((__constant double *) _vmldCbrtHATab_v2)[49 + (jRoot) * 2]));
            v1 = (t2 + (rLo) * (((__constant double *) _vmldCbrtHATab_v2)[48 + (jRoot) * 2]));
            rHi = t1;
            rLo = v1;;

            res = rHi + rLo;

            res *= TwoPowN;

            (*r) = dSign * res;
        }
        else
        {

            (*r) = ((*a) * (((__constant double *) _vmldCbrtHATab_v2)[55 + (0)]));
        }
    }
    else
    {

        (*r) = (*a) + (*a);
    }

    return nRet;

}

double __ocl_svml_cbrt (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    {

        unsigned long lX;
        unsigned int iX;
        unsigned int iRcpIndex;
        unsigned int iRcpIndex3;
        unsigned int iRangeMask;
        unsigned int iRcpIndexMask;
        unsigned int iAbsMask;
        unsigned int iSub;
        unsigned int iCmp;
        double dRcp;
        unsigned long lExpLoMask;
        unsigned long lExpHiMask;
        unsigned long lExp;
        unsigned long lExpo3;
        unsigned long l1556;
        unsigned int iExp;
        unsigned int iExpo3;
        unsigned int i2Expo3;
        unsigned int iCbrtIndex;
        double dCbrt[2];

        unsigned int iSign;
        unsigned int iSignMask;
        unsigned int i2k;
        unsigned int iBias;
        unsigned long l2k;
        double dMantissaMask;
        double dR;
        double dNegOne;
        double dSgnf6Mask;
        double dR6;
        double dZ;
        double dNeg65Div64;
        double dA8;
        double dA7;
        double dA6;
        double dA5;
        double dA4;
        double dA3;
        double dA2;
        double dA1;
        double dP;
        double d2k;
        double dCbrtHiZ;

        lExpHiMask = (__internal_dcbrt_la_data._lExpHiMask);
        lExpLoMask = (__internal_dcbrt_la_data._lExpLoMask);
        l1556 = (__internal_dcbrt_la_data._l1556);
        iRcpIndexMask = (__internal_dcbrt_la_data._iRcpIndexMask);
        iAbsMask = (__internal_dcbrt_la_data._iAbsMask);
        iSignMask = (__internal_dcbrt_la_data._iSignMask);
        iBias = (__internal_dcbrt_la_data._iBias);
        iSub = (__internal_dcbrt_la_data._iSub);
        iCmp = (__internal_dcbrt_la_data._iCmp);

        lX = as_ulong (va1);
        iX = ((unsigned int) ((unsigned long) lX >> 32));

        iRangeMask = (iX & iAbsMask);
        iRangeMask = (iRangeMask - iSub);
        iRangeMask = ((unsigned int) (-(signed int) ((signed int) iRangeMask > (signed int) iCmp)));
        vm = 0;
        vm = iRangeMask;

        iRcpIndex = (iX & iRcpIndexMask);
        iRcpIndex = ((unsigned int) (iRcpIndex) >> (12));

        dRcp = as_double (((__constant unsigned long *) (__internal_dcbrt_la_data._dRcp))[iRcpIndex >> 3]);

        lExp = ((unsigned long) (lX) >> (52));
        lExp = (lExp & lExpLoMask);
        lExpo3 = (lExp * l1556);
        iExp = (((unsigned int) lExp & (unsigned int) -1));
        iExpo3 = (((unsigned int) lExpo3 & (unsigned int) -1));
        iExpo3 = ((unsigned int) (iExpo3) >> (14));
        i2Expo3 = (iExpo3 + iExpo3);
        iExp = (iExp - iExpo3);
        iExp = (iExp - i2Expo3);
        iExp = ((unsigned int) (iExp) << (8));
        iCbrtIndex = (iRcpIndex + iExp);

        dCbrt[0] = as_double (((__constant unsigned long *) (__internal_dcbrt_la_data._dCbrtHiLo))[iCbrtIndex >> 3]);

        iSign = ((unsigned int) (iX) >> (20));
        iSign = (iSign & iSignMask);
        i2k = (iSign | iBias);
        i2k = (i2k + iExpo3);
        i2k = ((unsigned int) (i2k) << (20));
        l2k = (((unsigned long) (unsigned int) i2k << 32));

        d2k = as_double (l2k);

        dMantissaMask = as_double (__internal_dcbrt_la_data._dMantissaMask);
        dR = as_double ((as_ulong (va1) & as_ulong (dMantissaMask)));
        dNegOne = as_double (__internal_dcbrt_la_data._dNegOne);
        dR = as_double ((as_ulong (dR) | as_ulong (dNegOne)));
        dSgnf6Mask = as_double (__internal_dcbrt_la_data._dSgnf6Mask);
        dR6 = as_double ((as_ulong (va1) & as_ulong (dSgnf6Mask)));
        dNeg65Div64 = as_double (__internal_dcbrt_la_data._dNeg65Div64);
        dR6 = as_double ((as_ulong (dR6) | as_ulong (dNeg65Div64)));
        dR = (dR - dR6);
        dZ = (dR * dRcp);
        dA7 = as_double (__internal_dcbrt_la_data._dA7);
        dA6 = as_double (__internal_dcbrt_la_data._dA6);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dA7, dZ, dA6);
        dA5 = as_double (__internal_dcbrt_la_data._dA5);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dZ, dA5);
        dA4 = as_double (__internal_dcbrt_la_data._dA4);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dZ, dA4);
        dA3 = as_double (__internal_dcbrt_la_data._dA3);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dZ, dA3);
        dA2 = as_double (__internal_dcbrt_la_data._dA2);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dZ, dA2);
        dA1 = as_double (__internal_dcbrt_la_data._dA1);
        dP = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (dP, dZ, dA1);
        dCbrt[0] = (dCbrt[0] * d2k);

        dCbrtHiZ = (dCbrt[0] * dZ);

        dP = (dP * dCbrtHiZ);

        vr1 = (dP + dCbrt[0]);
    }

    if ((vm) != 0)
    {
        double _vapi_arg1[1];
        double _vapi_res1[1];
        ((double *) _vapi_arg1)[0] = va1;
        ((double *) _vapi_res1)[0] = vr1;
        __internal_dcbrt_la_cout (_vapi_arg1, _vapi_res1);
        vr1 = ((double *) _vapi_res1)[0];
    };
    r = vr1;;

    return r;

}
