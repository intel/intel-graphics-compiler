/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if calls to stack overflow detection subroutines are present
// in the generated vISA for a kernel with stack calls.

// windows unsupported due to issues on 32bit build, to be debugged.
// UNSUPPORTED: system-windows
// REQUIRES: regkeys

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1,StackOverflowDetection=1'" -device dg2 | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA: .kernel "test_simple"
// CHECK-VISA: call (M1, {{(8)|(16)}}) __stackoverflow_init{{.*}}

// CHECK-VISA: .global_function "fact"
// CHECK-VISA: call (M1, {{(8)|(16)}}) __stackoverflow_detection

int fact(int n) {
  return n < 2 ? 1 : n*fact(n-1);
}
kernel void test_simple(global int* out, int n) {
  out[0] = fact(n);
}
