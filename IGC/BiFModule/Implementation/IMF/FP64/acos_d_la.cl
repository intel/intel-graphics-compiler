/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long SgnBit;
    unsigned long OneHalf;
    unsigned long SmallNorm;
    unsigned long dRsqrtMsk;
    unsigned long MOne;
    unsigned long HalfMask;
    unsigned long Two;
    unsigned long sqrt_coeff[4];
    unsigned long poly_coeff[12];
    unsigned long PiL;
    unsigned long PiH;
    unsigned long Pi2L;
    unsigned long Pi2H;
    unsigned long Zero;
    unsigned long SgnMask;
    unsigned long NanMask;
    unsigned long ep_coeff[6];

    unsigned long dInfs[2];
    unsigned long dOnes[2];
    unsigned long dZeros[2];
} __internal_dacos_la_data_t;
static __constant __internal_dacos_la_data_t __internal_dacos_la_data = {

    0x8000000000000000uL, 0x3fe0000000000000uL, 0x3000000000000000uL, 0xffffff0000000000uL, 0xbff0000000000000uL, 0xfffffffffc000000uL,
        0x4000000000000000uL, {
                               0xbf918000993B24C3uL, 0x3fa400006F70D42DuL, 0xbfb7FFFFFFFFFE97uL, 0x3fcFFFFFFFFFFF9DuL}

    , {
       0x3fa07520C70EB909uL, 0xbf90FB17F7DBB0EDuL, 0x3f943F44BFBC3BAEuL, 0x3f7A583395D45ED5uL, 0x3f88F8DC2AFCCAD6uL, 0x3f8C6DBBCB88BD57uL,
       0x3f91C6DCF538AD2EuL, 0x3f96E89CEBDEFadduL, 0x3f9F1C72E13AD8BEuL, 0x3fa6DB6DB3B445F8uL, 0x3fb333333337E0DEuL, 0x3fc555555555529CuL}

    , 0x3ca1a62633145c07uL, 0x400921fb54442d18uL, 0x3c91a62633145c07uL, 0x3ff921fb54442d18uL, 0x0000000000000000uL, 0x8000000000000000uL,
        0xfffc000000000000uL, {
                               0x3fa36C5AF645A11EuL, 0x3f8CE147EA9E9282uL, 0x3fa056B4151FA155uL, 0x3fa6C8ED2A4CCE54uL, 0x3fb33399EBF85B6AuL,
                               0x3fc5555480C83A45uL}

    , {0x7ff0000000000000uL, 0xfff0000000000000uL}

    , {0x3ff0000000000000uL, 0xbff0000000000000uL}

    , {0x0000000000000000uL, 0x8000000000000000uL}

};
static __constant int_double __dacos_la_c12 = { 0x3fa07520c559a401UL };
static __constant int_double __dacos_la_c11 = { 0xbf90fb17f2824aa0UL };
static __constant int_double __dacos_la_c10 = { 0x3f943f44bc0c21f0UL };
static __constant int_double __dacos_la_c9 = { 0x3f7a58339bb6f20cUL };
static __constant int_double __dacos_la_c8 = { 0x3f88f8dc2a3c76d7UL };
static __constant int_double __dacos_la_c7 = { 0x3f8c6dbbcba98e67UL };
static __constant int_double __dacos_la_c6 = { 0x3f91c6dcf536c796UL };
static __constant int_double __dacos_la_c5 = { 0x3f96e89cebdf209cUL };
static __constant int_double __dacos_la_c4 = { 0x3f9f1c72e13ad6d3UL };
static __constant int_double __dacos_la_c3 = { 0x3fa6db6db3b44600UL };
static __constant int_double __dacos_la_c2 = { 0x3fb333333337e0deUL };
static __constant int_double __dacos_la_c1 = { 0x3fc555555555529cUL };
static __constant int_double __dacos_la_c0 = { 0x3ff0000000000000UL };
static __constant int_double __dacos_la_pi2h = { 0x3ff921fb54442d18UL };
static __constant int_double __dacos_la_pi2l = { 0x3c91a62633145c07UL };

static __constant int_double __dacos_la_pih = { 0x400921fb54442d18UL };
static __constant int_double __dacos_la_pil = { 0x3ca1a62633145c07UL };

static __constant int_float __dacos_la_small_float = { 0x01800000u };

__attribute__((always_inline))
inline int __internal_dacos_la_cout (double *pxin, double *pres)
{
    int nRet = 0;
    double xin = *pxin;
    int_double y, res;
    {
        int_double x, xa, RS, Shh2, High, R0;
        double R, E, poly, Sh;
        unsigned long sgn_x;
        float yf;
        int_float fcorr;

        x.f = xin;

        xa.f = __spirv_ocl_fabs (x.f);

        sgn_x = x.w ^ xa.w;

        y.f = __spirv_ocl_fma(-(0.5),xa.f,0.5);

        R = xin * xin;
        R = __spirv_ocl_fmin (R, y.f);

        High.f = sgn_x ? __dacos_la_pih.f : 0;
        High.f = (xa.f <= 0.5) ? __dacos_la_pi2h.f : High.f;

        yf = (float) y.f;

        yf += __dacos_la_small_float.f;
        yf = 1.0f / __spirv_ocl_sqrt (yf);
        RS.f = (double) (yf);

        RS.w |= sgn_x;

        Sh = (y.f * RS.f);

        Shh2.f = -2.0 * Sh;

        E = (__spirv_ocl_fma(-(RS.f),Sh,1.0));

        R0.f = __spirv_ocl_fma(0.375,E,0.5);
        R0.f *= E;

        R0.f = __spirv_ocl_fma(R0.f,Shh2.f,Shh2.f);

        poly = __spirv_ocl_fma(__dacos_la_c12.f,R,__dacos_la_c11.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c10.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c9.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c8.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c7.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c6.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c5.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c4.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c3.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c2.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c1.f);
        poly = __spirv_ocl_fma(poly,R,__dacos_la_c0.f);
        R0.f = (xa.f <= 0.5) ? x.f : R0.f;

        res.f = (__spirv_ocl_fma(-(poly),R0.f,High.f));

    }

    *pres = res.f;
    nRet = (y.f >= 0) ? 0 : 1;

    return nRet;
}

double __ocl_svml_acos (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    __internal_dacos_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}
