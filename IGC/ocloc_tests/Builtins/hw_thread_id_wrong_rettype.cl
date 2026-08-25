/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: not ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK: error{{.*}}Incorrect declaration of a __builtin_IB_{{.*}} function

long __builtin_IB_hw_thread_id(void);
__kernel void wrongkernel(long a, __global long *output) {
  long r = __builtin_IB_hw_thread_id();
  output[0] = r * a + r;
}
