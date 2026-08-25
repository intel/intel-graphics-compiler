/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: not ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK: error{{.*}}Incorrect declaration of a __builtin_IB_{{.*}} function

int __builtin_IB_native_sqrtf(float x);
__kernel void wrongkernel(int a, float x, __global int *output) {
  int r = __builtin_IB_native_sqrtf(x);
  output[0] = r * a + r;
}
