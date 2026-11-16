/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The any() of a vector compare carried out of the loop leaves a <4 x i1> phi
// at the loop exit; as one predicate per lane it overflows the vISA flags
// ("number of flags must be <= 32").

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-cl-std=CL3.0" -device cri 2>&1 | FileCheck %s

// CHECK-NOT: number of flags
// CHECK: Build succeeded

kernel void test(global float4 *in, global int *out, int n) {
  size_t gid = get_global_id(0);
  float4 a = in[gid], acc = a;
  int4 c;
  for (int i = 0;; ++i) {
    c = isgreaterequal(a, acc);
    if (i >= n)
      break;
    acc += a;
  }
  out[gid] = any(c);
}
