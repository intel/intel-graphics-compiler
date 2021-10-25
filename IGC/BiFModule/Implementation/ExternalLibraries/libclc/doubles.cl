/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"
#include "math.h"
#include "tables.cl"

#ifndef __DOUBLES_CL__
#define __DOUBLES_CL__

#if defined(cl_khr_fp64)

/*################################## Helper Functions ###############################################*/

#define bytealign(src0, src1, src2) \
  ((uint) (((((long)(src0)) << 32) | (long)(src1)) >> (((src2) & 3)*8)))

INLINE double __clc_exp_helper(double x, double x_min, double x_max, double r, int n) {

    int j = n & 0x3f;
    int m = n >> 6;

    // 6 term tail of Taylor expansion of e^r
    double z2 = r * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 0x1.6c16c16c16c17p-10, 0x1.1111111111111p-7),
                        0x1.5555555555555p-5),
                    0x1.5555555555555p-3),
                    0x1.0000000000000p-1),
                1.0);

    double2 tv = USE_TABLE(two_to_jby64_ep_tbl, j);
    z2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(tv.s0 + tv.s1, z2, tv.s1) + tv.s0;

    int small_value = (m < -1022) || ((m == -1022) && (z2 < 1.0));

    int n1 = m >> 2;
    int n2 = m-n1;
    double z3= z2 * as_double(((long)n1 + 1023) << 52);
    z3 *= as_double(((long)n2 + 1023) << 52);

    z2 = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(z2, m);
    z2 = small_value ? z3: z2;

    z2 = __intel_relaxed_isnan(x) ? x : z2;

    z2 = x > x_max ? as_double(PINFBITPATT_DP64) : z2;
    z2 = x < x_min ? 0.0 : z2;

    return z2;
}

#define LN0 8.33333333333317923934e-02
#define LN1 1.25000000037717509602e-02
#define LN2 2.23213998791944806202e-03
#define LN3 4.34887777707614552256e-04

#define LF0 8.33333333333333593622e-02
#define LF1 1.24999999978138668903e-02
#define LF2 2.23219810758559851206e-03

void __clc_ep_log(double x, int *xexp, double *r1, double *r2)
{
    // Computes natural log(x). Algorithm based on:
    // Ping-Tak Peter Tang
    // "Table-driven implementation of the logarithm function in IEEE
    // floating-point arithmetic"
    // ACM Transactions on Mathematical Software (TOMS)
    // Volume 16, Issue 4 (December 1990)
    int near_one = x >= 0x1.e0faap-1 & x <= 0x1.1082cp+0;

    ulong ux = as_ulong(x);
    ulong uxs = as_ulong(as_double(0x03d0000000000000UL | ux) - 0x1.0p-962);
    int c = ux < IMPBIT_DP64;
    ux = c ? uxs : ux;
    int expadjust = c ? 60 : 0;

    // Store the exponent of x in xexp and put f into the range [0.5,1)
    int xexp1 = ((as_int2(ux).hi >> 20) & 0x7ff) - EXPBIAS_DP64 - expadjust;
    double f = as_double(HALFEXPBITS_DP64 | (ux & MANTBITS_DP64));
    *xexp = near_one ? 0 : xexp1;

    double r = x - 1.0;
    double u1 = MATH_DIVIDE(r, 2.0 + r);
    double ru1 = -r * u1;
    u1 = u1 + u1;

    int index = as_int2(ux).hi >> 13;
    index = ((0x80 | (index & 0x7e)) >> 1) + (index & 0x1);

    double f1 = index * 0x1.0p-7;
    double f2 = f - f1;
    double u2 = MATH_DIVIDE(f2, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(0.5, f2, f1));

    double2 tv = USE_TABLE(ln_tbl, (index - 64));
    double z1 = tv.s0;
    double q = tv.s1;

    z1 = near_one ? r : z1;
    q = near_one ? 0.0 : q;
    double u = near_one ? u1 : u2;
    double v = u*u;

    double cc = near_one ? ru1 : u2;

    double z21 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, LN3, LN2), LN1), LN0);
    double z22 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, LF2, LF1), LF0);
    double z2 = near_one ? z21 : z22;
    z2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(u*v, z2, cc) + q;

    *r1 = z1;
    *r2 = z2;
}

// Reduction for medium sized arguments
void __clc_remainder_piby2_medium(double x, double *r, double *rr, int *regn) {
    // How many pi/2 is x a multiple of?
    const double two_by_pi = 0x1.45f306dc9c883p-1;
    double dnpi2 = SPIRV_OCL_BUILTIN(trunc, _f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x, two_by_pi, 0.5));

    const double piby2_h = -7074237752028440.0 / 0x1.0p+52;
    const double piby2_m = -2483878800010755.0 / 0x1.0p+105;
    const double piby2_t = -3956492004828932.0 / 0x1.0p+158;

    // Compute product of npi2 with 159 bits of 2/pi
    double p_hh = piby2_h * dnpi2;
    double p_ht = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(piby2_h, dnpi2, -p_hh);
    double p_mh = piby2_m * dnpi2;
    double p_mt = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(piby2_m, dnpi2, -p_mh);
    double p_th = piby2_t * dnpi2;
    double p_tt = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(piby2_t, dnpi2, -p_th);

    // Reduce to 159 bits
    double ph = p_hh;
    double pm = p_ht + p_mh;
    double t = p_mh - (pm - p_ht);
    double pt = p_th + t + p_mt + p_tt;
    t = ph + pm; pm = pm - (t - ph); ph = t;
    t = pm + pt; pt = pt - (t - pm); pm = t;

    // Subtract from x
    t = x + ph;
    double qh = t + pm;
    double qt = pm - (qh - t) + pt;

    *r = qh;
    *rr = qt;
    *regn = (int)(long)dnpi2 & 0x3;
}

// Given positive argument x, reduce it to the range [-pi/4,pi/4] using
// extra precision, and return the result in r, rr.
// Return value "regn" tells how many lots of pi/2 were subtracted
// from x to put it in the range [-pi/4,pi/4], mod 4.

void __clc_remainder_piby2_large(double x, double *r, double *rr, int *regn) {

    long ux = as_long(x);
    int e = (int)(ux >> 52) -  1023;
    int i = SPIRV_OCL_BUILTIN(s_max, _i32_i32, )(23, (e >> 3) + 17);
    int j = 150 - i;
    int j16 = j & ~0xf;
    double fract_temp;

    // The following extracts 192 consecutive bits of 2/pi aligned on an arbitrary byte boundary
    uint4 q0 = USE_TABLE(pibits_tbl, j16);
    uint4 q1 = USE_TABLE(pibits_tbl, (j16 + 16));
    uint4 q2 = USE_TABLE(pibits_tbl, (j16 + 32));

    int k = (j >> 2) & 0x3;
    int4 c = (int4)k == (int4)(0, 1, 2, 3);

    uint u0, u1, u2, u3, u4, u5, u6;

    u0 = c.s1 ? q0.s1 : q0.s0;
    u0 = c.s2 ? q0.s2 : u0;
    u0 = c.s3 ? q0.s3 : u0;

    u1 = c.s1 ? q0.s2 : q0.s1;
    u1 = c.s2 ? q0.s3 : u1;
    u1 = c.s3 ? q1.s0 : u1;

    u2 = c.s1 ? q0.s3 : q0.s2;
    u2 = c.s2 ? q1.s0 : u2;
    u2 = c.s3 ? q1.s1 : u2;

    u3 = c.s1 ? q1.s0 : q0.s3;
    u3 = c.s2 ? q1.s1 : u3;
    u3 = c.s3 ? q1.s2 : u3;

    u4 = c.s1 ? q1.s1 : q1.s0;
    u4 = c.s2 ? q1.s2 : u4;
    u4 = c.s3 ? q1.s3 : u4;

    u5 = c.s1 ? q1.s2 : q1.s1;
    u5 = c.s2 ? q1.s3 : u5;
    u5 = c.s3 ? q2.s0 : u5;

    u6 = c.s1 ? q1.s3 : q1.s2;
    u6 = c.s2 ? q2.s0 : u6;
    u6 = c.s3 ? q2.s1 : u6;

    uint v0 = bytealign(u1, u0, j);
    uint v1 = bytealign(u2, u1, j);
    uint v2 = bytealign(u3, u2, j);
    uint v3 = bytealign(u4, u3, j);
    uint v4 = bytealign(u5, u4, j);
    uint v5 = bytealign(u6, u5, j);

    // Place those 192 bits in 4 48-bit doubles along with correct exponent
    // If i > 1018 we would get subnormals so we scale p up and x down to get the same product
    i = 2 + 8*i;
    x *= i > 1018 ? 0x1.0p-136 : 1.0;
    i -= i > 1018 ? 136 : 0;

    uint ua = (uint)(1023 + 52 - i) << 20;
    double a = as_double((uint2)(0, ua));
    double p0 = as_double((uint2)(v0, ua | (v1 & 0xffffU))) - a;
    ua += 0x03000000U;
    a = as_double((uint2)(0, ua));
    double p1 = as_double((uint2)((v2 << 16) | (v1 >> 16), ua | (v2 >> 16))) - a;
    ua += 0x03000000U;
    a = as_double((uint2)(0, ua));
    double p2 = as_double((uint2)(v3, ua | (v4 & 0xffffU))) - a;
    ua += 0x03000000U;
    a = as_double((uint2)(0, ua));
    double p3 = as_double((uint2)((v5 << 16) | (v4 >> 16), ua | (v5 >> 16))) - a;

    // Exact multiply
    double f0h = p0 * x;
    double f0l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p0, x, -f0h);
    double f1h = p1 * x;
    double f1l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p1, x, -f1h);
    double f2h = p2 * x;
    double f2l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p2, x, -f2h);
    double f3h = p3 * x;
    double f3l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p3, x, -f3h);

    // Accumulate product into 4 doubles
    double s, t;

    double f3 = f3h + f2h;
    t = f2h - (f3 - f3h);
    s = f3l + t;
    t = t - (s - f3l);

    double f2 = s + f1h;
    t = f1h - (f2 - s) + t;
    s = f2l + t;
    t = t - (s - f2l);

    double f1 = s + f0h;
    t = f0h - (f1 - s) + t;
    s = f1l + t;

    double f0 = s + f0l;

    // Strip off unwanted large integer bits
    f3 = 0x1.0p+10 * SPIRV_OCL_BUILTIN(fract, _f64_p0f64, )(f3 * 0x1.0p-10, &fract_temp);
    f3 += f3 + f2 < 0.0 ? 0x1.0p+10 : 0.0;

    // Compute least significant integer bits
    t = f3 + f2;
    double di = t - SPIRV_OCL_BUILTIN(fract, _f64_p0f64, )(t, &fract_temp);
    i = (float)di;

    // Shift out remaining integer part
    f3 -= di;
    s = f3 + f2; t = f2 - (s - f3); f3 = s; f2 = t;
    s = f2 + f1; t = f1 - (s - f2); f2 = s; f1 = t;
    f1 += f0;

    // Subtract 1 if fraction is >= 0.5, and update regn
    int g = f3 >= 0.5;
    i += g;
    f3 -= (float)g;

    // Shift up bits
    s = f3 + f2; t = f2 -(s - f3); f3 = s; f2 = t + f1;

    // Multiply precise fraction by pi/2 to get radians
    const double p2h = 7074237752028440.0 / 0x1.0p+52;
    const double p2t = 4967757600021510.0 / 0x1.0p+106;

    double rhi = f3 * p2h;
    double rlo = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f2, p2h, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f3, p2t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f3, p2h, -rhi)));

    *r = rhi + rlo;
    *rr = rlo - (*r - rhi);
    *regn = i & 0x3;
}

double2 __clc_sincos_piby4(double x, double xx) {
    // Taylor series for sin(x) is x - x^3/3! + x^5/5! - x^7/7! ...
    //                      = x * (1 - x^2/3! + x^4/5! - x^6/7! ...
    //                      = x * f(w)
    // where w = x*x and f(w) = (1 - w/3! + w^2/5! - w^3/7! ...
    // We use a minimax approximation of (f(w) - 1) / w
    // because this produces an expansion in even powers of x.
    // If xx (the tail of x) is non-zero, we add a correction
    // term g(x,xx) = (1-x*x/2)*xx to the result, where g(x,xx)
    // is an approximation to cos(x)*sin(xx) valid because
    // xx is tiny relative to x.

    // Taylor series for cos(x) is 1 - x^2/2! + x^4/4! - x^6/6! ...
    //                      = f(w)
    // where w = x*x and f(w) = (1 - w/2! + w^2/4! - w^3/6! ...
    // We use a minimax approximation of (f(w) - 1 + w/2) / (w*w)
    // because this produces an expansion in even powers of x.
    // If xx (the tail of x) is non-zero, we subtract a correction
    // term g(x,xx) = x*xx to the result, where g(x,xx)
    // is an approximation to sin(x)*sin(xx) valid because
    // xx is tiny relative to x.

    const double sc1 = -0.166666666666666646259241729;
    const double sc2 =  0.833333333333095043065222816e-2;
    const double sc3 = -0.19841269836761125688538679e-3;
    const double sc4 =  0.275573161037288022676895908448e-5;
    const double sc5 = -0.25051132068021699772257377197e-7;
    const double sc6 =  0.159181443044859136852668200e-9;

    const double cc1 =  0.41666666666666665390037e-1;
    const double cc2 = -0.13888888888887398280412e-2;
    const double cc3 =  0.248015872987670414957399e-4;
    const double cc4 = -0.275573172723441909470836e-6;
    const double cc5 =  0.208761463822329611076335e-8;
    const double cc6 = -0.113826398067944859590880e-10;

    double x2 = x * x;
    double x3 = x2 * x;
    double r = 0.5 * x2;
    double t = 1.0 - r;

    double sp = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(sc6, x2, sc5), x2, sc4), x2, sc3), x2, sc2);

    double cp = t + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cc6, x2, cc5), x2, cc4), x2, cc3), x2, cc2), x2, cc1),
                        x2*x2, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x, xx, (1.0 - t) - r));

    double2 ret;
    ret.lo = x - SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-x3, sc1, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-x3, sp, 0.5*xx), x2, -xx));
    ret.hi = cp;

    return ret;
}

