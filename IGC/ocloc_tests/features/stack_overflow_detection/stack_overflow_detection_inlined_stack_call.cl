/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test contains a function call which will be inlined.
// Therefore this kernel won't contain any stack calls.
// This tests checks that calls to stack overflow detection subroutines
// are not present in generated vISA.

// 32bit unsupported due to issues on windows 32bit build, to be debugged.
// UNSUPPORTED: sys32
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1,StackOverflowDetection=1'" -device dg2 | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA: .kernel "test_simple"
// CHECK-VISA-NOT: call (M1, {{(8)|(16)}}) __stackoverflow_init{{.*}}
// CHECK-VISA-NOT: call (M1, {{(8)|(16)}}) __stackoverflow_detection{{.*}}

int pow5(int n) {
  return n*n*n*n*n;
}
kernel void test_simple(global int* out, int n) {
  out[0] = pow5(n);
}
