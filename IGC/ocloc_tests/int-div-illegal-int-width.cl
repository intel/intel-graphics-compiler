/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// IndVarSimplify narrows the inner counter to i3, so the division reaches
// IntDivConstantReduction as udiv i3. It used to assert with "invalid bitsize".

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-cl-std=CL3.0" -device cri 2>&1 | FileCheck %s

// CHECK-NOT: invalid bitsize
// CHECK: Build succeeded

kernel void test(global uint *out, int m) {
  for (int k = 0; k < m; ++k) {
    uint n = (uint)(3 * k + 3) & 7;
    uint i = 0;
    for (; i < n; ++i)
      ;
    out[k] = i / 3u;
  }
}