/*################################# libclc_acos_f64 ##############################################*/

INLINE double libclc_acos_f64(double x) {
  return (
      2.0 * SPIRV_OCL_BUILTIN(atan2, _f64_f64, )(
      SPIRV_OCL_BUILTIN(sqrt, _f64, )(1.0 - x),
      SPIRV_OCL_BUILTIN(sqrt, _f64, )(1.0 + x)
    )
  );
}

/*################################# libclc_acosh_f64 ##############################################*/

INLINE double libclc_acosh_f64(double x) {
    const double recrteps = 0x1.6a09e667f3bcdp+26;    // 1/SPIRV_OCL_BUILTIN(sqrt, _f64, )(eps) = 9.49062656242515593767e+07
    //log2_lead and log2_tail sum to an extra-precise version of log(2)
    const double log2_lead = 0x1.62e42ep-1;
    const double log2_tail = 0x1.efa39ef35793cp-25;

    // Handle x >= 128 here
    int xlarge = x > recrteps;
    double r = x + SPIRV_OCL_BUILTIN(sqrt, _f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x, x, -1.0));
    r = xlarge ? x : r;

    int xexp;
    double r1, r2;
    __clc_ep_log(r, &xexp, &r1, &r2);

    double dxexp = xexp + xlarge;
    r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_lead, r1);
    r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_tail, r2);

    double ret1 = r1 + r2;

    // Handle 1 < x < 128 here
    // We compute the value
    // t = x - 1.0 + SPIRV_OCL_BUILTIN(sqrt, _f64, )(2.0*(x - 1.0) + (x - 1.0)*(x - 1.0))
    // using simulated quad precision.
    double t = x - 1.0;
    double u1 = t * 2.0;

    // (t,0) * (t,0) -> (v1, v2)
    double v1 = t * t;
    double v2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, t, -v1);

    // (u1,0) + (v1,v2) -> (w1,w2)
    r = u1 + v1;
    double s = (((u1 - r) + v1) + v2);
    double w1 = r + s;
    double w2 = (r - w1) + s;

    // SPIRV_OCL_BUILTIN(sqrt, _f64, )(w1,w2) -> (u1,u2)
    double p1 = SPIRV_OCL_BUILTIN(sqrt, _f64, )(w1);
    double a1 = p1*p1;
    double a2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(p1, p1, -a1);
    double temp = (((w1 - a1) - a2) + w2);
    double p2 = MATH_DIVIDE(temp * 0.5, p1);
    u1 = p1 + p2;
    double u2 = (p1 - u1) + p2;

    // (u1,u2) + (t,0) -> (r1,r2)
    r = u1 + t;
    s = ((u1 - r) + t) + u2;
    // r1 = r + s;
    // r2 = (r - r1) + s;
    // t = r1 + r2;
    t = r + s;

    // For arguments 1.13 <= x <= 1.5 the log1p function is good enough
    double ret2 = SPIRV_OCL_BUILTIN(log1p, _f64, )(t);

    ulong ux = as_ulong(x);
    double ret = x >= 128.0 ? ret1 : ret2;

    ret = ux >= 0x7FF0000000000000 ? x : ret;
    ret = x == 1.0 ? 0.0 : ret;
    ret = (ux & SIGNBIT_DP64) != 0UL | x < 1.0 ? as_double(QNANBITPATT_DP64) : ret;

    return ret;
}



/*################################# libclc_acospi_f64 #############################################*/

INLINE double libclc_acospi_f64(double x) {
    // Computes arccos(x).
    // The argument is first reduced by noting that arccos(x)
    // is invalid for abs(x) > 1. For denormal and small
    // arguments arccos(x) = pi/2 to machine accuracy.
    // Remaining argument ranges are handled as follows.
    // For abs(x) <= 0.5 use
    // arccos(x) = pi/2 - arcSPIRV_OCL_BUILTIN(sin, _f64, )(x)
    // = pi/2 - (x + x^3*R(x^2))
    // where R(x^2) is a rational minimax approximation to
    // (arcSPIRV_OCL_BUILTIN(sin, _f64, )(x) - x)/x^3.
    // For abs(x) > 0.5 exploit the identity:
    // arccos(x) = pi - 2*sin(SPIRV_OCL_BUILTIN(sqrt, _f64, )(1-x)/2)
    // together with the above rational approximation, and
    // reconstruct the terms carefully.

    const double pi = 0x1.921fb54442d18p+1;
    const double piby2_tail = 6.12323399573676603587e-17;        /* 0x3c91a62633145c07 */

    double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);
    int xneg = as_int2(x).hi < 0;
    int xexp = (as_int2(y).hi >> 20) - EXPBIAS_DP64;

    // abs(x) >= 0.5
    int transform = xexp >= -1;

    // Transform y into the range [0,0.5)
    double r1 = 0.5 * (1.0 - y);
    double s = SPIRV_OCL_BUILTIN(sqrt, _f64, )(r1);
    double r = y * y;
    r = transform ? r1 : r;
    y = transform ? s : y;

    // Use a rational approximation for [0.0, 0.5]
    double un = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 0.0000482901920344786991880522822991,
                                       0.00109242697235074662306043804220),
                                -0.0549989809235685841612020091328),
                            0.275558175256937652532686256258),
                        -0.445017216867635649900123110649),
                    0.227485835556935010735943483075);

    double ud = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 0.105869422087204370341222318533,
                                   -0.943639137032492685763471240072),
                            2.76568859157270989520376345954),
                        -3.28431505720958658909889444194),
                    1.36491501334161032038194214209);

    double u = r * MATH_DIVIDE(un, ud);

    // Reconstruct acos carefully in transformed region
    double res1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-2.0, MATH_DIVIDE(s + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y, u, -piby2_tail), pi), 1.0);
    double s1 = as_double(as_ulong(s) & 0xffffffff00000000UL);
    double c = MATH_DIVIDE(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s1, s1, r), s + s1);
    double res2 = MATH_DIVIDE(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(2.0, s1, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(2.0, c, 2.0 * y * u)), pi);
    res1 = xneg ? res1 : res2;
    res2 = 0.5 - SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x, u, x) / pi;
    res1 = transform ? res1 : res2;

    const double qnan = as_double(QNANBITPATT_DP64);
    res2 = x == 1.0 ? 0.0 : qnan;
    res2 = x == -1.0 ? 1.0 : res2;
    res1 = xexp >= 0 ? res2 : res1;
    res1 = xexp < -56 ? 0.5 : res1;

    return res1;
}



/*################################# libclc_asin_f64 ###############################################*/

INLINE double libclc_asin_f64(double x) {
  return SPIRV_OCL_BUILTIN(atan2, _f64_f64, )(x, SPIRV_OCL_BUILTIN(sqrt, _f64, )( 1.0 - (x*x)));
}

/*################################# libclc_asinh_f64 ###############################################*/

#define NA0 -0.12845379283524906084997e0
#define NA1 -0.21060688498409799700819e0
#define NA2 -0.10188951822578188309186e0
#define NA3 -0.13891765817243625541799e-1
#define NA4 -0.10324604871728082428024e-3

#define DA0  0.77072275701149440164511e0
#define DA1  0.16104665505597338100747e1
#define DA2  0.11296034614816689554875e1
#define DA3  0.30079351943799465092429e0
#define DA4  0.235224464765951442265117e-1

#define NB0 -0.12186605129448852495563e0
#define NB1 -0.19777978436593069928318e0
#define NB2 -0.94379072395062374824320e-1
#define NB3 -0.12620141363821680162036e-1
#define NB4 -0.903396794842691998748349e-4

#define DB0  0.73119630776696495279434e0
#define DB1  0.15157170446881616648338e1
#define DB2  0.10524909506981282725413e1
#define DB3  0.27663713103600182193817e0
#define DB4  0.21263492900663656707646e-1

#define NC0 -0.81210026327726247622500e-1
#define NC1 -0.12327355080668808750232e0
#define NC2 -0.53704925162784720405664e-1
#define NC3 -0.63106739048128554465450e-2
#define NC4 -0.35326896180771371053534e-4

#define DC0  0.48726015805581794231182e0
#define DC1  0.95890837357081041150936e0
#define DC2  0.62322223426940387752480e0
#define DC3  0.15028684818508081155141e0
#define DC4  0.10302171620320141529445e-1

#define ND0 -0.4638179204422665073e-1
#define ND1 -0.7162729496035415183e-1
#define ND2 -0.3247795155696775148e-1
#define ND3 -0.4225785421291932164e-2
#define ND4 -0.3808984717603160127e-4
#define ND5  0.8023464184964125826e-6

#define DD0  0.2782907534642231184e0
#define DD1  0.5549945896829343308e0
#define DD2  0.3700732511330698879e0
#define DD3  0.9395783438240780722e-1
#define DD4  0.7200057974217143034e-2

#define NE0 -0.121224194072430701e-4
#define NE1 -0.273145455834305218e-3
#define NE2 -0.152866982560895737e-2
#define NE3 -0.292231744584913045e-2
#define NE4 -0.174670900236060220e-2
#define NE5 -0.891754209521081538e-12

#define DE0  0.499426632161317606e-4
#define DE1  0.139591210395547054e-2
#define DE2  0.107665231109108629e-1
#define DE3  0.325809818749873406e-1
#define DE4  0.415222526655158363e-1
#define DE5  0.186315628774716763e-1

#define NF0  -0.195436610112717345e-4
#define NF1  -0.233315515113382977e-3
#define NF2  -0.645380957611087587e-3
#define NF3  -0.478948863920281252e-3
#define NF4  -0.805234112224091742e-12
#define NF5   0.246428598194879283e-13

#define DF0   0.822166621698664729e-4
#define DF1   0.135346265620413852e-2
#define DF2   0.602739242861830658e-2
#define DF3   0.972227795510722956e-2
#define DF4   0.510878800983771167e-2

#define NG0  -0.209689451648100728e-6
#define NG1  -0.219252358028695992e-5
#define NG2  -0.551641756327550939e-5
#define NG3  -0.382300259826830258e-5
#define NG4  -0.421182121910667329e-17
#define NG5   0.492236019998237684e-19

#define DG0   0.889178444424237735e-6
#define DG1   0.131152171690011152e-4
#define DG2   0.537955850185616847e-4
#define DG3   0.814966175170941864e-4
#define DG4   0.407786943832260752e-4

#define NH0  -0.178284193496441400e-6
#define NH1  -0.928734186616614974e-6
#define NH2  -0.923318925566302615e-6
#define NH3  -0.776417026702577552e-19
#define NH4   0.290845644810826014e-21

#define DH0   0.786694697277890964e-6
#define DH1   0.685435665630965488e-5
#define DH2   0.153780175436788329e-4
#define DH3   0.984873520613417917e-5

#define NI0  -0.538003743384069117e-10
#define NI1  -0.273698654196756169e-9
#define NI2  -0.268129826956403568e-9
#define NI3  -0.804163374628432850e-29

#define DI0   0.238083376363471960e-9
#define DI1   0.203579344621125934e-8
#define DI2   0.450836980450693209e-8
#define DI3   0.286005148753497156e-8

