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

#ifndef __TRIG_CL__
#define __TRIG_CL__

float2 __libclc__sincosf_piby4(float x)
{
    // Taylor series for sin(x) is x - x^3/3! + x^5/5! - x^7/7! ...
    // = x * (1 - x^2/3! + x^4/5! - x^6/7! ...
    // = x * f(w)
    // where w = x*x and f(w) = (1 - w/3! + w^2/5! - w^3/7! ...
    // We use a minimax approximation of (f(w) - 1) / w
    // because this produces an expansion in even powers of x.

    // Taylor series for cos(x) is 1 - x^2/2! + x^4/4! - x^6/6! ...
    // = f(w)
    // where w = x*x and f(w) = (1 - w/2! + w^2/4! - w^3/6! ...
    // We use a minimax approximation of (f(w) - 1 + w/2) / (w*w)
    // because this produces an expansion in even powers of x.

    const float sc1 = -0.166666666638608441788607926e0F;
    const float sc2 =  0.833333187633086262120839299e-2F;
    const float sc3 = -0.198400874359527693921333720e-3F;
    const float sc4 =  0.272500015145584081596826911e-5F;

    const float cc1 =  0.41666666664325175238031e-1F;
    const float cc2 = -0.13888887673175665567647e-2F;
    const float cc3 =  0.24800600878112441958053e-4F;
    const float cc4 = -0.27301013343179832472841e-6F;

    float x2 = x * x;

    float2 ret;
    ret.x = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x*x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, sc4, sc3), sc2), sc1), x);
    ret.y = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2*x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, cc4, cc3), cc2), cc1), SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x2, -0.5f, 1.0f));
    return ret;
}

float __clc_sinf_piby4(float x, float y) {
    // Taylor series for sin(x) is x - x^3/3! + x^5/5! - x^7/7! ...
    // = x * (1 - x^2/3! + x^4/5! - x^6/7! ...
    // = x * f(w)
    // where w = x*x and f(w) = (1 - w/3! + w^2/5! - w^3/7! ...
    // We use a minimax approximation of (f(w) - 1) / w
    // because this produces an expansion in even powers of x.

    const float c1 = -0.1666666666e0f;
    const float c2 = 0.8333331876e-2f;
    const float c3 = -0.198400874e-3f;
    const float c4 = 0.272500015e-5f;
    const float c5 = -2.5050759689e-08f; // 0xb2d72f34
    const float c6 = 1.5896910177e-10f;  // 0x2f2ec9d3

    float z = x * x;
    float v = z * x;
    float r = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, c6, c5), c4), c3), c2);
    float ret = x - SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(v, -c1, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(y, 0.5f, -v*r), -y));

    return ret;
}

float __clc_cosf_piby4(float x, float y) {
    // Taylor series for cos(x) is 1 - x^2/2! + x^4/4! - x^6/6! ...
    // = f(w)
    // where w = x*x and f(w) = (1 - w/2! + w^2/4! - w^3/6! ...
    // We use a minimax approximation of (f(w) - 1 + w/2) / (w*w)
    // because this produces an expansion in even powers of x.

    const float c1 = 0.416666666e-1f;
    const float c2 = -0.138888876e-2f;
    const float c3 = 0.248006008e-4f;
    const float c4 = -0.2730101334e-6f;
    const float c5 = 2.0875723372e-09f;  // 0x310f74f6
    const float c6 = -1.1359647598e-11f; // 0xad47d74e

    float z = x * x;
    float r = z * SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, c6,  c5), c4), c3), c2), c1);

    // if |x| < 0.3
    float qx = 0.0f;

    int ix = as_int(x) & EXSIGNBIT_SP32;

    //  0.78125 > |x| >= 0.3
    float xby4 = as_float(ix - 0x01000000);
    qx = (ix >= 0x3e99999a) & (ix <= 0x3f480000) ? xby4 : qx;

    // x > 0.78125
    qx = ix > 0x3f480000 ? 0.28125f : qx;

    float hz = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, 0.5f, -qx);
    float a = 1.0f - qx;
    float ret = a - (hz - SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(z, r, -x*y));
    return ret;
}

