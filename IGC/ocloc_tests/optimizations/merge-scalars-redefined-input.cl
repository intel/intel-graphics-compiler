/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The loop-carried values of a and b live in the registers of their kernel
// arguments, so both inputs are redefined in the loop body before they are
// read into the vector. vISA's MergeScalars must not merge those two reads
// into a single read of a new input variable: the redefinitions' def-use
// edges would then point at an unrelated declare, which the G4 verifier in
// debug builds rejects with "G4Verification failure".

// REQUIRES: debug, dg2-supported

// RUN: ocloc compile -file %s -device dg2 2>&1 | FileCheck %s

// CHECK-NOT: G4Verification failure
// CHECK: Build succeeded

kernel void test(global int2 *out, int a, int b, int n, int c, int d) {
  for (int i = 0; i < n; i++) {
    a += c * i;
    b += d;
    out[i] = (int2)(a, b);
  }
}