INLINE double libclc_asinh_f64(double x) {
    const double rteps = 0x1.6a09e667f3bcdp-27;
    const double recrteps = 0x1.6a09e667f3bcdp+26;

    // log2_lead and log2_tail sum to an extra-precise version of log(2)
    const double log2_lead = 0x1.62e42ep-1;
    const double log2_tail = 0x1.efa39ef35793cp-25;

    ulong ux = as_ulong(x);
    ulong ax = ux & ~SIGNBIT_DP64;
    double absx = as_double(ax);

    double t = x * x;
    double pn, tn, pd, td;

    // XXX we are betting here that we can evaluate 8 pairs of
    // polys faster than we can grab 12 coefficients from a table
    // This also uses fewer registers

    // |x| >= 8
    pn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NI3, NI2), NI1), NI0);
    pd = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DI3, DI2), DI1), DI0);

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NH4, NH3), NH2), NH1), NH0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DH3, DH2), DH1), DH0);
    pn = absx < 8.0 ? tn : pn;
    pd = absx < 8.0 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NG5, NG4), NG3), NG2), NG1), NG0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DG4, DG3), DG2), DG1), DG0);
    pn = absx < 4.0 ? tn : pn;
    pd = absx < 4.0 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NF5, NF4), NF3), NF2), NF1), NF0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DF4, DF3), DF2), DF1), DF0);
    pn = absx < 2.0 ? tn : pn;
    pd = absx < 2.0 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NE5, NE4), NE3), NE2), NE1), NE0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DE5, DE4), DE3), DE2), DE1), DE0);
    pn = absx < 1.5 ? tn : pn;
    pd = absx < 1.5 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, ND5, ND4), ND3), ND2), ND1), ND0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DD4, DD3), DD2), DD1), DD0);
    pn = absx <= 1.0 ? tn : pn;
    pd = absx <= 1.0 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NC4, NC3), NC2), NC1), NC0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DC4, DC3), DC2), DC1), DC0);
    pn = absx < 0.75 ? tn : pn;
    pd = absx < 0.75 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NB4, NB3), NB2), NB1), NB0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DB4, DB3), DB2), DB1), DB0);
    pn = absx < 0.5 ? tn : pn;
    pd = absx < 0.5 ? td : pd;

    tn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, NA4, NA3), NA2), NA1), NA0);
    td = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, DA4, DA3), DA2), DA1), DA0);
    pn = absx < 0.25 ? tn : pn;
    pd = absx < 0.25 ? td : pd;

    double pq = MATH_DIVIDE(pn, pd);

    // |x| <= 1
    double result1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(absx*t, pq, absx);

    // Other ranges
    int xout = absx <= 32.0 | absx > recrteps;
    double y = absx + SPIRV_OCL_BUILTIN(sqrt, _f64, )(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(absx, absx, 1.0));
    y = xout ? absx : y;

    double r1, r2;
    int xexp;
    __clc_ep_log(y, &xexp, &r1, &r2);

    double dxexp = (double)(xexp + xout);
    r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_lead, r1);
    r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_tail, r2);

    // 1 < x <= 32
    double v2 = (pq + 0.25) / t;
    double r = v2 + r1;
    double s = ((r1 - r) + v2) + r2;
    double v1 = r + s;
    v2 = (r - v1) + s;
    double result2 = v1 + v2;

    // x > 32
    double result3 = r1 + r2;

    double ret = absx > 1.0 ? result2 : result1;
    ret = absx > 32.0 ? result3 : ret;
    ret = x < 0.0 ? -ret : ret;

    // NaN, +-Inf, or x small enough that asinh(x) = x
    ret = ax >= PINFBITPATT_DP64 | absx < rteps ? x : ret;
    return ret;
}



/*################################# libclc_asinpi_f64 #############################################*/

INLINE double libclc_asinpi_f64(double x) {
    // Computes arcSPIRV_OCL_BUILTIN(sin, _f64, )(x).
    // The argument is first reduced by noting that arcSPIRV_OCL_BUILTIN(sin, _f64, )(x)
    // is invalid for abs(x) > 1 and arcSPIRV_OCL_BUILTIN(sin, _f64, )(-x) = -arcSPIRV_OCL_BUILTIN(sin, _f64, )(x).
    // For denormal and small arguments arcSPIRV_OCL_BUILTIN(sin, _f64, )(x) = x to machine
    // accuracy. Remaining argument ranges are handled as follows.
    // For abs(x) <= 0.5 use
    // arcSPIRV_OCL_BUILTIN(sin, _f64, )(x) = x + x^3*R(x^2)
    // where R(x^2) is a rational minimax approximation to
    // (arcSPIRV_OCL_BUILTIN(sin, _f64, )(x) - x)/x^3.
    // For abs(x) > 0.5 exploit the identity:
    // arcSPIRV_OCL_BUILTIN(sin, _f64, )(x) = pi/2 - 2*arcSPIRV_OCL_BUILTIN(sin, _f64, )(SPIRV_OCL_BUILTIN(sqrt, _f64, )(1-x)/2)
    // together with the above rational approximation, and
    // reconstruct the terms carefully.

    const double pi = 0x1.921fb54442d18p+1;
    const double piby2_tail = 6.1232339957367660e-17;    /* 0x3c91a62633145c07 */
    const double hpiby2_head = 7.8539816339744831e-01;    /* 0x3fe921fb54442d18 */

    double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);
    int xneg = as_int2(x).hi < 0;
    int xexp = (as_int2(y).hi >> 20) - EXPBIAS_DP64;

    // abs(x) >= 0.5
    int transform = xexp >= -1;

    double rt = 0.5 * (1.0 - y);
    double y2 = y * y;
    double r = transform ? rt : y2;

    // Use a rational approximation for [0.0, 0.5]
    double un = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 0.0000482901920344786991880522822991,
                                       0.00109242697235074662306043804220),
                                -0.0549989809235685841612020091328),
                            0.275558175256937652532686256258),
                        -0.445017216867635649900123110649),
                    0.227485835556935010735943483075);

    double ud = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 0.105869422087204370341222318533,
                                   -0.943639137032492685763471240072),
                            2.76568859157270989520376345954),
                        -3.28431505720958658909889444194),
                    1.36491501334161032038194214209);

    double u = r * MATH_DIVIDE(un, ud);


    // Reconstruct asin carefully in transformed region
    double s = SPIRV_OCL_BUILTIN(sqrt, _f64, )(r);
    double sh = as_double(as_ulong(s) & 0xffffffff00000000UL);
    double c = MATH_DIVIDE(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-sh, sh, r), s + sh);
    double p = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(2.0*s, u, -SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-2.0, c, piby2_tail));
    double q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-2.0, sh, hpiby2_head);
    double vt = hpiby2_head - (p - q);
    double v = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y, u, y);
    v = transform ? vt : v;

    v = xexp < -28 ? y : v;
    v = MATH_DIVIDE(v, pi);
    v = xexp >= 0 ? as_double(QNANBITPATT_DP64) : v;
    v = y == 1.0 ? 0.5 : v;
    return xneg ? -v : v;
}




/*################################## libclc_atan_f64 ##############################################*/

INLINE double libclc_atan_f64(double x)
{
    const double piby2 = 1.5707963267948966e+00; // 0x3ff921fb54442d18

    double v = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    // 2^56 > v > 39/16
    double a = -1.0;
    double b = v;
    // (chi + clo) = arctan(infinity)
    double chi = 1.57079632679489655800e+00;
    double clo = 6.12323399573676480327e-17;

    double ta = v - 1.5;
    double tb = 1.0 + 1.5 * v;
    int l = v <= 0x1.38p+1; // 39/16 > v > 19/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(1.5)
    chi = l ? 9.82793723247329054082e-01 : chi;
    clo = l ? 1.39033110312309953701e-17 : clo;

    ta = v - 1.0;
    tb = 1.0 + v;
    l = v <= 0x1.3p+0; // 19/16 > v > 11/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(1.)
    chi = l ? 7.85398163397448278999e-01 : chi;
    clo = l ? 3.06161699786838240164e-17 : clo;

    ta = 2.0 * v - 1.0;
    tb = 2.0 + v;
    l = v <= 0x1.6p-1; // 11/16 > v > 7/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(0.5)
    chi = l ? 4.63647609000806093515e-01 : chi;
    clo = l ? 2.26987774529616809294e-17 : clo;

    l = v <= 0x1.cp-2; // v < 7/16
    a = l ? v : a;
    b = l ? 1.0 : b;;
    chi = l ? 0.0 : chi;
    clo = l ? 0.0 : clo;

    // Core approximation: Remez(4,4) on [-7/16,7/16]
    double r = a / b;
    double s = r * r;
    double qn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s, 0.142316903342317766e-3,
                                   0.304455919504853031e-1),
                            0.220638780716667420e0),
                        0.447677206805497472e0),
                    0.268297920532545909e0);

    double qd = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s, 0.389525873944742195e-1,
                   0.424602594203847109e0),
                            0.141254259931958921e1),
                        0.182596787737507063e1),
                    0.804893761597637733e0);

    double q = r * s * qn / qd;
    r = chi - ((q - clo) - r);

    double z = __intel_relaxed_isnan(x) ? x : piby2;
    z = v <= 0x1.0p+56 ? r : z;
    z = v < 0x1.0p-26 ? v : z;
    return x == v ? z : -z;
}




/*################################## libclc_atan2_f64_f64 ##############################################*/


INLINE double libclc_atan2_f64_f64(double y, double x)
{
    const double pi = 3.1415926535897932e+00;          /* 0x400921fb54442d18 */
    const double piby2 = 1.5707963267948966e+00;       /* 0x3ff921fb54442d18 */
    const double piby4 = 7.8539816339744831e-01;       /* 0x3fe921fb54442d18 */
    const double three_piby4 = 2.3561944901923449e+00; /* 0x4002d97c7f3321d2 */
    const double pi_head = 3.1415926218032836e+00;     /* 0x400921fb50000000 */
    const double pi_tail = 3.1786509547056392e-08;     /* 0x3e6110b4611a6263 */
    const double piby2_head = 1.5707963267948965e+00;  /* 0x3ff921fb54442d18 */
    const double piby2_tail = 6.1232339957367660e-17;  /* 0x3c91a62633145c07 */

    double x2 = x;
    int xneg = as_int2(x).hi < 0;
    int xexp = (as_int2(x).hi >> 20) & 0x7ff;

    double y2 = y;
    int yneg = as_int2(y).hi < 0;
    int yexp = (as_int2(y).hi >> 20) & 0x7ff;

    int cond2 = (xexp < 1021) & (yexp < 1021);
    int diffexp = yexp - xexp;

    // Scale up both x and y if they are both below 1/4
    double x1 = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(x, 1024);
    int xexp1 = (as_int2(x1).hi >> 20) & 0x7ff;
    double y1 = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(y, 1024);
    int yexp1 = (as_int2(y1).hi >> 20) & 0x7ff;
    int diffexp1 = yexp1 - xexp1;

    diffexp = cond2 ? diffexp1 : diffexp;
    x = cond2 ? x1 : x;
    y = cond2 ? y1 : y;

    // General case: take absolute values of arguments
    double u = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);
    double v = SPIRV_OCL_BUILTIN(fabs, _f64, )(y);

    // Swap u and v if necessary to obtain 0 < v < u. Compute v/u.
    int swap_vu = u < v;
    double uu = u;
    u = swap_vu ? v : u;
    v = swap_vu ? uu : v;

    double vbyu = v / u;
    double q1, q2;

    // General values of v/u. Use a look-up table and series expansion.

    {
        double val = vbyu > 0.0625 ? vbyu : 0.063;
        int index = (int)(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(256.0, val, 0.5));
    double2 tv = USE_TABLE(atan_jby256_tbl, index - 16);
    q1 = tv.s0;
    q2 = tv.s1;
        double c = (double)index * 0x1.0p-8;

        // We're going to scale u and v by 2^(-u_exponent) to bring them close to 1
        // u_exponent could be EMAX so we have to do it in 2 steps
        int m = -((int)(as_ulong(u) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64);
    //double um = __amdil_ldexp_f64(u, m);
    //double vm = __amdil_ldexp_f64(v, m);
    double um = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(u, m);
    double vm = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(v, m);

        // 26 leading bits of u
        double u1 = as_double(as_ulong(um) & 0xfffffffff8000000UL);
        double u2 = um - u1;

        double r = MATH_DIVIDE(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-c, u2, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-c, u1, vm)), SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(c, vm, um));

        // Polynomial approximation to atan(r)
        double s = r * r;
        q2 = q2 + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )((s * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s, 0.19999918038989143496, 0.33333333333224095522)), -r, r);
    }


    double q3, q4;
    {
        q3 = 0.0;
        q4 = vbyu;
    }

    double q5, q6;
    {
        double u1 = as_double(as_ulong(u) & 0xffffffff00000000UL);
        double u2 = u - u1;
        double vu1 = as_double(as_ulong(vbyu) & 0xffffffff00000000UL);
        double vu2 = vbyu - vu1;

        q5 = 0.0;
        double s = vbyu * vbyu;
        q6 = vbyu + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-vbyu * s,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s,
                                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s,
                                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-s, 0.90029810285449784439E-01,
                                        0.11110736283514525407),
                                    0.14285713561807169030),
                                0.19999999999393223405),
                            0.33333333333333170500),
             MATH_DIVIDE(SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-u, vu2, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-u2, vu1, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-u1, vu1, v))), u));
    }


    q3 = vbyu < 0x1.d12ed0af1a27fp-27 ? q3 : q5;
    q4 = vbyu < 0x1.d12ed0af1a27fp-27 ? q4 : q6;

    q1 = vbyu > 0.0625 ? q1 : q3;
    q2 = vbyu > 0.0625 ? q2 : q4;

    // Tidy-up according to which quadrant the arguments lie in
    double res1, res2, res3, res4;
    q1 = swap_vu ? piby2_head - q1 : q1;
    q2 = swap_vu ? piby2_tail - q2 : q2;
    q1 = xneg ? pi_head - q1 : q1;
    q2 = xneg ? pi_tail - q2 : q2;
    q1 = q1 + q2;
    res4 = yneg ? -q1 : q1;

    res1 = yneg ? -three_piby4 : three_piby4;
    res2 = yneg ? -piby4 : piby4;
    res3 = xneg ? res1 : res2;

    res3 = __intel_relaxed_isinf(x2) & __intel_relaxed_isinf(y2) ? res3 : res4;
    res1 = yneg ? -pi : pi;

    // abs(x)/abs(y) > 2^56 and x < 0
    res3 = (diffexp < -56 && xneg) ? res1 : res3;

    res4 = MATH_DIVIDE(y, x);
    // x positive and dominant over y by a factor of 2^28
    res3 = diffexp < -28 & xneg == 0 ? res4 : res3;

    // abs(y)/abs(x) > 2^56
    res4 = yneg ? -piby2 : piby2;       // atan(y/x) is insignificant compared to piby2
    res3 = diffexp > 56 ? res4 : res3;

    res3 = x2 == 0.0 ? res4 : res3;   // Zero x gives +- pi/2 depending on sign of y
    res4 = xneg ? res1 : y2;

    res3 = y2 == 0.0 ? res4 : res3;   // Zero y gives +-0 for positive x and +-pi for negative x
    res3 = __intel_relaxed_isnan(y2) ? y2 : res3;
    res3 = __intel_relaxed_isnan(x2) ? x2 : res3;

    return res3;
}


