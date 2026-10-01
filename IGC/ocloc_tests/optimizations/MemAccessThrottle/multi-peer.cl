/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A ring step that writes to two peers, as on 4 and 8 cards. Both stores reach
// the same reconvergence point, so they share one throttle: the rate is set by
// the work-group, and two pauses per iteration would halve it for no reason.

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s

// Three stores, then one throttle after the join.
// CHECK: store.ugm.d32x4.a64
// CHECK: store.ugm.d32x4.a64
// CHECK: store.ugm.d32x4.a64
// CHECK: join
// CHECK-COUNT-1: tm0.4<1>:ud
// CHECK-NOT: tm0.4<1>:ud

__attribute__((intel_reqd_sub_group_size(16))) kernel void
reduce_copy_two_peers(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *peerA,
                      global float4 *peerB, ulong count) {
  const size_t lid = get_local_id(0);
  for (size_t base = get_group_id(0) * get_local_size(0); base < count; base += get_global_size(0)) {
    const size_t idx = base + lid;
    if (idx < count) {
      out1[idx] = in1[idx] + in2[idx];
      peerA[idx] = ((volatile global float4 *)out1)[idx];
      peerB[idx] = ((volatile global float4 *)out1)[idx];
    }
  }
}