#define FULL_MUL(A, B, HI, LO) \
    LO = A * B; \
    HI = mul_hi(A, B)

#define FULL_MAD(A, B, C, HI, LO) \
    LO = ((A) * (B) + (C)); \
    HI = mul_hi(A, B); \
    HI += LO < C


int __clc_argReductionLargeS(float *r, float *rr, float x)
{
    int xe = (int)(as_uint(x) >> 23) - 127;
    uint xm = 0x00800000U | (as_uint(x) & 0x7fffffU);

    // 224 bits of 2/PI: . A2F9836E 4E441529 FC2757D1 F534DDC0 DB629599 3C439041 FE5163AB
    const uint b6 = 0xA2F9836EU;
    const uint b5 = 0x4E441529U;
    const uint b4 = 0xFC2757D1U;
    const uint b3 = 0xF534DDC0U;
    const uint b2 = 0xDB629599U;
    const uint b1 = 0x3C439041U;
    const uint b0 = 0xFE5163ABU;

    uint p0, p1, p2, p3, p4, p5, p6, p7, c0, c1;

    FULL_MUL(xm, b0, c0, p0);
    FULL_MAD(xm, b1, c0, c1, p1);
    FULL_MAD(xm, b2, c1, c0, p2);
    FULL_MAD(xm, b3, c0, c1, p3);
    FULL_MAD(xm, b4, c1, c0, p4);
    FULL_MAD(xm, b5, c0, c1, p5);
    FULL_MAD(xm, b6, c1, p7, p6);

    uint fbits = 224 + 23 - xe;

    // shift amount to get 2 lsb of integer part at top 2 bits
    //   min: 25 (xe=18) max: 134 (xe=127)
    uint shift = 256U - 2 - fbits;

    // Shift by up to 134/32 = 4 words
    int c = shift > 31;
    p7 = c ? p6 : p7;
    p6 = c ? p5 : p6;
    p5 = c ? p4 : p5;
    p4 = c ? p3 : p4;
    p3 = c ? p2 : p3;
    p2 = c ? p1 : p2;
    p1 = c ? p0 : p1;
    shift -= (-c) & 32;

    c = shift > 31;
    p7 = c ? p6 : p7;
    p6 = c ? p5 : p6;
    p5 = c ? p4 : p5;
    p4 = c ? p3 : p4;
    p3 = c ? p2 : p3;
    p2 = c ? p1 : p2;
    shift -= (-c) & 32;

    c = shift > 31;
    p7 = c ? p6 : p7;
    p6 = c ? p5 : p6;
    p5 = c ? p4 : p5;
    p4 = c ? p3 : p4;
    p3 = c ? p2 : p3;
    shift -= (-c) & 32;

    c = shift > 31;
    p7 = c ? p6 : p7;
    p6 = c ? p5 : p6;
    p5 = c ? p4 : p5;
    p4 = c ? p3 : p4;
    shift -= (-c) & 32;

    #define bitalign(hi, lo, shift) \
    ((hi) << (32 - (shift))) | ((lo) >> (shift));

    // bitalign cannot handle a shift of 32
    c = shift > 0;
    shift = 32 - shift;
    uint t7 = bitalign(p7, p6, shift);
    uint t6 = bitalign(p6, p5, shift);
    uint t5 = bitalign(p5, p4, shift);
    p7 = c ? t7 : p7;
    p6 = c ? t6 : p6;
    p5 = c ? t5 : p5;

    // Get 2 lsb of int part and msb of fraction
    int i = p7 >> 29;

    // Scoot up 2 more bits so only fraction remains
    p7 = bitalign(p7, p6, 30);
    p6 = bitalign(p6, p5, 30);
    p5 = bitalign(p5, p4, 30);

    // Subtract 1 if msb of fraction is 1, i.e. fraction >= 0.5
    uint flip = i & 1 ? 0xffffffffU : 0U;
    uint sign = i & 1 ? 0x80000000U : 0U;
    p7 = p7 ^ flip;
    p6 = p6 ^ flip;
    p5 = p5 ^ flip;

    // Find exponent and shift away leading zeroes and hidden bit
    xe = SPIRV_OCL_BUILTIN(clz, _i32, )((int)p7) + 1;
    shift = 32 - xe;
    p7 = bitalign(p7, p6, shift);
    p6 = bitalign(p6, p5, shift);

    // Most significant part of fraction
    float q1 = as_float(sign | ((127 - xe) << 23) | (p7 >> 9));

    // Shift out bits we captured on q1
    p7 = bitalign(p7, p6, 32-23);

    // Get 24 more bits of fraction in another float, there are not long strings of zeroes here
    int xxe = SPIRV_OCL_BUILTIN(clz, _i32, )((int)p7) + 1;
    p7 = bitalign(p7, p6, 32-xxe);
    float q0 = as_float(sign | ((127 - (xe + 23 + xxe)) << 23) | (p7 >> 9));

    // At this point, the fraction q1 + q0 is correct to at least 48 bits
    // Now we need to multiply the fraction by pi/2
    // This loses us about 4 bits
    // pi/2 = C90 FDA A22 168 C23 4C4

    const float pio2h = (float)0xc90fda / 0x1.0p+23f;
    const float pio2hh = (float)0xc90 / 0x1.0p+11f;
    const float pio2ht = (float)0xfda / 0x1.0p+23f;
    const float pio2t = (float)0xa22168 / 0x1.0p+47f;

    float rh, rt;

    if (HAVE_HW_FMA32()) {
        rh = q1 * pio2h;
        rt = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(q0, pio2h, SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(q1, pio2t, SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(q1, pio2h, -rh)));
    } else {
        float q1h = as_float(as_uint(q1) & 0xfffff000);
        float q1t = q1 - q1h;
        rh = q1 * pio2h;
        rt = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q1t, pio2ht, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q1t, pio2hh, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q1h, pio2ht, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q1h, pio2hh, -rh))));
        rt = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q0, pio2h, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(q1, pio2t, rt));
    }

    float t = rh + rt;
    rt = rt - (t - rh);

    *r = t;
    *rr = rt;
    return ((i >> 1) + (i & 1)) & 0x3;
}

