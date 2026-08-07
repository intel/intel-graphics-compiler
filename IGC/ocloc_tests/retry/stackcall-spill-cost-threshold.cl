/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test if stack call functions are retried based on a spill cost ratio.

// REQUIRES: regkeys, pvc-supported
// UNSUPPORTED: release

// RUN: ocloc compile -file %s -device pvc \
// RUN:   -options "-igc_opts 'EnableStackCallFuncCall=1, PrintStackCallDebugInfo=1, RetryStackCallSpillCostThreshold=1'" \
// RUN:   &> %t_retry.log
// RUN: FileCheck --input-file %t_retry.log %s --check-prefix=CHECK-RETRY

// CHECK-RETRY: Stack Function Spill Info:
// CHECK-RETRY: KERNEL: test
// CHECK-RETRY: STACK_FUNC Retry: heavy
// CHECK-RETRY-NEXT: numGRFSpill = {{[1-9][0-9]*}}
// CHECK-RETRY-NEXT: TotalInsts = {{[1-9][0-9]*}}
// CHECK-RETRY: [RetryManager] Start recompilation of the kernel

// RUN: ocloc compile -file %s -device pvc \
// RUN:   -options "-igc_opts 'EnableStackCallFuncCall=1, PrintStackCallDebugInfo=1, RetryStackCallSpillCostThreshold=100000'" \
// RUN:   &> %t_noretry.log
// RUN: FileCheck --input-file %t_noretry.log %s --check-prefix=CHECK-NO-RETRY

// CHECK-NO-RETRY-NOT: Stack Function Spill Info:
// CHECK-NO-RETRY-NOT: STACK_FUNC Retry:
// CHECK-NO-RETRY-NOT: [RetryManager] Start recompilation of the kernel

#define D 48

__attribute__((noinline)) double heavy(global double *p, int n) {
  double a[D];
  for (int i = 0; i < D; ++i)
    a[i] = p[i] * (double)i;
  for (int k = 0; k < n; ++k) {
    for (int i = 0; i < D; ++i)
      a[i] = a[i] * a[(i + 7) % D] + a[(i + 13) % D];
  }
  double s = 0;
  for (int i = 0; i < D; ++i)
    s += a[i];
  return s;
}

kernel void test(global double *p) { p[get_global_id(0)] = heavy(p, 3); }