/*################################## libclc_atanh_f64 ##############################################*/

INLINE double libclc_atanh_f64(double x) {
    double absx = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    double ret = absx == 1.0 ? as_double(PINFBITPATT_DP64) : as_double(QNANBITPATT_DP64);

    // |x| >= 0.5
    // Note that atanh(x) = 0.5 * ln((1+x)/(1-x))
    // For greater accuracy we use
    // ln((1+x)/(1-x)) = ln(1 + 2x/(1-x)) = SPIRV_OCL_BUILTIN(log1p, _f64, )(2x/(1-x)).
    double r = 0.5 * SPIRV_OCL_BUILTIN(log1p, _f64, )(2.0 * absx / (1.0 - absx));
    ret = absx < 1.0 ? r : ret;

    r = -ret;
    ret = x < 0.0 ? r : ret;

    // Arguments up to 0.5 in magnitude are
    // approximated by a [5,5] minimax polynomial
    double t = x * x;

    double pn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, -0.10468158892753136958e-3, 0.28728638600548514553e-1),
                                -0.28180210961780814148e0),
                            0.88468142536501647470e0),
                        -0.11028356797846341457e1),
                    0.47482573589747356373e0);

    double pd = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t,
                                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, -0.35861554370169537512e-1, 0.49561196555503101989e0),
                                -0.22608883748988489342e1),
                            0.45414700626084508355e1),
                        -0.41631933639693546274e1),
                    0.14244772076924206909e1);

    r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x*t, pn/pd, x);
    ret = absx < 0.5 ? r : ret;

    return ret;
}


/*################################## libclc_atanpi_f64 ##############################################*/

INLINE double libclc_atanpi_f64(double x) {
    const double pi = 0x1.921fb54442d18p+1;

    double v = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    // 2^56 > v > 39/16
    double a = -1.0;
    double b = v;
    // (chi + clo) = arctan(infinity)
    double chi = 1.57079632679489655800e+00;
    double clo = 6.12323399573676480327e-17;

    double ta = v - 1.5;
    double tb = 1.0 + 1.5 * v;
    int l = v <= 0x1.38p+1; // 39/16 > v > 19/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(1.5)
    chi = l ? 9.82793723247329054082e-01 : chi;
    clo = l ? 1.39033110312309953701e-17 : clo;

    ta = v - 1.0;
    tb = 1.0 + v;
    l = v <= 0x1.3p+0; // 19/16 > v > 11/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(1.)
    chi = l ? 7.85398163397448278999e-01 : chi;
    clo = l ? 3.06161699786838240164e-17 : clo;

    ta = 2.0 * v - 1.0;
    tb = 2.0 + v;
    l = v <= 0x1.6p-1; // 11/16 > v > 7/16
    a = l ? ta : a;
    b = l ? tb : b;
    // (chi + clo) = arctan(0.5)
    chi = l ? 4.63647609000806093515e-01 : chi;
    clo = l ? 2.26987774529616809294e-17 : clo;

    l = v <= 0x1.cp-2; // v < 7/16
    a = l ? v : a;
    b = l ? 1.0 : b;;
    chi = l ? 0.0 : chi;
    clo = l ? 0.0 : clo;

    // Core approximation: Remez(4,4) on [-7/16,7/16]
    double r = a / b;
    double s = r * r;
    double qn = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                    SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s, 0.142316903342317766e-3,
                                   0.304455919504853031e-1),
                            0.220638780716667420e0),
                        0.447677206805497472e0),
                    0.268297920532545909e0);

    double qd = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s,
                SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(s, 0.389525873944742195e-1,
                   0.424602594203847109e0),
                            0.141254259931958921e1),
                        0.182596787737507063e1),
                    0.804893761597637733e0);

    double q = r * s * qn / qd;
    r = (chi - ((q - clo) - r)) / pi;
    double vp = v / pi;

    double z = __intel_relaxed_isnan(x) ? x : 0.5;
    z = v <= 0x1.0p+56 ? r : z;
    z = v < 0x1.0p-26 ? vp : z;
    return x == v ? z : -z;
}

/*################################## libclc_cbrt_f64 ##############################################*/

INLINE double libclc_cbrt_f64(double x) {

    int return_x = __intel_relaxed_isinf(x) | __intel_relaxed_isnan(x) | x == 0.0;
    ulong ux = as_ulong(SPIRV_OCL_BUILTIN(fabs, _f64, )(x));
    int m = (as_int2(ux).hi >> 20) - 1023;

    // Treat subnormals
    ulong uxs = as_ulong(as_double(0x3ff0000000000000UL | ux) - 1.0);
    int ms = m + (as_int2(uxs).hi >> 20) - 1022;

    int c = m == -1023;
    ux = c ? uxs : ux;
    m = c ? ms : m;

    int mby3 = m / 3;
    int rem = m - 3*mby3;

    double mf = as_double((ulong)(mby3 + 1023) << 52);

    ux &= 0x000fffffffffffffUL;
    double Y = as_double(0x3fe0000000000000UL | ux);

    // nearest integer
    int index = as_int2(ux).hi >> 11;
    index = (0x100 | (index >> 1)) + (index & 1);
    double F = (double)index * 0x1.0p-9;

    double f = Y - F;
    double r = f * USE_TABLE(cbrt_inv_tbl, index-256);

    double z = r * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                           SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                               SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
                                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, -0x1.8090d6221a247p-6, 0x1.ee7113506ac13p-6),
                                   -0x1.511e8d2b3183bp-5),
                               0x1.f9add3c0ca458p-5),
                           -0x1.c71c71c71c71cp-4),
                       0x1.5555555555555p-2);

    double2 tv = USE_TABLE(cbrt_rem_tbl, rem+2);
    double Rem_h = tv.s0;
    double Rem_t = tv.s1;

    tv = USE_TABLE(cbrt_dbl_tbl, index-256);
    double F_h = tv.s0;
    double F_t = tv.s1;

    double b_h = F_h * Rem_h;
    double b_t = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Rem_t, F_h, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(F_t, Rem_h, F_t*Rem_t));

    double ans = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(z, b_h, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(z, b_t, b_t)) + b_h;
    ans = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(ans*mf, x);
    return return_x ? x : ans;
}


/*########################################## libclc_cos_f64 ##############################################*/

INLINE double libclc_cos_f64(double x) {
    x = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    double r, rr;
    int regn;

    if (x < 0x1.0p+47)
        __clc_remainder_piby2_medium(x, &r, &rr, &regn);
    else
        __clc_remainder_piby2_large(x, &r, &rr, &regn);

    double2 sc = __clc_sincos_piby4(r, rr);
    sc.lo = -sc.lo;

    int2 c = as_int2(regn & 1 ? sc.lo : sc.hi);
    c.hi ^= (regn > 1) << 31;

    return SPIRV_BUILTIN(IsNan, _f64, )(x) | SPIRV_BUILTIN(IsInf, _f64, )(x) ? as_double(QNANBITPATT_DP64) : as_double(c);
}

/*################################## libclc_cosh_f64 ##############################################*/

INLINE double libclc_cosh_f64(double x) {

    // After dealing with special cases the computation is split into
    // regions as follows:
    //
    // abs(x) >= max_cosh_arg:
    // cosh(x) = sign(x)*Inf
    //
    // abs(x) >= small_threshold:
    // cosh(x) = sign(x)*SPIRV_OCL_BUILTIN(exp, _f64, )(abs(x))/2 computed using the
    // splitexp and scaleDouble functions as for exp_amd().
    //
    // abs(x) < small_threshold:
    // compute p = SPIRV_OCL_BUILTIN(exp, _f64, )(y) - 1 and then z = 0.5*(p+(p/(p+1.0)))
    // cosh(x) is then sign(x)*z.

    // This is ln(2^1025)
    const double max_cosh_arg = 7.10475860073943977113e+02;      // 0x408633ce8fb9f87e

    // This is where SPIRV_OCL_BUILTIN(exp, _f64, )(-x) is insignificant compared to SPIRV_OCL_BUILTIN(exp, _f64, )(x) = ln(2^27)
    const double small_threshold = 0x1.2b708872320e2p+4;

    double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    // In this range we find the integer part y0 of y
    // and the increment dy = y - y0. We then compute
    // z = cosh(y) = cosh(y0)cosh(dy) + sinh(y0)sinh(dy)
    // where sinh(y0) and cosh(y0) are tabulated above.

    int ind = min((int)y, 36);
    double dy = y - ind;
    double dy2 = dy * dy;

    double sdy = dy * dy2 *
             SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
             SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
             SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                 SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                 SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                     SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2, 0.7746188980094184251527126e-12, 0.160576793121939886190847e-9),
                     0.250521176994133472333666e-7),
                 0.275573191913636406057211e-5),
                 0.198412698413242405162014e-3),
             0.833333333333329931873097e-2),
             0.166666666666666667013899e0);

    double cdy = dy2 * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dy2, 0.1163921388172173692062032e-10, 0.208744349831471353536305e-8),
                       0.275573350756016588011357e-6),
                       0.248015872460622433115785e-4),
                   0.138888888889814854814536e-2),
                   0.416666666666660876512776e-1),
               0.500000000000000005911074e0);

    // At this point sinh(dy) is approximated by dy + sdy,
    // and cosh(dy) is approximated by 1 + cdy.
    double2 tv = USE_TABLE(cosh_tbl, ind);
    double cl = tv.s0;
    double ct = tv.s1;
    tv = USE_TABLE(sinh_tbl, ind);
    double sl = tv.s0;
    double st = tv.s1;

    double z = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(sl, dy, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(sl, sdy, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(cl, cdy, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(st, dy, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(st, sdy, ct*cdy)) + ct))) + cl;

    // Other cases
    z = y < 0x1.0p-28 ? 1.0 : z;

    double t = SPIRV_OCL_BUILTIN(exp, _f64, )(y - 0x1.62e42fefa3800p-1);
    t =  SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(t, -0x1.ef35793c76641p-45, t);
    z = y >= small_threshold ? t : z;

    z = y >= max_cosh_arg ? as_double(PINFBITPATT_DP64) : z;

    z = __intel_relaxed_isinf(x) | __intel_relaxed_isnan(x) ? y : z;

    return z;

}

/*################################## libclc_cospi_f64 ##############################################*/

INLINE double libclc_cospi_f64(double x) {

    long ix = as_long(x) & 0x7fffffffffffffffL;
    double ax = as_double(ix);
    long iax = (long)ax;
    double r = ax - (double)iax;
    long xodd = iax & 0x1L ? 0x8000000000000000L : 0L;

    // Initialize with return for +-Inf and NaN
    long ir = 0x7ff8000000000000L;

    // 2^53 <= |x| < Inf, the result is always even integer
    ir = ix < 0x7ff0000000000000 ? 0x3ff0000000000000L : ir;

    // 2^52 <= |x| < 2^53, the result is always integer
    ir = ax < 0x1.0p+53 ? xodd | 0x3ff0000000000000L : ir;

    // 0x1.0p-7 <= |x| < 2^52, result depends on which 0.25 interval

    // r < 1.0
    double a = 1.0 - r;
    int e = 1;
    long s = xodd ^ 0x8000000000000000L;

    // r <= 0.75
    int c = r <= 0.75;
    double t = r - 0.5;
    a = c ? t : a;
    e = c ? 0 : e;

    // r < 0.5
    c = r < 0.5;
    t = 0.5 - r;
    a = c ? t : a;
    s = c ? xodd : s;

    // r <= 0.25
    c = r <= 0.25;
    a = c ? r : a;
    e = c ? 1 : e;

    double2 sc = __clc_sincos_piby4(a * M_PI, 0.0);
    long jr = s ^ as_long(e ? sc.hi : sc.lo);

    ir = ax < 0x1.0p+52 ? jr : ir;

    return as_double(ir);
}

/*################################## libclc_exp_f64 ##############################################*/

INLINE double libclc_exp_f64(double x) {

    const double X_MIN = -0x1.74910d52d3051p+9; // -1075*ln(2)
    const double X_MAX = 0x1.62e42fefa39efp+9; // 1024*ln(2)
    const double R_64_BY_LOG2 = 0x1.71547652b82fep+6; // 64/ln(2)
    const double R_LOG2_BY_64_LD = 0x1.62e42fefa0000p-7; // head ln(2)/64
    const double R_LOG2_BY_64_TL = 0x1.cf79abc9e3b39p-46; // tail ln(2)/64

    int n = (int)(x * R_64_BY_LOG2);
    double r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-R_LOG2_BY_64_TL, (double)n, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-R_LOG2_BY_64_LD, (double)n, x));
    return __clc_exp_helper(x, X_MIN, X_MAX, r, n);
}

/*################################## libclc_exp2_f64 ##############################################*/

INLINE double libclc_exp2_f64(double x) {
    const double R_LN2 = 0x1.62e42fefa39efp-1; // ln(2)
    const double R_1_BY_64 = 1.0 / 64.0;

    int n = (int)(x * 64.0);
    double r = R_LN2 * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(-R_1_BY_64, (double)n, x);
    return __clc_exp_helper(x, -1074.0, 1024.0, r, n);
}

/*################################## libclc_exp10_f64 ##############################################*/