void __clc_fullMulS(float *hi, float *lo, float a, float b, float bh, float bt)
{
    if (HAVE_HW_FMA32()) {
        float ph = a * b;
        *hi = ph;
        *lo = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(a, b, -ph);
    } else {
        float ah = as_float(as_uint(a) & 0xfffff000U);
        float at = a - ah;
        float ph = a * b;
        float pt = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(at, bt, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(at, bh, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(ah, bt, SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(ah, bh, -ph))));
        *hi = ph;
        *lo = pt;
    }
}

float __clc_removePi2S(float *hi, float *lo, float x)
{
    // 72 bits of pi/2
    const float fpiby2_1 = (float) 0xC90FDA / 0x1.0p+23f;
    const float fpiby2_1_h = (float) 0xC90 / 0x1.0p+11f;
    const float fpiby2_1_t = (float) 0xFDA / 0x1.0p+23f;

    const float fpiby2_2 = (float) 0xA22168 / 0x1.0p+47f;
    const float fpiby2_2_h = (float) 0xA22 / 0x1.0p+35f;
    const float fpiby2_2_t = (float) 0x168 / 0x1.0p+47f;

    const float fpiby2_3 = (float) 0xC234C4 / 0x1.0p+71f;
    const float fpiby2_3_h = (float) 0xC23 / 0x1.0p+59f;
    const float fpiby2_3_t = (float) 0x4C4 / 0x1.0p+71f;

    const float twobypi = 0x1.45f306p-1f;

    float fnpi2 = SPIRV_OCL_BUILTIN(trunc, _f32, )(SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x, twobypi, 0.5f));

    // subtract n * pi/2 from x
    float rhead, rtail;
    __clc_fullMulS(&rhead, &rtail, fnpi2, fpiby2_1, fpiby2_1_h, fpiby2_1_t);
    float v = x - rhead;
    float rem = v + (((x - v) - rhead) - rtail);

    float rhead2, rtail2;
    __clc_fullMulS(&rhead2, &rtail2, fnpi2, fpiby2_2, fpiby2_2_h, fpiby2_2_t);
    v = rem - rhead2;
    rem = v + (((rem - v) - rhead2) - rtail2);

    float rhead3, rtail3;
    __clc_fullMulS(&rhead3, &rtail3, fnpi2, fpiby2_3, fpiby2_3_h, fpiby2_3_t);
    v = rem - rhead3;

    *hi = v + ((rem - v) - rhead3);
    *lo = -rtail3;
    return fnpi2;
}

