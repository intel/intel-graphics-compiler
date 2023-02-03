/*===================== begin_copyright_notice ==================================

Copyright (c) 2023 Intel Corporation

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

__attribute__((always_inline))
static inline double __ddiv_ha_div_special(double a, double b)
{
    int_double x, y, res;
    int_double sgn_x, sgn_y;
    int_float  xf, yf;
    int        ex, ey;
    x.f = a;
    y.f = b;

    // exponent fields
    ex      = (x.w >> 52) & 0x7ff;
    ey      = (y.w >> 52) & 0x7ff;
    sgn_x.w = x.w & 0x8000000000000000ul;
    sgn_y.w = y.w & 0x8000000000000000ul;

    // sNaNs should set Invalid below
    if (ex == 0x7ff)
    {
        // NaN ?
        if (x.w & 0x000ffffffffffffful)
        {
            res.w = x.w | 0x0008000000000000ul;
            return res.f;
        }
        // x is +/- Inf
        if (ey == 0x7ff)
        {
            // y is NaN?
            if (y.w & 0x000ffffffffffffful)
            {
                res.f = y.f + y.f;
                return res.f;
            }
            // y is +/-Inf, return NaN
            res.f = y.f * sgn_x.f;
            return res.f;
        }
        // return Inf with correct sign
        res.w = x.w ^ sgn_y.w;
        return res.f;
    }

    if (ey == 0x7ff)
    {
        // NaN ?
        if (y.w & 0x000ffffffffffffful)
        {
            res.f = y.f + y.f;
            return res.f;
        }
        res.w = sgn_x.w ^ sgn_y.w;
        return res.f;
    }

    if (x.f == 0.0)
    {
        // y zero as well?
        if (y.f == 0.0)
        {
            res.w = 0xfff8000000000000ul;
            return res.f;
        }
        res.w = sgn_x.w ^ sgn_y.w;
        return res.f;
    }

    // (y.f == 0.0)
    // return Inf with correct sign
    // ****** also set Div-by-Zero
    yf.f = (float)y.f;
    xf.f = (float)sgn_x.f;
    xf.w |= 0x40000000;
    xf.f  = xf.f / yf.f; // This is done to set Div-by-Zero flag
    res.f = (double)xf.f;

    // Alternative, without setting the Div-by-Zero flag:  res.w = sgn_x.w ^
    // sgn_y.w ^ 0x7ff0000000000000ul;
    return res.f;
}

// Note:  The IEEE implementation requires manipulation of the rounding mode
// To ensure correct rounding, the internal computation must be done in RN mode
// IEEE version
__attribute__((always_inline))
inline int __ocl_svml_internal_ddiv_ha(double *pxin, double *pyin, double *pres)
{
    int    nRet = 0;
    double a = *pxin, b = *pyin;
    double y0, q0, e0, r0, y1, e1, q1, y2, y3, r1, q;

    union
    {
        unsigned long w;
        unsigned int  w32[2];
        int           s32[2];
        double        f;
    } x, y, y00, scale0, scale1, scale2, ma, mb, mbf, uq, uq1, __two64, __twom64, my0;

    int   expon_x, expon_y, expon_diff, g_ediff;
    long  expon_diff2;
    float y0f, bf;

    x.f = a;
    y.f = b;

    // biased exponents
    expon_x = ((x.w >> 52) & 0x7ff);
    expon_y = ((y.w >> 52) & 0x7ff);

    // check exponent ranges
    // Main path will be taken for expon_x in [bias-896, bias+896) and expon_y in
    // [bias-126, bias+126) Ranges chosen such that no overflow/underflow occurs
    // for any main path steps
    if (((unsigned int)(expon_x + 896 - 0x3ff) >= (896 + 896)) ||
        ((unsigned int)(expon_y + 126 - 0x3ff) >= (126 + 126)))
        goto LONG_DBL_DIV;

    // y0 = MATH_INV(b);
    // Save user rounding mode here, set new rounding mode to RN
    __builtin_IB_set_rounding_mode_fp(ROUND_TO_NEAREST_EVEN);

    mbf.f = b;
    bf    = (float)mbf.f;

    // get_rcp maps to math.inv here
    y0f = 1.0f / (bf);
    y0  = (double)y0f;

    // Because the double-to-float conversion is done in RN mode, we need this fix
    // to ensure corner cases round correctly Alternatively, maintain the AND
    // truncation shown above
    my0.f = y0;
    my0.w |= 1;
    y0 = my0.f;

    // Step(1), q0=a*y0
    q0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(a, y0, 0.0);
    // Step(2), e0=(1-b*y0)
    e0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(b, -y0, 1.0);
    // Step(3), r0=a-b*q0
    r0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(b, -q0, a);
    // Step(4), y1=y0+e0*y0
    y1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y0, e0, y0);
    // Step(5), e1=(1-b*y1)
    e1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(b, -y1, 1.0);
    // Step(6), y2=y0+e0*y1
    y2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(e0, y1, y0);
    // Step(7), q1=q0+r0*y1
    q1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r0, y1, q0);
    // Step(8), y3=y1+e1*y2
    y3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(e1, y2, y1);
    // Step(9), r1=a-b*q1
    r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(b, -q1, a);

    // Restore user rounding mode here
    __builtin_IB_reset_rounding_mode();

    // Step(10), q=q1+r1*y3
    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r1, y3, q1);

    *pres = q;
    return nRet;

LONG_DBL_DIV:

    scale0.w = 0x3fe0000000000000ul;

    if ((((unsigned int)(expon_x - 1)) >= 0x7fe) ||
        (((unsigned int)(expon_y - 1)) >= 0x7fe))
    {
        __two64.w  = 0x43f0000000000000ul;
        __twom64.w = 0x3bf0000000000000ul;

        // normalize inputs, if needed
        if (!expon_x)
        {
            x.f *= __two64.f;
            expon_x = ((x.w >> 52) & 0x7ff);
            if (expon_x) scale0.f *= __twom64.f; // if(expon_x still 0), DAZ must be on
        }

        if (!expon_y)
        {
            y.f *= __two64.f;
            expon_y = ((y.w >> 52) & 0x7ff);
            if (expon_y) scale0.f *= __two64.f; // if(expon_y still 0), DAZ must be on
        }

        // treatment of special cases
        if ((((unsigned int)(expon_x - 1)) >= 0x7fe) ||
            (((unsigned int)(expon_y - 1)) >= 0x7fe))
        {
            *pres = __ddiv_ha_div_special(a, b);
            return nRet;
        }
    }

    expon_diff = expon_x - expon_y + 1 + 0x7fe;

    // scale factors to be used at the end of the computation
    scale1.w = expon_diff >> 1;
    scale2.w = expon_diff - scale1.w;
    scale1.w <<= 52;
    scale2.w <<= 52;

    // Will compute mantissa(a)*2^scale0/mantissa(b)
    ma.w = scale0.w | (x.w & 0x800ffffffffffffful);
    mb.w = 0x3ff0000000000000ul | (y.w & 0x800ffffffffffffful);

    // arithmetic shift
    // This value is needed to detect gradual underflow
    expon_diff2 =
        ((long)((ma.w & 0x7ffffffffffffffful) - (mb.w & 0x7ffffffffffffffful))) >> 52;
    g_ediff = expon_diff + (int)expon_diff2;

    // y0 = MATH_INV(mb.f);
    // Save user rounding mode here, set new rounding mode to RN
    __builtin_IB_set_rounding_mode_fp(ROUND_TO_NEAREST_EVEN);

    // ensure exact conversion to float for correct status flags (not used here)
    mbf.w = mb.w; // mbf.w = mb.w & 0xffffffffe0000000ul;
    bf    = (float)mbf.f;

    // get_rcp maps to math.inv here
    y0f = 1.0f / (bf);
    y0  = (double)y0f;

    // Because the double-to-float conversion is done in RN mode, we need this fix
    // to ensure corner cases round correctly Alternatively, maintain the AND
    // truncation shown above
    my0.f = y0;
    my0.w |= 1;
    y0 = my0.f;

    // Step(1), q0=a*y0
    q0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(ma.f, y0, 0.0);
    // Step(2), e0=(1-b*y0)
    e0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(mb.f, -y0, 1.0);
    // Step(3), r0=a-b*q0
    r0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(mb.f, -q0, ma.f);
    // Step(4), y1=y0+e0*y0
    y1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y0, e0, y0);
    // Step(5), e1=(1-b*y1)
    e1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(mb.f, -y1, 1.0);
    // Step(6), y2=y0+e0*y1
    y2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(e0, y1, y0);
    // Step(7), q1=q0+r0*y1
    q1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r0, y1, q0);
    // Step(8), y3=y1+e1*y2
    y3 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(e1, y2, y1);
    // Step(9), r1=a-b*q1
    r1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(mb.f, -q1, ma.f);
    // Redirect gradual underflow cases

    // Restore user rounding mode here
    __builtin_IB_reset_rounding_mode();

    if ((g_ediff < 2046 - 1022) && (g_ediff >= 2046 - 1078))
        goto DBLDIV_G_UF_PATH;

    // Step(10), q=q1+r1*y3
    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r1, y3, q1);
    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(q, scale1.f, 0.0);

    // Scaling, may result in overflow/underflow
    q = q * scale2.f;

    *pres = q;
    return nRet;

DBLDIV_G_UF_PATH:
    // Step(10), q=q1+r1*y3 in RZ mode
    // Set rounding mode to RZ here (round-to-zero)
    __builtin_IB_set_rounding_mode_fp(ROUND_TO_ZERO);

    q = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(r1, y3, q1);

    // Restore user rounding mode here
    __builtin_IB_reset_rounding_mode();

    uq.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(q, scale1.f, 0.0);

    if (r1 == 0) // normal result would be exact
    {
        q     = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(uq.f, scale2.f, 0.0);
        *pres = q;
        return nRet;
    }

    // normal result is inexact
    if (g_ediff != 2046 - 1023)
    {
        uq.w |= 1; // OR in sticky bit
        q     = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(uq.f, scale2.f, 0.0);
        *pres = q;
        return nRet;
    }

    // Last case:  gradual underflow, shift amount is 1
    uq1.w = uq.w & 0xfff0000000000000ul;
    uq.f  = uq.f - uq1.f;

    // add sticky bit, and preserve sign after previous SUB
    uq.w &= 0x7ffffffffffffffful; // in case sign was set incorrectly in RD mode
                                  // (for uq=0)
    uq.w |= (1 | (uq1.w & 0x8000000000000000ul));
    uq.f = uq.f * scale2.f;

    // uq1.f = DP_FMA(uq1.f, scale2.f, 0.0);
    uq.w += 0x0008000000000000ul;
    scale0.w = 0x3ff0000000000000ul;

    // artificially set UF (needed when DAZ=1)
    scale1.w = 0x0010000000000000ul;
    scale1.f *= scale1.f;
    q = uq.f * scale0.f;

    *pres = q;
    return nRet;
}

double __ocl_svml_div_ha(double x, double y)
{
    double r;
    __ocl_svml_internal_ddiv_ha(&x, &y, &r);
    return r;
}
