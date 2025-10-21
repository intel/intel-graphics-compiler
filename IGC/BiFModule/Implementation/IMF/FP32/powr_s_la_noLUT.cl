/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

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
static __constant unsigned char __spowr_la___rcp_tbl[] = {
    0xff, 0xf0, 0xe3, 0xd7, 0xcc, 0xc2, 0xb9,
    0xb1, 0xaa, 0xa3, 0x9d, 0x97, 0x91, 0x8c,
    0x88, 0x83, 0x7f,
};

// -log2(_VSTATIC(__rcp_tbl)[i]/2^8))*2^(23+32)
static __constant unsigned long __spowr_la___log2_tbl[] = {
    0x0000000000000000uL, 0x000b2671360338acuL, 0x001563dc29ffacb2uL,
    0x001f5fd8a9063e36uL, 0x002906cbcd2baf2euL, 0x003243001249ba76uL,
    0x003afcd815786af2uL, 0x00431b2abc31565cuL, 0x004a83cf0d01c170uL,
    0x00523bbc64c5e644uL, 0x00591db662b66428uL, 0x006043e946fd97f4uL,
    0x0067b3d42fd0fc50uL, 0x006e232e68aad484uL, 0x007373af48dce654uL,
    0x007a514b229c40a0uL, 0x0080000000000000uL,
};

