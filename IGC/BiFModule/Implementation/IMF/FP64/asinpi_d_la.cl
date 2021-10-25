/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct
{
    unsigned long AbsMask;
    unsigned long OneHalf;
    unsigned long SmallNorm;
    unsigned long dRsqrtMsk;
    unsigned long One;
    unsigned long HalfMask;
    unsigned long Two;
    unsigned long sqrt_coeff[4];
    unsigned long poly_coeff[12];
    unsigned long InvPiH;
    unsigned long InvPiL;
    unsigned long InvPi;
    unsigned long InvPiLL;
    unsigned long SgnMask;
    unsigned long ep_coeff[6];

    unsigned long dInfs[2];
    unsigned long dOnes[2];
    unsigned long dZeros[2];
} __internal_dasinpi_la_data_t;
static __constant __internal_dasinpi_la_data_t __internal_dasinpi_la_data = {

    0x7fffffffffffffffuL, 0x3fe0000000000000uL, 0x3000000000000000uL, 0xffffff0000000000uL, 0x3ff0000000000000uL, 0xfffffffffc000000uL,
        0x4000000000000000uL, {
                               0xbf918000993B24C3uL, 0x3fa400006F70D42DuL, 0xbfb7FFFFFFFFFE97uL, 0x3fcFFFFFFFFFFF9DuL}

    , {
       0x3f84F4523BC020D8uL, 0xbf759EE439EEE799uL, 0x3f79C78AE09A5457uL, 0x3f60C57DF579794AuL, 0x3f6FCBA466EA069BuL, 0x3f7219262ADC70A8uL,
       0x3f76A256C108AAEduL, 0x3f7D2B0EA1978F74uL, 0x3f83CE53573AD4F4uL, 0x3f8D1A452B1C8F4EuL, 0x3f98723A1D5E7F21uL, 0x3faB2995E7B7B28BuL}

    , 0x3fd45F3070000000uL, 0xbe21b1bbead603d9uL, 0x3fd45F306DC9C883uL, 0xbc76b01ec5417056uL, 0x8000000000000000uL, {
                                                                                                                     0x3f88BAFFDA4549F0uL,
                                                                                                                     0x3f7262B57524FB3BuL,
                                                                                                                     0x3f84CD955BDDED9fuL,
                                                                                                                     0x3f8D02B66C2AD236uL,
                                                                                                                     0x3f9872BCE76EFA44uL,
                                                                                                                     0x3faB2994D916CB05uL}

    , {0x7ff0000000000000uL, 0xfff0000000000000uL}

    , {0x3ff0000000000000uL, 0xbff0000000000000uL}

    , {0x0000000000000000uL, 0x8000000000000000uL}

};
static __constant int_double __dasinpi_la_c12 = { 0x3f84f45239939e37UL };
static __constant int_double __dasinpi_la_c11 = { 0xbf759ee4331f54b9UL };
static __constant int_double __dasinpi_la_c10 = { 0x3f79c78adbe84367UL };
static __constant int_double __dasinpi_la_c9 = { 0x3f60c57df93896daUL };
static __constant int_double __dasinpi_la_c8 = { 0x3f6fcba465f52565UL };
static __constant int_double __dasinpi_la_c7 = { 0x3f7219262af154f1UL };
static __constant int_double __dasinpi_la_c6 = { 0x3f76a256c10640a7UL };
static __constant int_double __dasinpi_la_c5 = { 0x3f7d2b0ea197bf84UL };
static __constant int_double __dasinpi_la_c4 = { 0x3f83ce53573ad3bcUL };
static __constant int_double __dasinpi_la_c3 = { 0x3f8d1a452b1c8f59UL };
static __constant int_double __dasinpi_la_c2 = { 0x3f98723a1d5e7f21UL };
static __constant int_double __dasinpi_la_c1 = { 0x3fab2995e7b7b28cUL };
static __constant int_double __dasinpi_la_c0 = { 0x3fd45f306dc9c883UL };
static __constant int_double __dasinpi_la_two = { 0x4000000000000000UL };

static __constant int_double __dasinpi_la_pi2h = { 0x3ff921fb54442d18UL };
static __constant int_double __dasinpi_la_pi2l = { 0x3c91a62633145c07UL };

static __constant int_float __dasinpi_la_small_float = { 0x01800000u };

__attribute__((always_inline))
inline int __internal_dasinpi_la_cout (double *pxin, double *pres)
{
    int nRet = 0;
    double xin = *pxin;
    int_double y, res;
    {
        int_double x, xa, RS, RS2, Shh2, High, Low, R0;
        double R, E, poly, Sh, Shh;
        unsigned long sgn_x;
        float yf;

        x.f = xin;

        xa.f = SPIRV_OCL_BUILTIN(fabs, _f64, ) (x.f);

        sgn_x = x.w ^ xa.w;

        y.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-0.5, xa.f, 0.5);

        R = xin * xin;
        R = SPIRV_OCL_BUILTIN(fmin, _f64_f64, ) (R, y.f);

        yf = (float) y.f;
        yf += __dasinpi_la_small_float.f;
        yf = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, ) (yf);
        RS.f = (double) (yf);

        Sh = y.f * RS.f;

        Shh2.f = -2.0 * Sh;

        E = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (-RS.f, Sh, 1.0);

        R0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (0.375, E, 0.5);
        R0.f *= E;

        R0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R0.f, Shh2.f, Shh2.f);

        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dasinpi_la_c12.f, R, __dasinpi_la_c11.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c10.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c9.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c8.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c7.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c6.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c5.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c4.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c3.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c2.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c1.f);
        poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dasinpi_la_c0.f);
        R0.f = (xa.f <= 0.5) ? xa.f : R0.f;

        High.f = (xa.f <= 0.5) ? 0.0 : 0.5;

        res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R0.f, High.f);

        res.w ^= sgn_x;

    }

    *pres = res.f;
    nRet = (y.f >= 0) ? 0 : 1;

    return nRet;
}

double __ocl_svml_asinpi (double a)
{

    double va1;
    double vr1;
    unsigned int vm;

    double r;

    va1 = a;;

    __internal_dasinpi_la_cout (&va1, &vr1);
    r = vr1;;

    return r;

}