int __clc_argReductionSmallS(float *r, float *rr, float x)
{
    float fnpi2 = __clc_removePi2S(r, rr, x);
    return (int)fnpi2 & 0x3;
}


int __clc_argReductionS(float *r, float *rr, float x)
{
    float abs_float = SPIRV_OCL_BUILTIN(fabs, _f32, )(x);
    if (abs_float < 2.0f)
        return __clc_argReductionSmallS(r, rr, x);
    else
        return __clc_argReductionLargeS(r, rr, x);
}

INLINE float libclc_asinh_f32(float x) {
    uint ux = as_uint(x);
    uint ax = ux & EXSIGNBIT_SP32;
    uint xsgn = ax ^ ux;

    // |x| <= 2
    float t = x * x;
    float a = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
                  SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
              SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
                  SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t, -1.177198915954942694e-4f, -4.162727710583425360e-2f),
                  -5.063201055468483248e-1f),
              -1.480204186473758321f),
              -1.152965835871758072f);
    float b = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
              SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
              SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t,
              SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(t, 6.284381367285534560e-2f, 1.260024978680227945f),
              6.582362487198468066f),
              11.99423176003939087f),
          6.917795026025976739f);

    float q = a / b;
    float z1 = SPIRV_OCL_BUILTIN(mad, _f32_f32_f32, )(x*t, q, x);

    // |x| > 2

    // Arguments greater than 1/sqrt(epsilon) in magnitude are
    // approximated by asinh(x) = ln(2) + ln(abs(x)), with sign of x
    // Arguments such that 4.0 <= abs(x) <= 1/sqrt(epsilon) are
    // approximated by asinhf(x) = ln(abs(x) + sqrt(x*x+1))
    // with the sign of x (see Abramowitz and Stegun 4.6.20)

    float absx = as_float(ax);
    int hi = ax > 0x46000000U;
    float y = SPIRV_OCL_BUILTIN(sqrt, _f32, )(absx * absx + 1.0f) + absx;
    y = hi ? absx : y;
    float r = SPIRV_OCL_BUILTIN(log, _f32, )(y) + (hi ? 0x1.62e430p-1f : 0.0f);
    float z2 = as_float(xsgn | as_uint(r));

    float z = ax <= 0x40000000 ? z1 : z2;
    z = ax < 0x39800000U | ax >= PINFBITPATT_SP32 ? x : z;

    return z;
}

INLINE float libclc_cos_f32(float x)
{
    int ix = as_int(x);
    int ax = ix & 0x7fffffff;
    float dx = as_float(ax);

    float r0, r1, c;
    int regn = __clc_argReductionS(&r0, &r1, dx);

    if ((regn & 1) != 0)
        c = -__clc_sinf_piby4(r0, r1);
    else
        c = __clc_cosf_piby4(r0, r1);

    c = as_float(as_int(c) ^ ((regn > 1) << 31));

    c = ax >= PINFBITPATT_SP32 ? as_float(QNANBITPATT_SP32) : c;

    return c;
}