INLINE double libclc_exp10_f64(double val) {
    // exp10(x) = SPIRV_OCL_BUILTIN(exp2, _f64, )(x * SPIRV_OCL_BUILTIN(log2, _f64, )(10))
    return SPIRV_OCL_BUILTIN(exp2, _f64, )(val * SPIRV_OCL_BUILTIN(log2, _f64, )(10.0));
}

/*################################## libclc_expm1_f64 ##############################################*/

INLINE double libclc_expm1_f64(double x) {
    const double max_expm1_arg = 709.8;
    const double min_expm1_arg = -37.42994775023704;
    const double log_OnePlus_OneByFour = 0.22314355131420976;   //0x3FCC8FF7C79A9A22 = log(1+1/4)
    const double log_OneMinus_OneByFour = -0.28768207245178096; //0xBFD269621134DB93 = log(1-1/4)
    const double sixtyfour_by_lnof2 = 92.33248261689366;        //0x40571547652b82fe
    const double lnof2_by_64_head = 0.010830424696223417;       //0x3f862e42fefa0000
    const double lnof2_by_64_tail = 2.5728046223276688e-14;     //0x3d1cf79abc9e3b39

    // First, assume log(1-1/4) < x < log(1+1/4) i.e  -0.28768 < x < 0.22314
    double u = as_double(as_ulong(x) & 0xffffffffff000000UL);
    double v = x - u;
    double y = u * u * 0.5;
    double z = v * (x + u) * 0.5;

    double q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
               SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
               SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
               SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,
                       SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(x,2.4360682937111612e-8, 2.7582184028154370e-7),
                       2.7558212415361945e-6),
                       2.4801576918453420e-5),
                   1.9841269447671544e-4),
                   1.3888888890687830e-3),
               8.3333333334012270e-3),
               4.1666666666665560e-2),
           1.6666666666666632e-1);
    q *= x * x * x;

    double z1g = (u + y) + (q + (v + z));
    double z1 = x + (y + (q + z));
    z1 = y >= 0x1.0p-7 ? z1g : z1;

    // Now assume outside interval around 0
    int n = (int)(x * sixtyfour_by_lnof2);
    int j = n & 0x3f;
    int m = n >> 6;

    double2 tv = USE_TABLE(two_to_jby64_ep_tbl, j);
    double f1 = tv.s0;
    double f2 = tv.s1;
    double f = f1 + f2;

    double dn = -n;
    double r = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dn, lnof2_by_64_tail, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dn, lnof2_by_64_head, x));

    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
        SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r,
            SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r, 1.38889490863777199667e-03, 8.33336798434219616221e-03),
            4.16666666662260795726e-02),
        1.66666666665260878863e-01),
         5.00000000000000008883e-01);
    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r*r, q, r);

    double twopm = as_double((long)(m + EXPBIAS_DP64) << EXPSHIFTBITS_DP64);
    double twopmm = as_double((long)(EXPBIAS_DP64 - m) << EXPSHIFTBITS_DP64);

    // Computations for m > 52, including where result is close to Inf
    ulong uval = as_ulong(0x1.0p+1023 * (f1 + (f * q + (f2))));
    int e = (int)(uval >> EXPSHIFTBITS_DP64) + 1;

    double zme1024 = as_double(((long)e << EXPSHIFTBITS_DP64) | (uval & MANTBITS_DP64));
    zme1024 = e == 2047 ? as_double(PINFBITPATT_DP64) : zme1024;

    double zmg52 = twopm * (f1 + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f, q, f2 - twopmm));
    zmg52 = m == 1024 ? zme1024 : zmg52;

    // For m < 53
    double zml53 = twopm * ((f1 - twopmm) + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f1, q, f2*(1.0 + q)));

    // For m < -7
    double zmln7 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(twopm,  f1 + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f, q, f2), -1.0);

    z = m < 53 ? zml53 : zmg52;
    z = m < -7 ? zmln7 : z;
    z = x > log_OneMinus_OneByFour & x < log_OnePlus_OneByFour ? z1 : z;
    z = x > max_expm1_arg ? as_double(PINFBITPATT_DP64) : z;
    z = x < min_expm1_arg ? -1.0 : z;

    return z;
}

/*################################## libclc_log_f64 ##############################################*/

INLINE double libclc_log_f64(double x)
{
    // log2_lead and log2_tail sum to an extra-precise version of ln(2)
    const double log2_lead = 6.93147122859954833984e-01; /* 0x3fe62e42e0000000 */
    const double log2_tail = 5.76999904754328540596e-08; /* 0x3e6efa39ef35793c */

    // log_thresh1 = 9.39412117004394531250e-1 = 0x3fee0faa00000000
    // log_thresh2 = 1.06449508666992187500 = 0x3ff1082c00000000
    const double log_thresh1 = 0x1.e0faap-1;
    const double log_thresh2 = 0x1.1082cp+0;

    int is_near = x >= log_thresh1 & x <= log_thresh2;

    // Near 1 code
    double r = x - 1.0;
    double u = r / (2.0 + r);
    double correction = r * u;
    u = u + u;
    double v = u * u;
    double r1 = r;

    const double ca_1 = 8.33333333333317923934e-02; /* 0x3fb55555555554e6 */
    const double ca_2 = 1.25000000037717509602e-02; /* 0x3f89999999bac6d4 */
    const double ca_3 = 2.23213998791944806202e-03; /* 0x3f62492307f1519f */
    const double ca_4 = 4.34887777707614552256e-04; /* 0x3f3c8034c85dfff0 */

    double r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(u*v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, ca_4, ca_3), ca_2), ca_1), -correction);

    double ret_near = r1 + r2;

    // This is the far from 1 code

    // Deal with subnormal
    ulong ux = as_ulong(x);
    ulong uxs = as_ulong(as_double(0x03d0000000000000UL | ux) - 0x1.0p-962);
    int c = ux < IMPBIT_DP64;
    ux = c ? uxs : ux;
    int expadjust = c ? 60 : 0;

    int xexp = ((as_int2(ux).hi >> 20) & 0x7ff) - EXPBIAS_DP64 - expadjust;
    double f = as_double(HALFEXPBITS_DP64 | (ux & MANTBITS_DP64));
    int index = as_int2(ux).hi >> 13;
    index = ((0x80 | (index & 0x7e)) >> 1) + (index & 0x1);

    double2 tv = USE_TABLE(ln_tbl, index - 64);
    double z1 = tv.s0;
    double q = tv.s1;

    double f1 = index * 0x1.0p-7;
    double f2 = f - f1;
    u = f2 / SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(f2, 0.5, f1);
    v = u * u;

    const double cb_1 = 8.33333333333333593622e-02; /* 0x3fb5555555555557 */
    const double cb_2 = 1.24999999978138668903e-02; /* 0x3f89999999865ede */
    const double cb_3 = 2.23219810758559851206e-03; /* 0x3f6249423bd94741 */

    double poly = v * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, cb_3, cb_2), cb_1);
    double z2 = q + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(u, poly, u);

    double dxexp = (double)xexp;

    r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_lead, z1);
    r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_tail, z2);
    double ret_far = r1 + r2;
    double ret = is_near ? ret_near : ret_far;

    ret = SPIRV_BUILTIN(IsInf, _f64, )(x) ? as_double(PINFBITPATT_DP64) : ret;
    ret = SPIRV_BUILTIN(IsNan, _f64, )(x) | (x < 0.0) ? as_double(QNANBITPATT_DP64) : ret;
    ret = x == 0.0 ? as_double(NINFBITPATT_DP64) : ret;
    return ret;
}

/*################################## libclc_log1p_f64 ##############################################*/

INLINE double libclc_log1p_f64(double x)
{
    // Computes natural log(1+x). Algorithm based on:
    // Ping-Tak Peter Tang
    // "Table-driven implementation of the logarithm function in IEEE
    // floating-point arithmetic"
    // ACM Transactions on Mathematical Software (TOMS)
    // Volume 16, Issue 4 (December 1990)
    // Note that we use a lookup table of size 64 rather than 128,
    // and compensate by having extra terms in the minimax polynomial
    // for the kernel approximation.

    // Process Inside the threshold now
    ulong ux = as_ulong(1.0 + x);
    int xexp = ((as_int2(ux).hi >> 20) & 0x7ff) - EXPBIAS_DP64;
    double f = as_double(ONEEXPBITS_DP64 | (ux & MANTBITS_DP64));

    int j = as_int2(ux).hi >> 13;
    j = ((0x80 | (j & 0x7e)) >> 1) + (j & 0x1);
    double f1 = (double)j * 0x1.0p-6;
    j -= 64;

    double f2temp = f - f1;
    double m2 = as_double(convert_ulong(0x3ff - xexp) << EXPSHIFTBITS_DP64);
    double f2l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(m2, x, m2 - f1);
    double f2g = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(m2, x, -f1) + m2;
    double f2 = xexp <= MANTLENGTH_DP64-1 ? f2l : f2g;
    f2 = (xexp <= -2) | (xexp >= MANTLENGTH_DP64+8) ? f2temp : f2;

    double2 tv = USE_TABLE(ln_tbl, j);
    double z1 = tv.s0;
    double q = tv.s1;

    double u = MATH_DIVIDE(f2, SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(0.5, f2, f1));
    double v = u * u;

    double poly = v * SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v,
                          SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, 2.23219810758559851206e-03, 1.24999999978138668903e-02),
                          8.33333333333333593622e-02);

    // log2_lead and log2_tail sum to an extra-precise version of log(2)
    const double log2_lead = 6.93147122859954833984e-01; /* 0x3fe62e42e0000000 */
    const double log2_tail = 5.76999904754328540596e-08; /* 0x3e6efa39ef35793c */

    double z2 = q + SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(u, poly, u);
    double dxexp = (double)xexp;
    double r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_lead, z1);
    double r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(dxexp, log2_tail, z2);
    double result1 = r1 + r2;

    // Process Outside the threshold now
    double r = x;
    u = r / (2.0 + r);
    double correction = r * u;
    u = u + u;
    v = u * u;
    r1 = r;

    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v,
               SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v,
                   SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(v, 4.34887777707614552256e-04, 2.23213998791944806202e-03),
                   1.25000000037717509602e-02),
               8.33333333333317923934e-02);

    r2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(u*v, poly, -correction);

    // The values SPIRV_OCL_BUILTIN(exp, _f64, )(-1/16)-1 and SPIRV_OCL_BUILTIN(exp, _f64, )(1/16)-1
    const double log1p_thresh1 = -0x1.f0540438fd5c3p-5;
    const double log1p_thresh2 =  0x1.082b577d34ed8p-4;
    double result2 = r1 + r2;
    result2 = x < log1p_thresh1 | x > log1p_thresh2 ? result1 : result2;

    result2 = __intel_relaxed_isinf(x) ? x : result2;
    result2 = x < -1.0 ? as_double(QNANBITPATT_DP64) : result2;
    result2 = x == -1.0 ? as_double(NINFBITPATT_DP64) : result2;
    return result2;
}

/*################################## libclc_sin_f64 #####################################################*/

INLINE double libclc_sin_f64(double x) {
    double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(x);

    double r, rr;
    int regn;

    //if (y < 0x1.0p+47)
    //    __clc_remainder_piby2_medium(y, &r, &rr, &regn);
    //else
    __clc_remainder_piby2_large(y, &r, &rr, &regn);

    double2 sc = __clc_sincos_piby4(r, rr);

    int2 s = as_int2(regn & 1 ? sc.hi : sc.lo);
    s.hi ^= ((regn > 1) << 31) ^ ((x < 0.0) << 31);

    return  SPIRV_BUILTIN(IsNan, _f64, )( x ) | SPIRV_BUILTIN(IsInf, _f64, )( x ) ? as_double(QNANBITPATT_DP64) : as_double(s);
}

/*################################## libclc_sinpi_f64 #####################################################*/

INLINE double libclc_sinpi_f64(double x)
{
    long ix = as_long(x);
    long xsgn = ix & 0x8000000000000000L;
    ix ^= xsgn;
    double ax = as_double(ix);
    long iax = (long)ax;
    double r = ax - (double)iax;
    long xodd = xsgn ^ (iax & 0x1L ? 0x8000000000000000L : 0L);

    // Initialize with return for +-Inf and NaN
    long ir = 0x7ff8000000000000L;

    // 2^23 <= |x| < Inf, the result is always integer
    ir = ix < 0x7ff0000000000000 ? xsgn : ir;

    // 0x1.0p-7 <= |x| < 2^23, result depends on which 0.25 interval

    // r < 1.0
    double a = 1.0 - r;
    int e = 0;

    //  r <= 0.75
    int c = r <= 0.75;
    double t = r - 0.5;
    a = c ? t : a;
    e = c ? 1 : e;

    // r < 0.5
    c = r < 0.5;
    t = 0.5 - r;
    a = c ? t : a;

    // r <= 0.25
    c = r <= 0.25;
    a = c ? r : a;
    e = c ? 0 : e;

    double api = a * M_PI;
    double2 sc = __clc_sincos_piby4(api, 0.0);
    long jr = xodd ^ as_long(e ? sc.hi : sc.lo);

    ir = ax < 0x1.0p+52 ? jr : ir;

    return as_double(ir);
}


/*################################## libclc_tan_f64 ##############################################*/

INLINE double libclc_tan_f64(double x) {
  double sinx = SPIRV_OCL_BUILTIN(sin, _f64, )(x);
  return sinx / SPIRV_OCL_BUILTIN(sqrt, _f64, )( 1.0 - (sinx*sinx) );
}

