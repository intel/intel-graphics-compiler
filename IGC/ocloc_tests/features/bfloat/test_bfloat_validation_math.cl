/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if bfloat functions used internally by validation compile to
// vISA instructions correctly.

// REQUIRES: llvm-spirv,cri-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

ushort __builtin_bf16_sin(ushort) __attribute__((overloadable));
ushort __builtin_bf16_cos(ushort) __attribute__((overloadable));
ushort __builtin_bf16_log(ushort) __attribute__((overloadable));
ushort __builtin_bf16_exp(ushort) __attribute__((overloadable));
ushort __builtin_bf16_sqrt(ushort) __attribute__((overloadable));
ushort __builtin_bf16_tanh(ushort) __attribute__((overloadable));
ushort __builtin_bf16_sigm(ushort) __attribute__((overloadable));
ushort __builtin_bf16_inv(ushort) __attribute__((overloadable));

kernel void test_math(global ushort* out, ushort v1) {
// CHECK-VISA-DAG: sin (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC:.*]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
// CHECK-VISA-DAG: .decl [[SRC]] v_type=G type=bf
  out[0] = __builtin_bf16_sin(v1);
// CHECK-VISA-DAG: cos (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[1] = __builtin_bf16_cos(v1);
// CHECK-VISA-DAG: log (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[2] = __builtin_bf16_log(v1);
// CHECK-VISA-DAG: exp (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[3] = __builtin_bf16_exp(v1);
// CHECK-VISA-DAG: sqrt (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[4] = __builtin_bf16_sqrt(v1);
// CHECK-VISA-DAG: tanh (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[5] = __builtin_bf16_tanh(v1);
// CHECK-VISA-DAG: sigm (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[6] = __builtin_bf16_sigm(v1);
// CHECK-VISA-DAG: inv (M1_NM, 1) [[RES:.*]](0,0)<1> [[SRC]](0,0)<0;1,0>
// CHECK-VISA-DAG: .decl [[RES]] v_type=G type=bf
  out[7] = __builtin_bf16_inv(v1);
}
