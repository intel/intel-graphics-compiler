/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if bfloat functions used internally by validation compile to
// vISA instructions correctly.

// REQUIRES: llvm-spirv,cri-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

ushort2 __builtin_bf16_sin(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_cos(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_log(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_exp(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_sqrt(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_tanh(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_sigm(ushort2) __attribute__((overloadable));
ushort2 __builtin_bf16_inv(ushort2) __attribute__((overloadable));

kernel void test_math_vector(global ushort2* out) {
  int lid = get_local_id(0);
  ushort2 v1 = out[lid];
// CHECK-VISA-DAG: sin (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1:.*]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: .decl [[SRC1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: sin (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2:.*]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: .decl [[SRC2]] v_type=G type=bf num_elts=32
  out[lid] = __builtin_bf16_sin(v1);
// CHECK-VISA-DAG: cos (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: cos (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+1] = __builtin_bf16_cos(v1);
// CHECK-VISA-DAG: log (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: log (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+2] = __builtin_bf16_log(v1);
// CHECK-VISA-DAG: exp (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: exp (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+3] = __builtin_bf16_exp(v1);
// CHECK-VISA-DAG: sqrt (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: sqrt (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+4] = __builtin_bf16_sqrt(v1);
// CHECK-VISA-DAG: tanh (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: tanh (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+5] = __builtin_bf16_tanh(v1);
// CHECK-VISA-DAG: sigm (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: sigm (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+6] = __builtin_bf16_sigm(v1);
// CHECK-VISA-DAG: inv (M1, 32) [[RES1:.*]](0,0)<1> [[SRC1]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES1]] v_type=G type=bf num_elts=32
// CHECK-VISA-DAG: inv (M1, 32) [[RES2:.*]](0,0)<1> [[SRC2]](0,0)<1;1,0>
// CHECK-VISA-DAG: .decl [[RES2]] v_type=G type=bf num_elts=32
  out[lid+7] = __builtin_bf16_inv(v1);
}