/*################################## libclc_tanh_f64 ##############################################*/

INLINE double libclc_tanh_f64(double x)
{
    // The definition of tanh(x) is sinh(x)/cosh(x), which is also equivalent
    // to the following three formulae:
    // 1.  (SPIRV_OCL_BUILTIN(exp, _f64, )(x) - SPIRV_OCL_BUILTIN(exp, _f64, )(-x))/(SPIRV_OCL_BUILTIN(exp, _f64, )(x) + SPIRV_OCL_BUILTIN(exp, _f64, )(-x))
    // 2.  (1 - (2/(SPIRV_OCL_BUILTIN(exp, _f64, )(2*x) + 1 )))
    // 3.  (SPIRV_OCL_BUILTIN(exp, _f64, )(2*x) - 1)/(SPIRV_OCL_BUILTIN(exp, _f64, )(2*x) + 1)
    // but computationally, some formulae are better on some ranges.

    // The point at which e^-x is insignificant compared to e^x = ln(2^27)
    const double large_threshold = 0x1.2b708872320e2p+4;

    ulong ux = as_ulong(x);
    ulong ax = ux & ~SIGNBIT_DP64;
    ulong sx = ux ^ ax;
    double y = as_double(ax);
    double y2 = y * y;

    // y < 0.9
    double znl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                     SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                         SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2, -0.142077926378834722618091e-7, -0.200047621071909498730453e-3),
                         -0.176016349003044679402273e-1),
                     -0.274030424656179760118928e0);

    double zdl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                     SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                         SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2, 0.2091140262529164482568557e-3, 0.201562166026937652780575e-1),
                         0.381641414288328849317962e0),
                     0.822091273968539282568011e0);

    // 0.9 <= y <= 1
    double znm = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                     SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                         SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2, -0.115475878996143396378318e-7, -0.165597043903549960486816e-3),
                         -0.146173047288731678404066e-1),
                     -0.227793870659088295252442e0);

    double zdm = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                     SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2,
                         SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y2, 0.173076050126225961768710e-3, 0.167358775461896562588695e-1),
                         0.317204558977294374244770e0),
                     0.683381611977295894959554e0);

    int c = y < 0.9;
    double zn = c ? znl : znm;
    double zd = c ? zdl : zdm;
    double z = y + y*y2 * MATH_DIVIDE(zn, zd);

    // y > 1
    double p = SPIRV_OCL_BUILTIN(exp, _f64, )(2.0 * y) + 1.0;
    double zg = 1.0 - 2.0 / p;

    z = y > 1.0 ? zg : z;

    // Other cases
    z = y < 0x1.0p-28 | ax > PINFBITPATT_DP64 ? x : z;

    z = y > large_threshold ? 1.0 : z;

    return as_double(sx | as_ulong(z));
}


/*################################## libclc_sinh_f64 ##############################################*/

INLINE double libclc_sinh_f64(double x)
{
  return libclc_tanh_f64(x) * libclc_cosh_f64(x);
}

/*################################## libclc_tanpi_f64 ##############################################*/

INLINE double libclc_tanpi_f64(double x)
{
    return __builtin_spirv_divide_cr_f64_f64(libclc_sinpi_f64(x),libclc_cospi_f64(x));
}

/*################################## libclc_erf_f64 ###############################################*/

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* double erf(double x)
 * double erfc(double x)
 *                             x
 *                      2      |\
 *     erf(x)  =  ---------  | exp(-t*t)dt
 *                    sqrt(pi) \|
 *                             0
 *
 *     erfc(x) =  1-erf(x)
 *  Note that
 *                erf(-x) = -erf(x)
 *                erfc(-x) = 2 - erfc(x)
 *
 * Method:
 *        1. For |x| in [0, 0.84375]
 *            erf(x)  = x + x*R(x^2)
 *          erfc(x) = 1 - erf(x)           if x in [-.84375,0.25]
 *                  = 0.5 + ((0.5-x)-x*R)  if x in [0.25,0.84375]
 *           where R = P/Q where P is an odd poly of degree 8 and
 *           Q is an odd poly of degree 10.
 *                                                 -57.90
 *                        | R - (erf(x)-x)/x | <= 2
 *
 *
 *           Remark. The formula is derived by noting
 *          erf(x) = (2/sqrt(pi))*(x - x^3/3 + x^5/10 - x^7/42 + ....)
 *           and that
 *          2/sqrt(pi) = 1.128379167095512573896158903121545171688
 *           is close to one. The interval is chosen because the fix
 *           point of erf(x) is near 0.6174 (i.e., erf(x)=x when x is
 *           near 0.6174), and by some experiment, 0.84375 is chosen to
 *            guarantee the error is less than one ulp for erf.
 *
 *      2. For |x| in [0.84375,1.25], let s = |x| - 1, and
 *         c = 0.84506291151 rounded to single (24 bits)
 *                 erf(x)  = sign(x) * (c  + P1(s)/Q1(s))
 *                 erfc(x) = (1-c)  - P1(s)/Q1(s) if x > 0
 *                          1+(c+P1(s)/Q1(s))    if x < 0
 *                 |P1/Q1 - (erf(|x|)-c)| <= 2**-59.06
 *           Remark: here we use the taylor series expansion at x=1.
 *                erf(1+s) = erf(1) + s*Poly(s)
 *                         = 0.845.. + P1(s)/Q1(s)
 *           That is, we use rational approximation to approximate
 *                        erf(1+s) - (c = (single)0.84506291151)
 *           Note that |P1/Q1|< 0.078 for x in [0.84375,1.25]
 *           where
 *                P1(s) = degree 6 poly in s
 *                Q1(s) = degree 6 poly in s
 *
 *      3. For x in [1.25,1/0.35(~2.857143)],
 *                 erfc(x) = (1/x)*exp(-x*x-0.5625+R1/S1)
 *                 erf(x)  = 1 - erfc(x)
 *           where
 *                R1(z) = degree 7 poly in z, (z=1/x^2)
 *                S1(z) = degree 8 poly in z
 *
 *      4. For x in [1/0.35,28]
 *                 erfc(x) = (1/x)*exp(-x*x-0.5625+R2/S2) if x > 0
 *                        = 2.0 - (1/x)*exp(-x*x-0.5625+R2/S2) if -6<x<0
 *                        = 2.0 - tiny                (if x <= -6)
 *                 erf(x)  = sign(x)*(1.0 - erfc(x)) if x < 6, else
 *                 erf(x)  = sign(x)*(1.0 - tiny)
 *           where
 *                R2(z) = degree 6 poly in z, (z=1/x^2)
 *                S2(z) = degree 7 poly in z
 *
 *      Note1:
 *           To compute exp(-x*x-0.5625+R/S), let s be a single
 *           precision number and s := x; then
 *                -x*x = -s*s + (s-x)*(s+x)
 *                exp(-x*x-0.5626+R/S) =
 *                        exp(-s*s-0.5625)*exp((s-x)*(s+x)+R/S);
 *      Note2:
 *           Here 4 and 5 make use of the asymptotic series
 *                          exp(-x*x)
 *                erfc(x) ~ ---------- * ( 1 + Poly(1/x^2) )
 *                          x*sqrt(pi)
 *           We use rational approximation to approximate
 *              g(s)=f(1/x^2) = log(erfc(x)*x) - x*x + 0.5625
 *           Here is the error bound for R1/S1 and R2/S2
 *              |R1/S1 - f(x)|  < 2**(-62.57)
 *              |R2/S2 - f(x)|  < 2**(-61.52)
 *
 *      5. For inf > x >= 28
 *                 erf(x)  = sign(x) *(1 - tiny)  (raise inexact)
 *                 erfc(x) = tiny*tiny (raise underflow) if x > 0
 *                        = 2 - tiny if x<0
 *
 *      7. Special case:
 *                 erf(0)  = 0, erf(inf)  = 1, erf(-inf) = -1,
 *                 erfc(0) = 1, erfc(inf) = 0, erfc(-inf) = 2,
 *                   erfc/erf(NaN) is NaN
 */

#define AU0 -9.86494292470009928597e-03
#define AU1 -7.99283237680523006574e-01
#define AU2 -1.77579549177547519889e+01
#define AU3 -1.60636384855821916062e+02
#define AU4 -6.37566443368389627722e+02
#define AU5 -1.02509513161107724954e+03
#define AU6 -4.83519191608651397019e+02

#define AV1  3.03380607434824582924e+01
#define AV2  3.25792512996573918826e+02
#define AV3  1.53672958608443695994e+03
#define AV4  3.19985821950859553908e+03
#define AV5  2.55305040643316442583e+03
#define AV6  4.74528541206955367215e+02
#define AV7 -2.24409524465858183362e+01

#define BU0 -9.86494403484714822705e-03
#define BU1 -6.93858572707181764372e-01
#define BU2 -1.05586262253232909814e+01
#define BU3 -6.23753324503260060396e+01
#define BU4 -1.62396669462573470355e+02
#define BU5 -1.84605092906711035994e+02
#define BU6 -8.12874355063065934246e+01
#define BU7 -9.81432934416914548592e+00

#define BV1  1.96512716674392571292e+01
#define BV2  1.37657754143519042600e+02
#define BV3  4.34565877475229228821e+02
#define BV4  6.45387271733267880336e+02
#define BV5  4.29008140027567833386e+02
#define BV6  1.08635005541779435134e+02
#define BV7  6.57024977031928170135e+00
#define BV8 -6.04244152148580987438e-02

#define CU0 -2.36211856075265944077e-03
#define CU1  4.14856118683748331666e-01
#define CU2 -3.72207876035701323847e-01
#define CU3  3.18346619901161753674e-01
#define CU4 -1.10894694282396677476e-01
#define CU5  3.54783043256182359371e-02
#define CU6 -2.16637559486879084300e-03

#define CV1  1.06420880400844228286e-01
#define CV2  5.40397917702171048937e-01
#define CV3  7.18286544141962662868e-02
#define CV4  1.26171219808761642112e-01
#define CV5  1.36370839120290507362e-02
#define CV6  1.19844998467991074170e-02

#define DU0  1.28379167095512558561e-01
#define DU1 -3.25042107247001499370e-01
#define DU2 -2.84817495755985104766e-02
#define DU3 -5.77027029648944159157e-03
#define DU4 -2.37630166566501626084e-05

#define DV1  3.97917223959155352819e-01
#define DV2  6.50222499887672944485e-02
#define DV3  5.08130628187576562776e-03
#define DV4  1.32494738004321644526e-04
#define DV5 -3.96022827877536812320e-06

INLINE double libclc_erf_f64(double y) {
    double x = fabs(y);
    double x2 = x * x;
    double xm1 = x - 1.0;

    // Poly variable
    double t = 1.0 / x2;
    t = x < 1.25 ? xm1 : t;
    t = x < 0.84375 ? x2 : t;

    double u, ut, v, vt;

    // Evaluate rational poly
    // XXX We need to see of we can grab 16 coefficents from a table
    // faster than evaluating 3 of the poly pairs
    // if (x < 6.0)
    u = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, AU6, AU5), AU4), AU3), AU2), AU1), AU0);
    v = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, AV7, AV6), AV5), AV4), AV3), AV2), AV1);

    ut = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, BU7, BU6), BU5), BU4), BU3), BU2), BU1), BU0);
    vt = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, BV8, BV7), BV6), BV5), BV4), BV3), BV2), BV1);
    u = x < 0x1.6db6ep+1 ? ut : u;
    v = x < 0x1.6db6ep+1 ? vt : v;

    ut = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, CU6, CU5), CU4), CU3), CU2), CU1), CU0);
    vt = fma(t, fma(t, fma(t, fma(t, fma(t, CV6, CV5), CV4), CV3), CV2), CV1);
    u = x < 1.25 ? ut : u;
    v = x < 1.25 ? vt : v;

    ut = fma(t, fma(t, fma(t, fma(t, DU4, DU3), DU2), DU1), DU0);
    vt = fma(t, fma(t, fma(t, fma(t, DV5, DV4), DV3), DV2), DV1);
    u = x < 0.84375 ? ut : u;
    v = x < 0.84375 ? vt : v;

    v = fma(t, v, 1.0);

    // Compute rational approximation
    double q = u / v;

    // Compute results
    double z = as_double(as_long(x) & 0xffffffff00000000L);
    double r = exp(-z * z - 0.5625) * exp((z - x) * (z + x) + q);
    r = 1.0 - r / x;

    double ret = x < 6.0 ? r : 1.0;

    r = 8.45062911510467529297e-01 + q;
    ret = x < 1.25 ? r : ret;

    q = x < 0x1.0p-28 ? 1.28379167095512586316e-01 : q;

    r = fma(x, q, x);
    ret = x < 0.84375 ? r : ret;

    ret = isnan(x) ? x : ret;

    return y < 0.0 ? -ret : ret;
}

#undef AU0
#undef AU1
#undef AU2
#undef AU3
#undef AU4
#undef AU5
#undef AU6

#undef AV1
#undef AV2
#undef AV3
#undef AV4
#undef AV5
#undef AV6
#undef AV7

#undef BU0
#undef BU1
#undef BU2
#undef BU3
#undef BU4
#undef BU5
#undef BU6
#undef BU7

#undef BV1
#undef BV2
#undef BV3
#undef BV4
#undef BV5
#undef BV6
#undef BV7
#undef BV8

#undef CU0
#undef CU1
#undef CU2
#undef CU3
#undef CU4
#undef CU5
#undef CU6

