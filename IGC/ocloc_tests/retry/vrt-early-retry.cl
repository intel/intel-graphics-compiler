/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit this
software or the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.


============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported
// UNSUPPORTED: release

// Budget scaled down to 1% of the Xe3P top budget (512 GRFs) -> 5 GRFs, which
// this kernel is comfortably past, so the first compilation stage is skipped.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'VRTEarlyRetryBudgetPercent=1'" \
// RUN: | FileCheck %s --check-prefix=EARLY-RETRY

// Full top budget: the kernel is nowhere near 512 GRFs, so no early retry, even
// though the legacy thresholds are lowered enough that they would have fired.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'VRTEarlyRetryBudgetPercent=100,EarlyRetryDefaultGRFThreshold=3,EarlyRetryLargeGRFThreshold=3'" \
// RUN: | FileCheck %s --check-prefix=NO-EARLY-RETRY

// VRT path switched off the static thresholds are back in charge and
// the same kernel retries early again.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'EnableVRTEarlyRetry=0,EarlyRetryDefaultGRFThreshold=3,EarlyRetryLargeGRFThreshold=3'" \
// RUN: | FileCheck %s --check-prefix=EARLY-RETRY

// EARLY-RETRY: [RetryManager] Start recompilation of the kernel
// NO-EARLY-RETRY-NOT: [RetryManager] Start recompilation of the kernel

// More than one basic block, so the single-BB early-out does not apply.
__kernel void vrt_early_retry_example(__global float *input, __global float *output, int N) {
  for (int i = 0; i < N; ++i) {
    int gid = get_global_id(0) + i;
    float a = input[gid];
    float b = a * 2.0f;
    float c = a + b;
    float d = c * 4.0f;
    float e = d - b;
    float f = e / 6.0f;
    float g = f + d;
    float h = g * 8.0f;
    float k = h - f;
    float l = k / 10.0f;
    output[gid] += a + b + c + d + e + f + g + h + k + l;
  }
}
