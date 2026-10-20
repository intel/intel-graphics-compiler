/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, bmg-supported

// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-igc_opts 'PrintToConsole=1 PrintBefore=igc-scalarize'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s

// The fp64 ceil()/floor() BiF implementations used to finish with (+0.0) + trunc(x),
// which returns (+0.0) for (+0.0) + (-0.0) and loses a negative zero result. They are
// inlined and constant folded here, so the returned values are visible in the IR.

// CHECK-LABEL: define spir_kernel void @test
// CHECK: store double -0.000000e+00
// CHECK: store double -0.000000e+00
// CHECK: store double -0.000000e+00
// CHECK: store double -1.000000e+00
// CHECK: store double -1.000000e+00
// CHECK: store double 1.000000e+00
// CHECK: store double 0.000000e+00

kernel void test(global double *out) {
  out[0] = ceil(-0.5); // -0.0
  out[1] = ceil(-0.0); // -0.0
  out[2] = floor(-0.0); // -0.0
  out[3] = ceil(-1.5); // -1.0
  out[4] = floor(-0.5); // -1.0
  out[5] = ceil(0.5); //  1.0
  out[6] = floor(0.5); //  0.0
}