INLINE float libclc_cospi_f32(float x)
{
    int ix = as_int(x) & 0x7fffffff;
    float ax = as_float(ix);
    int iax = (int)ax;
    float r = ax - iax;
    int xodd = iax & 0x1 ? 0x80000000 : 0;

    // Initialize with return for +-Inf and NaN
    int ir = 0x7fc00000;

    // 2^24 <= |x| < Inf, the result is always even integer
    ir = ix < 0x7f800000 ? 0x3f800000 : ir;

    // 2^23 <= |x| < 2^24, the result is always integer
    ir = ix < 0x4b800000 ? xodd | 0x3f800000 : ir;

    // 0x1.0p-7 <= |x| < 2^23, result depends on which 0.25 interval

    // r < 1.0
    float a = 1.0f - r;
    int e = 1;
    int s = xodd ^ 0x80000000;

    // r <= 0.75
    int c = r <= 0.75f;
    a = c ? r - 0.5f : a;
    e = c ? 0 : e;

    // r < 0.5
    c = r < 0.5f;
    a = c ? 0.5f - r : a;
    s = c ? xodd : s;

    // r <= 0.25
    c = r <= 0.25f;
    a = c ? r : a;
    e = c ? 1 : e;

    float2 t = __libclc__sincosf_piby4(a * M_PI_F);
    int jr = s ^ as_int(e ? t.hi : t.lo);

    ir = ix < 0x4b000000 ? jr : ir;

    return as_float(ir);
}

INLINE float libclc_sin_f32(float x)
{
    int ix = as_int(x);
    int ax = ix & 0x7fffffff;
    float dx = as_float(ax);

    float r0, r1, s;
    int regn = __clc_argReductionS(&r0, &r1, dx);

    if ((regn & 1) != 0)
        s = __clc_cosf_piby4(r0, r1);
    else
        s = __clc_sinf_piby4(r0, r1);

    s = as_float(as_int(s) ^ ((regn > 1) << 31) ^ (ix ^ ax));

    s = ax >= PINFBITPATT_SP32 ? as_float(QNANBITPATT_SP32) : s;

    //Subnormals
    s = x == 0.0f ? x : s;

    return s;
}

INLINE float libclc_sinpi_f32(float x)
{
    int ix = as_int(x);
    int xsgn = ix & 0x80000000;
    ix ^= xsgn;
    float ax = as_float(ix);
    int iax = (int)ax;
    float r = ax - iax;
    int xodd = xsgn ^ (iax & 0x1 ? 0x80000000 : 0);

    // Initialize with return for +-Inf and NaN
    int ir = 0x7fc00000;

    // 2^23 <= |x| < Inf, the result is always integer
    ir = ix < 0x7f800000 ? xsgn : ir;

    // 0x1.0p-7 <= |x| < 2^23, result depends on which 0.25 interval

    // r < 1.0
    float a = 1.0f - r;
    int e = 0;

    // r <= 0.75
    int c = r <= 0.75f;
    a = c ? r - 0.5f : a;
    e = c ? 1 : e;

    // r < 0.5
    c = r < 0.5f;
    a = c ? 0.5f - r : a;

    // 0 < r <= 0.25
    c = r <= 0.25f;
    a = c ? r : a;
    e = c ? 0 : e;

    float2 t = __libclc__sincosf_piby4(a * M_PI_F);
    int jr = xodd ^ as_int(e ? t.hi : t.lo);

    ir = ix < 0x4b000000 ? jr : ir;

    return as_float(ir);
}

INLINE float libclc_tan_f32(float x)
{
    int ix = as_int(x);
    int ax = ix & 0x7fffffff;
    float dx = as_float(ax);

    float r0, r1;
    int regn = __clc_argReductionS(&r0, &r1, dx);

    float ss = __clc_sinf_piby4(r0, r1);
    float cc = __clc_cosf_piby4(r0, r1);

    //Logic for sin

    float s = (regn & 1) != 0 ? cc : ss;
    s = as_float(as_int(s) ^ ((regn > 1) << 31) ^ (ix ^ ax));
    s = ax >= PINFBITPATT_SP32 ? as_float(QNANBITPATT_SP32) : s;
    //Subnormals
    s = x == 0.0f ? x : s;

    //Logic for cos

    float c =  (regn & 1) != 0 ? -ss : cc;
    c = as_float(as_int(c) ^ ((regn > 1) << 31));
    c = ax >= PINFBITPATT_SP32 ? as_float(QNANBITPATT_SP32) : c;

    //tan(x) = sin(x)/cos(x)
    return __builtin_spirv_divide_cr_f32_f32(s,c);
}

#endif //__TRIG_CL__













