/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A chunked ring step: an outer loop over chunks, a grid-stride loop inside.
// The throttle belongs at the inner latch, so it paces every batch of peer
// writes. At the outer latch it would fire once per chunk and throttle nothing.

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s

// Two back edges follow the barrier, the inner loop's and then the outer one.
// Hoisted to the outer latch there would be only the outer back edge left, so
// the second jmpi is what pins the throttle to the inner loop. Block names are
// deliberately not matched, they move with unrelated changes.
// CHECK: store.ugm.d32x4.a64
// CHECK: store.ugm.d32x4.a64
// CHECK: join
// CHECK: mov (1|M0) tm0.4<1>:ud 0x3E0:uw
// CHECK: signal barrier
// CHECK: sync.bar
// CHECK: jmpi
// CHECK: jmpi
// CHECK-NOT: tm0.4<1>:ud

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy_chunked(global const float4 *in1,
                                                                               global const float4 *in2,
                                                                               global float4 *out1, global float4 *out2,
                                                                               ulong count, ulong chunk) {
  const size_t lid = get_local_id(0);
  for (size_t c = 0; c < count; c += chunk) {
    for (size_t base = c + get_group_id(0) * get_local_size(0); base < c + chunk; base += get_global_size(0)) {
      const size_t idx = base + lid;
      if (idx < count) {
        out1[idx] = in1[idx] + in2[idx];
        out2[idx] = ((volatile global float4 *)out1)[idx];
      }
    }
  }
}
