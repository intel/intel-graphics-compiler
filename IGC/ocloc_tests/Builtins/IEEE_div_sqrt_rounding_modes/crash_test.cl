/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-cl-fp32-correctly-rounded-divide-sqrt -cl-std=CL3.0" \
// RUN: | FileCheck %s

// CHECK-NOT: basic_string: construction from null is not valid
// CHECK-NOT: Build failed with error code: -11

kernel void foo(global float* a, global float* b, global float* c) {
  int id = get_global_id(0);
  c[id] = a[id] / b[id];
}
