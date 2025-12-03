/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file needs to be included with externally defined ADDRSPACE_NAME
//

// StochasticRoundFP16ToE4M3INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_hftohf8_1(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_hftohf8_2(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_hftohf8_3(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_hftohf8_4(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_hftohf8_8(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE4M3INTEL(half16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_hftohf8_16(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// StochasticRoundFP16ToE5M2INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_hftobf8_1(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_hftobf8_2(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_hftobf8_3(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_hftobf8_4(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_hftobf8_8(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE5M2INTEL(half16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_hftobf8_16(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// ClampStochasticRoundFP16ToE4M3INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_hftohf8_1_sat(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_hftohf8_2_sat(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_hftohf8_3_sat(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_hftohf8_4_sat(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_hftohf8_8_sat(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE4M3INTEL(half16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_hftohf8_16_sat(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// ClampStochasticRoundFP16ToE5M2INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_hftobf8_1_sat(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_hftobf8_2_sat(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_hftobf8_3_sat(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_hftobf8_4_sat(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_hftobf8_8_sat(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE5M2INTEL(half16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_hftobf8_16_sat(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// StochasticRoundBF16ToE5M2INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_bftobf8_1(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_bftobf8_2(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_bftobf8_3(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_bftobf8_4(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_bftobf8_8(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE5M2INTEL(short16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_bftobf8_16(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// StochasticRoundBF16ToE4M3INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_bftohf8_1(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_bftohf8_2(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_bftohf8_3(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_bftohf8_4(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_bftohf8_8(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE4M3INTEL(short16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_bftohf8_16(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// ClampStochasticRoundBF16ToE5M2INTEL without output pointer
char __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_bftobf8_1_sat(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_bftobf8_2_sat(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_bftobf8_3_sat(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_bftobf8_4_sat(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_bftobf8_8_sat(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL(short16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_bftobf8_16_sat(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}

// ClampStochasticRoundBF16ToE4M3INTEL with output pointer
char __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char result = __builtin_IB_srnd_bftohf8_1_sat(a, as_char4(new_seed).x);
    *new_seed_ptr = new_seed;
    return result;
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short2 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char2 result = __builtin_IB_srnd_bftohf8_2_sat(a, as_char4(new_seed).xy);
    *new_seed_ptr = new_seed;
    return result;
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short3 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char3 result = __builtin_IB_srnd_bftohf8_3_sat(a, as_char4(new_seed).xyz);
    *new_seed_ptr = new_seed;
    return result;
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short4 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    int new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
    char4 result = __builtin_IB_srnd_bftohf8_4_sat(a, as_char4(new_seed));
    *new_seed_ptr = new_seed;
    return result;
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short8 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
    char8 result = __builtin_IB_srnd_bftohf8_8_sat(a, as_char8(new_seed));
    *new_seed_ptr = new_seed.y;
    return result;
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL(short16 a, int seed, ADDRSPACE_NAME int *new_seed_ptr) {
    uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
    char16 result = __builtin_IB_srnd_bftohf8_16_sat(a, as_char16(new_seed));
    *new_seed_ptr = new_seed.w;
    return result;
}
