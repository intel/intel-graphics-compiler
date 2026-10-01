/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The in-kernel throttle from the CRI peer2peer write guide, applied by hand.
// The pass must leave it alone: exactly one write of the pause register, the
// one written here.
//
// The reload is volatile, as in the benchmark. Without that the stored value is
// forwarded, no pattern is left to match, and the test would pass without ever
// reaching the decision to stand aside.

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s

// CHECK-COUNT-1: tm0.4<1>:ud
// CHECK-NOT: tm0.4<1>:ud

// Separate run: the warning says the pattern was matched and then declined, not
// that it was never found. Its own prefix, so nothing depends on how the
// diagnostic and the assembly interleave.
// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-WARN

// CHECK-WARN: mem-access-throttle: kernel already throttles itself

#define MAX_EU_THREAD_PAUSE 32

void __builtin_IB_eu_thread_pause(uint value);

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy_throttled(global const float4 *in1,
                                                                                 global const float4 *in2,
                                                                                 global float4 *out1,
                                                                                 global float4 *out2, ulong count) {
  const size_t lid = get_local_id(0);
  for (size_t base = get_group_id(0) * get_local_size(0); base < count; base += get_global_size(0)) {
    const size_t idx = base + lid;
    if (idx < count) {
      out1[idx] = in1[idx] + in2[idx];
      out2[idx] = ((volatile global float4 *)out1)[idx];
    }
    if (lid == 0) {
      __builtin_IB_eu_thread_pause(MAX_EU_THREAD_PAUSE);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
}
