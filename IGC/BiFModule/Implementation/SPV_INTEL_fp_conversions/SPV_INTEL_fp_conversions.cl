/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

typedef char char1 __attribute__((ext_vector_type(1)));
typedef uchar uchar1 __attribute__((ext_vector_type(1)));

// concatenation of four int8 polynomials with a period of 255
#define LfsrPolynomial_b8v4 0x8E8E8E8E
static uint4 __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(int seed) {
    uint4 result;
    result.x = __builtin_IB_lfsr_b8v4(as_uint(seed), LfsrPolynomial_b8v4);
    result.y = __builtin_IB_lfsr_b8v4(result.x, LfsrPolynomial_b8v4);
    result.z = __builtin_IB_lfsr_b8v4(result.y, LfsrPolynomial_b8v4);
    result.w = __builtin_IB_lfsr_b8v4(result.z, LfsrPolynomial_b8v4);
    return result;
}
static uint2 __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(int seed) {
    uint2 result;
    result.x = __builtin_IB_lfsr_b8v4(as_uint(seed), LfsrPolynomial_b8v4);
    result.y = __builtin_IB_lfsr_b8v4(result.x, LfsrPolynomial_b8v4);
    return result;
}

// Concatenation of two maximal-length 16-bit polynomials.
#define LfsrPolynomial_b16v2 0xB400B400
static uint2 __builtin_IB_lfsr_helper_b16v2_to_b16v2x2(int seed)
{
    uint2 result;
    result.x = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    result.y = __builtin_IB_lfsr_b16v2(result.x, LfsrPolynomial_b16v2);
    return result;
}
static uint4 __builtin_IB_lfsr_helper_b16v2_to_b16v2x4(int seed)
{
    uint4 result;
    result.x = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    result.y = __builtin_IB_lfsr_b16v2(result.x, LfsrPolynomial_b16v2);
    result.z = __builtin_IB_lfsr_b16v2(result.y, LfsrPolynomial_b16v2);
    result.w = __builtin_IB_lfsr_b16v2(result.z, LfsrPolynomial_b16v2);
    return result;
}
static uint8 __builtin_IB_lfsr_helper_b16v2_to_b16v2x8(int seed)
{
    uint8 result;
    result.s0 = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    result.s1 = __builtin_IB_lfsr_b16v2(result.s0, LfsrPolynomial_b16v2);
    result.s2 = __builtin_IB_lfsr_b16v2(result.s1, LfsrPolynomial_b16v2);
    result.s3 = __builtin_IB_lfsr_b16v2(result.s2, LfsrPolynomial_b16v2);
    result.s4 = __builtin_IB_lfsr_b16v2(result.s3, LfsrPolynomial_b16v2);
    result.s5 = __builtin_IB_lfsr_b16v2(result.s4, LfsrPolynomial_b16v2);
    result.s6 = __builtin_IB_lfsr_b16v2(result.s5, LfsrPolynomial_b16v2);
    result.s7 = __builtin_IB_lfsr_b16v2(result.s6, LfsrPolynomial_b16v2);
    return result;
}

#include "upconversions_fp4.cl"
#include "downconversions_fp4.cl"
#include "StochasticRound_fp4.cl"
#include "StochasticRound_fp16.cl"

#include "conversions_fp8.cl"
#include "StochasticRound_fp8.cl"
