/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: not ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK: error{{.*}}Incorrect use of an image/sampler __builtin_IB_{{.*}} function

__global int *__builtin_IB_get_image_width(long h);
__kernel void wrongkernel(long h, __global int *dummy) {
  __global int *p = __builtin_IB_get_image_width(h);
  p[0] = 42;
}
