/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// StochasticRoundFP16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half2 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half3 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in1 = (half2)(a.s2, 0);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half4 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half8 a, int seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half16 a, int seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_hf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_hf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// StochasticRoundFP16ToE2M1INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  *output_seed = new_seed;
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half2 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  *output_seed = new_seed;
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half3 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half4 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half8 a, int seed, private int *output_seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  *output_seed = new_seed.s1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(half16 a, int seed, private int *output_seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_hf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_hf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  *output_seed = new_seed.s3;
  return as_uchar8(result_uint2);
}

// StochasticRoundFP16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half2 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half3 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in1 = (half2)(a.s2, 0);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half4 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half8 a, int seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half16 a, int seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_hf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_hf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// StochasticRoundFP16ToInt4INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  *output_seed = new_seed;
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half2 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  *output_seed = new_seed;
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half3 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  half2 in1 = (half2)(a.s2, 0);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half4 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half8 a, int seed, private int *output_seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  *output_seed = new_seed.s1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundFP16ToInt4INTEL(half16 a, int seed, private int *output_seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_hf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_hf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  *output_seed = new_seed.s3;
  return as_uchar8(result_uint2);
}

// StochasticRoundBF16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short2 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short3 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short4 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short8 a, int seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short16 a, int seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_bf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_bf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// StochasticRoundBF16ToE2M1INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  *output_seed = new_seed;
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short2 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  *output_seed = new_seed;
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short3 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short4 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short8 a, int seed, private int *output_seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  *output_seed = new_seed.s1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(short16 a, int seed, private int *output_seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_bf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_bf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  *output_seed = new_seed.s3;
  return as_uchar8(result_uint2);
}

// StochasticRoundBF16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short2 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short3 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short4 a, int seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short8 a, int seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short16 a, int seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_bf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_bf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// StochasticRoundBF16ToInt4INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(in0), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  *output_seed = new_seed;
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short2 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a), in1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  *output_seed = new_seed;
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short3 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(as_uint(a.s01), as_uint(in1), new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short4 a, int seed, private int *output_seed) {
  uint new_seed = __builtin_IB_lfsr_b8v4(seed, LfsrPolynomial_b8v4);
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s1, new_seed, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  *output_seed = new_seed;
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short8 a, int seed, private int *output_seed) {
  uint2 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x2(seed);
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  *output_seed = new_seed.s1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_StochasticRoundBF16ToInt4INTEL(short16 a, int seed, private int *output_seed) {
  uint4 new_seed = __builtin_IB_lfsr_helper_b8v4_to_b8v4x4(seed);
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16_srnd(in.s0, in.s2, new_seed.s0, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16_srnd(in.s1, in.s3, new_seed.s1, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_bf16_srnd(in.s4, in.s6, new_seed.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_bf16_srnd(in.s5, in.s7, new_seed.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  *output_seed = new_seed.s3;
  return as_uchar8(result_uint2);
}

//
// Clamp wrappers (for int4/fp4 clamp and regular variants are the same)
//

// ClampStochasticRoundFP16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half2 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half3 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half4 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half8 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half16 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed);
}

// ClampStochasticRoundFP16ToE2M1INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half2 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half3 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half4 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half8 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToE2M1INTEL(half16 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToE2M1INTEL(a, seed, output_seed);
}

// ClampStochasticRoundFP16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half2 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half3 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half4 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half8 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half16 a, int seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed);
}

// ClampStochasticRoundFP16ToInt4INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half2 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half3 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half4 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half8 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundFP16ToInt4INTEL(half16 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundFP16ToInt4INTEL(a, seed, output_seed);
}

// ClampStochasticRoundBF16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short2 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short3 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short4 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short8 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short16 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed);
}

// ClampStochasticRoundBF16ToE2M1INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short2 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short3 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short4 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short8 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToE2M1INTEL(short16 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToE2M1INTEL(a, seed, output_seed);
}

// ClampStochasticRoundBF16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short2 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short3 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short4 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short8 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short16 a, int seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed);
}

// ClampStochasticRoundBF16ToInt4INTEL + output seed pointer
uchar __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
uchar1 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short2 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short3 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
uchar2 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short4 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
uchar4 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short8 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
uchar8 __attribute__((overloadable)) __builtin_spirv_ClampStochasticRoundBF16ToInt4INTEL(short16 a, int seed, private int *output_seed) {
  return __builtin_spirv_StochasticRoundBF16ToInt4INTEL(a, seed, output_seed);
}
