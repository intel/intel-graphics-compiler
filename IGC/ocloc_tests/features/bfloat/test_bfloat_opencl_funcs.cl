/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if bfloat functions used internally by validation compile to
// vISA instructions correctly.

// REQUIRES: llvm-spirv,cri-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA-NOT: faddr

#define DECLARE_VECTOR_2ARG(funcName, resType, src0Type, src1Type, N) \
resType##N funcName (src0Type##N, src1Type##N) __attribute__((overloadable));

#define DECLARE_VECTOR_3ARG(funcName, resType, src0Type, src1Type, src2Type, N) \
resType##N funcName (src0Type##N, src1Type##N, src2Type##N) __attribute__((overloadable));

#define DECLARE_2ARG(funcName, resType, src0Type, src1Type) \
resType funcName (src0Type, src1Type) __attribute__((overloadable)); \
DECLARE_VECTOR_2ARG(funcName, resType, src0Type, src1Type, 2) \
DECLARE_VECTOR_2ARG(funcName, resType, src0Type, src1Type, 4) \
DECLARE_VECTOR_2ARG(funcName, resType, src0Type, src1Type, 8) \
DECLARE_VECTOR_2ARG(funcName, resType, src0Type, src1Type, 16)

#define DECLARE_3ARG(funcName, resType, src0Type, src1Type, src2Type) \
resType funcName (src0Type, src1Type, src2Type) __attribute__((overloadable)); \
DECLARE_VECTOR_3ARG(funcName, resType, src0Type, src1Type, src2Type, 2) \
DECLARE_VECTOR_3ARG(funcName, resType, src0Type, src1Type, src2Type, 4) \
DECLARE_VECTOR_3ARG(funcName, resType, src0Type, src1Type, src2Type, 8) \
DECLARE_VECTOR_3ARG(funcName, resType, src0Type, src1Type, src2Type, 16)

DECLARE_2ARG(__builtin_bf16_isequal, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_isgreater, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_isless, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_isnotequal, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_islessequal, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_isgreaterequal, int, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_isunordered, int, ushort, ushort)

DECLARE_3ARG(__builtin_bf16_select, ushort, short, ushort, ushort)
DECLARE_3ARG(__builtin_bf16_select, ushort, int, ushort, ushort)

DECLARE_2ARG(__builtin_bf16_min, ushort, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_max, ushort, ushort, ushort)

DECLARE_2ARG(__builtin_bf16_add, ushort, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_add, ushort, float, ushort)
DECLARE_2ARG(__builtin_bf16_add, ushort, ushort, float)
DECLARE_2ARG(__builtin_bf16_addf, float, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_addf, float, float, ushort)
DECLARE_2ARG(__builtin_bf16_addf, float, ushort, float)

DECLARE_2ARG(__builtin_bf16_sub, ushort, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_sub, ushort, float, ushort)
DECLARE_2ARG(__builtin_bf16_sub, ushort, ushort, float)
DECLARE_2ARG(__builtin_bf16_subf, float, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_subf, float, float, ushort)
DECLARE_2ARG(__builtin_bf16_subf, float, ushort, float)

DECLARE_2ARG(__builtin_bf16_mul, ushort, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_mul, ushort, float, ushort)
DECLARE_2ARG(__builtin_bf16_mul, ushort, ushort, float)
DECLARE_2ARG(__builtin_bf16_mulf, float, ushort, ushort)
DECLARE_2ARG(__builtin_bf16_mulf, float, float, ushort)
DECLARE_2ARG(__builtin_bf16_mulf, float, ushort, float)

DECLARE_3ARG(__builtin_bf16_mad, ushort, ushort, ushort, ushort)
DECLARE_3ARG(__builtin_bf16_mad, ushort, float, ushort, ushort)
DECLARE_3ARG(__builtin_bf16_mad, ushort, ushort, ushort, float)
DECLARE_3ARG(__builtin_bf16_madf, float, ushort, ushort, ushort)
DECLARE_3ARG(__builtin_bf16_madf, float, float, ushort, ushort)
DECLARE_3ARG(__builtin_bf16_madf, float, ushort, ushort, float)


#define TEST_2ARG(testSuffix, builtinName, resType, src0Type, src1Type)                                        \
kernel void test_##testSuffix (                                 \
  global resType* out1, src0Type v1_1, src1Type v2_1,                    \
  global resType##2* out2, src0Type##2 v1_2, src1Type##2 v2_2,                 \
  global resType##4* out4, src0Type##4 v1_4, src1Type##4 v2_4,                 \
  global resType##8* out8, src0Type##8 v1_8, src1Type##8 v2_8,                 \
  global resType##16* out16, src0Type##16 v1_16, src1Type##16 v2_16            \
  ) {                                                            \
  out1[0] = __builtin_bf16_##builtinName(v1_1, v2_1);          \
  out2[1] = __builtin_bf16_##builtinName(v1_2, v2_2);                \
  out4[2] = __builtin_bf16_##builtinName(v1_4, v2_4);                \
  out8[3] = __builtin_bf16_##builtinName(v1_8, v2_8);                \
  out16[4] = __builtin_bf16_##builtinName(v1_16, v2_16);             \
}

