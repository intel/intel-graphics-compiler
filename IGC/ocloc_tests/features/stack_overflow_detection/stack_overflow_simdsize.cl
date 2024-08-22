/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if calls to stack overflow detection subroutines don't change kernel SIMD size.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1,StackOverflowDetection=1'" -device pvc | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA: .kernel "test_nosubroutines"
// CHECK-VISA-NOT: .function
// CHECK-VISA: .kernel_attr SimdSize=32

// CHECK-VISA: .kernel "test_subroutines"
// CHECK-VISA-NOT: .function
// CHECK-VISA: .kernel_attr SimdSize=16

int fact(int n) {
  return n < 2 ? 1 : n*fact(n-1);
}

kernel void test_subroutines(global int* out, int n) {
  out[0] = fact(n);
}

kernel void test_nosubroutines(global int* out, int n) {
  out[0] = n;
}
