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

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c14 = { 0x3d4632316239fad4UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c13 = { 0xbd9583260d78e81fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c12 = { 0x3dd7d6c1e967b53eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c11 = { 0xbe14bfbad0880ba1UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c10 = { 0x3e4fb4ac3bad06c5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c9 = { 0xbe85f519a7a3c52dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c8 = { 0x3ebb9e23384dde28UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c7 = { 0xbeef4d1ebc4a5988UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c6 = { 0x3f1f9a31f5473d46UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c5 = { 0xbf4c02db3a4a8f77UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c4 = { 0x3f7565bcd0b9b66aUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c3 = { 0xbf9b82ce3126e0d4UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c2 = { 0x3fbce2f21a042390UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c1 = { 0xbfd812746b0379d6UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___c0 = { 0x3fc06eba8214db68UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r216 = { 0xbea52e12a697a62cUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r215 = { 0x3eb0ea52ae7b85b5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r214 = { 0x3ed06e497ca391ceUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r213 = { 0xbef10da44fcb2514UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r212 = { 0x3ed5f941870678e4UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r211 = { 0x3f1b33e6a5536ea5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r210 = { 0xbf31391db5fdeb0cUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r29 = { 0xbf0a46d9369b2871UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r28 = { 0x3f5a114367309cdeUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r27 = { 0xbf6bae0ab414abbaUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r26 = { 0x3f3e1935e324118fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r25 = { 0x3f8ace7404b467abUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r24 = { 0xbfa1a2c597574a37UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r23 = { 0x3fa8b0ae3a47990aUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r22 = { 0xbfa529b9e8cf9b19UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r21 = { 0x3f9529b9e8cf99f5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r20 = { 0x3fefd9ae142795e3UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r318 = { 0x3e212eb7ad190d9cUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r317 = { 0x3e0f759e39ff536dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r316 = { 0xbe5db60bccdb55a9UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r315 = { 0x3e7279e980191ae8UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r314 = { 0x3e2eff5b81ce2984UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r313 = { 0xbea271f9ab95caefUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r312 = { 0x3ebff2cd5a91b895UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r311 = { 0xbec805e42e4cfb17UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r310 = { 0xbec38928c1188e4bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r39 = { 0x3efb2c29f7cf82d6UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r38 = { 0xbf156747aa7ba3d5UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r37 = { 0x3f2647f742f5b988UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r36 = { 0xbf3145464e50f205UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r35 = { 0x3f3490a4d107748fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r34 = { 0xbf32c7d5ef09173dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r33 = { 0x3f29aa489e4f9c92UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r32 = { 0xbf18de3cd2905ee2UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r31 = { 0x3efe9b5e8cff6455UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r30 = { 0x3feffff6f9f67e55UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r417 = { 0x3d987b4417eaf36dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r416 = { 0xbdc085b75720c6a6UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r415 = { 0x3dd78ecc9429a326UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r414 = { 0xbdeb70e185e567c1UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r413 = { 0x3dfd4cd2d3b0acb6UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r412 = { 0xbe0b6e867ec36d97UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r411 = { 0x3e162553731107f0UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r410 = { 0xbe1f3c344c6247c2UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r49 = { 0x3e234d95061910ebUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r48 = { 0xbe24bd9ec47f0fbfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r47 = { 0x3e232d1bda20cff8UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r46 = { 0xbe1e231ca9b2d22dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r45 = { 0x3e13c4f1db039226UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r44 = { 0xbe0516ddf2b5a8e2UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r43 = { 0x3df196e40460a88bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r42 = { 0xbdd589af770a029eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r41 = { 0x3db13af23e58ca63UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __derf_la___r40 = { 0x3fefffffffffc9e8UL };

__attribute__((always_inline))
inline int __internal_derf_nolut_cout (double *a, double *pres)
{
    int nRet = 0;
    double xin = *a;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, xa, res, cpoly;
    unsigned long sgn_x;
    double dR, dR2;
    xa.f = xin;
    sgn_x = xa.w & 0x8000000000000000UL;
    xa.w ^= sgn_x;
    if (xa.f < 2.5)
    {
        if (xa.f < 1.5)
        {
            dR2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (xa.f, xa.f, 0.0);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_la___c14.f, dR2, __derf_la___c13.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c12.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c11.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c10.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c9.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c8.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c7.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c6.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c5.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c4.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c3.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c2.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c1.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___c0.f);
            res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, xin, xin);
            *pres = res.f;
            return nRet;
        }
        else
        {
            dR2 = xa.f - 2.0;
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_la___r216.f, dR2, __derf_la___r215.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r214.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r213.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r212.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r211.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r210.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r29.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r28.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r27.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r26.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r25.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r24.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r23.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r22.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r21.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r20.f);
            res.f = cpoly.f;
            res.w ^= sgn_x;
            *pres = res.f;
            return nRet;
        }
    }
    else
    {
        if (xa.f < 4.0)
        {
            dR2 = xa.f - 3.25;
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_la___r318.f, dR2, __derf_la___r317.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r316.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r315.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r314.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r313.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r312.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r311.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r310.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r39.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r38.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r37.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r36.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r35.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r34.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r33.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r32.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r31.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r30.f);
            res.f = cpoly.f;
            res.w ^= sgn_x;
            *pres = res.f;
            return nRet;
        }
        else
        {
            dR = (xa.f > 6.0) ? 6.0 : xa.f;
            dR = (xa.w <= 0x7ff0000000000000UL) ? dR : xa.f;
            dR2 = dR - 5.0;
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__derf_la___r417.f, dR2, __derf_la___r416.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r415.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r414.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r413.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r412.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r411.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r410.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r49.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r48.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r47.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r46.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r45.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r44.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r43.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r42.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r41.f);
            cpoly.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (cpoly.f, dR2, __derf_la___r40.f);
            res.f = cpoly.f;
            res.w ^= sgn_x;
            *pres = res.f;
            return nRet;
        }
    }
    return nRet;
}

double __ocl_svml_erf_noLUT (double a)
{
    double va1;
    double vr1;
    double r;
    va1 = a;
    __internal_derf_nolut_cout (&va1, &vr1);
    r = vr1;
    return r;
}