#undef CV1
#undef CV2
#undef CV3
#undef CV4
#undef CV5
#undef CV6

#undef DU0
#undef DU1
#undef DU2
#undef DU3
#undef DU4

#undef DV1
#undef DV2
#undef DV3
#undef DV4
#undef DV5

/*################################## libclc_erfc_f64 ###############################################*/

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

 /* double erf(double x)
  * double erfc(double x)
  *                             x
  *                      2      |\
  *     erf(x)  =  ---------  | exp(-t*t)dt
  *                    sqrt(pi) \|
  *                             0
  *
  *     erfc(x) =  1-erf(x)
  *  Note that
  *                erf(-x) = -erf(x)
  *                erfc(-x) = 2 - erfc(x)
  *
  * Method:
  *        1. For |x| in [0, 0.84375]
  *            erf(x)  = x + x*R(x^2)
  *          erfc(x) = 1 - erf(x)           if x in [-.84375,0.25]
  *                  = 0.5 + ((0.5-x)-x*R)  if x in [0.25,0.84375]
  *           where R = P/Q where P is an odd poly of degree 8 and
  *           Q is an odd poly of degree 10.
  *                                                 -57.90
  *                        | R - (erf(x)-x)/x | <= 2
  *
  *
  *           Remark. The formula is derived by noting
  *          erf(x) = (2/sqrt(pi))*(x - x^3/3 + x^5/10 - x^7/42 + ....)
  *           and that
  *          2/sqrt(pi) = 1.128379167095512573896158903121545171688
  *           is close to one. The interval is chosen because the fix
  *           point of erf(x) is near 0.6174 (i.e., erf(x)=x when x is
  *           near 0.6174), and by some experiment, 0.84375 is chosen to
  *            guarantee the error is less than one ulp for erf.
  *
  *      2. For |x| in [0.84375,1.25], let s = |x| - 1, and
  *         c = 0.84506291151 rounded to single (24 bits)
  *                 erf(x)  = sign(x) * (c  + P1(s)/Q1(s))
  *                 erfc(x) = (1-c)  - P1(s)/Q1(s) if x > 0
  *                          1+(c+P1(s)/Q1(s))    if x < 0
  *                 |P1/Q1 - (erf(|x|)-c)| <= 2**-59.06
  *           Remark: here we use the taylor series expansion at x=1.
  *                erf(1+s) = erf(1) + s*Poly(s)
  *                         = 0.845.. + P1(s)/Q1(s)
  *           That is, we use rational approximation to approximate
  *                        erf(1+s) - (c = (single)0.84506291151)
  *           Note that |P1/Q1|< 0.078 for x in [0.84375,1.25]
  *           where
  *                P1(s) = degree 6 poly in s
  *                Q1(s) = degree 6 poly in s
  *
  *      3. For x in [1.25,1/0.35(~2.857143)],
  *                 erfc(x) = (1/x)*exp(-x*x-0.5625+R1/S1)
  *                 erf(x)  = 1 - erfc(x)
  *           where
  *                R1(z) = degree 7 poly in z, (z=1/x^2)
  *                S1(z) = degree 8 poly in z
  *
  *      4. For x in [1/0.35,28]
  *                 erfc(x) = (1/x)*exp(-x*x-0.5625+R2/S2) if x > 0
  *                        = 2.0 - (1/x)*exp(-x*x-0.5625+R2/S2) if -6<x<0
  *                        = 2.0 - tiny                (if x <= -6)
  *                 erf(x)  = sign(x)*(1.0 - erfc(x)) if x < 6, else
  *                 erf(x)  = sign(x)*(1.0 - tiny)
  *           where
  *                R2(z) = degree 6 poly in z, (z=1/x^2)
  *                S2(z) = degree 7 poly in z
  *
  *      Note1:
  *           To compute exp(-x*x-0.5625+R/S), let s be a single
  *           precision number and s := x; then
  *                -x*x = -s*s + (s-x)*(s+x)
  *                exp(-x*x-0.5626+R/S) =
  *                        exp(-s*s-0.5625)*exp((s-x)*(s+x)+R/S);
  *      Note2:
  *           Here 4 and 5 make use of the asymptotic series
  *                          exp(-x*x)
  *                erfc(x) ~ ---------- * ( 1 + Poly(1/x^2) )
  *                          x*sqrt(pi)
  *           We use rational approximation to approximate
  *              g(s)=f(1/x^2) = log(erfc(x)*x) - x*x + 0.5625
  *           Here is the error bound for R1/S1 and R2/S2
  *              |R1/S1 - f(x)|  < 2**(-62.57)
  *              |R2/S2 - f(x)|  < 2**(-61.52)
  *
  *      5. For inf > x >= 28
  *                 erf(x)  = sign(x) *(1 - tiny)  (raise inexact)
  *                 erfc(x) = tiny*tiny (raise underflow) if x > 0
  *                        = 2 - tiny if x<0
  *
  *      7. Special case:
  *                 erf(0)  = 0, erf(inf)  = 1, erf(-inf) = -1,
  *                 erfc(0) = 1, erfc(inf) = 0, erfc(-inf) = 2,
  *                   erfc/erf(NaN) is NaN
  */

#define AU0 -9.86494292470009928597e-03
#define AU1 -7.99283237680523006574e-01
#define AU2 -1.77579549177547519889e+01
#define AU3 -1.60636384855821916062e+02
#define AU4 -6.37566443368389627722e+02
#define AU5 -1.02509513161107724954e+03
#define AU6 -4.83519191608651397019e+02

#define AV0  3.03380607434824582924e+01
#define AV1  3.25792512996573918826e+02
#define AV2  1.53672958608443695994e+03
#define AV3  3.19985821950859553908e+03
#define AV4  2.55305040643316442583e+03
#define AV5  4.74528541206955367215e+02
#define AV6 -2.24409524465858183362e+01

#define BU0 -9.86494403484714822705e-03
#define BU1 -6.93858572707181764372e-01
#define BU2 -1.05586262253232909814e+01
#define BU3 -6.23753324503260060396e+01
#define BU4 -1.62396669462573470355e+02
#define BU5 -1.84605092906711035994e+02
#define BU6 -8.12874355063065934246e+01
#define BU7 -9.81432934416914548592e+00

#define BV0  1.96512716674392571292e+01
#define BV1  1.37657754143519042600e+02
#define BV2  4.34565877475229228821e+02
#define BV3  6.45387271733267880336e+02
#define BV4  4.29008140027567833386e+02
#define BV5  1.08635005541779435134e+02
#define BV6  6.57024977031928170135e+00
#define BV7 -6.04244152148580987438e-02

#define CU0 -2.36211856075265944077e-03
#define CU1  4.14856118683748331666e-01
#define CU2 -3.72207876035701323847e-01
#define CU3  3.18346619901161753674e-01
#define CU4 -1.10894694282396677476e-01
#define CU5  3.54783043256182359371e-02
#define CU6 -2.16637559486879084300e-03

#define CV0 1.06420880400844228286e-01
#define CV1 5.40397917702171048937e-01
#define CV2 7.18286544141962662868e-02
#define CV3 1.26171219808761642112e-01
#define CV4 1.36370839120290507362e-02
#define CV5 1.19844998467991074170e-02

#define DU0  1.28379167095512558561e-01
#define DU1 -3.25042107247001499370e-01
#define DU2 -2.84817495755985104766e-02
#define DU3 -5.77027029648944159157e-03
#define DU4 -2.37630166566501626084e-05

#define DV0  3.97917223959155352819e-01
#define DV1  6.50222499887672944485e-02
#define DV2  5.08130628187576562776e-03
#define DV3  1.32494738004321644526e-04
#define DV4 -3.96022827877536812320e-06

INLINE double libclc_erfc_f64(double x) {
    long lx = as_long(x);
    long ax = lx & 0x7fffffffffffffffL;
    double absx = as_double(ax);
    int xneg = lx != ax;

    // Poly arg
    double x2 = x * x;
    double xm1 = absx - 1.0;
    double t = 1.0 / x2;
    t = absx < 1.25 ? xm1 : t;
    t = absx < 0.84375 ? x2 : t;


    // Evaluate rational poly
    // XXX Need to evaluate if we can grab the 14 coefficients from a
    // table faster than evaluating 3 pairs of polys
    double tu, tv, u, v;

    // |x| < 28
    u = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, AU6, AU5), AU4), AU3), AU2), AU1), AU0);
    v = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, AV6, AV5), AV4), AV3), AV2), AV1), AV0);

    tu = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, BU7, BU6), BU5), BU4), BU3), BU2), BU1), BU0);
    tv = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, BV7, BV6), BV5), BV4), BV3), BV2), BV1), BV0);
    u = absx < 0x1.6db6dp+1 ? tu : u;
    v = absx < 0x1.6db6dp+1 ? tv : v;

    tu = fma(t, fma(t, fma(t, fma(t, fma(t, fma(t, CU6, CU5), CU4), CU3), CU2), CU1), CU0);
    tv = fma(t, fma(t, fma(t, fma(t, fma(t, CV5, CV4), CV3), CV2), CV1), CV0);
    u = absx < 1.25 ? tu : u;
    v = absx < 1.25 ? tv : v;

    tu = fma(t, fma(t, fma(t, fma(t, DU4, DU3), DU2), DU1), DU0);
    tv = fma(t, fma(t, fma(t, fma(t, DV4, DV3), DV2), DV1), DV0);
    u = absx < 0.84375 ? tu : u;
    v = absx < 0.84375 ? tv : v;

    v = fma(t, v, 1.0);
    double q = u / v;


    // Evaluate return value

    // |x| < 28
    double z = as_double(ax & 0xffffffff00000000UL);
    double ret = exp(-z * z - 0.5625) * exp((z - absx) * (z + absx) + q) / absx;
    t = 2.0 - ret;
    ret = xneg ? t : ret;

    const double erx = 8.45062911510467529297e-01;
    z = erx + q + 1.0;
    t = 1.0 - erx - q;
    t = xneg ? z : t;
    ret = absx < 1.25 ? t : ret;

    // z = 1.0 - fma(x, q, x);
    // t = 0.5 - fma(x, q, x - 0.5);
    // t = xneg == 1 | absx < 0.25 ? z : t;
    t = fma(-x, q, 1.0 - x);
    ret = absx < 0.84375 ? t : ret;

    ret = x >= 28.0 ? 0.0 : ret;
    ret = x <= -6.0 ? 2.0 : ret;
    ret = ax > 0x7ff0000000000000UL ? x : ret;

    return ret;
}

#undef AU0
#undef AU1
#undef AU2
#undef AU3
#undef AU4
#undef AU5
#undef AU6

#undef AV0
#undef AV1
#undef AV2
#undef AV3
#undef AV4
#undef AV5
#undef AV6

#undef BU0
#undef BU1
#undef BU2
#undef BU3
#undef BU4
#undef BU5
#undef BU6
#undef BU7

#undef BV0
#undef BV1
#undef BV2
#undef BV3
#undef BV4
#undef BV5
#undef BV6
#undef BV7

#undef CU0
#undef CU1
#undef CU2
#undef CU3
#undef CU4
#undef CU5
#undef CU6

#undef CV0
#undef CV1
#undef CV2
#undef CV3
#undef CV4
#undef CV5

#undef DU0
#undef DU1
#undef DU2
#undef DU3
#undef DU4

#undef DV0
#undef DV1
#undef DV2
#undef DV3
#undef DV4

/*################################## libclc_lgamma_r_f64 ###############################################*/

// ====================================================
// Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//
// Developed at SunPro, a Sun Microsystems, Inc. business.
// Permission to use, copy, modify, and distribute this
// software is freely granted, provided that this notice
// is preserved.
// ====================================================

// lgamma_r(x, i)
// Reentrant version of the logarithm of the Gamma function
// with user provide pointer for the sign of Gamma(x).
//
// Method:
//   1. Argument Reduction for 0 < x <= 8
//      Since gamma(1+s)=s*gamma(s), for x in [0,8], we may
//      reduce x to a number in [1.5,2.5] by
//              lgamma(1+s) = log(s) + lgamma(s)
//      for example,
//              lgamma(7.3) = log(6.3) + lgamma(6.3)
//                          = log(6.3*5.3) + lgamma(5.3)
//                          = log(6.3*5.3*4.3*3.3*2.3) + lgamma(2.3)
//   2. Polynomial approximation of lgamma around its
//      minimun ymin=1.461632144968362245 to maintain monotonicity.
//      On [ymin-0.23, ymin+0.27] (i.e., [1.23164,1.73163]), use
//              Let z = x-ymin;
//              lgamma(x) = -1.214862905358496078218 + z^2*poly(z)
//      where
//              poly(z) is a 14 degree polynomial.
//   2. Rational approximation in the primary interval [2,3]
//      We use the following approximation:
//              s = x-2.0;
//              lgamma(x) = 0.5*s + s*P(s)/Q(s)
//      with accuracy
//              |P/Q - (lgamma(x)-0.5s)| < 2**-61.71
//      Our algorithms are based on the following observation
//
//                             zeta(2)-1    2    zeta(3)-1    3
// lgamma(2+s) = s*(1-Euler) + --------- * s  -  --------- * s  + ...
//                                 2                 3
//
//      where Euler = 0.5771... is the Euler constant, which is very
//      close to 0.5.
//
//   3. For x>=8, we have
//      lgamma(x)~(x-0.5)log(x)-x+0.5*log(2pi)+1/(12x)-1/(360x**3)+....
//      (better formula:
//         lgamma(x)~(x-0.5)*(log(x)-1)-.5*(log(2pi)-1) + ...)
//      Let z = 1/x, then we approximation
//              f(z) = lgamma(x) - (x-0.5)(log(x)-1)
//      by
//                                  3       5             11
//              w = w0 + w1*z + w2*z  + w3*z  + ... + w6*z
//      where
//              |w - f(z)| < 2**-58.74
//
//   4. For negative x, since (G is gamma function)
//              -x*G(-x)*G(x) = pi/sin(pi*x),
//      we have
//              G(x) = pi/(sin(pi*x)*(-x)*G(-x))
//      since G(-x) is positive, sign(G(x)) = sign(sin(pi*x)) for x<0
//      Hence, for x<0, signgam = sign(sin(pi*x)) and
//              lgamma(x) = log(|Gamma(x)|)
//                        = log(pi/(|x*sin(pi*x)|)) - lgamma(-x);
//      Note: one should avoid compute pi*(-x) directly in the
//            computation of sin(pi*(-x)).
//
//   5. Special Cases
//              lgamma(2+s) ~ s*(1-Euler) for tiny s
//              lgamma(1)=lgamma(2)=0
//              lgamma(x) ~ -log(x) for tiny x
//              lgamma(0) = lgamma(inf) = inf
//              lgamma(-integer) = +-inf
//
#define PI 3.14159265358979311600e+00   /* 0x400921FB, 0x54442D18 */