#define TEST_3ARG(testSuffix, builtinName, resType, src0Type, src1Type, src2Type)                                        \
kernel void test_##testSuffix (                                 \
  global resType* out1, src0Type v1_1, src1Type v2_1, src2Type v3_1,                    \
  global resType##2* out2, src0Type##2 v1_2, src1Type##2 v2_2, src2Type##2 v3_2,                 \
  global resType##4* out4, src0Type##4 v1_4, src1Type##4 v2_4, src2Type##4 v3_4,                 \
  global resType##8* out8, src0Type##8 v1_8, src1Type##8 v2_8, src2Type##8 v3_8,                \
  global resType##16* out16, src0Type##16 v1_16, src1Type##16 v2_16, src2Type##16 v3_16            \
  ) {                                                            \
  out1[0] = __builtin_bf16_##builtinName(v1_1, v2_1, v3_1);          \
  out2[1] = __builtin_bf16_##builtinName(v1_2, v2_2, v3_2);                \
  out4[2] = __builtin_bf16_##builtinName(v1_4, v2_4, v3_4);                \
  out8[3] = __builtin_bf16_##builtinName(v1_8, v2_8, v3_8);                \
  out16[4] = __builtin_bf16_##builtinName(v1_16, v2_16, v3_16);             \
}


TEST_2ARG(isequal, isequal, int, ushort, ushort)
TEST_2ARG(isgreater, isgreater, int, ushort, ushort)
TEST_2ARG(isless, isless, int, ushort, ushort)
TEST_2ARG(isnotequal, isnotequal, int, ushort, ushort)
TEST_2ARG(islessequal, islessequal, int, ushort, ushort)
TEST_2ARG(isgreaterequal, isgreaterequal, int, ushort, ushort)
TEST_2ARG(isunordered, isunordered, int, ushort, ushort)

TEST_2ARG(min, min, ushort, ushort, ushort)
TEST_2ARG(max, max, ushort, ushort, ushort)

kernel void test_select (
  global ushort* out1, ushort v1_1, ushort v2_1,
  global ushort2* out2, ushort2 v1_2, ushort2 v2_2,
  global ushort4* out4, ushort4 v1_4, ushort4 v2_4,
  global ushort8* out8, ushort8 v1_8, ushort8 v2_8,
  global ushort16* out16, short16 c16, ushort16 v1_16, ushort16 v2_16,
  short c1s, short2 c2s, short4 c4s, short8 c8s, short16 c16s,
  int c1i, int2 c2i, int4 c4i, int8 c8i, int16 c16i
  ) {
  out1[0] = __builtin_bf16_select(c1s, v1_1, v2_1);
  out2[1] = __builtin_bf16_select(c2s, v1_2, v2_2);
  out4[2] = __builtin_bf16_select(c4s, v1_4, v2_4);
  out8[3] = __builtin_bf16_select(c8s, v1_8, v2_8);
  out16[4] = __builtin_bf16_select(c16s, v1_16, v2_16);

  out1[5] = __builtin_bf16_select(c1i, v1_1, v2_1);
  out2[6] = __builtin_bf16_select(c2i, v1_2, v2_2);
  out4[7] = __builtin_bf16_select(c4i, v1_4, v2_4);
  out8[8] = __builtin_bf16_select(c8i, v1_8, v2_8);
  out16[9] = __builtin_bf16_select(c16i, v1_16, v2_16);
}

TEST_2ARG(add_v1, add, ushort, ushort, ushort)
TEST_2ARG(add_v2, add, ushort, float, ushort)
TEST_2ARG(add_v3, add, ushort, ushort, float)
TEST_2ARG(addf_v1, addf, float, ushort, ushort)
TEST_2ARG(addf_v2, addf, float, ushort, float)
TEST_2ARG(addf_v3, addf, float, float, ushort)


TEST_2ARG(sub_v1, sub, ushort, ushort, ushort)
TEST_2ARG(sub_v2, sub, ushort, float, ushort)
TEST_2ARG(sub_v3, sub, ushort, ushort, float)
TEST_2ARG(subf_v1, subf, float, ushort, ushort)
TEST_2ARG(subf_v2, subf, float, ushort, float)
TEST_2ARG(subf_v3, subf, float, float, ushort)



TEST_2ARG(mul_v1, mul, ushort, ushort, ushort)
TEST_2ARG(mul_v2, mul, ushort, float, ushort)
TEST_2ARG(mul_v3, mul, ushort, ushort, float)
TEST_2ARG(mulf_v1, mulf, float, ushort, ushort)
TEST_2ARG(mulf_v2, mulf, float, ushort, float)
TEST_2ARG(mulf_v3, mulf, float, float, ushort)

TEST_3ARG(mad_v1, mad, ushort, ushort, ushort, ushort)
TEST_3ARG(mad_v2, mad, ushort, float, ushort, ushort)
TEST_3ARG(mad_v3, mad, ushort, ushort, ushort, float)
TEST_3ARG(madf_v1, madf, float, ushort, ushort, ushort)
TEST_3ARG(madf_v2, madf, float, float, ushort, ushort)
TEST_3ARG(madf_v3, madf, float, ushort, ushort, float)

