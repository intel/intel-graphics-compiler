/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (c) 2014 Advanced Micro Devices, Inc.

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
#include "tables.cl"
#include "math.h"

/* Refer to the exp routine for the underlying algorithm */

INLINE float libclc_expm1_f32(float x) {
    const float X_MAX = 0x1.62e42ep+6f; // 128*log2 : 88.722839111673
    const float X_MIN = -0x1.9d1da0p+6f; // -149*log2 : -103.27892990343184

    const float R_64_BY_LOG2 = 0x1.715476p+6f;     // 64/log2 : 92.332482616893657
    const float R_LOG2_BY_64_LD = 0x1.620000p-7f;  // log2/64 lead: 0.0108032227
    const float R_LOG2_BY_64_TL = 0x1.c85fdep-16f; // log2/64 tail: 0.0000272020388

    uint xi = as_uint(x);
    int n = (int)(x * R_64_BY_LOG2);
    float fn = (float)n;

    int j = n & 0x3f;
    int m = n >> 6;

    float r = mad(fn, -R_LOG2_BY_64_TL, mad(fn, -R_LOG2_BY_64_LD, x));

    // Truncated Taylor series
    float z2 = mad(r*r, mad(r, mad(r, 0x1.555556p-5f,  0x1.555556p-3f), 0.5f), r);

    float m2 = as_float((m + EXPBIAS_SP32) << EXPSHIFTBITS_SP32);
    float2 tv = USE_TABLE(exp_tbl_ep, j);

    float two_to_jby64_h = tv.s0 * m2;
    float two_to_jby64_t = tv.s1 * m2;
    float two_to_jby64 = two_to_jby64_h + two_to_jby64_t;

    z2 = mad(z2, two_to_jby64, two_to_jby64_t) + (two_to_jby64_h - 1.0f);
    //Make subnormals work
    z2 = x == 0.f ? x : z2;
    z2 = x < X_MIN | m < -24 ? -1.0f : z2;
    z2 = x > X_MAX ? as_float(PINFBITPATT_SP32) : z2;
    z2 = isnan(x) ? x : z2;

    return z2;
}