// polynomial coefficients
// c6*2^31
static __constant int __spowr_la___lc6 = 0xE158260E;
// c5*2^31
static __constant int __spowr_la___lc5 = 0x24F7FD36;
// c4*2^31
static __constant int __spowr_la___lc4 = 0xD1D568F0;
// c3*2^31
static __constant int __spowr_la___lc3 = 0x3D8E12ED;
// c2*2^31
static __constant int __spowr_la___lc2 = 0xA3AAE26C;
// c1*2^(23+32)
static __constant unsigned long __spowr_la___lc1 = 0xB8AA3B295EBB00uL;
// exp2 coefficients
// c7*2^32
static __constant int __spowr_la___sc7 = 0x00016B68;
// c6*2^32
static __constant int __spowr_la___sc6 = 0x00095E83;
// c5*2^32
static __constant int __spowr_la___sc5 = 0x00580436;
// c4*2^32
static __constant int __spowr_la___sc4 = 0x027607DE;
// c3*2^32
static __constant int __spowr_la___sc3 = 0x0E359872;
// c2*2^32
static __constant int __spowr_la___sc2 = 0x3D7F7977;
// c1*2^32
static __constant int __spowr_la___sc1 = 0xB1721817;
static unsigned int __spowr_la_powrf_cout (unsigned int xin, unsigned int yin, int *errcode)
{
    int mant, expon, index, sgn_y, R, poly, N;
    int expon_y, is_int, mant_y, mi_y;
    unsigned int rcp, res, shift, abs_x, abs_y, poly_low, poly_h, sgn_x = 0, p_inf;
    unsigned long poly64, exp64, poly_s1;
    // unpack mantissa, unbiased exponent
    mant = ((xin) & 0x7fffff);
    expon = ((xin) >> 23) - 0x7f;
    abs_x = xin & 0x7fffffff;
    abs_y = yin & 0x7fffffff;
    sgn_y = (((int) (yin)) >> (31));
    if ((((unsigned int) (abs_y - 1)) >= (0x7F800000 - 1)))
        goto SPOW_SPECIAL_Y;
    // filter out special and negative cases, as well as denormals
    if ((((unsigned int) (xin - 0x00800000)) >= (0x7F800000 - 0x00800000)))
        goto SPOW_SPECIAL_X;
  SPOW_LOG_MAIN:
    // add leading mantissa bit
    mant |= 0x00800000;
    // table index
    index = ((mant + 0x00040000) >> (23 - 4)) - 0x10;
    // rcp ~ 2^8/mant
    rcp = 1 + __spowr_la___rcp_tbl[index];
    // reduced argument R = mant*rcp - 1, scale 2^32
    R = (((unsigned int) mant) * ((unsigned int) rcp)); // scale 2^31
    R = R + R;
    // (c6*R+c5)*2^31
    poly = ((((long) ((int) (__spowr_la___lc6))) * ((int) (R))) >> 32);
    poly = poly + __spowr_la___lc5;
    // poly*R+c4, scale 2^31
    poly = ((((long) ((int) (poly))) * ((int) (R))) >> 32);
    poly = poly + __spowr_la___lc4;
    // poly*R+c3, scale 2^31
    poly = ((((long) ((int) (poly))) * ((int) (R))) >> 32);
    poly = poly + __spowr_la___lc3;
    // poly*R+c2, scale 2^31
    poly = ((((long) ((int) (poly))) * ((int) (R))) >> 32);
    poly = poly + __spowr_la___lc2;
    // poly*2^(23+32)
    poly_low = poly << (32 - 8);
    poly_h = (((int) (poly)) >> (8));
    // c1+R*poly, scale 2^(23+32)
    poly64 = (((long) ((int) (poly_h))) * ((int) (R))) + __spowr_la___lc1;
    // poly_low to be treated as positive value
    poly_low = (((unsigned int) (poly_low)) >> (1));
    poly_low = ((((long) ((int) (poly_low))) * ((int) (R))) >> 32);
    poly_low += poly_low;
    poly64 += (long) ((int) poly_low);
    // adjustment for x near 1.0
    shift = 0x7f + 21;
    if (!((expon << 4) + index))
    {
        poly64 <<= 7;
        shift = 7 + 0x7f + 21;
        // is x exactly 1.0?
        if (!R)
            return sgn_x | 0x3f800000;
    }
    // T+R*poly, scale 2^(2+32+(shift-bias))
    poly_low = (unsigned int) poly64;
    poly_h = (unsigned int) (poly64 >> 32);
    poly64 = (((long) ((int) (poly_h))) * ((int) (R))) + __spowr_la___log2_tbl[index];
    // adjust for sign of poly_low
    poly_low = (((unsigned int) (poly_low)) >> (1));
    poly_low = ((((long) ((int) (poly_low))) * ((int) (R))) >> 32);
    poly_low += poly_low;
    poly64 += (long) ((int) poly_low);
    // log2(x) ~ expon+T+R*poly, sc 2^(2+32+(shift-bias))
    expon <<= 23;
    exp64 = (unsigned long) expon;
    poly64 += (exp64 << 32);
    poly_s1 = poly64 << 1;
    while (poly_s1 && (((long) (poly_s1 ^ poly64)) >= 0))
    {
        poly64 = poly_s1;
        poly_s1 <<= 1;
        shift++;
    }
    // y, sc 2^(30-expon_y)
    // unpack mantissa, biased exponent
    expon_y = shift - ((abs_y) >> 23);
    mant = ((abs_y) & 0x7fffff);
    // denormal y?
    if (abs_y < 0x00800000)
        expon_y = shift - 1;
    else
        mant |= 0x00800000;
    // apply sign to mantissa bits
    mant = (mant ^ sgn_y) - sgn_y;
    // mant, scale 2^30
    mant <<= 7;
    // y*log2(x), sc 2^(2+30 + expon_y)
    poly_low = (unsigned int) poly64;
    poly_h = (unsigned int) (poly64 >> 32);
    poly64 = (((long) ((int) (poly_h))) * ((int) (mant)));
    // adjust for sign of poly_low
    poly_low = (((unsigned int) (poly_low)) >> (1));
    poly_low = ((((long) ((int) (poly_low))) * ((int) (mant))) >> 32);
    poly_low += poly_low;
    poly64 += (long) ((int) poly_low);
    if (expon_y < 0)    //overflow/underflow
    {
        poly_h = (unsigned int) (poly64 >> 32);
        if (((int) poly_h) < 0)
            goto SPOW_UF;
        goto SPOW_OF;
    }
    if (expon_y >= 32)
    {
        expon_y -= 32;
        poly64 = (((long) (poly64)) >> (32));
        if (expon_y >= 32)
            return sgn_x | 0x3f800000;
    }
    // integer part in high 32 bits, fractional bits in low part
    poly64 = (((long) (poly64)) >> (expon_y));
    N = (unsigned int) (poly64 >> 32);
    // reduced exp2 argument, sc 2^32
    R = (unsigned int) poly64;
    // (c7*R+c6)*2^32
    poly = ((((unsigned long) ((unsigned int) (__spowr_la___sc7))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc6;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc5;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc4;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc3;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc2;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    poly = poly + __spowr_la___sc1;
    // poly*2^32
    poly = ((((unsigned long) ((unsigned int) (poly))) * ((unsigned int) (R))) >> 32);
    // rounding and overflow/underflow checking
    // poly*2^31
    poly = (((unsigned int) (poly)) >> (1)) + 128;
    expon = N + 0x7f;
    N = expon + (((unsigned int) (poly)) >> (31));
    // overflow?
    if (N >= 0xff)
        goto SPOW_OF;
    // underflow, possibly gradual?
    if (N <= 0)
        goto SPOW_GRAD_UF;
    res = sgn_x | ((expon << 23) + (((unsigned int) (poly)) >> (8)));
    return res;
  SPOW_OF:
    res = sgn_x | 0x7f800000;
    *errcode = 3;
    return res; // goto POWF_ERRCALL;
  SPOW_GRAD_UF:
    if (N < -24)
        goto SPOW_UF;
    // poly*2^31, undo rounding to 24 bits
    poly = poly + 0x80000000 - 128;
    N = expon;
    while (N < 1)
    {
        poly = (((unsigned int) (poly)) >> (1));
        N++;
    }
    poly = (((unsigned int) (poly + 128)) >> (8));
    if (poly)
        return sgn_x | poly;
  SPOW_UF:
    res = sgn_x;
    *errcode = 4;
    return res; // goto POWF_ERRCALL;
  SPOW_SPECIAL_Y:
    // 0, Inf, NaN
    // y=0?
    if (!abs_y)
    {
        if (!abs_x)
        {
            unsigned int r = xin ^ yin;
            r = r | 0x7fffffff;
            *errcode = 1;
            return r;
        }
        else
        {
            if (abs_x > 0x7f800000)
            {
                return abs_x | yin;
            }
            else
            {
                return 0x3f800000;
            }
        }
    }
    // y=NaN?
    if (abs_y > 0x7f800000)
    {
        return abs_y;
    }
    // return ((xin == 0x3f800000) ? xin : 0xffc00000);
    // +/-Inf
    // x is NaN?
    if (((unsigned int) (xin + xin)) > 0xff000000u)
        return 0xffc00000;
    // |x| == 1?
    R = (xin & 0x7fffffff) - 0x3f800000;
    if (R == 0)
        return 0x3f800000;
    R ^= sgn_y;
    if (((int) R) < 0)
        return 0;
    res = 0x7f800000;
    if (!(xin + xin))
    {
        *errcode = 1;
    }
    return res;
  SPOW_SPECIAL_X:
    p_inf = 0x7f800000;
    // +Inf?
    if (xin == p_inf)
        return (sgn_y ? 0 : xin);
    // NaN
    if (((unsigned int) (xin + xin)) > 0xff000000u)
        return 0xffc00000;
    if (((int) xin) > 0)
    {
      SPOW_DENORM_X:
        // denormal input, normalize
        expon = 1 - 0x7f;
        while (mant < 0x00800000)
        {
            expon--;
            mant <<= 1;
        }
        // return to main computation
        goto SPOW_LOG_MAIN;
    }
    // is y an integer?
    is_int = 0;
    if (abs_y >= 0x3f800000)
    {
        if (abs_y >= 0x4b800000)
            is_int = 1; // and even integer (>=2^24)
        else
        {
            shift = 23 + 0x7f - (((unsigned int) (abs_y)) >> (23));
            mant_y = ((abs_y) & 0x7fffff) | 0x00800000;
            mi_y = (((unsigned int) (mant_y)) >> (shift));
            if (mant_y == (mi_y << shift))
            {
                is_int = 1;
                // set sign for odd integers
                sgn_x = mi_y << 31;
            }
        }
    }
    // +/-zero?
    if (!(xin + xin))
    {
        if (!sgn_y)
            return 0;
        sgn_x &= xin;
        res = sgn_x | 0x7f800000;
        *errcode = 1;
        return res; // goto POWF_ERRCALL;
    }
    // negative?
    if (((int) xin) < 0)
    {
        if (xin == 0xff800000)
            return (sgn_y ? sgn_x : (sgn_x | 0x7f800000));
        if (!is_int)
        {
            *errcode = 1;
            res = 0xffc00000;
            return res; // goto POWF_ERRCALL;
        }
        expon -= 0x100;
        if (xin == 0xbf800000)
            return sgn_x | 0x3f800000;
        if (expon >= -126)
            goto SPOW_LOG_MAIN;
        goto SPOW_DENORM_X;
    }
    return xin;
}

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_c1 = { 0x3eaaaaa8 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_c2 = { 0x3e4cd0b0 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_c3 = { 0x3e1166f0 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_c4 = { 0x3e046000 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_m181o256 = { 0x3f350000 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_half = { 0x3f000000 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_two = { 0x40000000 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_log2hi = { 0x3f317218 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_slog_log2lo = { 0xb102e308 };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_shft = { 0x4ac000feu };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_l2e = { 0x3FB8AA3Bu };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_l2h = { 0x3f317218u };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_l2l = { 0xb102E308u };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_c5 = { 0x3c08ba8bu };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_c4 = { 0x3d2aec4eu };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_c3 = { 0x3e2aaa9cu };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_c2 = { 0x3effffe8u };

static __constant union
{
    unsigned int w;
    float f;
} __spowr_la_sexp_c1 = { 0x3f800000u };

__attribute__((always_inline))
static inline float __spowr_la_internal_frexpf (float arg, int *exp_res)
{
    unsigned int uX;
    float fS, fR, fOne;
    uX = ((*(int *) &arg) & ~0x80000000) - 0x00800000;
    // Normal args
    if (uX < 0x7f800000 - 0x00800000)
    {
        (*(int *) &fR) = (*(int *) &arg) & 0x807fffff;
        (*(int *) &fR) |= 0x3f000000;
        *exp_res = ((int) (uX >> 23) - (0x007F - 1)) + 1;
    }
    // Subnormal args
    else
    {
        (*(int *) &fR) = (*(int *) &arg) | 0x3f000000;
        (*(int *) &fS) = 0x3f000000 | ((*(int *) &fR)) & 0x80000000;
        fR = fR - fS;
        uX = (*(int *) &fR);
        uX = uX & 0x7f800000;
        (*(int *) &fR) &= ~0x7f800000;
        (*(int *) &fR) |= 0x3f000000;
        *exp_res = ((int) (uX >> 23) - (0x007F - 1)) - 125;
    }
    return fR;
}

__attribute__((always_inline))
inline int __ocl_svml_internal_spowr_noLUT (float *pxin, float *pyin, float *pres)
{
    int nRet = 0;
    union
    {
        unsigned int w;
        float f;
    } fwX, fwY;
    union
    {
        unsigned int w;
        float f;
    } fwYLogX;
    union
    {
        unsigned int w;
        float f;
    } fwS, fwTh, fwTh2, fwRes;
    float fX, fY;
    float fN, fR, fPoly;
    float fExpArgHi, fExpArgLo;
    float fLogResHi, fLogResLo, fLogTHi, fLogTLo, fLogPolyHi, fLogPolyLo, fLogR;
    float fLogMant, fLogExp, fLogV3, fLogV2, fLogV1, fLogFHi, fLogFLo;
    unsigned int uXa32, uSgnX, uExpCorr;
    unsigned int uExpX;
    unsigned int uAbsYLogX;
    int iIdxMask;
    int iExp32, iMask32, iMaskH;
    int iExpX, iSmallX;
    fX = *pxin;
    fY = *pyin;
    fwX.f = *pxin;
    fwY.f = *pyin;
    // Return NaN for negative X
    if (fwX.w > 0x80000000u)
    {
        *pres = 0.0f / 0.0f;
        nRet = 1;
        return nRet;
    }
    // Argument reduction to range from 181/256 to 362/256:
    // Get mantissa and exponent of argument
    fLogMant = __spowr_la_internal_frexpf (fX, &iExpX);
    // If mantissa less than 181/256 then multiply it by 2 and decrease exponent by 1
    iSmallX = (fLogMant < __spowr_la_slog_m181o256.f);
    fLogMant = (iSmallX) ? (2.0f * fLogMant) : fLogMant;
    iExpX = (iSmallX) ? (iExpX - 1) : iExpX;
    // Fraction f = (fLogMant-1)/(fLogMant+1)
    fLogV1 = __spirv_ocl_fma(fLogMant,1.0f,1.0f);
    fLogMant = __spirv_ocl_fma(fLogMant,1.0f,-1.0f);
    fLogR = 1.0f / fLogV1;
    fLogFHi = __spirv_ocl_fma(fLogMant,fLogR,0.0f);
    fLogV2 = __spirv_ocl_fma(fLogFHi,-__spowr_la_slog_two.f,fLogMant);
    fLogV3 = __spirv_ocl_fma(fLogFHi,-fLogMant,fLogV2);
    fLogFLo = __spirv_ocl_fma(fLogR,fLogV3,0.0f);
    // atanh(f) approximation
    fLogV3 = fLogFHi * fLogFHi;
    fLogR = __spirv_ocl_fma(__spowr_la_slog_c4.f,fLogV3,__spowr_la_slog_c3.f);
    fLogR = __spirv_ocl_fma(fLogR,fLogV3,__spowr_la_slog_c2.f);
    fLogR = __spirv_ocl_fma(fLogR,fLogV3,__spowr_la_slog_c1.f);
    fLogV2 = __spirv_ocl_fma(fLogFHi,fLogFLo+fLogFLo,__spirv_ocl_fma(fLogFHi,fLogFHi,-fLogV3));
    fLogV1 = __spirv_ocl_fma(fLogV3,fLogFHi,0.0f);
    fLogV2 =
        __spirv_ocl_fma(fLogV3, fLogFLo,
                                                __spirv_ocl_fma(fLogV2, fLogFHi,
                                                                                        __spirv_ocl_fma(fLogV3, fLogFHi,
                                                                                                                                -fLogV1)));
    fLogV3 = __spirv_ocl_fma(fLogR,fLogV1,__spirv_ocl_fma(fLogR,fLogV2,fLogFLo));
    // Log exponent as floating point
    fLogExp = (float) iExpX;
    // log = 2 * atanh(f) + fLogExp * log(2)
    fLogV2 =
        __spirv_ocl_fma(__spirv_ocl_fma (__spowr_la_slog_half.f, __spowr_la_slog_log2hi.f, 0.0f),
                                                fLogExp, fLogFHi);
    fLogV1 =
        __spirv_ocl_fma(__spirv_ocl_fma (__spowr_la_slog_half.f, -__spowr_la_slog_log2hi.f, 0.0f),
                                                fLogExp, fLogV2);
    fLogV3 = __spirv_ocl_fma(__spirv_ocl_fma(fLogFHi,1.0f,-fLogV1),1.0f,fLogV3);
    fLogV3 = __spirv_ocl_fma(__spowr_la_slog_log2lo.f*__spowr_la_slog_half.f,fLogExp,fLogV3);
    fLogR = __spirv_ocl_fma(fLogV2,1.0f,fLogV2);
    // log result in high and low parts
    fLogResHi = __spirv_ocl_fma(__spowr_la_slog_two.f,fLogV3,fLogR);
    fLogResLo =
        __spirv_ocl_fma(__spowr_la_slog_two.f,fLogV3,__spirv_ocl_fma(fLogR,1.0f,-fLogResHi));
    // Multiply  y * log(x)
    fLogTHi = __spirv_ocl_fma(fLogResHi,fY,0.0f);
    // Check for special args processing branch
    fwYLogX.f = (float) (fLogTHi);
    uExpX = fwX.w >> 23;
    uExpX--;
    uAbsYLogX = fwYLogX.w & 0x7fffffffu;
    if ((uExpX >= 0xfe) || (uAbsYLogX >= 0x42afb6e0))
    {
        goto SPOW_MAIN_SPECIAL;
    }
    // Rest of multi-precision  y * log(x)
    fLogTLo = __spirv_ocl_fma(fLogResHi,fY,-fLogTHi);
    fLogTLo = __spirv_ocl_fma(fLogResLo,fY,+fLogTLo);
    fLogPolyHi = __spirv_ocl_fma(fLogTHi,1.0f,fLogTLo);
    fLogPolyLo = __spirv_ocl_fma(__spirv_ocl_fma(fLogTHi,1.0f,-fLogPolyHi),1.0f,+fLogTLo);
    fExpArgHi = fLogPolyHi;
    fExpArgLo = fLogPolyLo;
    // Exp part computation
    fwS.f = __spirv_ocl_fma(fExpArgHi,__spowr_la_sexp_l2e.f,__spowr_la_sexp_shft.f);
    fN = fwS.f - __spowr_la_sexp_shft.f;
    fR = __spirv_ocl_fma((-fN),__spowr_la_sexp_l2h.f,fExpArgHi);
    fR = __spirv_ocl_fma((-fN),__spowr_la_sexp_l2l.f,fR);
    fR += fExpArgLo;
    // Set exponent in place
    fwTh.w = fwS.w << 22;
    // iIdxMask is based on last bit of fwS.w
    iIdxMask = 0 - (fwS.w & 1);
    // Set fwTh mantissa
    fwTh.w ^= (iIdxMask & 0x7504F3u);
    // Exp polynomial
    fPoly = __spirv_ocl_fma(fR,__spowr_la_sexp_c5.f,__spowr_la_sexp_c4.f);
    fPoly = __spirv_ocl_fma(fR,fPoly,__spowr_la_sexp_c3.f);
    fPoly = __spirv_ocl_fma(fR,fPoly,__spowr_la_sexp_c2.f);
    fPoly = __spirv_ocl_fma(fR,fPoly,__spowr_la_sexp_c1.f);
    fPoly = fR * fPoly;
    // Big abs exp arg branch
    if (uAbsYLogX > 0x42AEAC4Fu)
    {
        fwS.w += 0xfe;
        fwTh2.w = (fwS.w >> 2) & 0xff;
        fwS.w -= (fwTh2.w << 1);
        fwTh2.w <<= 23;
        fwTh.w = fwS.w << 22;
        fwTh.w ^= (iIdxMask & 0x7504F3u);
        fwRes.f = __spirv_ocl_fma(fPoly,fwTh.f,fwTh.f);
        fwRes.f *= fwTh2.f;
    }
    else
    {
        fwRes.f = __spirv_ocl_fma(fPoly,fwTh.f,fwTh.f);
    }
    *pres = fwRes.f;
    return nRet;
  SPOW_MAIN_SPECIAL:
    fwRes.w = __spowr_la_powrf_cout (fwX.w, fwY.w, &nRet);
    *pres = fwRes.f;
    return nRet;
}

float __ocl_svml_powrf_noLUT (float a, float b)
{
    float r;
    __ocl_svml_internal_spowr_noLUT (&a, &b, &r);
    return r;
}
