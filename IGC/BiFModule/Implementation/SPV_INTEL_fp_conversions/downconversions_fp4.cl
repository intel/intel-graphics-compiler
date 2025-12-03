/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ConvertFP16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half a) {
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(in0), in1, DNSCL_CONVERT_TO_E2M1, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half2 a) {
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(a), in1, DNSCL_CONVERT_TO_E2M1, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half3 a) {
  half2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(a.s01), as_uint(in1), DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half4 a) {
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16(in.s0, in.s1, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half8 a) {
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16(in.s0, in.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16(in.s1, in.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE2M1INTEL(half16 a) {
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16(in.s0, in.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_hf16(in.s1, in.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_hf16(in.s4, in.s6, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_hf16(in.s5, in.s7, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// ConvertFP16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half a) {
  half2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(in0), in1, DNSCL_CONVERT_TO_INT4, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half2 a) {
  uint in1;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(a), in1, DNSCL_CONVERT_TO_INT4, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half3 a) {
  half2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_hf16(as_uint(a.s01), as_uint(in1), DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half4 a) {
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_hf16(in.s0, in.s1, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half8 a) {
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_hf16(in.s0, in.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16(in.s1, in.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToInt4INTEL(half16 a) {
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_hf16(in.s0, in.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_hf16(in.s1, in.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_hf16(in.s4, in.s6, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_hf16(in.s5, in.s7, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// ConvertBF16ToE2M1INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short a) {
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(in0), in1, DNSCL_CONVERT_TO_E2M1, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short2 a) {
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(a), in1, DNSCL_CONVERT_TO_E2M1, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short3 a) {
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(a.s01), as_uint(in1), DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short4 a) {
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16(in.s0, in.s1, DNSCL_CONVERT_TO_E2M1, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short8 a) {
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16(in.s0, in.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16(in.s1, in.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE2M1INTEL(short16 a) {
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16(in.s0, in.s2, DNSCL_CONVERT_TO_E2M1, 0);
  uint part1 = __builtin_IB_dnscl_bf16(in.s1, in.s3, DNSCL_CONVERT_TO_E2M1, 2);
  uint part2 = __builtin_IB_dnscl_bf16(in.s4, in.s6, DNSCL_CONVERT_TO_E2M1, 0);
  uint part3 = __builtin_IB_dnscl_bf16(in.s5, in.s7, DNSCL_CONVERT_TO_E2M1, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}

// ConvertBF16ToInt4INTEL
uchar __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short a) {
  short2 in0;
  in0.s0 = a;
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(in0), in1, DNSCL_CONVERT_TO_INT4, 0);
  return as_uchar4(result_uint).x;
}

uchar1 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short2 a) {
  uint in1;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(a), in1, DNSCL_CONVERT_TO_INT4, 0);
  return (uchar1)(as_uchar4(result_uint).x);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short3 a) {
  short2 in1;
  in1.s0 = a.s2;
  uint result_uint = __builtin_IB_dnscl_bf16(as_uint(a.s01), as_uint(in1), DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short4 a) {
  uint2 in = as_uint2(a);
  uint result_uint = __builtin_IB_dnscl_bf16(in.s0, in.s1, DNSCL_CONVERT_TO_INT4, 0);
  uchar4 result_uchar = as_uchar4(result_uint);
  return (uchar2)(result_uchar.x, result_uchar.z);
}

uchar4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short8 a) {
  uint4 in = as_uint4(a);
  uint part0 = __builtin_IB_dnscl_bf16(in.s0, in.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16(in.s1, in.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint result_uint = part0 | part1;
  return as_uchar4(result_uint);
}

uchar8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToInt4INTEL(short16 a) {
  uint8 in = as_uint8(a);
  uint part0 = __builtin_IB_dnscl_bf16(in.s0, in.s2, DNSCL_CONVERT_TO_INT4, 0);
  uint part1 = __builtin_IB_dnscl_bf16(in.s1, in.s3, DNSCL_CONVERT_TO_INT4, 2);
  uint part2 = __builtin_IB_dnscl_bf16(in.s4, in.s6, DNSCL_CONVERT_TO_INT4, 0);
  uint part3 = __builtin_IB_dnscl_bf16(in.s5, in.s7, DNSCL_CONVERT_TO_INT4, 2);
  uint2 result_uint2 = (uint2)(part0 | part1, part2 | part3);
  return as_uchar8(result_uint2);
}
