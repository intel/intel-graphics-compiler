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
static inline double __dsqrt_ha_sqrt_special(double a)
{
    int_double    x, y, res;
    unsigned long sgn_x;
    unsigned      ex;
    x.f = a;

    // exponent fields
    ex    = (x.w >> 52) & 0xfff;
    sgn_x = x.w & 0x8000000000000000ull;

    if ((ex & 0x7ff) == 0x7ff)
    {
        // NaN or +Inf?
        if (x.w != 0xfff0000000000000ull)
            return a + a;
    }

    if (x.f == 0.0)
    {
        res.w = sgn_x;
        return res.f;
    }

    // negative input
    res.w = 0xfff8000000000000ull;
    return res.f;
}

// IEEE version
// Note:  The IEEE implementation requires manipulation of the rounding mode
// To ensure correct rounding, the internal computation must be done in RN mode
__attribute__((always_inline))
inline int __ocl_svml_internal_dsqrt_ha(double *pxin, double *pres)
{
    int    nRet = 0;
    double a    = *pxin;
    double special_res, y0, S0, H0, S1, H1, S;
    double e, d, d1;

    union
    {
        unsigned long w;
        unsigned int  w32[2];
        int           s32[2];
        double        f;
    } ma, maf, __sc0, __sc1;

    unsigned es;
    float    y0f, af;

    ma.f = a;

    if ((a == 0.0) || (ma.w >= 0x7ff0000000000000ull))
    {
        *pres = __dsqrt_ha_sqrt_special(a);
        return nRet;
    }

    es      = (unsigned)(ma.w >> 53);
    __sc0.w = 0x3ff + 0x1ff - es;
    __sc0.w <<= 52;
    __sc1.w = es + 0x200;
    __sc1.w <<= 52;

    // prescaling;  prevent (sc0.f*sc0.f) optimization
    a = (a * __sc0.f);
    a = (a * __sc0.f);

    // Now start the SQRT computation
    // y0 = MATH_RSQT(a);
    // ensure conversion is exact (to avoid setting spurious status flags; not
    // used here)
    // Save user rounding mode here, set new rounding mode to RN
    __builtin_IB_set_rounding_mode_fp(ROUND_TO_NEAREST_EVEN);

    maf.f = a;
    af    = (float)maf.f;
    // get_rsqrt maps to math.rsqt
    y0f = 1.0f / SPIRV_OCL_BUILTIN(sqrt, _f32, )(af);
    y0  = (double)y0f;
    // H0 = 0.5*y0
    H0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(y0, 0.5, 0.0);
    // S0 = a*y0
    S0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(a, y0, 0.0);
    // d = 0.5 - S0*H0
    d = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S0, -H0, 0.5);
    // e = 1 + 1.5*d
    e = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(d, 1.5, 1.0);
    // e = e*d
    e = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(e, d, 0.0);
    // S1 = S0 + e*S0
    S1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S0, e, S0);
    // H1 = H0 + e*H0
    H1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(H0, e, H0);
    // d1 = a - S1*S1
    d1 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(S1, -S1, a);
    // S = S1+d1*H1

    // Restore user rounding mode here
    __builtin_IB_reset_rounding_mode();

    S = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(H1, d1, S1);
    S *= __sc1.f;

    *pres = S;
    return nRet;
}

double __ocl_svml_sqrt_ha(double x)
{
    double r;
    __ocl_svml_internal_dsqrt_ha(&x, &r);
    return r;
}