#define A0 7.72156649015328655494e-02    /* 0x3FB3C467, 0xE37DB0C8 */
#define A1 3.22467033424113591611e-01    /* 0x3FD4A34C, 0xC4A60FAD */
#define A2 6.73523010531292681824e-02    /* 0x3FB13E00, 0x1A5562A7 */
#define A3 2.05808084325167332806e-02    /* 0x3F951322, 0xAC92547B */
#define A4 7.38555086081402883957e-03    /* 0x3F7E404F, 0xB68FEFE8 */
#define A5 2.89051383673415629091e-03    /* 0x3F67ADD8, 0xCCB7926B */
#define A6 1.19270763183362067845e-03    /* 0x3F538A94, 0x116F3F5D */
#define A7 5.10069792153511336608e-04    /* 0x3F40B6C6, 0x89B99C00 */
#define A8 2.20862790713908385557e-04    /* 0x3F2CF2EC, 0xED10E54D */
#define A9 1.08011567247583939954e-04    /* 0x3F1C5088, 0x987DFB07 */
#define A10 2.52144565451257326939e-05    /* 0x3EFA7074, 0x428CFA52 */
#define A11 4.48640949618915160150e-05    /* 0x3F07858E, 0x90A45837 */

#define TC 1.46163214496836224576e+00    /* 0x3FF762D8, 0x6356BE3F */
#define TF -1.21486290535849611461e-01    /* 0xBFBF19B9, 0xBCC38A42 */
#define TT -3.63867699703950536541e-18    /* 0xBC50C7CA, 0xA48A971F */

#define T0 4.83836122723810047042e-01    /* 0x3FDEF72B, 0xC8EE38A2 */
#define T1 -1.47587722994593911752e-01    /* 0xBFC2E427, 0x8DC6C509 */
#define T2 6.46249402391333854778e-02    /* 0x3FB08B42, 0x94D5419B */
#define T3 -3.27885410759859649565e-02    /* 0xBFA0C9A8, 0xDF35B713 */
#define T4 1.79706750811820387126e-02    /* 0x3F9266E7, 0x970AF9EC */
#define T5 -1.03142241298341437450e-02    /* 0xBF851F9F, 0xBA91EC6A */
#define T6 6.10053870246291332635e-03    /* 0x3F78FCE0, 0xE370E344 */
#define T7 -3.68452016781138256760e-03    /* 0xBF6E2EFF, 0xB3E914D7 */
#define T8 2.25964780900612472250e-03    /* 0x3F6282D3, 0x2E15C915 */
#define T9 -1.40346469989232843813e-03    /* 0xBF56FE8E, 0xBF2D1AF1 */
#define T10 8.81081882437654011382e-04    /* 0x3F4CDF0C, 0xEF61A8E9 */
#define T11 -5.38595305356740546715e-04    /* 0xBF41A610, 0x9C73E0EC */
#define T12 3.15632070903625950361e-04    /* 0x3F34AF6D, 0x6C0EBBF7 */
#define T13 -3.12754168375120860518e-04    /* 0xBF347F24, 0xECC38C38 */
#define T14 3.35529192635519073543e-04    /* 0x3F35FD3E, 0xE8C2D3F4 */

#define U0 -7.72156649015328655494e-02    /* 0xBFB3C467, 0xE37DB0C8 */
#define U1 6.32827064025093366517e-01    /* 0x3FE4401E, 0x8B005DFF */
#define U2 1.45492250137234768737e+00    /* 0x3FF7475C, 0xD119BD6F */
#define U3 9.77717527963372745603e-01    /* 0x3FEF4976, 0x44EA8450 */
#define U4 2.28963728064692451092e-01    /* 0x3FCD4EAE, 0xF6010924 */
#define U5 1.33810918536787660377e-02    /* 0x3F8B678B, 0xBF2BAB09 */

#define V1 2.45597793713041134822e+00    /* 0x4003A5D7, 0xC2BD619C */
#define V2 2.12848976379893395361e+00    /* 0x40010725, 0xA42B18F5 */
#define V3 7.69285150456672783825e-01    /* 0x3FE89DFB, 0xE45050AF */
#define V4 1.04222645593369134254e-01    /* 0x3FBAAE55, 0xD6537C88 */
#define V5 3.21709242282423911810e-03    /* 0x3F6A5ABB, 0x57D0CF61 */

#define S0 -7.72156649015328655494e-02    /* 0xBFB3C467, 0xE37DB0C8 */
#define S1 2.14982415960608852501e-01    /* 0x3FCB848B, 0x36E20878 */
#define S2 3.25778796408930981787e-01    /* 0x3FD4D98F, 0x4F139F59 */
#define S3 1.46350472652464452805e-01    /* 0x3FC2BB9C, 0xBEE5F2F7 */
#define S4 2.66422703033638609560e-02    /* 0x3F9B481C, 0x7E939961 */
#define S5 1.84028451407337715652e-03    /* 0x3F5E26B6, 0x7368F239 */
#define S6 3.19475326584100867617e-05    /* 0x3F00BFEC, 0xDD17E945 */

#define R1 1.39200533467621045958e+00    /* 0x3FF645A7, 0x62C4AB74 */
#define R2 7.21935547567138069525e-01    /* 0x3FE71A18, 0x93D3DCDC */
#define R3 1.71933865632803078993e-01    /* 0x3FC601ED, 0xCCFBDF27 */
#define R4 1.86459191715652901344e-02    /* 0x3F9317EA, 0x742ED475 */
#define R5 7.77942496381893596434e-04    /* 0x3F497DDA, 0xCA41A95B */
#define R6 7.32668430744625636189e-06    /* 0x3EDEBAF7, 0xA5B38140 */

#define W0 4.18938533204672725052e-01    /* 0x3FDACFE3, 0x90C97D69 */
#define W1 8.33333333333329678849e-02    /* 0x3FB55555, 0x5555553B */
#define W2 -2.77777777728775536470e-03    /* 0xBF66C16C, 0x16B02E5C */
#define W3 7.93650558643019558500e-04    /* 0x3F4A019F, 0x98CF38B6 */
#define W4 -5.95187557450339963135e-04    /* 0xBF4380CB, 0x8C0FE741 */
#define W5 8.36339918996282139126e-04    /* 0x3F4B67BA, 0x4CDAD5D1 */
#define W6 -1.63092934096575273989e-03    /* 0xBF5AB89D, 0x0B9E43E4 */

INLINE double libclc_lgamma_r_f64(double x, private int* ip) {
    ulong ux = as_ulong(x);
    ulong ax = ux & EXSIGNBIT_DP64;
    double absx = as_double(ax);

    if (ax >= 0x7ff0000000000000UL) {
        // +-Inf, NaN
        *ip = 1;
        return absx;
    }

    if (absx < 0x1.0p-70) {
        *ip = ax == ux ? 1 : -1;
        return -log(absx);
    }

    // Handle rest of range
    double r;

    if (absx < 2.0) {
        int i = 0;
        double y = 2.0 - absx;

        int c = absx < 0x1.bb4c3p+0;
        double t = absx - TC;
        i = c ? 1 : i;
        y = c ? t : y;

        c = absx < 0x1.3b4c4p+0;
        t = absx - 1.0;
        i = c ? 2 : i;
        y = c ? t : y;

        c = absx <= 0x1.cccccp-1;
        t = -log(absx);
        r = c ? t : 0.0;
        t = 1.0 - absx;
        i = c ? 0 : i;
        y = c ? t : y;

        c = absx < 0x1.76944p-1;
        t = absx - (TC - 1.0);
        i = c ? 1 : i;
        y = c ? t : y;

        c = absx < 0x1.da661p-3;
        i = c ? 2 : i;
        y = c ? absx : y;

        double p, q;

        switch (i) {
        case 0:
            p = fma(y, fma(y, fma(y, fma(y, A11, A10), A9), A8), A7);
            p = fma(y, fma(y, fma(y, fma(y, p, A6), A5), A4), A3);
            p = fma(y, fma(y, fma(y, p, A2), A1), A0);
            r = fma(y, p - 0.5, r);
            break;
        case 1:
            p = fma(y, fma(y, fma(y, fma(y, T14, T13), T12), T11), T10);
            p = fma(y, fma(y, fma(y, fma(y, fma(y, p, T9), T8), T7), T6), T5);
            p = fma(y, fma(y, fma(y, fma(y, fma(y, p, T4), T3), T2), T1), T0);
            p = fma(y * y, p, -TT);
            r += (TF + p);
            break;
        case 2:
            p = y * fma(y, fma(y, fma(y, fma(y, fma(y, U5, U4), U3), U2), U1), U0);
            q = fma(y, fma(y, fma(y, fma(y, fma(y, V5, V4), V3), V2), V1), 1.0);
            r += fma(-0.5, y, p / q);
        }
    }
    else if (absx < 8.0) {
        int i = absx;
        double y = absx - (double)i;
        double p = y * fma(y, fma(y, fma(y, fma(y, fma(y, fma(y, S6, S5), S4), S3), S2), S1), S0);
        double q = fma(y, fma(y, fma(y, fma(y, fma(y, fma(y, R6, R5), R4), R3), R2), R1), 1.0);
        r = fma(0.5, y, p / q);
        double z = 1.0;
        // lgamma(1+s) = log(s) + lgamma(s)
        double y6 = y + 6.0;
        double y5 = y + 5.0;
        double y4 = y + 4.0;
        double y3 = y + 3.0;
        double y2 = y + 2.0;
        z *= i > 6 ? y6 : 1.0;
        z *= i > 5 ? y5 : 1.0;
        z *= i > 4 ? y4 : 1.0;
        z *= i > 3 ? y3 : 1.0;
        z *= i > 2 ? y2 : 1.0;
        r += log(z);
    }
    else {
        double z = 1.0 / absx;
        double z2 = z * z;
        double w = fma(z, fma(z2, fma(z2, fma(z2, fma(z2, fma(z2, W6, W5), W4), W3), W2), W1), W0);
        r = (absx - 0.5) * (log(absx) - 1.0) + w;
    }

    if (x < 0.0) {
        double t = sinpi(x);
        r = log(PI / fabs(t * x)) - r;
        r = t == 0.0 ? as_double(PINFBITPATT_DP64) : r;
        *ip = t < 0.0 ? -1 : 1;
    }
    else
        *ip = 1;

    return r;
}

/*################################## libclc_lgamma_f64 ###############################################*/

INLINE double libclc_lgamma_f64(double x) {
    int s;
    return libclc_lgamma_r_f64(x, &s);
}

#undef PI

#undef A0
#undef A1
#undef A2
#undef A3
#undef A4
#undef A5
#undef A6
#undef A7
#undef A8
#undef A9
#undef A10
#undef A11

#undef TC
#undef TF
#undef TT

#undef T0
#undef T1
#undef T2
#undef T3
#undef T4
#undef T5
#undef T6
#undef T7
#undef T8
#undef T9
#undef T10
#undef T11
#undef T12
#undef T13
#undef T14

#undef U0
#undef U1
#undef U2
#undef U3
#undef U4
#undef U5

#undef V1
#undef V2
#undef V3
#undef V4
#undef V5

#undef S0
#undef S1
#undef S2
#undef S3
#undef S4
#undef S5
#undef S6

#undef R1
#undef R2
#undef R3
#undef R4
#undef R5
#undef R6

#undef W0
#undef W1
#undef W2
#undef W3
#undef W4
#undef W5
#undef W6

/*################################## libclc_tgamma_f64 ###############################################*/

INLINE double libclc_tgamma_f64(double x) {
    const double pi = 3.1415926535897932384626433832795;
    double ax = fabs(x);
    double lg = libclc_lgamma_f64(ax);
    double g = exp(lg);

    if (x < 0.0) {
        double z = sinpi(x);
        g = g * ax * z;
        g = pi / g;
        g = g == 0 ? as_double(PINFBITPATT_DP64) : g;
        g = z == 0 ? as_double(QNANBITPATT_DP64) : g;
    }

    return g;
}

#endif // defined(cl_khr_fp64)

#endif // define __DOUBLES_CL__
