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

#include "upconversions_fp4.cl"
#include "downconversions_fp4.cl"
#include "StochasticRound_fp4.cl"

#include "conversions_fp8.cl"
#include "StochasticRound_fp8.cl"
